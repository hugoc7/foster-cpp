#pragma once

#include "SDL.h"
#include "Geometry.h"
#include <vector>
#include "Components.h"

//les positions doivent etre flottantes ?
struct Rect {
    float x;
    float y;
    float w;
    float h;
};


Vector2 worldToScreenCoords(Vector2 const& world_pos, Rect const& camera, const int screen_w, const int screen_h) {
	Vector2 screen_pos;
	screen_pos.x = (world_pos.x - camera.x) * float(screen_w) / camera.w;
	screen_pos.y = (camera.y + camera.h - world_pos.y ) * float(screen_h) / camera.h;//axe des y inverse avec la SDL
	return screen_pos;
}

SDL_Rect worldToScreenCoords(Rect world_rect, Rect const& camera, const int screen_w, const int screen_h) {
    SDL_Rect screen_rect;
    screen_rect.x = std::roundf((world_rect.x - camera.x) * float(screen_w) / camera.w);
    screen_rect.y = std::roundf((camera.y + camera.h - world_rect.y) * float(screen_h) / camera.h);//axe des y inverse avec la SDL
    screen_rect.w = std::roundf(world_rect.w * float(screen_w) / camera.w);
    screen_rect.h = std::roundf(world_rect.h * float(screen_h) / camera.h);
    return screen_rect;
}



struct RenderComponent {
	SDL_Rect position; //position relative to transform's origin
	int texture_id;
};

#define PLAYER_TEX 0
#define WALL_TEX 1
#define MAX_TEXTURES 2

float quadraticDistance(SDL_Rect p, SDL_Rect q) {
    return  (p.x - q.x) * (p.x - q.x) + (p.y - q.y) * (p.y - q.y);
}

//needs transform + renderComponent
class Renderer {
private:
    SDL_Window* win = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Texture* textures[MAX_TEXTURES];
    int window_w{800};
    int window_h{600};
public:

    void getNewWindowSize() {
        SDL_GetWindowSize(win, &window_w, &window_h);
    }



    Renderer() {
        SDL_Surface* surfaces[MAX_TEXTURES]{};

        win = SDL_CreateWindow("Foster Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

        surfaces[PLAYER_TEX] = SDL_LoadBMP("img/player.bmp");
        surfaces[WALL_TEX] = SDL_LoadBMP("img/hello.bmp");

        for (int i = 0; i < MAX_TEXTURES; i++) {
            textures[i] = SDL_CreateTextureFromSurface(renderer, surfaces[i]);
            SDL_FreeSurface(surfaces[i]);
        }

    }
    ~Renderer() {
        for (int i = 0; i < MAX_TEXTURES; i++) {
            SDL_DestroyTexture(textures[i]);
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(win);

    }
	void render(ArrayList<EntityID> const& players, ArrayList<EntityID> const& plateforms)
    {
        SDL_RenderClear(renderer);
        SDL_Rect position{};
        Rect position_world;
        Rect camera{ 0.0f, 0.0f, 20.0f, 13.0f };
        const float maxDistanceFromRealPos = 2.0f * 2.0f; //quadratic distance to avoid square root calculus
        static SDL_Rect lastMove{ 0, 0, 0, 0 };
        //camera.x = players[0].position.x - camera.w/2.0f;
        //camera.y = players[0].position.y - camera.h / 2.0f;
        static SDL_Rect oldPosition{0, 0, 0, 0};


        for (int i = 0; i < plateforms.v.size(); i++) {

            BoxCollider& plateformCollider = ecs.getComponent<BoxCollider>(plateforms.v[i]);
            VisibleObject& plateform = ecs.getComponent<VisibleObject>(plateforms.v[i]);


            position_world.w = plateformCollider.w;
            position_world.h = plateformCollider.h;
            position_world.x = plateform.position.x - float(plateformCollider.w) / 2.0f;
            position_world.y = plateform.position.y + float(plateformCollider.h) / 2.0f;
            position = worldToScreenCoords(position_world, camera, window_w, window_h);
            
            SDL_RenderCopy(renderer, textures[WALL_TEX], NULL, &position);
        }

        for (int i = 0; i < players.v.size(); i++) {
            BoxCollider& playerCollider = ecs.getComponent<BoxCollider>(players.v[i]);
            MovingObject& player = ecs.getComponent<MovingObject>(players.v[i]);

            position_world.w = playerCollider.w;
            position_world.h = playerCollider.h;
         

            position_world.x = player.position.x - float(playerCollider.w) / 2.0f;
            position_world.y = player.position.y + float(playerCollider.h) / 2.0f;
            position = worldToScreenCoords(position_world, camera, window_w, window_h);

            SDL_Rect newMove{ position.x - oldPosition.x , position.y - oldPosition.y , 0, 0 };


            SDL_Rect newPosWithOldMove{oldPosition.x + lastMove.x, oldPosition.y + lastMove.y, 0, 0 };

            oldPosition.x = position.x;
            oldPosition.y = position.y;
            SDL_RenderCopy(renderer, textures[PLAYER_TEX], NULL, &position);

        }
        
        SDL_RenderPresent(renderer);
	}

};