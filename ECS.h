#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <set> 
#include <ctime>
#include <cstdlib>
#include <chrono>
#include <assert.h>

#define MAX_ENTITIES 5000

#define VELOCITY 0
#define TRANSFORM 1
#define COLLIDER 2
#define MAX_COMPONENTS 3

#define NO_COMPONENT -1

using EntityID = int;
using ComponentID = int;
using ComponentType = int;

extern int componentTypesCounter;

template <typename T>
class ArrayList {
public:
    std::vector<T> v{};
    ArrayList() : v(MAX_ENTITIES) {
    }

    //avec un move ca irait un peu plus vite, j'ai pas testé avec  const& ???
    void insert(T val) {
        v.push_back(val);
    }
    void erase(T const& val) {
        for (unsigned int i = 0; i < v.size() - 1; i++) {
            if (v[i] == val) {
                v[i] = v[v.size() - 1];
                break;
            }
        }
        v.pop_back();
    }
};

//C'est l'utilisateur qui gere lui meme ses systemes, il faut juste utiliser ArrayList au lieu de std::set pour enrgeistrer les entities
//
class System {
public:
    //ArrayList<int> entitySet;
};

class Entity {
public:
    Entity() {
        std::fill(std::begin(components), std::end(components), NO_COMPONENT);
    }
    ComponentID components[MAX_COMPONENTS];
};


template<typename T>
inline void removeFromVector(std::vector<T>& vec, int ind) {
    int lastIndex = vec.size() - 1;
    if (ind != lastIndex) {
        vec[ind] = vec[lastIndex];
    }
    vec.pop_back();
}
template<typename T>
inline void removeFromVector(std::vector<T>* vec, int ind) {
    int lastIndex = vec->size() - 1;
    if (ind != lastIndex) {
        (*vec)[ind] = (*vec)[lastIndex];
    }
    vec->pop_back();
}



class ECS {
public:
    //Entitys
    EntityID addEntity() {
        entities.emplace_back();
        return entities.size() - 1;
    }
    void removeEntity(EntityID entityID) {
        removeFromVector<Entity>(entities, entityID);
    }

    //Component
    template <typename T>
    void registerComponent() {
        ComponentType compType = getComponentType<T>();
        assert(componentVectors[compType] == nullptr, "This component has already been registerd.");
        componentVectors[compType] = reinterpret_cast<void*>(new std::vector<T>);

        //the destructor of the component vector is called automatically when ECS is destroyed
        vectorDestructors[compType] = &ECS::unregisterComponent<T>;
    }

    template <typename T, typename ... Args>
    ComponentID addComponent(EntityID entity, Args&& ... args) {

        //ajout du composant dans le vector des composants
        ComponentType compType = getComponentType<T>();
        assert(componentVectors[compType] != nullptr);
        std::vector<T>* compVec = static_cast<std::vector<T>*>(componentVectors[compType]);
        compVec->emplace_back(std::forward<Args>(args)...);
        ComponentID compID = compVec->size() - 1;

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
        removeFromVector(compVec, (int)entities[entity].components[compType]);
        entities[entity].components[compType] = NO_COMPONENT;
    }
    template <typename T>
    T& getComponent(EntityID entity) {
        ComponentType compType = getComponentType<T>();
        assert(componentVectors[compType] != nullptr);
        std::vector<T>* compVec = static_cast<std::vector<T>*>(componentVectors[compType]);
        assert(entity >= 0 && entity < entities.size());
        assert(entities[entity].components[compType] != NO_COMPONENT);
        return (*compVec)[entities[entity].components[compType]];
    }

    ~ECS() {
        for (int i = 0; i < MAX_COMPONENTS; i++) {
            if (vectorDestructors[i] != nullptr) {
                ((*this).*vectorDestructors[i])();
            }
        }
    }
    ECS() {
        std::fill(std::begin(componentVectors), std::end(componentVectors), nullptr);
        std::fill(std::begin(vectorDestructors), std::end(vectorDestructors), nullptr);
    }

private:
    std::vector<Entity> entities;
    void* componentVectors[MAX_COMPONENTS];
    void (ECS::* vectorDestructors[MAX_COMPONENTS])(void);//array of pointers of functions


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
