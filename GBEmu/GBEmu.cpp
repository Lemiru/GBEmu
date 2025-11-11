#include "GBEmu.h"

uint32_t CYCLE_TIME = (1 / (4194304 / 4)) * 1000000000;

int main(int argc, char* argv[]) {
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "DirectSound");

    Config::init();

    SDL_Window* window;
    bool useKeyboard = false;
    bool done = false;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);
    window = SDL_CreateWindow(
        "GBEmu",
        160 * Config::getResolutionScale(),
        144 * Config::getResolutionScale(),
        SDL_WINDOW_OPENGL
    );
    Config::setWindow(window);
    if (window == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
        return 1;
    }
    const SDL_AudioSpec spec = { SDL_AUDIO_F32, 2, 32768};

    const uint16_t buffer_len = 1140;
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, "opengl");

    startImGui(window, renderer);

    SDL_SetWindowSize(window, 160 * Config::getResolutionScale(), 144 * Config::getResolutionScale() + getMenuHeight());

    SDL_AudioStream* audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    Config::setAudioStream(audio_stream);
    SDL_ResumeAudioStreamDevice(audio_stream);
    const char* path = NULL;
    while (path == NULL and !done)
    {
        SDL_Delay(10);
        SDL_Event event;
        while (SDL_PollEvent(&event) and !done) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            switch (event.type) {
            case SDL_EVENT_DROP_FILE:
                path = event.drop.data;
                break;
            case SDL_EVENT_QUIT:
                done = true;
                break;
            }
        }

        drawImGuiMenu(renderer);
        SDL_RenderPresent(renderer);
        SDL_RenderClear(renderer);
    }
    if (path == NULL) path = "";
    std::shared_ptr<SaveManager> save_manager = std::make_shared<SaveManager>();
    MemoryManager MEM = MemoryManager(path, save_manager);
    CPU GB_CPU = CPU(&MEM);
    Audio GB_Audio = Audio(&MEM, buffer_len, audio_stream);
    Display GB_Display = Display(renderer, &MEM);
    InputHandler GB_Input = InputHandler(&MEM);
    Timer GB_Timer = Timer(&MEM);
    

    uint16_t cycle_count_for_input = 0;

    SDL_JoystickID joystick_id = NULL;
    SDL_Gamepad* gamepad = nullptr;

    while (!done) {
        SDL_Event event;

        if (cycle_count_for_input == 17556)
        {
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL3_ProcessEvent(&event);
                switch (event.type) {
                case SDL_EVENT_GAMEPAD_ADDED:
                    if (joystick_id != event.gdevice.which)
                    {
                        joystick_id = event.gdevice.which;
                        gamepad = SDL_OpenGamepad(joystick_id);
                    }
                    useKeyboard = false;
                    break;

                case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                case SDL_EVENT_GAMEPAD_BUTTON_UP:
                    if (joystick_id != event.gbutton.which)
                    {
                        joystick_id = event.gbutton.which;
                        gamepad = SDL_GetGamepadFromID(joystick_id);
                    }
                    useKeyboard = false;
                    break;

                case SDL_EVENT_KEY_DOWN:
                    useKeyboard = true;
                    break;

                case SDL_EVENT_DROP_FILE:
                    Config::initiateReset(event.drop.data);
                    cycle_count_for_input = 0;
                    break;

                case SDL_EVENT_QUIT:
                    done = true;
                    Config::writeToFile("config.ini");
                    break;
                }
            }
            if (useKeyboard)
            {
                GB_Input.set_inputs();
            }
            else
            {
                GB_Input.set_inputs(gamepad);
            }
            cycle_count_for_input = 0;

        }
        if (Config::isResetPending())
        {
            MEM.reset(Config::getPathForReset());
            GB_CPU.reset();
            GB_Timer.reset();
            GB_Audio.reset();
            GB_Display.reset();
            Config::finishReset();
        }

        GB_Input.update();
        GB_Timer.cycle();
        if (!MEM.isPerformingGPDMA())
        {
            GB_CPU.cycle();
        }
        GB_Audio.cycle();
        MEM.cycle();
        GB_Display.cycle();
        if (MEM.isInDoubleSpeedMode())
        {
            if (!MEM.isPerformingGPDMA())
            {
                GB_CPU.cycle();
            }
            GB_Timer.cycle();
            MEM.cycle(true);
        }
        cycle_count_for_input++;
    }
    SDL_DestroyWindow(window);
    SDL_DestroyAudioStream(audio_stream);
    SDL_Quit();

    return 0;
}