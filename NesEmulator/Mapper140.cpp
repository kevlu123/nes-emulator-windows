#include "Mapper140.h"

Mapper140::Mapper140(int mapperNumber, int prgChunks, int chrChunks, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr) :
	Mapper(mapperNumber, prgChunks, chrChunks, prg, chr)
{
	Reset();
}

Mapper140::Mapper140(Snapshot& bytes, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr) :
	Mapper(bytes, prg, chr)
{
	LoadBytes(bytes, prgBank);
	prgBank &= 0b11;
	LoadBytes(bytes, chrBank);
	chrBank &= 0b11;
}

Snapshot Mapper140::SaveState() const
{
	auto bytes = Mapper::SaveState();
	SaveBytes(bytes, prgBank);
	SaveBytes(bytes, chrBank);
	return bytes;
}

void Mapper140::Reset()
{
	prgBank = prgChunks / 2 - 1;
	chrBank = 0;
}

bool Mapper140::MapCpuRead(uint16_t& addr, uint8_t& data, bool readonly)
{
	uint32_t newAddr;
	if (addr >= 0x8000)
		newAddr = 0x8000 + prgBank * 0x8000 + (addr & 0x7FFF);
	else
		newAddr = addr;
	return Mapper::MapCpuRead(newAddr, data);
}

bool Mapper140::MapCpuWrite(uint16_t& addr, uint8_t data)
{
	if (addr >= 0x6000 && addr < 0x8000)
	{
		prgBank = (data >> 4) & 0b11;
		chrBank = data & 0b11;
	}
	return false;
}

bool Mapper140::MapPpuRead(uint16_t& addr, uint8_t& data, bool readonly)
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

bool Mapper140::MapPpuWrite(uint16_t& addr, uint8_t data)
{
	return false;
}
