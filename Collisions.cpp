

#include "Collisions.h"
#include "Math.h"
#include <iostream>
#include <cassert>

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

/*detecte et calcul la distance d'une collision entre un segment en translation et un segmnt fixe
* return = la distance en pourcentage du vecteur de deplacement jusqu'à la collision ou 1 si pas de collision
* */
float collisionMovingSegmentWithSegment(Vector2  (&const movingSegment) [2], Vector2 (&const staticSegment)[2], Vector2& move) {
	float cutsDetected[4]{ -1.0f, -1.0f, -1.0f, -1.0f };//negative value = no cut
	float distanceToCollision = -1.0f;

	


	//important: verifier si le segment est colineaire avec la vitesse, dans ce cas, on a pas de parallelogramme
	//RMQ: Ce cas a ete ajoute apres coup et n'a pas forcement ete optimise
	if (equals(determinant(movingSegment[1] - movingSegment[0], move), 0)) {

		//std::cout << "cas non gere ";
		cutsDetected[0] = collisionSegmentSegment(movingSegment[0], movingSegment[0] + move, staticSegment[0], staticSegment[1]);
		cutsDetected[1] = collisionSegmentSegment(movingSegment[1], movingSegment[1] + move, staticSegment[0], staticSegment[1]);
		//std::cout << "move=" << move.x << "; " << move.y << "\n";

		//std::cout << "det" << determinant(movingSegment[0] - movingSegment[1], move) << "; " << cutsDetected[0] << ";" << cutsDetected[1] << "\n";

		if (cutsDetected[0] >= 0)
			distanceToCollision = cutsDetected[1] >= 0 ? std::min(cutsDetected[0], cutsDetected[1]) : cutsDetected[0];
		else if (cutsDetected[1] >= 0)
			distanceToCollision = cutsDetected[1];
		else
			distanceToCollision = 1.0f;

		//return distanceToCollision;
		move *= distanceToCollision;
		std::cout << "col:";
		return distanceToCollision;
	}

	//edges detection (NEW !!!)
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
	cutsDetected[0] = collisionSegmentSegment(movingSegment[0], movingSegment[1], staticSegment[0], staticSegment[1]);
	cutsDetected[1] = collisionSegmentSegment(movingSegment[0] + move, movingSegment[1] + move, staticSegment[0], staticSegment[1]);
	cutsDetected[2] = collisionSegmentSegment(movingSegment[0], movingSegment[0] + move, staticSegment[0], staticSegment[1]);
	cutsDetected[3] = collisionSegmentSegment(movingSegment[1], movingSegment[1] + move, staticSegment[0], staticSegment[1]);

	//avant toute choses si collision avec la position d'origine du segment, pas de mouvement.
	if (cutsDetected[0] >= 0) {
		//std::cout << "origine only ";

		//GLISSEMENT TANGENTIEL
		Vector2 AB{ staticSegment[1] - staticSegment[0] };
		Vector2 tangent{1.0f/norme(AB) * AB};
		move = dot(move, tangent) * tangent;
		return -3;
	}

	if (cutsDetected[2] >= 0) {
		if (cutsDetected[1] >= 0) {
			///std::cout << "cote 2 et arrive ";
			distanceToCollision = cutsDetected[2];
		}
		else if (cutsDetected[3] >= 0) {
			distanceToCollision = std::min(cutsDetected[2], cutsDetected[3]);
		}
		else {
			//Vector2 O{ movingSegment[0] };//origine
			//std::cout << "O:" << O.x << ";" << O.y << "  and   " << movingSegment[0].x << ";" << movingSegment[0].y << "\n";

			//float distToEdges = distance(staticSegment[0] - O, staticSegment[1] - O, movingSegment[1] - O, move);
			distanceToCollision = std::min(cutsDetected[2], distToEdges);
		}
	}
	else if (cutsDetected[3] >= 0)
	{
		if (cutsDetected[1] >= 0) {
			distanceToCollision = cutsDetected[3];
		}
		else {
			//Vector2 O{ movingSegment[0] };//origine
			//std::cout << "O:" << O.x << ";" << O.y << "  and   " << movingSegment[0].x << ";" << movingSegment[0].y << "\n";
			//float distToEdges = distance(staticSegment[0] - O, staticSegment[1] - O, movingSegment[1] - O, move);
			distanceToCollision = std::min(cutsDetected[3], distToEdges);
		}
	}
	else if (cutsDetected[1] >= 0) {
		//Vector2 O{ movingSegment[0] };//origine
		//std::cout << "O:" << O.x <<";" << O.y<< "  and   " << movingSegment[0].x<<";" << movingSegment[0].y<<"\n";
		distanceToCollision = distToEdges;//distance(staticSegment[0] - O, staticSegment[1] - O, movingSegment[1] - O, move);
	}
	//le segmet fixe est dans le parallelogram, le point d'impact est un des 2 bords du segment
	else if (pointInParallelogram(staticSegment[0], movingSegment[0], movingSegment[1] - movingSegment[0], move)) {
		//Vector2 O{ movingSegment[0] };//origine
		//std::cout << "O:" << O.x << ";" << O.y << "  and   " << movingSegment[0].x << ";" << movingSegment[0].y << "\n";

		distanceToCollision = distToEdges;//distance(staticSegment[0] - O, staticSegment[1] - O, movingSegment[1] - O, move);
	}
	else {
		distanceToCollision = 1.0f;
	}
	//std::cout << "d2:" << distanceToCollision << "\n";
	move *= distanceToCollision;
	return distanceToCollision;
}

