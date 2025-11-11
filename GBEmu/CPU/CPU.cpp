#include "CPU.h"

uint8_t INSTRUCTION_TIMING[]
{
	1,3,2,2,1,1,2,1,5,2,2,2,1,1,2,1,
	0,3,2,2,1,1,2,1,3,2,2,2,1,1,2,1,
	3,3,2,2,1,1,2,1,3,2,2,2,1,1,2,1,
	3,3,2,2,3,3,3,1,3,2,2,2,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	2,2,2,2,2,2,0,2,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	5,3,4,4,6,4,2,4,5,4,4,0,6,6,2,4,
	5,3,4,0,6,4,2,4,5,4,4,0,6,0,2,4,
	3,3,2,0,0,4,2,4,4,1,4,0,0,0,2,4,
	3,3,2,1,0,4,2,4,3,2,4,1,0,0,2,4
};

uint8_t INSTRUCTION_TIMING_PREFIXED[]
{
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2
};

uint16_t RST_VECTOR[]
{
	0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38
};

CPU::CPU(MemoryManager* memory)
{
	MEM = memory;
	reset();
}

void CPU::reset()
{
	if (MEM->bootROMAvailable())
	{
		AF = 0x0000;
		BC = 0x0000;
		DE = 0x0000;
		HL = 0x0000;
		SP = 0x0000;
		PC = 0x0000;
		
	}
	else
	{
		if (MEM->isInCGBMode())
		{
			AF = 0x1180;
			BC = 0x0000;
			DE = 0xFF56;
			HL = 0x000D;
			SP = 0xFFFE;
			PC = 0x0100;
		}
		else
		{
			AF = 0x01B0;
			BC = 0x0013;
			DE = 0x00D8;
			HL = 0x014D;
			SP = 0xFFFE;
			PC = 0x0100;
		}
	}
	IME = false;
	busyCycles = 0;
	EI_called = false;
	EI_pending = false;
	prefix_instruction = false;
	conditional = false;
	interrupt_in_progress = false;
	halt = false;
	halt_bug = false;
	val8 = 0;
	val16 = 0;
}

