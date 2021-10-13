#pragma once

#include "Components.h"
#include "Collisions.h"
#include "Move.h"
#include "SDL_timer.h"
#include <vector>
#include "SDL_events.h"
#include "Rendering.h"
#include <fstream>


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


public:
	Game() {
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
			std::cout << x << ", " << y << "  ;  " << w << ", " << h << std::endl;
			EntityID plateform = ecs.addEntity();
			ecs.addComponent<VisibleObject>(plateform, x, y);
			ecs.addComponent<BoxCollider>(plateform, w, h);
			plateforms.insert(plateform);
			//plateforms.emplace_back(x, y);
			//plateformColliders.emplace_back(w, h);
		}
	}

	void gameLoop() {
		bool quit = false;
		while (!quit) {
			updateDeltaTime();
			if (deltaTime >= refreshTimeInterval) {
				lastTime = currentTime;
				handleInputs(quit, renderingManager, player);
				updateMovementBeforeCollision(players, deltaTime);

				//sliding must be checked before collisions !
				detectStaticSliding(players, plateforms);
				detectStaticCollisions(players, plateforms);

				applyMovement(players, deltaTime);//also apply gravity
				renderingManager.render(players, plateforms);
			}
			else
				SDL_Delay(lastTime + refreshTimeInterval * 1000 - currentTime);
			
		}
	};
		
	void updateDeltaTime() {
		currentTime = SDL_GetTicks();
		deltaTime = float(currentTime - lastTime) / 1000.0f;
	};

	///@brief une classe sera dedicasse a la gestion d'evenements
	void handleInputs(bool &quit, Renderer& renderer, EntityID playerID) {
		SDL_Event e;
		SDL_PollEvent(&e);
		MovingObject& player{ ecs.getComponent<MovingObject>(playerID) };

		
		const Uint8* state = SDL_GetKeyboardState(NULL);
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
				break;
			case SDL_KEYDOWN:
				if (e.key.keysym.sym == SDLK_UP)
					player.newSpeed.y = playerMaxSpeed.y;
				break;
		}
	}
};