#pragma once

#include "../Memory/MemoryManager.h"

enum TMA {
	CYCLES_256,
	CYCLES_4,
	CYCLES_16,
	CYCLES_64
};

class Timer
{
private:
	MemoryManager* mem;
	uint16_t cycle_count;
	bool inc_tima;
	bool last_tac;
	bool tima_overflow;

public:
	Timer(MemoryManager* memory);
	void reset();
	void resetCycleCount();
	void cycle();
};