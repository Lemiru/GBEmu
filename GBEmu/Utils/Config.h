#pragma once

#include <SDL3/SDL.h>
#include <algorithm>
#include <string>
#include <fstream>

namespace Config
{
	void init();
	bool readFromFile(std::string path);
	void writeToFile(std::string path);
	void setWindow(SDL_Window* win);
	void setAudioStream(SDL_AudioStream* stream);
	void setMenuHeight(int h);
	void initiateReset(std::string path = "");
	bool isResetPending();
	void finishReset();
	void setDMGMode(bool val);
	bool getDMGMode();
	std::string getPathForReset();
	float getVolume();
	void setVolume(float vol);
	int getResolutionScale();
	void setReslutionScale(int scale);
	float getSpeedScale();
	void setSpeedScale(float scale);
	bool getChannelEnabled(uint8_t no);
	void flipChannelEnabled(uint8_t no);
}