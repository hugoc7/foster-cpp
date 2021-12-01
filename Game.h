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

class Game {
private:
	int lastTime{ 0 }, currentTime{ 0 };
	const float FPS{ 60.0 };
	float deltaTime{}, refreshTimeInterval{ 1.0f / FPS};
	ArrayList<EntityID> plateforms{};
	ArrayList<EntityID> players{};
	/*std::vector<VisibleObject> plateforms{};
	std::vector<BoxCollider> plateformColliders{};
	std::vector<MovingObject> players{};
	std::vector<BoxCollider> playersColliders{};*/
	Renderer renderingManager{};
	Vector2 playerMaxSpeed{ 7.0f, 10.0f};
	EntityID player;

	//Network
	std::vector<PlayerInfos> playersInfos;
	TCPServer server;
	TCPClient client;
	bool isHost;
	std::string myName;
	std::string ip;
	Uint16 port;
	Uint16 udpPort;
	UDPClient udpClient;
	UDPServer udpServer;

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
		player = ecs.addEntity();
		ecs.addComponent<MovingObject>(player, 4.5, 7);
		ecs.addComponent<BoxCollider>(player, 1, 2);
		players.insert(player);
		//players.emplace_back(4.5, 7);
		//playersColliders.emplace_back(1, 2);

		readPlateformsFromFile("map.txt", plateforms);
		/*plateforms.emplace_back(4.5f, 3.5f);
		plateformColliders.emplace_back(5.0f, 1.0f);

		plateforms.emplace_back(9.5f, 5.5f);
		plateformColliders.emplace_back(3.0f, 1.0f);*/

		
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
		if (isHost) {
			//server.messages.lockQueue();
			TCPmessage new_message;
			while (server.messages.try_dequeue(new_message)){//!server.messages.empty()) {

				activity = true;
				//TCPmessage const& new_message{ server.messages.read(0) };
				switch (new_message.type) {
					//MESSAGE RECEIVED BY SERVER
				case TcpMsgType::END_OF_THREAD:
					chatWindow.messages.enqueue("Arret du serveur suite à une erreur réseau: " + std::move(new_message.message));
					server.stop();
					break;
				case TcpMsgType::SEND_CHAT_MESSAGE:
					std::cout << new_message.playerName << ": " << new_message.message << std::endl;
					chatWindow.messages.enqueue(new_message.playerName + ": " + new_message.message);
					break;

				case TcpMsgType::CONNECTION_REQUEST:
					std::cout << new_message.playerName << " s'est connecte." << std::endl;
					chatWindow.messages.enqueue(new_message.playerName + " s'est connecte.");
					std::cout << "New UDP client: " << new_message.clientIp << " : " << new_message.udpPort;
					udpServer.addClient(new_message.playerID, new_message.clientIp, new_message.udpPort);
					playersInfos.emplace_back(new_message.playerName, new_message.playerID);
					break;

				case TcpMsgType::DISCONNECTION_REQUEST:
					std::cout << new_message.playerName << " s'est deconnecte." << std::endl;
					chatWindow.messages.enqueue(new_message.playerName + " s'est deconnecte.");
					udpServer.removeClient(new_message.playerID);
					found = false;
					for (int i = 0; i < playersInfos.size(); i++) {
						if (found = (playersInfos[i].id == new_message.playerID)) {
							removeFromVector(playersInfos, i);
							break;
						}
					}
					if (!found) std::cerr << "\nPlayer ID not found, disconnection failed";
					break;
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
					playersInfos.emplace_back(new_message.playerName, new_message.playerID);
					break;

				case TcpMsgType::NEW_CHAT_MESSAGE:
					found = false;
					for (int i = 0; i < playersInfos.size(); i++) {
						if (found = (playersInfos[i].id == new_message.playerID)) {
							std::cout << playersInfos[i].name << " : " << new_message.message << std::endl;
							std::string pname{ playersInfos[i].name };
							std::transform(pname.begin(), pname.end(), pname.begin(), ::toupper);
							chatWindow.messages.enqueue(pname + " : " + new_message.message);
							break;
						}
					}
					if (!found) std::cerr << "\nPlayer ID not found, receiving chat message failed";
					break;

				case TcpMsgType::NEW_DISCONNECTION:
					found = false;
					for (int i = 0; i < playersInfos.size(); i++) {
						if (found = (playersInfos[i].id == new_message.playerID)) {
							std::cout << playersInfos[i].name << " s'est deconnecte." << std::endl;
							chatWindow.messages.enqueue(new_message.playerName + " s'est deconnecte.");
							removeFromVector(playersInfos, i);
							break;
						}
					}
					if (!found) std::cerr << "\nPlayer ID not found, disconnection failed";
					break;

				case TcpMsgType::PLAYER_LIST:
					//start udp client on the port given by server
					udpClient.start(ip, new_message.udpPort, udpPort);
					playersInfos.clear();
					std::cout << "New player list received: \n";
					assert(new_message.playerIDs.size() == new_message.playerNames.size());
					for (int i = 0; i < new_message.playerIDs.size(); i++) {
						std::cout << new_message.playerIDs[i] << " : " << new_message.playerNames[i] << std::endl;
						playersInfos.emplace_back(new_message.playerNames[i], new_message.playerIDs[i]);
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

				lastTime = currentTime;
				handleInputs(quit, renderingManager, player, scoreBoard, chatWindow);
				updateMovementBeforeCollision(players, deltaTime);

				//sliding must be checked before collisions !
				detectStaticSliding(players, plateforms);
				detectStaticCollisions(players, plateforms);

				applyMovement(players, deltaTime);//also apply gravity


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
		MovingObject& player{ ecs.getComponent<MovingObject>(playerID) };
		//TODO: faire en sorte d'analyser tous les events de la file d'attente et pas juste le premier
		
		const Uint8* state = SDL_GetKeyboardState(NULL);
		if (!isHost) {
			chatWin.handleEvents(e, client);
		}
		scoreBoard.handleEvents(e);

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
	}
};