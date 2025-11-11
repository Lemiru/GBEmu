#pragma once

#include <stdint.h>

#include <fstream>
#include <vector>

class SaveManager {
private:
	std::ofstream out_stream;
	std::ifstream in_stream;

public:
	~SaveManager();
	std::vector<uint8_t> OpenOrCreateSave(std::string filename);
	void WriteToSave(uint32_t address, uint8_t val);
};