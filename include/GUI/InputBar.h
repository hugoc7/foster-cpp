#pragma once

#include "SDL.h"
#include <vector>
#include <iostream>
#include "Geometry.h"
#include "SDL_FontCache.h"
#include "Rendering.h"

//GUI class: a GUI class is apart from the ECS, it uses classical POO widgets representation
//TODO later: refactor the code such as GUI and GAME share the same rendering primitives and the same input primitives

//Input bar used to ask the user to enter text


class InputBar {
public:
	SDL_Rect position{ 0, 0, 500, 250 };//x,y will be centered
	SDL_FRect relativePos{};
	FC_Font* font;
	Renderer& renderer;
	std::string text{};
	int input_position{0};
	char input_text[2048];//TODO: replace with text 
	bool active{ false };
	SDL_Rect cursorRect{};
	SDL_Color txtColor{0,0,0,255};
	Uint32 txtSize{ 18 };
	SDL_Color bckgColor{ 106, 84, 231, 110 }; 
	SDL_Color activeBckgColor{ 188, 179, 244, 150 };
	const int txtMargin = 30;



	InputBar(Renderer& renderer) : renderer{ renderer } {
		font = FC_CreateFont();
		int r = FC_LoadFont(font, renderer.renderer, "fonts/Sansation_Regular.ttf", txtSize, txtColor, TTF_STYLE_NORMAL);
	}
	void setText(std::string&& newText) {
		assert(newText.size() < 100);
		std::memcpy(input_text, newText.c_str(), newText.size() + 1);
		input_position = 0;
	}
	void stealText(std::string& destinationStr) {
		destinationStr = std::move(text);
		input_position = 0;
		input_text[0] = '\0';
	}

	void reloadFont() {
		FC_ClearFont(font);
		//font = FC_CreateFont();
		int r = FC_LoadFont(font, renderer.renderer, "fonts/Sansation_Regular.ttf", txtSize, txtColor, TTF_STYLE_NORMAL);
	}

	void updatePosition() {
	}
	void handleEvents(SDL_Event const& event) {

		if (!active) return;

		const Uint8* keystates = SDL_GetKeyboardState(NULL);
		
		if (event.type == SDL_KEYDOWN)
		{
			
			if (event.key.keysym.sym == SDLK_BACKSPACE)
			{
				U8_strdel(input_text, input_position - 1);

				--input_position;
				if (input_position < 0)
					input_position = 0;
			}
			else if (event.key.keysym.sym == SDLK_RETURN)
			{
				//U8_strinsert(input_text, input_position, "\n", 2048);
				//input_position++;
				text.assign(input_text);
			}
			else if (event.key.keysym.sym == SDLK_TAB)
			{
				U8_strinsert(input_text, input_position, "\t", 2048);
				input_position++;
			}
			else if (event.key.keysym.sym == SDLK_LEFT)
			{
				--input_position;
				if (input_position < 0)
					input_position = 0;
			}
			else if (event.key.keysym.sym == SDLK_RIGHT)
			{
				++input_position;
				int len = U8_strlen(input_text);
				if (input_position >= len)
					input_position = len;
			}
		}
		else if (event.type == SDL_TEXTINPUT)
		{
			if (U8_strinsert(input_text, input_position, event.text.text, 2048))
				input_position += U8_strlen(event.text.text);
		}
		else if (event.type == SDL_MOUSEBUTTONDOWN)
		{
			if (FC_InRect(event.button.x, event.button.y, position))
			{
				input_position = FC_GetPositionFromOffset(font, event.button.x - position.x, event.button.y - position.y, position.w, FC_ALIGN_LEFT, "%s", input_text);
			}
		}
		
		
	}

	void render() {

		if (active) {
			SDL_SetRenderDrawColor(renderer.renderer, activeBckgColor.r, activeBckgColor.g, activeBckgColor.b, activeBckgColor.a);
		}
		else {
			SDL_SetRenderDrawColor(renderer.renderer, bckgColor.r, bckgColor.g, bckgColor.b, bckgColor.a);
		}
		SDL_SetRenderDrawBlendMode(renderer.renderer, SDL_BLENDMODE_BLEND);
		SDL_RenderFillRect(renderer.renderer, &position);
		

		FC_Rect input_cursor_pos = FC_GetCharacterOffset(font, input_position, position.w, "%s", input_text);
		cursorRect = FC_MakeRect(position.x + input_cursor_pos.x,
			position.y + input_cursor_pos.y, input_cursor_pos.w, input_cursor_pos.h);

		SDL_SetRenderDrawColor(renderer.renderer, txtColor.r, txtColor.g, txtColor.b, txtColor.a);//cursor color
		if (active) {
			if (SDL_GetTicks() % 1000 < 500)
				SDL_RenderFillRect(renderer.renderer, &cursorRect);
		}
		
		if (active) {
			SDL_SetRenderDrawColor(renderer.renderer, 0, 0, 255, 255);
			SDL_RenderDrawRect(renderer.renderer, &position);
		}
		/*SDL_Rect txtPos{ position };
		txtPos.x += txtMargin;
		txtPos.w -= 2 * txtMargin;*/
		FC_DrawBox(font, renderer.renderer, position, input_text);
		//FC_DrawColumn(font, renderer.renderer, position.x, position.y, position.w, "Hugo\nAlex\nPhil");
		//FC_Draw(font, renderer.renderer, 0, 0, "This is %s.\n It works.", "example text");
	}

	~InputBar() {
		FC_FreeFont(font);
	}
};