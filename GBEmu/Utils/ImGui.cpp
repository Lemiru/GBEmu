#include "ImGui.h"

static int MENU_HEIGHT = 0;

int getMenuHeight()
{
    if (MENU_HEIGHT == 0)
    {
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        MENU_HEIGHT = ImGui::GetFrameHeight();
        ImGui::Render();
    }
    Config::setMenuHeight(MENU_HEIGHT);
    return MENU_HEIGHT;
}

void startImGui(SDL_Window* window, SDL_Renderer* renderer)
{
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
}
void stopImGui()
{
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui::DestroyContext();
}
void drawImGuiMenu(SDL_Renderer* renderer)
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("General"))
        {
            if (ImGui::MenuItem("Restart")) {
                Config::initiateReset();
            }
            if (ImGui::MenuItem("DMG mode", NULL, Config::getDMGMode())) {
                Config::setDMGMode(!Config::getDMGMode());
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            {
                ImGui::SetTooltip("Forces the emulator\nto run in DMG mode.\nRequires restart.");
            }
            if (ImGui::MenuItem("Quit")) {
                //properly close SDL and ImGui
                Config::writeToFile("config.ini");
                exit(0);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Resolution")) {
            int resScale = Config::getResolutionScale();
            if (ImGui::MenuItem("Original", NULL, resScale == 1)) {
                Config::setReslutionScale(1);
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            {
                ImGui::SetTooltip("Original resolution\nwill obscure some\nsettings options due\nto low available\nspace");
            }
            if (ImGui::MenuItem("x2", NULL, resScale == 2)) {
                Config::setReslutionScale(2);
            }
            if (ImGui::MenuItem("x3", NULL, resScale == 3)) {
                Config::setReslutionScale(3);
            }
            if (ImGui::MenuItem("x4", NULL, resScale == 4)) {
                Config::setReslutionScale(4);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Speed")) {
            float spdScale = Config::getSpeedScale();
            if (ImGui::MenuItem("x0.5", NULL, spdScale == 0.5f)) {
                Config::setSpeedScale(0.5f);
            }
            if (ImGui::MenuItem("Original", NULL, spdScale == 1.0f)) {
                Config::setSpeedScale(1.0f);
            }
            if (ImGui::MenuItem("x2", NULL, spdScale == 2.0f)) {
                Config::setSpeedScale(2.0f);
            }
            if (ImGui::MenuItem("x4", NULL, spdScale == 4.0f)) {
                Config::setSpeedScale(4.0f);
            }
            if (ImGui::MenuItem("x8", NULL, spdScale == 8.0f)) {
                Config::setSpeedScale(8.0f);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Audio"))
        {
            static float vol = Config::getVolume();
            ImGui::SliderFloat("Volume", &vol, 0.0f, 1.0f);
            Config::setVolume(vol);
            if (ImGui::MenuItem("Channel 1", NULL, Config::getChannelEnabled(1))) {
                Config::flipChannelEnabled(1);
            }
            if (ImGui::MenuItem("Channel 2", NULL, Config::getChannelEnabled(2))) {
                Config::flipChannelEnabled(2);
            }
            if (ImGui::MenuItem("Channel 3", NULL, Config::getChannelEnabled(3))) {
                Config::flipChannelEnabled(3);
            }
            if (ImGui::MenuItem("Channel 4", NULL, Config::getChannelEnabled(4))) {
                Config::flipChannelEnabled(4);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}