#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "SaveStateUtil.h"

class Nes;

class Cpu
{
	friend class Emulator;
public:
	Cpu(Nes& nes);
	Cpu(Nes& nes, Snapshot& bytes);
	Cpu(const Cpu&) = delete;
	Cpu& operator=(const Cpu&) = delete;
	void Clock();
	void Reset();
	void Irq();
	void Nmi();
	void ClockInstruction();
	bool InstructionComplete() const;
	Snapshot SaveState() const;
private:
	Nes& nes;
	uint8_t opcode = 0;
	uint8_t data = 0;
	uint16_t addr = 0;
	int cyclesToNextInstruction = 0;
	void ReadAddr();
	void WriteStack(uint8_t data);
	uint8_t ReadStack();

	static bool PageChanged(uint16_t addr0, uint16_t addr1);
	static uint16_t JoinBytes(uint8_t lo, uint8_t hi);

	using AddrModeFn = bool(void);
	using OpcodeFn = bool(void);
	using AddrMode = bool(Cpu::*)();
	using Opcode = bool(Cpu::*)();
	static struct Instruction
	{
		std::string opname;
		Opcode op = nullptr;
		std::string addrname;
		AddrMode addrmode = nullptr;
		int cycles = 0;
		int bytes = 0;
		Instruction() = default;
		Instruction(
			std::string opname,
			Opcode op,
			std::string addrname,
			AddrMode addrmode,
			int cycles
		);
		Instruction& operator=(const Instruction&) = default;
	} instructions[256];

	// Registers
	uint8_t ra = 0;
	uint8_t rx = 0;
	uint8_t ry = 0;
	uint8_t sp = 0;
	uint16_t pc = 0;
	union
	{
		struct
		{
			bool c : 1;
			bool z : 1;
			bool i : 1;
			bool d : 1;
			bool b : 1;
			bool u : 1;
			bool v : 1;
			bool n : 1;
		};
		uint8_t reg = 0;
	} status;

	// Addressing modes
	AddrModeFn IMP, IMM, ACC;
	AddrModeFn ABS, ABX, ABY;
	AddrModeFn ZRP, ZPX, ZPY;
	AddrModeFn IDX, IDY, IND;
	AddrModeFn REL;

	// Opcodes
	OpcodeFn ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK, BVC, BVS, CLC;
	OpcodeFn CLD, CLI, CLV, CMP, CPX, CPY, DEC, DEX, DEY, EOR, INC, INX, INY, JMP;
	OpcodeFn JSR, LDA, LDX, LDY, LSR, NOP, ORA, PHA, PHP, PLA, PLP, ROL, ROR, RTI;
	OpcodeFn RTS, SBC, SEC, SED, SEI, STA, STX, STY, TAX, TAY, TSX, TXA, TXS, TYA;

	// Illegal opcodes
	OpcodeFn AAC, SAX, ARR, ASR, ATX, AXA, AXS, DCP, DOP, ISB, KIL;
	OpcodeFn LAR, LAX, RLA, RRA, SLO, SRE, SXA, SYA, TOP, XAA, XAS;
};
