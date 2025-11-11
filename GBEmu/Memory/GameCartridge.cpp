#include "GameCartridge.h"

bool GameCartridge::supportsCGB()
{
	if (GameROM.size() < 0x143) return false;
	return GameROM[0x143] == 0x80 or GameROM[0x143] == 0xC0;
}

MBC0::MBC0(std::vector<uint8_t> ROM, bool RAM, std::shared_ptr<SaveManager> save_manager, std::string path)
{
	GameROM = ROM;
	GamePath = path;
	GameROM = ROM;
	saveManager = save_manager;
	if (RAM)
	{
		GameRAM = std::vector<uint8_t>(8192);
		has_RAM = true;
	}
	else
	{
		GameRAM = std::vector<uint8_t>(0);
		has_RAM = false;
	}
}

uint8_t MBC0::getROMValue(uint16_t address)
{
	return GameROM[address];
}

void MBC0::setROMValue(uint16_t address, uint8_t value)
{
	return;
}

uint8_t MBC0::getRAMValue(uint16_t address)
{
	if (has_RAM)
	{
		return GameRAM[address];
	}
	else
	{
		return 0xFF;
	}
}

void MBC0::setRAMValue(uint16_t address, uint8_t value)
{
	if (has_RAM)
	{
		GameRAM[address] = value;
		if (saveManager != nullptr and GamePath != "")
		{
			saveManager->WriteToSave(address, value);
		}
	}
}

MBC1::MBC1(std::vector<uint8_t> ROM, bool RAM, std::shared_ptr<SaveManager> save_manager, std::string path)
{
	GamePath = path;
	GameROM = ROM;
	saveManager = save_manager;
	if (RAM)
	{
		has_RAM = true;
		switch (GameROM[0x149]) {
		case 0x00:
			has_RAM = false;
			GameRAM = std::vector<uint8_t>(0);
			break;
		case 0x02:
			GameRAM = std::vector<uint8_t>(8192);
			break;
		case 0x03:
			GameRAM = std::vector<uint8_t>(32768);
			break;
		case 0x04:
			GameRAM = std::vector<uint8_t>(131072);
			break;
		case 0x05:
			GameRAM = std::vector<uint8_t>(65536);
			break;
		}
		if (saveManager != nullptr and GamePath != "")
		{
			std::vector<uint8_t> temp = saveManager->OpenOrCreateSave(path);
			for (int i = 0; i < temp.size(); i++)
			{
				GameRAM[i] = temp[i];
			}
		}
	}
	else
	{
		has_RAM = false;
		GameRAM = std::vector<uint8_t>(0);
	}
	ROM_bank_amount = GameROM.size() / 0x4000;
	RAM_enabled = false;
	ROM_bank = 0x00;
	RAM_bank = 0x00;
	banking_mode_1 = false;
}

uint8_t MBC1::getROMValue(uint16_t address)
{
	if (!banking_mode_1)
	{
		if (address < 0x4000)
		{
			return GameROM[address];
		}
		else if (address < 0x8000)
		{
			address -= 0x4000;
			uint8_t bank = (RAM_bank * 32 + ROM_bank) % ROM_bank_amount;
			//printf("ROM %d, RAM %d\n", ROM_bank, RAM_bank);
			if (ROM_bank == 0)
			{
				bank++;
			}
			return GameROM[(address + (0x4000 * bank))];
		}
		else return 0xFF;
	}
	else
	{
		if (address < 0x4000)
		{
			uint8_t bank = (RAM_bank * 32) % ROM_bank_amount;
			return GameROM[address + (0x4000 * bank)];
		}
		else if (address < 0x8000)
		{
			address -= 0x4000;
			uint8_t bank = (RAM_bank * 32 + ROM_bank) % ROM_bank_amount;
			if (ROM_bank == 0)
			{
				bank++;
			}
			return GameROM[(address + (0x4000 * bank))];
		}
	}
}

uint8_t MBC1::getRAMValue(uint16_t address)
{
	if (has_RAM and RAM_enabled)
	{
		if (GameRAM.size() == 32768)
		{
			return GameRAM[address + (0x2000 * (RAM_bank & 0b00000011))];
		}
		else
		{
			return GameRAM[address];
		}
	}
	else 
	{
		return 0xFF;
	}
}

