#pragma once
#include <cstdint>
#include <memory>
#include "Cartridge.h"
#include <atomic>
#include "SaveStateUtil.h"
#include <array>

class Nes;
void DrawPixel(int x, int y, uint8_t c);

struct ObjectAttributeMemory
{
	uint8_t y = 0xFF;
	uint8_t tileID = 0;
	union
	{
		struct
		{
			uint8_t palette : 2;
			uint8_t unused : 3;
			bool priority : 1;
			bool flipHorizontally : 1;
			bool flipVertically : 1;
		};
		uint8_t flags;
	};
	uint8_t x = 0;
};

class Ppu
{
	friend class Emulator;
public:
	Ppu(Nes& nes, std::shared_ptr<Cartridge> cart);
	Ppu(Nes& nes, std::shared_ptr<Cartridge> cart, Snapshot& bytes);
	void Reset();
	void Clock();
	void WriteFromCpu(uint16_t addr, uint8_t data);
	uint8_t ReadFromCpu(uint16_t addr, bool readonly = false);
	bool CheckNmi();
	bool IsBeginningFrame() const;
	void Reposition(int x, int y);
	Snapshot SaveState() const;
	void ClearCurrentSpriteNumbers();
	static constexpr int DRAWABLE_WIDTH = 256;
	static constexpr int DRAWABLE_HEIGHT = 240;
	static constexpr int DOT_COUNT = 341;
	static constexpr int SCANLINE_COUNT = 262;
	static uint8_t* GetPowerOffScreen();
private:
	std::atomic_int drawXOffset;
	std::atomic_int drawYOffset;

	// Private bus
	void PpuWrite(uint16_t addr, uint8_t data);
	uint8_t PpuRead(uint16_t addr, bool readonly = false);

	static constexpr int PRE_RENDER_SCANLINE = SCANLINE_COUNT - 1;
	static constexpr int POST_RENDER_SCANLINE = DRAWABLE_HEIGHT;

	Nes& nes;
	std::shared_ptr<Cartridge> cart;
	uint8_t nameTables[2][0x400];
	uint8_t palettes[8][4];

	void OutputColour();
	void LoadBackgroundShifters();
	void UpdateShifters();
	int dot = DOT_COUNT - 1;
	int scanline = PRE_RENDER_SCANLINE;
	bool latch = false;
	uint8_t delayedDataBuffer = 0;
	bool nmi = false;
	bool oddFrame = false;
	bool isDrawing = false;
	uint8_t colourOutput = 0x3F;
	uint8_t busData = 0;

	// Background data
	uint8_t bgPaletteNumber = 0;
	uint8_t bgPaletteIndex = 0;
	uint8_t bgNextTileID = 0;
	uint16_t patternAddr = 0;
	uint16_t bgPatternShifterLo = 0;
	uint16_t bgPatternShifterHi = 0;
	uint16_t bgAttributeShifterLo = 0;
	uint16_t bgAttributeShifterHi = 0;
	uint8_t bgNextPatternShifterLo = 0;
	uint8_t bgNextPatternShifterHi = 0;
	uint8_t bgNextAttributeShifterLo = 0;
	uint8_t bgNextAttributeShifterHi = 0;

	// Registers
	union
	{
		struct
		{
			uint8_t nametableNumber : 2;
			uint8_t incrMode : 1;
			uint8_t spritePatternTable : 1;
			uint8_t backgroundPatternTable : 1;
			uint8_t spriteSize : 1;
			uint8_t unusedSlaveMode : 1;
			bool nmiAtVBlank : 1;
		};
		uint8_t reg = 0;
	} ctrl;
	union
	{
		struct
		{
			bool greyScale : 1;
			bool drawLeftBackground : 1;
			bool drawLeftSprite : 1;
			bool backgroundEnabled : 1;
			bool spriteEnabled : 1;
			bool redEmphasis : 1;
			bool greenEmphasis : 1;
			bool blueEmphasis : 1;
		};
		uint8_t reg = 0;
	} mask;
	union
	{
		struct
		{
			uint8_t unused : 5;
			bool spriteOverflow : 1;
			bool sprite0Hit : 1;
			bool vblank : 1;
		};
		uint8_t reg = 0;
	} status;
	union
	{
		struct
		{
			uint16_t coarseX : 5;
			uint16_t coarseY : 5;
			uint16_t nametableX : 1;
			uint16_t nametableY : 1;
			uint16_t fineY : 3;
			uint16_t unusedScroll : 1;
		};
		struct
		{
			uint8_t lo : 8;
			uint8_t hi : 6;
			uint8_t unusedAddr0 : 2;
		};
		struct
		{
			uint16_t addr : 12;
			uint16_t unusedAddr1 : 4;
		};
		uint16_t reg = 0;
	} vramAddr, tramAddr;
	uint8_t scrollFineX = 0;

	// Sprites
	ObjectAttributeMemory currentSprites[8];
	std::array<bool, 64> currentSpriteNumbers;
	int spriteCount = 0;
	bool sprite0Loaded = false;
	uint8_t spritePatternShifterLo[8];
	uint8_t spritePatternShifterHi[8];
};
