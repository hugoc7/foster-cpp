#pragma once

#include "Components.h"
#include <vector>
#include <algorithm>


void detectStaticCollisions(std::vector<MovingObject>& movingObjects, std::vector<BoxCollider> const& movingColliders,
	std::vector<VisibleObject> const& fixedObjects, std::vector<BoxCollider> const& fixedColliders, int deltaTime);

void detectStaticCollisions(MovingObject& movingObject, BoxCollider const& movingCollider,
	VisibleObject const& fixedObject, BoxCollider const& fixedCollider, int deltaTime);

float collisionSegmentSegment(Vector2 A, Vector2 B, Vector2 O, Vector2 P);
bool pointInParallelogram(Vector2 const& P, Vector2 const& O, Vector2 const& A, Vector2 const& B);

float distance(Vector2 const& P, Vector2 const& U, Vector2 const& V);
float distance(Vector2 const& A, Vector2 const& B, Vector2 const& U, Vector2 const& V);