void MBC1::setROMValue(uint16_t address, uint8_t value)
{
	if (address < 0x2000)
	{
		if (has_RAM) RAM_enabled = ((value & 0x0F) == 0x0A);
	}
	else if (address < 0x4000)
	{
		ROM_bank = value & 0b00011111;
	}
	else if (address < 0x6000)
	{
		RAM_bank = ((value << 6) >> 6);
	}
	else if (address < 0x8000)
	{
		banking_mode_1 = (((value << 7) >> 7) == 0x1);
	}
	return;
}

void MBC1::setRAMValue(uint16_t address, uint8_t value)
{
	if (RAM_enabled)
	{
		if (GameRAM.size() == 32768)
		{
			GameRAM[address + (0x2000 * (RAM_bank & 0b00000011))] = value;
			if (saveManager != nullptr and GamePath != "")
			{
				saveManager->WriteToSave(address + (0x2000 * (RAM_bank & 0b00000011)), value);
			}
		}
		else
		{
			GameRAM[address] = value;
			if (saveManager != nullptr and GamePath != "")
			{
				saveManager->WriteToSave(address, value);
			}
		}
	}
}

MBC2::MBC2(std::vector<uint8_t> ROM, std::shared_ptr<SaveManager> save_manager, std::string path)
{
	GamePath = path;
	GameROM = ROM;
	saveManager = save_manager;
	GameRAM = std::vector<uint8_t>(512, 0xFF);
	if (saveManager != nullptr and GamePath != "")
	{
		std::vector<uint8_t> temp = saveManager->OpenOrCreateSave(path);
		for (int i = 0; i < temp.size(); i++)
		{
			GameRAM[i] = temp[i];
		}
	}
	ROM_bank_amount = GameROM.size() / 0x4000;
	RAM_enabled = false;
	ROM_bank = 0x01;
}

uint8_t MBC2::getROMValue(uint16_t address)
{
	if (address < 0x4000)
	{
		return GameROM[address];
	}
	else if (address < 0x8000)
	{
		address -= 0x4000;
		uint8_t bank = ROM_bank % ROM_bank_amount;
		if (ROM_bank == 0)
		{
			bank++;
		}
		return GameROM[(address + (0x4000 * bank))];
	}
}

uint8_t MBC2::getRAMValue(uint16_t address)
{
	if (RAM_enabled)
	{
		return GameRAM[address & 0b0000000111111111] + 0xF0;
	}
	else
	{
		return 0xFF;
	}
}

void MBC2::setROMValue(uint16_t address, uint8_t value)
{
	if (address < 0x4000)
	{
		if ((address & 0b100000000) != 0)
		{
			ROM_bank = value & 0b00001111;
		}
		else
		{
			RAM_enabled = ((value & 0x0F) == 0x0A);
		}
		
	}
	return;
}

void MBC2::setRAMValue(uint16_t address, uint8_t value)
{
	if (RAM_enabled)
	{
		GameRAM[address & 0b0000000111111111] = (value & 0b00001111);
		if (saveManager != nullptr and GamePath != "")
		{
			saveManager->WriteToSave(address & 0b0000000111111111, value & 0b00001111);
		}
	}
}