void CPU::cycle()
{

	if (busyCycles == 0)
	{
		if (EI_called)
		{
			EI_called = false;
			EI_pending = true;
		}
		else if (EI_pending)
		{
			EI_pending = false;
			IME = true;
		}
		check_and_initiate_interrupt();
		if (!interrupt_in_progress and !halt)
		{
			
			/*uint8_t cur_pc = get_16bit_register(REGISTER_SP);
			printf("A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
				get_8bit_register(REGISTER_A),
				get_8bit_register(REGISTER_F),
				get_8bit_register(REGISTER_B),
				get_8bit_register(REGISTER_C),
				get_8bit_register(REGISTER_D),
				get_8bit_register(REGISTER_E),
				get_8bit_register(REGISTER_H),
				get_8bit_register(REGISTER_L),
				get_16bit_register(REGISTER_SP),
				get_16bit_register(REGISTER_PC),
				MEM->getMemValue(PC),
				MEM->getMemValue(PC + 1),
				MEM->getMemValue(PC + 2),
				MEM->getMemValue(PC + 3));*/
			
			uint8_t current_bit = get_immediate_8bit_value();

			prefix_instruction = false;
			switch (current_bit)
			{
			case 0x10:
				//STOP
				nextInstruction = current_bit;
				busyCycles = 1;
				break;
			case 0x76:
				//HALT
				// printf("HALT CALLED\n");
				nextInstruction = current_bit;
				busyCycles = 1;
				break;
			case 0xCB:
				current_bit = get_immediate_8bit_value();
				nextInstruction = current_bit;
				busyCycles = INSTRUCTION_TIMING_PREFIXED[current_bit];
				prefix_instruction = true;
				break;
			default:
				nextInstruction = current_bit;
				busyCycles = INSTRUCTION_TIMING[current_bit];
				break;
			}
		}
	}

	if (interrupt_in_progress)
	{
		busyCycles--;
		handle_interrupt();
		return;
	}

	if (!halt)
	{
		busyCycles--;
		if (prefix_instruction)
		{
			switch (nextInstruction)
			{
			case 0x00:RLC(REGISTER_B); break;
			case 0x01:RLC(REGISTER_C); break;
			case 0x02:RLC(REGISTER_D); break;
			case 0x03:RLC(REGISTER_E); break;
			case 0x04:RLC(REGISTER_H); break;
			case 0x05:RLC(REGISTER_L); break;
			case 0x06:RLC_MEM(); break;
			case 0x07:RLC(REGISTER_A); break;
			case 0x08:RRC(REGISTER_B); break;
			case 0x09:RRC(REGISTER_C); break;
			case 0x0A:RRC(REGISTER_D); break;
			case 0x0B:RRC(REGISTER_E); break;
			case 0x0C:RRC(REGISTER_H); break;
			case 0x0D:RRC(REGISTER_L); break;
			case 0x0E:RRC_MEM(); break;
			case 0x0F:RRC(REGISTER_A); break;
			case 0x10:RL(REGISTER_B); break;
			case 0x11:RL(REGISTER_C); break;
			case 0x12:RL(REGISTER_D); break;
			case 0x13:RL(REGISTER_E); break;
			case 0x14:RL(REGISTER_H); break;
			case 0x15:RL(REGISTER_L); break;
			case 0x16:RL_MEM(); break;
			case 0x17:RL(REGISTER_A); break;
			case 0x18:RR(REGISTER_B); break;
			case 0x19:RR(REGISTER_C); break;
			case 0x1A:RR(REGISTER_D); break;
			case 0x1B:RR(REGISTER_E); break;
			case 0x1C:RR(REGISTER_H); break;
			case 0x1D:RR(REGISTER_L); break;
			case 0x1E:RR_MEM(); break;
			case 0x1F:RR(REGISTER_A); break;
			case 0x20:SLA(REGISTER_B); break;
			case 0x21:SLA(REGISTER_C); break;
			case 0x22:SLA(REGISTER_D); break;
			case 0x23:SLA(REGISTER_E); break;
			case 0x24:SLA(REGISTER_H); break;
			case 0x25:SLA(REGISTER_L); break;
			case 0x26:SLA_MEM(); break;
			case 0x27:SLA(REGISTER_A); break;
			case 0x28:SRA(REGISTER_B); break;
			case 0x29:SRA(REGISTER_C); break;
			case 0x2A:SRA(REGISTER_D); break;
			case 0x2B:SRA(REGISTER_E); break;
			case 0x2C:SRA(REGISTER_H); break;
			case 0x2D:SRA(REGISTER_L); break;
			case 0x2E:SRA_MEM(); break;
			case 0x2F:SRA(REGISTER_A); break;
			case 0x30:SWAP(REGISTER_B); break;
			case 0x31:SWAP(REGISTER_C); break;
			case 0x32:SWAP(REGISTER_D); break;
			case 0x33:SWAP(REGISTER_E); break;
			case 0x34:SWAP(REGISTER_H); break;
			case 0x35:SWAP(REGISTER_L); break;
			case 0x36:SWAP_MEM(); break;
			case 0x37:SWAP(REGISTER_A); break;
			case 0x38:SRL(REGISTER_B); break;
			case 0x39:SRL(REGISTER_C); break;
			case 0x3A:SRL(REGISTER_D); break;
			case 0x3B:SRL(REGISTER_E); break;
			case 0x3C:SRL(REGISTER_H); break;
			case 0x3D:SRL(REGISTER_L); break;
			case 0x3E:SRL_MEM(); break;
			case 0x3F:SRL(REGISTER_A); break;
			case 0x40:BIT(0, REGISTER_B); break;
			case 0x41:BIT(0, REGISTER_C); break;
			case 0x42:BIT(0, REGISTER_D); break;
			case 0x43:BIT(0, REGISTER_E); break;
			case 0x44:BIT(0, REGISTER_H); break;
			case 0x45:BIT(0, REGISTER_L); break;
			case 0x46:BIT_MEM(0); break;
			case 0x47:BIT(0, REGISTER_A); break;
			case 0x48:BIT(1, REGISTER_B); break;
			case 0x49:BIT(1, REGISTER_C); break;
			case 0x4A:BIT(1, REGISTER_D); break;
			case 0x4B:BIT(1, REGISTER_E); break;
			case 0x4C:BIT(1, REGISTER_H); break;
			case 0x4D:BIT(1, REGISTER_L); break;
			case 0x4E:BIT_MEM(1); break;
			case 0x4F:BIT(1, REGISTER_A); break;
			case 0x50:BIT(2, REGISTER_B); break;
			case 0x51:BIT(2, REGISTER_C); break;
			case 0x52:BIT(2, REGISTER_D); break;
			case 0x53:BIT(2, REGISTER_E); break;
			case 0x54:BIT(2, REGISTER_H); break;
			case 0x55:BIT(2, REGISTER_L); break;
			case 0x56:BIT_MEM(2); break;
			case 0x57:BIT(2, REGISTER_A); break;
			case 0x58:BIT(3, REGISTER_B); break;
			case 0x59:BIT(3, REGISTER_C); break;
			case 0x5A:BIT(3, REGISTER_D); break;
			case 0x5B:BIT(3, REGISTER_E); break;
			case 0x5C:BIT(3, REGISTER_H); break;
			case 0x5D:BIT(3, REGISTER_L); break;
			case 0x5E:BIT_MEM(3); break;
			case 0x5F:BIT(3, REGISTER_A); break;
			case 0x60:BIT(4, REGISTER_B); break;
			case 0x61:BIT(4, REGISTER_C); break;
			case 0x62:BIT(4, REGISTER_D); break;
			case 0x63:BIT(4, REGISTER_E); break;
			case 0x64:BIT(4, REGISTER_H); break;
			case 0x65:BIT(4, REGISTER_L); break;
			case 0x66:BIT_MEM(4); break;
			case 0x67:BIT(4, REGISTER_A); break;
			case 0x68:BIT(5, REGISTER_B); break;
			case 0x69:BIT(5, REGISTER_C); break;
			case 0x6A:BIT(5, REGISTER_D); break;
			case 0x6B:BIT(5, REGISTER_E); break;
			case 0x6C:BIT(5, REGISTER_H); break;
			case 0x6D:BIT(5, REGISTER_L); break;
			case 0x6E:BIT_MEM(5); break;
			case 0x6F:BIT(5, REGISTER_A); break;
			case 0x70:BIT(6, REGISTER_B); break;
			case 0x71:BIT(6, REGISTER_C); break;
			case 0x72:BIT(6, REGISTER_D); break;
			case 0x73:BIT(6, REGISTER_E); break;
			case 0x74:BIT(6, REGISTER_H); break;
			case 0x75:BIT(6, REGISTER_L); break;
			case 0x76:BIT_MEM(6); break;
			case 0x77:BIT(6, REGISTER_A); break;
			case 0x78:BIT(7, REGISTER_B); break;
			case 0x79:BIT(7, REGISTER_C); break;
			case 0x7A:BIT(7, REGISTER_D); break;
			case 0x7B:BIT(7, REGISTER_E); break;
			case 0x7C:BIT(7, REGISTER_H); break;
			case 0x7D:BIT(7, REGISTER_L); break;
			case 0x7E:BIT_MEM(7); break;
			case 0x7F:BIT(7, REGISTER_A); break;
			case 0x80:RES(0, REGISTER_B); break;
			case 0x81:RES(0, REGISTER_C); break;
			case 0x82:RES(0, REGISTER_D); break;
			case 0x83:RES(0, REGISTER_E); break;
			case 0x84:RES(0, REGISTER_H); break;
			case 0x85:RES(0, REGISTER_L); break;
			case 0x86:RES_MEM(0); break;
			case 0x87:RES(0, REGISTER_A); break;
			case 0x88:RES(1, REGISTER_B); break;
			case 0x89:RES(1, REGISTER_C); break;
			case 0x8A:RES(1, REGISTER_D); break;
			case 0x8B:RES(1, REGISTER_E); break;
			case 0x8C:RES(1, REGISTER_H); break;
			case 0x8D:RES(1, REGISTER_L); break;
			case 0x8E:RES_MEM(1); break;
			case 0x8F:RES(1, REGISTER_A); break;
			case 0x90:RES(2, REGISTER_B); break;
			case 0x91:RES(2, REGISTER_C); break;
			case 0x92:RES(2, REGISTER_D); break;
			case 0x93:RES(2, REGISTER_E); break;
			case 0x94:RES(2, REGISTER_H); break;
			case 0x95:RES(2, REGISTER_L); break;
			case 0x96:RES_MEM(2); break;
			case 0x97:RES(2, REGISTER_A); break;
			case 0x98:RES(3, REGISTER_B); break;
			case 0x99:RES(3, REGISTER_C); break;
			case 0x9A:RES(3, REGISTER_D); break;
			case 0x9B:RES(3, REGISTER_E); break;
			case 0x9C:RES(3, REGISTER_H); break;
			case 0x9D:RES(3, REGISTER_L); break;
			case 0x9E:RES_MEM(3); break;
			case 0x9F:RES(3, REGISTER_A); break;
			case 0xA0:RES(4, REGISTER_B); break;
			case 0xA1:RES(4, REGISTER_C); break;
			case 0xA2:RES(4, REGISTER_D); break;
			case 0xA3:RES(4, REGISTER_E); break;
			case 0xA4:RES(4, REGISTER_H); break;
			case 0xA5:RES(4, REGISTER_L); break;
			case 0xA6:RES_MEM(4); break;
			case 0xA7:RES(4, REGISTER_A); break;
			case 0xA8:RES(5, REGISTER_B); break;
			case 0xA9:RES(5, REGISTER_C); break;
			case 0xAA:RES(5, REGISTER_D); break;
			case 0xAB:RES(5, REGISTER_E); break;
			case 0xAC:RES(5, REGISTER_H); break;
			case 0xAD:RES(5, REGISTER_L); break;
			case 0xAE:RES_MEM(5); break;
			case 0xAF:RES(5, REGISTER_A); break;
			case 0xB0:RES(6, REGISTER_B); break;
			case 0xB1:RES(6, REGISTER_C); break;
			case 0xB2:RES(6, REGISTER_D); break;
			case 0xB3:RES(6, REGISTER_E); break;
			case 0xB4:RES(6, REGISTER_H); break;
			case 0xB5:RES(6, REGISTER_L); break;
			case 0xB6:RES_MEM(6); break;
			case 0xB7:RES(6, REGISTER_A); break;
			case 0xB8:RES(7, REGISTER_B); break;
			case 0xB9:RES(7, REGISTER_C); break;
			case 0xBA:RES(7, REGISTER_D); break;
			case 0xBB:RES(7, REGISTER_E); break;
			case 0xBC:RES(7, REGISTER_H); break;
			case 0xBD:RES(7, REGISTER_L); break;
			case 0xBE:RES_MEM(7); break;
			case 0xBF:RES(7, REGISTER_A); break;
			case 0xC0:SET(0, REGISTER_B); break;
			case 0xC1:SET(0, REGISTER_C); break;
			case 0xC2:SET(0, REGISTER_D); break;
			case 0xC3:SET(0, REGISTER_E); break;
			case 0xC4:SET(0, REGISTER_H); break;
			case 0xC5:SET(0, REGISTER_L); break;
			case 0xC6:SET_MEM(0); break;
			case 0xC7:SET(0, REGISTER_A); break;
			case 0xC8:SET(1, REGISTER_B); break;
			case 0xC9:SET(1, REGISTER_C); break;
			case 0xCA:SET(1, REGISTER_D); break;
			case 0xCB:SET(1, REGISTER_E); break;
			case 0xCC:SET(1, REGISTER_H); break;
			case 0xCD:SET(1, REGISTER_L); break;
			case 0xCE:SET_MEM(1); break;
			case 0xCF:SET(1, REGISTER_A); break;
			case 0xD0:SET(2, REGISTER_B); break;
			case 0xD1:SET(2, REGISTER_C); break;
			case 0xD2:SET(2, REGISTER_D); break;
			case 0xD3:SET(2, REGISTER_E); break;
			case 0xD4:SET(2, REGISTER_H); break;
			case 0xD5:SET(2, REGISTER_L); break;
			case 0xD6:SET_MEM(2); break;
			case 0xD7:SET(2, REGISTER_A); break;
			case 0xD8:SET(3, REGISTER_B); break;
			case 0xD9:SET(3, REGISTER_C); break;
			case 0xDA:SET(3, REGISTER_D); break;
			case 0xDB:SET(3, REGISTER_E); break;
			case 0xDC:SET(3, REGISTER_H); break;
			case 0xDD:SET(3, REGISTER_L); break;
			case 0xDE:SET_MEM(3); break;
			case 0xDF:SET(3, REGISTER_A); break;
			case 0xE0:SET(4, REGISTER_B); break;
			case 0xE1:SET(4, REGISTER_C); break;
			case 0xE2:SET(4, REGISTER_D); break;
			case 0xE3:SET(4, REGISTER_E); break;
			case 0xE4:SET(4, REGISTER_H); break;
			case 0xE5:SET(4, REGISTER_L); break;
			case 0xE6:SET_MEM(4); break;
			case 0xE7:SET(4, REGISTER_A); break;
			case 0xE8:SET(5, REGISTER_B); break;
			case 0xE9:SET(5, REGISTER_C); break;
			case 0xEA:SET(5, REGISTER_D); break;
			case 0xEB:SET(5, REGISTER_E); break;
			case 0xEC:SET(5, REGISTER_H); break;
			case 0xED:SET(5, REGISTER_L); break;
			case 0xEE:SET_MEM(5); break;
			case 0xEF:SET(5, REGISTER_A); break;
			case 0xF0:SET(6, REGISTER_B); break;
			case 0xF1:SET(6, REGISTER_C); break;
			case 0xF2:SET(6, REGISTER_D); break;
			case 0xF3:SET(6, REGISTER_E); break;
			case 0xF4:SET(6, REGISTER_H); break;
			case 0xF5:SET(6, REGISTER_L); break;
			case 0xF6:SET_MEM(6); break;
			case 0xF7:SET(6, REGISTER_A); break;
			case 0xF8:SET(7, REGISTER_B); break;
			case 0xF9:SET(7, REGISTER_C); break;
			case 0xFA:SET(7, REGISTER_D); break;
			case 0xFB:SET(7, REGISTER_E); break;
			case 0xFC:SET(7, REGISTER_H); break;
			case 0xFD:SET(7, REGISTER_L); break;
			case 0xFE:SET_MEM(7); break;
			case 0xFF:SET(7, REGISTER_A); break;
			default:
				break;
			}
		}
		else
		{
			switch (nextInstruction)
			{
			case 0x00:break;
			case 0x01:LD_16BIT(REGISTER_BC); break;
			case 0x02:LD_TO_MEM(REGISTER_BC, REGISTER_A); break;
			case 0x03:INC(REGISTER_BC); break;
			case 0x04:INC(REGISTER_B); break;
			case 0x05:DEC(REGISTER_B); break;
			case 0x06:LD(REGISTER_B); break;
			case 0x07:RLC(REGISTER_A, true); break;
			case 0x08:LD_16BIT_TO_MEM(); break;
			case 0x09:ADD_HL(REGISTER_BC); break;
			case 0x0A:LD_FROM_MEM(REGISTER_A, REGISTER_BC); break;
			case 0x0B:DEC(REGISTER_BC); break;
			case 0x0C:INC(REGISTER_C); break;
			case 0x0D:DEC(REGISTER_C); break;
			case 0x0E:LD(REGISTER_C); break;
			case 0x0F:RRC(REGISTER_A, true); break;
			case 0x10:STOP(); break;
			case 0x11:LD_16BIT(REGISTER_DE); break;
			case 0x12:LD_TO_MEM(REGISTER_DE, REGISTER_A); break;
			case 0x13:INC(REGISTER_DE); break;
			case 0x14:INC(REGISTER_D); break;
			case 0x15:DEC(REGISTER_D); break;
			case 0x16:LD(REGISTER_D); break;
			case 0x17:RL(REGISTER_A, true); break;
			case 0x18:JR(); break;
			case 0x19:ADD_HL(REGISTER_DE); break;
			case 0x1A:LD_FROM_MEM(REGISTER_A, REGISTER_DE); break;
			case 0x1B:DEC(REGISTER_DE); break;
			case 0x1C:INC(REGISTER_E); break;
			case 0x1D:DEC(REGISTER_E); break;
			case 0x1E:LD(REGISTER_E); break;
			case 0x1F:RR(REGISTER_A, true); break;
			case 0x20:JR(check_condition(NZ)); break;
			case 0x21:LD_16BIT(REGISTER_HL); break;
			case 0x22:LDI_TO_MEM(); break;
			case 0x23:INC(REGISTER_HL); break;
			case 0x24:INC(REGISTER_H); break;
			case 0x25:DEC(REGISTER_H); break;
			case 0x26:LD(REGISTER_H); break;
			case 0x27:DAA(); break;
			case 0x28:JR(check_condition(Z)); break;
			case 0x29:ADD_HL(REGISTER_HL); break;
			case 0x2A:LDI_FROM_MEM(); break;
			case 0x2B:DEC(REGISTER_HL); break;
			case 0x2C:INC(REGISTER_L); break;
			case 0x2D:DEC(REGISTER_L); break;
			case 0x2E:LD(REGISTER_L); break;
			case 0x2F:CPL(); break;
			case 0x30:JR(check_condition(NC)); break;
			case 0x31:LD_16BIT(REGISTER_SP); break;
			case 0x32:LDD_TO_MEM(); break;
			case 0x33:INC(REGISTER_SP); break;
			case 0x34:INC_MEM(); break;
			case 0x35:DEC_MEM(); break;
			case 0x36:LD_TO_MEM(REGISTER_HL); break;
			case 0x37:SCF(); break;
			case 0x38:JR(check_condition(C)); break;
			case 0x39:ADD_HL(REGISTER_SP); break;
			case 0x3A:LDD_FROM_MEM(); break;
			case 0x3B:DEC(REGISTER_SP); break;
			case 0x3C:INC(REGISTER_A); break;
			case 0x3D:DEC(REGISTER_A); break;
			case 0x3E:LD(REGISTER_A); break;
			case 0x3F:CCF(); break;
			case 0x40:LD(REGISTER_B, REGISTER_B); break;
			case 0x41:LD(REGISTER_B, REGISTER_C); break;
			case 0x42:LD(REGISTER_B, REGISTER_D); break;
			case 0x43:LD(REGISTER_B, REGISTER_E); break;
			case 0x44:LD(REGISTER_B, REGISTER_H); break;
			case 0x45:LD(REGISTER_B, REGISTER_L); break;
			case 0x46:LD_FROM_MEM(REGISTER_B, REGISTER_HL); break;
			case 0x47:LD(REGISTER_B, REGISTER_A); break;
			case 0x48:LD(REGISTER_C, REGISTER_B); break;
			case 0x49:LD(REGISTER_C, REGISTER_C); break;
			case 0x4A:LD(REGISTER_C, REGISTER_D); break;
			case 0x4B:LD(REGISTER_C, REGISTER_E); break;
			case 0x4C:LD(REGISTER_C, REGISTER_H); break;
			case 0x4D:LD(REGISTER_C, REGISTER_L); break;
			case 0x4E:LD_FROM_MEM(REGISTER_C, REGISTER_HL); break;
			case 0x4F:LD(REGISTER_C, REGISTER_A); break;
			case 0x50:LD(REGISTER_D, REGISTER_B); break;
			case 0x51:LD(REGISTER_D, REGISTER_C); break;
			case 0x52:LD(REGISTER_D, REGISTER_D); break;
			case 0x53:LD(REGISTER_D, REGISTER_E); break;
			case 0x54:LD(REGISTER_D, REGISTER_H); break;
			case 0x55:LD(REGISTER_D, REGISTER_L); break;
			case 0x56:LD_FROM_MEM(REGISTER_D, REGISTER_HL); break;
			case 0x57:LD(REGISTER_D, REGISTER_A); break;
			case 0x58:LD(REGISTER_E, REGISTER_B); break;
			case 0x59:LD(REGISTER_E, REGISTER_C); break;
			case 0x5A:LD(REGISTER_E, REGISTER_D); break;
			case 0x5B:LD(REGISTER_E, REGISTER_E); break;
			case 0x5C:LD(REGISTER_E, REGISTER_H); break;
			case 0x5D:LD(REGISTER_E, REGISTER_L); break;
			case 0x5E:LD_FROM_MEM(REGISTER_E, REGISTER_HL); break;
			case 0x5F:LD(REGISTER_E, REGISTER_A); break;
			case 0x60:LD(REGISTER_H, REGISTER_B); break;
			case 0x61:LD(REGISTER_H, REGISTER_C); break;
			case 0x62:LD(REGISTER_H, REGISTER_D); break;
			case 0x63:LD(REGISTER_H, REGISTER_E); break;
			case 0x64:LD(REGISTER_H, REGISTER_H); break;
			case 0x65:LD(REGISTER_H, REGISTER_L); break;
			case 0x66:LD_FROM_MEM(REGISTER_H, REGISTER_HL); break;
			case 0x67:LD(REGISTER_H, REGISTER_A); break;
			case 0x68:LD(REGISTER_L, REGISTER_B); break;
			case 0x69:LD(REGISTER_L, REGISTER_C); break;
			case 0x6A:LD(REGISTER_L, REGISTER_D); break;
			case 0x6B:LD(REGISTER_L, REGISTER_E); break;
			case 0x6C:LD(REGISTER_L, REGISTER_H); break;
			case 0x6D:LD(REGISTER_L, REGISTER_L); break;
			case 0x6E:LD_FROM_MEM(REGISTER_L, REGISTER_HL); break;
			case 0x6F:LD(REGISTER_L, REGISTER_A); break;
			case 0x70:LD_TO_MEM(REGISTER_HL, REGISTER_B); break;
			case 0x71:LD_TO_MEM(REGISTER_HL, REGISTER_C); break;
			case 0x72:LD_TO_MEM(REGISTER_HL, REGISTER_D); break;
			case 0x73:LD_TO_MEM(REGISTER_HL, REGISTER_E); break;
			case 0x74:LD_TO_MEM(REGISTER_HL, REGISTER_H); break;
			case 0x75:LD_TO_MEM(REGISTER_HL, REGISTER_L); break;
			case 0x76:HALT(); break;
			case 0x77:LD_TO_MEM(REGISTER_HL, REGISTER_A); break;
			case 0x78:LD(REGISTER_A, REGISTER_B); break;
			case 0x79:LD(REGISTER_A, REGISTER_C); break;
			case 0x7A:LD(REGISTER_A, REGISTER_D); break;
			case 0x7B:LD(REGISTER_A, REGISTER_E); break;
			case 0x7C:LD(REGISTER_A, REGISTER_H); break;
			case 0x7D:LD(REGISTER_A, REGISTER_L); break;
			case 0x7E:LD_FROM_MEM(REGISTER_A, REGISTER_HL); break;
			case 0x7F:LD(REGISTER_A, REGISTER_A); break;
			case 0x80:ADD(REGISTER_B); break;
			case 0x81:ADD(REGISTER_C); break;
			case 0x82:ADD(REGISTER_D); break;
			case 0x83:ADD(REGISTER_E); break;
			case 0x84:ADD(REGISTER_H); break;
			case 0x85:ADD(REGISTER_L); break;
			case 0x86:ADD_MEM(); break;
			case 0x87:ADD(REGISTER_A); break;
			case 0x88:ADC(REGISTER_B); break;
			case 0x89:ADC(REGISTER_C); break;
			case 0x8A:ADC(REGISTER_D); break;
			case 0x8B:ADC(REGISTER_E); break;
			case 0x8C:ADC(REGISTER_H); break;
			case 0x8D:ADC(REGISTER_L); break;
			case 0x8E:ADC_MEM(); break;
			case 0x8F:ADC(REGISTER_A); break;
			case 0x90:SUB(REGISTER_B); break;
			case 0x91:SUB(REGISTER_C); break;
			case 0x92:SUB(REGISTER_D); break;
			case 0x93:SUB(REGISTER_E); break;
			case 0x94:SUB(REGISTER_H); break;
			case 0x95:SUB(REGISTER_L); break;
			case 0x96:SUB_MEM(); break;
			case 0x97:SUB(REGISTER_A); break;
			case 0x98:SBC(REGISTER_B); break;
			case 0x99:SBC(REGISTER_C); break;
			case 0x9A:SBC(REGISTER_D); break;
			case 0x9B:SBC(REGISTER_E); break;
			case 0x9C:SBC(REGISTER_H); break;
			case 0x9D:SBC(REGISTER_L); break;
			case 0x9E:SBC_MEM(); break;
			case 0x9F:SBC(REGISTER_A); break;
			case 0xA0:AND(REGISTER_B); break;
			case 0xA1:AND(REGISTER_C); break;
			case 0xA2:AND(REGISTER_D); break;
			case 0xA3:AND(REGISTER_E); break;
			case 0xA4:AND(REGISTER_H); break;
			case 0xA5:AND(REGISTER_L); break;
			case 0xA6:AND_MEM(); break;
			case 0xA7:AND(REGISTER_A); break;
			case 0xA8:XOR(REGISTER_B); break;
			case 0xA9:XOR(REGISTER_C); break;
			case 0xAA:XOR(REGISTER_D); break;
			case 0xAB:XOR(REGISTER_E); break;
			case 0xAC:XOR(REGISTER_H); break;
			case 0xAD:XOR(REGISTER_L); break;
			case 0xAE:XOR_MEM(); break;
			case 0xAF:XOR(REGISTER_A); break;
			case 0xB0:OR(REGISTER_B); break;
			case 0xB1:OR(REGISTER_C); break;
			case 0xB2:OR(REGISTER_D); break;
			case 0xB3:OR(REGISTER_E); break;
			case 0xB4:OR(REGISTER_H); break;
			case 0xB5:OR(REGISTER_L); break;
			case 0xB6:OR_MEM(); break;
			case 0xB7:OR(REGISTER_A); break;
			case 0xB8:CP(REGISTER_B); break;
			case 0xB9:CP(REGISTER_C); break;
			case 0xBA:CP(REGISTER_D); break;
			case 0xBB:CP(REGISTER_E); break;
			case 0xBC:CP(REGISTER_H); break;
			case 0xBD:CP(REGISTER_L); break;
			case 0xBE:CP_MEM(); break;
			case 0xBF:CP(REGISTER_A); break;
			case 0xC0:RET(check_condition(NZ)); break;
			case 0xC1:POP(REGISTER_BC); break;
			case 0xC2:JP(check_condition(NZ)); break;
			case 0xC3:JP(); break;
			case 0xC4:CALL(check_condition(NZ)); break;
			case 0xC5:PUSH(REGISTER_BC); break;
			case 0xC6:ADD(); break;
			case 0xC7:RST(0); break;
			case 0xC8:RET(check_condition(Z)); break;
			case 0xC9:RET(); break;
			case 0xCA:JP(check_condition(Z)); break;
			case 0xCB:break;
			case 0xCC:CALL(check_condition(Z)); break;
			case 0xCD:CALL(); break;
			case 0xCE:ADC(); break;
			case 0xCF:RST(1); break;
			case 0xD0:RET(check_condition(NC)); break;
			case 0xD1:POP(REGISTER_DE); break;
			case 0xD2:JP(check_condition(NC)); break;
			case 0xD3:break; //ILLEGAL INSTRUCTION
			case 0xD4:CALL(check_condition(NC)); break;
			case 0xD5:PUSH(REGISTER_DE); break;
			case 0xD6:SUB(); break;
			case 0xD7:RST(2); break;
			case 0xD8:RET(check_condition(C)); break;
			case 0xD9:RETI(); break;
			case 0xDA:JP(check_condition(C)); break;
			case 0xDB:break; //ILLEGAL INSTRUCTION
			case 0xDC:CALL(check_condition(C)); break;
			case 0xDD:break; //ILLEGAL INSTRUCTION
			case 0xDE:SBC(); break;
			case 0xDF:RST(3); break;
			case 0xE0:LDH_TO_MEM(REGISTER_A); break;
			case 0xE1:POP(REGISTER_HL); break;
			case 0xE2:LDH_TO_MEM(REGISTER_C, REGISTER_A); break;
			case 0xE3:break; //ILLEGAL INSTRUCTION
			case 0xE4:break; //ILLEGAL INSTRUCTION
			case 0xE5:PUSH(REGISTER_HL); break;
			case 0xE6:AND(); break;
			case 0xE7:RST(4); break;
			case 0xE8:ADD_SP(0); break;
			case 0xE9:JP(REGISTER_HL); break;
			case 0xEA:LD_TO_MEM(REGISTER_A); break;
			case 0xEB:break; //ILLEGAL INSTRUCTION
			case 0xEC:break; //ILLEGAL INSTRUCTION
			case 0xED:break; //ILLEGAL INSTRUCTION
			case 0xEE:XOR(); break;
			case 0xEF:RST(5); break;
			case 0xF0:LDH_FROM_MEM(REGISTER_A); break;
			case 0xF1:POP(REGISTER_AF); break;
			case 0xF2:LDH_FROM_MEM(REGISTER_A, REGISTER_C); break;
			case 0xF3:DI(); break;
			case 0xF4:break; //ILLEGAL INSTRUCTION
			case 0xF5:PUSH(REGISTER_AF); break;
			case 0xF6:OR(); break;
			case 0xF7:RST(6); break;
			case 0xF8:LD_SP_WITH_ADD(); break;
			case 0xF9:LD_16BIT(REGISTER_SP, REGISTER_HL); break;
			case 0xFA:LD_FROM_MEM(REGISTER_A); break;
			case 0xFB:EI(); break;
			case 0xFC:break; //ILLEGAL INSTRUCTION
			case 0xFD:break; //ILLEGAL INSTRUCTION
			case 0xFE:CP(); break;
			case 0xFF:RST(7); break;
			default:
				break;
			}
		}	
	}
}

