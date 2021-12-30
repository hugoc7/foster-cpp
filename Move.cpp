#include "Components.h"
#include <vector>
#include "Move.h"
#include <iostream>

const float Gravity = 15.0f;

void applyMovement(float deltaTime) {
	std::vector<MovingObject>& objects = ecs.getComponentVector<MovingObject>();
	for (int i = 0; i < objects.size(); i++) {
		applyMovement(objects[i], deltaTime);
	}
}

void updateMovementBeforeCollision(float deltaTime) {
	std::vector<MovingObject>& objects = ecs.getComponentVector<MovingObject>();
	for (int i = 0; i < objects.size(); i++) {
		updateMovementBeforeCollision(objects[i], deltaTime);
	}

}

//also applies gravity
void applyMovement(MovingObject& object, float deltaTime) {
	object.position += object.move;
	object.speed = (1.0f / deltaTime) * object.move;
	object.newSpeed = object.speed;
}

void updateMovementBeforeCollision(MovingObject& object, float deltaTime) {
	object.newSpeed.y -= Gravity * deltaTime;
	object.move = deltaTime * object.newSpeed;
}