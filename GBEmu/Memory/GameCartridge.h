#pragma once

#include <stdint.h>

#include "../SaveManager/SaveManager.h"

#include <memory>
#include <fstream>
#include <iterator>
#include <vector>
#include <ctime>
#include <SDL3/SDL.h>

constexpr auto MBC3_SAVE_OFFSET = 0x10;;

class GameCartridge
{
protected:
	std::vector<uint8_t> GameROM;
	std::string GamePath;
	std::vector<uint8_t> GameRAM;
	bool has_RAM;
	std::shared_ptr<SaveManager> saveManager;
public:
	virtual uint8_t getROMValue(uint16_t address) = 0;
	virtual uint8_t getRAMValue(uint16_t address) = 0;
	virtual void setROMValue(uint16_t address, uint8_t value) = 0;
	virtual void setRAMValue(uint16_t address, uint8_t value) = 0;
	virtual void tickRTC() = 0;
	bool supportsCGB();
};

class MBC0: public GameCartridge
{
public:
	MBC0() = default;
	MBC0(std::vector<uint8_t> ROM, bool RAM = false, std::shared_ptr<SaveManager> save_manager = nullptr, std::string path = "");
	uint8_t getROMValue(uint16_t address);
	uint8_t getRAMValue(uint16_t address);
	void setROMValue(uint16_t address, uint8_t value);
	void setRAMValue(uint16_t address, uint8_t value);
	void tickRTC() { return; }
};

class MBC1 : public GameCartridge
{
private:
	bool RAM_enabled;
	uint8_t ROM_bank;
	uint8_t ROM_bank_amount;
	uint8_t RAM_bank;
	bool banking_mode_1;

public:
	MBC1() = default;
	MBC1(std::vector<uint8_t> ROM, bool RAM = false, std::shared_ptr<SaveManager> save_manager = nullptr, std::string path = "");
	uint8_t getROMValue(uint16_t address);
	uint8_t getRAMValue(uint16_t address);
	void setROMValue(uint16_t address, uint8_t value);
	void setRAMValue(uint16_t address, uint8_t value);
	void tickRTC() { return; }
};

class MBC2 : public GameCartridge
{
private:
	bool RAM_enabled;
	uint8_t ROM_bank;
	uint8_t ROM_bank_amount;

public:
	MBC2() = default;
	MBC2(std::vector<uint8_t> ROM, std::shared_ptr<SaveManager> save_manager = nullptr, std::string path = "");
	uint8_t getROMValue(uint16_t address);
	uint8_t getRAMValue(uint16_t address);
	void setROMValue(uint16_t address, uint8_t value);
	void setRAMValue(uint16_t address, uint8_t value);
	void tickRTC() { return; }
};

class MBC3 : public GameCartridge
{
private:
	std::vector<uint8_t> latched_time;
	bool has_RAM;
	bool RAM_enabled;
	bool has_clock;
	bool rtc_halt;
	bool rtc_latched;
	uint16_t ROM_bank;
	uint16_t ROM_bank_amount;
	uint8_t RAM_bank;
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint16_t days;
	uint32_t ticks;
	uint64_t last_time;
	bool days_overflow;

public:
	MBC3() = default;
	MBC3(std::vector<uint8_t> ROM, bool RAM = false, bool clock = false, std::shared_ptr<SaveManager> save_manager = nullptr, std::string path = "");
	uint8_t getROMValue(uint16_t address);
	uint8_t getRAMValue(uint16_t address);
	void setROMValue(uint16_t address, uint8_t value);
	void setRAMValue(uint16_t address, uint8_t value);
	void tickRTC();
};

class MBC5 : public GameCartridge
{
private:
	bool has_RAM;
	bool has_rumble;
	bool RAM_enabled;
	uint16_t ROM_bank;
	uint16_t ROM_bank_amount;
	uint8_t RAM_bank;

public:
	MBC5() = default;
	MBC5(std::vector<uint8_t> ROM, bool RAM = false, bool rumble = false, std::shared_ptr<SaveManager> save_manager = nullptr, std::string path = "");
	uint8_t getROMValue(uint16_t address);
	uint8_t getRAMValue(uint16_t address);
	void setROMValue(uint16_t address, uint8_t value);
	void setRAMValue(uint16_t address, uint8_t value);
	void tickRTC() { return; }
};

std::unique_ptr<GameCartridge> load_ROM(std::string rom_path, std::shared_ptr<SaveManager> save_manager);
