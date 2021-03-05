#pragma once
#include <cstdint>
#include <vector>
#include "SaveStateUtil.h"

enum class MirrorMode
{
	Hardwired,
	Horizontal,
	Vertical,
	OneScreenLo,
	OneScreenHi,
};

class Mapper
{
	friend class Emulator;
public:
	virtual ~Mapper() = default;
	virtual Snapshot SaveState() const;
	int MapperNumber() const;

	virtual bool MapCpuRead(uint16_t& addr, uint8_t& data, bool readonly = false);
	virtual bool MapCpuWrite(uint16_t& addr, uint8_t data);
	virtual bool MapPpuRead(uint16_t& addr, uint8_t& data, bool readonly = false);
	virtual bool MapPpuWrite(uint16_t& addr, uint8_t data);
	virtual void Reset();
	virtual MirrorMode GetMirrorMode() const;
	virtual bool GetIrq() const;
	virtual void ClearIrq();
	virtual void CountScanline();
	virtual const std::vector<uint8_t>* GetSRam() const;
	virtual void SetSRam(const std::vector<uint8_t>& data);
protected:
	Mapper(int mapperNumber, int prgChunks, int chrChunks, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr);
	Mapper(Snapshot& bytes, std::vector<uint8_t>& prg, std::vector<uint8_t>& chr);
	bool MapCpuRead(uint32_t addr, uint8_t& data);
	bool MapCpuWrite(uint32_t addr, uint8_t data);
	std::vector<uint8_t>& prg;
	std::vector<uint8_t>& chr;
	int mapperNumber;
	int prgChunks;
	int chrChunks;
};
