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
#include "TCPConnection.h"
#include "TCPMessage.h"

enum class MsgFromGameType : Uint8 {
	PLAYER_ENTITY_CREATED = 0
};

//Structs used to share information from game to TCP server thread
class MsgFromGame {
public:
	virtual MsgFromGameType type() const noexcept = 0;
	virtual ~MsgFromGame() = default;//IMPORTANT!
};
class PlayerEntityCreated : public MsgFromGame {
public:
	Uint16 entityID;
	Uint16 playerID;

	virtual MsgFromGameType type() const noexcept override {
		return MsgFromGameType::PLAYER_ENTITY_CREATED;
	}

	PlayerEntityCreated(Uint16 playerId, Uint16 entityId) :
		playerID{ playerId },
		entityID{ entityId }
	{}
};

class TCPServer : TCPNetworkNode {
	
public:
	moodycamel::ReaderWriterQueue<std::unique_ptr<TCPMessage>> messages;
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
			case MsgFromGameType::PLAYER_ENTITY_CREATED:
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
		Uint16 size = Goodbye::writeToBuffer(sendingBuffer, reason);
		sendPacket(client, size);
	}

	//send to all a new connection msg
	void sendNewPlayerNotification(ClientConnection const& newClient) {
		Uint16 size = NewConnection::writeToBuffer(sendingBuffer, newClient.playerName, newClient.playerID, newClient.entityID);
		for (int i = 0; i < connections.size(); i++) {
			if (connections[i].playerID != newClient.playerID)
				sendPacket(connections[i], size);
		}
	}

	void sendChatMessage(std::string const& message, Uint16 senderPlayerID) {
		Uint16 size = NewChatMsg::writeToBuffer(sendingBuffer, message, senderPlayerID);
		for (int i = 0; i < connections.size(); i++) {
			sendPacket(connections[i], size);
		}
	}

	void sendPlayerList(ClientConnection& newClient) {
		Uint16 size = PlayerList::writeToBuffer(sendingBuffer, udpServerPort, connections);
		sendPacket(newClient, size);
	}

	void sendDisconnectionsNotifications() {
		while (!disconnectedPlayersIDToNotify.empty()) {
			Uint16 playerID = disconnectedPlayersIDToNotify.back();
			disconnectedPlayersIDToNotify.pop_back();
			const Uint16 packetSize = NewDisconnection::writeToBuffer(sendingBuffer, playerID);

			//for each client
			for (int i = 0; i < connections.size(); i++) {
				try {
					sendPacket(connections[i], packetSize);
				}
				catch (std::exception& e) {
					closeConnection(i);
				}

			}
		}
	}

	void closeConnection(int clientID);

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
	std::vector<Uint16> disconnectedPlayersIDToNotify;//ids of disconnected players to notify to others

	const int MAX_CLIENTS = 10;
	int connectedClients = 0;//number of connected clients
	std::vector<int> availablePlayerIDs;
	std::vector<Uint16> playerIdToConnectionIndex;
	moodycamel::ReaderWriterQueue<std::unique_ptr<MsgFromGame>> messagesFromGame;
};
