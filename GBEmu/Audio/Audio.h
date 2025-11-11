#pragma once

#include <stdint.h>
#include <SDL3/SDL.h>

#include "Channels.h"
#include <vector>
#include "../Memory/MemoryManager.h"

class Audio 
{
private:
	MemoryManager* mem;
	SDL_AudioStream* audio_stream;
	float* buffer;
	uint16_t buffer_pos;
	uint16_t buffer_lenght;
	uint8_t sample_timer;
	uint8_t sample_timer_period;
	uint8_t DIV_APU;
	bool last_div_bit;
	std::shared_ptr<PulseWithSweep> ch1;
	std::shared_ptr<Pulse> ch2;
	std::shared_ptr<Wave> ch3;
	std::shared_ptr<Noise> ch4;
	uint64_t last_tick_time;

public:
	Audio(MemoryManager* memory, uint16_t buffer_len, SDL_AudioStream* stream);
	void cycle();
	void reset();
};