MBC3::MBC3(std::vector<uint8_t> ROM, bool RAM, bool clock, std::shared_ptr<SaveManager> save_manager, std::string path)
{
	GamePath = path;
	GameROM = ROM;
	saveManager = save_manager;
	has_clock = clock;
	seconds = 0;
	minutes = 0;
	hours = 0;
	days = 0;
	days_overflow = false;
	ticks = 0;
	rtc_halt = false;
	rtc_latched = false;
	latched_time = std::vector<uint8_t>(5);
	has_RAM = RAM;
	last_time = SDL_GetTicksNS();
	if (has_RAM or has_clock)
	{
		if (has_RAM)
		{
			switch (GameROM[0x149]) {
			case 0x00:
				has_RAM = false;
				GameRAM = std::vector<uint8_t>(0);
				break;
			case 0x02:
				GameRAM = std::vector<uint8_t>(8192);
				break;
			case 0x03:
				GameRAM = std::vector<uint8_t>(32768);
				break;
			case 0x04:
				GameRAM = std::vector<uint8_t>(131072);
				break;
			case 0x05:
				GameRAM = std::vector<uint8_t>(65536);
				break;
			}
		}
		
		if (saveManager != nullptr and GamePath != "")
		{
			std::vector<uint8_t> temp = saveManager->OpenOrCreateSave(path);
			for (int i = 0; i + MBC3_SAVE_OFFSET < temp.size(); i++)
			{
				GameRAM[i] = temp[i + MBC3_SAVE_OFFSET];
			}
			if (temp.size() >= 15 and has_clock)
			{
				seconds = temp[0x00];
				minutes = temp[0x01];
				hours = temp[0x02];
				days = temp[0x03] + ((temp[0x04] & 0b00000001) << 8);
				days_overflow = (temp[0x04] & 0b10000000) != 0;
				time_t now = time(NULL);
				time_t then = (uint64_t)temp[0x0008]
					+ ((uint64_t)temp[0x0009] << 8)
					+ ((uint64_t)temp[0x000A] << 16)
					+ ((uint64_t)temp[0x000B] << 24)
					+ ((uint64_t)temp[0x000C] << 32)
					+ ((uint64_t)temp[0x000D] << 40)
					+ ((uint64_t)temp[0x000E] << 48)
					+ ((uint64_t)temp[0x000F] << 56);
				if (now > then)
				{
					uint64_t diff = now - then;
					seconds += diff % 60;
					diff /= 60;
					minutes += diff % 60;
					diff /= 60;
					hours += diff % 24;
					diff /= 24;
					days += diff;
					while (seconds >= 60)
					{
						seconds -= 60;
						minutes++;
					}
					while (minutes >= 60)
					{
						minutes -= 60;
						hours++;
					}
					while (hours >= 24)
					{
						hours -= 24;
						days++;
					}
					if (days >= 512)
					{
						days = days % 512;
						days_overflow = true;
					}
				}
			}
		}
	}
	else
	{
		has_RAM = false;
		GameRAM = std::vector<uint8_t>(0);
	}
	ROM_bank_amount = GameROM.size() / 0x4000;
	RAM_enabled = false;
	ROM_bank = 0x01;
	RAM_bank = 0x00;
}

uint8_t MBC3::getROMValue(uint16_t address)
{
	if (address < 0x4000)
	{
		return GameROM[address];
	}
	else if (address < 0x8000)
	{
		if (ROM_bank == 0)
		{
			return GameROM[address];
		}
		address -= 0x4000;

		return GameROM[(address + 0x4000 * (ROM_bank % ROM_bank_amount))];
	}
}

uint8_t MBC3::getRAMValue(uint16_t address)
{
	if (RAM_enabled and RAM_bank < 8)
	{
		return GameRAM[address + (0x2000 * RAM_bank)];
	}
	else if (RAM_enabled)
	{
		//printf("RTC not implemented\n");
		switch (RAM_bank)
		{
		case 0x08:
			return latched_time[0];
		case 0x09:
			return latched_time[1];
		case 0x0A:
			return latched_time[2];
		case 0x0B:
			return latched_time[3];
		case 0x0C:
			return latched_time[4];
		}
	}
	return 0xFF;
}

void MBC3::setROMValue(uint16_t address, uint8_t value)
{
	if (address < 0x2000)
	{
		if (has_RAM or has_clock) RAM_enabled = ((value & 0x0F) == 0x0A);
	}
	else if (address < 0x4000)
	{
		ROM_bank = value;
	}
	else if (address < 0x6000)
	{
		RAM_bank = value & 0b00001111;
	}
	else if (address < 0x8000)
	{
		if (value == 0x01 and !rtc_latched)
		{
			//latch clock
			latched_time[0] = seconds;
			latched_time[1] = minutes;
			latched_time[2] = hours;
			latched_time[3] = days & 0b11111111;
			latched_time[4] = (days > 8) + (64 * rtc_halt) + (128 * days_overflow);
			rtc_latched = true;
		}
		else if (value == 0x00 and rtc_latched)
		{
			rtc_latched = false;
		}
	}
	return;
}

