#include "Mapper003.h"

Mapper003::Mapper003(int mapperNumber, int prgChunks, int chrChunks, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr) :
	Mapper(mapperNumber, prgChunks, chrChunks, prg, chr),
	chrBank(0)
{
}

Mapper003::Mapper003(Snapshot& bytes, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr) :
	Mapper(bytes, prg, chr)
{
	LoadBytes(bytes, chrBank);
	chrBank &= 0b11;
}

Snapshot Mapper003::SaveState() const
{
	auto bytes = Mapper::SaveState();
	SaveBytes(bytes, chrBank);
	return bytes;
}

void Mapper003::Reset()
{
	chrBank = 0;
}

bool Mapper003::MapCpuRead(uint16_t& addr, uint8_t& data, bool readonly)
{
	uint32_t newAddr;
	if (addr >= 0x8000)
		newAddr = 0x8000 + (prgChunks > 1 ? (addr & 0x7FFF) : (addr & 0x3FFF));
	else
		newAddr = addr;
	return Mapper::MapCpuRead(newAddr, data);
}

bool Mapper003::MapCpuWrite(uint16_t& addr, uint8_t data)
{
	if (addr >= 0x8000)
		chrBank = data & 0b11;
	return false;
}

bool Mapper003::MapPpuRead(uint16_t& addr, uint8_t& data, bool readonly)
{
	if (addr < 0x2000)
	{
		auto newAddr = (chrBank * 0x2000) + (addr & 0x1FFF);
		if (newAddr < chr.size())
		{
			data = chr[newAddr];
			return true;
		}
	}
	return false;
}

bool Mapper003::MapPpuWrite(uint16_t& addr, uint8_t data)
{
	return false;
}
