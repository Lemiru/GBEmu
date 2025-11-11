#include "MemoryManager.h"

MemoryManager::MemoryManager(std::string rom_path, std::shared_ptr<SaveManager> save_manager)
{
	saveManager = save_manager;
	gameCart = nullptr;
	//loadBootROM();
	reset(rom_path);
}

void MemoryManager::loadBootROM()
{
	if (isInCGBMode())
	{
		return;
		std::ifstream input("boot/cgb_boot.bin", std::ios::binary);
		if (input.good() == 0)
		{
			bootROM = std::vector<uint8_t>(0);
			return;
		}
		printf("boot rom found\n");
		bootROM = std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
	}
	else
	{
		std::ifstream input("boot/dmg_boot.bin", std::ios::binary);
		if (input.good() == 0)
		{
			bootROM = std::vector<uint8_t>(0);
			return;
		}
		printf("boot rom found\n");
		bootROM = std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
	}
	
}

void MemoryManager::reset(std::string rom_path)
{
	VRAM0 = std::vector<uint8_t>(0x2000, 0);
	VRAM1 = std::vector<uint8_t>(0x2000, 0);
	WorkRAM0 = std::vector<uint8_t>(0x1000);
	WorkRAM1 = std::vector<uint8_t>(0x1000);
	WorkRAM2 = std::vector<uint8_t>(0x1000);
	WorkRAM3 = std::vector<uint8_t>(0x1000);
	WorkRAM4 = std::vector<uint8_t>(0x1000);
	WorkRAM5 = std::vector<uint8_t>(0x1000);
	WorkRAM6 = std::vector<uint8_t>(0x1000);
	WorkRAM7 = std::vector<uint8_t>(0x1000);
	std::srand(unsigned(std::time(nullptr)));
	std::generate(WorkRAM0.begin(), WorkRAM0.end(), std::rand);
	std::generate(WorkRAM1.begin(), WorkRAM1.end(), std::rand);
	OAM = std::vector<uint8_t>(0xA0, 0);
	NU = std::vector<uint8_t>(0x60, 0);
	IOREG = std::vector<uint8_t>(0x80);
	HRAM = std::vector<uint8_t>(0x7F, 0);
	BCRAM = std::vector<uint8_t>(0x40, 0);
	OCRAM = std::vector<uint8_t>(0x40, 0);
	IE = 0x00;
	if (rom_path != "") gameCart = load_ROM(rom_path, saveManager);
	if (bootROM.size() != 0)
	{
		IOREG[0x50] = 0;
		boot_ROM_enabled = true;
	}
	else
	{
		boot_ROM_enabled = false;
		set_initial_register_values();
	}
	display_mode = 0;
	OAM_DMA_in_progress = false;
	DMA_in_progress = false;
	general_purpose_DMA = false;
	double_speed = false;
	cpu_halted = false;
	div_reset = false;
	audio_length_first_half = false;
	force_dmg = Config::getDMGMode();
	DMA_source_address = 0;
	DMA_dest_address = 0;
	DMA_offset = 0;
}

void MemoryManager::setAudioChannelRefs(std::shared_ptr<PulseWithSweep> c1, std::shared_ptr<Pulse> c2, std::shared_ptr<Wave> c3, std::shared_ptr<Noise> c4)
{
	ch1 = c1;
	ch2 = c2;
	ch3 = c3;
	ch4 = c4;
}

