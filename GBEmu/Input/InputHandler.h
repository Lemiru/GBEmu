#pragma once

#include <stdint.h>

#include "../Memory/MemoryManager.h"
#include <SDL3/SDL.h>

class InputHandler
{
private:
	MemoryManager* mem;
	uint8_t last_value;
	uint8_t directional;
	uint8_t buttons;
	bool updated_inputs;
	const bool* keyboard_state;

public:
	InputHandler(MemoryManager* memory);
	void set_inputs();
	void set_inputs(SDL_Gamepad* gamepad);
	void update();
};