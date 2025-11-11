#include "SaveManager.h"

SaveManager::~SaveManager()
{
	in_stream.close();
	out_stream.close();
}

std::vector<uint8_t> SaveManager::OpenOrCreateSave(std::string path)
{
	path = path + ".sav";
	std::vector<uint8_t> game_RAM;
	in_stream.open(path, std::ios::binary);
	if (in_stream.is_open())
	{
		game_RAM = std::vector<uint8_t>(std::istreambuf_iterator<char>(in_stream), {});
	}
	else
	{
		game_RAM = std::vector<uint8_t>(0);
	}
	in_stream.close();
	if (out_stream.is_open()) out_stream.close();
	out_stream.open(path, std::ios::in | std::ios::binary);
	if (!out_stream.is_open())
	{
		out_stream.open(path, std::ios::out | std::ios::binary);
	}
	return game_RAM;
}

void SaveManager::WriteToSave(uint32_t address, uint8_t val)
{
	out_stream.seekp(address);
	out_stream.put(val);
}