void MemoryManager::set_initial_register_values()
{
	IOREG[0x00] = 0xCF;
	IOREG[0x01] = 0x00;
	IOREG[0x02] = 0x7E;
	IOREG[0x04] = 0xAB;
	IOREG[0x05] = 0x00;
	IOREG[0x06] = 0x00;
	IOREG[0x07] = 0xF8;
	IOREG[0x0F] = 0xE1;
	IOREG[0x10] = 0x80;
	IOREG[0x11] = 0xBF;
	IOREG[0x12] = 0xF3;
	IOREG[0x13] = 0xFF;
	IOREG[0x14] = 0xBF;
	IOREG[0x16] = 0x3F;
	IOREG[0x17] = 0x00;
	IOREG[0x18] = 0xFF;
	IOREG[0x19] = 0xBF;
	IOREG[0x1A] = 0x7F;
	IOREG[0x1B] = 0xFF;
	IOREG[0x1C] = 0x9F;
	IOREG[0x1D] = 0xFF;
	IOREG[0x1E] = 0xBF;
	IOREG[0x20] = 0xFF;
	IOREG[0x21] = 0x00;
	IOREG[0x22] = 0x00;
	IOREG[0x23] = 0xBF;
	IOREG[0x24] = 0x77;
	IOREG[0x25] = 0xF3;
	IOREG[0x26] = 0xF1;
	IOREG[0x40] = 0x91;
	IOREG[0x41] = 0x81;
	IOREG[0x42] = 0x00;
	IOREG[0x43] = 0x00;
	IOREG[0x44] = 0x00;
	IOREG[0x45] = 0x00;
	IOREG[0x47] = 0xFC;
	IOREG[0x48] = 0xFF;
	IOREG[0x49] = 0xFF;
	IOREG[0x4A] = 0x00;
	IOREG[0x4B] = 0x00;
	IOREG[0x50] = 0xFF;
	if (isInCGBMode())
	{
		IOREG[0x46] = 0x00;
		IOREG[0x4C] = 0x00;
		IOREG[0x4D] = 0x7E;
		IOREG[0x4F] = 0xFE;
		IOREG[0x51] = 0xFF;
		IOREG[0x52] = 0xFF;
		IOREG[0x53] = 0xFF;
		IOREG[0x54] = 0xFF;
		IOREG[0x55] = 0xFF;
		IOREG[0x56] = 0x3F;
		IOREG[0x6C] = 0x00;
		IOREG[0x70] = 0xF8;
	}
	else
	{
		IOREG[0x46] = 0xFF;
		IOREG[0x4C] = 0x04; 
		IOREG[0x6C] = 0x01;
	}
}

void MemoryManager::cycle_GPDMA()
{
	if (((IOREG[0x4F] & 0b00000001) == 1))
	{
		VRAM1[DMA_dest_address + DMA_offset] = getMemValue(DMA_source_address + DMA_offset);
		VRAM1[DMA_dest_address + DMA_offset + 1] = getMemValue(DMA_source_address + DMA_offset + 1);
	}
	else
	{
		VRAM0[DMA_dest_address + DMA_offset] = getMemValue(DMA_source_address + DMA_offset);
		VRAM0[DMA_dest_address + DMA_offset + 1] = getMemValue(DMA_source_address + DMA_offset + 1);
	}
	DMA_offset += 2;
	if ((DMA_dest_address + DMA_offset) >= 0x2000)
	{
		DMA_in_progress = false;
		general_purpose_DMA = false;
		IOREG[0x55] = 0xFF;
	}
	if ((DMA_offset & 0b00001111) == 0)
	{
		IOREG[0x55] = IOREG[0x55] - 1;
	}
	if (IOREG[0x55] == 0xFF)
	{
		DMA_in_progress = false;
		DMA_offset = 0;
	}
}

void MemoryManager::cycleHDMA()
{
	if (!cpu_halted and (DMA_in_progress and !general_purpose_DMA))
	{
		for (int i = 0; i < 0x10; i++)
		{
			if (((IOREG[0x4F] & 0b00000001) == 1))
			{
				VRAM1[DMA_dest_address + DMA_offset] = getMemValue(DMA_source_address + DMA_offset);
			}
			else
			{
				VRAM0[DMA_dest_address + DMA_offset] = getMemValue(DMA_source_address + DMA_offset);
			}
			DMA_offset++;
		}
		if ((DMA_dest_address + DMA_offset) >= 0x2000)
		{
			DMA_in_progress = false;
			DMA_offset = 0;
			IOREG[0x55] = 0xFF;
			return;
		}
		IOREG[0x55] = IOREG[0x55] - 1;
		if (IOREG[0x55] == 0xFF)
		{
			DMA_in_progress = false;
			DMA_offset = 0;
		}
	}
}

