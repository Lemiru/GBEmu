#pragma once

#include <stdint.h>
#include <vector>

const bool DUTY_CYCLES[4][8] = {
	{0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 1, 1, 1},
	{0, 1, 1, 1, 1, 1, 1, 0}
};

class AudioChannel
{
protected:
	bool enabled;
	bool length_enabled;
	bool dac_enabled;
	bool debug_mode;
	uint16_t length_timer;
	uint16_t period_timer;
	uint16_t period;
	uint8_t volume;
public:
	virtual void reset() = 0;
	void trigger(uint8_t nrx3, uint8_t nrx4, bool clock_timer);
	void setLenghtEnabled(bool enabled);
	virtual void setLenghtTimer(uint8_t val);
	void tickLenghtTimer();
	virtual uint8_t getDigitalOutput() const;
	virtual float getOutput() const;
	void setPeriod(uint16_t val);
	void setDACEnabled(bool val);
	bool getEnabled();
};

class Pulse : public AudioChannel
{
protected:
	uint8_t current_duty;
	uint8_t duty_cycle_step;
	uint8_t envelope_sweep_pace;
	uint8_t envelope_timer;
	bool envelope_dir;
public:
	Pulse(bool debug = false);
	void trigger(uint8_t nrx1, uint8_t nrx2, uint8_t nrx3, uint8_t nrx4, bool clock_timer);
	void setDutyCycle(uint8_t cycle);
	void tickPulse();
	void tickEnvelope();
	void envelopeDirWrite(bool val);
	void reset() override;
	uint8_t getDigitalOutput() const override;
	float getOutput() const override;
};

class PulseWithSweep : public Pulse
{
private:
	bool sweep_enabled;
	uint16_t shadow_period;
	uint8_t sweep_timer;
	uint8_t sweep_pace;
	bool sweep_dir;
	
	uint8_t sweep_step;
public:
	PulseWithSweep(bool debug = false);
	void trigger(uint8_t nrx0, uint8_t nrx1, uint8_t nrx2, uint8_t nrx3, uint8_t nrx4, bool clock_timer);
	void tickSweep();
	void calculateFrequency();
	void reset() override;
	void setSweepValues(bool dir, uint8_t step, uint8_t pace);
	void setSweepEnabled(bool val);
	bool getSweepEnabled();
	uint16_t getShadowRegister() const;
};

class Wave : public AudioChannel
{
private:
	uint8_t wave_index;
	uint8_t last_wave_value;
	std::vector<uint8_t> wave_RAM;
public:
	Wave(bool debug = false);
	void trigger(uint8_t nrx0, uint8_t nrx1, uint8_t nrx2, uint8_t nrx3, uint8_t nrx4, bool clock_timer);
	void setLenghtTimer(uint8_t val) override;
	void setWaveRAMValue(uint8_t pos, uint8_t val);
	void setVolume(uint8_t val);
	void tickWave();
	void readWaveValue();
	void reset() override;
	uint8_t getDigitalOutput() const override;
	float getOutput() const override;
};

class Noise : public AudioChannel
{
private:
	bool is_short;
	uint16_t lfsr_state;
	uint8_t envelope_sweep_pace;
	uint8_t envelope_timer;
	bool envelope_dir;
	uint8_t shift;
	uint8_t divider;
	uint16_t ticks;
public:
	Noise(bool debug = false);
	void trigger(uint8_t nrx1, uint8_t nrx2, uint8_t nrx3, uint8_t nrx4, bool clock_timer);
	void tickNoise();
	void setDivider(uint8_t val);
	void setShift(uint8_t val);
	void setShort(bool val);
	void tickEnvelope();
	void envelopeDirWrite(bool val);
	void reset() override;
	uint8_t getDigitalOutput() const override;
	float getOutput() const override;
};