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
#include "SDLNetCpp.h"
#include "PlayerInfos.h"
#include <memory>

//Structs used to share information from game to TCP server thread
class MsgFromGame {
public:
	virtual char type() const noexcept = 0;
	virtual ~MsgFromGame() = default;//IMPORTANT!
};
class PlayerEntityCreated : public MsgFromGame {
public:
	Uint16 entityID;
	Uint16 playerID;

	virtual char type() const noexcept override {
		return 0;//TODO: use an enum instead!
	}

	PlayerEntityCreated(Uint16 playerId, Uint16 entityId) :
		playerID{ playerId },
		entityID{ entityId }
	{}
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


class TCPServer : TCPNetworkNode {
	
public:
	moodycamel::ReaderWriterQueue<TCPmessage> messages;
	void stop();
	void start(Uint16 port, Uint16 udpPort);
	
	void playerEntityCreated(Uint16 playerID, Uint16 entityID) {
		messagesFromGame.emplace(std::make_unique<PlayerEntityCreated>(playerID, entityID));
	}

	TCPServer();
	~TCPServer();

private:

	void handleMessagesFromGame() {
		std::unique_ptr<MsgFromGame> new_message;
		while (messagesFromGame.try_dequeue(new_message)) {
			switch (new_message.get()->type()) {
			case 0://todo: use a enum
			{
				PlayerEntityCreated* msg = static_cast<PlayerEntityCreated*>(new_message.get());
				assert(msg->playerID < playerIdToConnectionIndex.size());
				Uint16 clientID = playerIdToConnectionIndex[msg->playerID];
				connections[clientID].entityID = msg->entityID;
				sendNewPlayerNotification(connections[clientID]);
				sendPlayerList(connections[clientID]);
				break;
			}
			}
		}
	}

	///@brief receive TCP messages from all clients (non blocking)
	void receiveMessagesFromClients();

	void acceptNewConnection();

	void loop();

	//tell a client that he will be disconnected
	void sendGoodbye(ClientConnection& client, std::string const& reason) {
		Uint16 size = 4 + reason.size();
		if (sendingBuffer.Size() < size) {
			sendingBuffer.lossfulRealloc(size);
		}
		SDLNet_Write16((Uint16)size, sendingBuffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::GOODBYE, sendingBuffer.get() + 2);
		std::memcpy(sendingBuffer.get() + 4, reason.c_str(), reason.size());
		sendPacket(client, size);
	}

	//send to all a new connection msg
	void sendNewPlayerNotification(ClientConnection const& newClient) {
		Uint16 size = newClient.playerName.size() + 4 + 2 + 2;
		assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
		if (sendingBuffer.Size() < size) {
			sendingBuffer.lossfulRealloc(size);
		}
		SDLNet_Write16((Uint16)size, sendingBuffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::NEW_CONNECTION, sendingBuffer.get() + 2);
		SDLNet_Write16((Uint16)newClient.playerID, sendingBuffer.get() + 4);

		Packing::WriteUint16(newClient.entityID, &sendingBuffer.get()[6]);

		std::memcpy(sendingBuffer.get() + 8, newClient.playerName.c_str(), newClient.playerName.size());

		//for each client
		for (int i = 0; i < connections.size(); i++) {
			if (connections[i].playerID != newClient.playerID)
				sendPacket(connections[i], size);
		}
	}

	void sendChatMessage(std::string const& message, Uint16 senderPlayerID) {
		Uint16 size = message.size() + 4 + 2;
		assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
		if (sendingBuffer.Size() < size) {
			sendingBuffer.lossfulRealloc(size);
		}
		SDLNet_Write16((Uint16)size, sendingBuffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::NEW_CHAT_MESSAGE, sendingBuffer.get() + 2);
		SDLNet_Write16((Uint16)senderPlayerID, sendingBuffer.get() + 4);
		std::memcpy(sendingBuffer.get() + 6, message.c_str(), message.size());

		//for each client
		for (int i = 0; i < connections.size(); i++) {
			sendPacket(connections[i], size);
		}
	}

