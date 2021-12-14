#pragma once

#include "SDL.h"
#include <vector>
#include <iostream>
#include "Geometry.h"
#include "SDL_FontCache.h"
#include "Rendering.h"
#include "PlayerInfos.h"

//GUI class: a GUI class is apart from the ECS, it uses classical POO widgets representation
//TODO later: refactor the code such as GUI and GAME share the same rendering primitives and the same input primitives

//The score board is showed when tab is pressed and present the list of the players with their scores and stats
//Stats provided:
// PlayerIcon/Color | PlayerName | Score | Kills | Deaths | Headshots | Ping |
//For now we just show player names


class ScoreBoard {
public:
	SDL_Rect position{ 0, 0, 500, 250 };//x,y will be centered
	FC_Font* font;
	Renderer& renderer;
	std::string scoreText;
	bool visible{ false };


	ScoreBoard(Renderer& renderer) : renderer{ renderer } {
		font = FC_CreateFont();
		int r = FC_LoadFont(font, renderer.renderer, "fonts/Sansation_Regular.ttf", 30, FC_MakeColor(255, 0, 255, 255), TTF_STYLE_NORMAL);
	}

	void reloadFont() {
		FC_ClearFont(font);
		//font = FC_CreateFont();
		int r = FC_LoadFont(font, renderer.renderer, "fonts/Sansation_Regular.ttf", 30, FC_MakeColor(255, 0, 255, 255), TTF_STYLE_NORMAL);
	}
	void handleEvents(SDL_Event const& event) {
		const Uint8* state = SDL_GetKeyboardState(NULL);
		visible = state[SDL_SCANCODE_TAB];
	}

	void render(ArrayMap<PlayerInfos> const& playerInfos, std::string const& myName) {

		if (!visible) return;

		//center the score board in the screen
		position.x = renderer.window_w / 2 - position.w / 2;
		position.y = renderer.window_h / 2 - position.h / 2;

		SDL_SetRenderDrawColor(renderer.renderer, 190, 190, 190, 128);
		SDL_SetRenderDrawBlendMode(renderer.renderer, SDL_BLENDMODE_BLEND);
		SDL_RenderFillRect(renderer.renderer, &position);

		scoreText.clear();
		scoreText += "    - SCORE BOARD (";
		scoreText += myName;
		scoreText += ") -\n\n";
		for (int i = 0; i < playerInfos.Size(); i++) {
			scoreText += std::to_string(playerInfos[i].id) + " : ";
			scoreText += playerInfos[i].name;
			scoreText += "\n";
		}

		FC_DrawBox(font, renderer.renderer, position, scoreText.c_str());
		//FC_DrawColumn(font, renderer.renderer, position.x, position.y, position.w, "Hugo\nAlex\nPhil");
		//FC_Draw(font, renderer.renderer, 0, 0, "This is %s.\n It works.", "example text");
	}

	~ScoreBoard() {
		FC_FreeFont(font);
	}
};