//registry related helper functions

uint8_t CPU::get_reg_high(uint16_t* reg)
{
	return *reg >> 8;
}

uint8_t CPU::get_reg_low(uint16_t* reg)
{
	return *reg & 0x00FF;
}

void CPU::override_reg_high(uint16_t* reg, uint8_t val)
{
	uint16_t val16 = (val << 8);
	*reg = (*reg & 0x00FF) | val16;
}

void CPU::override_reg_low(uint16_t* reg, uint8_t val)
{
	uint16_t val16 = val;
	*reg = (*reg & 0xFF00) | val16;
}

uint8_t CPU::get_8bit_register(Register_8Bit reg)
{
	switch (reg)
	{
	case REGISTER_A:
		return get_reg_high(&AF);
	case REGISTER_F:
		return get_reg_low(&AF);
	case REGISTER_B:
		return get_reg_high(&BC);
	case REGISTER_C:
		return get_reg_low(&BC);
	case REGISTER_D:
		return get_reg_high(&DE);
	case REGISTER_E:
		return get_reg_low(&DE);
	case REGISTER_H:
		return get_reg_high(&HL);
	case REGISTER_L:
		return get_reg_low(&HL);
	default:
		throw std::invalid_argument("Invalid 8bit register");
	}
}

uint16_t CPU::get_16bit_register(Register_16Bit reg)
{
	switch (reg)
	{
	case REGISTER_AF:
		return AF;
	case REGISTER_BC:
		return BC;
	case REGISTER_DE:
		return DE;
	case REGISTER_HL:
		return HL;
	case REGISTER_SP:
		return SP;
	case REGISTER_PC:
		return PC;
	default:
		throw std::invalid_argument("Invalid 8bit register");
	}
}