uint8_t MemoryManager::getMemValue(uint16_t address) const
{
	if (address < 0x0100 and boot_ROM_enabled)
	{
		return bootROM[address];
	}

	//if (address == 0xFF44) return 0x90; //REMEMBER TO REMOVE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	if (address < 0x8000) {
		return gameCart->getROMValue(address);
	}
	address -= 0x8000;
	if (address < 0x2000) {
		if (display_mode == 3)
		{
			return 0xFF;
		}
		if (isInCGBMode() and ((IOREG[0x4F] & 0b00000001) == 1))
		{
			return VRAM1[address];
		}
		return VRAM0[address];
	}
	address -= 0x2000;
	if (address < 0x2000) {
		return gameCart->getRAMValue(address);
	}
	address -= 0x2000;
	if (address < 0x1000) {
		return WorkRAM0[address];
	}
	address -= 0x1000;
	if (address < 0x1000) {
		if (isInCGBMode())
		{
			switch (IOREG[0x70] & 0b00000111)
			{
			case 0:
			case 1:
				return WorkRAM1[address];
			case 2:
				return WorkRAM2[address];
			case 3:
				return WorkRAM3[address];
			case 4:
				return WorkRAM4[address];
			case 5:
				return WorkRAM5[address];
			case 6:
				return WorkRAM6[address];
			case 7:
				return WorkRAM7[address];
			}
		}
		return WorkRAM1[address];
	}
	address -= 0x1000;
	if (address < 0x1000) {
		return WorkRAM0[address];
	}
	address -= 0x1000;
	if (address < 0x0E00) {
		if (isInCGBMode())
		{
			switch (IOREG[0x70] & 0b00000111)
			{
			case 0:
			case 1:
				return WorkRAM1[address];
			case 2:
				return WorkRAM2[address];
			case 3:
				return WorkRAM3[address];
			case 4:
				return WorkRAM4[address];
			case 5:
				return WorkRAM5[address];
			case 6:
				return WorkRAM6[address];
			case 7:
				return WorkRAM7[address];
			}
		}
		return WorkRAM1[address];
	}
	address -= 0x0E00;
	if (address < 0x00A0) {
		if(OAM_DMA_in_progress)
			return 0xFF;
		return OAM[address];
	}
	address -= 0x00A0;
	if (address < 0x0060) {
		return NU[address];
	}
	address -= 0x0060;
	if (address < 0x0080) {
		switch (address)
		{
		case 0x00:
			return IOREG[0x00] | 0xB0;
		case 0x10:
			return IOREG[0x10] | 0x80;
		case 0x11:
			return IOREG[0x11] | 0x3F;
		case 0x13:
			return 0xFF;
		case 0x14:
			return IOREG[0x14] | 0xBF;
		case 0x16:
			return IOREG[0x16] | 0x3F;
		case 0x18:
			return 0xFF;
		case 0x19:
			return IOREG[0x19] | 0xBF;
		case 0x1A:
			return IOREG[0x1A] | 0x7F;
		case 0x1B:
			return 0xFF;
		case 0x1C:
			return IOREG[0x1C] | 0x9F;
		case 0x1D:
			return 0xFF;
		case 0x1E:
			return IOREG[0x1E] | 0xBF;
		case 0x20:
			return 0xFF;
		case 0x23:
			return IOREG[0x1E] | 0xBF;
		case 0x26:			
			if ((IOREG[0x26] & 0b10000000) == 0)
			{
				return 0b01110000;
			}
			return (IOREG[0x26] & 0b10000000) + 0b01110000 + ch4->getEnabled() * 0b1000 + ch3->getEnabled() * 0b0100 + ch2->getEnabled() * 0b0010 + ch1->getEnabled() * 0b0001;
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:
		case 0x39:
		case 0x3A:
		case 0x3B:
		case 0x3C:
		case 0x3D:
		case 0x3E:
		case 0x3F:
			if (ch3->getEnabled())
			{
				return 0xFF;
			}
			else
			{
				return IOREG[address];
			}
		case 0x41:
			return (IOREG[0x41] & 0b01111111) + 0b10000000;
		case 0x4D:
			if (isInCGBMode())
			{
				return IOREG[0x4D];
			}
			return 0xFF;
		case 0x4F:
			if (isInCGBMode())
			{
				return 0b11111110 + (IOREG[0x4F] & 0b00000001);
			}
			return 0xFF;
		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
		case 0x54:
		case 0x55:
			return 0xFF;
		case 0x68:
			if (isInCGBMode())
			{
				return IOREG[0x68];
			}
			return 0xFF;
		case 0x69:
			if (isInCGBMode())
			{
				return BCRAM[IOREG[0x68] & 0b00111111];
			}
			return 0xFF;
		case 0x6A:
			if (isInCGBMode())
			{
				return IOREG[0x6A];
			}
			return 0xFF;
		case 0x6B:
			if (isInCGBMode())
			{
				return OCRAM[IOREG[0x6A] & 0b00111111];
			}
			return 0xFF;
		case 0x6C:
		case 0x6D:
		case 0x6E:
		case 0x6F:
			return 0xFF;
		case 0x70:
			if (isInCGBMode())
			{
				return IOREG[0x70];
			}
			return 0xFF;
		case 0x76:
			if (isInCGBMode())
			{
				return ch1->getDigitalOutput() + (ch2->getDigitalOutput() << 4);
			}
			return 0xFF;
		case 0x77:
			if (isInCGBMode())
			{
				return ch3->getDigitalOutput() + (ch4->getDigitalOutput() << 4);
			}
			return 0xFF;
		case 0x03:case 0x08:case 0x09:case 0x0A:case 0x0B:case 0x0C:case 0x0D:case 0x0E:case 0x15:case 0x1F:
		case 0x27:case 0x28:case 0x29:case 0x2A:case 0x2B:case 0x2C:case 0x2D:case 0x2E:case 0x2F:case 0x4C:
		case 0x4E:case 0x56:case 0x57:case 0x58:case 0x59:case 0x5A:case 0x5B:case 0x5C:case 0x5D:case 0x5E:
		case 0x5F:case 0x60:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:case 0x66:case 0x67:case 0x71:
		case 0x72:case 0x73:case 0x74:case 0x75:case 0x78:case 0x79:case 0x7A:case 0x7B:
		case 0x7C:case 0x7D:case 0x7E:case 0x7F:
			return 0xFF;
		default:
			return IOREG[address];
		}
	}
	address -= 0x0080;
	if (address < 0x7F) {
		return HRAM[address];
	}
	return IE;
}

