

#include "Collisions.h"
#include "Math.h"
#include <iostream>
#include <cassert>

//here it manages only static collision => moving object VS static object ONLY ;)

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
	std::vector<VisibleObject> const& staticObjects, std::vector<BoxCollider> const& staticColliders, float deltaTime) {
	
	//for each moving object
	for (int i = 0; i < movingObjects.size(); i++) {

		//for each static object (plateform)
		for (int j = 0; j < staticObjects.size(); j++) {
			detectStaticCollisions(movingObjects[i], movingColliders[i], staticObjects[j], staticColliders[j], deltaTime);
		}
	}
}

void detectStaticSliding(std::vector<MovingObject>& movingObjects, std::vector<BoxCollider> const& movingColliders,
	std::vector<VisibleObject> const& staticObjects, std::vector<BoxCollider> const& staticColliders, float deltaTime) {

	//for each moving object
	for (int i = 0; i < movingObjects.size(); i++) {

		//for each static object (plateform)
		for (int j = 0; j < staticObjects.size(); j++) {
			detectStaticSliding(movingObjects[i], movingColliders[i], staticObjects[j], staticColliders[j], deltaTime);
		}
	}
}

bool glissement(Vector2 const& A, Vector2 const& B, Vector2 const& O, Vector2 const& P, Vector2& move) {
	float kAB{ -1 }, kOP{ -1 };
	collisionSegmentSegment(O, P, kOP, A, B, kAB);
	float vraiGlissement = false;//on entend par la un glissement qui modifie le mouvement.
	Vector2 nouvelAxeDeGlissement;

	//Si les segments se touchent (mais ne se coupent pas!) des leur position initiales
	//alors il y a GLISSEMENT
	if (equals(kOP, -3)) {
		//on a besoin que un des segments soit oriente, ici c'est le segment fixe (les normales peuvent etre precalculees)
		Vector2 nStatic{ -(B - A).y, (B - A).x };
		nStatic *= 1.0f / norme(nStatic);
		if (dot(move, nStatic) < 0) {
			//std::cout << "WoWa";
			vraiGlissement = true;
			move = move - dot(move, nStatic) * nStatic;
		}

	}
	else if (equals(kAB, 1) && !equals(kOP, 0) && !equals(kOP, 1)) {
		Vector2 nMoving{ -(P - O).y, (P - O).x };
		nMoving *= 1.0f / norme(nMoving);
		if (dot(move, nMoving) * dot(A - B, nMoving) > 0) {
			vraiGlissement = true;
			move = move - dot(move, nMoving) * nMoving;
		}
	}
	else if (equals(kAB, 0) && !equals(kOP, 0) && !equals(kOP, 1)) {
		Vector2 nMoving{ -(P - O).y, (P - O).x };
		nMoving *= 1.0f / norme(nMoving);
		if (dot(move, nMoving) * dot(B - A, nMoving) > 0) {
			vraiGlissement = true;
			move = move - dot(move, nMoving) * nMoving;
		}
	}
	else if (equals(kOP, 1) && !equals(kAB, 0) && !equals(kAB, 1)) {
		Vector2 nStatic{ -(B - A).y, (B - A).x };
		nStatic *= 1.0f / norme(nStatic);
		//std::cout << "A1. ";
		if (dot(move, nStatic) * dot(O - P, nStatic) < 0) {
			vraiGlissement = true;
			move = move - dot(move, nStatic) * nStatic;
		}
	}
	else if (equals(kOP, 0) && !equals(kAB, 0) && !equals(kAB, 1)) {
		Vector2 nStatic{ -(B - A).y, (B - A).x };
		nStatic *= 1.0f / norme(nStatic);
		//std::cout << "A2. ";

		if (dot(move, nStatic) * dot(P - O, nStatic) < 0) {
			vraiGlissement = true;
			move = move - dot(move, nStatic) * nStatic;
		}
	}
	else
		return false;

	if (vraiGlissement) {
		//std::cout << "\nmove: " << oldMove;
		printTouchingSegments(O, P, A, B);
	}
	return true;
}