void CPU::set_8bit_register(Register_8Bit reg, uint8_t val)
{
	switch (reg)
	{
	case REGISTER_A:
		override_reg_high(&AF, val); break;
	case REGISTER_F:
		val = (val & 0xF0);
		override_reg_low(&AF, val); break;
	case REGISTER_B:
		override_reg_high(&BC, val); break;
	case REGISTER_C:
		override_reg_low(&BC, val); break;
	case REGISTER_D:
		override_reg_high(&DE, val); break;
	case REGISTER_E:
		override_reg_low(&DE, val); break;
	case REGISTER_H:
		override_reg_high(&HL, val); break;
	case REGISTER_L:
		override_reg_low(&HL, val); break;
	default:
		throw std::invalid_argument("Invalid 8bit register");
	}
}

void CPU::set_16bit_register(Register_16Bit reg, uint16_t val)
{
	switch (reg)
	{
	case REGISTER_AF:
		val = (val & 0xFFF0);
		AF = val; break;
	case REGISTER_BC:
		BC = val; break;
	case REGISTER_DE:
		DE = val; break;
	case REGISTER_HL:
		HL = val; break;
	case REGISTER_SP:
		SP = val; break;
	case REGISTER_PC:
		PC = val; break;
	default:
		throw std::invalid_argument("Invalid 16bit register");
	}
}

void CPU::increment_PC()
{
	PC++;
}

//flags related helper functions

void CPU::set_flag_values(bool z, bool n, bool h, bool c) {
	uint8_t flags = 0;
	if (z)
		flags = flags | 1;
	flags = (flags << 1);
	if (n)
		flags = flags | 1;
	flags = (flags << 1);
	if (h)
		flags = flags | 1;
	flags = (flags << 1);
	if (c)
		flags = flags | 1;
	flags = (flags << 4);
	set_8bit_register(REGISTER_F, flags);
}

bool CPU::get_flag(FlagBit bit) {
	if ((AF & (1 << bit)) != 0)
		return true;
	else
		return false;
	return false;
}

bool CPU::check_condition(Condition_Code con) {
	switch (con)
	{
	case Z:
		return get_flag(FLAG_Z);
		break;
	case NZ:
		return !get_flag(FLAG_Z);
		break;
	case C:
		return get_flag(FLAG_C);
	case NC:
		return !get_flag(FLAG_C);
		break;
	default:
		break;
	}
	return false;
}

//values related functions

