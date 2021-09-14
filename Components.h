#pragma once

#include "Geometry.h"
#include <vector>

//A box collider is a rectangle centered on a object which is used to detect collisions
struct BoxCollider {
	int w;
	int h;
};


struct CollisionDetector {
	int collisionEntity; //l'entite avec laquelle on a une collision
	Vector2 moveToCollision; //vecteur qui va juska la collision
};


typedef struct {
	Vector2 position;
} VisibleObject;

struct MovingObject : public VisibleObject {
	Vector2 speed;
	Vector2 newSpeed;
};

#define BOX_COLLIDER 0
#define VISIBLE_OBJECT 1
#define MOVING_OBJECT 2
#define MAX_COMPONENTS 3

class ComponentsManager {
public:
	std::vector<BoxCollider> boxColliders{};
	std::vector<VisibleObject> visibleObjects{};
	std::vector<MovingObject> movingObjects{};
};