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
	std::vector<VisibleObject> plateforms{};
	std::vector<BoxCollider> plateformColliders{};
	std::vector<MovingObject> players{};
	std::vector<BoxCollider> playersColliders{};
	Renderer renderingManager{};
	Vector2 playerMaxSpeed{ 7.0f, 10.0f};


public:
	Game() {
		players.emplace_back(4.5, 7);
		playersColliders.emplace_back(1, 2);

		readPlateformsFromFile("map.txt", plateforms, plateformColliders);
		/*plateforms.emplace_back(4.5f, 3.5f);
		plateformColliders.emplace_back(5.0f, 1.0f);

		plateforms.emplace_back(9.5f, 5.5f);
		plateformColliders.emplace_back(3.0f, 1.0f);*/


		/*Vector2 OP[2]{Vector2(4.264,4.144),Vector2(4.264,6.144) };
		Vector2 AB[2]{ Vector2(2,4),Vector2(7,4) };
		std::cout << OP[0] << OP[1] << " => " << AB[0] << AB[1];
		float kaka{ collisionMovingSegmentWithSegment(movingSegments[i], staticSegments[staticSegmentIndice], deltaMove) };
		std::cout << " => " << kaka << "\n";
		collisionMovingSegmentWithSegment(Vector2(& const movingSegment)[2], Vector2(& const staticSegment)[2], Vector2 const& move)*/


	}
	void readPlateformsFromFile(std::string const& filename, std::vector<VisibleObject>& plateforms, std::vector<BoxCollider>& plateformColliders) {
		std::ifstream file(filename);
		float x, y, w, h;
		while (!file.eof() && file.good()) {
			file >> x >> y >> w >> h;
			std::cout << x << ", " << y << "  ;  " << w << ", " << h << std::endl;
			plateforms.emplace_back(x, y);
			plateformColliders.emplace_back(w, h);
		}
	}

	void gameLoop() {
		bool quit = false;
		while (!quit) {
			updateDeltaTime();
			if (deltaTime >= refreshTimeInterval) {
				lastTime = currentTime;
				handleInputs(quit, renderingManager, players[0]);
				updateMovementBeforeCollision(players, deltaTime);

				//sliding must be checked before collisions !
				detectStaticSliding(players, playersColliders, plateforms, plateformColliders);
				detectStaticCollisions(players, playersColliders, plateforms, plateformColliders);

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

	///@brief une classe sera dedicasse a la gestion d'evenements
	void handleInputs(bool &quit, Renderer& renderer, MovingObject& player) {
		SDL_Event e;
		SDL_PollEvent(&e);


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