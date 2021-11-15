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
	UniqueByteBuffer buffer;
	int bufferSize{ 30 };//buffer size MUST always be >= 4
	int bytesReceived{ 0 };
	Uint16 packetSize{ 0 };//0 is a special value which means we dont know the size of the packet yet

	TCPConnection() : buffer(30) {};

	void setSocket(TCPsocketObject&& arg_tcpSocket, SocketSetObject& arg_socketSet) {
		tcpSocket = std::move(arg_tcpSocket);
		arg_socketSet.addSocket(tcpSocket);
	}

	TCPConnection(TCPsocketObject&& socket, SocketSetObject& socketSet) :
		tcpSocket{ std::move(socket) }, buffer(30) {
		socketSet.addSocket(tcpSocket);
	}
	TCPConnection(TCPConnection&& other) noexcept :
		tcpSocket{ std::move(other.tcpSocket) },
		buffer{ std::move(other.buffer) },
		bufferSize{ std::move(other.bufferSize) },
		bytesReceived{ std::move(other.bytesReceived) },
		packetSize{ std::move(other.packetSize) }
	{
	}

	//NO COPY ALLOWED BECAUSE TCP SOCKET IS NOT COPYABLE
	TCPConnection(TCPConnection const& other) = delete;
	TCPConnection& operator=(TCPConnection const& other) = delete;

	TCPConnection& operator=(TCPConnection&& other) noexcept {
		tcpSocket = std::move(other.tcpSocket);
		buffer = std::move(other.buffer);
		bufferSize = std::move(other.bufferSize);
		bytesReceived = std::move(other.bytesReceived);
		packetSize = std::move(other.packetSize);
		return *this;
	}

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

public:
	TCPNetworkNode() : MAX_SOCKETS{ 16 }, socketSet{ 16 } {
	}

	TCPNetworkNode(TCPNetworkNode const&) = delete;
	TCPNetworkNode(TCPNetworkNode &&) = delete;
	TCPNetworkNode& operator=(TCPNetworkNode const&) = delete;
	TCPNetworkNode& operator=(TCPNetworkNode&&) = delete;

	//return true if the packet has been fully sent
	/*bool sendPacket(TCPConnection const& connection, UniqueByteBuffer const& buffer, int size) const noexcept {
		assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
		return (SDLNet_TCP_Send(connection.tcpSocket, buffer.get(), size) == size);
	}*/

	//check if the sockets in the set are ready, and so a future call to receivePacket wont block
	//the number of sockets should be >= 1
	void checkSockets(Uint32 timeout = 0) {
		socketSet.checkSockets(timeout);	
	}

	//return true if the packet is fully received
	//throw if an error occured, if so the connection should be closed
	//TODO: make this method member of TCPConnection !
	bool receivePacket(TCPConnection& connection) const {
		if (!connection.tcpSocket.isReady()) 
			return false;

		int ret;
		//if we already know the packet size
		if (connection.bytesReceived >= 2) {
			ret = connection.tcpSocket.recv(connection.buffer.get() + connection.bytesReceived, 
				connection.packetSize - connection.bytesReceived);
			
			connection.bytesReceived += ret;
		}

		//if we dont know the packet size yet
		else {
			ret = connection.tcpSocket.recv(connection.buffer.get() + connection.bytesReceived,
				connection.bufferSize - connection.bytesReceived);

			connection.bytesReceived += ret;

			//if we have received the packet size header
			if (connection.bytesReceived >= 2) {
				connection.packetSize = (Uint16)SDLNet_Read16(connection.buffer.get());
				if (connection.packetSize > MAX_TCP_PACKET_SIZE) {
					throw std::runtime_error("The TCP packet is too big, closing connection");//todo: create an exception
				}
				if (connection.bytesReceived > connection.packetSize) {
					throw std::runtime_error("The TCP packet is too big, closing connection");//todo: create an exception
				}

				//grow up the buffer if needed (lossless)
				if (connection.bufferSize < connection.packetSize) {
					connection.bufferSize = connection.packetSize;
					connection.buffer.realloc(connection.bufferSize);
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