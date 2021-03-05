#pragma once
#include "Mapper.h"

class Mapper004 : public Mapper
{
public:
	Mapper004(int mapperNumber, int prgChunks, int chrChunks, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr);
	Mapper004(Snapshot& bytes, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr);
	bool MapCpuRead(uint16_t& addr, uint8_t& data, bool readonly = false) override;
	bool MapCpuWrite(uint16_t& addr, uint8_t data) override;
	bool MapPpuRead(uint16_t& addr, uint8_t& data, bool readonly = false) override;
	bool MapPpuWrite(uint16_t& addr, uint8_t data) override;
	Snapshot SaveState() const override;
	MirrorMode GetMirrorMode() const override;
	void Reset() override;
	bool GetIrq() const override;
	void ClearIrq() override;
	void CountScanline() override;
	const std::vector<uint8_t>* GetSRam() const override;
	void SetSRam(const std::vector<uint8_t>& data) override;
private:
	bool MapPpuAddr(uint16_t& addr, uint32_t& newAddr) const;
	int lastPrgBankNumber;
	std::vector<uint8_t> sram;
	uint8_t regs[8];
	union
	{
		struct
		{
			uint8_t regNumber : 3;
			uint8_t unused : 3;
			uint8_t prgMode : 1;
			uint8_t chrMode : 1;
		};
		uint8_t reg;
	} bankSelect;
	MirrorMode mirrorMode;
	int irqReload;
	int irqCounter;
	bool irqEnabled;
	bool irqState;
	bool reloadPending;
};
