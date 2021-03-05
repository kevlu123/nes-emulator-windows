#include "Cpu.h"
#include "Nes.h"
#include <vector>

#define X(op, addrmode, cycles) Cpu::Instruction(#op, &Cpu::op, #addrmode, &Cpu::addrmode, cycles)
Cpu::Instruction Cpu::instructions[256]
{
/* YX*/ /* 0 */         /* 1 */         /* 2 */         /* 3 */         /* 4 */         /* 5 */         /* 6 */         /* 7 */         /* 8 */         /* 9 */         /* A */         /* B */         /* C */         /* D */         /* E */         /* F */
/* 0 */ X(BRK, IMM, 7), X(ORA, IDX, 6), X(KIL, IMP, 2), X(SLO, IDX, 8), X(DOP, ZRP, 3), X(ORA, ZRP, 3), X(ASL, ZRP, 5), X(SLO, ZRP, 5), X(PHP, IMP, 3), X(ORA, IMM, 2), X(ASL, ACC, 2), X(AAC, IMM, 2), X(TOP, ABS, 4), X(ORA, ABS, 4), X(ASL, ABS, 6), X(SLO, ABS, 6),
/* 1 */ X(BPL, REL, 2), X(ORA, IDY, 5), X(KIL, IMP, 2), X(SLO, IDY, 8), X(DOP, ZPX, 4), X(ORA, ZPX, 4), X(ASL, ZPX, 6), X(SLO, ZPX, 6), X(CLC, IMP, 2), X(ORA, ABY, 4), X(NOP, IMP, 2), X(SLO, ABY, 7), X(TOP, ABX, 4), X(ORA, ABX, 4), X(ASL, ABX, 7), X(SLO, ABX, 7),
/* 2 */ X(JSR, ABS, 6), X(AND, IDX, 6), X(KIL, IMP, 2), X(RLA, IDX, 8), X(BIT, ZRP, 3), X(AND, ZRP, 3), X(ROL, ZRP, 5), X(RLA, ZRP, 5), X(PLP, IMP, 4), X(AND, IMM, 2), X(ROL, ACC, 2), X(AAC, IMM, 2), X(BIT, ABS, 4), X(AND, ABS, 4), X(ROL, ABS, 6), X(RLA, ABS, 6),
/* 3 */ X(BMI, REL, 2), X(AND, IDY, 5), X(KIL, IMP, 2), X(RLA, IDY, 8), X(DOP, ZPX, 4), X(AND, ZPX, 4), X(ROL, ZPX, 6), X(RLA, ZPX, 6), X(SEC, IMP, 2), X(AND, ABY, 4), X(NOP, IMP, 2), X(RLA, ABY, 7), X(TOP, ABX, 4), X(AND, ABX, 4), X(ROL, ABX, 7), X(RLA, ABX, 7),

/* 4 */ X(RTI, IMP, 6), X(EOR, IDX, 6), X(KIL, IMP, 2), X(SRE, IDX, 8), X(DOP, ZRP, 3), X(EOR, ZRP, 3), X(LSR, ZRP, 5), X(SRE, ZRP, 5), X(PHA, IMP, 3), X(EOR, IMM, 2), X(LSR, ACC, 2), X(ASR, IMM, 2), X(JMP, ABS, 3), X(EOR, ABS, 4), X(LSR, ABS, 6), X(SRE, ABS, 6),
/* 5 */ X(BVC, REL, 2), X(EOR, IDY, 5), X(KIL, IMP, 2), X(SRE, IDY, 8), X(DOP, ZPX, 4), X(EOR, ZPX, 4), X(LSR, ZPX, 6), X(SRE, ZPX, 6), X(CLI, IMP, 2), X(EOR, ABY, 4), X(NOP, IMP, 2), X(SRE, ABY, 7), X(TOP, ABX, 4), X(EOR, ABX, 4), X(LSR, ABX, 7), X(SRE, ABX, 7),
/* 6 */ X(RTS, IMP, 6), X(ADC, IDX, 6), X(KIL, IMP, 2), X(RRA, IDX, 8), X(DOP, ZRP, 3), X(ADC, ZRP, 3), X(ROR, ZRP, 5), X(RRA, ZRP, 5), X(PLA, IMP, 4), X(ADC, IMM, 2), X(ROR, ACC, 2), X(ARR, IMM, 2), X(JMP, IND, 5), X(ADC, ABS, 4), X(ROR, ABS, 6), X(RRA, ABS, 6),
/* 7 */ X(BVS, REL, 2), X(ADC, IDY, 5), X(KIL, IMP, 2), X(RRA, IDY, 8), X(DOP, ZPX, 4), X(ADC, ZPX, 4), X(ROR, ZPX, 6), X(RRA, ZPX, 6), X(SEI, IMP, 2), X(ADC, ABY, 4), X(NOP, IMP, 2), X(RRA, ABY, 7), X(TOP, ABX, 4), X(ADC, ABX, 4), X(ROR, ABX, 7), X(RRA, ABX, 7),

/* 8 */ X(DOP, IMM, 2), X(STA, IDX, 6), X(DOP, IMM, 2), X(SAX, IDX, 6), X(STY, ZRP, 3), X(STA, ZRP, 3), X(STX, ZRP, 3), X(SAX, ZRP, 3), X(DEY, IMP, 2), X(DOP, IMM, 2), X(TXA, IMP, 2), X(XAA, IMM, 2), X(STY, ABS, 4), X(STA, ABS, 4), X(STX, ABS, 4), X(SAX, ABS, 4),
/* 9 */ X(BCC, REL, 2), X(STA, IDY, 6), X(KIL, IMP, 2), X(AXA, IDY, 6), X(STY, ZPX, 4), X(STA, ZPX, 4), X(STX, ZPY, 4), X(SAX, ZPY, 4), X(TYA, IMP, 2), X(STA, ABY, 5), X(TXS, IMP, 2), X(XAS, ABY, 5), X(SYA, ABX, 5), X(STA, ABX, 5), X(SXA, ABY, 5), X(AXA, ABY, 5),
/* A */ X(LDY, IMM, 2), X(LDA, IDX, 6), X(LDX, IMM, 2), X(LAX, IDX, 6), X(LDY, ZRP, 3), X(LDA, ZRP, 3), X(LDX, ZRP, 3), X(LAX, ZRP, 3), X(TAY, IMP, 2), X(LDA, IMM, 2), X(TAX, IMP, 2), X(ATX, IMM, 2), X(LDY, ABS, 4), X(LDA, ABS, 4), X(LDX, ABS, 4), X(LAX, ABS, 4),
/* B */ X(BCS, REL, 2), X(LDA, IDY, 5), X(KIL, IMP, 2), X(LAX, IDY, 5), X(LDY, ZPX, 4), X(LDA, ZPX, 4), X(LDX, ZPY, 4), X(LAX, ZPY, 4), X(CLV, IMP, 2), X(LDA, ABY, 4), X(TSX, IMP, 2), X(LAR, ABY, 4), X(LDY, ABX, 4), X(LDA, ABX, 4), X(LDX, ABY, 4), X(LAX, ABY, 4),

/* C */ X(CPY, IMM, 2), X(CMP, IDX, 6), X(DOP, IMM, 2), X(DCP, IDX, 8), X(CPY, ZRP, 3), X(CMP, ZRP, 3), X(DEC, ZRP, 5), X(DCP, ZRP, 5), X(INY, IMP, 2), X(CMP, IMM, 2), X(DEX, IMP, 2), X(AXS, IMM, 2), X(CPY, ABS, 4), X(CMP, ABS, 4), X(DEC, ABS, 6), X(DCP, ABS, 6),
/* D */ X(BNE, REL, 2), X(CMP, IDY, 5), X(KIL, IMP, 2), X(DCP, IDY, 8), X(DOP, ZPX, 4), X(CMP, ZPX, 4), X(DEC, ZPX, 6), X(DCP, ZPX, 6), X(CLD, IMP, 2), X(CMP, ABY, 4), X(NOP, IMP, 2), X(DCP, ABY, 7), X(TOP, ABX, 4), X(CMP, ABX, 4), X(DEC, ABX, 7), X(DCP, ABX, 7),
/* E */ X(CPX, IMM, 2), X(SBC, IDX, 6), X(DOP, IMM, 2), X(ISB, IDX, 8), X(CPX, ZRP, 3), X(SBC, ZRP, 3), X(INC, ZRP, 5), X(ISB, ZRP, 5), X(INX, IMP, 2), X(SBC, IMM, 2), X(NOP, IMP, 2), X(SBC, IMM, 2), X(CPX, ABS, 4), X(SBC, ABS, 4), X(INC, ABS, 6), X(ISB, ABS, 6),
/* F */ X(BEQ, REL, 2), X(SBC, IDY, 5), X(KIL, IMP, 2), X(ISB, IDY, 8), X(DOP, ZPX, 4), X(SBC, ZPX, 4), X(INC, ZPX, 6), X(ISB, ZPX, 6), X(SED, IMP, 2), X(SBC, ABY, 4), X(NOP, IMP, 2), X(ISB, ABY, 7), X(TOP, ABX, 4), X(SBC, ABX, 4), X(INC, ABX, 7), X(ISB, ABX, 7),
};
#undef X


