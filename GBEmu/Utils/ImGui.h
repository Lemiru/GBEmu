#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <stdio.h>
#include "Config.h"

int getMenuHeight();
void startImGui(SDL_Window* window, SDL_Renderer* renderer);
void stopImGui();
void drawImGuiMenu(SDL_Renderer* renderer);