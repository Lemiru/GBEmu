#include "Channels.h"
#include <stdio.h>

void AudioChannel::trigger(uint8_t nrx3, uint8_t nrx4, bool clock_timer)
{
	if (!dac_enabled)
	{
		return;
	}
	enabled = true;
	period = (((nrx4 & 0b00000111)) << 8) + nrx3;
	if (period_timer == 0) period_timer = period;
	if (clock_timer)
	{
		tickLenghtTimer();
	}
}

void AudioChannel::setLenghtEnabled(bool enabled)
{
	length_enabled = enabled;
}

void AudioChannel::setLenghtTimer(uint8_t val)
{
	length_timer = 64 - val;
	if (length_timer == 0)
	{
		length_timer = 64;
	}
}

void AudioChannel::tickLenghtTimer()
{
	if (length_enabled)
	{
		if (length_timer > 0)
		{
			length_timer--;
		}
		if (length_timer == 0)
		{
			enabled = false;
		}
	}

}

void AudioChannel::setPeriod(uint16_t val)
{
	period = val;
}

uint8_t AudioChannel::getDigitalOutput() const
{
	return 0;
}

float AudioChannel::getOutput() const
{
	return 1.0f;
}

void AudioChannel::setDACEnabled(bool val)
{
	if (val)
	{
		dac_enabled = true;
	}
	else
	{
		dac_enabled = false;
		enabled = false;
	}
}

bool AudioChannel::getEnabled()
{
	return enabled;
}

Pulse::Pulse(bool debug)
{
	debug_mode = debug;
	reset();
}

void Pulse::trigger(uint8_t nrx1, uint8_t nrx2, uint8_t nrx3, uint8_t nrx4, bool clock_timer)
{
	AudioChannel::trigger(nrx3, nrx4, clock_timer);
	if (!enabled)
	{
		return;
	}
	if (length_timer == 0)
	{
		if (length_timer == 0)
		{
			length_timer = 64;
		}
	}
	volume = nrx2 >> 4;
	envelope_dir = (nrx2 & 0b00001000) != 0;
	envelope_sweep_pace = nrx2 & 0b00000111;
	envelope_timer = 0;
	if (volume == 0 and !envelope_dir)
	{
		enabled = false;
		dac_enabled = false;
	}
}

void Pulse::setDutyCycle(uint8_t cycle)
{
	current_duty = cycle;
}

void Pulse::tickPulse()
{
	period_timer++;
	if(period_timer == 0x800)
	{
		period_timer = period;
		duty_cycle_step++;
		if (duty_cycle_step == 8)
			duty_cycle_step = 0;
	}
}

void Pulse::tickEnvelope()
{
	envelope_timer++;
	if (envelope_sweep_pace != 0 and (envelope_timer == envelope_sweep_pace))
	{
		envelope_timer = 0;
		if (envelope_dir and volume != 15)
			volume++;
		else if (!envelope_dir and volume != 0)
			volume--;
	}
}

void Pulse::envelopeDirWrite(bool val)
{
	if (envelope_sweep_pace == 0)
	{
		volume = (volume + 1 + !val) & 0b00001111;
	}
}

void Pulse::reset()
{
	current_duty = 0;
	duty_cycle_step = 0;
	period_timer = 0;
	envelope_timer = 0;
	envelope_dir = true;
	envelope_sweep_pace = 0;
	enabled = false;
	dac_enabled = false;
}

uint8_t Pulse::getDigitalOutput() const
{
	return volume * DUTY_CYCLES[current_duty][duty_cycle_step];
}

float Pulse::getOutput() const
{
	if (!dac_enabled)
	{
		return 0.0f;
	}
	if (!enabled)
	{
		return 1.0f;
	}
	return 1.0f - (( getDigitalOutput() / 15.0f) * 2);
}

PulseWithSweep::PulseWithSweep(bool debug)
{
	debug_mode = debug;
	reset();
}

