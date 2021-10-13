#include "Components.h"
#include <vector>
#include "Move.h"
#include <iostream>

const float Gravity = 15.0f;

void applyMovement(ArrayList<EntityID> const& objects, float deltaTime) {
	for (int i = 0; i < objects.v.size(); i++) {
		applyMovement(ecs.getComponent<MovingObject>(objects.v[i]), deltaTime);
	}
}

void updateMovementBeforeCollision(ArrayList<EntityID> const& objects, float deltaTime) {
	for (int i = 0; i < objects.v.size(); i++) {
		updateMovementBeforeCollision(ecs.getComponent<MovingObject>(objects.v[i]), deltaTime);
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