uint8_t CPU::get_immediate_8bit_value()
{
	uint8_t value = MEM->getMemValue(PC);
	if (halt_bug)
	{
		halt_bug = false;
	}
	else 
	{
		increment_PC();
	}
	return value;
}

uint16_t CPU::get_immediate_16bit_value()
{
	uint16_t value = uint16_t{ MEM->getMemValue(PC) };
	increment_PC();
	value += (uint16_t( MEM->getMemValue(PC) ) << 8);
	increment_PC();
	return value;
}

//interrupt handling functions

void CPU::check_and_initiate_interrupt()
{
	uint8_t IE = MEM->getMemValue(0xFFFF);
	uint8_t IF = MEM->getMemValue(0xFF0F);

	if (!IME)
	{
		if ((IE & IF) != 0)
		{
			halt = false;
			MEM->setCPUHalted(false);
		}
		else halt_bug = false;
		return;
	}

	halt_bug = false;

	if ((IE & IF) != 0)
	{
		interrupt_in_progress = true;
		busyCycles = 5;
		return;
	}

	
}

void CPU::handle_interrupt()
{
	static uint8_t IE;
	static uint8_t IF;
	static uint16_t val;
	uint8_t mask = 0x01;
	//printf("busy cycles: %d\n", busyCycles);
	switch (busyCycles)
	{
	case 2:
		val = get_16bit_register(REGISTER_PC);
		IF = MEM->getMemValue(0xFF0F);
		SP--;
		MEM->setMemValue(SP, val >> 8);
		return;
	case 1:
		IE = MEM->getMemValue(0xFFFF);
		SP--;
		MEM->setMemValue(SP, val & 0xFF);
		return;
	case 0:
		interrupt_in_progress = false;
		halt = false;
		IME = false;
		MEM->setCPUHalted(false);
		if ((IE & mask) != 0 and (IF & mask) != 0) //VBlank interrupt
		{
			// printf("handling VBLANK interrupt\n");
			MEM->discardInterrupt(VBLANK_INTERRUPT);
			current_interrupt = VBLANK_INTERRUPT;
			PC = 0x0040 + (0x0008 * (uint8_t)VBLANK_INTERRUPT);
			return;
		}

		mask = mask << 1;

		if ((IE & mask) != 0 and (IF & mask) != 0) //STAT interrupt
		{
			// printf("handling STAT interrupt\n");
			MEM->discardInterrupt(STAT_INTERRUPT);
			current_interrupt = STAT_INTERRUPT;
			PC = 0x0040 + (0x0008 * (uint8_t)STAT_INTERRUPT);
			return;
		}

		mask = mask << 1;

		if ((IE & mask) != 0 and (IF & mask) != 0) //Timer interrupt
		{
			// printf("handling TIMER interrupt\n");
			MEM->discardInterrupt(TIMER_INTERRUPT);
			current_interrupt = TIMER_INTERRUPT;
			PC = 0x0040 + (0x0008 * (uint8_t)TIMER_INTERRUPT);
			return;
		}

		mask = mask << 1;

		if ((IE & mask) != 0 and (IF & mask) != 0) //Serial interrupt
		{
			MEM->discardInterrupt(SERIAL_INTERRUPT);
			current_interrupt = SERIAL_INTERRUPT;
			PC = 0x0040 + (0x0008 * (uint8_t)SERIAL_INTERRUPT);
			return;
		}

		mask = mask << 1;

		if ((IE & mask) != 0 and (IF & mask) != 0) //Joypad interrupt
		{
			MEM->discardInterrupt(JOYPAD_INTERRUPT);
			current_interrupt = JOYPAD_INTERRUPT;
			PC = 0x0040 + (0x0008 * (uint8_t)JOYPAD_INTERRUPT);
			return;
		}


		PC = 0x0000;
		
		return;
	}
	
	
}

//instructions implementations

void CPU::ADC(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		uint8_t before = get_8bit_register(REGISTER_A);
		uint8_t before_lower = before & 0x0F;
		uint8_t val = get_8bit_register(reg);
		uint8_t val_lower = val & 0x0F;
		uint16_t added = before + val;
		uint8_t added_lower = before_lower + val_lower;


		if (get_flag(FLAG_C))
		{
			added++;
			added_lower++;
		}

		bool zero_flag = (added % 0x100) == 0;
		bool half_carry_flag = added_lower > 0x0F;
		bool carry_flag = added > 0xFF;

		set_flag_values(zero_flag, false, half_carry_flag, carry_flag);

		set_8bit_register(REGISTER_A, added & 0xFF);
		break;
	}
}

void CPU::ADC(Register_8Bit target, uint8_t val)
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_immediate_8bit_value();
		break;
	case 0:
		uint8_t before = get_8bit_register(target);
		uint8_t before_lower = before & 0x0F;
		uint8_t val_lower = val8 & 0x0F;
		uint16_t added = before + val8;
		uint8_t added_lower = before_lower + val_lower;


		if (get_flag(FLAG_C))
		{
			added++;
			added_lower++;
		}

		bool zero_flag = (added % 0x100) == 0;
		bool half_carry_flag = added_lower > 0x0F;
		bool carry_flag = added > 0xFF;

		set_flag_values(zero_flag, false, half_carry_flag, carry_flag);

		set_8bit_register(target, added & 0xFF);
		break;
	}
}

void CPU::ADC()
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_immediate_8bit_value();
		break;
	case 0:
		uint8_t before = get_8bit_register(REGISTER_A);
		uint8_t before_lower = before & 0x0F;
		uint8_t val_lower = val8 & 0x0F;
		uint16_t added = before + val8;
		uint8_t added_lower = before_lower + val_lower;


		if (get_flag(FLAG_C))
		{
			added++;
			added_lower++;
		}


		bool zero_flag = ((added % 0x100) == 0);
		bool half_carry_flag = added_lower > 0x0F;
		bool carry_flag = added > 0xFF;

		set_flag_values(zero_flag, false, half_carry_flag, carry_flag);

		set_8bit_register(REGISTER_A, added & 0xFF);
		break;
	}
}

void CPU::ADC_MEM()
{
	switch (busyCycles)
	{
	case 1:
		val8 = MEM->getMemValue(get_16bit_register(REGISTER_HL));
		break;
	case 0:
		uint8_t before = get_8bit_register(REGISTER_A);
		uint8_t before_lower = before & 0x0F;
		uint8_t val_lower = val8 & 0x0F;
		uint16_t added = before + val8;
		uint8_t added_lower = before_lower + val_lower;


		if (get_flag(FLAG_C))
		{
			added++;
			added_lower++;
		}

		bool zero_flag = ((added % 0x100) == 0);
		bool half_carry_flag = added_lower > 0x0F;
		bool carry_flag = added > 0xFF;

		set_flag_values(zero_flag, false, half_carry_flag, carry_flag);

		set_8bit_register(REGISTER_A, added & 0xFF);
		break;
	}
}

void CPU::ADD(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(reg);
		uint8_t before = get_8bit_register(REGISTER_A);
		uint8_t before_lower = before & 0x0F;
		uint8_t val_lower = val8 & 0x0F;
		uint16_t added = before + val8;
		uint8_t added_lower = before_lower + val_lower;

		bool zero_flag = added % 0x100 == 0;
		bool half_carry_flag = added_lower > 0x0F;
		bool carry_flag = added > 0xFF;

		set_flag_values(zero_flag, false, half_carry_flag, carry_flag);

		set_8bit_register(REGISTER_A, added);
		break;
	}
}

void CPU::ADD(Register_8Bit target, uint8_t val)
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_immediate_8bit_value();
		break;
	case 0:
		uint8_t before = get_8bit_register(target);
		uint8_t before_lower = before & 0x0F;
		uint8_t val_lower = val8 & 0x0F;
		uint16_t added = before + val8;
		uint8_t added_lower = before_lower + val_lower;

		bool zero_flag = added % 0x100 == 0;
		bool half_carry_flag = added_lower > 0x0F;
		bool carry_flag = added > 0xFF;

		set_flag_values(zero_flag, false, half_carry_flag, carry_flag);

		set_8bit_register(target, added);
		break;
	}
}

void CPU::ADD()
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_immediate_8bit_value();
		break;
	case 0:
		uint8_t before = get_8bit_register(REGISTER_A);
		uint8_t before_lower = before & 0x0F;
		uint8_t val_lower = val8 & 0x0F;
		uint16_t added = before + val8;
		uint8_t added_lower = before_lower + val_lower;

		bool zero_flag = added % 0x100 == 0;
		bool half_carry_flag = added_lower > 0x0F;
		bool carry_flag = added > 0xFF;

		set_flag_values(zero_flag, false, half_carry_flag, carry_flag);

		set_8bit_register(REGISTER_A, added);
		break;
	}
	
}

void CPU::ADD_MEM()
{
	switch (busyCycles)
	{
	case 1:
		val8 = MEM->getMemValue(get_16bit_register(REGISTER_HL));
		break;
	case 0:
		uint8_t before = get_8bit_register(REGISTER_A);
		uint8_t before_lower = before & 0x0F;
		uint8_t val_lower = val8 & 0x0F;
		uint16_t added = before + val8;
		uint8_t added_lower = before_lower + val_lower;

		bool zero_flag = added % 0x100 == 0;
		bool half_carry_flag = added_lower > 0x0F;
		bool carry_flag = added > 0xFF;

		set_flag_values(zero_flag, false, half_carry_flag, carry_flag);

		set_8bit_register(REGISTER_A, added);
		break;
	}
}

