#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include "Mapper.h"
#include "SaveStateUtil.h"

class Cartridge
{
	friend class Emulator;
public:
	Cartridge(const std::wstring& sramPath, const std::wstring& filename);
	Cartridge(const std::wstring& sramPath, const std::wstring& filename, Snapshot& bytes);
	Cartridge(const Cartridge&) = delete;
	Cartridge& operator=(const Cartridge&) = delete;
	~Cartridge();
	void Reset();
	bool CpuWrite(uint16_t& addr, uint8_t data);
	bool CpuRead(uint16_t& addr, uint8_t& data, bool readonly);
	bool PpuWrite(uint16_t& addr, uint8_t data);
	bool PpuRead(uint16_t& addr, uint8_t& data, bool readonly);
	MirrorMode GetMirrorMode() const;
	Mapper& GetMapper();
	const Mapper& GetMapper() const;
	Snapshot SaveState() const;
	bool SaveSRam() const;
	const std::wstring filename;
private:
	void LoadMapper(int mapperNumber, Snapshot* bytes = nullptr);
	std::unique_ptr<Mapper> mapper;
	struct Header
	{
		uint8_t signature[4];
		uint8_t prgChunks;
		uint8_t chrChunks;
		struct
		{
			uint8_t mirroring : 1;
			uint8_t batteryBacked : 1;
			uint8_t unknown2 : 1;
			uint8_t unknown3 : 1;
			uint8_t lowerMapperNumber : 4;
		} flag6;
		struct
		{
			uint8_t unknown0 : 1;
			uint8_t unknown1 : 1;
			uint8_t unknown2 : 2;
			uint8_t upperMapperNumber : 4;
		} flag7;
		uint8_t prgRamSize;
		uint8_t padding[7];
	} header;
	std::vector<uint8_t> prg;
	std::vector<uint8_t> chr;
	std::wstring sramPath;
};
