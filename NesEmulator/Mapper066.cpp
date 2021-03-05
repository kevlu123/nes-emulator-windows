#include "Mapper066.h"

Mapper066::Mapper066(int mapperNumber, int prgChunks, int chrChunks, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr) :
	Mapper(mapperNumber, prgChunks, chrChunks, prg, chr)
{
	Reset();
}

Mapper066::Mapper066(Snapshot& bytes, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr) :
	Mapper(bytes, prg, chr)
{
	LoadBytes(bytes, prgBank);
	prgBank &= 0b11;
	LoadBytes(bytes, chrBank);
	chrBank &= 0b11;
}

Snapshot Mapper066::SaveState() const
{
	auto bytes = Mapper::SaveState();
	SaveBytes(bytes, prgBank);
	SaveBytes(bytes, chrBank);
	return bytes;
}

void Mapper066::Reset()
{
	prgBank = 0;
	chrBank = 0;
}

bool Mapper066::MapCpuRead(uint16_t& addr, uint8_t& data, bool readonly)
{
	uint32_t newAddr;
	if (addr >= 0x8000)
		newAddr = 0x8000 + prgBank * 0x8000 + (addr & 0x7FFF);
	else
		newAddr = addr;
	return Mapper::MapCpuRead(newAddr, data);
}

bool Mapper066::MapCpuWrite(uint16_t& addr, uint8_t data)
{
	if (addr >= 0x8000)
	{
		prgBank = (data >> 4) & 0b11;
		chrBank = data & 0b11;
	}
	return false;
}

bool Mapper066::MapPpuRead(uint16_t& addr, uint8_t& data, bool readonly)
{
	if (addr < 0x2000)
	{
		auto newAddr = chrBank * 0x2000 + (addr & 0x1FFF);
		if (newAddr < chr.size())
		{
			data = chr[newAddr];
			return true;
		}
	}
	return false;
}

bool Mapper066::MapPpuWrite(uint16_t& addr, uint8_t data)
{
	return false;
}
