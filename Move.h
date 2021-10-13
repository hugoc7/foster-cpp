#pragma once

void applyMovement(std::vector<MovingObject>& objects, float deltaTime);
void applyMovement(MovingObject& objects, float deltaTime);


void updateMovementBeforeCollision(std::vector<MovingObject>& objects, float deltaTime);
void updateMovementBeforeCollision(MovingObject& object, float deltaTime);
