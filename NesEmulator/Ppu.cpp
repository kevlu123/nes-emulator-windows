#include "Ppu.h"
#include "Nes.h"
#include <cstdlib>

Ppu::Ppu(Nes& nes, std::shared_ptr<Cartridge> cart) :
	nes(nes),
	cart(cart),
	drawXOffset(0),
	drawYOffset(0),
	currentSpriteNumbers{}
{
	std::memset(palettes, 0x3F, sizeof(palettes));
	std::memset(nameTables, 0, sizeof(nameTables));
	std::memset(currentSprites, 0xFF, sizeof(currentSprites));
	std::memset(spritePatternShifterLo, 0, sizeof(spritePatternShifterLo));
	std::memset(spritePatternShifterHi, 0, sizeof(spritePatternShifterHi));
	status.reg = 0;
	vramAddr.reg = 0;
	tramAddr.reg = 0;
}

Ppu::Ppu(Nes& nes, std::shared_ptr<Cartridge> cart, Snapshot& bytes) :
	nes(nes),
	cart(cart),
	drawXOffset(0),
	drawYOffset(0),
	currentSpriteNumbers{}
{
	LoadBytes(bytes, (uint8_t*)nameTables, std::size(nameTables) * std::size(nameTables[0]));
	LoadBytes(bytes, (uint8_t*)palettes, std::size(palettes) * std::size(palettes[0]));

	LoadBytes(bytes, dot);
	LoadBytes(bytes, scanline);
	LoadBytes(bytes, latch);
	LoadBytes(bytes, delayedDataBuffer);
	LoadBytes(bytes, nmi);
	LoadBytes(bytes, oddFrame);
	LoadBytes(bytes, isDrawing);
	LoadBytes(bytes, colourOutput);

	LoadBytes(bytes, bgPaletteNumber);
	LoadBytes(bytes, bgPaletteIndex);
	LoadBytes(bytes, bgNextTileID);
	LoadBytes(bytes, patternAddr);
	LoadBytes(bytes, bgPatternShifterLo);
	LoadBytes(bytes, bgPatternShifterHi);
	LoadBytes(bytes, bgAttributeShifterLo);
	LoadBytes(bytes, bgAttributeShifterHi);
	LoadBytes(bytes, bgNextPatternShifterLo);
	LoadBytes(bytes, bgNextPatternShifterHi);
	LoadBytes(bytes, bgNextAttributeShifterLo);
	LoadBytes(bytes, bgNextAttributeShifterHi);

	LoadBytes(bytes, ctrl);
	LoadBytes(bytes, mask);
	LoadBytes(bytes, status);
	LoadBytes(bytes, vramAddr);
	LoadBytes(bytes, tramAddr);
	LoadBytes(bytes, scrollFineX);
	LoadBytes(bytes, busData);

	LoadBytes(bytes, currentSprites, std::size(currentSprites));
	LoadBytes(bytes, spritePatternShifterLo, std::size(spritePatternShifterLo));
	LoadBytes(bytes, spritePatternShifterHi, std::size(spritePatternShifterHi));
	LoadBytes(bytes, spriteCount);
	LoadBytes(bytes, sprite0Loaded);
}

