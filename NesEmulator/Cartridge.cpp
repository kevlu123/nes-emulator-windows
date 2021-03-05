#include "Cartridge.h"
#include <fstream>
#include <filesystem>
#include "EmuFileException.h"
#include "Mapper000.h"
#include "Mapper001.h"
#include "Mapper002.h"
#include "Mapper003.h"
#include "Mapper004.h"
#include "Mapper007.h"
#include "Mapper066.h"
#include "Mapper140.h"

Cartridge::Cartridge(const std::wstring& sramPath, const std::wstring& filename) :
	filename(filename),
	sramPath(sramPath)
{
	// Load file
	std::ifstream f(filename, std::ios::binary | std::ios::ate);
	if (!f.is_open())
		throw EmuFileException("could not open file");
	size_t len = static_cast<size_t>(f.tellg());
	std::vector<uint8_t> bytes(len);
	f.seekg(0, std::ios::beg);
	f.read(reinterpret_cast<char*>(bytes.data()), len);
	if (f.bad() || f.fail())
		throw EmuFileException("error reading file");
	f.close();

	// Extract header
	if (bytes.size() < sizeof(header))
		throw EmuFileException("invalid file format");
	std::memcpy(&header, bytes.data(), sizeof(header));
	if (std::string(reinterpret_cast<char*>(header.signature), 4) != "NES\x1A")
		throw EmuFileException("invalid file format");
	bytes.erase(bytes.begin(), bytes.begin() + sizeof(header));

	// Extract prg
	if (bytes.size() < static_cast<size_t>(header.prgChunks * 0x4000))
		throw EmuFileException("expected more PRG");
	prg.resize(header.prgChunks * 16384);
	std::memcpy(prg.data(), bytes.data(), header.prgChunks * 0x4000);
	bytes.erase(bytes.begin(), bytes.begin() + header.prgChunks * 0x4000);

	// Extract chr
	if (header.chrChunks == 0)
	{
		header.chrChunks = 4;
		chr.resize(header.chrChunks * 8192);
	}
	else
	{
		if (bytes.size() < static_cast<size_t>(header.chrChunks * 0x2000))
			throw EmuFileException("expected more CHR");
		chr.resize(header.chrChunks * 8192);
		std::memcpy(chr.data(), bytes.data(), header.chrChunks * 0x2000);
		bytes.erase(bytes.begin(), bytes.begin() + header.chrChunks * 0x2000);
	}

	// Select mapper
	if (!header.padding[3] && !header.padding[4] && !header.padding[5] && !header.padding[6])
		LoadMapper((header.flag7.upperMapperNumber << 4) | header.flag6.lowerMapperNumber);
	else
		LoadMapper(header.flag6.lowerMapperNumber);

	// Load sram
	std::ifstream sf(sramPath, std::ios::binary | std::ios::ate);
	if (!sf.is_open())
		return;
	len = (size_t)sf.tellg();
	sf.seekg(0);
	std::vector<uint8_t> sram(len);
	sf.read(reinterpret_cast<char*>(sram.data()), len);
	if (sf.bad() || sf.fail())
		return;
	mapper->SetSRam(sram);
}

Cartridge::Cartridge(const std::wstring& sramPath, const std::wstring& filename, Snapshot& bytes) :
	filename(filename),
	sramPath(sramPath)
{
	int mapperNumber = 0;
	LoadBytes(bytes, mapperNumber);
	LoadMapper(mapperNumber, &bytes);

	LoadBytes(bytes, header);

	size_t vecLen;
	LoadBytes(bytes, vecLen);
	if (vecLen > 1024 * 1024 * 512)
		throw EmuFileException("invalid file");
	prg.resize(vecLen);
	LoadBytes(bytes, prg.data(), prg.size());

	LoadBytes(bytes, vecLen);
	if (vecLen > 1024 * 1024 * 512)
		throw EmuFileException("invalid file");
	chr.resize(vecLen);
	LoadBytes(bytes, chr.data(), chr.size());
}

Cartridge::~Cartridge()
{
	SaveSRam();
}

Snapshot Cartridge::SaveState() const
{
	std::vector<uint8_t> bytes;
	SaveBytes(bytes, mapper->MapperNumber());
	AppendVector(bytes, mapper->SaveState());

	SaveBytes(bytes, header);

	size_t vecLen = prg.size();
	SaveBytes(bytes, vecLen);
	SaveBytes(bytes, prg.data(), prg.size());

	vecLen = chr.size();
	SaveBytes(bytes, vecLen);
	SaveBytes(bytes, chr.data(), chr.size());

	return bytes;
}

void Cartridge::Reset()
{
	mapper->Reset();
}

void Cartridge::LoadMapper(int mapperNumber, Snapshot* bytes)
{
#define Load(type) mapper = bytes ? std::make_unique<type>(*bytes, prg, chr) : std::make_unique<type>(mapperNumber, header.prgChunks, header.chrChunks, prg, chr)
	switch (mapperNumber)
	{
	case   0: Load(Mapper000); break;
	case   1: Load(Mapper001); break;
	case   2: Load(Mapper002); break;
	case   3: Load(Mapper003); break;
	case   4: Load(Mapper004); break;
	case   7: Load(Mapper007); break;
	case  66: Load(Mapper066); break;
	case 140: Load(Mapper140); break;
	default: throw EmuFileException("unsupported mapper " + std::to_string(mapperNumber));
	}
#undef Load
}

bool Cartridge::CpuWrite(uint16_t& addr, uint8_t data)
{
	return mapper->MapCpuWrite(addr, data);
}

bool Cartridge::CpuRead(uint16_t& addr, uint8_t& data, bool readonly)
{
	return mapper->MapCpuRead(addr, data, readonly);
}

bool Cartridge::PpuWrite(uint16_t& addr, uint8_t data)
{
	return mapper->MapPpuWrite(addr, data);
}

bool Cartridge::PpuRead(uint16_t& addr, uint8_t& data, bool readonly)
{
	return mapper->MapPpuRead(addr, data, readonly);
}

MirrorMode Cartridge::GetMirrorMode() const
{
	auto mode = mapper->GetMirrorMode();
	if (mode == MirrorMode::Hardwired)
		return header.flag6.mirroring ? MirrorMode::Vertical : MirrorMode::Horizontal;
	else
		return mode;
}

Mapper& Cartridge::GetMapper()
{
	return *mapper;
}

const Mapper& Cartridge::GetMapper() const
{
	return *mapper;
}

bool Cartridge::SaveSRam() const
{
	const auto* sram = GetMapper().GetSRam();
	if (sram == nullptr || sram->size() == 0)
		return true;

	auto GetDirectory = [](const std::wstring& path)
	{
		return path.substr(0, path.find_last_of(L"/\\"));
	};
	std::error_code ec;
	std::filesystem::create_directory(GetDirectory(sramPath), ec);

	std::ofstream f(sramPath, std::ios::binary);
	if (!f.is_open())
		return false;

	f.write(reinterpret_cast<const char*>(sram->data()), sram->size());
	return !(f.bad() || f.fail());
}
