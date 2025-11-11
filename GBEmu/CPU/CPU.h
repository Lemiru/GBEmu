#pragma once

#include <stdint.h>

#include "../Memory/MemoryManager.h"
#include <stdexcept>
#include <iostream>

enum FlagBit
{
	FLAG_C = 4,
	FLAG_H = 5,
	FLAG_N = 6,
	FLAG_Z = 7
};

enum Register_8Bit
{
	REGISTER_A,
	REGISTER_F,
	REGISTER_B,
	REGISTER_C,
	REGISTER_D,
	REGISTER_E,
	REGISTER_H,
	REGISTER_L
};

enum Register_16Bit
{
	REGISTER_AF,
	REGISTER_BC,
	REGISTER_DE,
	REGISTER_HL,
	REGISTER_SP,
	REGISTER_PC
};

enum Condition_Code
{
	Z,
	NZ,
	C,
	NC
};

class CPU 
{
private:
	MemoryManager* MEM;
	uint16_t AF;
	uint16_t BC;
	uint16_t DE;
	uint16_t HL;
	uint16_t SP;
	uint16_t PC;

	uint16_t val16;

	uint8_t interruptBusyCycles;
	uint8_t busyCycles;
	uint8_t nextInstruction;

	uint8_t val8;

	bool IME;
	bool EI_called;
	bool EI_pending;
	bool prefix_instruction;
	bool conditional;
	bool interrupt_in_progress;
	bool halt;
	bool halt_bug;
	Interrupt current_interrupt;

public:
	CPU(MemoryManager* memory);
	void reset();
	uint8_t get_8bit_register(Register_8Bit reg);
	uint16_t get_16bit_register(Register_16Bit reg);
	void set_8bit_register(Register_8Bit reg, uint8_t val);
	void set_16bit_register(Register_16Bit reg, uint16_t val);
	void cycle();

private:
	uint8_t get_reg_high(uint16_t* reg);
	uint8_t get_reg_low(uint16_t* reg);
	void override_reg_high(uint16_t* reg, uint8_t val);
	void override_reg_low(uint16_t* reg, uint8_t val);
	void increment_PC();

	void set_flag_values(bool z, bool n, bool h, bool c);
	bool get_flag(FlagBit bit);
	bool check_condition(Condition_Code con);

	uint8_t get_immediate_8bit_value();
	uint16_t get_immediate_16bit_value();

	void check_and_initiate_interrupt();
	void handle_interrupt();

	void ADC(Register_8Bit reg);
	void ADC(Register_8Bit target, uint8_t val);
	void ADC();
	void ADC_MEM();
	void ADD(Register_8Bit reg);
	void ADD(Register_8Bit target, uint8_t val);
	void ADD();
	void ADD_MEM();
	void ADD_HL(Register_16Bit reg);
	void ADD_SP(uint8_t val);
	void AND(Register_8Bit reg);
	void AND();
	void AND_MEM();
	void BIT(uint8_t bit, Register_8Bit reg);
	void BIT_MEM(uint8_t bit);
	void CALL(bool condition = true);
	void CCF();
	void CP(Register_8Bit reg);
	void CP();
	void CP_MEM();
	void CPL();
	void DAA();
	void DEC(Register_8Bit reg);
	void DEC(Register_16Bit reg);
	void DEC_MEM();
	void DI();
	void EI();
	void HALT();
	void INC(Register_8Bit reg);
	void INC(Register_16Bit reg);
	void INC_MEM();
	void JP(bool condition = true);
	void JP(Register_16Bit reg);
	void JR(bool condition = true);
	void LD(Register_8Bit to, Register_8Bit from);
	void LD(Register_8Bit reg);
	void LD_TO_MEM(Register_16Bit address, Register_8Bit from);
	void LD_TO_MEM(Register_16Bit address);
	void LD_TO_MEM(Register_8Bit from);
	void LD_FROM_MEM(Register_8Bit reg, Register_16Bit address);
	void LD_FROM_MEM(Register_8Bit reg);
	void LDH_TO_MEM(Register_8Bit address, Register_8Bit from);
	void LDH_TO_MEM(Register_8Bit from);
	void LDH_FROM_MEM(Register_8Bit reg, Register_8Bit address);
	void LDH_FROM_MEM(Register_8Bit reg);
	void LDI_TO_MEM();
	void LDD_TO_MEM();
	void LDI_FROM_MEM();
	void LDD_FROM_MEM();
	void LD_16BIT(Register_16Bit reg);
	void LD_16BIT(Register_16Bit to, Register_16Bit from);
	void LD_16BIT_TO_MEM();
	void LD_SP_WITH_ADD();
	void OR(Register_8Bit reg);
	void OR();
	void OR_MEM();
	void POP(Register_16Bit reg);
	void PUSH(Register_16Bit reg);
	void RES(uint8_t bit, Register_8Bit reg);
	void RES_MEM(uint8_t bit);
	void RET(bool condition = true);
	void RETI();
	void RL(Register_8Bit reg, bool ignore_zero_flag = false);
	void RL_MEM();
	void RLC(Register_8Bit reg, bool ignore_zero_flag = false);
	void RLC_MEM();
	void RR(Register_8Bit reg, bool ignore_zero_flag = false);
	void RR_MEM();
	void RRC(Register_8Bit reg, bool ignore_zero_flag = false);
	void RRC_MEM();
	void RST(uint8_t index);
	void SBC(Register_8Bit reg);
	void SBC();
	void SBC_MEM();
	void SCF();
	void SET(uint8_t bit, Register_8Bit reg);
	void SET_MEM(uint8_t bit);
	void SLA(Register_8Bit reg);
	void SLA_MEM();
	void SRA(Register_8Bit reg);
	void SRA_MEM();
	void SRL(Register_8Bit reg);
	void SRL_MEM();
	void STOP();
	void SUB(Register_8Bit reg);
	void SUB();
	void SUB_MEM();
	void SWAP(Register_8Bit reg);
	void SWAP_MEM();
	void XOR(Register_8Bit reg);
	void XOR();
	void XOR_MEM();
};