/*detecte et calcul la distance d'une collision entre un segment en translation et un segmnt fixe
* return = la distance en pourcentage du vecteur de deplacement jusqu'à la collision ou 1 si pas de collision
* */
float collisionMovingSegmentWithSegment(Vector2  (&const movingSegment) [2], Vector2 (&const staticSegment)[2], Vector2& move) {
	float cutsDetected[4]{ -1.0f, -1.0f, -1.0f, -1.0f };//negative value = no cut
	float kStatic[4]{ -1,-1,-1,-1 };
	float kMoving[4]{ -1,-1,-1,-1 };
	float distanceToCollision = -1.0f;

	//std::cout << "\nMOVE INIT: " << move;


	//Si le segment est colineaire avec la vitesse on a pas de parallelogramme
	if (equals(determinant(movingSegment[1] - movingSegment[0], move), 0)) {

		cutsDetected[0] = collisionSegmentSegment(movingSegment[0], movingSegment[0] + move, staticSegment[0], staticSegment[1]);
		cutsDetected[1] = collisionSegmentSegment(movingSegment[1], movingSegment[1] + move, staticSegment[0], staticSegment[1]);


		if (cutsDetected[0] > 0)
			distanceToCollision = cutsDetected[1] > 0 ? std::min(cutsDetected[0], cutsDetected[1]) : cutsDetected[0];
		else if (cutsDetected[1] > 0)
			distanceToCollision = cutsDetected[1];
		else
			distanceToCollision = 1.0f;

		move *= distanceToCollision;
		if (!equals(distanceToCollision, 1.0f)) {
			printCollidingSegments(movingSegment, staticSegment);
			std::cout << "caca";
		}
		return distanceToCollision;
	}


	//edges detection
	float edges[2];
	edges[0] = projection(staticSegment[0] - movingSegment[0], move, movingSegment[1] - movingSegment[0]);
	edges[1] = projection(staticSegment[1] - movingSegment[0], move, movingSegment[1] - movingSegment[0]);
	if (!pointInParallelogram(staticSegment[0], movingSegment[0], movingSegment[1] - movingSegment[0], move))
		edges[0] = 10000.0f;
	if (!pointInParallelogram(staticSegment[1], movingSegment[0], movingSegment[1] - movingSegment[0], move))
		edges[1] = 10000.0f;
	float distToEdges{ std::min(edges[0], edges[1]) };


	//0 : segment à sa position orgine
	//1 : segment à sa position d'arrivee
	//2 : segment de trajectoire partant du point 0
	//3 : segment de trajectoire partant du point 1
	//cutsDetected[0] = collisionSegmentSegment(movingSegment[0], movingSegment[1], staticSegment[0], staticSegment[1]);
	cutsDetected[1] = collisionSegmentSegment(movingSegment[0] + move, movingSegment[1] + move, staticSegment[0], staticSegment[1]);
	cutsDetected[2] = collisionSegmentSegment(movingSegment[0], movingSegment[0] + move, staticSegment[0], staticSegment[1]);
	cutsDetected[3] = collisionSegmentSegment(movingSegment[1], movingSegment[1] + move, staticSegment[0], staticSegment[1]);

	if (cutsDetected[2] > 0) {
		if (cutsDetected[1] > 0) {
			distanceToCollision = cutsDetected[2];
		}
		else if (cutsDetected[3] > 0) {
			distanceToCollision = std::min(cutsDetected[2], cutsDetected[3]);
		}
		else {
			distanceToCollision = std::min(cutsDetected[2], distToEdges);
		}
	}
	else if (cutsDetected[3] > 0)
	{
		if (cutsDetected[1] > 0) {
			distanceToCollision = cutsDetected[3];
		}
		else {
			distanceToCollision = std::min(cutsDetected[3], distToEdges);
		}
	}
	else if (cutsDetected[1] > 0) {
		distanceToCollision = distToEdges;
	}
	//le segmet fixe est dans le parallelogram, le point d'impact est un des 2 bords du segment
	else if (pointInParallelogram(staticSegment[0], movingSegment[0], movingSegment[1] - movingSegment[0], move)) {
		distanceToCollision = distToEdges;
		std::cout << "point in parallelogramm";
	}
	else {
		distanceToCollision = 1.0f;
	}
	move *= distanceToCollision;
	if (!equals(distanceToCollision, 1.0f)) {
		printCollidingSegments(movingSegment, staticSegment);
	}
}

void showRects(Vector2 (&const a)[4], Vector2 (&const b)[4]) {
	std::cout << a[1] << " " << a[3] << "  --  " << b[1] << " " << b[3] << "\n";
	std::cout << a[0] << " " << a[2] << "  --  " << b[0] << " " << b[2] << "\n\n\n";
}

void printCollidingSegments(Vector2(& const a)[2], Vector2(& const b)[2]) {
	std::cout << "\ncollision: \n" << a[0] << " " << a[1] << "  --  " << b[0] << " " << b[1] << "\n";
}
void printTouchingSegments(Vector2 const& a, Vector2 const& b, Vector2 const& o, Vector2 const& p) {
	std::cout << "\nglissement: \n" << a << " " << b << "  --  " << o << " " << p << "\n";
}