void MemoryManager::setMemValue(uint16_t address, uint8_t val)
{
	if (address < 0x8000) {
		gameCart->setROMValue(address, val);
		return;
	}
	address -= 0x8000;
	if (address < 0x2000) {
		if (display_mode == 3)
		{
			return;
		}
		if (isInCGBMode() and ((IOREG[0x4F] & 0b00000001) == 1))
		{
			VRAM1[address] = val;
			return;
		}
		VRAM0[address] = val;
		return;
	}
	address -= 0x2000;
	if (address < 0x2000) {
		gameCart->setRAMValue(address, val);
		return;
	}
	address -= 0x2000;
	if (address < 0x1000) {
		WorkRAM0[address] = val;
		return;
	}
	address -= 0x1000;
	if (address < 0x1000) {
		if (isInCGBMode())
		{
			switch (IOREG[0x70] & 0b00000111)
			{
			case 0:
			case 1:
				WorkRAM1[address] = val;
				return;
			case 2:
				WorkRAM2[address] = val;
				return;
			case 3:
				WorkRAM3[address] = val;
				return;
			case 4:
				WorkRAM4[address] = val;
				return;
			case 5:
				WorkRAM5[address] = val;
				return;
			case 6:
				WorkRAM6[address] = val;
				return;
			case 7:
				WorkRAM7[address] = val;
				return;
			}
		}
		WorkRAM1[address] = val;
		return;
	}
	address -= 0x1000;
	if (address < 0x1000) {
		WorkRAM0[address] = val;
		return;
	}
	address -= 0x1000;
	if (address < 0x0E00) {
		if (isInCGBMode())
		{
			switch (IOREG[0x70] & 0b00000111)
			{
			case 0:
			case 1:
				WorkRAM1[address] = val;
				return;
			case 2:
				WorkRAM2[address] = val;
				return;
			case 3:
				WorkRAM3[address] = val;
				return;
			case 4:
				WorkRAM4[address] = val;
				return;
			case 5:
				WorkRAM5[address] = val;
				return;
			case 6:
				WorkRAM6[address] = val;
				return;
			case 7:
				WorkRAM7[address] = val;
				return;
			}
		}
		WorkRAM1[address] = val;
		return;
	}
	address -= 0x0E00;
	if (address < 0x00A0) {
		if (OAM_DMA_in_progress or display_mode == 2 or display_mode == 3)
			return;
		OAM[address] = val;
		return;
	}
	address -= 0x00A0;
	if (address < 0x0060) {
		NU[address] = val;
		return;
	}
	address -= 0x0060;
	if (address < 0x0080) {
		switch (address)
		{
		case 0x00:
			IOREG[0x00] = (IOREG[0x00] & 0b11001111) + (val & 0b00110000);
			break;
		case 0x01:
			IOREG[0x01] = val;
			//std::cout << val;
			break;
		case 0x02:
			if (isInCGBMode())
			{
				IOREG[0x02] = (val & 0b10000011) + 0b01111100;
			}
			else
			{
				IOREG[0x02] = (val & 0b10000001) + 0b01111110;
			}
			break;
		case 0x04:
			IOREG[0x04] = 0;
			div_reset = true;
			break;
		case 0x07:
			IOREG[0x07] = (val & 0b00000111) + 0b11111000;
			break;
		case 0x0F:
			IOREG[0x0F] = (val & 0b00011111) + 0b11100000;
			break;
		case 0x10:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x10] = (val & 0b01111111) + 0b10000000;
			ch1->setSweepValues((IOREG[0x10] & 0b00001000) != 0, IOREG[0x10] & 0b00000111, (IOREG[0x10] & 0b01110000) >> 4);
			if ((IOREG[0x10] & 0b01110000) == 0)
			{
				ch1->setSweepEnabled(false);
			}
			else
			{
				ch1->setSweepEnabled(true);
			}
			break;
		case 0x11:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x11] = val;
			ch1->setLenghtTimer(IOREG[0x11] & 0b00111111);
			ch1->setDutyCycle(IOREG[0x11] >> 6);
			break;
		case 0x12:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x12] = val;
			if ((IOREG[0x12] & 0b11111000) == 0)
			{
				ch1->setDACEnabled(false);
			}
			else
			{
				ch1->setDACEnabled(true);
			}
			ch1->envelopeDirWrite((IOREG[0x12] & 0b00001000) != 0);
			break;
		case 0x13:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x13] = val;
			ch1->setPeriod((((uint16_t)(IOREG[0x14] & 0b00000111)) << 8) + IOREG[0x13]);
			break;
		case 0x14:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x14] = val;
			ch1->setLenghtEnabled((IOREG[0x14] & 0b01000000) != 0);
			if ((val & 0b10000000) != 0)
			{
				ch1->trigger(IOREG[0x10], IOREG[0x11], IOREG[0x12], IOREG[0x13], IOREG[0x14], audio_length_first_half);
				ch1->getEnabled();
			}
			else
			{
				ch1->setPeriod((((uint16_t)(IOREG[0x14] & 0b00000111)) << 8) + IOREG[0x13]);
			}
			break;
		case 0x16:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x16] = val;
			ch2->setLenghtTimer(IOREG[0x16] & 0b00111111);
			ch2->setDutyCycle(IOREG[0x16] >> 6);
			break;
		case 0x17:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x17] = val;
			if ((IOREG[0x17] & 0b11111000) == 0)
			{
				ch2->setDACEnabled(false);
			}
			else
			{
				ch2->setDACEnabled(true);
			}
			ch2->envelopeDirWrite((IOREG[0x17] & 0b00001000) != 0);
			break;
		case 0x18:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x18] = val;
			ch2->setPeriod((((uint16_t)(IOREG[0x19] & 0b00000111)) << 8) + IOREG[0x18]);
			break;
		case 0x19:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x19] = val;
			ch2->setLenghtEnabled((IOREG[0x19] & 0b01000000) != 0);
			if ((val & 0b10000000) != 0)
			{
				ch2->trigger(IOREG[0x16], IOREG[0x17], IOREG[0x18], IOREG[0x19], audio_length_first_half);
			}
			else
			{
				ch2->setPeriod((((uint16_t)(IOREG[0x19] & 0b00000111)) << 8) + IOREG[0x18]);
			}
			break;
		case 0x1A:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x1A] = val;
			if ((IOREG[0x1A] & 0b10000000) == 0)
			{
				ch3->setDACEnabled(false);
			}
			else
			{
				ch3->setDACEnabled(true);
			}
			//ch3 DAC off
			break;
		case 0x1B:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x1B] = val;
			ch3->setLenghtTimer(val);
			break;
		case 0x1C:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x1C] = (val & 0b01100000) + 0b10011111;
			ch3->setVolume((val & 0b01100000) >> 5);
			break;
		case 0x1D:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x1D] = val;
			ch3->setPeriod((((uint16_t)(IOREG[0x1E] & 0b00000111)) << 8) + IOREG[0x1D]);
			break;
		case 0x1E:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x1E] = val;
			ch3->setLenghtEnabled((IOREG[0x1E] & 0b01000000) != 0);
			if ((val & 0b10000000) != 0)
			{
				ch3->trigger(IOREG[0x1A], IOREG[0x1B], IOREG[0x1C], IOREG[0x1D], IOREG[0x1E], audio_length_first_half);
			}
			else
			{
				ch3->setPeriod((((uint16_t)(IOREG[0x1E] & 0b00000111)) << 8) + IOREG[0x1D]);
			}
			break;
		case 0x20:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x20] = (val & 0b00111111) + 0b11000000;
			ch4->setLenghtTimer(IOREG[0x20] & 0b00111111);
			break;
		case 0x21:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x21] = val;
			if ((IOREG[0x21] & 0b11111000) == 0)
			{
				ch4->setDACEnabled(false);
			}
			else
			{
				ch4->setDACEnabled(true);
			}
			ch4->envelopeDirWrite((IOREG[0x21] & 0b00001000) != 0);
			break;
		case 0x22:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x22] = val;
			ch4->setShort(((IOREG[0x22] >> 3) & 0b00000001) == 1);
			ch4->setDivider(IOREG[0x22] & 0b00000111);
			ch4->setShift((IOREG[0x22] >> 4) & 0b00001111);
			break;
		case 0x23:
			if ((IOREG[0x26] & 0b10000000) == 0) break;
			IOREG[0x23] = (val & 0b11000000) + 0b00111111;
			ch4->setLenghtEnabled((IOREG[0x23] & 0b01000000) != 0);
			if ((val & 0b10000000) != 0)
			{
				ch4->trigger(IOREG[0x20], IOREG[0x21], IOREG[0x22], IOREG[0x23], audio_length_first_half);
			}
			break;
		case 0x26:
			IOREG[0x26] = val;
			break;
		//wave RAM adresses
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:
		case 0x39:
		case 0x3A:
		case 0x3B:
		case 0x3C:
		case 0x3D:
		case 0x3E:
		case 0x3F:
			if (!ch3->getEnabled())
			{
				IOREG[address] = val;
				ch3->setWaveRAMValue(address - 0x30, val);
			}
			break;
		case 0x41:
			IOREG[address] = (val & 0b01111111) + 0b10000000;
			break;
		case 0x46:
			IOREG[0x46] = val;
			if (val <= 0xDF)
			{
				start_OAM_DMA();
			}
			break;
		case 0x4C:
			break;
		case 0x4D:
			IOREG[0x4D] = (IOREG[0x4D] & 0b10000000) + (val & 0b00000001);
			break;
		case 0x4F:
			IOREG[0x4F] = val;
			break;
		case 0x50:
			boot_ROM_enabled = false;
			break;
		case 0x51:
			IOREG[0x51] = val;
			break;
		case 0x52:
			IOREG[0x52] = val & 0b11110000;
			break;
		case 0x53:
			IOREG[0x53] = val & 0b00011111;
			break;
		case 0x54:
			IOREG[0x54] = val & 0b11110000;
			break;
		case 0x55:
			if (DMA_in_progress)
			{
				if ((val & 0b10000000) == 0)
				{
					DMA_in_progress = false;
				}
			}
			else
			{
				DMA_in_progress = true;
				IOREG[0x55] = val & 0b01111111;
				DMA_source_address = (IOREG[0x51] << 8) + IOREG[0x52];
				DMA_dest_address = (IOREG[0x53] << 8) + IOREG[0x54];
				DMA_offset = 0;
				if ((val & 0b10000000) == 0)
				{
					general_purpose_DMA = true;
				}
				else
				{
					general_purpose_DMA = false;
				}
			}
			break;
		case 0x68:
			IOREG[0x68] = (val & 0b10111111);
			break;
		case 0x69:
			if (display_mode != 3)
			{
				BCRAM[(IOREG[0x68] & 0b00111111)] = val;
			}
			if (IOREG[(0x68 & 0b10000000)] != 0)
			{
				IOREG[0x68] = ((IOREG[0x68] + 1) & 0b10111111);
			}
			break;
		case 0x6A:
			IOREG[0x6A] = (val & 0b10111111);
			break;
		case 0x6B:
			if (display_mode != 3)
			{
				OCRAM[(IOREG[0x6A] & 0b00111111)] = val;
			}
			if (IOREG[(0x6A & 0b10000000)] != 0)
			{
				IOREG[0x6A] = ((IOREG[0x6A] + 1) & 0b00111111);
			}
			break;
		case 0x6C:
			break;
		default:
			IOREG[address] = val;
		}
		return;
	}
	address -= 0x0080;
	if (address < 0x7F) {
		HRAM[address] = val;
		return;
	}
	IE = val;
}