void PulseWithSweep::trigger(uint8_t nrx0, uint8_t nrx1, uint8_t nrx2, uint8_t nrx3, uint8_t nrx4, bool clock_timer)
{
	//printf("trigger\n");
	Pulse::trigger(nrx1, nrx2, nrx3, nrx4, clock_timer);
	if (!enabled)
	{
		return;
	}
	sweep_pace = (nrx0 & 0b01111111)>>4;
	sweep_timer = sweep_pace;
	sweep_dir = (nrx0 & 0b00001000) != 0;
	sweep_step = nrx0 & 0b00000111;
	sweep_enabled = sweep_pace != 0 or sweep_step != 0;
	shadow_period = period;
	if (sweep_enabled and (sweep_step != 0))
	{
		calculateFrequency();
	}
}

void PulseWithSweep::tickSweep()
{
	if (sweep_pace != 0)
	{
		sweep_timer--;
		if (sweep_timer == sweep_pace)
		{
			calculateFrequency();
			sweep_timer = sweep_pace;
		}
	}	
}

void PulseWithSweep::calculateFrequency()
{
	if (sweep_enabled)
	{
		//printf("Calculated frequency \n");
		//printf("register: %X\n", shadow_period);
		uint16_t shifted = shadow_period >> sweep_step;
		if (sweep_dir)
		{
			shifted = shadow_period - shifted;
		}
		else
		{
			shifted += shadow_period;
		}
		//printf("register after: %X\n", shifted);
		if (shifted > 0x7FF)
		{
			enabled = false;
			return;
		}
		else
		{
			shadow_period = shifted;
		}
		shifted = shadow_period >> sweep_step;
		if (sweep_dir)
		{
			shifted = shadow_period - shifted;
		}
		else
		{
			shifted += shadow_period;
		}
		if (shifted > 0x7FF)
		{
			enabled = false;
		}
	}
}

void PulseWithSweep::setSweepValues(bool dir, uint8_t step, uint8_t pace)
{
	sweep_dir = dir;
	sweep_step = step;
	sweep_pace = pace;
}

void PulseWithSweep::setSweepEnabled(bool val)
{
	sweep_enabled = val;
}

bool PulseWithSweep::getSweepEnabled()
{
	return sweep_enabled;
}

uint16_t PulseWithSweep::getShadowRegister() const
{
	return shadow_period;
}

void PulseWithSweep::reset()
{
	Pulse::reset();
	shadow_period = 0;
	sweep_timer = 0;
	sweep_pace = 0;
	sweep_dir = 0;
	sweep_step = 0;
	sweep_enabled = false;
	duty_cycle_step = 0;
	period_timer = 0;
	envelope_timer = 0;
}

Wave::Wave(bool debug)
{
	debug_mode = debug;
	reset();
}

void Wave::trigger(uint8_t nrx0, uint8_t nrx1, uint8_t nrx2, uint8_t nrx3, uint8_t nrx4, bool clock_timer)
{
	AudioChannel::trigger(nrx3, nrx4, clock_timer);
	if (!enabled)
	{
		return;
	}
	if (length_timer == 0)
	{
		if (length_timer == 0)
		{
			length_timer = 256;
		}
	}
	volume = (nrx2 & 0b01100000) >> 5;
	wave_index = 0;
}

void Wave::tickWave()
{
	period_timer++;
	if (period_timer == 0x800)
	{
		period_timer = period;
		readWaveValue();
	}
}

void Wave::setWaveRAMValue(uint8_t pos, uint8_t val)
{
	if (pos >= 16)
		return;
	wave_RAM[pos] = val;
}

void Wave::readWaveValue()
{
	wave_index++;
	if (wave_index == 32) wave_index = 0;
	uint8_t wave_byte = wave_RAM[wave_index>>1];
	last_wave_value = (wave_byte >> (1 - (wave_index & 0b00000001)) * 4) & 0b00001111;
}