void detectStaticSliding(MovingObject& movingObject, BoxCollider const& movingCollider,
	VisibleObject const& staticObject, BoxCollider const& staticCollider, float deltaTime) {

	//if speed is 0 then stop
	if (equals(movingObject.newSpeed.x, 0) && equals(movingObject.newSpeed.y, 0)) return;

	Vector2 movingColliderPoints[4]{
		{movingObject.position.x - movingCollider.w / 2.0f, movingObject.position.y - movingCollider.h / 2.0f},
		{movingObject.position.x - movingCollider.w / 2.0f, movingObject.position.y + movingCollider.h / 2.0f},
		{movingObject.position.x + movingCollider.w / 2.0f, movingObject.position.y - movingCollider.h / 2.0f},
		{movingObject.position.x + movingCollider.w / 2.0f, movingObject.position.y + movingCollider.h / 2.0f},
	};
	Vector2 movingSegments[4][2]{
		{movingColliderPoints[0], movingColliderPoints[1]},
		{movingColliderPoints[1], movingColliderPoints[3]},
		{movingColliderPoints[3], movingColliderPoints[2]},
		{movingColliderPoints[2], movingColliderPoints[0]},
	};
	Vector2 staticColliderPoints[4]{
		{staticObject.position.x - staticCollider.w / 2.0f, staticObject.position.y - staticCollider.h / 2.0f},
		{staticObject.position.x - staticCollider.w / 2.0f, staticObject.position.y + staticCollider.h / 2.0f},
		{staticObject.position.x + staticCollider.w / 2.0f, staticObject.position.y - staticCollider.h / 2.0f},
		{staticObject.position.x + staticCollider.w / 2.0f, staticObject.position.y + staticCollider.h / 2.0f},
	};
	Vector2 staticSegments[4][2]{
		{staticColliderPoints[0], staticColliderPoints[1]},
		{staticColliderPoints[1], staticColliderPoints[3]},
		{staticColliderPoints[3], staticColliderPoints[2]},
		{staticColliderPoints[2], staticColliderPoints[0]},
	};
	Vector2 deltaMove{ movingObject.move };
	for (int j = 0; j < 4; j++) {

		//Ignorer les cotes de l'objet fixe dont la normale
		//est dans le sens du mouvement (attention a la convention)
		//Vector2 n{ -1.0f * (staticSegments[j][1].y - staticSegments[j][0].y), staticSegments[j][1].x - staticSegments[j][0].x };
		//if (dot(n, deltaMove) > 0) continue;

		for (int i = 0; i < 4; i++) {

			//Ignorer les cotes de l'objet en mouvement dont la normale
			//n'est pas dans le sens du mouvement (attention a la convention)
			//Vector2 n{ -1.0f * (movingSegments[i][1].y - movingSegments[i][0].y), movingSegments[i][1].x - movingSegments[i][0].x };
			//if (dot(n, deltaMove) < 0) continue;
			glissement(staticSegments[j][0], staticSegments[j][1], movingSegments[i][0], movingSegments[i][1], deltaMove);

		}
	}
	movingObject.move = deltaMove;
	//movingObject.newSpeed = (1.0f / deltaTime) * deltaMove;
	//showRects(movingColliderPoints, staticColliderPoints);
}