uint8_t MemoryManager::getIOREGValue(uint16_t address) const
{
	return IOREG[address - 0xFF00];
}

void MemoryManager::setIOREGValue(uint16_t address, uint8_t val)
{
	IOREG[address - 0xFF00] = val;
}

uint8_t MemoryManager::getVRAMValue(uint16_t address, bool bank1) const
{
	if (bank1)
	{
		return VRAM1[address - 0x8000];
	}
	return VRAM0[address - 0x8000];
}

void MemoryManager::setVRAMValue(uint16_t address, uint8_t val, bool bank1)
{
	if (bank1)
	{
		VRAM1[address - 0x8000] = val;
		return;
	}
	VRAM0[address - 0x8000] = val;
}

uint8_t MemoryManager::getBCRAMValue(uint8_t address) const
{
	return BCRAM[address];
}

uint8_t MemoryManager::getOCRAMValue(uint8_t address) const
{
	return OCRAM[address];
}

void MemoryManager::setCPUHalted(bool val)
{
	cpu_halted = val;
}

void MemoryManager::setAudioLengthFirstHalf(bool val)
{
	audio_length_first_half = val;
}

void MemoryManager::setDisplayMode(uint8_t val)
{
	display_mode = val;
}

void MemoryManager::requestInterrupt(Interrupt inter)
{
	IOREG[0x0F] = IOREG[0x0F] | (0x01 << (uint8_t)inter);
	// printf("interrupt requested: %X\n", IOREG[0x0F]);
}