Cpu::Instruction::Instruction(
	std::string opname,
	Opcode op,
	std::string addrname,
	AddrMode addrmode,
	int cycles
) :
	opname(opname),
	op(op),
	addrname(addrname),
	addrmode(addrmode),
	cycles(cycles),
	bytes(0)
{
	if (addrmode == &Cpu::IMP) bytes = 1;
	else if (addrmode == &Cpu::IMM) bytes = 2;
	else if (addrmode == &Cpu::ACC) bytes = 1;
	else if (addrmode == &Cpu::ABS) bytes = 3;
	else if (addrmode == &Cpu::ABX) bytes = 3;
	else if (addrmode == &Cpu::ABY) bytes = 3;
	else if (addrmode == &Cpu::ZRP) bytes = 2;
	else if (addrmode == &Cpu::ZPX) bytes = 2;
	else if (addrmode == &Cpu::ZPY) bytes = 2;
	else if (addrmode == &Cpu::REL) bytes = 2;
	else if (addrmode == &Cpu::IDX) bytes = 2;
	else if (addrmode == &Cpu::IDY) bytes = 2;
	else if (addrmode == &Cpu::IND) bytes = 3;
}

Cpu::Cpu(Nes& nes) :
	nes(nes)
{
	pc = JoinBytes(nes.CpuRead(0xFFFC), nes.CpuRead(0xFFFD));

	ra = rx = ry = 0;
	sp = 0xFD;
	status.reg = 0b0010'0100; // Set U, I

	cyclesToNextInstruction = 8;
}

