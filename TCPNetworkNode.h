#pragma once

#include "SDL_net.h"
#include <iostream>
#include <string>
#include "Network.h"

#include <chrono>
#include <thread>
#include <mutex>
#include "ReaderWriterQueue.h"
#include "Containers.h"
#include <atomic>
#include <functional>
#include "SDLNetCpp.h"
#include "TCPConnection.h"
#include "TCPMessage.h"

class TCPNetworkNode {
protected:
	SocketSetObject socketSet;

	const int MAX_SOCKETS;
	const int SOCKET_TIMEOUT_DURATION{ 30 * 1000 };//60 seconds
	const int SEND_STILL_ALIVE_PERIOD{ 20 * 1000 };//period of time for sending "still alive" message

	//receiving buffers
	std::vector<UniqueByteBuffer> recvBuffers;
	std::vector<int> availableRecvBuffers;

	//sending buffer
	UniqueByteBuffer sendingBuffer;


public:
	TCPNetworkNode() : MAX_SOCKETS{ MAX_TCP_SOCKETS }, socketSet{ MAX_TCP_SOCKETS }, sendingBuffer(4) {
		assert(SOCKET_TIMEOUT_DURATION > SEND_STILL_ALIVE_PERIOD);
	}

	TCPNetworkNode(TCPNetworkNode const&) = delete;
	TCPNetworkNode(TCPNetworkNode &&) = delete;
	TCPNetworkNode& operator=(TCPNetworkNode const&) = delete;
	TCPNetworkNode& operator=(TCPNetworkNode&&) = delete;

	void keepConnectionAlive(TCPConnection& connection) {
		if (SDL_GetTicks() - connection.lastLocalActivityDate >= SEND_STILL_ALIVE_PERIOD) {
			Uint16 size = StillAlive::writeToBuffer(sendingBuffer);
			sendPacket(connection, size);
		}
	}
	inline bool isRemoteAlive(TCPConnection const& connection) const {
		return (SDL_GetTicks() - connection.lastRemoteActivityDate) <= SOCKET_TIMEOUT_DURATION;
	}

	int getAvailableRecvBufferIndex(unsigned int initSize) {
		assert(initSize >= 4);
		if (availableRecvBuffers.empty()) {
			recvBuffers.emplace_back(initSize);
			return recvBuffers.size() - 1;
		}
		else {
			int index = availableRecvBuffers.back();
			availableRecvBuffers.pop_back();
			return index;
		}
	}
	UniqueByteBuffer& getRecvBuffer(int index) {
		return recvBuffers[index];
	}
	void releaseRecvBuffer(TCPConnection& connection) {
		availableRecvBuffers.emplace_back(connection.recvBufferIndex);
		connection.recvBufferIndex = NO_BUFFER;
	}
	void try_releaseRecvBuffer(TCPConnection& connection) {
		if (connection.recvBufferIndex == NO_BUFFER) return;
		availableRecvBuffers.emplace_back(connection.recvBufferIndex);
		connection.recvBufferIndex = NO_BUFFER;
	}

	inline int sendPacket(TCPConnection& connection, int packetSize) const {
		connection.lastLocalActivityDate = SDL_GetTicks();
		return connection.tcpSocket.send(sendingBuffer.get(), packetSize);
	}

	//check if the sockets in the set are ready, and so a future call to receivePacket wont block
	//the number of sockets should be >= 1
	void checkSockets(Uint32 timeout = 0) {
		socketSet.checkSockets(timeout);	
	}

	//return true if the packet is fully received
	//throw if an error occured, if so the connection should be closed
	//TODO: make this method member of TCPConnection !
	bool receivePacket(TCPConnection& connection) {
		if (!connection.tcpSocket.isReady()) 
			return false;

		//acquire a buffer
		if (connection.bytesReceived == 0) {
			connection.recvBufferIndex = getAvailableRecvBufferIndex(INITIAL_TCP_BUFFER_SIZE);
		}
		UniqueByteBuffer& buffer{ getRecvBuffer(connection.recvBufferIndex) };

		int ret;
		//if we already know the packet size
		if (connection.bytesReceived >= 2) {
			ret = connection.tcpSocket.recv(buffer.get() + connection.bytesReceived, 
				connection.packetSize - connection.bytesReceived);
			
			connection.bytesReceived += ret;
		}

		//if we dont know the packet size yet
		else {
			ret = connection.tcpSocket.recv(buffer.get() + connection.bytesReceived,
				buffer.Size() - connection.bytesReceived);

			connection.bytesReceived += ret;

			//if we have received the packet size header
			if (connection.bytesReceived >= 2) {
				connection.packetSize = (Uint16)SDLNet_Read16(buffer.get());
				if (connection.packetSize > MAX_TCP_PACKET_SIZE) {
					throw std::runtime_error("The TCP packet is too big, closing connection");//todo: create an exception
				}
				if (connection.bytesReceived > connection.packetSize) {
					throw std::runtime_error("The TCP packet is too big, closing connection");//todo: create an exception
				}

				//grow up the buffer if needed (lossless)
				if (buffer.Size() < connection.packetSize) {
					buffer.realloc(connection.packetSize);
				}
			}
		}

		//packet is fully received
		if (connection.bytesReceived == connection.packetSize) {
			connection.bytesReceived = 0;
			connection.lastRemoteActivityDate = SDL_GetTicks();
			return true;
		}
		else {
			return false;
		}
	}
};