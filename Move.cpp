#include "Components.h"
#include <vector>
#include "Move.h"


void applyMovement(std::vector<MovingObject>& objects, int deltaTime) {
	for (int i = 0; i < objects.size(); i++) {
		applyMovement(objects[i], deltaTime);
	}
}

void applyMovement(MovingObject& object, int deltaTime) {
	object.speed = object.newSpeed;
	object.position += deltaTime * object.speed;
}