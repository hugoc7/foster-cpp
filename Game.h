#pragma once

#include "Components.h"
#include "Collisions.h"
#include "Move.h"
#include "SDL_timer.h"
#include <vector>
#include "SDL_events.h"
#include "Rendering.h"
#include <fstream>
#include "TCPServer.h"
#include "ScoreBoard.h"
#include "TCPClient.h"
#include <iostream>
#include "ChatWindow.h"
#include <algorithm>
#include "UDPClient.h"
#include "UDPServer.h"
#include <mutex>
#include <cmath>



class Game {
private:
	int lastTime{ 0 }, currentTime{ 0 };
	const float FPS{ 60.0 };
	float deltaTime{}, refreshTimeInterval{ 1.0f / FPS};
	ArrayList<EntityID> plateforms{};
	ArrayList<EntityID> players{};
	Renderer renderingManager{};
	Vector2 playerMaxSpeed{ 7.0f, 10.0f};
	EntityID player;
	Vector2 spawnPos{4.5, 7};
	Vector2 charactersDimensions{1, 2};

	//Network
	ArrayMap<PlayerInfos> playersInfos;
	TCPServer server;
	TCPClient client;
	bool isHost;
	std::string myName;
	std::string ip;
	Uint16 port;
	Uint16 udpPort;
	UDPClient udpClient;
	UDPServer udpServer;
	std::vector<EntityID> localEntityIds;
	//std::vector<std::pair<int, Uint8>> clientInputs;//first = player ID ; second = keyboard input
	
	//TODO: put all this in a class which manage networked entities !
	//==================================================================
	//std::vector<NetworkEntity> localNetEntities;
	std::vector<bool> hasEntityBeenUpdated;
	std::vector<bool> isEntityActive;
	std::vector<int> netEntityDeleteCountDown;
	std::vector<int> localNetEntityIDs;//networked entity ECS ids
	std::vector<int> localNetEntityVersions;
	std::vector<EntityType> localNetEntityTypes;
	std::vector<int> availableNetIDs;//available networked entity netIDs
	std::vector<int> netIDs;//networked entity netIDs
	const int netEntityDeleteDelay = 10000;//10 sec
	//==================================================================



	//GUI
	ScoreBoard scoreBoard;
	ChatWindow chatWindow;



public:
	Game(bool isHost, std::string const& ip, Uint16 port, Uint16 udpPort) :
		isHost{ isHost },
		renderingManager(),
		scoreBoard(renderingManager),
		chatWindow(renderingManager),
		ip{ip},
		port{port},
		udpPort{udpPort}
	{
		if (isHost) {
			player = ecs.addEntity();
			ecs.addComponent<MovingObject>(player, 4.5, 7);
			ecs.addComponent<BoxCollider>(player, 1, 2);
			players.insert(player);
		}
		readPlateformsFromFile("map.txt", plateforms);
	}
	//TODO: put this 5 followinf fucntions in a class NetworkManager
	//remove the components of an entity
	void removeAllComponents(EntityID entity, EntityType type) {
		if (type == EntityType::PLAYER) {
			ecs.removeComponent<MovingObject>(entity);
			ecs.removeComponent<BoxCollider>(entity);
		}
	}
	//desactivate the components of an entity
	void desactivateAllComponents(EntityID entity, EntityType type) {
		if (type == EntityType::PLAYER) {
			players.erase(entity);
		}
	}
	//activate the components of an entity
	void activateAllComponents(EntityID entity, EntityType type) {
		if (type == EntityType::PLAYER) {
			players.insert(entity);
		}
	}
	//add all components of an entity
	void addAllComponents(EntityID entity, EntityType type, float x, float y, float vx, float vy) {
		if (type == EntityType::PLAYER) {
			ecs.addComponent<MovingObject>(entity, x, y, vx, vy);
			ecs.addComponent<BoxCollider>(entity, charactersDimensions.x, charactersDimensions.y);
		}
	}
	//udate all components of an entity
	void updateAllComponents(EntityID entity, EntityType type, float x, float y, float vx, float vy) {
		if (type == EntityType::PLAYER) {
			MovingObject& moveComp = ecs.getComponent<MovingObject>(entity);
			moveComp.position.x = x;
			moveComp.position.y = y;
			moveComp.newSpeed.x = vx;
			moveComp.newSpeed.y = vy;
			moveComp.oldPosition = moveComp.position;//maybe useful ? I dont remember
		}
	}

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
		NetEntityID netID{0};
		bool found{false};
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

