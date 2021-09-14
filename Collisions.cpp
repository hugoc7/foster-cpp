

#include "Collisions.h"
#include "Math.h"

/*
TO DO:
==============
_ quand newSpeed = 0, on peut arreter tous les tests !
_ on peut eviter de faire des tests sur les segments dont la normale a un produit scalaire positif avec la vitesse :
	=> en effet il s'agit d'une face que l'objet ne peut collisioner qu'en arrivant de l'intérieur ce qui est impossible sauf
	si l'objet peut se déplacer dans la platforme
_ on peut partitionner l'espace en zones pour éviter de tester toutes les plateformes
*/


void detectStaticCollisions(std::vector<MovingObject>& movingObjects, std::vector<BoxCollider> const& movingColliders,
	std::vector<VisibleObject> const& staticObjects, std::vector<BoxCollider> const& staticColliders, int deltaTime) {
	
	//for each moving object
	for (int i = 0; i < movingObjects.size(); i++) {

		//for each static object (plateform)
		for (int j = 0; j < movingObjects.size(); j++) {
			detectStaticCollisions(movingObjects[i], movingColliders[i], staticObjects[j], staticColliders[j], deltaTime);
		}
	}
}

/*detecte et calcul la distance d'une collision entre un segment en translation et un segmnt fixe
* return = la distance en pourcentage du vecteur de deplacement jusqu'à la collision ou 1 si pas de collision
* */
float collisionMovingSegmentWithSegment(Vector2  (&const movingSegment) [2], Vector2 (&const staticSegment)[2], Vector2 const& move) {
	float cutsDetected[4]{ -1, -1, -1, -1 };//negative value = no cut
	float distanceToCollision = -1;
	//0 : segment à sa position orgine
	//1 : segment à sa position d'arrivee
	//2 : segment de trajectoire partant du point 0
	//3 : segment de trajectoire partant du point 1
	cutsDetected[0] = collisionSegmentSegment(movingSegment[0], movingSegment[1], staticSegment[0], staticSegment[1]);
	cutsDetected[1] = collisionSegmentSegment(movingSegment[0] + move, movingSegment[1] + move, staticSegment[0], staticSegment[1]);
	cutsDetected[2] = collisionSegmentSegment(movingSegment[0], movingSegment[0] + move, staticSegment[0], staticSegment[1]);
	cutsDetected[3] = collisionSegmentSegment(movingSegment[1], movingSegment[1] + move, staticSegment[0], staticSegment[1]);

	//avant toute choses si collision avec la position d'origine du segment, pas de mouvement.
	if (cutsDetected[0] >= 0) {
		return 0;
	}

	if (cutsDetected[2] >= 0) {
		if (cutsDetected[1] >= 0) {
			distanceToCollision = cutsDetected[2];
		}
		else if (cutsDetected[3]) {
			distanceToCollision = std::min(cutsDetected[2], cutsDetected[3]);
		}
		else {
			Vector2 O{ movingSegment[0] };//origine
			float distToEdges = distance(staticSegment[0] - O, staticSegment[1] - O, movingSegment[1] - O, move);
			distanceToCollision = std::min(cutsDetected[2], distToEdges);
		}
	}
	else if (cutsDetected[3] >= 0)
	{
		if (cutsDetected[1] >= 0) {
			distanceToCollision = cutsDetected[3];
		}
		else {
			Vector2 O{ movingSegment[0] };//origine
			float distToEdges = distance(staticSegment[0] - O, staticSegment[1] - O, movingSegment[1] - O, move);
			distanceToCollision = std::min(cutsDetected[3], distToEdges);
		}
	}
	else if (cutsDetected[1] >= 0) {
		Vector2 O{ movingSegment[0] };//origine
		distanceToCollision = distance(staticSegment[0] - O, staticSegment[1] - O, movingSegment[1] - O, move);
	}
	//le segmet fixe est dans le parallelogram, le point d'impact est un des 2 bords du segment
	else if (pointInParallelogram(staticSegment[0], movingSegment[0], movingSegment[1] - movingSegment[0],
		move + movingSegment[0])) {
		Vector2 O{ movingSegment[0] };//origine
		distanceToCollision = distance(staticSegment[0] - O, staticSegment[1] - O, movingSegment[1] - O, move);
	}
	else {
		distanceToCollision = 1;
	}
	return distanceToCollision;
}

