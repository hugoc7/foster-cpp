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
#include "InputBar.h"

#define MAX_CHAT_MESSAGES 30

//GUI chat window

class ChatWindow {
public:
	SDL_Rect position{ 0, 0, 350, 250 };//x,y will be modified
	FC_Font* font;
	Renderer& renderer;
	std::string chatText;
	CircularQueue<std::string> messages{MAX_CHAT_MESSAGES};
	InputBar inputBar;
	std::string msgToSend{};
	Uint32 txtSize{ 18 };
	SDL_Color txtColor{ 0,0,0,255 };
	const int linesMax = 11;





	ChatWindow(Renderer& renderer) : renderer{ renderer }, inputBar{ renderer } {
		font = FC_CreateFont();
		int r = FC_LoadFont(font, renderer.renderer, "fonts/Sansation_Regular.ttf", txtSize, txtColor, TTF_STYLE_NORMAL);
	}

	void reloadFont() {
		FC_ClearFont(font);
		//font = FC_CreateFont();
		int r = FC_LoadFont(font, renderer.renderer, "fonts/Sansation_Regular.ttf", txtSize, txtColor, TTF_STYLE_NORMAL);
		inputBar.reloadFont();
	}

	void updateText() {
		chatText.clear();
		for (int i = 0; i < messages.length(); i++) {
			chatText += messages.read(i) + "\n";
		}

		//Calcul du nombre de lignes
		int maxWidth{ position.w };
		int lines = 0;
		int minCharsPerLine = maxWidth / (txtSize);//this is an overestimation
		int first = 0;

		UniqueByteBuffer buffer(chatText.size() + chatText.size() / minCharsPerLine + 1);
		int n = FC_GetWrappedText(font, buffer.get(), buffer.Size(), maxWidth, chatText.c_str());

		for (int i = n - 1; i >= 0; i--) {
			if (buffer.get()[i] == '\n') {
				lines++;
				if (lines >= linesMax) {
					first = std::min(i + 1, n - 1);
					break;
				}
			}
		}
		chatText = std::move(chatText.substr(first));
	}
	

	inline void handleEvents(SDL_Event const& event, TCPClient& client) {
		inputBar.handleEvents(event);

		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {

			//if the user pressed enter after entering text, send the message
			if (inputBar.active) {
				inputBar.stealText(msgToSend);
				if (msgToSend.size() > 0) {
					client.sendChatMsg(std::move(msgToSend));
				}
				inputBar.active = false;
			}
			else {
				inputBar.active = true;
			}
		}
	}
	

	void render() {
		inputBar.position.h = 30;
		inputBar.position.w = position.w;

		position.x = 0;
		position.y = renderer.window_h - position.h - inputBar.position.h;

		inputBar.position.x = 0;
		inputBar.position.y = position.y + position.h;


		SDL_SetRenderDrawColor(renderer.renderer, 190, 190, 190, 100);
		SDL_SetRenderDrawBlendMode(renderer.renderer, SDL_BLENDMODE_BLEND);
		SDL_RenderFillRect(renderer.renderer, &position);

		
		FC_DrawBox(font, renderer.renderer, position, chatText.c_str());
		//FC_DrawColumn(font, renderer.renderer, position.x, position.y, position.w, "Hugo\nAlex\nPhil");
		//FC_Draw(font, renderer.renderer, 0, 0, "This is %s.\n It works.", "example text");
		inputBar.render();
	}

	~ChatWindow() {
		FC_FreeFont(font);
	}
};