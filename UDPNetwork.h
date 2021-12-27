#pragma once

#define MAX_UDP_PACKET_SIZE 1200//MTU

#include "SDLNetCpp.h"
#include "Packing.h"
#include "CRC.h"

enum class EntityType : Uint8 {
	PLAYER = 0,
	PROJECTILE = 1,
};

//todo: generalize the use of this type
using NetEntityID = Uint16;

struct NetworkEntity
{
	Uint16 id;
	Uint8 version;
	float x;
	float y;
	float vx;
	float vy;
	Uint8 type;
	NetworkEntity(Uint16 id, Uint8 version, float x, float y, float vx, float vy, Uint8 type) :
		id{ id }, version { version }, x{ x }, y{ y }, type{ type }, vx{ vx }, vy{ vy }
	{
	}
	NetworkEntity() = default;
};

class UDPNetworkNode {
protected:
	UDPsocketObject socket;
	//TODO: replace running (not correct !) by a 8-bit state variable with different states, LAUNCHING, RUNNING, QUITING ETC...
	std::atomic<bool> running{ false };//only main thread can modify it
	std::atomic<bool> error{ false };//only udp network thread can modify it 
	std::thread thread;
	Uint16 myPort;

public:
	std::mutex entitiesMutex;

	//The sequence number of the last packet received, used to check if what we received is more recent
	//Uint16 lastIncomingPacketNumber{ 0xFFFF };
	//The sequence number of the last packet sent
	//Uint16 outgoingPacketNumber{ 0 };

	
	UDPNetworkNode() {};



	//receive a valid packet (correct checksum and sequence number)
	/*bool receive(UDPpacketObject& packet) {
		if (!socket.recv(packet)) return false;

		assert(packet.packet->len > 4 && packet.packet->len <= MAX_UDP_PACKET_SIZE);

		//Verify checksum for integrity (against random errors)
		if (!verifyChecksum(packet)) return false;

		Uint16 incomingPacketNumber = Packing::ReadUint16(packet.packet->data);
		if (!isGreaterModulo(incomingPacketNumber, lastIncomingPacketNumber)) return false;

		lastIncomingPacketNumber = incomingPacketNumber;
		return true;
	}*/
	//return true if a > b modulo 2 pow 16
	bool isGreaterModulo(Uint16 a, Uint16 b)
	{
		if (a > b)
			return (a - b) <= 32768;
		else if (b > a)
			return (b - a) > 32768;
		else
			return false;
	}
	//calculate a 16 bit checksum on a UDP packet
	void calculateChecksum(UDPpacketObject& packet) {
		assert(packet.packet->len >= 3);
		Uint16 checksum = CRC::Calculate(packet.packet->data, packet.packet->len-2, CRC::CRC_16_ARC());
		Packing::WriteUint16(checksum, &packet.packet->data[packet.packet->len - 2]);
	}
	//verify a 16 bit checksum on a UDP packet
	bool verifyChecksum(UDPpacketObject& packet) {
		assert(packet.packet->len >= 3);
		Uint16 checksum = CRC::Calculate(packet.packet->data, packet.packet->len - 2, CRC::CRC_16_ARC());
		return checksum == Packing::ReadUint16(&packet.packet->data[packet.packet->len - 2]);
	}

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
