#pragma once
#include "SDLNetCpp.h"
#include "Network.h"

struct TCPConnection {

	TCPsocketObject tcpSocket;
	int bytesReceived{ 0 };
	Uint16 packetSize{ 0 };//0 is a special value which means we dont know the size of the packet yet

	//Sending packet:
	int bytesToSend{ 0 };
	int sendingBufferIndex{ -1 };
	int recvBufferIndex{ NO_BUFFER };//-1 is a special value which means no current buffer

	Uint32 lastLocalActivityDate;
	Uint32 lastRemoteActivityDate;

	TCPConnection() {
		Uint32 now = SDL_GetTicks();
		lastLocalActivityDate = lastRemoteActivityDate = now;
	};

	void setSocket(TCPsocketObject&& arg_tcpSocket, SocketSetObject& arg_socketSet) {
		tcpSocket = std::move(arg_tcpSocket);
		arg_socketSet.addSocket(tcpSocket);
		lastLocalActivityDate = lastRemoteActivityDate = SDL_GetTicks();
	}

	TCPConnection(TCPsocketObject&& socket, SocketSetObject& socketSet) :
		tcpSocket{ std::move(socket) } {
		socketSet.addSocket(tcpSocket);
		Uint32 now = SDL_GetTicks();
		lastLocalActivityDate = lastRemoteActivityDate = now;
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


struct ClientConnection : TCPConnection {
	std::string playerName;
	bool connectedToGame{ false };//a client is "connected to game" from a new player request until a disconnection request  
	Uint16 playerID{};
	Uint16 entityID{};


	ClientConnection(TCPsocketObject&& tcpSocket, SocketSetObject& socketSet) :
		TCPConnection(std::move(tcpSocket), socketSet)
	{
	}

	ClientConnection(ClientConnection&& other) = default;
	ClientConnection& operator=(ClientConnection&& other) = default;
};