void detectStaticCollisions(MovingObject& movingObject, BoxCollider const& movingCollider,
	VisibleObject const& staticObject, BoxCollider const& staticCollider, float deltaTime) {

	//if speed is 0 then stop
	if (equals(movingObject.newSpeed.x, 0) && equals(movingObject.newSpeed.y, 0)) return;

	//float moveReductionFactor{ 1.0f };//factor which reduces the move to avoid collision

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
	Vector2 deltaMove{ deltaTime * movingObject.newSpeed };
	//il faudra optimiser en utilisant le constructeur par déplacement ... std::move (en fait pas forcément besoin ...)
	//float distancesToCollisions[4][4]{};
	for (int j = 0; j < 4; j++) {

		//Ignorer les cotes de l'objet fixe dont la normale
		//est dans le sens du mouvement (attention a la convention)
		Vector2 n{ -1.0f * (staticSegments[j][1].y - staticSegments[j][0].y), staticSegments[j][1].x - staticSegments[j][0].x };
		if (dot(n, deltaMove) > 0) continue;

		for (int i = 0; i < 4; i++) {

			//Ignorer les cotes de l'objet en mouvement dont la normale
			//n'est pas dans le sens du mouvement (attention a la convention)
			Vector2 n{ -1.0f * (movingSegments[i][1].y - movingSegments[i][0].y), movingSegments[i][1].x - movingSegments[i][0].x };
			if (dot(n, deltaMove) < 0) continue;

		
			std::cout << movingSegments[i][0] << movingSegments[i][1] << " => " << staticSegments[j][0] << staticSegments[j][1] << " V:" << deltaMove;
			float newMoveReductionFactor{ collisionMovingSegmentWithSegment(movingSegments[i], staticSegments[j], deltaMove) };
			std::cout << " => " << newMoveReductionFactor << "\n";

			/*if (newMoveReductionFactor < moveReductionFactor)
				moveReductionFactor = newMoveReductionFactor;*/
		}
	}
	//std::cout << "\t\t=>" << moveReductionFactor << "\n";
	movingObject.newSpeed = (1.0f / deltaTime) * deltaMove;
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

	return !(k < 0 || k > 1 || t < 0 || t > 1);
}

//return k, such as k is = A + k*AB, return -1 if no collision
float collisionSegmentSegment(Vector2 A, Vector2 B, Vector2 O, Vector2 P)
{
	Vector2 AB{ B - A }, OP{ P - O };
	float det = determinant(AB, -1.0f * OP);
	if (equals(det, 0)) {
		//non c'est pas aussi simple ...
		//std::cout << "cas non gere ";
		/*Vector2 dir = (1.0f / norme(OP)) * OP; // c'est le vecteur directeur

		//verification de la distance à l'axe OP
		float dA = std::sqrt(dot(A - O, A - O) - dot(A - O, dir) * dot(A - O, dir));
		float dB = std::sqrt(dot(B - O, B - O) - dot(B - O, dir) * dot(B - O, dir));
		const float maxDistance = 0.0001f;
		if (dA > maxDistance || dB > maxDistance) {
			return -1;
		}

		//vérification le long de l'axe
		float xA = dot(dir, A - O);
		float xB = dot(dir, B - O);
		float xP = dot(dir, OP);
		if (xP > 0 && (xA <= xP || xB <= xP))
			return 0;
		if (xP < 0 && (xA >= xP || xB >= xP))
			return 0;*/

		//fonction test si les segments sont aligne en testant : dot(normale(OP), OA) == 0
		//si oui alors balaalalalal

		return -1;
	}
	/*
	k = (A_x * (P_y - O_y) - O_x * (P_y - O_y) - (P_x - O_x) * A_y + (P_x - O_x) * O_y) / det;
	t = -(A_y * (B_x - A_x) - O_y * (B_x - A_x) - (B_y - A_y) * A_x + (B_y - A_y) * O_x) / det;

	*/
	float k = (A.x * OP.y - O.x * OP.y - OP.x * A.y + OP.x * O.y) / det;
	float t = -1.0f*(A.y * AB.x - O.y * AB.x - AB.y * A.x + AB.y * O.x) / det;
	if (k < 0 || k > 1 || t < 0 || t > 1)
		return -1.0f;
	else
		return k;
}

//retourne la projection du point P sur le vecteur U parallelement au vecteur V (U et V ne doivent pas etre alignes)
float projection(Vector2 const& P, Vector2 const& U, Vector2 const& V) {
	assert(!equals(determinant(U, V), 0));
	return (P.x * V.y - P.y * V.x) / determinant(U, V);
}