Snapshot Ppu::SaveState() const
{
	std::vector<uint8_t> bytes;
	SaveBytes(bytes, (uint8_t*)nameTables, std::size(nameTables) * std::size(nameTables[0]));
	SaveBytes(bytes, (uint8_t*)palettes, std::size(palettes) * std::size(palettes[0]));

	SaveBytes(bytes, dot);
	SaveBytes(bytes, scanline);
	SaveBytes(bytes, latch);
	SaveBytes(bytes, delayedDataBuffer);
	SaveBytes(bytes, nmi);
	SaveBytes(bytes, oddFrame);
	SaveBytes(bytes, isDrawing);
	SaveBytes(bytes, colourOutput);

	SaveBytes(bytes, bgPaletteNumber);
	SaveBytes(bytes, bgPaletteIndex);
	SaveBytes(bytes, bgNextTileID);
	SaveBytes(bytes, patternAddr);
	SaveBytes(bytes, bgPatternShifterLo);
	SaveBytes(bytes, bgPatternShifterHi);
	SaveBytes(bytes, bgAttributeShifterLo);
	SaveBytes(bytes, bgAttributeShifterHi);
	SaveBytes(bytes, bgNextPatternShifterLo);
	SaveBytes(bytes, bgNextPatternShifterHi);
	SaveBytes(bytes, bgNextAttributeShifterLo);
	SaveBytes(bytes, bgNextAttributeShifterHi);

	SaveBytes(bytes, ctrl);
	SaveBytes(bytes, mask);
	SaveBytes(bytes, status);
	SaveBytes(bytes, vramAddr);
	SaveBytes(bytes, tramAddr);
	SaveBytes(bytes, scrollFineX);
	SaveBytes(bytes, busData);

	SaveBytes(bytes, currentSprites);
	SaveBytes(bytes, spritePatternShifterLo);
	SaveBytes(bytes, spritePatternShifterHi);
	SaveBytes(bytes, spriteCount);
	SaveBytes(bytes, sprite0Loaded);
	return bytes;
}

bool Ppu::IsBeginningFrame() const
{
	return dot == 1 && scanline == 0;
}

uint8_t* Ppu::GetPowerOffScreen()
{
	static uint8_t bytes[Ppu::DRAWABLE_WIDTH * Ppu::DRAWABLE_HEIGHT];
	static bool init = false;
	if (!init)
	{
		for (int x = 0; x < Ppu::DRAWABLE_WIDTH; x++)
			for (int y = 0; y < Ppu::DRAWABLE_HEIGHT; y++)
				bytes[y * Ppu::DRAWABLE_WIDTH + x] = y / 22 + 0x31;
		init = true;
	}
	return bytes;
}

void Ppu::Reset()
{
	ctrl.reg = 0;
	mask.reg = 0;
	nes.SetOAMAddr(0);
	latch = false;
	oddFrame = false;
	scanline = PRE_RENDER_SCANLINE;
	dot = DOT_COUNT - 1;
}