	//WARNING: it modifies the ecs global variable
	void updateNetEntitiesPos(float deltaTime) {

		//client side
		if (!isHost) {
			std::lock_guard<std::mutex> lockClient(udpClient.entitiesMutex);
			std::vector<NetworkEntity> const& packet = udpClient.netEntities;

			/*
			//1-1 correspondance between packet array and localEntitiesID array
			for (int i = 0; i < packet.size(); i++) {
				//create new entity
				if (i >= localEntityIds.size()) {
					EntityID id = ecs.addEntity();
					ecs.addComponent<MovingObject>(id, packet[i].x, packet[i].y);
					ecs.getComponent<MovingObject>(id).newSpeed = Vector2(packet[i].vx, packet[i].vy);
					ecs.addComponent<BoxCollider>(id, 1, 2);
					players.insert(id);
					localEntityIds.push_back(id);
				}
				//update entity
				else {
					MovingObject& moveComp = ecs.getComponent<MovingObject>(localEntityIds[i]);
					moveComp.position.x = packet[i].x;
					moveComp.position.y = packet[i].y;
					moveComp.newSpeed.x = packet[i].vx;
					moveComp.newSpeed.y = packet[i].vy;
					moveComp.oldPosition = moveComp.position;//maybe useful ? I dont remember
				}
			}*/

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

						addAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID],
							packet[i].x, packet[i].y, packet[i].vx, packet[i].vy);
						activateAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID]);
					}
					//update existing entitys
					else if (packet[i].version == localNetEntityVersions[netID]) {
						//if the entity was desactivated in the ECS it should be activated
						if (!isEntityActive[netID]) {
							activateAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID]);
							isEntityActive[i] = true;
						}
						updateAllComponents(localNetEntityIDs[netID], static_cast<EntityType>(packet[i].type), 
							packet[i].x, packet[i].y, packet[i].vx, packet[i].vy);
					}
					//replace existing entity (delete and add)
					else {
						if (isEntityActive[netID]) {
							desactivateAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID]);
						}
						removeAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID]);

						localNetEntityVersions[netID] = packet[i].version;
						localNetEntityTypes[netID] = static_cast<EntityType>(packet[i].type);
						isEntityActive[netID] = true;
						addAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID],
							packet[i].x, packet[i].y, packet[i].vx, packet[i].vy);
						activateAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID]);
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

					addAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID],
						packet[i].x, packet[i].y, packet[i].vx, packet[i].vy);
					activateAllComponents(localNetEntityIDs[netID], localNetEntityTypes[netID]);
				}
			}

			
			for (int i = 0; i < hasEntityBeenUpdated.size(); i++) {
				//desactivate entities that are not in the packet
				if (!hasEntityBeenUpdated[i] && isEntityActive[i]) {
					desactivateAllComponents(localNetEntityIDs[i], localNetEntityTypes[i]);
					netEntityDeleteCountDown[i] = netEntityDeleteDelay;
					isEntityActive[i] = false;
				}
				else if (netEntityDeleteCountDown[i] > 0) {
					netEntityDeleteCountDown[i] -= std::lround(1000.0f * deltaTime);
					//delete expired entity
					if (netEntityDeleteCountDown[i] <= 0) {
						netEntityDeleteCountDown[i] = 0;
						removeAllComponents(localNetEntityIDs[i], localNetEntityTypes[i]);
						ecs.removeEntity(localNetEntityIDs[i]);
					}
				}
			}
		}
		//server side
		else {
			udpServer.writeEntitiesToPacket(netIDs, localNetEntityIDs, localNetEntityVersions, localNetEntityTypes, ecs);
		}
	}


	void readPlateformsFromFile(std::string const& filename, ArrayList<EntityID>& plateforms) {
		std::ifstream file(filename);
		float x, y, w, h;
		while (!file.eof() && file.good()) {
			file >> x >> y >> w >> h;
			//std::cout << x << ", " << y << "  ;  " << w << ", " << h << std::endl;
			EntityID plateform = ecs.addEntity();
			ecs.addComponent<VisibleObject>(plateform, x, y);
			ecs.addComponent<BoxCollider>(plateform, w, h);
			plateforms.insert(plateform);
			//plateforms.emplace_back(x, y);
			//plateformColliders.emplace_back(w, h);
		}
	}

	void handleNetwork(ChatWindow& chatWindow) {
		//Show chat messages in the console
		bool found{ false };
		bool activity{ false };
		EntityID newEntity;
		if (isHost) {
			//server.messages.lockQueue();
			TCPmessage new_message;
			while (server.messages.try_dequeue(new_message)){//!server.messages.empty()) {

				activity = true;
				//TCPmessage const& new_message{ server.messages.read(0) };
				switch (new_message.type) {
					//MESSAGE RECEIVED BY SERVER
				case TcpMsgType::END_OF_THREAD:
					chatWindow.messages.enqueue("Arret du serveur suite a une erreur reseau: " + std::move(new_message.message));
					server.stop();
					break;
				case TcpMsgType::SEND_CHAT_MESSAGE:
					std::cout << new_message.playerName << ": " << new_message.message << std::endl;
					chatWindow.messages.enqueue(new_message.playerName + ": " + new_message.message);
					break;

				case TcpMsgType::CONNECTION_REQUEST: {
					chatWindow.messages.enqueue(new_message.playerName + " s'est connecte.");
					std::cout << "New UDP client: " << new_message.clientIp << " : " << new_message.udpPort;
					udpServer.addClient(new_message.playerID, new_message.clientIp, new_message.udpPort);

					//create new entity for the player
					newEntity = ecs.addEntity();
					std::cout << new_message.playerName << " s'est connecte. Entite = " << newEntity << std::endl;
					ecs.addComponent<MovingObject>(newEntity, spawnPos.x, spawnPos.y);
					ecs.addComponent<BoxCollider>(newEntity, 1, 2);
					players.insert(newEntity);

					playersInfos.Add(new_message.playerID, new_message.playerName, new_message.playerID, newEntity);
					//TODO: convert newEntityID to network entity ID !!!
					NetEntityID netEntityID = addNetEntity(newEntity, EntityType::PLAYER);
					server.playerEntityCreated(new_message.playerID, netEntityID);

					break;
				}

				case TcpMsgType::DISCONNECTION_REQUEST: {
					std::cout << new_message.playerName << " s'est deconnecte." << std::endl;
					chatWindow.messages.enqueue(new_message.playerName + " s'est deconnecte.");
					udpServer.removeClient(new_message.playerID);
					if (!playersInfos.Contains(new_message.playerID)) {
						std::cerr << "\nPlayer ID not found, disconnection failed";
						break;
					}
					PlayerInfos const& infos = playersInfos.Get(new_message.playerID);
					removeAllComponents(infos.entity, EntityType::PLAYER);
					desactivateAllComponents(infos.entity, EntityType::PLAYER);
					ecs.removeEntity(infos.entity);
					removeNetEntity(infos.entity);

					playersInfos.Remove(new_message.playerID);//after that point infos is not valid !
					break;
				}
					
				default:
					std::cerr << "Message TCP non traite par le jeu";
					//TODO: handle this error in TCPclient and server !
					break;
				}
				//server.messages.dequeue();
			}
			//.unlockQueue();
		}
		else {
			TCPmessage new_message;
			while (client.receivedMessages.try_dequeue(new_message)){
				activity = true;
				//TODO: move message content instead of copying them
				switch (new_message.type) {
					//MESSAGE RECEIVED BY CLIENT
				case TcpMsgType::END_OF_THREAD:
					chatWindow.messages.enqueue("Vous avez ete deconnecte. Raison: " + std::move(new_message.message));
					client.stop();
					break;
				case TcpMsgType::GOODBYE:
					chatWindow.messages.enqueue("Vous avez ete deconnecte par le serveur. Raison: " 
						+ std::move(new_message.message));
					client.stop();
					break;
				case TcpMsgType::NEW_CONNECTION:
					std::cout << new_message.playerName << " s'est connecte." << std::endl;
					chatWindow.messages.enqueue(new_message.playerName + " s'est connecte.");
					assert(!new_message.entityIDs.empty());
					playersInfos.Add(new_message.playerID, new_message.playerName, new_message.playerID, 0);
					playersInfos.Get(new_message.playerID).netEntityId = new_message.entityIDs.at(0);
					break;

				case TcpMsgType::NEW_CHAT_MESSAGE: {
					if (!playersInfos.Contains(new_message.playerID)) {
						std::cerr << "\nPlayer ID not found, receiving chat message failed";
						break;
					}
					PlayerInfos const& infos = playersInfos.Get(new_message.playerID);
					std::cout << infos.name << " : " << new_message.message << std::endl;
					std::string pname{ infos.name };
					std::transform(pname.begin(), pname.end(), pname.begin(), ::toupper);
					chatWindow.messages.enqueue(pname + " : " + new_message.message);
					break;
				}

				case TcpMsgType::NEW_DISCONNECTION: {
					if (!playersInfos.Contains(new_message.playerID)) {
						std::cerr << "\nPlayer ID not found, disconnection failed";
						break;
					}
					PlayerInfos const& infos = playersInfos.Get(new_message.playerID);
					std::cout << infos.name << " s'est deconnecte." << std::endl;
					chatWindow.messages.enqueue(new_message.playerName + " s'est deconnecte.");
					playersInfos.Remove(new_message.playerID);
					break;
				}

				case TcpMsgType::PLAYER_LIST:
					//start udp client on the port given by server
					udpClient.start(ip, new_message.udpPort, udpPort);
					playersInfos.Clear();
					std::cout << "New player list received: \n";
					assert(new_message.playerIDs.size() == new_message.playerNames.size());
					for (int i = 0; i < new_message.playerIDs.size(); i++) {
						std::cout << new_message.playerIDs[i] << " : " << new_message.playerNames[i] 
							<< "(entite = "<< new_message.entityIDs[i] << ")" << std::endl;

						//NEW: SEND ALL PLAYER'S ENTITY IDs IN PLAYER_LIST's PACKET !!!!
						//===============================
						playersInfos.Add(new_message.playerIDs[i], new_message.playerNames[i], new_message.playerIDs[i], 0);
						playersInfos.Get(new_message.playerIDs[i]).netEntityId = new_message.entityIDs[i];
					}
					std::cout << "=================" << std::endl;
					break;
				}
			}
		}
		if (activity) {
			chatWindow.updateText();
		}
	}

	void gameLoop() {
		bool quit = false;
		//Network
		if (isHost) {
			server.start(port, udpPort);
			udpServer.start(udpPort);
		}
		else {
			std::cout << "\nEnter YOUR NAME: ";
			std::cin >> myName;
			client.start(myName, ip, port, udpPort);

		}

		while (!quit) {
			updateDeltaTime();

			if (deltaTime >= refreshTimeInterval) {
				handleNetwork(chatWindow);

				updateNetEntitiesPos(deltaTime);

				lastTime = currentTime;
				handleInputs(quit, renderingManager, player, scoreBoard, chatWindow);
				updateMovementBeforeCollision(players, deltaTime);

				//sliding must be checked before collisions !
				detectStaticSliding(players, plateforms);
				detectStaticCollisions(players, plateforms);

				if (isHost) {
					applyMovement(players, deltaTime);//also apply gravity
				}


				renderingManager.render(players, plateforms);
				scoreBoard.render(playersInfos, myName);


				chatWindow.render();



				renderingManager.endRendering();
			}
			else
				SDL_Delay(lastTime + refreshTimeInterval * 1000 - currentTime);
			
		}
		//Close Network threads
		server.stop();
		client.stop();
		udpClient.stop();
		udpServer.stop();
	};
	void updateDeltaTime() {
		currentTime = SDL_GetTicks();
		deltaTime = float(currentTime - lastTime) / 1000.0f;
	};

	///@brief une classe sera dedicasse a la gestion d'evenements
	void handleInputs(bool &quit, Renderer& renderer, EntityID playerID, ScoreBoard& scoreBoard, ChatWindow& chatWin) {
		SDL_Event e;
		SDL_PollEvent(&e);
		
		//TODO: faire en sorte d'analyser tous les events de la file d'attente et pas juste le premier
		
		const Uint8* state = SDL_GetKeyboardState(NULL);
		if (!isHost) {
			chatWin.handleEvents(e, client);
		}
		scoreBoard.handleEvents(e);

		//HOST
		if (isHost) {
			MovingObject& player{ ecs.getComponent<MovingObject>(playerID) };

			if (state[SDL_SCANCODE_RIGHT]) {
				player.newSpeed.x = playerMaxSpeed.x;
			}
			else if (state[SDL_SCANCODE_LEFT]) {
				player.newSpeed.x = -1.0f * playerMaxSpeed.x;
			}
			else {
				player.newSpeed.x = 0;
			}
			switch (e.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_WINDOWEVENT:
				renderer.getNewWindowSize();
				scoreBoard.reloadFont();
				chatWin.reloadFont();
				break;
			case SDL_KEYDOWN:
					if (e.key.keysym.sym == SDLK_UP)
						player.newSpeed.y = playerMaxSpeed.y;
				break;
			}

			//Handle network inputs
			//======================
			std::vector<Uint8> oldNetInputs;
			for (int i = 0; i < playersInfos.Size(); i++) {
				oldNetInputs.push_back(playersInfos[i].controls);
			}
			udpServer.getClientInputs(playersInfos);
			for (int i = 0; i < playersInfos.Size(); i++) {
				MovingObject& currPlayer{ ecs.getComponent<MovingObject>(playersInfos[i].entity) };
				//right
				if (playersInfos[i].controls & 1) {
					currPlayer.newSpeed.x = playerMaxSpeed.x;
				}
				//left
				else if (playersInfos[i].controls & 2) {
					currPlayer.newSpeed.x = -1.0f * playerMaxSpeed.x;
				}
				else {
					currPlayer.newSpeed.x = 0;
				}
				//up
				if (!(oldNetInputs[i] & 4) && (playersInfos[i].controls & 4)) {
					currPlayer.newSpeed.y = playerMaxSpeed.y;
				}
			}
		}
		//CLIENT
		else {
			Uint8 netInputState = 0;
			if (state[SDL_SCANCODE_RIGHT]) {
				netInputState |= 1;
 			}
			if (state[SDL_SCANCODE_LEFT]) {
				netInputState |= 2;
			}
			if (state[SDL_SCANCODE_UP]) {
				netInputState |= 4;
			}
			udpClient.userInputs = netInputState;

			switch (e.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_WINDOWEVENT:
				renderer.getNewWindowSize();
				scoreBoard.reloadFont();
				chatWin.reloadFont();
				break;
			}
		}
	}
};