#pragma once

#include "Components.h"
#include "Collisions.h"
#include "Move.h"
#include "SDL_timer.h"
#include <vector>
#include "SDL_events.h"
#include "Rendering.h"
#include <fstream>
#include "Server.h"
#include "ScoreBoard.h"
#include "Client.h"
#include <iostream>
#include "ChatWindow.h"
#include <algorithm>

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
	Server server;
	Client client;
	bool isHost;
	std::string myName;
	std::string ip;
	Uint16 port;

	//GUI
	ScoreBoard scoreBoard;
	ChatWindow chatWindow;



public:
	Game(bool isHost, std::string const& ip, Uint16 port = 9999) :
		isHost{ isHost },
		renderingManager(),
		scoreBoard(renderingManager),
		chatWindow(renderingManager),
		ip{ip},
		port{port}
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
				case TcpMsgType::SEND_CHAT_MESSAGE:
					std::cout << new_message.playerName << ": " << new_message.message << std::endl;
					chatWindow.messages.enqueue(new_message.playerName + ": " + new_message.message);
					break;

				case TcpMsgType::CONNECTION_REQUEST:
					std::cout << new_message.playerName << " s'est connecte." << std::endl;
					chatWindow.messages.enqueue(new_message.playerName + " s'est connecte.");
					playersInfos.emplace_back(new_message.playerName, new_message.playerID);
					break;

				case TcpMsgType::DISCONNECTION_REQUEST:
					std::cout << new_message.playerName << " s'est deconnecte." << std::endl;
					chatWindow.messages.enqueue(new_message.playerName + " s'est deconnecte.");
					found = false;
					for (int i = 0; i < playersInfos.size(); i++) {
						if (found = (playersInfos[i].id == new_message.playerID)) {
							removeFromVector(playersInfos, i);
							break;
						}
					}
					if (!found) std::cerr << "\nPlayer ID not found, disconnection failed";
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
				//for this we need a moody camel queue for all msg + a cirular queue for text messgaes only
				switch (new_message.type) {
					//MESSAGE RECEIVED BY CLIENT
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
			server.start(port);
		}
		else {
			std::cout << "\nEnter YOUR NAME: ";
			std::cin >> myName;
			client.start(myName, ip, port);

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
		//SDL_PumpEvents();
		/*if (state[SDL_SCANCODE_UP]) {
			player.newSpeed.y = playerMaxSpeed.y;
		}
		else if (state[SDL_SCANCODE_DOWN]) {
			player.newSpeed.y = -1.0f * playerMaxSpeed.y;
		}
		else {
			player.newSpeed.y = 0;
		}*/
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