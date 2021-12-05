#pragma once
#include "SDL_Net.h"
#include <utility>
#include <string>
#include <iostream>

//SDL NET WRAPPER FOR C++ (RAII)
//TODO ... WORK IN PROGRESS
//ALMOST FINISHED ! JUST NEEDS TO BE INTEGRATED AND TESTED !

//WARNING: peut etre rajouter une condition if (socket!=NULL) throw au debut de chaque methode de TCPSocketObject

//helper
std::string getIpAdressAsString(Uint32 ip);

class IPaddressObject {
	//friend class TCPsocketObject;
	//friend class UDPsocketObject;
public:
	IPaddress ip;

	IPaddressObject() = default;
	void resolveHost(Uint16 port) {
		if (SDLNet_ResolveHost(&ip, NULL, port) == -1) {
			throw std::runtime_error(std::string("SDLNet_ResolveHost: ") + SDLNet_GetError());
		}
	}
	void resolveHost(std::string const& hostname, Uint16 port) {
		if (SDLNet_ResolveHost(&ip, hostname.c_str(), port) == -1) {
			throw std::runtime_error(std::string("SDLNet_ResolveHost: ") + SDLNet_GetError());
		}
	}
};

class TCPsocketObject {
	friend class SocketSetObject;
protected:
	TCPsocket socket;
public:
	TCPsocketObject() noexcept : socket{ NULL } {
	};

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
		return *this;
	}
	void open(IPaddressObject& ip) {
		socket = SDLNet_TCP_Open(&(ip.ip));
		if (socket == NULL) {
			throw std::runtime_error(std::string("SDLNet_TCP_Open: ") + SDLNet_GetError());
		}
	}
	int send(void const* data, int len) const {
		assert(socket != NULL);
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
		assert(socket != NULL);
		int nbBytesReceived = SDLNet_TCP_Recv(socket, data, maxlen);
		if (nbBytesReceived == -1) {
			std::string errorMsg{ "Error in SDLNet_TCP_Recv: " };
			errorMsg += SDLNet_GetError();
			std::cerr << errorMsg << std::endl;
			throw std::runtime_error(errorMsg);//todo: create an exception
		}
		return nbBytesReceived;
	}

	inline bool isReady() const noexcept {
		assert(socket != NULL);
		return SDLNet_SocketReady(socket);
	}

	//no verification, the socket should be valid 
	void close() noexcept {
		assert(socket != NULL);
		SDLNet_TCP_Close(socket);
		socket = NULL;
	}
	bool accept(TCPsocketObject& newSocket) noexcept {
		//problem: SDLNet_TCP_Accept ne permet pas de differencier une erreur d'une simple absence de nouvelle connexion
		assert(socket != NULL);
		assert(newSocket.socket == NULL);
		return (newSocket.socket = SDLNet_TCP_Accept(socket)) != NULL;
	}
	std::string getPeerIP() const {
		assert(socket != NULL);
		IPaddress* remote_ip{ NULL };
		remote_ip = SDLNet_TCP_GetPeerAddress(socket);
		if (!remote_ip) {
			throw std::runtime_error(std::string("SDLNet_TCP_GetPeerAddress: %s\n") + SDLNet_GetError() 
				+ "This may be a server socket.\n");
		}
		return getIpAdressAsString(remote_ip->host);
	}
	Uint16 getPeerPort() const {
		assert(socket != NULL);
		IPaddress* remote_ip{ NULL };
		remote_ip = SDLNet_TCP_GetPeerAddress(socket);
		if (!remote_ip) {
			throw std::runtime_error(std::string("SDLNet_TCP_GetPeerAddress: %s\n") + SDLNet_GetError()
				+ "This may be a server socket.\n");
		}
		return remote_ip->port;
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
		other.size = 0;
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
		assert(set != NULL);
		assert(socket.socket != NULL);
		if (SDLNet_TCP_AddSocket(set, socket.socket) == -1) {
			std::string msg{ "SDLNet_AddSocket: " };
			msg += SDLNet_GetError();
			std::cerr << msg << std::endl;
			throw std::runtime_error(msg);//TODO: make a custom exception for SDL_Net errors
		}
		size++;
	}

	void delSocket(TCPsocketObject const& socket) {
		assert(set != NULL);
		assert(socket.socket != NULL);
		if (SDLNet_TCP_DelSocket(set, socket.socket) == -1) {
			std::string msg{ "SDLNet_DelSocket: " };
			msg += SDLNet_GetError();
			std::cerr << msg << std::endl;
			throw std::runtime_error(msg);//TODO: make a custom exception for SDL_Net errors
		}
		size--;
	}
	void checkSockets(Uint32 timeout) {
		assert(set != NULL);
		if (SDLNet_CheckSockets(set, timeout) == -1 && size > 0) {
			throw std::runtime_error(std::string{ "SDLNet_CheckSockets: " } + SDLNet_GetError());//TODO: make a custom exception for SDL_Net errors
		}
	}
	

	~SocketSetObject() noexcept {
		if (set != NULL) {
			SDLNet_FreeSocketSet(set);
		}
	}
};