void CPU::ADD_HL(Register_16Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		uint16_t before = HL;
		uint16_t before_lower = before & 0x0FFF;
		uint16_t val = get_16bit_register(reg);
		uint16_t val_lower = val & 0x0FFF;
		uint32_t added = before + val;
		uint16_t added_lower = before_lower + val_lower;

		bool half_carry_flag = added_lower > 0x0FFF;
		bool carry_flag = added > 0xFFFF;

		set_flag_values(get_flag(FLAG_Z), false, half_carry_flag, carry_flag);

		HL = added;
		break;
	}
	
}

void CPU::ADD_SP(uint8_t val)
{
	switch (busyCycles)
	{
	case 2:
		val8 = get_immediate_8bit_value();
		break;
	case 0:
		int8_t sign_val = static_cast<int8_t> (val8);
		uint16_t before = get_16bit_register(REGISTER_SP);
		int added = before + sign_val;

		uint16_t xored = before ^ sign_val ^ (added & 0xFFFF);

		bool half_carry_flag = (xored & 0x0010) == 0x0010;
		bool carry_flag = (xored & 0x0100) == 0x0100;

		set_flag_values(false, false, half_carry_flag, carry_flag);

		set_16bit_register(REGISTER_SP, static_cast<uint16_t>(added));
		break;
	}
}

void CPU::AND(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(REGISTER_A) & get_8bit_register(reg);

		bool zero_flag = val8 == 0x00;

		set_flag_values(zero_flag, false, true, false);

		set_8bit_register(REGISTER_A, val8);
		break;
	}
}

void CPU::AND()
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_8bit_register(REGISTER_A) & get_immediate_8bit_value();
		break;
	case 0:

		bool zero_flag = val8 == 0x00;

		set_flag_values(zero_flag, false, true, false);

		set_8bit_register(REGISTER_A, val8);
		break;
	}
}

void CPU::AND_MEM()
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_8bit_register(REGISTER_A) & MEM->getMemValue(get_16bit_register(REGISTER_HL));
		break;
	case 0:

		bool zero_flag = val8 == 0x00;

		set_flag_values(zero_flag, false, true, false);

		set_8bit_register(REGISTER_A, val8);
		break;
	}
	
}

void CPU::BIT(uint8_t bit, Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		bool bit_is_zero = (get_8bit_register(reg) & (0x01 << bit)) == 0;
		set_flag_values(bit_is_zero, false, true, get_flag(FLAG_C));
		break;
	}
}

void CPU::BIT_MEM(uint8_t bit)
{
	switch (busyCycles)
	{
	case 1:
		val8 = MEM->getMemValue(get_16bit_register(REGISTER_HL));
		break;
	case 0:
		bool bit_is_zero = (val8 & (0x01 << bit)) == 0;
		set_flag_values(bit_is_zero, false, true, get_flag(FLAG_C));
		break;
	}
}

void CPU::CALL(bool condition)
{
	switch (busyCycles)
	{
	case 4:
		val16 = get_immediate_8bit_value();
		break;
	case 3:
		val16 += (uint16_t(get_immediate_8bit_value()) << 8);
		if (!condition) busyCycles = 0;
		break;
	case 2:
		break;
	case 1:
		MEM->setMemValue(--SP, get_16bit_register(REGISTER_PC) >> 8);
		break;
	case 0:
		MEM->setMemValue(--SP, get_16bit_register(REGISTER_PC) & 0xFF);
		set_16bit_register(REGISTER_PC, val16);
		break;
	}
}

void CPU::CCF()
{
	switch (busyCycles)
	{
	case 0:
		set_flag_values(get_flag(FLAG_Z), false, false, !get_flag(FLAG_C));
		break;
	}
}

void CPU::CP(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		uint8_t a = get_8bit_register(REGISTER_A);
		val8 = get_8bit_register(reg);

		set_flag_values(a == val8, true, (a & 0x0F) < (val8 & 0x0F), a < val8);
		break;
	}
	
}

void CPU::CP()
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_immediate_8bit_value();
		break;
	case 0:
		uint8_t a = get_8bit_register(REGISTER_A);

		set_flag_values(a == val8, true, (a & 0x0F) < (val8 & 0x0F), a < val8);
		break;
	}
	
}

void CPU::CP_MEM()
{
	switch (busyCycles)
	{
	case 1:
		val8 = MEM->getMemValue(get_16bit_register(REGISTER_HL));
		break;
	case 0:
		uint8_t a = get_8bit_register(REGISTER_A);
		set_flag_values(a == val8, true, (a & 0x0F) < (val8 & 0x0F), a < val8);
		break;
	}
	
}

void CPU::CPL()
{
	switch (busyCycles)
	{
	case 0:
		set_8bit_register(REGISTER_A, ~get_8bit_register(REGISTER_A));

		set_flag_values(get_flag(FLAG_Z), true, true, get_flag(FLAG_C));
		break;
	}
	
}

void CPU::DAA()
{
	switch (busyCycles)
	{
	case 0:
		uint8_t adj = 0x00;
		uint8_t a = get_8bit_register(REGISTER_A);
		bool carry_flag = false;
		bool n_flag = get_flag(FLAG_N);
		if (n_flag)
		{
			carry_flag = get_flag(FLAG_C);
			adj += get_flag(FLAG_H) * 0x06 + carry_flag * 0x60;
			a -= adj;
		}
		else
		{
			carry_flag = (get_flag(FLAG_C) or a > 0x99);
			adj += (get_flag(FLAG_H) or (a & 0x0F) > 0x09) * 0x06 + carry_flag * 0x60;
			a += adj;
		}

		set_8bit_register(REGISTER_A, a);

		set_flag_values(a == 0, n_flag, false, carry_flag);
		break;
	}
	
}

void CPU::DEC(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(reg) - 1;
		set_8bit_register(reg, val8);

		set_flag_values(val8 == 0, true, ((val8 & 0x0F) == 0x0F), get_flag(FLAG_C));
		break;
	}
	
}

void CPU::DEC(Register_16Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		set_16bit_register(reg, get_16bit_register(reg) - 1);
		break;
	}
}

void CPU::DEC_MEM()
{
	switch (busyCycles)
	{
	case 2:
		val16 = get_16bit_register(REGISTER_HL);
		val8 = MEM->getMemValue(val16) - 1;
		break;
	case 1:
		
		MEM->setMemValue(val16, val8);

		set_flag_values(val8 == 0, true, ((val8 & 0x0F) == 0x0F), get_flag(FLAG_C));
		break;
	}
	
}

void CPU::DI()
{
	switch (busyCycles)
	{
	case 0:
		IME = false;
		EI_pending = false;
		break;
	}
}

void CPU::EI()
{
	switch (busyCycles)
	{
	case 0:
		EI_called = true;
		break;
	}
}

void CPU::HALT()
{
	switch (busyCycles)
	{
	case 0:
		halt = true;
		MEM->setCPUHalted(true);
		halt_bug = true;
		break;
	}
}

void CPU::INC(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(reg) + 1;
		set_8bit_register(reg, val8);

		set_flag_values(val8 == 0, false, ((val8 & 0x0F) == 0x00), get_flag(FLAG_C));
		break;
	}
	
}

void CPU::INC(Register_16Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		set_16bit_register(reg, get_16bit_register(reg) + 1);
		break;
	}
}

void CPU::INC_MEM()
{
	switch (busyCycles)
	{
	case 2:
		val16 = get_16bit_register(REGISTER_HL);
		val8 = MEM->getMemValue(val16) + 1;
		break;
	case 1:
		MEM->setMemValue(val16, val8);

		set_flag_values(val8 == 0, false, ((val8 & 0x0F) == 0x00), get_flag(FLAG_C));
		break;
	}
	
}

void CPU::JP(bool condition)
{
	switch (busyCycles)
	{
	case 2:
		val16 = get_immediate_8bit_value();
		break;
	case 1:
		val16 += (uint16_t(get_immediate_8bit_value()) << 8);
		if (!condition) busyCycles = 0;
		break;
	case 0:
		set_16bit_register(REGISTER_PC, val16);
		break;
	}
}

void CPU::JP(Register_16Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		set_16bit_register(REGISTER_PC, get_16bit_register(reg));
		break;
	}
}

void CPU::JR(bool condition)
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_immediate_8bit_value();
		if (!condition) busyCycles = 0;
		break;
	case 0:
		set_16bit_register(REGISTER_PC, uint16_t(int16_t(get_16bit_register(REGISTER_PC)) + int8_t(val8)));
		break;
	}
}

void CPU::LD(Register_8Bit to, Register_8Bit from)
{
	switch (busyCycles)
	{
	case 0:
		set_8bit_register(to, get_8bit_register(from));
		break;
	}
}

void CPU::LD(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_immediate_8bit_value();
		break;
	case 0:
		set_8bit_register(reg, val8);
		break;
	}
}

void CPU::LD_TO_MEM(Register_16Bit address, Register_8Bit from)
{
	switch (busyCycles)
	{
	case 1:
		MEM->setMemValue(get_16bit_register(address), get_8bit_register(from));
		break;
	}
}

void CPU::LD_TO_MEM(Register_16Bit address)
{
	switch (busyCycles)
	{
	case 2:
		val8 = get_immediate_8bit_value();
		break;
	case 1:
		MEM->setMemValue(get_16bit_register(address), val8);
		break;
	}
	
}

void CPU::LD_TO_MEM(Register_8Bit from)
{
	switch (busyCycles)
	{
	case 3:
		val16 = get_immediate_8bit_value();
		break;
	case 2:
		val16 += (uint16_t(get_immediate_8bit_value()) << 8);
		break;
	case 1:
		MEM->setMemValue(val16, get_8bit_register(from));
		break;
	case 0:
		break;
	}
}

void CPU::LD_FROM_MEM(Register_8Bit reg, Register_16Bit address)
{
	switch (busyCycles)
	{
	case 1:
		val8 = MEM->getMemValue(get_16bit_register(address));
		break;
	case 0:
		set_8bit_register(reg, val8);
		break;
	}
}