void detectStaticCollisions(MovingObject& movingObject, BoxCollider const& movingCollider,
	VisibleObject const& staticObject, BoxCollider const& staticCollider, int deltaTime) {

	float moveReductionFactor{ 1 };//factor which reduces the move to avoid collision

	Vector2 movingColliderPoints[4]{
		{movingObject.position.x - movingCollider.w / 2.0, movingObject.position.y - movingCollider.h / 2.0},
		{movingObject.position.x - movingCollider.w / 2.0, movingObject.position.y + movingCollider.h / 2.0},
		{movingObject.position.x + movingCollider.w / 2.0, movingObject.position.y - movingCollider.h / 2.0},
		{movingObject.position.x + movingCollider.w / 2.0, movingObject.position.y + movingCollider.h / 2.0},
	};
	Vector2 movingSegments[4][2] {
		{movingColliderPoints[0], movingColliderPoints[1]},
		{movingColliderPoints[1], movingColliderPoints[3]},
		{movingColliderPoints[3], movingColliderPoints[2]},
		{movingColliderPoints[2], movingColliderPoints[0]},
	};
	Vector2 staticColliderPoints[4]{
		{staticObject.position.x - staticCollider.w / 2.0, staticObject.position.y - staticCollider.h / 2.0},
		{staticObject.position.x - staticCollider.w / 2.0, staticObject.position.y + staticCollider.h / 2.0},
		{staticObject.position.x + staticCollider.w / 2.0, staticObject.position.y - staticCollider.h / 2.0},
		{staticObject.position.x + staticCollider.w / 2.0, staticObject.position.y + staticCollider.h / 2.0},
	};
	Vector2 staticSegments[4][2]{
		{staticColliderPoints[0], staticColliderPoints[1]},
		{staticColliderPoints[1], staticColliderPoints[3]},
		{staticColliderPoints[3], staticColliderPoints[2]},
		{staticColliderPoints[2], staticColliderPoints[0]},
	};
	Vector2 deltaMove{ deltaTime * movingObject.newSpeed };
	//il faudra optimiser en utilisant le constructeur par déplacement ... std::move (en fait pas forcément besoin ...)
	float distancesToCollisions[4][4]{};
	for (int staticSegmentIndice = 0; staticSegmentIndice < 4; staticSegmentIndice) {
		for (int i = 0; i < 4; i++) {
			float newMoveReductionFactor{ collisionMovingSegmentWithSegment(movingSegments[i], staticSegments[staticSegmentIndice], deltaMove) };
			if (newMoveReductionFactor < moveReductionFactor)
				moveReductionFactor = newMoveReductionFactor;
		}
	}
	movingObject.newSpeed = moveReductionFactor * deltaMove;
}
//retourne la distance entre le point P et le vecteur V par rapport au vecteur U (U et V ne doivent pas etre alignes)
float distance(Vector2 const& P, Vector2 const& U, Vector2 const& V) {
	return (U.x * P.y - U.y * P.x) / determinant(U, V);
}

//retourne la distance entre le segment [AB] et le vecteur V par rapport au vecteur U (U et V ne doivent pas etre alignes)
float distance(Vector2 const& A, Vector2 const& B, Vector2 const& U, Vector2 const& V) {
	return std::min(distance(A, U, V), distance(B, U, V));
}


//test if P is in parallogram OAB
bool pointInParallelogram(Vector2 const& P, Vector2 const& O, Vector2 const& A, Vector2 const& B) {
	float dets[4]{
		determinant(A - O, P - O), determinant(B - O, P - A),
		determinant(O - A, P - (A + B - O)), determinant(O - B, P - B) 
	};
	//if all determinants have same sign, P is inside
	for (int i = 1; i < 4; i++) {
		if (dets[i] * dets[0] < 0)
			return false;
	}
	return true;
}

//return k, such as k is = A + k*AB, return -1 if no collision
float collisionSegmentSegment(Vector2 A, Vector2 B, Vector2 O, Vector2 P)
{
	Vector2 AB{ B - A }, OP{ P - O };
	float det = determinant(AB, -1.0 * OP);
	if (equals(det, 0.0)) 
		return -1;
	
	float k = -(A.x * OP.y - O.x * OP.y - OP.x * A.y + OP.x * O.y) / det;
	if (k < 0 || k > 1)
		return -1;
	else
		return k;
}