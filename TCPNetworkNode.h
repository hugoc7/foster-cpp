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


struct TCPConnection {

	TCPsocket tcpSocket{ NULL };
	SDLNet_SocketSet socketSet{ NULL };
	UniqueByteBuffer buffer;
	int bufferSize;//buffer size MUST always be >= 4
	int bytesReceived{ 0 };
	Uint16 packetSize{ 0 };//0 is a special value which means we dont know the size of the packet yet


	TCPConnection(TCPsocket tcpSocket, SDLNet_SocketSet socketSet) :
		tcpSocket{ tcpSocket }, bufferSize{ 30 }, buffer(30), socketSet{ socketSet } {

		if (SDLNet_TCP_AddSocket(socketSet, tcpSocket) == -1) {
			printf("SDLNet_AddSocket: %s\n", SDLNet_GetError());
			throw std::bad_alloc();//TODO: make a custom exception for SDL_Net errors
		}
	}
	TCPConnection(TCPConnection&& other) noexcept :
		tcpSocket{ std::move(other.tcpSocket) },
		buffer{ std::move(other.buffer) },
		bufferSize{ std::move(other.bufferSize) },
		bytesReceived{ std::move(other.bytesReceived) },
		packetSize{ std::move(other.packetSize) },
		socketSet{ std::move(other.socketSet) }
	{
		other.tcpSocket = NULL;
		other.socketSet = NULL;
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
		socketSet = std::move(other.socketSet);
		other.tcpSocket = NULL;
		other.socketSet = NULL;
		return *this;
	}

	void close() {
		//remove the socket form the socket set
		if (SDLNet_TCP_DelSocket(socketSet, tcpSocket) == -1) {
			printf("SDLNet_DelSocket: %s\n", SDLNet_GetError());
			// perhaps the socket is not in the set
			exit(1);//TODO: make a custom exception for SDL_Net errors
		}
		SDLNet_TCP_Close(tcpSocket);
		tcpSocket = NULL;
	}
	~TCPConnection() {
		if (tcpSocket != NULL) {
			if (socketSet != NULL) {
				//remove the socket form the socket set
				if (SDLNet_TCP_DelSocket(socketSet, tcpSocket) == -1) {
					printf("SDLNet_DelSocket: %s\n", SDLNet_GetError());
					// perhaps the socket is not in the set
					exit(1);//TODO: make a custom exception for SDL_Net errors
				}
			}
			SDLNet_TCP_Close(tcpSocket);
		}
	}

};

/* TODO: this class shouldnt be movable nor copyable (maybe move can be implemented later ...)*/
class TCPNetworkNode {
protected:
	SDLNet_SocketSet socketSet{ NULL };
	const int MAX_SOCKETS;

public:
	TCPNetworkNode() : MAX_SOCKETS{ 16 }, socketSet{ SDLNet_AllocSocketSet(16) }
	{
		if (!socketSet) {
			std::cerr << "SDLNet_AllocSocketSet: %s\n", SDLNet_GetError();
			throw std::bad_alloc();
		}
	}
	~TCPNetworkNode() {
		SDLNet_FreeSocketSet(socketSet);
	}

	//return true if the packet has been fully sent
	bool sendPacket(TCPConnection const& connection, UniqueByteBuffer const& buffer, int size) const noexcept {
		assert(size < 65536 && size <= MAX_TCP_PACKET_SIZE);
		return (SDLNet_TCP_Send(connection.tcpSocket, buffer.get(), size) == size);
	}

	//check if the sockets in the set are ready, and so a future call to receivePacket wont block
	//the number of sockets should be >= 1
	void checkSockets(int socketSetSize) {
		if (SDLNet_CheckSockets(socketSet, 0) == -1 && socketSetSize > 0) {
			std::cerr << "SDLNet_CheckSockets: %s\n" << SDLNet_GetError();
			//most of the time this is a system error, where perror might help you.
			perror("SDLNet_CheckSockets");
			throw std::bad_alloc();//create a real exception
		}
	}

	//return true if the packet is fully received
	//throw if an error occured, if so the connection should be closed
	bool receivePacket(TCPConnection& connection) const {
		if (!SDLNet_SocketReady(connection.tcpSocket)) 
			return false;

		int ret;
		//if we already know the packet size
		if (connection.bytesReceived >= 2) {
			ret = SDLNet_TCP_Recv(connection.tcpSocket, connection.buffer.get() + connection.bytesReceived, 
				connection.packetSize - connection.bytesReceived);
			if (ret == -1) {
				std::string errorMsg{ "Error in SDLNet_TCP_Recv: " };
				errorMsg += SDLNet_GetError();
				std::cerr << errorMsg << std::endl;
				throw std::runtime_error(errorMsg);//todo: create an exception
			}
			connection.bytesReceived += ret;
		}

		//if we dont know the packet size yet
		else {
			ret = SDLNet_TCP_Recv(connection.tcpSocket, connection.buffer.get() + connection.bytesReceived, 
				connection.bufferSize - connection.bytesReceived);
			if (ret == -1) {
				std::string errorMsg{ "Error in SDLNet_TCP_Recv: " };
				errorMsg += SDLNet_GetError();
				std::cerr << errorMsg << std::endl;
				throw std::runtime_error(errorMsg);//todo: create an exception class
			}
			connection.bytesReceived += ret;

			//if we have received the packet size header
			if (connection.bytesReceived >= 2) {
				connection.packetSize = (Uint16)SDLNet_Read16(connection.buffer.get());
				if (connection.packetSize > MAX_TCP_PACKET_SIZE) {
					std::cerr << "The TCP packet is too big, closing connection.\n";
					throw std::runtime_error("The TCP packet is too big, closing connection");//todo: create an exception
				}
				if (connection.bytesReceived > connection.packetSize) {
					std::cerr << "The TCP packet is too short, closing connection.\n";
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