void CPU::LD_FROM_MEM(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 3:
		val16 = get_immediate_8bit_value();
		break;
	case 2:
		val16 += (uint16_t(get_immediate_8bit_value()) << 8);
		break;
	case 1:
		val8 = MEM->getMemValue(val16);
		break;
	case 0:
		set_8bit_register(reg, val8);
		break;
	}
	
}

void CPU::LDH_TO_MEM(Register_8Bit address, Register_8Bit from)
{
	switch (busyCycles)
	{
	case 1:
		MEM->setMemValue(get_8bit_register(address) + 0xFF00, get_8bit_register(from));
		break;
	case 0:
		break;
	}
}

void CPU::LDH_TO_MEM(Register_8Bit from)
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_immediate_8bit_value();
		MEM->setMemValue(val8 + 0xFF00, get_8bit_register(from));
		break;
	case 0:
		break;
	}
}

void CPU::LDH_FROM_MEM(Register_8Bit reg, Register_8Bit address)
{
	switch (busyCycles)
	{
	case 1:
		val8 = MEM->getMemValue(get_8bit_register(address) + 0xFF00);
		break;
	case 0:
		set_8bit_register(reg, val8);
		break;
	}
}

void CPU::LDH_FROM_MEM(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 2:
		val8 = get_immediate_8bit_value();
		break;
	case 1:
		val8 = MEM->getMemValue(val8 + 0xFF00);
		break;
	case 0:
		set_8bit_register(reg, val8);
		break;
	}
}

void CPU::LDI_TO_MEM()
{
	switch (busyCycles)
	{
	case 1:
		MEM->setMemValue(get_16bit_register(REGISTER_HL), get_8bit_register(REGISTER_A));
		break;
	case 0:
		HL++;
		break;
	}
}

void CPU::LDD_TO_MEM()
{
	switch (busyCycles)
	{
	case 1:
		MEM->setMemValue(get_16bit_register(REGISTER_HL), get_8bit_register(REGISTER_A));
		break;
	case 0:
		HL--;
		break;
	}
}

void CPU::LDI_FROM_MEM()
{
	switch (busyCycles)
	{
	case 1:
		val8 = MEM->getMemValue(get_16bit_register(REGISTER_HL));
		HL++;
		break;
	case 0:
		set_8bit_register(REGISTER_A, val8);
		break;
	}
}

void CPU::LDD_FROM_MEM()
{
	switch (busyCycles)
	{
	case 1:
		val8 = MEM->getMemValue(get_16bit_register(REGISTER_HL));
		HL--;
		break;
	case 0:
		set_8bit_register(REGISTER_A, val8);
		break;
	}
}

void CPU::LD_16BIT(Register_16Bit reg)
{
	switch (busyCycles)
	{
	case 1:
		val16 = get_immediate_16bit_value();
		break;
	case 0:
		set_16bit_register(reg, val16);
		break;
	}
}

void CPU::LD_16BIT(Register_16Bit to, Register_16Bit from)
{
	switch (busyCycles)
	{
	case 0:
		set_16bit_register(to, get_16bit_register(from));
		break;
	}
}

void CPU::LD_16BIT_TO_MEM()
{
	switch (busyCycles)
	{
	case 1:
		val16 = get_immediate_16bit_value();
		MEM->setMemValue(val16, get_16bit_register(REGISTER_SP) & 0xFF);
		break;
	case 0:
		MEM->setMemValue(val16 + 1, get_16bit_register(REGISTER_SP) >> 8);
		break;
	}
}

void CPU::LD_SP_WITH_ADD()
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_immediate_8bit_value();
		break;
	case 0:
		val16 = get_16bit_register(REGISTER_SP);

		uint16_t old_SP = val16;

		uint8_t SP_low = val16 & 0xFF;

		bool val_neg = (val8 & 0x80) != 0;
		if (val_neg)
			val8 += 0xFF00;

		val16 += (int8_t)val8;

		uint16_t test_val = (uint16_t)(val8 & 0x00FF) + (uint16_t)SP_low;

		bool carry_flag = (test_val > 0xFF);

		test_val = (val8 & 0x000F) + (SP_low & 0x000F);


		bool half_carry_flag = (test_val > 0x0F);

		set_16bit_register(REGISTER_HL, uint16_t(val16));
		set_flag_values(false, false, half_carry_flag, carry_flag);
		break;
	}
}

void CPU::OR(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(REGISTER_A) | get_8bit_register(reg);

		bool zero_flag = val8 == 0x00;

		set_flag_values(zero_flag, false, false, false);

		set_8bit_register(REGISTER_A, val8);
		break;
	}
}

void CPU::OR()
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_8bit_register(REGISTER_A) | get_immediate_8bit_value();
		break;
	case 0:
		bool zero_flag = val8 == 0x00;

		set_flag_values(zero_flag, false, false, false);

		set_8bit_register(REGISTER_A, val8);
		break;
	}
}

void CPU::OR_MEM()
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_8bit_register(REGISTER_A) | MEM->getMemValue(HL);
		break;
	case 0:
		bool zero_flag = val8 == 0x00;

		set_flag_values(zero_flag, false, false, false);

		set_8bit_register(REGISTER_A, val8);
		break;
	}
}

void CPU::POP(Register_16Bit reg)
{
	switch (busyCycles)
	{
	case 1:
		val16 = MEM->getMemValue(SP++);
		break;
	case 0:
		val16 += (uint16_t(MEM->getMemValue(SP++)) << 8);
		set_16bit_register(reg, val16);
		break;
	}
}

void CPU::PUSH(Register_16Bit reg)
{
	switch (busyCycles)
	{
	case 1:
		val16 = get_16bit_register(reg);
		if (reg == REGISTER_AF)
		{
			val16 = val16 & 0xFFF0;
		}
		SP--;
		MEM->setMemValue(SP, val16 >> 8);
		break;
	case 0:
		SP--;
		MEM->setMemValue(SP, val16 & 0xFF);
		break;
	}
}

void CPU::RES(uint8_t bit, Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_8bit_register(reg) & ~(0x01 << bit);
		break;
	case 0:
		set_8bit_register(reg, val8);
		break;
	}
}

void CPU::RES_MEM(uint8_t bit)
{
	switch (busyCycles)
	{
	case 2:
		val8 = MEM->getMemValue(HL) & ~(0x01 << bit);
		break;
	case 1:
		MEM->setMemValue(HL, val8);
		break;
	}
}

void CPU::RET(bool condition)
{
	switch (busyCycles)
	{
	case 3:
		if (!condition) busyCycles = 0;
		break;
	case 2:
		if (condition) val16 = MEM->getMemValue(SP++);;
		break;
	case 1:
		if (condition) val16 += (uint16_t(MEM->getMemValue(SP++)) << 8);
		set_16bit_register(REGISTER_PC, val16);
		break;
	}
}

void CPU::RETI()
{
	switch (busyCycles)
	{
	case 1:
		POP(REGISTER_PC);
		break;
	case 0:
		IME = true;
		POP(REGISTER_PC);
		break;
	}
}

void CPU::RL(Register_8Bit reg, bool ignore_zero_flag)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(reg);
		bool carry_flag = ((val8 & 0x80) != 0);
		val8 = ((val8 << 1) + get_flag(FLAG_C));
		set_8bit_register(reg, val8);
		set_flag_values((val8 == 0) and !ignore_zero_flag, false, false, carry_flag);
		break;
	}
}

void CPU::RL_MEM()
{
	switch (busyCycles)
	{
	case 2:
		val8 = MEM->getMemValue(HL);
		break;
	case 1:
		bool carry_flag = ((val8 & 0x80) != 0);
		val8 = ((val8 << 1) + get_flag(FLAG_C));
		MEM->setMemValue(HL, val8);
		set_flag_values(val8 == 0, false, false, carry_flag);
		break;
	}
}

void CPU::RLC(Register_8Bit reg, bool ignore_zero_flag)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(reg);
		bool carry_flag = ((val8 & 0x80) != 0);
		val8 = ((val8 << 1) + carry_flag);
		set_8bit_register(reg, val8);
		set_flag_values((val8 == 0) and !ignore_zero_flag, false, false, carry_flag);
		break;
	}
}

void CPU::RLC_MEM()
{
	switch (busyCycles)
	{
	case 2:
		val8 = MEM->getMemValue(HL);
		break;
	case 1:
		bool carry_flag = ((val8 & 0x80) != 0);
		val8 = ((val8 << 1) + carry_flag);
		MEM->setMemValue(HL, val8);
		set_flag_values(val8 == 0, false, false, carry_flag);
		break;
	}	
}

void CPU::RR(Register_8Bit reg, bool ignore_zero_flag)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(reg);
		bool carry_flag = ((val8 & 0x01) != 0);
		val8 = ((val8 >> 1) + (get_flag(FLAG_C) * 0x80));
		set_8bit_register(reg, val8);
		set_flag_values((val8 == 0) and !ignore_zero_flag, false, false, carry_flag);
		break;
	}
}

void CPU::RR_MEM()
{
	switch (busyCycles)
	{
	case 2:
		val8 = MEM->getMemValue(HL);
		break;
	case 1:
		bool carry_flag = ((val8 & 0x01) != 0);
		val8 = ((val8 >> 1) + (get_flag(FLAG_C) * 0x80));
		MEM->setMemValue(HL, val8);
		set_flag_values(val8 == 0, false, false, carry_flag);
		break;
	}
}

