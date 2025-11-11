#include "InputHandler.h"

InputHandler::InputHandler(MemoryManager* memory)
{
	mem = memory;
	keyboard_state = SDL_GetKeyboardState(NULL);
	last_value = 0xFF;
	updated_inputs = false;
	directional = 0x00;
	buttons = 0x00;
}

void InputHandler::set_inputs()
{
	updated_inputs = true;
	directional = 0xF0 + (0x01 * keyboard_state[SDL_SCANCODE_RIGHT]) + (0x02 * keyboard_state[SDL_SCANCODE_LEFT])
		+ (0x04 * keyboard_state[SDL_SCANCODE_UP]) + (0x08 * keyboard_state[SDL_SCANCODE_DOWN]);
	buttons = 0xF0 + (0x01 * keyboard_state[SDL_SCANCODE_X]) + (0x02 * keyboard_state[SDL_SCANCODE_Z])
		+ (0x04 * keyboard_state[SDL_SCANCODE_O]) + (0x08 * keyboard_state[SDL_SCANCODE_P]);
}

void InputHandler::set_inputs(SDL_Gamepad* gamepad)
{
	if (gamepad != NULL)
	{
		updated_inputs = true;
		directional = 0xF0 + (0x01 * SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) + (0x02 * SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT)) 
			+ (0x04 * SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP)) + (0x08 * SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN));
		buttons = 0xF0 + (0x01 * SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH)) + (0x02 * SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST))
			+ (0x04 * SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_BACK)) + (0x08 * SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START));
	}
}

void InputHandler::update()
{
	uint8_t p1_value = mem->getIOREGValue(0xFF00);
	if (updated_inputs or last_value != p1_value)
	{
		last_value = p1_value;
		updated_inputs = false;
		p1_value = (p1_value & 0b11110000);
		switch ((p1_value & 0x30))
		{
		case 0:   //BOTH
			mem->setIOREGValue(0xFF00, (p1_value | ((~directional) & (~buttons))));
			break;
		case 16:  //BUTTONS
			mem->setIOREGValue(0xFF00, (p1_value | ~buttons));
			break;
		case 32:  //DPAD
			mem->setIOREGValue(0xFF00, (p1_value | ~directional));
			break;
		case 48:  //NONE
			mem->setIOREGValue(0xFF00, (p1_value | 0x0F));
			break;
		}
	}
}