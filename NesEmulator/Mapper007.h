#pragma once
#include "Mapper.h"

class Mapper007 : public Mapper
{
public:
	Mapper007(int mapperNumber, int prgChunks, int chrChunks, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr);
	Mapper007(Snapshot& bytes, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr);
	bool MapCpuRead(uint16_t& addr, uint8_t& data, bool readonly = false) override;
	bool MapCpuWrite(uint16_t& addr, uint8_t data) override;
	Snapshot SaveState() const override;
	void Reset() override;
	MirrorMode GetMirrorMode() const override;
private:
	int prgBank;
	MirrorMode mirrorMode;
};
