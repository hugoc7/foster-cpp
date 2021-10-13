#pragma once

#include "Components.h"
#include <vector>
#include <algorithm>
#include "ECS.h"


void detectStaticCollisions(ArrayList<EntityID> const& movingObjects, ArrayList<EntityID> const& staticObjects);

void detectStaticCollisions(MovingObject& movingObject, BoxCollider const& movingCollider,
	VisibleObject const& fixedObject, BoxCollider const& fixedCollider);

void collisionSegmentSegment(Vector2 const& A, Vector2 const& B, float& kAB, Vector2 const& O, Vector2 const& P, float& kOP);
float collisionSegmentSegment(Vector2 A, Vector2 B, Vector2 O, Vector2 P);
bool pointInParallelogram(Vector2 const& P, Vector2 const& O, Vector2 const& A, Vector2 const& B);

float distance(Vector2 const& P, Vector2 const& U, Vector2 const& V);
float distance(Vector2 const& A, Vector2 const& B, Vector2 const& U, Vector2 const& V);

float projection(Vector2 const& P, Vector2 const& U, Vector2 const& V);

float collisionMovingSegmentWithSegment(Vector2(& const movingSegment)[2], Vector2(& const staticSegment)[2], Vector2& move);
bool slidingMovingSegmentWithSegment(Vector2 const& A, Vector2 const& B, Vector2 const& O, Vector2 const& P, Vector2& move);

void printCollidingSegments(Vector2(& const a)[2], Vector2(& const b)[2]);
void printTouchingSegments(Vector2 const& a, Vector2 const& b, Vector2 const& o, Vector2 const& p);

void detectStaticSliding(ArrayList<EntityID> const& movingObjects, ArrayList<EntityID> const& staticObjects);
void detectStaticSliding(MovingObject& movingObject, BoxCollider const& movingCollider,
	VisibleObject const& staticObject, BoxCollider const& staticCollider);