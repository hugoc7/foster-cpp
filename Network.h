#pragma once

#include "SDL_net.h"
#include <iostream>
#include <string>
#include <cassert>
#include "Containers.h"

std::string getIpAdressAsString(Uint32 ip);

#define MAX_TCP_PACKET_SIZE 1200 //it is not the MTU though I put the same value ^^

enum class TcpMsgType {
	CONNECTION_REQUEST = 0,
	DISCONNECTION_REQUEST = 1,
	SEND_CHAT_MESSAGE = 2,
	COUNT = 3,
	//max = 255
};

struct TCPmessage {
	TcpMsgType type;
	std::string message;//can be either a text message or a player name
	std::string playerName;
	int playerID;//unique ID of a player related to this message (sender/receiver)

	TCPmessage() = default;
	TCPmessage(TCPmessage&& other) = default;

	TCPmessage(TcpMsgType&& type, std::string&& playerName, int playerID) :
		playerName{ std::move(playerName) },
		type{ std::move(type) },
		playerID{ playerID }
	{
	}
	TCPmessage(UniqueByteBuffer const& buffer, int bufferSize) :
		type{ static_cast<TcpMsgType>(SDLNet_Read16(buffer.get()+2)) }, playerID{playerID}
	{
		switch (type) {
		case TcpMsgType::CONNECTION_REQUEST:
			playerName.assign(buffer.get() + 4, bufferSize - 4);
			break;
		case TcpMsgType::SEND_CHAT_MESSAGE:
			message.assign(buffer.get() + 4, bufferSize - 4);
			break;
		case TcpMsgType::DISCONNECTION_REQUEST:
			//nothing
			break;
		default:
			std::cerr << "Invalid TCP message type\n";
			throw std::runtime_error("Invalid TCP message type\n");
		}
	}
	TCPmessage& operator=(const TCPmessage& other) = default; /*{
		type = other.type;
		message = other.message;
		return *this;
	}*/
};


//should a method of the server classvz
/*
class NetworkPlayersManager {
	public:
		std::vector<std::string> playerNames;//index by clientID

		void serverAddNewPlayer(TCPmessage& msg) {
			playerNames.insert(playerNames.begin()+msg.clientID, msg.message);
		}
};*/

/*
* PROCESSUS : PACKET UDP => NetworkEntity To Entity Vector => ECS
* ================================================================
*/
#include <mutex>
//the UDP packet a client receive is a vector of NetworkEntitiess
/*
struct NetworkEntity {
	int networkEntityID;
	int version;
	Vector2 position;
};

struct NetworkEntityManager {
	std::vector<int> versions;
	std::vector<int> entityIDs;
	std::vector<bool> updated;

	//update an entity in the ECS (here position)
	void updateECS(int entity, Vector2 position) {

	}
	int ECS_add(Vector2 position) {
		return 4;//the id of the new entity
	}
	void ECS_remove(int entity) {

	}
	void ECS_desactivate(int entity) {

	}

	void updateNetworkEntities(std::vector<NetworkEntity> const& packet){

		//=> lock in read mode the packet

		std::fill(updated.begin(), updated.end(), false);

		for (int i = 0; i < packet.size(); i++) {
			int netID = packet[i].networkEntityID;
			if (netID < entityIDs.size()) {
				//update existing
				if (packet[i].version == versions[netID]) {
					//if the entity was desactivated in the ECS it should be activated
					updateECS(entityIDs[netID], packet[i].position);
				}
				//replace existing (delete and add)
				else {
					//here an information about the type of entity is missing = what are the components owned by the entity
					ECS_remove(entityIDs[netID]);
					versions[netID] = packet[i].version;
					entityIDs[netID] = ECS_add(packet[i].position);
				}
				updated[netID] = true;

			}
			//add new
			else {
				versions.insert(versions.begin()+netID, packet[i].version);
				entityIDs.insert(versions.begin() + netID, ECS_add(packet[i].position));
				//updated.insert(versions.begin() + netID, true);
			}
		}

	
		for (int i = 0; i < updated.size(); i++) {
			if (!updated[i]) {
				ECS_desactivate(i);
			}
		}

		//unlock packet
	}

};*/