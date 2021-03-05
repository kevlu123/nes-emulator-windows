#include "Mapper000.h"

Mapper000::Mapper000(int mapperNumber, int prgChunks, int chrChunks, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr) :
	Mapper(mapperNumber, prgChunks, chrChunks, prg, chr)
{
}

Mapper000::Mapper000(Snapshot& bytes, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr) :
	Mapper(bytes, prg, chr)
{
}

bool Mapper000::MapCpuRead(uint16_t& addr, uint8_t& data, bool readonly)
{
	uint32_t newAddr;
	if (addr >= 0x8000)
		newAddr = 0x8000 + (prgChunks > 1 ? (addr & 0x7FFF) : (addr & 0x3FFF));
	else
		newAddr = addr;
	return Mapper::MapCpuRead(newAddr, data);
}

bool Mapper000::MapCpuWrite(uint16_t& addr, uint8_t data)
{
	return false;
}