void CPU::RRC(Register_8Bit reg, bool ignore_zero_flag)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(reg);
		bool carry_flag = ((val8 & 0x01) != 0);
		val8 = ((val8 >> 1) + carry_flag * 0x80);
		set_8bit_register(reg, val8);
		set_flag_values((val8 == 0) and !ignore_zero_flag, false, false, carry_flag);
		break;
	}
}

void CPU::RRC_MEM()
{
	switch (busyCycles)
	{
	case 2:
		val8 = MEM->getMemValue(HL);
		break;
	case 1:
		bool carry_flag = ((val8 & 0x01) != 0);
		val8 = ((val8 >> 1) + carry_flag * 0x80);
		MEM->setMemValue(HL, val8);
		set_flag_values(val8 == 0, false, false, carry_flag);
		break;
	}	
}

void CPU::RST(uint8_t index)
{
	switch (busyCycles)
	{
	case 2:
		val16 = get_16bit_register(REGISTER_PC);
		SP--;
		MEM->setMemValue(SP, val16 >> 8);
		break;
	case 1:
		SP--;
		MEM->setMemValue(SP, val16 & 0xFF);
		val16 = RST_VECTOR[index];
		break;
	case 0:
		set_16bit_register(REGISTER_PC, val16);
		break;
	}
}

void CPU::SBC(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(reg);
		uint8_t before = get_8bit_register(REGISTER_A);
		uint8_t before_lower = before & 0x0F;
		uint8_t val_lower = val8 & 0x0F;
		uint8_t subtracted = before - val8;

		bool check_c = get_flag(FLAG_C);

		if (check_c)
			subtracted--;

		bool half_carry_flag = before_lower < (val_lower + check_c);
		bool carry_flag = before < (val8 + check_c);

		set_flag_values(subtracted == 0, true, half_carry_flag, carry_flag);

		set_8bit_register(REGISTER_A, subtracted);
		break;
	}
}

void CPU::SBC()
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_immediate_8bit_value();
		break;
	case 0:
		uint8_t before = get_8bit_register(REGISTER_A);
		uint8_t before_lower = before & 0x0F;
		uint8_t val_lower = val8 & 0x0F;
		uint8_t subtracted = before - val8;

		bool check_c = get_flag(FLAG_C);

		if (check_c)
			subtracted--;

		bool half_carry_flag = before_lower < (val_lower + check_c);
		bool carry_flag = before < (val8 + check_c);

		set_flag_values(subtracted == 0, true, half_carry_flag, carry_flag);

		set_8bit_register(REGISTER_A, subtracted);
		break;
	}
}

void CPU::SBC_MEM()
{
	switch (busyCycles)
	{
	case 1:
		val8 = MEM->getMemValue(HL);
		break;
	case 0:
		uint8_t before = get_8bit_register(REGISTER_A);
		uint8_t before_lower = before & 0x0F;
		uint8_t val_lower = val8 & 0x0F;
		uint8_t subtracted = before - val8;

		bool check_c = get_flag(FLAG_C);

		if (check_c)
			subtracted--;

		bool half_carry_flag = before_lower < (val_lower + check_c);
		bool carry_flag = before < (val8 + check_c);

		set_flag_values(subtracted == 0, true, half_carry_flag, carry_flag);

		set_8bit_register(REGISTER_A, subtracted);
		break;
	}
}

void CPU::SCF()
{
	switch (busyCycles)
	{
	case 0:
		set_flag_values(get_flag(FLAG_Z), false, false, true);
		break;
	}
}

void CPU::SET(uint8_t bit, Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_8bit_register(reg) | (0x01 << bit);
		break;
	case 0:
		set_8bit_register(reg, val8);
		break;
	}
}

void CPU::SET_MEM(uint8_t bit)
{
	switch (busyCycles)
	{
	case 2:
		val8 = MEM->getMemValue(HL) | (0x01 << bit);
		break;
	case 1:
		MEM->setMemValue(HL, val8);
		break;
	}
}

void CPU::SLA(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(reg);
		bool carry_flag = ((val8 & 0x80) != 0);
		val8 = (val8 << 1);
		set_8bit_register(reg, val8);
		set_flag_values(val8 == 0, false, false, carry_flag);
		break;
	}
}

void CPU::SLA_MEM()
{
	switch (busyCycles)
	{
	case 2:
		val8 = MEM->getMemValue(HL);
		break;
	case 1:
		bool carry_flag = ((val8 & 0x80) != 0);
		val8 = (val8 << 1);
		MEM->setMemValue(HL, val8);
		set_flag_values(val8 == 0, false, false, carry_flag);
		break;
	}
}
void CPU::SRA(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(reg);
		bool carry_flag = ((val8 & 0x01) != 0);
		uint8_t bit7 = val8 & 0x80;
		val8 = (val8 >> 1) + bit7;
		set_8bit_register(reg, val8);
		set_flag_values(val8 == 0, false, false, carry_flag);
		break;
	}
}
void CPU::SRA_MEM()
{
	switch (busyCycles)
	{
	case 2:
		val8 = MEM->getMemValue(HL);
		break;
	case 1:
		bool carry_flag = ((val8 & 0x01) != 0);
		uint8_t bit7 = val8 & 0x80;
		val8 = (val8 >> 1) + bit7;
		MEM->setMemValue(HL, val8);
		set_flag_values(val8 == 0, false, false, carry_flag);
		break;
	}
}
void CPU::SRL(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(reg);
		bool carry_flag = ((val8 & 0x01) != 0);
		val8 = (val8 >> 1);
		set_8bit_register(reg, val8);
		set_flag_values(val8 == 0, false, false, carry_flag);
		break;
	}
}

void CPU::SRL_MEM()
{
	switch (busyCycles)
	{
	case 2:
		val8 = MEM->getMemValue(HL);
		break;
	case 1:
		bool carry_flag = ((val8 & 0x01) != 0);
		val8 = (val8 >> 1);
		MEM->setMemValue(HL, val8);
		set_flag_values(val8 == 0, false, false, carry_flag);
		break;
	}
}
void CPU::STOP()
{
	switch (busyCycles)
	{
	case 0:
		//TODO
		if (MEM->isInCGBMode() and (MEM->getMemValue(0xFF4D) & 0b00000001) != 0)
		{
			MEM->performSpeedSwitch();
		}
		break;
	}
}

void CPU::SUB(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(reg);
		uint8_t before = get_8bit_register(REGISTER_A);
		uint8_t before_lower = before & 0x0F;
		uint8_t val_lower = val8 & 0x0F;
		uint8_t subtracted = before - val8;

		bool half_carry_flag = before_lower < val_lower;
		bool carry_flag = before < val8;

		set_flag_values(subtracted == 0, true, half_carry_flag, carry_flag);

		set_8bit_register(REGISTER_A, subtracted);
		break;
	}
}

void CPU::SUB()
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_immediate_8bit_value();
		break;
	case 0:
		uint8_t before = get_8bit_register(REGISTER_A);
		uint8_t before_lower = before & 0x0F;
		uint8_t val_lower = val8 & 0x0F;
		uint8_t subtracted = before - val8;

		bool half_carry_flag = before_lower < val_lower;
		bool carry_flag = before < val8;

		set_flag_values(subtracted == 0, true, half_carry_flag, carry_flag);

		set_8bit_register(REGISTER_A, subtracted);
		break;
	}
}

void CPU::SUB_MEM()
{
	switch (busyCycles)
	{
	case 1:
		val8 = MEM->getMemValue(HL);
		break;
	case 0:
		uint8_t before = get_8bit_register(REGISTER_A);
		uint8_t before_lower = before & 0x0F;
		uint8_t val_lower = val8 & 0x0F;
		uint8_t subtracted = before - val8;

		bool half_carry_flag = (before_lower < val_lower);
		bool carry_flag = (before < val8);

		set_flag_values(subtracted == 0, true, half_carry_flag, carry_flag);

		set_8bit_register(REGISTER_A, subtracted);
		break;
	}
}

void CPU::SWAP(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(reg);
		val8 = (val8 >> 4) + (val8 << 4);

		bool zero_flag = (val8 == 0x00);

		set_flag_values(zero_flag, false, false, false);

		set_8bit_register(reg, val8);
		break;
	}
}

void CPU::SWAP_MEM()
{
	switch (busyCycles)
	{
	case 2:
		val8 = MEM->getMemValue(HL);
		break;
	case 1:
		val8 = (val8 >> 4) + (val8 << 4);

		bool zero_flag = (val8 == 0x00);

		set_flag_values(zero_flag, false, false, false);

		MEM->setMemValue(HL, val8);
		break;
	}
}

void CPU::XOR(Register_8Bit reg)
{
	switch (busyCycles)
	{
	case 0:
		val8 = get_8bit_register(REGISTER_A) ^ get_8bit_register(reg);

		bool zero_flag = (val8 == 0x00);

		set_flag_values(zero_flag, false, false, false);

		set_8bit_register(REGISTER_A, val8);
		break;
	}
}

void CPU::XOR()
{
	switch (busyCycles)
	{
	case 1:
		val8 = get_immediate_8bit_value();
		break;
	case 0:
		val8 = get_8bit_register(REGISTER_A) ^ val8;
		bool zero_flag = (val8 == 0x00);
		set_flag_values(zero_flag, false, false, false);
		set_8bit_register(REGISTER_A, val8);
		break;
	}
}

void CPU::XOR_MEM()
{
	switch (busyCycles)
	{
	case 1:
		val8 = MEM->getMemValue(HL);
		break;
	case 0:
		val8 = get_8bit_register(REGISTER_A) ^ val8;
		bool zero_flag = (val8 == 0x00);
		set_flag_values(zero_flag, false, false, false);
		set_8bit_register(REGISTER_A, val8);
		break;
	}
}