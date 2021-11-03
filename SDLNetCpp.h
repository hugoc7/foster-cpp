#pragma once
#include "SDL_Net.h"
#include <utility>
#include <string>
#include <iostream>

//SDL NET WRAPPER FOR C++ (RAII)
//TODO ... WORK IN PROGRESS
//ALMOST FINISHED ! JUST NEEDS TO BE INTEGRATED AND TESTED !


class TCPsocketObject {
	friend class SocketSetObject;
protected:
	TCPsocket socket{ NULL };
public:
	//No copy allowed
	TCPsocketObject(TCPsocketObject const& other) = delete;
	TCPsocketObject& operator=(TCPsocketObject const& other) = delete;

	//Move allowed
	TCPsocketObject(TCPsocketObject&& other) noexcept :
		socket{ std::move(other.socket) }
	{
		other.socket = NULL;
	}
	TCPsocketObject& operator=(TCPsocketObject&& other) noexcept {
		socket = std::move(other.socket);
		other.socket = NULL;
	}
	bool open(IPaddress* ip) noexcept {
		socket = SDLNet_TCP_Open(ip);
		if (socket == NULL) {
			std::cerr << "SDLNet_TCP_Open: " << SDLNet_GetError() << std::endl;
			return false;
		}
		return true;
	}
	int send(void const* data, int len) const {
		int nbBytesSent = SDLNet_TCP_Send(socket, data, len);
		if (nbBytesSent < len) {
			std::string msg{ "SDLNet_TCP_Send: " };
			msg += SDLNet_GetError();
			std::cerr << msg << std::endl;
			throw std::runtime_error(msg);//todo: create exception
		}
		return nbBytesSent;
	}
	int recv(void* data, int maxlen) const {
		int nbBytesReceived = SDLNet_TCP_Recv(socket, data, maxlen);
		if (nbBytesReceived == -1) {
			std::string errorMsg{ "Error in SDLNet_TCP_Recv: " };
			errorMsg += SDLNet_GetError();
			std::cerr << errorMsg << std::endl;
			throw std::runtime_error(errorMsg);//todo: create an exception
		}
		return nbBytesReceived;
	}

	bool isReady() const noexcept {
		return SDLNet_SocketReady(socket);
	}

	//no verification, the socket should be valid 
	void close() noexcept {
		SDLNet_TCP_Close(socket);
	}

	bool accept(TCPsocketObject const& serverSocket) noexcept {
		return (socket = SDLNet_TCP_Accept(serverSocket.socket)) != NULL;
	}

	~TCPsocketObject() noexcept {
		if (socket != NULL) {
			SDLNet_TCP_Close(socket);
		}
	}
};

class SocketSetObject {
protected:
	SDLNet_SocketSet set{ NULL };
	unsigned int size{ 0 };
public:

	//No copy allowed
	SocketSetObject(SocketSetObject const& other) = delete;
	SocketSetObject& operator=(SocketSetObject const& other) = delete;

	//Move allowed
	SocketSetObject(SocketSetObject&& other) noexcept :
		set{ std::move(other.set) },
		size{std::move(other.size)}
	{
		other.set = NULL;
	}
	SocketSetObject& operator=(SocketSetObject&& other) noexcept {
		set = std::move(other.set);
		size = std::move(other.size);

		other.set = NULL;
	}

	SocketSetObject(int maxSockets) : set{ SDLNet_AllocSocketSet(maxSockets) } {
		if (!set) {
			std::cerr << "SDLNet_AllocSocketSet: %s\n" << SDLNet_GetError();
			throw std::bad_alloc();
		}
	}

	void addSocket(TCPsocketObject const& socket) {
		if (SDLNet_TCP_AddSocket(set, socket.socket) == -1) {
			std::string msg{ "SDLNet_AddSocket: " };
			msg += SDLNet_GetError();
			std::cerr << msg << std::endl;
			throw std::runtime_error(msg);//TODO: make a custom exception for SDL_Net errors
		}
		size++;
	}

	void delSocket(TCPsocketObject const& socket) {
		if (SDLNet_TCP_DelSocket(set, socket.socket) == -1) {
			std::string msg{ "SDLNet_DelSocket: " };
			msg += SDLNet_GetError();
			std::cerr << msg << std::endl;
			throw std::runtime_error(msg);//TODO: make a custom exception for SDL_Net errors
		}
		size--;
	}
	void checkSockets() {
		if (SDLNet_CheckSockets(set, 0) == -1 && size > 0) {
			std::string msg{ "SDLNet_CheckSockets: " };
			msg += SDLNet_GetError();
			msg += "\n";
			msg += strerror(errno);
			std::cerr << msg << std::endl;
			throw std::runtime_error(msg);//TODO: make a custom exception for SDL_Net errors
		}
	}

	~SocketSetObject() noexcept {
		if (set != NULL) {
			SDLNet_FreeSocketSet(set);
		}
	}
};