struct UDPpacketObject {
public:
	UDPpacket* packet{ NULL };
	UDPpacketObject() noexcept {
	}
	UDPpacketObject(int size) {
		assert(size > 0);
		if ((packet = SDLNet_AllocPacket(size)) == NULL) {
			throw std::bad_alloc();
		}
	}
	~UDPpacketObject() noexcept {
		if (packet != NULL)
			SDLNet_FreePacket(packet);
	}

	void resize(int size) {
		assert(size > 0);
		assert(packet != NULL);
		if (SDLNet_ResizePacket(packet, size) < size) {
			throw std::bad_alloc();
		}
	}


	//No copy allowed
	UDPpacketObject(UDPpacketObject const& other) = delete;
	UDPpacketObject& operator=(UDPpacketObject const& other) = delete;

	//Move allowed
	UDPpacketObject(UDPpacketObject&& other) noexcept :
		packet{ other.packet }
	{
		other.packet = NULL;
	}
	UDPpacketObject& operator=(UDPpacketObject&& other) noexcept {
		packet = other.packet;
		other.packet = NULL;
		return *this;
	}
};


class UDPsocketObject {
protected:
	UDPsocket socket{NULL};
public:
	~UDPsocketObject() noexcept {
		if (socket != NULL) {
			SDLNet_UDP_Close(socket);
		}
	}
	void open(Uint16 port) {
		socket = SDLNet_UDP_Open(port);
		if (socket == NULL)
			throw std::runtime_error(std::string("SDLNet_UDP_Open ") + SDLNet_GetError());
	}
	void close() noexcept {
		assert(socket != NULL);
		SDLNet_UDP_Close(socket);
	}
	inline int bind(int channel, IPaddressObject const& ip) {
		return bind(channel, ip.ip);
	}
	int bind(int channel, IPaddress const& ip) {
		assert(socket != NULL);
		int ret;
		if ((ret = SDLNet_UDP_Bind(socket, channel, &ip)) == -1) {
			throw std::runtime_error(std::string("SDLNet_UDP_Bind ") + SDLNet_GetError());
		}
		return ret;
	}
	void unbind(int channel) noexcept {
		assert(socket != NULL);
		SDLNet_UDP_Unbind(socket, channel);
	}
	//assume that the channel is bound to ONLY 1 destination !!
	void send(int channel, UDPpacketObject& packet) {
		assert(socket != NULL);
		if(SDLNet_UDP_Send(socket, channel, packet.packet) != 1)
			throw std::runtime_error(std::string("SDLNet_UDP_Send ") + SDLNet_GetError());
	}
	//INFO: Recv throws when it receives a packet from an unbound address
	bool recv(UDPpacketObject& packet) {
		assert(socket != NULL);
		int ret;
		 
		if ((ret = SDLNet_UDP_Recv(socket, packet.packet)) == -1) {
			throw std::runtime_error(std::string("SDLNet_UDP_Recv ") + SDLNet_GetError());
		}
		return static_cast<bool>(ret);
	}
	std::string getPeerIP(int channel) const {
		assert(socket != NULL);
		IPaddress* remote_ip{ NULL };
		remote_ip = SDLNet_UDP_GetPeerAddress(socket, channel);
		if (!remote_ip) {
			throw std::runtime_error(std::string("SDLNet_UDP_GetPeerAddress ") + SDLNet_GetError());
		}
		return getIpAdressAsString(remote_ip->host);
	}
	Uint16 getPeerPort(int channel) const {
		assert(socket != NULL);
		IPaddress* remote_ip{ NULL };
		remote_ip = SDLNet_UDP_GetPeerAddress(socket, channel);
		if (!remote_ip) {
			throw std::runtime_error(std::string("SDLNet_TCP_GetPeerAddress ") + SDLNet_GetError());
		}
		return remote_ip->port;
	}
	
};

