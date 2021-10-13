#pragma once

#include "Geometry.h"
#include <vector>

//A box collider is a rectangle centered on a object which is used to detect collisions
struct BoxCollider {
	int w;
	int h;
	BoxCollider() = default;
	BoxCollider(int w, int h) : w{ w }, h{ h } {};
};

struct CollisionDetector {
	int collisionEntity; //l'entite avec laquelle on a une collision
	Vector2 moveToCollision; //vecteur qui va juska la collision
};


struct VisibleObject {
	Vector2 position;
	Vector2 oldPosition;
	VisibleObject() = default;
	VisibleObject(float x, float y) : position{ x, y }, oldPosition{ x,y } {};
};

struct MovingObject : public VisibleObject {
	Vector2 speed{};
	Vector2 newSpeed{};
	Vector2 move{0,0};
	MovingObject() = default;
	MovingObject(float x, float y) : VisibleObject(x, y) {};
};