void MBC3::tickRTC()
{
	if (has_clock)
	{
		if (last_time < (SDL_GetTicksNS() - 1000000000))
		{
			last_time += 1000000000;
			if (!rtc_halt)
			{
				seconds += 1;
				if (seconds == 60)
				{
					seconds = 0;
					minutes += 1;
					if (minutes == 60)
					{
						minutes = 0;
						hours += 1;
						if (hours == 24)
						{
							hours = 0;
							days += 1;
							if (days == 512)
							{
								days = 0;
								days_overflow = true;
							}
						}
					}
				}
			}
			saveManager->WriteToSave(0x0000, seconds);
			saveManager->WriteToSave(0x0001, minutes);
			saveManager->WriteToSave(0x0002, hours);
			saveManager->WriteToSave(0x0003, days & 0b11111111);
			saveManager->WriteToSave(0x0004, (days > 8) + (64 * rtc_halt) + (128 * days_overflow));
			time_t now = time(NULL);
			saveManager->WriteToSave(0x0008, now & 0b11111111);
			saveManager->WriteToSave(0x0009, (now >> 8) & 0b11111111);
			saveManager->WriteToSave(0x000A, (now >> 16) & 0b11111111);
			saveManager->WriteToSave(0x000B, (now >> 24) & 0b11111111);
			saveManager->WriteToSave(0x000C, (now >> 32) & 0b11111111);
			saveManager->WriteToSave(0x000D, (now >> 40) & 0b11111111);
			saveManager->WriteToSave(0x000E, (now >> 48) & 0b11111111);
			saveManager->WriteToSave(0x000F, (now >> 56) & 0b11111111);
			//write timestamp to file
		}
	}
	else
	{
		last_time = SDL_GetTicksNS();
	}
}

void MBC3::setRAMValue(uint16_t address, uint8_t value)
{
	if (RAM_enabled and RAM_bank < 8)
	{
		GameRAM[address + (0x2000 * (RAM_bank))] = value;
		if (saveManager != nullptr and GamePath != "")
		{
			saveManager->WriteToSave(address + (0x2000 * (RAM_bank)) + MBC3_SAVE_OFFSET, value);
		}
	}
	else if (RAM_enabled)
	{
		switch (RAM_bank)
		{
		case 0x08:
			latched_time[0] = value % 60;
			seconds = latched_time[0];
			break;
		case 0x09:
			latched_time[1] = value % 60;
			minutes = latched_time[1];
			break;
		case 0x0A:
			latched_time[2] = value % 24;
			hours = latched_time[2];
			break;
		case 0x0B:
			latched_time[3] = value;
			days = latched_time[3] + (days & 0b0000000100000000);
			break;
		case 0x0C:
			latched_time[4] = value;
			days = ((latched_time[4] & 0b00000001) * 256) + (days & 0b11111111);
			rtc_halt = (latched_time[4] & 0b01000000) != 0;
			days_overflow = (latched_time[4] & 0b10000000) != 0;
			if (rtc_halt) ticks = 0;
			break;
		}
		//printf("RTC not implemented\n");
	}
}

MBC5::MBC5(std::vector<uint8_t> ROM, bool RAM, bool rumble, std::shared_ptr<SaveManager> save_manager, std::string path)
{
	GamePath = path;
	GameROM = ROM;
	saveManager = save_manager;
	if (RAM)
	{
		has_RAM = true;
		switch (GameROM[0x149]) {
		case 0x00:
			has_RAM = false;
			GameRAM = std::vector<uint8_t>(0);
			break;
		case 0x02:
			GameRAM = std::vector<uint8_t>(8192);
			break;
		case 0x03:
			GameRAM = std::vector<uint8_t>(32768);
			break;
		case 0x04:
			GameRAM = std::vector<uint8_t>(131072);
			break;
		case 0x05:
			GameRAM = std::vector<uint8_t>(65536);
			break;
		}
		if (saveManager != nullptr and GamePath != "")
		{
			std::vector<uint8_t> temp = saveManager->OpenOrCreateSave(path);
			for (int i = 0; i < temp.size(); i++)
			{
				GameRAM[i] = temp[i];
			}
		}
	}
	else
	{
		has_RAM = false;
		GameRAM = std::vector<uint8_t>(0);
	}
	has_rumble = rumble;
	ROM_bank_amount = GameROM.size() / 0x4000;
	RAM_enabled = false;
	ROM_bank = 0x01;
	RAM_bank = 0x00;
}

