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
    screen_rect.w = world_rect.w * float(screen_w) / camera.w;
    screen_rect.h = world_rect.h * float(screen_h) / camera.h;
    return screen_rect;
}



struct RenderComponent {
	SDL_Rect position; //position relative to transform's origin
	int texture_id;
};

#define PLAYER_TEX 0
#define WALL_TEX 1
#define MAX_TEXTURES 2

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

        win = SDL_CreateWindow("Foster Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h, SDL_WINDOW_SHOWN);//SDL_WINDOW_RESIZABLE);
        renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

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
	void render(std::vector<MovingObject> const& players, std::vector<BoxCollider> const& playersColliders,
        std::vector<VisibleObject> const& plateforms, std::vector<BoxCollider> const& plateformColliders)
    {
        SDL_RenderClear(renderer);
        SDL_Rect position{};
        Rect position_world;
        const Rect camera{ 0.0f, 0.0f, 20.0f, 13.0f };


        for (int i = 0; i < plateforms.size(); i++) {
            //position_world.x = plateforms[i].oldPosition.x - float(plateformColliders[i].w) / 2.0f;
            //position_world.y = plateforms[i].oldPosition.y - float(plateformColliders[i].h) / 2.0f;
            position_world.w = plateformColliders[i].w;
            position_world.h = plateformColliders[i].h;
           // position = worldToScreenCoords(position_world, camera, window_w, window_h);

           // SDL_RenderFillRect(renderer, &position);


            position_world.x = plateforms[i].position.x - float(plateformColliders[i].w) / 2.0f;
            position_world.y = plateforms[i].position.y - float(plateformColliders[i].h) / 2.0f;
            position = worldToScreenCoords(position_world, camera, window_w, window_h);
            
            SDL_RenderCopy(renderer, textures[WALL_TEX], NULL, &position);
        }

        for (int i = 0; i < players.size(); i++) {
           // position_world.x = players[i].oldPosition.x - float(playersColliders[i].w) / 2.0f;
         //   position_world.y = players[i].oldPosition.y - float(playersColliders[i].h) / 2.0f;
            position_world.w = playersColliders[i].w;
            position_world.h = playersColliders[i].h;
            //position = worldToScreenCoords(position_world, camera, window_w, window_h);
           // SDL_RenderFillRect(renderer, &position);

            position_world.x = players[i].position.x - float(playersColliders[i].w) / 2.0f;
            position_world.y = players[i].position.y - float(playersColliders[i].h) / 2.0f;
            position = worldToScreenCoords(position_world, camera, window_w, window_h);
            SDL_RenderCopy(renderer, textures[PLAYER_TEX], NULL, &position);
        }
        
        SDL_RenderPresent(renderer);
	}

};