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
	Uint16 playerID{};


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
	//moodycamel::ReaderWriterQueue<TCPmessage> messagesToSend;//concurrent queue
	moodycamel::ReaderWriterQueue<TCPmessage> messages;
	std::thread thread{};
	std::atomic<bool> serverRunning{ false };
	TCPsocket listeningTcpSock{ NULL };

	UniqueByteBuffer buffer; //buffer for sending !
	int bufferSize;

	//SDLNet_SocketSet socketSet{ NULL };
	//const int MAX_SOCKETS{16};

	///@brief receive TCP messages from all clients (non blocking)
	void receiveMessagesFromClients(std::vector<ClientConnection>& clients, moodycamel::ReaderWriterQueue<TCPmessage>& messages);

	void acceptNewConnection(TCPsocket listeningTcpSock);

	void loop(TCPsocket listeningTcpSock, moodycamel::ReaderWriterQueue<TCPmessage>& msgQueue);
	
	void stop();

	void start();

	//send to all a new connection msg
	void sendNewPlayerNotification(ClientConnection const& newClient) {
		Uint16 size = newClient.playerName.size() + 4 + 2;
		assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
		if (bufferSize < size) {
			bufferSize = size;
			buffer.lossfulRealloc(size);
		}
		SDLNet_Write16((Uint16)size, buffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::NEW_CONNECTION, buffer.get() + 2);
		SDLNet_Write16((Uint16)newClient.playerID, buffer.get() + 4);
		std::memcpy(buffer.get() + 6, newClient.playerName.c_str(), newClient.playerName.size());

		//for each client
		for(int i =0;i< connections.size();i++) {
			if (connections[i].playerID != newClient.playerID)
				SDLNet_TCP_Send(connections[i].tcpSocket, buffer.get(), size);
		}
	}

	void sendChatMessage(std::string const& message, Uint16 senderPlayerID) {
		Uint16 size = message.size() + 4 + 2;
		assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
		if (bufferSize < size) {
			bufferSize = size;
			buffer.lossfulRealloc(size);
		}
		SDLNet_Write16((Uint16)size, buffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::NEW_CHAT_MESSAGE, buffer.get() + 2);
		SDLNet_Write16((Uint16)senderPlayerID, buffer.get() + 4);
		std::memcpy(buffer.get() + 6, message.c_str(), message.size());

		//for each client
		for (int i = 0; i < connections.size(); i++) {
			if (SDLNet_TCP_Send(connections[i].tcpSocket, buffer.get(), size) < size) {
				std::cerr << "HOUSTON WE HAVE A PB";
			}
		}
	}

	void sendPlayerList(ClientConnection const& newClient) {
	
		//PACKET SCHEMA (unit = byte)
		//packetSize(2) + type(2) + Nplayers * ( playerId(2) + nameSize(1) + name(n) ) 
		Uint16 size{ 4 };
		unsigned int currentByteIndex{ 4 };
		for (int i = 0; i < connections.size(); i++) {
			size += 1 + 2 + connections[i].playerName.size();
		}
		assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
		if (bufferSize < size) {
			bufferSize = size;
			buffer.lossfulRealloc(size);
		}
		SDLNet_Write16((Uint16)size, buffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::PLAYER_LIST, buffer.get() + 2);
		for (int i = 0; i < connections.size(); i++) {
			SDLNet_Write16((Uint16)connections[i].playerID, buffer.get() + currentByteIndex);
			currentByteIndex += 2;
			assert(connections[i].playerName.size() <= 255);
			buffer.get()[currentByteIndex] = static_cast<Uint8>(connections[i].playerName.size());
			currentByteIndex += 1;
			std::memcpy(buffer.get() + currentByteIndex, connections[i].playerName.c_str(), connections[i].playerName.size());
			currentByteIndex += connections[i].playerName.size();
		}
		SDLNet_TCP_Send(newClient.tcpSocket, buffer.get(), size);
	}

	//send to all a new disconnection msg
	void sendPlayerDisconnectionNotification(Uint16 disconnectedPlayerID) {
		const Uint16 size{ 6 };
		if (bufferSize < size) {
			bufferSize = size;
			buffer.lossfulRealloc(size);
		}
		SDLNet_Write16((Uint16)size, buffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::NEW_DISCONNECTION, buffer.get() + 2);
		SDLNet_Write16(disconnectedPlayerID, buffer.get() + 4);

		//for each client
		for (int i = 0; i < connections.size(); i++) {
			SDLNet_TCP_Send(connections[i].tcpSocket, buffer.get(), size);
		}
	}

	//void sendToAll(TCPmessage&& message);

	Server();
	~Server();
private:
	void closeConnection(int clientID);

	//generate a unique player ID (different from clientID which is an index in a client vector)
	//a player ID is unique
	Uint16 generatePlayerID() {
		static Uint16 counter = 0;
		if(counter+1u == 65535u) {
			return (counter = 0);
		}
		return counter++;
	}
};
