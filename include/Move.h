#pragma once
#include "ECS.h"

void applyMovement(float deltaTime);
void applyMovement(MovingObject& objects, float deltaTime);


void updateMovementBeforeCollision(float deltaTime);
void updateMovementBeforeCollision(MovingObject& object, float deltaTime);
