#pragma once
#include "Mapper.h"

class Mapper000 : public Mapper
{
public:
	Mapper000(int mapperNumber, int prgChunks, int chrChunks, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr);
	Mapper000(Snapshot& bytes, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr);
	bool MapCpuRead(uint16_t& addr, uint8_t& data, bool readonly = false) override;
	bool MapCpuWrite(uint16_t& addr, uint8_t data) override;
};
