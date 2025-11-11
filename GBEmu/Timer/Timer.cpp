#include "Timer.h"

Timer::Timer(MemoryManager* memory)
{
	mem = memory;
	reset();
}

void Timer::reset()
{
	cycle_count = 1;
	inc_tima = false;
	last_tac = false;
	tima_overflow = false;
}

void Timer::resetCycleCount()
{
	cycle_count = 0;
}

void Timer::cycle()
{
	cycle_count++;
	bool tac_high = false;
	if (mem->wasDIVResetPerformed())
	{
		cycle_count = 0;
	}
	uint8_t tac = mem->getIOREGValue(0xFF07);

	mem->setIOREGValue(0xFF04, cycle_count >> 6);

	if (tima_overflow)
	{
		// printf("timer overflow\n");
		tima_overflow = false;
		mem->setIOREGValue(0xFF05, mem->getIOREGValue(0xFF06));
		mem->requestInterrupt(TIMER_INTERRUPT);
	}

	switch (tac & 0b00000011) {
	case CYCLES_256:
		if ((cycle_count & 0b10000000) != 0)
		{
			tac_high = true;
		}
		break;

	case CYCLES_4:
		if ((cycle_count & 0b00000010) != 0)
		{
			tac_high = true;
		}
		break;

	case CYCLES_16:
		if ((cycle_count & 0b00001000) != 0)
		{
			tac_high = true;
		}
		break;

	case CYCLES_64:
		if ((cycle_count & 0b00100000) != 0)
		{
			tac_high = true;
		}
		break;
	}
	if (!mem->isInCGBMode())
	{
		tac_high = tac_high and ((tac & 0b00000100) != 0);
	}

	if (last_tac == true and tac_high == false)
	{
		if (!mem->isInCGBMode() or ((tac & 0b00000100) != 0))
		{
			uint8_t previous_tim = mem->getIOREGValue(0xFF05);
			if (previous_tim == 0xFF)
			{
				mem->setIOREGValue(0xFF05, 0);
				tima_overflow = true;
			}
			else
			{
				mem->setIOREGValue(0xFF05, previous_tim + 1);
			}
		}
	}
	last_tac = tac_high;
}