void Ppu::Clock()
{
	if (scanline < DRAWABLE_HEIGHT || scanline == PRE_RENDER_SCANLINE)
	{
		if (scanline < DRAWABLE_HEIGHT && dot == 1)
		{
			// Drawable scanline start
			isDrawing = true;
		}
		if (scanline < DRAWABLE_HEIGHT && dot == DRAWABLE_WIDTH + 1)
		{
			// Drawable scanline end
			isDrawing = false;
		}

		if (scanline == PRE_RENDER_SCANLINE && dot == 1)
		{
			// Clear vblank, sprite 0 hit, and sprite overflow
			status.reg = 0;
			// Clear sprite shifters
			sprite0Loaded = false;
			std::memset(spritePatternShifterLo, 0, 8);
			std::memset(spritePatternShifterHi, 0, 8);
		}
		if (dot >= 2 && dot < 258 || dot >= 321 && dot < 338)
		{
			// Shift background and foreground shifters
			UpdateShifters();

			switch ((dot - 1) % 8)
			{
			case 0:
			{
				// Load current background shifters
				LoadBackgroundShifters();
				// Load next pattern table ID from nametable
				bgNextTileID = PpuRead(0x2000 + vramAddr.addr);
				break;
			}
			case 2:
			{
				// Load next attribute lo and hi bit for the next 8 pixels
				uint16_t attrAddr = 0x23C0 + (vramAddr.reg & 0b0000'1100'0000'0000) + ((vramAddr.coarseY >> 2) << 3) + (vramAddr.coarseX >> 2);
				uint8_t quadrant = ((vramAddr.coarseX & 2) >> 1) | (vramAddr.coarseY & 2);
				uint8_t paletteNumber = (PpuRead(attrAddr) >> (2 * quadrant)) & 0b0011;
				bgNextAttributeShifterLo = (paletteNumber & 0b01) * 0xFF;
				bgNextAttributeShifterHi = ((paletteNumber & 0b10) >> 1) * 0xFF;
				break;
			}
			case 4:
			{
				// Load next 8 lo pattern bits
				patternAddr = ctrl.backgroundPatternTable * 0x1000 + 16 * bgNextTileID + vramAddr.fineY;
				bgNextPatternShifterLo = PpuRead(patternAddr);
				break;
			}
			case 6:
			{
				// Load next 8 hi pattern bits
				bgNextPatternShifterHi = PpuRead(patternAddr + 8);
				break;
			}
			case 7:
				if (mask.backgroundEnabled || mask.spriteEnabled)
				{
					// Increment X
					vramAddr.coarseX++;
					if (vramAddr.coarseX == 0)
						vramAddr.nametableX = ~vramAddr.nametableX;
				}
				break;
			}
		}
		if (dot == DRAWABLE_WIDTH && (mask.backgroundEnabled || mask.spriteEnabled))
		{
			// Increment Y
			vramAddr.fineY++;
			if (vramAddr.fineY == 0)
			{
				vramAddr.coarseY++;
				if (vramAddr.coarseY == 30)
				{
					vramAddr.coarseY = 0;
					vramAddr.nametableY = ~vramAddr.nametableY;
				}
			}
		}
		if (dot == DRAWABLE_WIDTH + 1)
		{
			// Load background shifters
			LoadBackgroundShifters();
			// Reload X
			if (mask.backgroundEnabled || mask.spriteEnabled)
			{
				vramAddr.nametableX = tramAddr.nametableX;
				vramAddr.coarseX = tramAddr.coarseX;
			}
		}
		if (dot == 338 || dot == 340)
		{
			// Dummy reads
			bgNextTileID = PpuRead(0x2000 + vramAddr.addr);
		}
		if (scanline == PRE_RENDER_SCANLINE && dot >= 280 && dot < 305 && (mask.backgroundEnabled || mask.spriteEnabled))
		{
			// Reload Y
			vramAddr.nametableY = tramAddr.nametableY;
			vramAddr.coarseY = tramAddr.coarseY;
			vramAddr.fineY = tramAddr.fineY;
		}

		if (dot == DRAWABLE_WIDTH + 1)
		{
			// Clear sprite shifters
			std::memset(spritePatternShifterLo, 0, 8);
			std::memset(spritePatternShifterHi, 0, 8);
			// Sprite evaluation
			status.spriteOverflow = nes.GetCurrentSprites(scanline, ctrl.spriteSize, currentSprites, spriteCount, sprite0Loaded, currentSpriteNumbers);
		}
		if (dot >= DRAWABLE_WIDTH + 1 && dot < 321)
		{
			// Clear oam address
			nes.SetOAMAddr(0);
		}

		if (dot == DOT_COUNT - 1)
		{
			// Load sprite shifters
			uint16_t addr = 0;
			for (int i = 0; i < spriteCount; i++)
			{
				const auto& sprite = currentSprites[i];
				int yDiff = scanline - sprite.y;

				if (ctrl.spriteSize == 0)
				{
					// 8x8 sprites
					addr = ctrl.spritePatternTable * 0x1000 + sprite.tileID * 16;
					addr |= sprite.flipVertically ? (7 - yDiff) : yDiff;
				}
				else
				{
					// 8x16 sprites
					if ((yDiff < 8) != (sprite.flipVertically))
					{
						// First tile
						addr = (sprite.tileID & 1) * 0x1000 + (sprite.tileID & 0xFE) * 16;
					}
					else
					{
						// Second tile
						addr = (sprite.tileID & 1) * 0x1000 + (sprite.tileID | 1) * 16;
					}
					addr |= (sprite.flipVertically ? (7 - yDiff) : yDiff) & 0b0111;
				}

				uint8_t loBits = PpuRead(addr);
				uint8_t hiBits = PpuRead(addr + 8);

				// Flip horizontally
				if (sprite.flipHorizontally)
				{
					// Flip bytes
					loBits = (loBits & 0xF0) >> 4 | (loBits & 0x0F) << 4;
					loBits = (loBits & 0xCC) >> 2 | (loBits & 0x33) << 2;
					loBits = (loBits & 0xAA) >> 1 | (loBits & 0x55) << 1;
					hiBits = (hiBits & 0xF0) >> 4 | (hiBits & 0x0F) << 4;
					hiBits = (hiBits & 0xCC) >> 2 | (hiBits & 0x33) << 2;
					hiBits = (hiBits & 0xAA) >> 1 | (hiBits & 0x55) << 1;
				}

				spritePatternShifterLo[i] = loBits;
				spritePatternShifterHi[i] = hiBits;
			}
		}
	}
	if (scanline == POST_RENDER_SCANLINE + 1 && dot == 1)
	{
		// Set vblank
		status.vblank = true;
		if (ctrl.nmiAtVBlank)
			nmi = true;
	}

	OutputColour();

	// Update column and scanline
	dot++;
	if (dot == 260 && scanline < 240 && (mask.backgroundEnabled || mask.spriteEnabled))
		cart->GetMapper().CountScanline();
	if (dot == DOT_COUNT)
	{
		// Scanline start
		dot = 0;
		scanline++;
		if (scanline == SCANLINE_COUNT)
		{
			// Frame start
			scanline = 0;
			oddFrame = !oddFrame;
			if (oddFrame && (mask.backgroundEnabled || mask.spriteEnabled))
			{
				dot = 1;
			}
		}
	}
}

void Ppu::ClearCurrentSpriteNumbers()
{
	std::memset(currentSpriteNumbers.data(), false, currentSpriteNumbers.size());
}

uint8_t Ppu::PpuRead(uint16_t addr, bool readonly)
{
	addr &= 0b0011'1111'1111'1111;
	uint8_t data = 0;
	if (cart->PpuRead(addr, data, readonly))
	{
	}
	else if (addr >= 0x2000 && addr < 0x3F00)
	{
		switch (cart->GetMirrorMode())
		{
		case MirrorMode::Vertical:
			data = nameTables[(bool)(addr & 0b0100'0000'0000)][addr & 0b0011'1111'1111];
			break;
		case MirrorMode::Horizontal:
			data = nameTables[(bool)(addr & 0b1000'0000'0000)][addr & 0b0011'1111'1111];
			break;
		case MirrorMode::OneScreenLo:
			data = nameTables[0][addr & 0b0011'1111'1111];
			break;
		case MirrorMode::OneScreenHi:
			data = nameTables[1][addr & 0b0011'1111'1111];
			break;
		}
	}
	else if (addr >= 0x3F00)
	{
		addr &= 0b0001'1111;
		if ((addr & 0b0011) || addr < 0x10)
			data = palettes[addr >> 2][addr & 0b0011];
		else
			data = palettes[(addr - 0x10) >> 2][0];

		if (mask.greyScale)
			data &= 0x30;
	}
	return data;
}

void Ppu::PpuWrite(uint16_t addr, uint8_t data)
{
	addr &= 0b0011'1111'1111'1111;
	if (cart->PpuWrite(addr, data))
	{
	}
	else if (addr >= 0x2000 && addr < 0x3F00)
	{
		switch (cart->GetMirrorMode())
		{
		case MirrorMode::Vertical:
			nameTables[(bool)(addr & 0b0100'0000'0000)][addr & 0b0011'1111'1111] = data;
			break;
		case MirrorMode::Horizontal:
			nameTables[(bool)(addr & 0b1000'0000'0000)][addr & 0b0011'1111'1111] = data;
			break;
		case MirrorMode::OneScreenLo:
			nameTables[0][addr & 0b0011'1111'1111] = data;
			break;
		case MirrorMode::OneScreenHi:
			nameTables[1][addr & 0b0011'1111'1111] = data;
			break;
		}
	}
	else if (addr >= 0x3F00)
	{
		addr &= 0b0001'1111;
		if ((addr & 0b0011) || addr < 0x10)
			palettes[addr >> 2][addr & 0b0011] = data;
		else
			palettes[(addr - 0x10) >> 2][0] = data;
	}
}

uint8_t Ppu::ReadFromCpu(uint16_t addr, bool readonly)
{
	addr &= 7;
	uint8_t data = busData;
	if (readonly)
	{
		switch (addr)
		{
		case 0: // Control (W)
		case 1: // Mask (W)
			break;
		case 2: // Status (R)
			data &= 0b0001'1111;
			data |= status.reg & 0b1110'0000;
			break;
		case 3: // OAM Address (W)
			break;
		case 4: // OAM Data (RW)
			data = nes.ReadOAM();
			break;
		case 5: // Scroll (W)
		case 6: // PPU Address (W)
			break;
		case 7: // PPU Data (RW)
			data = PpuRead(vramAddr.reg, true);
			break;
		}
	}
	else
	{
		switch (addr)
		{
		case 0: // Control (W)
		case 1: // Mask (W)
			break;
		case 2: // Status (R)
			data &= 0b0001'1111;
			data |= status.reg & 0b1110'0000;
			status.vblank = false;
			latch = false;
			break;
		case 3: // OAM Address (W)
			break;
		case 4: // OAM Data (RW)
			data = nes.ReadOAM();
			break;
		case 5: // Scroll (W)
		case 6: // PPU Address (W)
			break;
		case 7: // PPU Data (RW)
			if (vramAddr.reg >= 0x3F00)
			{
				delayedDataBuffer = PpuRead(0x2000 + (vramAddr.reg & 0x7FF));
				data = PpuRead(vramAddr.reg);
			}
			else
			{
				data = delayedDataBuffer;
				delayedDataBuffer = PpuRead(vramAddr.reg);
			}
			vramAddr.reg += (1 << (ctrl.incrMode * 5));
			break;
		}
	}
	return data;
}

void Ppu::WriteFromCpu(uint16_t addr, uint8_t data)
{
	addr &= 7;
	busData = data;
	switch (addr)
	{
	case 0: // Control (W)
		ctrl.reg = data;
		tramAddr.nametableX = data & 0b0001;
		tramAddr.nametableY = (data & 0b0010) >> 1;
		break;
	case 1: // Mask (W)
		mask.reg = data;
		break;
	case 2: // Status (R)
		break;
	case 3: // OAM Address (W)
		nes.SetOAMAddr(data);
		break;
	case 4: // OAM Data (RW)
		nes.WriteOAM(data);
		break;
	case 5: // Scroll (W)
		if (!latch)
		{
			tramAddr.coarseX = data >> 3;
			scrollFineX = data & 0b0111;
		}
		else
		{
			tramAddr.coarseY = data >> 3;
			tramAddr.fineY = data & 0b0111;
		}
		latch = !latch;
		break;
	case 6: // PPU Address (W)
		if (!latch)
		{
			tramAddr.hi = data & 0b0011'1111;
			tramAddr.unusedAddr0 = 0;
		}
		else
		{
			tramAddr.lo = data;
			vramAddr.reg = tramAddr.reg;
		}
		latch = !latch;
		break;
	case 7: // PPU Data (RW)
		PpuWrite(vramAddr.reg, data);
		vramAddr.reg += (1 << (ctrl.incrMode * 5));
		break;
	}
}

bool Ppu::CheckNmi()
{
	bool state = nmi;
	nmi = false;
	return state;
}

void Ppu::LoadBackgroundShifters()
{
	bgAttributeShifterLo = bgNextAttributeShifterLo | (bgAttributeShifterLo & 0xFF00);
	bgAttributeShifterHi = bgNextAttributeShifterHi | (bgAttributeShifterHi & 0xFF00);
	bgPatternShifterLo = bgNextPatternShifterLo | (bgPatternShifterLo & 0xFF00);
	bgPatternShifterHi = bgNextPatternShifterHi | (bgPatternShifterHi & 0xFF00);
}

void Ppu::UpdateShifters()
{
	// Background
	if (mask.backgroundEnabled)
	{
		bgPatternShifterLo <<= 1;
		bgPatternShifterHi <<= 1;
		bgAttributeShifterLo <<= 1;
		bgAttributeShifterHi <<= 1;
	}

	// Foreground
	if (dot >= 1 && dot < 258 && mask.spriteEnabled)
	{
		for (int i = 0; i < spriteCount; i++)
		{
			if (currentSprites[i].x > 0)
			{
				currentSprites[i].x--;
			}
			else
			{
				spritePatternShifterLo[i] <<= 1;
				spritePatternShifterHi[i] <<= 1;
			}
		}
	}
}

void Ppu::OutputColour()
{
	// Background
	if (mask.backgroundEnabled)
	{
		int shift = 15 - scrollFineX;
		uint16_t bitmask = 1 << shift;
		bgPaletteNumber = (bgAttributeShifterLo & bitmask) >> shift;
		bgPaletteNumber |= ((bgAttributeShifterHi & bitmask) >> shift) << 1;
		bgPaletteIndex = (bgPatternShifterLo & bitmask) >> shift;
		bgPaletteIndex |= ((bgPatternShifterHi & bitmask) >> shift) << 1;
	}
	else
	{
		bgPaletteNumber = bgPaletteIndex = 0;
	}

	// Foreground
	uint8_t fgPaletteNumber = 0;
	uint8_t fgPaletteIndex = 0;
	bool fgPriority;
	bool isSprite0 = false;
	for (int i = 0; i < spriteCount; i++)
	{
		const auto& sprite = currentSprites[i];
		if (sprite.x == 0)
		{
			uint8_t loBit = spritePatternShifterLo[i] >> 7;
			uint8_t hiBit = spritePatternShifterHi[i] >> 7;
			fgPaletteIndex = (hiBit << 1) | loBit;
			fgPaletteNumber = sprite.palette + 4;
			if (fgPaletteIndex)
			{
				if (i == 0)
					isSprite0 = true;
				fgPriority = sprite.priority;
				break;
			}
		}
	}

	// Sprite 0 hit
	if (bgPaletteIndex && fgPaletteIndex && sprite0Loaded && isSprite0 && mask.backgroundEnabled && mask.spriteEnabled)
		status.sprite0Hit = true;

	// Final output
	uint8_t paletteNumber;
	uint8_t paletteIndex;
	if ((dot < 8 + 1 && !mask.drawLeftBackground) || !nes.masterBg)
		bgPaletteIndex = 0;
	if ((dot < 8 + 1 && !mask.drawLeftSprite) || !nes.masterFg)
		fgPaletteIndex = 0;

	if (bgPaletteIndex == 0)
	{
		if (fgPaletteIndex == 0)
		{
			paletteNumber = paletteIndex = 0;
		}
		else
		{
			paletteNumber = fgPaletteNumber;
			paletteIndex = fgPaletteIndex;
		}
	}
	else
	{
		if (fgPaletteIndex == 0)
		{
			paletteNumber = bgPaletteNumber;
			paletteIndex = bgPaletteIndex;
		}
		else
		{
			if (fgPriority)
			{
				paletteNumber = bgPaletteNumber;
				paletteIndex = bgPaletteIndex;
			}
			else
			{
				paletteNumber = fgPaletteNumber;
				paletteIndex = fgPaletteIndex;
			}
		}
	}

	if (isDrawing)
	{
		colourOutput = PpuRead(0x3F00 + paletteNumber * 4 + paletteIndex);
		DrawPixel(
			drawXOffset + dot - 1,
			drawYOffset + scanline,
			colourOutput
		);
	}
}

void Ppu::Reposition(int x, int y)
{
	drawXOffset = x;
	drawYOffset = y;
}
