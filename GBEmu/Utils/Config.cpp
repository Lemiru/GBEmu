#include "Config.h"

static std::string PATH = "";
static bool RESET;
static bool DMG_MODE;
static SDL_Window* WINDOW = NULL;
static SDL_AudioStream* STREAM = NULL;
static int MENU_HEIGHT;
static int RESOLUTION_SCALE;
static float VOLUME;
static float SPEED_SCALE;
static bool CHANNELS_ENABLE[4];

enum class StringCode {
	undefined,
	dmg_mode,
	resolution_scale,
	speed_scale,
	volume,
};

StringCode string_to_enum(std::string val)
{
	if (val == "dmg_mode") return StringCode::dmg_mode;
	if (val == "resolution_scale") return StringCode::resolution_scale;
	if (val == "speed_scale") return StringCode::speed_scale;
	if (val == "volume") return StringCode::volume;
	return StringCode::undefined;
}

void Config::init()
{
	RESET = false;
	DMG_MODE = false;
	RESOLUTION_SCALE = 2;
	VOLUME = 1.0f;
	SPEED_SCALE = 1.0f;
	CHANNELS_ENABLE[0] = true;
	CHANNELS_ENABLE[1] = true;
	CHANNELS_ENABLE[2] = true;
	CHANNELS_ENABLE[3] = true;
	readFromFile("config.ini");
}

bool Config::readFromFile(std::string path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		return false;
	}
	std::string line;
	while (std::getline(file, line))
	{
		int pos = line.find("=");
		if (pos == std::string::npos or pos < 1)
		{
			continue;
		}
		std::string prop = line.substr(0, pos);
		std::string::iterator end_pos = std::remove(prop.begin(), prop.end(), ' ');
		prop.erase(end_pos, prop.end());
		std::string val = line.substr(pos + 1);
		end_pos = std::remove(val.begin(), val.end(), ' ');
		val.erase(end_pos, val.end());
		switch (string_to_enum(prop))
		{
		case StringCode::dmg_mode:
			if (val == "true" or val == "1")
			{
				DMG_MODE = true;
			}
			break;
		case StringCode::resolution_scale:
			try
			{
				RESOLUTION_SCALE = std::stoi(val);
				if (RESOLUTION_SCALE < 1)
				{
					RESOLUTION_SCALE = 2;
					printf("invalid resolution scale\n");
				}
			}
			catch(std::invalid_argument)
			{
				printf("invalid resolution scale\n");
			}
			break;
		case StringCode::speed_scale:
			try
			{
				SPEED_SCALE = std::stof(val);
				if (SPEED_SCALE < 0)
				{
					SPEED_SCALE = 1;
					printf("invalid speed scale\n");
				}
			}
			catch(std::invalid_argument)
			{
				printf("invalid speed scale\n");
			}
			break;
		case StringCode::volume:
			try
			{
				VOLUME = std::stof(val);
				if (VOLUME < 0 or VOLUME > 1)
				{
					VOLUME = 1;
					printf("invalid volume\n");
				}
			}
			catch (std::invalid_argument)
			{
				printf("invalid volume\n");
			}
			break;
		}
	}
	file.close();
	return false;
}

void Config::writeToFile(std::string path)
{
	std::ofstream file(path, std::ios::trunc);
	if (file.is_open())
	{
		file << "dmg_mode=" << DMG_MODE << std::endl;
		file << "resolution_scale=" << RESOLUTION_SCALE << std::endl;
		file << "speed_scale=" << SPEED_SCALE << std::endl;
		file << "volume=" << VOLUME << std::endl;
		file.close();
	}
	else
	{
		printf("Failed to open file\n");
	}
}

void Config::setWindow(SDL_Window* win)
{
	WINDOW = win;
}

void Config::setAudioStream(SDL_AudioStream* stream)
{
	STREAM = stream;
	SDL_SetAudioStreamGain(STREAM, VOLUME);
}

void Config::setMenuHeight(int h)
{
	MENU_HEIGHT = h;
}

void Config::initiateReset(std::string path)
{
	RESET = true;
	if(path != "")
	{
		PATH = path;
	}
}

bool Config::isResetPending()
{
	return RESET;
}

void Config::finishReset()
{
	RESET = false;
}

void Config::setDMGMode(bool val)
{
	DMG_MODE = val;
}

bool Config::getDMGMode()
{
	return DMG_MODE;
}

std::string Config::getPathForReset()
{
	return PATH;
}

float Config::getVolume()
{
	return VOLUME;
}
void Config::setVolume(float vol)
{
	VOLUME = vol;
	SDL_SetAudioStreamGain(STREAM, VOLUME);
}

int Config::getResolutionScale()
{
	return RESOLUTION_SCALE;
}
void Config::setReslutionScale(int scale)
{
	RESOLUTION_SCALE = scale;
	SDL_SetWindowSize(WINDOW, 160 * RESOLUTION_SCALE, 144 * RESOLUTION_SCALE + MENU_HEIGHT);
}
float Config::getSpeedScale()
{
	return SPEED_SCALE;
}
void Config::setSpeedScale(float scale)
{
	SPEED_SCALE = scale;
	SDL_SetAudioStreamFrequencyRatio(STREAM, scale);
}

bool Config::getChannelEnabled(uint8_t no)
{
	return CHANNELS_ENABLE[no - 1];
}

void Config::flipChannelEnabled(uint8_t no)
{
	CHANNELS_ENABLE[no - 1] = !CHANNELS_ENABLE[no - 1];
}
