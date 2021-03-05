#pragma once
#include "Cpu.h"
#include "Ppu.h"
#include "Apu.h"
#include "Cartridge.h"
#include "Controller.h"
#include <atomic>
#include <thread>
#include <mutex>
#include "Timer.h"
#include "SaveStateUtil.h"

class Nes
{
	friend class Emulator;
public:
	Nes(const std::wstring& sramPath);
	Nes(const Nes&) = delete;
	Nes& operator=(const Nes&) = delete;
	~Nes();
	void Reset();
	void RemoveCartridge();
	void InsertCartridge(std::shared_ptr<Cartridge> cart);
	void SetController(int port, std::unique_ptr<Controller> controller);
	void UnplugController(int port);
	uint8_t CpuRead(uint16_t addr, bool readonly = false);
	void CpuWrite(uint16_t addr, uint8_t data);
	bool GetCurrentSprites(int scanline, uint8_t spriteSize, ObjectAttributeMemory out[8], int& spriteCount, bool& sprite0Loaded, std::array<bool, 64>& currentSpriteNumbers) const;
	uint8_t ReadOAM() const;
	void SetOAMAddr(uint8_t addr);
	void WriteOAM(uint8_t data);
	void Reposition(int x, int y);
	void DoIteration();
	void StartAsync();
	void StopAsync();
	int GetEmulationSpeed() const;
	void SetEmulationSpeed(int speed);
	bool NotRunning() const;
	std::vector<uint8_t> SaveState() const;
	void LoadState(const std::wstring& filename, std::vector<uint8_t>& bytes);
	bool SaveSRam() const;
	int drawXOffset = 0;
	int drawYOffset = 0;
	std::atomic_bool offDisplay = false;
	std::atomic_bool frameComplete = true;
	std::atomic_bool running = true;
	std::atomic_int saveLoadState = 0;
	std::atomic_bool masterBg = true;
	std::atomic_bool masterFg = true;
	std::atomic_bool mute = false;

	void Clock();
	void ClockCpuInstruction();
	void ClockFrame();
private:
	std::wstring sramPath;
	int emuStep = 0;
	int clockNumber = 0;
	std::shared_ptr<Cartridge> cart;
	std::unique_ptr<Cpu> cpu;
	std::unique_ptr<Ppu> ppu;
	std::shared_ptr<Apu> apu;
	uint8_t ram[0x800];
	std::unique_ptr<Controller> controllers[2];
	uint8_t controllerLatch = 0;

	// Sprites
	void ClockDMA();
	ObjectAttributeMemory oam[64];
	uint8_t oamAddr = 0;
	uint16_t dmaAddr = 0;
	uint8_t dmaData = 0;
	bool dmaReady = false;
	bool dmaMode = false;

	// Multi-threading
	std::atomic_bool quit = false;
	std::mutex stateMtx;
	std::thread thrd;
	std::atomic_int emulationSpeed = 1;
	void RunAsync();
};