uint8_t MBC5::getROMValue(uint16_t address)
{
	if (address < 0x4000)
	{
		return GameROM[address];
	}
	else if (address < 0x8000)
	{
		address -= 0x4000;
		return GameROM[(address + 0x4000 * (ROM_bank % ROM_bank_amount))];
	}
}

uint8_t MBC5::getRAMValue(uint16_t address)
{
	if (RAM_enabled)
	{
		return GameRAM[address + (0x2000 * RAM_bank)];
	}
	else
	{
		return 0xFF;
	}
}

void MBC5::setROMValue(uint16_t address, uint8_t value)
{
	if (address < 0x2000)
	{
		if (has_RAM) RAM_enabled = ((value & 0x0F) == 0x0A);
	}
	else if (address < 0x3000)
	{
		ROM_bank = value + (ROM_bank & 0b1111111100000000);
	}
	else if (address < 0x4000)
	{
		ROM_bank = (value & 0b00000001) * 256 + (ROM_bank & 0b0000000011111111);
	}
	else if (address < 0x6000)
	{
		if(value < 0x10)
			if (has_rumble)
			{
				RAM_bank = value & 0b00000111;
			}
			else
			{
				RAM_bank = value & 0b00001111;
			}
	}
	return;
}

void MBC5::setRAMValue(uint16_t address, uint8_t value)
{
	if (RAM_enabled)
	{
		GameRAM[address + (0x2000 * (RAM_bank))] = value;
		if (saveManager != nullptr and GamePath != "")
		{
			saveManager->WriteToSave(address + (0x2000 * (RAM_bank)), value);
		}
	}
}



std::unique_ptr<GameCartridge> load_ROM(std::string rom_path, std::shared_ptr<SaveManager> save_manager)
{
	std::ifstream input(rom_path, std::ios::binary);
	std::vector<uint8_t> game_ROM = std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
	switch (game_ROM[0x0147])
	{
	case 0x00:
		return std::make_unique<MBC0>(game_ROM);
	case 0x01:
		return std::make_unique<MBC1>(game_ROM);
	case 0x02:
		return std::make_unique<MBC1>(game_ROM, true);
	case 0x03:
		return std::make_unique<MBC1>(game_ROM, true, save_manager, rom_path);
	case 0x05:
		return std::make_unique<MBC2>(game_ROM);
	case 0x06:
		return std::make_unique<MBC2>(game_ROM, save_manager, rom_path);
	case 0x08:
		return std::make_unique<MBC0>(game_ROM, true);
	case 0x09:
		return std::make_unique<MBC0>(game_ROM, true, save_manager, rom_path);
	case 0x0F:
		return std::make_unique<MBC3>(game_ROM, false, true, save_manager, rom_path);
	case 0x10:
		return std::make_unique<MBC3>(game_ROM, true, true, save_manager, rom_path);
	case 0x11:
		return std::make_unique<MBC3>(game_ROM);
	case 0x12:
		return std::make_unique<MBC3>(game_ROM, true);
	case 0x13:
		return std::make_unique<MBC3>(game_ROM, true, false, save_manager, rom_path);
	case 0x19:
		return std::make_unique<MBC5>(game_ROM);
	case 0x1A:
		return std::make_unique<MBC5>(game_ROM, true);
	case 0x1B:
		return std::make_unique<MBC5>(game_ROM, true, false, save_manager, rom_path);
	case 0x1C:
		return std::make_unique<MBC5>(game_ROM, false, true);
	case 0x1D:
		return std::make_unique<MBC5>(game_ROM, true, true);
	case 0x1E:
		return std::make_unique<MBC5>(game_ROM, true, true, save_manager, rom_path);
	default:
		throw std::invalid_argument("ROM of an unsupported cartridge type");
	}
}