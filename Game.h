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
#include "EntityManager.h"
#include "NetEntityManager.h"



class Game {
private:
	int lastTime{ 0 }, currentTime{ 0 };
	const float FPS{ 60.0 };
	float deltaTime{}, refreshTimeInterval{ 1.0f / FPS};
	
	Renderer renderingManager{};
	Vector2 playerMaxSpeed{ 7.0f, 10.0f};
	EntityID player;
	Vector2 spawnPos{4.5, 7};
	Vector2 charactersDimensions{1, 2};
	EntityManager entityManager;

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
	ClientNetEntityManager clientNetEntityManager;
	ServerNetEntityManager serverNetEntityManager;

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
			entityManager.addAllComponents(player, EntityType::PLAYER, spawnPos.x, spawnPos.y, 0, 0);
			entityManager.activateAllComponents(player, EntityType::PLAYER);
		}
		readPlateformsFromFile("map.txt");
	}
	

	


	void readPlateformsFromFile(std::string const& filename) {
		std::ifstream file(filename);
		float x, y, w, h;
		while (!file.eof() && file.good()) {
			file >> x >> y >> w >> h;
			entityManager.addPlateform(x, y, w, h);
		}
	}

	void handleNetwork(ChatWindow& chatWindow) {
		bool found{ false };
		bool activity{ false };
		EntityID newEntity;

		//SERVER
		if (isHost) {
			TCPmessage new_message;
			while (server.messages.try_dequeue(new_message)){

				activity = true;
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
					entityManager.addAllComponents(newEntity, EntityType::PLAYER, spawnPos.x, spawnPos.y, 0, 0);
					entityManager.activateAllComponents(newEntity, EntityType::PLAYER);

					playersInfos.Add(new_message.playerID, new_message.playerName, new_message.playerID, newEntity);
					//TODO: convert newEntityID to network entity ID !!!
					NetEntityID netEntityID = serverNetEntityManager.addNetEntity(newEntity, EntityType::PLAYER);
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
					entityManager.removeAllComponents(infos.entity, EntityType::PLAYER);
					entityManager.desactivateAllComponents(infos.entity, EntityType::PLAYER);
					ecs.removeEntity(infos.entity);
					serverNetEntityManager.removeNetEntity(infos.entity);

					playersInfos.Remove(new_message.playerID);//after that point infos is not valid !
					break;
				}
					
				default:
					std::cerr << "Message TCP non traite par le jeu";
					//TODO: handle this error in TCPclient and server !
					break;
				}
			}
			
		}

		//CLIENT
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
				handleNetwork(chatWindow);//handle tcp msg

				if (isHost) {
					serverNetEntityManager.sendNetEntities(udpServer, deltaTime);
				}
				else {
					clientNetEntityManager.updateNetEntities(udpClient, entityManager, deltaTime);
				}

				lastTime = currentTime;
				handleInputs(quit, renderingManager, player, scoreBoard, chatWindow);
				updateMovementBeforeCollision(deltaTime);

				//sliding must be checked before collisions !
				detectStaticSliding(entityManager.players, entityManager.plateforms);
				detectStaticCollisions(entityManager.players, entityManager.plateforms);

				if (isHost) {
					applyMovement(deltaTime);//also apply gravity
				}


				renderingManager.render(entityManager.players, entityManager.plateforms);
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