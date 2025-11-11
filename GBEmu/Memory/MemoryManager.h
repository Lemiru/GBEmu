#pragma once

#include <stdint.h>

#include "GameCartridge.h"
#include "../SaveManager/SaveManager.h"
#include "../Audio/Channels.h"
#include "../Utils/Config.h"
#include <iostream>
#include <algorithm>

enum Interrupt
{
	VBLANK_INTERRUPT = 0,
	STAT_INTERRUPT = 1,
	TIMER_INTERRUPT = 2,
	SERIAL_INTERRUPT = 3,
	JOYPAD_INTERRUPT = 4
};

class MemoryManager 
{
private:
	std::vector<uint8_t> bootROM;
	std::vector<uint8_t> VRAM0;
	std::vector<uint8_t> VRAM1;
	std::vector<uint8_t> WorkRAM0;
	std::vector<uint8_t> WorkRAM1;
	std::vector<uint8_t> WorkRAM2;
	std::vector<uint8_t> WorkRAM3;
	std::vector<uint8_t> WorkRAM4;
	std::vector<uint8_t> WorkRAM5;
	std::vector<uint8_t> WorkRAM6;
	std::vector<uint8_t> WorkRAM7;
	std::vector<uint8_t> OAM;
	std::vector<uint8_t> NU;
	std::vector<uint8_t> IOREG;
	std::vector<uint8_t> HRAM;
	std::vector<uint8_t> BCRAM;
	std::vector<uint8_t> OCRAM;
	uint8_t IE;
	std::unique_ptr<GameCartridge> gameCart;
	std::shared_ptr<SaveManager> saveManager;
	std::shared_ptr<PulseWithSweep> ch1;
	std::shared_ptr<Pulse> ch2;
	std::shared_ptr<Wave> ch3;
	std::shared_ptr<Noise> ch4;
	bool OAM_DMA_in_progress;
	bool DMA_in_progress;
	bool general_purpose_DMA;
	bool double_speed;
	bool cpu_halted;
	bool div_reset;
	bool boot_ROM_enabled;
	bool force_dmg;
	bool audio_length_first_half;
	uint8_t display_mode;
	uint8_t OAM_DMA_progress;
	uint16_t OAM_DMA_address;
	uint16_t DMA_source_address;
	uint16_t DMA_dest_address;
	uint16_t DMA_offset;

public: 
	MemoryManager(std::string rom_path, std::shared_ptr<SaveManager> saveManager);
	uint8_t getMemValue(uint16_t address) const;
	uint8_t getIOREGValue(uint16_t address) const;
	uint8_t getVRAMValue(uint16_t address, bool bank1 = false) const;
	uint8_t getBCRAMValue(uint8_t address) const;
	uint8_t getOCRAMValue(uint8_t address) const;
	void loadBootROM();
	void reset(std::string rom_path);
	void setAudioChannelRefs(std::shared_ptr<PulseWithSweep> c1, std::shared_ptr<Pulse> c2, std::shared_ptr<Wave> c3, std::shared_ptr<Noise> c4);
	void setMemValue(uint16_t address, uint8_t val);
	void setIOREGValue(uint16_t address, uint8_t val);
	void setVRAMValue(uint16_t address, uint8_t val, bool bank1 = false);
	void setCPUHalted(bool val);
	void setAudioLengthFirstHalf(bool val);
	void setDisplayMode(uint8_t val);
	void requestInterrupt(Interrupt inter);
	void discardInterrupt(Interrupt inter);
	void performSpeedSwitch();
	void cycle(bool ignore_GPDMA = false);
	void cycleHDMA();
	bool bootROMAvailable() const;
	bool isInCGBMode() const;
	bool isInDoubleSpeedMode() const;
	bool isPerformingGPDMA() const;
	bool wasDIVResetPerformed();

private:
	void set_initial_register_values();
	void cycle_GPDMA();
	void start_OAM_DMA();
};