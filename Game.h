#pragma once

#include "Components.h"
#include "Collisions.h"
#include "Move.h"
#include "SDL_timer.h"
#include <vector>

/*
* TODO:
* Une fonction pour passer des coordonn�es du monde aux coordonn�es de l'�cran, viewport ?
*/

class Game {
private:
	int lastTime{ 0 }, refreshTimeInterval{ 20 }, currentTime{ 0 }, deltaTime{};
	std::vector<VisibleObject> plateforms{};
	std::vector<BoxCollider> plateformColliders{};
	std::vector<MovingObject> players{};
	std::vector<BoxCollider> playersColliders{};
public:

	void gameLoop() {
		updateDeltaTime();
		//todo: user inputs
		detectStaticCollisions(players, playersColliders, plateforms, plateformColliders, deltaTime);
		applyMovement(players, deltaTime);
		//rendering ...
	};
		
	void updateDeltaTime() {
		currentTime = SDL_GetTicks();
		deltaTime = currentTime - lastTime;
		if (currentTime - lastTime >= refreshTimeInterval) {
			lastTime = currentTime;
		}
	};




};