Cpu::Cpu(Nes& nes, Snapshot& bytes) :
	nes(nes)
{
	LoadBytes(bytes, cyclesToNextInstruction);
	LoadBytes(bytes, ra);
	LoadBytes(bytes, rx);
	LoadBytes(bytes, ry);
	LoadBytes(bytes, sp);
	LoadBytes(bytes, pc);
	LoadBytes(bytes, status);
}

Snapshot Cpu::SaveState() const
{
	std::vector<uint8_t> bytes;
	SaveBytes(bytes, cyclesToNextInstruction);
	SaveBytes(bytes, ra);
	SaveBytes(bytes, rx);
	SaveBytes(bytes, ry);
	SaveBytes(bytes, sp);
	SaveBytes(bytes, pc);
	SaveBytes(bytes, status);
	return bytes;
}

void Cpu::Clock()
{
	if (cyclesToNextInstruction <= 0)
	{
		opcode = nes.CpuRead(pc);
		const auto& instruction = Cpu::instructions[opcode];
		pc++;

		cyclesToNextInstruction = instruction.cycles;
		bool extraCyclePossible = (this->*instruction.addrmode)();
		cyclesToNextInstruction += extraCyclePossible & (this->*instruction.op)();
	}
	cyclesToNextInstruction--;
}

void Cpu::Reset()
{
	pc = JoinBytes(nes.CpuRead(0xFFFC), nes.CpuRead(0xFFFD));

	sp -= 3;
	status.i = true;

	cyclesToNextInstruction = 8;
}

void Cpu::Irq()
{
	if (!status.i)
	{
		WriteStack((pc & 0xFF00) >> 8);
		WriteStack(pc & 0xFF);
		WriteStack(status.reg);
		status.i = true;
		pc = JoinBytes(nes.CpuRead(0xFFFE), nes.CpuRead(0xFFFF));
		cyclesToNextInstruction = 7;
	}
}

void Cpu::Nmi()
{
	WriteStack((pc & 0xFF00) >> 8);
	WriteStack(pc & 0xFF);
	WriteStack(status.reg);
	status.i = true;
	pc = JoinBytes(nes.CpuRead(0xFFFA), nes.CpuRead(0xFFFB));
	cyclesToNextInstruction = 8;
}

