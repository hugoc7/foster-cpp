
#include <iostream>
#include "SDL.h"
#include "Collisions.h"
#include "Geometry.h"
#include <iostream>
#include "ECS.h"
#include "Game.h"
#include "Components.h"
#include "SDL_net.h"
#include "Server.h"
#include "Client.h"
#include <map>
#include <thread>
#include "Containers.h"

/*
struct Velocity {
    int speed{};
};
struct Transform {
    Transform(int poss) : pos{ poss } {};
    int pos{};
};
struct Collider {
    int w{};
};

void testECS(){
    
    ECS ecs{};
    ecs.registerComponent<Transform>();
    ecs.registerComponent<Collider>();
    ecs.registerComponent<Velocity>();
    EntityID hugo = ecs.addEntity();
    ecs.addComponent<Transform>(hugo, 42);
    ecs.addComponent<Velocity>(hugo);
    ecs.addComponent<Collider>(hugo);
    EntityID mina = ecs.addEntity();
    ecs.addComponent<Transform>(mina, 35);
    ecs.addComponent<Collider>(mina);
    EntityID valou = ecs.addEntity();
    ecs.addComponent<Transform>(valou, 24);
    ecs.addComponent<Velocity>(valou);
    EntityID alex = ecs.addEntity();
    ecs.addComponent<Collider>(alex);
    ecs.addComponent<Velocity>(alex);
    ecs.removeComponent<Transform>(hugo);
    ecs.removeEntity(valou);
    std::cout << "mina" << ecs.getComponent<Velocity>(hugo).speed << std::endl;
}*/

void testGeometry() {
    Matrix2 m{ 2.0, 1.0, 1.0, 1.0 };
    float det = determinant(m);
    
    Matrix2 inv = inverse(m, det);
    std::cout << det << (float)inv.a << inv.b << inv.c << inv.d << std::endl;
}

void testCollisions() {
    Vector2 A{ 0, -2 }, B{ 0, 4 }, O{ 7,0 }, P{ -5, 0 };
    Matrix2 sys(B-A, O-P);
    float det = determinant(sys);
    Vector2 param{inverse(sys, det) * (A-O)};
    std::cout << "Colisiion seg seg : " << collisionSegmentSegment(A, B, O, P) << std::endl;
    std::cout << "Collision seg seg a la mano: " << param.x << " " << param.y << std::endl;
}

struct A {
    int x;
};
struct B {
    int x;
    void f() {
        std::cout << "hello" << std::endl;
    }
};

void registerAllComponents() {
    ecs.registerComponent<BoxCollider>();
    ecs.registerComponent<MovingObject>();
    ecs.registerComponent<VisibleObject>();
}



void testIterators() {
    std::map<int, std::string> map;
    map.emplace(1, "hugo");
    map.emplace(2, "jean");
    map.emplace(3, "pierre");

    for (auto it{ map.begin() }; it != map.end(); ++it) {
        std::string& nom{ it->second };
        nom.push_back('X');
    }

    std::cout << map.find(1)->second << std::endl;
    std::cout << map.find(2)->second << std::endl;
}

void testStrings() {
    
    char b[]{ "Salut les idiots !" };
    char* ptr = b;
    std::string str(ptr);
    ptr[0] = 'X';
    std::cout << "str: " << str << std::endl;
    std::cout << "ptr: " << const_cast<const char*>(ptr) << std::endl;

}

void testCircularQueue() {
    ConcurrentCircularQueue<int> q(4);
    q.enqueue(1);
    q.enqueue(2);
    q.enqueue(3);
   
    q.enqueue(4);
    q.enqueue(5);
    q.enqueue(6);
    //q.startReading();
    q.read(0);
    q.read(1);

    q.enqueue(7);
}

int main(int argc, char* args[])
{

    SDL_Init(SDL_INIT_VIDEO);
    if (SDLNet_Init() == -1) {
        std::cout << "SDLNet_Init: " << SDLNet_GetError << std::endl;
        exit(2);
    }
    //testStrings();
    //testIterators();
    testCircularQueue();
    
    int choice = 1;
    std::cout << "TESTING TCP NEWORK\n===============================\n\n1. Server\n2. Client" << std::endl;
    std::cin >> choice;
    if (choice == 1) {
        registerAllComponents();
        Game game{};
        game.gameLoop();
    }
    else
        Client client{};

   /* testCollisions();
    testGeometry();
    testECS();*/

 

    SDL_Quit();
    SDLNet_Quit();

    return 0;
}