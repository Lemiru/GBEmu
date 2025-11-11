#include "Audio.h"

Audio::Audio(MemoryManager* memory, uint16_t buffer_len, SDL_AudioStream* stream)
{
	ch1 = std::make_shared<PulseWithSweep>();
	ch2 = std::make_shared<Pulse>();
	ch3 = std::make_shared<Wave>();
	ch4 = std::make_shared<Noise>();
	mem = memory;
	audio_stream = stream;
	buffer = new float[buffer_len * 2] {0.0f};
	buffer_lenght = buffer_len;
	sample_timer_period = 32;
	reset();
	mem->setAudioChannelRefs(ch1, ch2, ch3, ch4);
}

void Audio::cycle()
{
	if ((mem->getIOREGValue(0xFF26) & 0b10000000) != 0)
	{
		ch3->tickWave();

		ch1->tickPulse();
		ch2->tickPulse();
		ch3->tickWave();
		ch4->tickNoise();

		if (--sample_timer == 0)
		{
			sample_timer = sample_timer_period;
			uint8_t nr51 = mem->getIOREGValue(0xFF25);
			uint8_t nr50 = mem->getIOREGValue(0xFF24);
			uint8_t volume_left = (((nr50 >> 4) & 0b00000111) + 1) / 8;
			uint8_t volume_right = ((nr50 & 0b00000111) + 1) / 8;
			buffer[buffer_pos * 2] = ((((nr51 >> 4) & 0b00000001) * ch1->getOutput() * Config::getChannelEnabled(1) 
				+ ((nr51 >> 5) & 0b00000001) * ch2->getOutput() * Config::getChannelEnabled(2)
				+ ((nr51 >> 6) & 0b00000001) * ch3->getOutput() * Config::getChannelEnabled(3)
				+ ((nr51 >> 7) & 0b00000001) * ch4->getOutput() * Config::getChannelEnabled(4)) / 4)  * volume_left;
			buffer[buffer_pos * 2 + 1] = (((nr51 & 0b00000001) * ch1->getOutput() * Config::getChannelEnabled(1)
				+ ((nr51 >> 1) & 0b00000001) * ch2->getOutput() * Config::getChannelEnabled(2)
				+ ((nr51 >> 2) & 0b00000001) * ch3->getOutput() * Config::getChannelEnabled(3)
				+ ((nr51 >> 3) & 0b00000001) * ch4->getOutput() * Config::getChannelEnabled(4)) / 4) * volume_right;
			if (++buffer_pos == buffer_lenght)
			{
				buffer_pos = 0;
				SDL_PutAudioStreamData(audio_stream, buffer, buffer_lenght * 2 * sizeof(float));
			}
		}
		if ((mem->isInDoubleSpeedMode() and (mem->getIOREGValue(0xFF04) & 0b00100000) == 0) or (!mem->isInDoubleSpeedMode() and (mem->getIOREGValue(0xFF04) & 0b00010000) == 0))
		{
			if (!last_div_bit) return;
			last_div_bit = false;
			DIV_APU++;
			if ((DIV_APU & 0b00000001) == 0)
			{
				mem->setAudioLengthFirstHalf(true);
				ch1->tickLenghtTimer();
				ch2->tickLenghtTimer();
				ch3->tickLenghtTimer();
				ch4->tickLenghtTimer();
			}
			else
			{
				mem->setAudioLengthFirstHalf(false);
			}

			if ((DIV_APU & 0b00000011) == 2)
			{
				ch1->tickSweep();
				if (ch1->getSweepEnabled())
				{
					uint16_t shadow = ch1->getShadowRegister();
					mem->setIOREGValue(0xFF13, (shadow & 0b11111111));
					mem->setIOREGValue(0xFF14, (shadow >> 8));
				}
			}
			if ((DIV_APU & 0b00000111) == 7)
			{
				ch1->tickEnvelope();
				ch2->tickEnvelope();
				ch4->tickEnvelope();
			}
		}
		else
		{
			last_div_bit = true;
		}
	}
	else
	{
		mem->setIOREGValue(0xFF10, 0x00);
		mem->setIOREGValue(0xFF11, 0x00);
		mem->setIOREGValue(0xFF12, 0x00);
		mem->setIOREGValue(0xFF13, 0x00);
		mem->setIOREGValue(0xFF14, 0x00);
		mem->setIOREGValue(0xFF16, 0x00);
		mem->setIOREGValue(0xFF17, 0x00);
		mem->setIOREGValue(0xFF18, 0x00);
		mem->setIOREGValue(0xFF19, 0x00);
		mem->setIOREGValue(0xFF1A, 0x00);
		mem->setIOREGValue(0xFF1B, 0x00);
		mem->setIOREGValue(0xFF1C, 0x00);
		mem->setIOREGValue(0xFF1D, 0x00);
		mem->setIOREGValue(0xFF1E, 0x00);
		mem->setIOREGValue(0xFF20, 0x00);
		mem->setIOREGValue(0xFF21, 0x00);
		mem->setIOREGValue(0xFF22, 0x00);
		mem->setIOREGValue(0xFF23, 0x00);
		mem->setIOREGValue(0xFF24, 0x00);
		mem->setIOREGValue(0xFF25, 0x00);
		ch1->reset();
		ch2->reset();
		ch3->reset();
		ch4->reset();
	}
}

void Audio::reset()
{
	buffer_pos = 0;
	sample_timer = sample_timer_period;
	DIV_APU = 0;
	last_div_bit = false;
	ch1->reset();
	ch2->reset();
	ch3->reset();
	ch4->reset();
	SDL_ClearAudioStream(audio_stream);
	last_tick_time = SDL_GetTicksNS();
}
