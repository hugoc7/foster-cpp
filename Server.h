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
#include "TCPNetworkNode.h"

struct ClientConnection : TCPConnection {
	std::string playerName;
	bool connectedToGame{ false };//a client is "connected to game" from a new player request until a disconnection request  
	int playerID{-1};


	ClientConnection(TCPsocket tcpSocket, SDLNet_SocketSet socketSet) : TCPConnection(tcpSocket, socketSet) {
	}

	ClientConnection(ClientConnection&& other) noexcept :
		TCPConnection(std::move(other)),
		playerName{ std::move(other.playerName) },
		connectedToGame{other.connectedToGame},
		playerID{other.playerID}
	{
	}
	ClientConnection& operator=(ClientConnection&& other) noexcept {
		TCPConnection::operator=(std::move(other));
		playerName = std::move(other.playerName);
		connectedToGame = other.connectedToGame;
		playerID = other.playerID;
		return *this;
	}
	
};


class Server : TCPNetworkNode {
public:
	std::vector<ClientConnection> connections;
	std::chrono::milliseconds delayForNewConnection{ 500 };
	//moodycamel::ReaderWriterQueue<TCPmessage> messages;//concurrent queue
	moodycamel::ReaderWriterQueue<TCPmessage> messagesToSend;//concurrent queue
	ConcurrentCircularQueue<TCPmessage> messages;
	std::thread thread{};
	std::atomic<bool> serverRunning{ false };
	TCPsocket listeningTcpSock{ NULL };
	//SDLNet_SocketSet socketSet{ NULL };
	//const int MAX_SOCKETS{16};

	///@brief receive TCP messages from all clients (non blocking)
	void receiveMessagesFromClients(std::vector<ClientConnection>& clients, ConcurrentCircularQueue<TCPmessage>& messages);

	void acceptNewConnection(TCPsocket listeningTcpSock);

	void loop(TCPsocket listeningTcpSock, ConcurrentCircularQueue<TCPmessage>& msgQueue);
	
	void stop();

	void start();

	//void sendToAll(TCPmessage&& message);

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