void Cpu::ClockInstruction()
{
	do
	{
		Clock();
	} while (!InstructionComplete());
}

bool Cpu::InstructionComplete() const
{
	return cyclesToNextInstruction == 0;
}

void Cpu::ReadAddr()
{
	data = nes.CpuRead(addr);
}

void Cpu::WriteStack(uint8_t data)
{
	nes.CpuWrite(0x0100 + sp, data);
	sp--;
}

uint8_t Cpu::ReadStack()
{
	sp++;
	uint8_t res = nes.CpuRead(0x0100 + sp);
	return res;
}

bool Cpu::PageChanged(uint16_t addr0, uint16_t addr1)
{
	return (addr0 & 0x100) ^ (addr1 & 0x100);
}

uint16_t Cpu::JoinBytes(uint8_t lo, uint8_t hi)
{
	return (hi << 8) | lo;
}

// Addressing modes

// All data is implied by opcode
bool Cpu::IMP()
{
	return false;
}

// Use next byte as data
bool Cpu::IMM()
{
	addr = pc;
	pc++;
	return false;
}

// Follow the address formed by the next 2 bytes
bool Cpu::ABS()
{
	addr = JoinBytes(nes.CpuRead(pc), nes.CpuRead(pc + 1));
	pc += 2;
	return false;
}

// Follow the address formed by the next 2 bytes added to X
bool Cpu::ABX()
{
	uint16_t tempAddr = JoinBytes(nes.CpuRead(pc), nes.CpuRead(pc + 1));
	addr = tempAddr + rx;
	pc += 2;
	return PageChanged(tempAddr, addr);
}

// Follow the address formed by the next 2 bytes added to Y
bool Cpu::ABY()
{
	uint16_t tempAddr = JoinBytes(nes.CpuRead(pc), nes.CpuRead(pc + 1));
	addr = tempAddr + ry;
	pc += 2;
	return PageChanged(tempAddr, addr);
}

// Follow the address formed by the next byte
bool Cpu::ZRP()
{
	addr = nes.CpuRead(pc);
	pc++;
	return false;
}

// Follow zero page the address formed by the next byte added to X
bool Cpu::ZPX()
{
	addr = (nes.CpuRead(pc) + rx) & 0xFF;
	pc++;
	return false;
}

// Follow zero page the address formed by the next byte added to Y
bool Cpu::ZPY()
{
	addr = (nes.CpuRead(pc) + ry) & 0xFF;
	pc++;
	return false;
}

// Uses A as data
bool Cpu::ACC()
{
	data = ra;
	return false;
}

// Used by branch instructions. Add next byte (signed) to pc to get new pc
bool Cpu::REL()
{
	addr = static_cast<int8_t>(nes.CpuRead(pc)) + pc + 1;
	pc++;
	return PageChanged(pc + 1, addr);
}

// Add X to the next byte to get a zero page address. The data is pointed to by the 2 byte address at this address
bool Cpu::IDX()
{
	uint8_t zpAddr = nes.CpuRead(pc) + rx;
	pc++;
	addr = JoinBytes(nes.CpuRead(zpAddr), nes.CpuRead((zpAddr + 1) & 0xFF));
	return false;
}

// Follow the next one byte address to get a two byte zero page address. The data is pointed to by this address added to Y. 
bool Cpu::IDY()
{
	uint8_t zpAddr = nes.CpuRead(pc);
	pc++;
	uint16_t tmp = JoinBytes(nes.CpuRead(zpAddr), nes.CpuRead((zpAddr + 1) & 0xFF));
	addr = tmp + ry;
	return PageChanged(tmp, addr);
}

// Used by JMP. Jump to the location pointed to by the next 2 bytes
bool Cpu::IND()
{
	addr = JoinBytes(nes.CpuRead(pc), nes.CpuRead(pc + 1));
	addr = JoinBytes(nes.CpuRead(addr), nes.CpuRead(((addr + 1) & 0xFF) | (addr & 0xFF00)));
	return false;
}

// Opcodes

bool Cpu::ADC()
{
	ReadAddr();
	uint8_t res = ra + data + status.c;
	status.c = static_cast<uint16_t>(ra) + data + status.c > 255;
	status.z = res == 0;
	status.n = res & 0x80;
	status.v = ((~(ra ^ data) & (ra ^ res)) & 0x80) >> 7;
	ra = res;
	return true;
}

