#pragma once


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
