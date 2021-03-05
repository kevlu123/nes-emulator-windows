#pragma once
#include "Mapper.h"

class Mapper066 : public Mapper
{
public:
	Mapper066(int mapperNumber, int prgChunks, int chrChunks, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr);
	Mapper066(Snapshot& bytes, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr);
	bool MapCpuRead(uint16_t& addr, uint8_t& data, bool readonly = false) override;
	bool MapCpuWrite(uint16_t& addr, uint8_t data) override;
	bool MapPpuRead(uint16_t& addr, uint8_t& data, bool readonly = false) override;
	bool MapPpuWrite(uint16_t& addr, uint8_t data) override;
	Snapshot SaveState() const override;
	void Reset() override;
private:
	int prgBank;
	int chrBank;
};
