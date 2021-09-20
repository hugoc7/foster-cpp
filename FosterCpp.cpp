
#include <iostream>
#include "SDL.h"
#include "Collisions.h"
#include "Geometry.h"
#include <iostream>
#include "ECS.h"
#include "Game.h"

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
}

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

int main(int argc, char* args[])
{

    SDL_Init(SDL_INIT_VIDEO);

   /* testCollisions();
    testGeometry();
    testECS();*/

    Game game{};
    game.gameLoop();

    SDL_Quit();

    return 0;
}