bool Cpu::AND()
{
	ReadAddr();
	ra &= data;
	status.n = ra & 0x80;
	status.z = ra == 0;
	return true;
}

bool Cpu::ASL()
{
	uint8_t res;
	bool carry;
	if (instructions[opcode].addrmode == &Cpu::ACC)
	{
		carry = ra & 0x80;
		res = ra << 1;
		ra = res;
	}
	else
	{
		ReadAddr();
		carry = data & 0x80;
		res = data << 1;
		nes.CpuWrite(addr, res);
	}
	status.c = carry;
	status.n = res & 0x80;
	status.z = res == 0;
	return false;
}

bool Cpu::BIT()
{
	ReadAddr();
	status.z = (data & ra) == 0;
	status.n = data & 0x80;
	status.v = data & 0x40;
	return false;
}

bool Cpu::BPL()
{
	if (!status.n)
	{
		pc = addr;
		cyclesToNextInstruction++;
		return true;
	}
	return false;
}

bool Cpu::BMI()
{
	if (status.n)
	{
		pc = addr;
		cyclesToNextInstruction++;
		return true;
	}
	return false;
}

bool Cpu::BVC()
{
	if (!status.v)
	{
		pc = addr;
		cyclesToNextInstruction++;
		return true;
	}
	return false;
}

bool Cpu::BVS()
{
	if (status.v)
	{
		pc = addr;
		cyclesToNextInstruction++;
		return true;
	}
	return false;
}

bool Cpu::BCC()
{
	if (!status.c)
	{
		pc = addr;
		cyclesToNextInstruction++;
		return true;
	}
	return false;
}

bool Cpu::BCS()
{
	if (status.c)
	{
		pc = addr;
		cyclesToNextInstruction++;
		return true;
	}
	return false;
}

bool Cpu::BNE()
{
	if (!status.z)
	{
		pc = addr;
		cyclesToNextInstruction++;
		return true;
	}
	return false;
}

bool Cpu::BEQ()
{
	if (status.z)
	{
		pc = addr;
		cyclesToNextInstruction++;
		return true;
	}
	return false;
}