void Wave::setLenghtTimer(uint8_t val)
{
	length_timer = 256 - val;
	if (length_timer == 0)
	{
		length_timer = 256;
	}
}

void Wave::setVolume(uint8_t val)
{
	volume = val;
}

uint8_t Wave::getDigitalOutput() const
{
	uint8_t offset = (volume - 1);
	return (last_wave_value >> offset);
}

float Wave::getOutput() const
{
	if (!dac_enabled) return 0.0f;
	if (volume == 0) return 1.0f;
	uint8_t offset = (volume - 1);
	return 1.0f - (( getDigitalOutput() / 15.0f) * 2);
}

void Wave::reset()
{
	enabled = false;
	dac_enabled = false;
	volume = 0;
	wave_index = 0;
	last_wave_value = 0;
	wave_RAM = std::vector<uint8_t>(16, 0);
}


Noise::Noise(bool debug)
{
	debug_mode = debug;
	reset();
}

void Noise::reset()
{
	is_short = false;
	lfsr_state = 0;
	envelope_timer = 0;
	envelope_dir = true;
	envelope_sweep_pace = 0;
	shift = 0;
	divider = 0;
	ticks = 0;
	enabled = false;
	dac_enabled = false;
}

void Noise::trigger(uint8_t nrx1, uint8_t nrx2, uint8_t nrx3, uint8_t nrx4, bool clock_timer)
{
	AudioChannel::trigger(nrx3, nrx4, clock_timer);
	if (!enabled)
	{
		return;
	}
	if (length_timer == 0)
	{
		if (length_timer == 0)
		{
			length_timer = 64;
		}
	}
	volume = nrx2 >> 4;
	envelope_dir = (nrx2 & 0b00001000) != 0;
	envelope_sweep_pace = nrx2 & 0b00000111;
	envelope_timer = 0;
	if (volume == 0 and !envelope_dir)
	{
		enabled = false;
		dac_enabled = false;
	}
	lfsr_state = 0;
	ticks = 0;
}

void Noise::tickNoise()
{
	if (enabled)
	{
		if (shift > 13) return;
		if (ticks == 0)
		{
			if (divider == 0)
			{
				ticks = 4 << shift;
			}
			else
			{
				ticks = (divider * 8) << shift;
			}
			lfsr_state = (lfsr_state & 0b0111111111111111) + (~((lfsr_state & 0b0000000000000001) ^ (lfsr_state & 0b0000000000000010) >> 1) << 15);
			if (is_short)
			{
				lfsr_state = (lfsr_state & 0b1111111101111111) + ((lfsr_state >> 15) << 7);
			}
			lfsr_state = lfsr_state >> 1;
			//printf("ticks: %d\n", ticks);
		}
		ticks--;
	}
}

void Noise::setDivider(uint8_t val)
{
	divider = val;
}

void Noise::setShift(uint8_t val)
{
	shift = val;
}

void Noise::setShort(bool val)
{
	is_short = val;
}

void Noise::tickEnvelope()
{
	if (envelope_sweep_pace != 0)
	{
		envelope_timer++;
		if (envelope_timer == envelope_sweep_pace)
		{
			envelope_timer = 0;
			if (envelope_dir and volume != 15)
				volume++;
			else if (!envelope_dir and volume != 0)
				volume--;
		}
	}
}

void Noise::envelopeDirWrite(bool val)
{
	if (envelope_sweep_pace == 0)
	{
		volume = (volume + 1 + !val) & 0b00001111;
	}
}

uint8_t Noise::getDigitalOutput() const
{
	return (lfsr_state & 0b0000000000000001) * volume;
}

float Noise::getOutput() const
{
	if (!dac_enabled)
	{
		return 0.0f;
	}
	if (!enabled)
	{
		return 1.0f;
	}
	return 1.0f - ((getDigitalOutput() / 15.0f) * 2);
}