void detectStaticCollisions(MovingObject& movingObject, BoxCollider const& movingCollider,
	VisibleObject const& staticObject, BoxCollider const& staticCollider, float deltaTime) {

	//if speed is 0 then stop
	if (equals(movingObject.newSpeed.x, 0) && equals(movingObject.newSpeed.y, 0)) return;

	Vector2 movingColliderPoints[4]{
		{movingObject.position.x - movingCollider.w / 2.0f, movingObject.position.y - movingCollider.h / 2.0f},
		{movingObject.position.x - movingCollider.w / 2.0f, movingObject.position.y + movingCollider.h / 2.0f},
		{movingObject.position.x + movingCollider.w / 2.0f, movingObject.position.y - movingCollider.h / 2.0f},
		{movingObject.position.x + movingCollider.w / 2.0f, movingObject.position.y + movingCollider.h / 2.0f},
	};
	Vector2 movingSegments[4][2] {
		{movingColliderPoints[0], movingColliderPoints[1]},
		{movingColliderPoints[1], movingColliderPoints[3]},
		{movingColliderPoints[3], movingColliderPoints[2]},
		{movingColliderPoints[2], movingColliderPoints[0]},
	};
	Vector2 staticColliderPoints[4]{
		{staticObject.position.x - staticCollider.w / 2.0f, staticObject.position.y - staticCollider.h / 2.0f},
		{staticObject.position.x - staticCollider.w / 2.0f, staticObject.position.y + staticCollider.h / 2.0f},
		{staticObject.position.x + staticCollider.w / 2.0f, staticObject.position.y - staticCollider.h / 2.0f},
		{staticObject.position.x + staticCollider.w / 2.0f, staticObject.position.y + staticCollider.h / 2.0f},
	};
	Vector2 staticSegments[4][2]{
		{staticColliderPoints[0], staticColliderPoints[1]},
		{staticColliderPoints[1], staticColliderPoints[3]},
		{staticColliderPoints[3], staticColliderPoints[2]},
		{staticColliderPoints[2], staticColliderPoints[0]},
	};
	Vector2 deltaMove{ movingObject.move };
	//bool coteCollisions[4]{ false, false, false, false };
	//il faudra optimiser en utilisant le constructeur par déplacement ... std::move (en fait pas forcément besoin ...)
	for (int j = 0; j < 4; j++) {

		//Ignorer les cotes de l'objet fixe dont la normale
		//est dans le sens du mouvement (attention a la convention)
		//Vector2 n{ -1.0f * (staticSegments[j][1].y - staticSegments[j][0].y), staticSegments[j][1].x - staticSegments[j][0].x };
		//if (dot(n, deltaMove) > 0) continue;

		for (int i = 0; i < 4; i++) {

			//Ignorer les cotes de l'objet en mouvement dont la normale
			//n'est pas dans le sens du mouvement (attention a la convention)
			//Vector2 n{ -1.0f * (movingSegments[i][1].y - movingSegments[i][0].y), movingSegments[i][1].x - movingSegments[i][0].x };
			//if (dot(n, deltaMove) < 0) continue;

			//On ne teste que les segments qui sont parallèles entre eux 
			/*if (!equals(determinant(movingSegments[i][1] - movingSegments[i][0], staticSegments[j][1] - staticSegments[j][0]), 0)) {
				continue;
			}*/

			//std::cout << movingSegments[i][0] << movingSegments[i][1] << " => " << staticSegments[j][0] << staticSegments[j][1] << " V:" << deltaMove;
			float newMoveReductionFactor{ collisionMovingSegmentWithSegment(movingSegments[i], staticSegments[j], deltaMove) };
			//std::cout << " => " << newMoveReductionFactor << "\n";

			/*if((newMoveReductionFactor >= 0 && newMoveReductionFactor < 1) || newMoveReductionFactor == -3){
				const float marge = 0.001f;
				if (i == 0) {
					coteCollisions[GAUCHE] = true;
				}
				if (i == 1) {
					coteCollisions[HAUT] = true;

				}
				if (i == 2) {
					coteCollisions[DROITE] = true;
				}
				if (i == 3) {
					coteCollisions[BAS] = true;
				}
			}*/

		}
	}
	/*const float marge = 0.001f;
	if (coteCollisions[GAUCHE]) {
		std::cout << "\nCollision gauche";
		deltaMove.x += marge;
	}
	if (coteCollisions[HAUT]) {
		std::cout << "\nCollision haut";
		deltaMove.y -= marge;
	}
	if (coteCollisions[DROITE]) {
		std::cout << "\nCollision droite";
		deltaMove.x -= marge;
	}
	if (coteCollisions[BAS]) {
		std::cout << "\nCollision bas";
		deltaMove.y += marge;
	}*/
	//std::cout << "\nfinal delta move: " << deltaMove;
	movingObject.move = deltaMove;
	//movingObject.newSpeed = (1.0f / deltaTime) * deltaMove;
	//showRects(movingColliderPoints, staticColliderPoints);
}
//retourne la distance entre le point P et le vecteur V par rapport au vecteur U (U et V ne doivent pas etre alignes)
float distance(Vector2 const& P, Vector2 const& U, Vector2 const& V) {
	assert(std::abs(determinant(U, V)) > PRECISION);
	return (U.x * P.y - U.y * P.x) / determinant(U, V);
}

//retourne la distance entre le segment [AB] et le vecteur V par rapport au vecteur U (U et V ne doivent pas etre alignes)
float distance(Vector2 const& A, Vector2 const& B, Vector2 const& U, Vector2 const& V) {
	return std::min(distance(A, U, V), distance(B, U, V));
}


