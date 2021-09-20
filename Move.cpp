#include "Components.h"
#include <vector>
#include "Move.h"


void applyMovement(std::vector<MovingObject>& objects, float deltaTime) {
	for (int i = 0; i < objects.size(); i++) {
		applyMovement(objects[i], deltaTime);
	}
}

//also applies gravity
void applyMovement(MovingObject& object, float deltaTime) {
	static const float gravity = 3.0f;

	object.speed = object.newSpeed;
	object.oldPosition = object.position;
	//object.speed.y -= gravity * deltaTime;
	object.position += deltaTime * object.speed;
	object.newSpeed = object.speed;
}