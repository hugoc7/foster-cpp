#pragma once

#include "Components.h"
#include "Collisions.h"
#include "Move.h"
#include "SDL_timer.h"
#include <vector>
#include "SDL_events.h"
#include "Rendering.h"

/*
* TODO:
* Une fonction pour passer des coordonnées du monde aux coordonnées de l'écran, viewport ?
*/

class Game {
private:
	int lastTime{ 0 }, currentTime{ 0 };
	const float FPS = 80.0;
	float deltaTime{}, refreshTimeInterval{ 1.0f / FPS};
	std::vector<VisibleObject> plateforms{};
	std::vector<BoxCollider> plateformColliders{};
	std::vector<MovingObject> players{};
	std::vector<BoxCollider> playersColliders{};
	Renderer renderingManager{};
	Vector2 playerMaxSpeed{ 4.0f, 4.0f };


public:
	Game() {
		players.emplace_back(4.5, 7);
		playersColliders.emplace_back(1, 2);

		plateforms.emplace_back(4.5f, 3.5f);
		plateformColliders.emplace_back(5.0f, 1.0f);

		plateforms.emplace_back(9.5f, 5.5f);
		plateformColliders.emplace_back(3.0f, 1.0f);
	}

	void gameLoop() {
		bool quit = false;
		while (!quit) {
			updateDeltaTime();
			if (deltaTime >= refreshTimeInterval) {
				lastTime = currentTime;
				handleInputs(quit, renderingManager, players[0]);
				detectStaticCollisions(players, playersColliders, plateforms, plateformColliders, deltaTime);
				applyMovement(players, deltaTime);//also apply gravity
				renderingManager.render(players, playersColliders, plateforms, plateformColliders);
			}
			else
				SDL_Delay(lastTime + refreshTimeInterval * 1000 - currentTime);
			
		}
	};
		
	void updateDeltaTime() {
		currentTime = SDL_GetTicks();
		deltaTime = float(currentTime - lastTime) / 1000.0f;
	};

	//une classe sera dedicasse a la gestion d'evenements
	void handleInputs(bool &quit, Renderer& renderer, MovingObject& player) {
		SDL_Event e;
		SDL_PollEvent(&e);

		switch (e.type) {
			case SDL_QUIT:
			quit = true;
			break;
			case SDL_KEYDOWN:
				if (e.key.repeat) break;
				switch (e.key.keysym.sym) {
				case SDLK_UP:
					player.newSpeed.y = playerMaxSpeed.y;
					break;
				case SDLK_DOWN:
					player.newSpeed.y = -1.0f * playerMaxSpeed.y;
					break;
				case SDLK_RIGHT:
					player.newSpeed.x = playerMaxSpeed.x;
					break;
				case SDLK_LEFT:
					player.newSpeed.x = -1.0f * playerMaxSpeed.x;
					break;
				}
				std::cout << "new speed: " << player.newSpeed.x << "; " << player.newSpeed.y << "\n";
				break;
			case SDL_KEYUP:
				if (e.key.repeat) break;
				switch (e.key.keysym.sym) {
				case SDLK_UP:
					player.newSpeed.y = 0;
					break;
				case SDLK_DOWN:
					player.newSpeed.y = 0;
					break;
				case SDLK_RIGHT:
					player.newSpeed.x = 0;
					break;
				case SDLK_LEFT:
					player.newSpeed.x = 0;
					break;
				}
				std::cout << "new speed: " << player.newSpeed.x << "; " << player.newSpeed.y << "\n";
				break;
			case SDL_WINDOWEVENT:
				//renderer.getNewWindowSize();
				break;
		}
		
	
	}
};