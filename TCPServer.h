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



struct ClientConnection : TCPConnection {
	std::string playerName;
	bool connectedToGame{ false };//a client is "connected to game" from a new player request until a disconnection request  
	Uint16 playerID{};


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
	
	void setPlayerEntityID(Uint16 playerID, Uint16 entityID) {
		std::lock_guard<std::mutex> lock(playerInfosMutex);
		assert(playerInfos.Contains(playerID));
		ServerPlayerInfos& infos = playerInfos.Get(playerID);
		infos.entity = entityID;
		infos.playerListNeedsToBeSent = true;
	}

	TCPServer();
	~TCPServer();

private:

	void informClientsOfNewPlayers() {
		std::lock_guard<std::mutex> lock(playerInfosMutex);
		//TODO: optimize it by checking only players that we have not declared to clients yet
		//TODO2: manage exceptions
		for (int i = 0; i < playerInfos.Size(); i++) {
			if (playerInfos[i].playerListNeedsToBeSent) {
				Uint16 clientID = playerInfos[i].clientIndex;
				assert(clientID < connections.size());
				sendPlayerList(connections[clientID]);
				sendNewPlayerNotification(connections[clientID]);
				playerInfos[i].playerListNeedsToBeSent = false;
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

		//write player's entity ID
		//std::unique_lock<std::mutex> playerInfosLock(playerInfosMutex);
		assert(playerInfos.Contains(newClient.playerID));
		Packing::WriteUint16(playerInfos.Get(newClient.playerID).entity, &sendingBuffer.get()[6]);
		//playerInfosLock.unlock();

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

			//write player's entity ID
			//playerInfosLock.lock();
			assert(playerInfos.Contains(connections[i].playerID));
			Packing::WriteUint16(playerInfos.Get(connections[i].playerID).entity, &sendingBuffer.get()[currentByteIndex]);
			currentByteIndex += 2;
			//playerInfosLock.unlock();

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
	//moodycamel::ReaderWriterQueue<TCPmessage> messages;//concurrent queue
	//moodycamel::ReaderWriterQueue<TCPmessage> messagesToSend;//concurrent queue
	std::thread thread{};
	std::atomic<bool> serverRunning{ false };
	TCPsocketObject listeningTcpSock;
	Uint16 udpServerPort;

	const int MAX_CLIENTS = 10;
	int connectedClients = 0;//number of connected clients
	std::vector<int> availablePlayerIDs;
	std::vector<Uint16> playerIdToConnectionIndex;
	ArrayMap<ServerPlayerInfos> playerInfos;
	std::mutex playerInfosMutex;
};
