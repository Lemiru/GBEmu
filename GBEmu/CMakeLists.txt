find_package(SDL3 REQUIRED)
find_package(imgui REQUIRED)

add_executable (GBEmu "GBEmu.cpp" "GBEmu.h" "CPU/CPU.cpp" "CPU/CPU.h" "Memory/MemoryManager.h" "Memory/MemoryManager.cpp" "Memory/GameCartridge.h" "Memory/GameCartridge.cpp" "Display/Display.h" "Display/Display.cpp" "Input/InputHandler.h" "Input/InputHandler.cpp" "Timer/Timer.h" "Timer/Timer.cpp" "SaveManager/SaveManager.cpp" "SaveManager/SaveManager.h" "Audio/Audio.cpp" "Audio/Audio.h" "Audio/Channels.cpp" "Audio/Channels.h" "Utils/ImGui.h" "Utils/ImGui.cpp" "Utils/Config.cpp"   )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET GBEmu PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(GBEmu PRIVATE SDL3::SDL3 imgui::imgui)