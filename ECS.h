#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <set> 
#include <ctime>
#include <cstdlib>
#include <chrono>
#include <assert.h>
#include "Containers.h"
#include <functional>

//Attention, l'Entity Component System est un singleton, il ne peut y en avoir qu'un !

#define MEAN_ENTITIES 30

#define MAX_COMPONENTS 4

#define NO_COMPONENT -1

using EntityID = int;
using ComponentID = int;
using ComponentType = int;

extern int componentTypesCounter;



//C'est l'utilisateur qui gere lui meme ses systemes, il faut juste utiliser ArrayList au lieu de std::set pour enrgeistrer les entities
//
class System {
public:
    //ArrayList<int> entitySet;
};

class Entity {
public:
    Entity() {
        reinit();
    }
    void reinit() {
        std::fill(std::begin(components), std::end(components), NO_COMPONENT);
    }
    ComponentID components[MAX_COMPONENTS];

};




class ECS {
public:
    //Entitys
    EntityID addEntity() {
        if (!removedEntities.empty()) {
            //recycling last removed entity
            EntityID entity = removedEntities[removedEntities.size() - 1];
            removedEntities.pop_back();
            entities[entity].reinit();//put all components to NO_COMPONENT value
            return entity;
        }
        else {
            entities.emplace_back();
            return entities.size() - 1;
        }
    }
    //warning: user has to remove all components manually before removing the entity
    void removeEntity(EntityID entityID) {
        //removeFromVector<Entity>(entities, entityID); //NO because it invalidates the last entity ID :(
        //Solution 1 : naive
        //entities.erase(entities.begin()+entityID);
        //Solution 2 : "clever"
        removedEntities.push_back(entityID);
    }

    //Component
    template <typename T>
    void registerComponent() {
        ComponentType compType = getComponentType<T>();
        assert(componentVectors[compType] == nullptr, "This component has already been registerd.");
        componentVectors[compType] = reinterpret_cast<void*>(new std::vector<T>);

        //the destructor of the component vector is called automatically when ECS is destroyed
        vectorDestructors.push_back(std::mem_fn(&ECS::unregisterComponent<T>));
    }

    template <typename T, typename ... Args>
    ComponentID addComponent(EntityID entity, Args&& ... args) {

        //ajout du composant dans le vector des composants
        ComponentType compType = getComponentType<T>();
        assert(componentVectors[compType] != nullptr);
        std::vector<T>* compVec = static_cast<std::vector<T>*>(componentVectors[compType]);
        compVec->emplace_back(std::forward<Args>(args)...);
        ComponentID compID = compVec->size() - 1;

        componentToEntity[compType].push_back(entity);

        //ajout du composant dans le vector des entites
        assert(entities[entity].components[compType] == NO_COMPONENT);
        entities[entity].components[compType] = compID;
        return compID;
    }

    template <typename T>
    void removeComponent(EntityID entity) {

        //ajout du composant dans le vector des composants
        ComponentType compType = getComponentType<T>();
        assert(componentVectors[compType] != nullptr);
        std::vector<T>* compVec = static_cast<std::vector<T>*>(componentVectors[compType]);
        assert(entity >= 0 && entity < entities.size());
        assert(entities[entity].components[compType] != NO_COMPONENT);

        ComponentID componentToDelete = entities[entity].components[compType];
        removeFromVector(compVec, (int)componentToDelete);//replace the componentToDelete by the last component
        removeFromVector(componentToEntity[compType], (int)componentToDelete);

        //if we moved the last component
        if (componentToDelete < compVec->size() - 1) {
            assert(componentToEntity[compType].size() == compVec->size());
            //now the entity of the replaced component has the good index to the component
            entities[componentToEntity[compType][componentToDelete]].components[compType] = componentToDelete;
        }

        entities[entity].components[compType] = NO_COMPONENT;
    }
    template <typename T>
    T& getComponent(EntityID entity) {
        ComponentType compType = getComponentType<T>();
        assert(componentVectors[compType] != nullptr, "This component does not exist.");
        std::vector<T>* compVec = static_cast<std::vector<T>*>(componentVectors[compType]);
        assert(entity >= 0 && entity < entities.size(), "Bad entity ID.");
        assert(entities[entity].components[compType] != NO_COMPONENT, "This entity does not own this component.");
        return (*compVec)[entities[entity].components[compType]];
    }

    ~ECS() {
        for (int i = 0; i < vectorDestructors.size(); i++) {
            vectorDestructors[i](this);
        }
    }
    ECS() {
        std::fill(std::begin(componentVectors), std::end(componentVectors), nullptr);
    }

private:
    std::vector<Entity> entities;
    void* componentVectors[MAX_COMPONENTS];
    std::vector<std::function<void(ECS*)>> vectorDestructors;
    std::vector<EntityID> componentToEntity[MAX_COMPONENTS];
    std::vector<EntityID> removedEntities;//contains indices of removed entities (they are sparse in the entities vector and can be reused)



    template <typename T>
    void unregisterComponent() {
        ComponentType compType = getComponentType<T>();
        delete static_cast<std::vector<T>*>(componentVectors[compType]);
    }


    template <typename T>
    ComponentType getComponentType()
    {
        static ComponentType const componentType = componentTypesCounter++;
        assert(componentType >= 0 && componentType < MAX_COMPONENTS);
        return componentType;
    }
};

extern ECS ecs;