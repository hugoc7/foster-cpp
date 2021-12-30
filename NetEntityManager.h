#pragma once

#include "EntityManager.h"
#include "UDPClient.h"
#include "UDPServer.h"

//used by server to send networked entities to the clients
class ServerNetEntityManager {
public:
	std::vector<int> localNetEntityIDs;//networked entity ECS ids
	std::vector<int> localNetEntityVersions;
	std::vector<EntityType> localNetEntityTypes;
	std::vector<int> availableNetIDs;//available networked entity netIDs
	std::vector<int> netIDs;//networked entity netIDs

	//Used by server
	//TODO: create a special class to manage all operations on networkd entities !
	NetEntityID addNetEntity(EntityID entityID, EntityType type) {
		assert(localNetEntityTypes.size() == localNetEntityVersions.size());
		assert(localNetEntityIDs.size() == localNetEntityVersions.size());
		assert(netIDs.size() <= localNetEntityVersions.size());

		NetEntityID netID{};

		//no netIDs availables, create a new netID
		if (availableNetIDs.empty()) {
			assert(netIDs.size() == localNetEntityVersions.size());
			netID = localNetEntityIDs.size();

			localNetEntityVersions.push_back(0);
			localNetEntityTypes.push_back(type);
			localNetEntityIDs.push_back(entityID);
		}
		else {
			netID = availableNetIDs.back();
			assert(netID < localNetEntityVersions.size() && netID >= 0);

			availableNetIDs.pop_back();
			localNetEntityTypes[netID] = type;
			localNetEntityIDs[netID] = entityID;
			if (localNetEntityVersions[netID] >= 255)
				localNetEntityVersions[netID] = 0;
			else
				localNetEntityVersions[netID]++;
		}
		netIDs.push_back(netID);
		return netID;
	}
	//Used by server
	//TODO: create a special class to manage all operations on networkd entities !
	void removeNetEntity(EntityID entityID) {
		assert(localNetEntityTypes.size() == localNetEntityVersions.size());
		assert(localNetEntityIDs.size() == localNetEntityVersions.size());
		assert(netIDs.size() <= localNetEntityVersions.size());

		//find and remove netID (we can implement another system)
		NetEntityID netID{ 0 };
		bool found{ false };
		for (int i = 0; i < netIDs.size(); i++) {
			if (localNetEntityIDs.at(netIDs[i]) == entityID) {
				netID = netIDs[i];
				removeFromVector(netIDs, i);
				found = true;
				break;
			}
		}
		assert(found);
		availableNetIDs.push_back(netID);
	}
	void sendNetEntities(UDPServer& udpServer, float deltaTime) {
		udpServer.writeEntitiesToPacket(netIDs, localNetEntityIDs, localNetEntityVersions, localNetEntityTypes, ecs);
	}
};


//used to receive networked entitities with client
class ClientNetEntityManager {
public:
	
	std::vector<bool> hasEntityBeenUpdated;
	std::vector<bool> isEntityActive;
	std::vector<int> netEntityDeleteCountDown;
	std::vector<int> localNetEntityIDs;//networked entity ECS ids
	std::vector<int> localNetEntityVersions;
	std::vector<EntityType> localNetEntityTypes;
	const int netEntityDeleteDelay = 10000;//10 sec

	//WARNING: it modifies the ecs global variable
	void updateNetEntities(UDPClient& udpClient, EntityManager& entityManager, float deltaTime) {
			std::lock_guard<std::mutex> lockClient(udpClient.entitiesMutex);
			std::vector<NetworkEntity> const& packet = udpClient.netEntities;

			std::fill(hasEntityBeenUpdated.begin(), hasEntityBeenUpdated.end(), false);

			assert(localNetEntityVersions.size() == localNetEntityIDs.size());
			assert(localNetEntityVersions.size() == hasEntityBeenUpdated.size());
			assert(localNetEntityVersions.size() == isEntityActive.size());
			assert(localNetEntityVersions.size() == netEntityDeleteCountDown.size());
			assert(localNetEntityVersions.size() == localNetEntityTypes.size());

			for (int i = 0; i < packet.size(); i++) {
				int netID = packet[i].id;
				//std::cout << i << ", " << netID << "\n";
				if (netID < localNetEntityIDs.size()) {
					//add a new entity

					if (!isEntityActive[netID] && netEntityDeleteCountDown[netID] == 0) {
						localNetEntityIDs[netID] = ecs.addEntity();
						isEntityActive[netID] = true;
						localNetEntityTypes[netID] = static_cast<EntityType>(packet[i].type);
						localNetEntityVersions[netID] = packet[i].version;

						entityManager.addAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID],
							packet[i].x, packet[i].y, packet[i].vx, packet[i].vy);
						entityManager.activateAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID]);
					}
					//update existing entitys
					else if (packet[i].version == localNetEntityVersions[netID]) {
						//if the entity was desactivated in the ECS it should be activated
						if (!isEntityActive[netID]) {
							entityManager.activateAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID]);
							isEntityActive[netID] = true;
						}
						entityManager.updateAllComponents(localNetEntityIDs[netID], static_cast<EntityType>(packet[i].type),
							packet[i].x, packet[i].y, packet[i].vx, packet[i].vy);
					}
					//replace existing entity (delete and add)
					else {
						if (isEntityActive[netID]) {
							entityManager.desactivateAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID]);
						}
						entityManager.removeAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID]);

						localNetEntityVersions[netID] = packet[i].version;
						localNetEntityTypes[netID] = static_cast<EntityType>(packet[i].type);
						isEntityActive[netID] = true;
						entityManager.addAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID],
							packet[i].x, packet[i].y, packet[i].vx, packet[i].vy);
						entityManager.activateAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID]);
					}
					hasEntityBeenUpdated[netID] = true;
				}
				//add new entity
				else {
					localNetEntityTypes.resize(netID + 1);
					hasEntityBeenUpdated.resize(netID + 1, false);
					localNetEntityIDs.resize(netID + 1);
					localNetEntityVersions.resize(netID + 1);
					isEntityActive.resize(netID + 1, false);
					netEntityDeleteCountDown.resize(netID + 1, 0);


					localNetEntityIDs[netID] = ecs.addEntity();
					hasEntityBeenUpdated[netID] = true;
					isEntityActive[netID] = true;
					localNetEntityTypes[netID] = static_cast<EntityType>(packet[i].type);
					localNetEntityVersions[netID] = packet[i].version;

					entityManager.addAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID],
						packet[i].x, packet[i].y, packet[i].vx, packet[i].vy);
					entityManager.activateAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID]);
				}
			}


			for (int i = 0; i < hasEntityBeenUpdated.size(); i++) {
				//desactivate entities that are not in the packet
				if (!hasEntityBeenUpdated[i] && isEntityActive[i]) {
					entityManager.desactivateAllComponents(localNetEntityIDs[i], localNetEntityTypes[i]);
					netEntityDeleteCountDown[i] = netEntityDeleteDelay;
					isEntityActive[i] = false;
				}
				//if the entity has been desactivated
				else if (!isEntityActive[i] && netEntityDeleteCountDown[i] > 0) {
					netEntityDeleteCountDown[i] -= std::lround(1000.0f * deltaTime);
					//delete expired entity
					if (netEntityDeleteCountDown[i] <= 0) {
						netEntityDeleteCountDown[i] = 0;
						entityManager.removeAllComponents(localNetEntityIDs[i], localNetEntityTypes[i]);
						ecs.removeEntity(localNetEntityIDs[i]);
						std::cout << "\nremove entity " << localNetEntityIDs[i];
					}
				}
			}
		
	}

};