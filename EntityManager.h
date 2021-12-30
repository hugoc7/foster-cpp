#pragma once
#include "ECS.h"
#include "Containers.h"
#include "Components.h"

enum class EntityType : Uint8 {
	PLAYER = 0,
	PROJECTILE = 1,
};


/*use this class to:
_ add/remove/update entities in the ECS
_ subscribe/unsbsribe entities to systems
_ manage entities groups
_ activate/desactivate entities
*/
class EntityManager {
public:

	ArrayList<EntityID> plateforms{};
	ArrayList<EntityID> players{};
	Vector2 charactersDimensions{ 1, 2 };

	void addPlateform(float x, float y, float w, float h) {
		EntityID plateform = ecs.addEntity();
		ecs.addComponent<VisibleObject>(plateform, x, y);
		ecs.addComponent<BoxCollider>(plateform, w, h);
		plateforms.insert(plateform);
	}

	//TODO: put this 5 followinf fucntions in a class NetworkManager
	//remove the components of an entity
	void removeAllComponents(EntityID entity, EntityType type) {
		std::cout << "\nremove components of entity " << entity;
		if (type == EntityType::PLAYER) {
			ecs.removeComponent<MovingObject>(entity);
			ecs.removeComponent<BoxCollider>(entity);
		}
	}
	//desactivate the components of an entity
	void desactivateAllComponents(EntityID entity, EntityType type) {
		std::cout << "\ndesactivate entity " << entity;
		if (type == EntityType::PLAYER) {
			players.erase(entity);
		}
	}
	//activate the components of an entity
	void activateAllComponents(EntityID entity, EntityType type) {
		std::cout << "\nactivate entity " << entity;
		if (type == EntityType::PLAYER) {
			players.insert(entity);
		}
	}
	//add all components of an entity
	void addAllComponents(EntityID entity, EntityType type, float x, float y, float vx, float vy) {
		std::cout << "\nadd components to entity " << entity;
		if (type == EntityType::PLAYER) {
			ecs.addComponent<MovingObject>(entity, x, y, vx, vy);
			ecs.addComponent<BoxCollider>(entity, charactersDimensions.x, charactersDimensions.y);
		}
	}
	//udate all components of an entity
	void updateAllComponents(EntityID entity, EntityType type, float x, float y, float vx, float vy) {
		if (type == EntityType::PLAYER) {
			MovingObject& moveComp = ecs.getComponent<MovingObject>(entity);
			moveComp.position.x = x;
			moveComp.position.y = y;
			moveComp.newSpeed.x = vx;
			moveComp.newSpeed.y = vy;
			moveComp.oldPosition = moveComp.position;//maybe useful ? I dont remember
		}
	}
};