//test if P is in parallogram OAB
bool pointInParallelogram(Vector2 const& P, Vector2 const& O, Vector2 const& U, Vector2 const& V) {
	float det = determinant(U, V);
	assert(!equals(det, 0));
	float t = ((P.x - O.x) * V.y - (P.y - O.y) * V.x) / det;
	float k = (-1.0f * (P.x - O.x) * U.y + (P.y - O.y) * U.x) / det;

	return k > 0 && k < 1 && t > 0 && t < 1;
}

//Entree: segments [AB] et [OP]
//Teste les collisions entre [AB] et [OP] (fixe) et modifie kAb et kOP en fonction du resultat.
//Si les segments se coupent alors AI = A + kAB * AB et OI = O + kOP * OP
//Si les segments ne se touchent pas alors kAB = kOP = -1
//Si les segments se chevauchent alors kAB = kOP = -3
//Si les segments se chevauchent en se touchant a leur extremités, alors kAB = kOP = -4
void collisionSegmentSegment(Vector2 const& A, Vector2 const& B, float& kAB, Vector2 const& O, Vector2 const& P, float& kOP)
{
	kAB = -1;
	kOP = -1;

	Vector2 AB{ B - A }, OP{ P - O };
	float det = determinant(AB, -1.0f * OP);

	//Si [AB] et [OP] sont paralleles
	if (equals(det, 0)) {

		//Si [AB] et [OP] sont alignees
		if (equals(determinant(OP, A - O), 0)) {

			//On projette A, B et P sur la droite (OP)
			float xA = dot(OP, A - O);
			float xB = dot(OP, B - O);
			float xP = dot(OP, OP);
			//Si les segments se chevauchent
			if (xA == 0 || xA == xP || xB == 0 || xB == xP) {
				kAB = -4;
				kOP = -4;
			}
			//Si [AB] == [OP]
			else if ((xA == 0 && xB == xP) || (xA == xP && xB == 0)) {
				kAB = -3;
				kOP = -3;
			}
			else if ((0 < xA && xA < xP) || (0 < xB && xB < xP)) {
				kAB = -3;
				kOP = -3;
			}
		}
	}
	//Si [AB] et [OP] ne sont PAS paralleles
	else {
		kAB = (A.x * OP.y - O.x * OP.y - OP.x * A.y + OP.x * O.y) / det;
		kOP = -1.0f * (A.y * AB.x - O.y * AB.x - AB.y * A.x + AB.y * O.x) / det;
		if (kAB < 0-PRECISION || kAB > 1+PRECISION || kOP < 0-PRECISION || kOP > 1+PRECISION) { // attention aux comparaison de flottants !
			kAB = -1;
			kOP = -1;
		}
	}
}

//if collision (cut, not just touch !) return k such as AI = A + k*AB
//return -1 or -3 if no collision
float collisionSegmentSegment(Vector2 A, Vector2 B, Vector2 O, Vector2 P)
{
	Vector2 AB{ B - A }, OP{ P - O };
	float det = determinant(AB, -1.0f * OP);
	//Si [AB] et [OP] sont paralleles
	if (equals(det, 0)) {

		//Si [AB] et [OP] sont alignees
		/*if (equals(determinant(OP, A - O), 0)) {

			//On projette A, B et P sur la droite (OP)
			float xA = dot(OP, A - O);
			float xB = dot(OP, B - O);
			float xP = dot(OP, OP);
			//Si les segments se chevauchent
			if ((0 <= xA && xA <= xP) || (0 <= xB && xB <= xP)) {
				return -3.0f;
			}
		}
		else*/
		//inutile ici car on ne détecte pas le contact seul
		return -1.0f;
	}
	//Si [AB] et [OP] ne sont PAS paralleles
	float k = (A.x * OP.y - O.x * OP.y - OP.x * A.y + OP.x * O.y) / det;
	float t = -1.0f * (A.y * AB.x - O.y * AB.x - AB.y * A.x + AB.y * O.x) / det;
	if (k <= 0 || k >= 1 || t <= 0 || t >= 1)
		return -1.0f;// attention aux comparaison de flottants !
	else
		return k;
}

//retourne la projection du point P sur le vecteur U parallelement au vecteur V (U et V ne doivent pas etre alignes)
float projection(Vector2 const& P, Vector2 const& U, Vector2 const& V) {
	assert(!equals(determinant(U, V), 0));
	return (P.x * V.y - P.y * V.x) / determinant(U, V);
}