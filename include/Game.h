#pragma once

#include "Components.h"
#include "Collisions.h"
#include "Move.h"
#include "SDL_timer.h"
#include <vector>
#include "SDL_events.h"
#include "Rendering.h"
#include <fstream>
#include <Network/TCPServer.h>
#include <GUI/ScoreBoard.h>
#include <Network/TCPClient.h>
#include <iostream>
#include <GUI/ChatWindow.h>
#include <algorithm>
#include <Network/UDPClient.h>
#include <Network/UDPServer.h>
#include <mutex>
#include <cmath>
#include "EntityManager.h"
#include <Network/NetEntityManager.h>



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
			std::unique_ptr<TCPMessage> new_message;
			while (server.messages.try_dequeue(new_message)){

				activity = true;
				switch (new_message.get()->type) {
					//MESSAGE RECEIVED BY SERVER
				case TcpMsgType::END_OF_THREAD: {
					EndOfThread* msg = static_cast<EndOfThread*>(new_message.get());

					chatWindow.messages.enqueue("Arret du serveur suite a une erreur reseau: " + std::move(msg->message));
					server.stop();
					break;
				}
				case TcpMsgType::SEND_CHAT_MESSAGE: {
					SendChatMsg* msg = static_cast<SendChatMsg*>(new_message.get());
					std::cout << msg->playerName << ": " << msg->message << std::endl;
					chatWindow.messages.enqueue(msg->playerName + ": " + msg->message);
					break;
				}

				case TcpMsgType::CONNECTION_REQUEST: {
					ConnectionRequest* msg = static_cast<ConnectionRequest*>(new_message.get());
					chatWindow.messages.enqueue(msg->playerName + " s'est connecte.");
					std::cout << "New UDP client: " << msg->clientIp << " : " << msg->udpPort;
					udpServer.addClient(msg->playerID, msg->clientIp, msg->udpPort);

					//create new entity for the player
					newEntity = ecs.addEntity();
					std::cout << msg->playerName << " s'est connecte. Entite = " << newEntity << std::endl;
					entityManager.addAllComponents(newEntity, EntityType::PLAYER, spawnPos.x, spawnPos.y, 0, 0);
					entityManager.activateAllComponents(newEntity, EntityType::PLAYER);

					playersInfos.Add(msg->playerID, msg->playerName, msg->playerID, newEntity);
					NetEntityID netEntityID = serverNetEntityManager.addNetEntity(newEntity, EntityType::PLAYER);
					server.playerEntityCreated(msg->playerID, netEntityID);

					break;
				}

				case TcpMsgType::DISCONNECTION_REQUEST: {
					DisconnectionRequest* msg = static_cast<DisconnectionRequest*>(new_message.get());

					std::cout << msg->playerName << " s'est deconnecte." << std::endl;
					chatWindow.messages.enqueue(msg->playerName + " s'est deconnecte.");
					udpServer.removeClient(msg->playerID);
					if (!playersInfos.Contains(msg->playerID)) {
						std::cerr << "\nPlayer ID not found, disconnection failed";
						break;
					}
					PlayerInfos const& infos = playersInfos.Get(msg->playerID);
					entityManager.removeAllComponents(infos.entity, EntityType::PLAYER);
					entityManager.desactivateAllComponents(infos.entity, EntityType::PLAYER);
					ecs.removeEntity(infos.entity);
					serverNetEntityManager.removeNetEntity(infos.entity);

					playersInfos.Remove(msg->playerID);//after that point infos is not valid !
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
			std::unique_ptr<TCPMessage> new_message;
			while (client.receivedMessages.try_dequeue(new_message)){
				activity = true;
				switch (new_message.get()->type) {
					//MESSAGE RECEIVED BY CLIENT
				case TcpMsgType::END_OF_THREAD: {
					EndOfThread* msg = static_cast<EndOfThread*>(new_message.get());
					chatWindow.messages.enqueue("Vous avez ete deconnecte. Raison: " + std::move(msg->message));
					client.stop();
					break;
				}
				case TcpMsgType::GOODBYE: {
					Goodbye* msg = static_cast<Goodbye*>(new_message.get());
					chatWindow.messages.enqueue("Vous avez ete deconnecte par le serveur. Raison: "
						+ std::move(msg->message));
					client.stop();
					break;
				}
				case TcpMsgType::NEW_CONNECTION: {
					NewConnection* msg = static_cast<NewConnection*>(new_message.get());
					std::cout << msg->playerName << " s'est connecte." << std::endl;
					chatWindow.messages.enqueue(msg->playerName + " s'est connecte.");
					playersInfos.Add(msg->playerID, msg->playerName, msg->playerID, 0);
					playersInfos.Get(msg->playerID).netEntityId = msg->entityID;
					break;
				}

				case TcpMsgType::NEW_CHAT_MESSAGE: {
					NewChatMsg* msg = static_cast<NewChatMsg*>(new_message.get());

					if (!playersInfos.Contains(msg->playerID)) {
						std::cerr << "\nPlayer ID not found, receiving chat message failed";
						break;
					}
					PlayerInfos const& infos = playersInfos.Get(msg->playerID);
					std::cout << infos.name << " : " << msg->message << std::endl;
					std::string pname{ infos.name };
					std::transform(pname.begin(), pname.end(), pname.begin(), ::toupper);
					chatWindow.messages.enqueue(pname + " : " + msg->message);
					break;
				}

				case TcpMsgType::NEW_DISCONNECTION: {
					NewDisconnection* msg = static_cast<NewDisconnection*>(new_message.get());

					if (!playersInfos.Contains(msg->playerID)) {
						std::cerr << "\nPlayer ID not found, disconnection failed";
						break;
					}
					PlayerInfos const& infos = playersInfos.Get(msg->playerID);
					std::cout << infos.name << " s'est deconnecte." << std::endl;
					chatWindow.messages.enqueue(msg->playerName + " s'est deconnecte.");
					playersInfos.Remove(msg->playerID);
					break;
				}

				case TcpMsgType::PLAYER_LIST: {
					PlayerList* msg = static_cast<PlayerList*>(new_message.get());

					//start udp client on the port given by server
					udpClient.start(ip, msg->udpPort, udpPort);
					playersInfos.Clear();
					std::cout << "New player list received: \n";
					assert(msg->playerIDs.size() == msg->playerNames.size());
					for (int i = 0; i < msg->playerIDs.size(); i++) {
						std::cout << msg->playerIDs[i] << " : " << msg->playerNames[i]
							<< "(entite = " << msg->entityIDs[i] << ")" << std::endl;
						playersInfos.Add(msg->playerIDs[i], msg->playerNames[i], msg->playerIDs[i], 0);
						playersInfos.Get(msg->playerIDs[i]).netEntityId = msg->entityIDs[i];
					}
					std::cout << "=================" << std::endl;
					break;
				}
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