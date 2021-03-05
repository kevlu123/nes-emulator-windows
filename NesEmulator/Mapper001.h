#pragma once
#include "Mapper.h"

class Mapper001 : public Mapper
{
public:
	Mapper001(int mapperNumber, int prgChunks, int chrChunks, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr);
	Mapper001(Snapshot& bytes, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr);
	bool MapCpuRead(uint16_t& addr, uint8_t& data, bool readonly = false) override;
	bool MapCpuWrite(uint16_t& addr, uint8_t data) override;
	bool MapPpuRead(uint16_t& addr, uint8_t& data, bool readonly = false) override;
	bool MapPpuWrite(uint16_t& addr, uint8_t data) override;
	Snapshot SaveState() const override;
	void Reset() override;
	MirrorMode GetMirrorMode() const override;
	const std::vector<uint8_t>* GetSRam() const override;
	void SetSRam(const std::vector<uint8_t>& data) override;
private:
	bool MapPpuAddr(uint16_t& addr, uint32_t& newAddr) const;
	uint8_t shift;
	uint8_t chrLo;
	uint8_t chrHi;
	uint8_t prgLo;
	union
	{
		struct
		{
			uint8_t mirror : 2;
			uint8_t prgBankMode : 2;
			uint8_t chrBankMode : 1;
		};
		uint8_t reg;
	} ctrl;
	std::vector<uint8_t> sram;
};