	void sendPlayerList(ClientConnection& newClient) {

		//PACKET SCHEMA (unit = byte)
		//packetSize(2) + type(2) + udpPort(2) + Nplayers * ( playerId(2) + nameSize(1) + name(n) ) 
		//TODO: packetSize(2) + type(2) + udpPort(2) + Nplayers * ( playerId(2) + playerEntityId(2) + nameSize(1) + name(n) ) 

		Uint16 size{ 6 };
		unsigned int currentByteIndex{ 6 };
		for (int i = 0; i < connections.size(); i++) {
			size += 1 + 2 + 2 + connections[i].playerName.size();
		}
		assert(size <= MAX_16_BIT_VALUE && size <= MAX_TCP_PACKET_SIZE);
		if (sendingBuffer.Size() < size) {
			sendingBuffer.lossfulRealloc(size);
		}
		SDLNet_Write16((Uint16)size, sendingBuffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::PLAYER_LIST, sendingBuffer.get() + 2);
		SDLNet_Write16(udpServerPort, sendingBuffer.get() + 4);
		//std::unique_lock<std::mutex> playerInfosLock(playerInfosMutex, std::defer_lock);
		for (int i = 0; i < connections.size(); i++) {
			SDLNet_Write16((Uint16)connections[i].playerID, sendingBuffer.get() + currentByteIndex);
			currentByteIndex += 2;

			Packing::WriteUint16(connections[i].entityID, &sendingBuffer.get()[currentByteIndex]);
			currentByteIndex += 2;

			assert(connections[i].playerName.size() <= 255);
			sendingBuffer.get()[currentByteIndex] = static_cast<Uint8>(connections[i].playerName.size());
			currentByteIndex += 1;
			std::memcpy(sendingBuffer.get() + currentByteIndex, connections[i].playerName.c_str(), connections[i].playerName.size());
			currentByteIndex += connections[i].playerName.size();
		}
		sendPacket(newClient, size);
	}

	//send to all a new disconnection msg
	//TODO: corriger le pb de gestion des exceptions lors de l'envoi de notifications aux autres clients
	void sendPlayerDisconnectionNotification(Uint16 disconnectedPlayerID) {
		const Uint16 size{ 6 };
		if (sendingBuffer.Size() < size) {
			sendingBuffer.lossfulRealloc(size);
		}
		SDLNet_Write16((Uint16)size, sendingBuffer.get());
		SDLNet_Write16((Uint16)TcpMsgType::NEW_DISCONNECTION, sendingBuffer.get() + 2);
		SDLNet_Write16(disconnectedPlayerID, sendingBuffer.get() + 4);

		//for each client
		for (int i = 0; i < connections.size(); i++) {
			sendPacket(connections[i], size);
		}
	}

	void closeConnection(int clientID);

	//UNUSED ! 
	//generate a unique player ID (different from clientID which is an index in a client vector)
	Uint16 generatePlayerID() {
		static Uint16 counter = 0;
		if(counter+1u == 65535u) {
			return (counter = 0);
		}
		return counter++;
	}

	Uint16 getNewPlayerID() {
		assert(!availablePlayerIDs.empty());
		Uint16 id = availablePlayerIDs.back();
		availablePlayerIDs.pop_back();
		return id;
	}
	void releasePlayerID(Uint16 id) {
		assert(id < MAX_CLIENTS && id >= 0);
		assert(availablePlayerIDs.size() < MAX_CLIENTS);
		availablePlayerIDs.push_back(id);
	}

	std::vector<ClientConnection> connections;
	std::chrono::milliseconds delayForNewConnection{ 500 };
	std::thread thread{};
	std::atomic<bool> serverRunning{ false };
	TCPsocketObject listeningTcpSock;
	Uint16 udpServerPort;

	const int MAX_CLIENTS = 10;
	int connectedClients = 0;//number of connected clients
	std::vector<int> availablePlayerIDs;
	std::vector<Uint16> playerIdToConnectionIndex;
	moodycamel::ReaderWriterQueue<std::unique_ptr<MsgFromGame>> messagesFromGame;
};
