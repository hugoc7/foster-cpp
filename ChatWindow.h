#pragma once

#include "SDL.h"
#include <vector>
#include <iostream>
#include "Geometry.h"
#include "SDL_FontCache.h"
#include "Rendering.h"
#include "PlayerInfos.h"
#include "Containers.h"
#include "Network.h"
#define MAX_CHAT_MESSAGES 20
//GUI chat window

class ChatWindow {
public:
	SDL_Rect position{ 0, 0, 500, 250 };//x,y will be modified
	FC_Font* font;
	Renderer& renderer;
	std::string chatText;
	CircularQueue<std::string> messages{MAX_CHAT_MESSAGES};//TODO: do not need to be thread safe


	ChatWindow(Renderer& renderer) : renderer{ renderer } {
		font = FC_CreateFont();
		int r = FC_LoadFont(font, renderer.renderer, "fonts/Sansation_Regular.ttf", 12, FC_MakeColor(255, 0, 255, 255), TTF_STYLE_NORMAL);
	}

	void reloadFont() {
		FC_ClearFont(font);
		//font = FC_CreateFont();
		int r = FC_LoadFont(font, renderer.renderer, "fonts/Sansation_Regular.ttf", 12, FC_MakeColor(255, 0, 255, 255), TTF_STYLE_NORMAL);
	}

	void render() {
		//center the score board in the screen
		position.x = 0;
		position.y = renderer.window_h - position.h;

		SDL_SetRenderDrawColor(renderer.renderer, 190, 190, 190, 128);
		SDL_SetRenderDrawBlendMode(renderer.renderer, SDL_BLENDMODE_BLEND);
		SDL_RenderFillRect(renderer.renderer, &position);

		chatText.clear();
		chatText += "    - CHAT WITH OTHER PLAYERS -\n";
		for (int i = 0; i < messages.length(); i++) {
			chatText += messages.read(i) + "\n";
		}
		FC_DrawBox(font, renderer.renderer, position, chatText.c_str());
		//FC_DrawColumn(font, renderer.renderer, position.x, position.y, position.w, "Hugo\nAlex\nPhil");
		//FC_Draw(font, renderer.renderer, 0, 0, "This is %s.\n It works.", "example text");
	}

	~ChatWindow() {
		FC_FreeFont(font);
	}
};