bool Cpu::BRK()
{
	WriteStack((pc & 0xFF00) >> 8);
	WriteStack(pc & 0xFF);
	WriteStack(status.reg | 0b0001'0000); // Set break flag
	status.i = true;
	pc = (nes.CpuRead(0xFFFE) << 8) | nes.CpuRead(0xFFFF);
	return false;
}

bool Cpu::CMP()
{
	ReadAddr();
	uint8_t res = ra - data;
	status.n = res & 0x80;
	status.z = res == 0;
	status.c = ra >= data;
	return false;
}

bool Cpu::CPX()
{
	ReadAddr();
	uint8_t res = rx - data;
	status.n = res & 0x80;
	status.z = res == 0;
	status.c = rx >= data;
	return false;
}

bool Cpu::CPY()
{
	ReadAddr();
	uint8_t res = ry - data;
	status.n = res & 0x80;
	status.z = res == 0;
	status.c = ry >= data;
	return false;
}

bool Cpu::DEC()
{
	ReadAddr();
	uint8_t res = data - 1;
	nes.CpuWrite(addr, res);
	status.n = res & 0x80;
	status.z = res == 0;
	return false;
}

bool Cpu::EOR()
{
	ReadAddr();
	ra ^= data;
	status.n = ra & 0x80;
	status.z = ra == 0;
	return true;
}

bool Cpu::CLC()
{
	status.c = false;
	return false;
}

bool Cpu::SEC()
{
	status.c = true;
	return false;
}

bool Cpu::CLI()
{
	status.i = false;
	return false;
}

bool Cpu::SEI()
{
	status.i = true;
	return false;
}

bool Cpu::CLV()
{
	status.v = false;
	return false;
}

bool Cpu::CLD()
{
	status.d = false;
	return false;
}

bool Cpu::SED()
{
	status.d = true;
	return false;
}

bool Cpu::INC()
{
	ReadAddr();
	uint8_t res = data + 1;
	nes.CpuWrite(addr, res);
	status.n = res & 0x80;
	status.z = res == 0;
	return false;
}

bool Cpu::JMP()
{
	pc = addr;
	return false;
}

bool Cpu::JSR()
{
	uint16_t prevAddr = pc - 1;
	WriteStack((prevAddr & 0xFF00) >> 8);
	WriteStack(prevAddr & 0xFF);
	pc = addr;
	return false;
}

bool Cpu::LDA()
{
	ReadAddr();
	ra = data;
	status.z = ra == 0;
	status.n = ra & 0x80;
	return true;
}

bool Cpu::LDX()
{
	ReadAddr();
	rx = data;
	status.z = rx == 0;
	status.n = rx & 0x80;
	return true;
}

bool Cpu::LDY()
{
	ReadAddr();
	ry = data;
	status.z = ry == 0;
	status.n = ry & 0x80;
	return true;
}

bool Cpu::LSR()
{
	uint8_t res;
	bool carry;
	if (instructions[opcode].addrmode == &Cpu::ACC)
	{
		carry = ra & 1;
		res = ra >> 1;
		ra = res;
	}
	else
	{
		ReadAddr();
		carry = data & 1;
		res = data >> 1;
		nes.CpuWrite(addr, res);
	}
	status.c = carry;
	status.n = res & 0x80;
	status.z = res == 0;
	return false;
}

bool Cpu::NOP()
{
	return false;
}

bool Cpu::ORA()
{
	ReadAddr();
	ra |= data;
	status.n = ra & 0x80;
	status.z = ra == 0;
	return true;
}

bool Cpu::TAX()
{
	rx = ra;
	status.n = rx & 0x80;
	status.z = rx == 0;
	return false;
}

bool Cpu::TXA()
{
	ra = rx;
	status.n = ra & 0x80;
	status.z = ra == 0;
	return false;
}

bool Cpu::DEX()
{
	rx--;
	status.n = rx & 0x80;
	status.z = rx == 0;
	return false;
}

bool Cpu::INX()
{
	rx++;
	status.n = rx & 0x80;
	status.z = rx == 0;
	return false;
}

bool Cpu::TAY()
{
	ry = ra;
	status.n = ry & 0x80;
	status.z = ry == 0;
	return false;
}

bool Cpu::TYA()
{
	ra = ry;
	status.n = ra & 0x80;
	status.z = ra == 0;
	return false;
}

bool Cpu::DEY()
{
	ry--;
	status.n = ry & 0x80;
	status.z = ry == 0;
	return false;
}

bool Cpu::INY()
{
	ry++;
	status.n = ry & 0x80;
	status.z = ry == 0;
	return false;
}

bool Cpu::ROL()
{
	uint8_t res;
	bool carry;
	if (instructions[opcode].addrmode == &Cpu::ACC)
	{
		carry = ra & 0x80;
		res = (ra << 1) | static_cast<uint8_t>(status.c);
		ra = res;
	}
	else
	{
		ReadAddr();
		carry = data & 0x80;
		res = (data << 1) | static_cast<uint8_t>(status.c);
		nes.CpuWrite(addr, res);
	}
	status.c = carry;
	status.n = res & 0x80;
	status.z = res == 0;
	return false;
}

bool Cpu::ROR()
{
	uint8_t res;
	bool carry;
	if (instructions[opcode].addrmode == &Cpu::ACC)
	{
		carry = ra & 0x01;
		res = (ra >> 1) | (status.c << 7);
		ra = res;
	}
	else
	{
		ReadAddr();
		carry = data & 0x01;
		res = (data >> 1) | (status.c << 7);
		nes.CpuWrite(addr, res);
	}
	status.c = carry;
	status.n = res & 0x80;
	status.z = res == 0;
	return false;
}

bool Cpu::RTI()
{
	status.reg = ReadStack();
	status.b = false;
	status.u = true;
	pc = ReadStack();
	pc |= ReadStack() << 8;
	return false;
}

bool Cpu::RTS()
{
	pc = ReadStack();
	pc |= ReadStack() << 8;
	pc++;
	return false;
}

bool Cpu::SBC()
{
	ReadAddr();
	data = ~data;
	uint8_t res = ra + data + status.c;
	status.c = static_cast<uint16_t>(ra) + data + status.c > 255;
	status.z = res == 0;
	status.n = res & 0x80;
	status.v = ((~(ra ^ data) & (ra ^ res)) & 0x80) >> 7;
	ra = res;
	return true;
}

bool Cpu::STA()
{
	nes.CpuWrite(addr, ra);
	return false;
}

bool Cpu::TXS()
{
	sp = rx;
	return false;
}

bool Cpu::TSX()
{
	rx = sp;
	status.n = rx & 0x80;
	status.z = rx == 0;
	return false;
}

bool Cpu::PHA()
{
	WriteStack(ra);
	return false;
}

bool Cpu::PLA()
{
	ra = ReadStack();
	status.z = ra == 0;
	status.n = ra & 0x80;
	return false;
}

bool Cpu::PHP()
{
	WriteStack(status.reg | 0b0001'0000);
	return false;
}

bool Cpu::PLP()
{
	status.reg = ReadStack();
	status.b = false;
	status.u = true;
	return false;
}

bool Cpu::STX()
{
	nes.CpuWrite(addr, rx);
	return false;
}

bool Cpu::STY()
{
	nes.CpuWrite(addr, ry);
	return false;
}

// Illegal opcodes

bool Cpu::AAC()
{
	AND();
	status.c = status.z;
	return false;
}

bool Cpu::SAX()
{
	uint8_t res = rx & ra;
	nes.CpuWrite(addr, res);
	return false;
}

bool Cpu::ARR()
{
	ReadAddr();
	ra &= data;
	ra = (ra >> 1) | (ra << 7);
	status.n = ra & 0x80;
	status.z = ra == 0;
	bool b5 = ra & 0b0010'0000;
	bool b6 = ra & 0b0100'0000;
	status.c = b6;
	status.v = b5 ^ b6;
	return false;
}

bool Cpu::ASR()
{
	ReadAddr();
	ra &= data;
	status.c = ra & 1;
	ra >>= 1;
	status.n = ra & 0x80;
	status.z = ra == 0;
	return false;
}

bool Cpu::ATX()
{
	ReadAddr();
	ra &= data;
	rx = ra;
	status.n = rx & 0x80;
	status.z = rx == 0;
	return false;
}

bool Cpu::AXA()
{
	nes.CpuWrite(addr, ra & rx & 7);
	return false;
}

bool Cpu::AXS()
{
	ReadAddr();
	rx &= ra;
	status.c = rx < data;
	rx -= data;
	status.n = rx & 0x80;
	status.z = rx == 0;
	return false;
}

bool Cpu::DCP()
{
	DEC();
	data--;
	CMP();
	return false;
}

bool Cpu::DOP()
{
	return false;
}

bool Cpu::ISB()
{
	ReadAddr();
	data++;
	nes.CpuWrite(addr, data);
	SBC();
	return false;
}

bool Cpu::KIL()
{
	pc--;
	return false;
}

bool Cpu::LAR()
{
	ReadAddr();
	ra = rx = sp = data & sp;
	status.n = ra & 0x80;
	status.z = ra == 0;
	return true;
}

bool Cpu::LAX()
{
	ReadAddr();
	ra = rx = data;
	status.n = ra & 0x80;
	status.z = ra == 0;
	return true;
}

bool Cpu::RLA()
{
	ReadAddr();
	uint8_t tmp = (data << 1) | static_cast<uint8_t>(status.c);
	ROL();
	data = tmp;
	AND();
	return false;
}

bool Cpu::RRA()
{
	ReadAddr();
	uint8_t tmp = (data >> 1) | (status.c << 7);
	ROR();
	data = tmp;
	ADC();
	return false;
}

bool Cpu::SLO()
{
	ReadAddr();
	uint8_t tmp = data << 1;
	ASL();
	data = tmp;
	ORA();
	return false;
}

bool Cpu::SRE()
{
	ReadAddr();
	uint8_t tmp = data >> 1;
	LSR();
	data = tmp;
	EOR();
	return false;
}

bool Cpu::SXA()
{
	nes.CpuWrite(addr, ((addr >> 8) & rx) + 1);
	return false;
}

bool Cpu::SYA()
{
	nes.CpuWrite(addr, ((addr >> 8) & ry) + 1);
	return false;
}

bool Cpu::TOP()
{
	return true;
}

bool Cpu::XAA()
{
	// Unpredictable and should not be used
	ReadAddr();
	ra &= rx & data;
	return false;
}

bool Cpu::XAS()
{
	sp = rx & ra;
	nes.CpuWrite(addr, ((addr >> 8) & rx) + 1);
	return false;
}