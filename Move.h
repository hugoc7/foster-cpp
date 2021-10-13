#pragma once
#include "ECS.h"

void applyMovement(ArrayList<EntityID> const& objects, float deltaTime);
void applyMovement(MovingObject& objects, float deltaTime);


void updateMovementBeforeCollision(ArrayList<EntityID> const& objects, float deltaTime);
void updateMovementBeforeCollision(MovingObject& object, float deltaTime);
