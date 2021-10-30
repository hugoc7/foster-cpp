#pragma once


#include "SDL_net.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include "ReaderWriterQueue.h"
#include "Containers.h"
#include "Network.h"
#include <atomic>
#include <functional>

struct ClientConnection {
	TCPsocket tcpSocket{ NULL };
	SDLNet_SocketSet socketSet{ NULL };
	std::string playerName;
	bool connectedToGame{ false };//a client is "connected to game" from a new player request until a disconnection request  
	
	UniqueByteArray buffer;
	int bufferSize;//buffer size MUST always be >= 4
	int bytesReceived{ 0 };
	Uint16 packetSize{0};//0 is a special value which means we dont know the size of the packet yet
	int playerID{-1};


	ClientConnection(TCPsocket tcpSocket, SDLNet_SocketSet socketSet) :
		tcpSocket{ tcpSocket }, bufferSize{ 30 }, buffer(30), socketSet{ socketSet } {

		if (SDLNet_TCP_AddSocket(socketSet, tcpSocket) == -1) {
			printf("SDLNet_AddSocket: %s\n", SDLNet_GetError());
			// perhaps you need to restart the set and make it bigger...
			throw std::bad_alloc();//TODO: make a custom exception for SDL_Net errors
		}
	}
	ClientConnection(ClientConnection&& other) noexcept :
		tcpSocket{ std::move(other.tcpSocket) },
		buffer{ std::move(other.buffer) },
		bufferSize{ std::move(other.bufferSize) },
		bytesReceived{ std::move(other.bytesReceived) },
		packetSize{ std::move(other.packetSize) },
		playerName{ std::move(other.playerName) },
		socketSet{ std::move(other.socketSet) },
		connectedToGame{other.connectedToGame},
		playerID{other.playerID}
	{
		other.tcpSocket = NULL;
		other.socketSet = NULL;
	}
	ClientConnection& operator=(ClientConnection&& other) noexcept {
		tcpSocket = std::move(other.tcpSocket);
		buffer = std::move(other.buffer);
		bufferSize = std::move(other.bufferSize);
		bytesReceived = std::move(other.bytesReceived);
		packetSize = std::move(other.packetSize);
		playerName = std::move(other.playerName);
		socketSet = std::move(other.socketSet);
		connectedToGame = other.connectedToGame;
		playerID = other.playerID;
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
	~ClientConnection() {
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


class Server {
public:
	std::vector<ClientConnection> connections;
	std::chrono::milliseconds delayForNewConnection{ 500 };
	//moodycamel::ReaderWriterQueue<TCPmessage> messages;//concurrent queue
	moodycamel::ReaderWriterQueue<TCPmessage> messagesToSend;//concurrent queue
	ConcurrentCircularQueue<TCPmessage> messages;
	std::thread thread{};
	std::atomic<bool> serverRunning{ false };
	TCPsocket listeningTcpSock{ NULL };
	SDLNet_SocketSet socketSet{ NULL };
	const int MAX_SOCKETS{16};

	///@brief receive TCP messages from all clients (non blocking)
	void receiveMessagesFromClients(std::vector<ClientConnection>& clients, ConcurrentCircularQueue<TCPmessage>& messages);

	void acceptNewConnection(TCPsocket listeningTcpSock);

	void loop(TCPsocket listeningTcpSock, ConcurrentCircularQueue<TCPmessage>& msgQueue);
	
	void stop();

	void start();

	Server();
	~Server();
private:
	void handleFullyReceivedPacket(int clientID);
	void closeConnection(int clientID);

	//generate a unique player ID (different from clientID which is an index in a client vector)
	//a player ID is unique
	int generatePlayerID() {
		static int counter = 0;
		if(counter+1 == std::numeric_limits<int>::max()) {
			return (counter = 0);
		}
		return counter++;
	}
};
