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

struct TCPConnection {

	TCPsocketObject tcpSocket;
	int bytesReceived{ 0 };
	Uint16 packetSize{ 0 };//0 is a special value which means we dont know the size of the packet yet

	//Sending packet:
	int bytesToSend{ 0 };
	int sendingBufferIndex{ -1 };
	int recvBufferIndex{ NO_BUFFER };//-1 is a special value which means no current buffer

	TCPConnection() {};

	void setSocket(TCPsocketObject&& arg_tcpSocket, SocketSetObject& arg_socketSet) {
		tcpSocket = std::move(arg_tcpSocket);
		arg_socketSet.addSocket(tcpSocket);
	}

	TCPConnection(TCPsocketObject&& socket, SocketSetObject& socketSet) :
		tcpSocket{ std::move(socket) } {
		socketSet.addSocket(tcpSocket);
	}

	//NO COPY ALLOWED BECAUSE TCP SOCKET IS NOT COPYABLE
	TCPConnection(TCPConnection const& other) = delete;
	TCPConnection& operator=(TCPConnection const& other) = delete;

	//move allowed
	TCPConnection(TCPConnection&& other) = default;
	TCPConnection& operator=(TCPConnection&& other) = default;

	void close(SocketSetObject& set) {
		//remove the socket from the socket set
		set.delSocket(tcpSocket);
		tcpSocket.close();
	}

};

class TCPNetworkNode {
protected:
	SocketSetObject socketSet;
	const int MAX_SOCKETS;

	//receiving buffers
	std::vector<UniqueByteBuffer> recvBuffers;
	std::vector<int> availableRecvBuffers;


public:
	TCPNetworkNode() : MAX_SOCKETS{ MAX_TCP_SOCKETS }, socketSet{ MAX_TCP_SOCKETS } {
	}

	TCPNetworkNode(TCPNetworkNode const&) = delete;
	TCPNetworkNode(TCPNetworkNode &&) = delete;
	TCPNetworkNode& operator=(TCPNetworkNode const&) = delete;
	TCPNetworkNode& operator=(TCPNetworkNode&&) = delete;


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
			return true;
		}
		else {
			return false;
		}
	}
};