void MemoryManager::discardInterrupt(Interrupt inter)
{
	IOREG[0x0F] = IOREG[0x0F] & ~(0x01 << (uint8_t)inter);
}

void MemoryManager::performSpeedSwitch()
{
	if (isInDoubleSpeedMode())
	{
		IOREG[0x4D] = 0b00000000;
	}
	else
	{
		IOREG[0x4D] = 0b10000000;
	}
}

void MemoryManager::cycle(bool ignore_GPDMA)
{
	gameCart->tickRTC();
	if (OAM_DMA_in_progress and !(DMA_in_progress and general_purpose_DMA))
	{
		uint16_t DMA_current_address = OAM_DMA_address + OAM_DMA_progress;
		uint8_t current_val = 0x00;
		if (DMA_current_address < 0x8000) {
			current_val = gameCart->getROMValue(DMA_current_address);
		}
		DMA_current_address -= 0x8000;
		if (DMA_current_address < 0x2000) {
			if (isInCGBMode() and ((IOREG[0x4F] & 0b00000001) == 1))
			{
				current_val = VRAM1[DMA_current_address];
			}
			current_val = VRAM0[DMA_current_address];
		}
		DMA_current_address -= 0x2000;
		if (DMA_current_address < 0x2000) {
			current_val = gameCart->getRAMValue(DMA_current_address);
		}
		DMA_current_address -= 0x2000;
		if (DMA_current_address < 0x1000) {
			current_val = WorkRAM0[DMA_current_address];
		}
		DMA_current_address -= 0x1000;
		if (DMA_current_address < 0x1000) {
			if (isInCGBMode())
			{
				switch (IOREG[0x70] & 0b00000111)
				{
				case 0:
				case 1:
					current_val = WorkRAM1[DMA_current_address];
					break;
				case 2:
					current_val = WorkRAM2[DMA_current_address];
					break;
				case 3:
					current_val = WorkRAM3[DMA_current_address];
					break;
				case 4:
					current_val = WorkRAM4[DMA_current_address];
					break;
				case 5:
					current_val = WorkRAM5[DMA_current_address];
					break;
				case 6:
					current_val = WorkRAM6[DMA_current_address];
					break;
				case 7:
					current_val = WorkRAM7[DMA_current_address];
					break;
				}
			}
			else
			{
				current_val = WorkRAM1[DMA_current_address];
			}
		}
		OAM[OAM_DMA_progress] = current_val;
		OAM_DMA_progress++;
		if (OAM_DMA_progress == 160)
		{
			OAM_DMA_in_progress = false;
		}
	}
	if (DMA_in_progress and general_purpose_DMA and !ignore_GPDMA)
	{
		cycle_GPDMA();
	}
}

void MemoryManager::start_OAM_DMA()
{
	OAM_DMA_in_progress = true;
	OAM_DMA_progress = 0;
	OAM_DMA_address = IOREG[0x46] << 8;
}

bool MemoryManager::bootROMAvailable() const
{
	return bootROM.size() != 0;
}

bool MemoryManager::isInCGBMode() const
{
	if (gameCart == nullptr or force_dmg)
	{
		return false;
	}
	return gameCart->supportsCGB();
}

bool MemoryManager::isInDoubleSpeedMode() const
{
	return (IOREG[0x4D] & 0b10000000) != 0;
}

bool MemoryManager::isPerformingGPDMA() const
{
	return DMA_in_progress && general_purpose_DMA;
}

bool MemoryManager::wasDIVResetPerformed()
{
	if (div_reset)
	{
		div_reset = false;
		return true;
	}
	return false;
}