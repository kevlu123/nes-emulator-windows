#include "Nes.h"
#include <Windows.h>
#include "EmuFileException.h"
#include "Audio.h"

Nes::Nes(const std::wstring& sramPath) :
	controllers{ std::make_unique<Controller>(), std::make_unique<Controller>() },
	ram{},
	oam{},
	sramPath(sramPath)
{
}

void Nes::LoadState(const std::wstring& filename, std::vector<uint8_t>& bytes)
{
	std::vector<uint8_t> backup;
	std::wstring backupFilename;
	bool restore = (bool)cart;
	if (restore)
	{
		backup = SaveState();
		backupFilename = cart->filename;
	}
	try
	{
		size_t strLen;
		LoadBytes(bytes, strLen);
		if (strLen > 255)
			throw EmuFileException("invalid file");

		std::wstring str; // Unused
		str.resize(strLen);
		LoadBytes(bytes, str.data(), strLen);

		auto GetFilenameFromPath = [](const std::wstring& path)
		{
			return path.substr(path.find_last_of(L"/\\") + 1);
		};
		cpu = std::make_unique<Cpu>(*this, bytes);
		cart = std::make_shared<Cartridge>(sramPath + GetFilenameFromPath(filename) + L".sram", filename, bytes);
		ppu = std::make_unique<Ppu>(*this, cart, bytes);
		apu = std::make_shared<Apu>(*this, bytes);
		SetEmulationSpeed(emulationSpeed);
		LoadBytes(bytes, ram, std::size(ram));
		LoadBytes(bytes, oam, std::size(oam));
		LoadBytes(bytes, oamAddr);
		LoadBytes(bytes, dmaAddr);
		LoadBytes(bytes, dmaData);
		LoadBytes(bytes, dmaReady);
		LoadBytes(bytes, dmaMode);
		LoadBytes(bytes, controllerLatch);
		LoadBytes(bytes, clockNumber);
		if (bytes.size() > 0)
			throw EmuFileException("invalid file");
	}
	catch (EmuFileException&)
	{
		if (restore)
			LoadState(backupFilename, backup);
		throw;
	}
}

std::vector<uint8_t> Nes::SaveState() const
{
	if (!cart)
		throw EmuFileException("tried to save without a cartridge loaded");
	std::vector<uint8_t> bytes;
	SaveBytes(bytes, cart->filename.size());
	SaveBytes(bytes, cart->filename.data(), cart->filename.size());

	AppendVector(bytes, cpu->SaveState());
	AppendVector(bytes, cart->SaveState());
	AppendVector(bytes, ppu->SaveState());
	AppendVector(bytes, apu->SaveState());
	SaveBytes(bytes, ram, std::size(ram));
	SaveBytes(bytes, oam, std::size(oam));
	SaveBytes(bytes, oamAddr);
	SaveBytes(bytes, dmaAddr);
	SaveBytes(bytes, dmaData);
	SaveBytes(bytes, dmaReady);
	SaveBytes(bytes, dmaMode);
	SaveBytes(bytes, controllerLatch);
	SaveBytes(bytes, clockNumber);
	return bytes;
}

Nes::~Nes()
{
	if (thrd.joinable())
		StopAsync();
	SaveSRam();
}

void Nes::StartAsync()
{
	quit = false;
	thrd = std::thread(&Nes::RunAsync, this);
}

void Nes::StopAsync()
{
	quit = true;
	if (thrd.joinable())
		thrd.join();
}

void Nes::RunAsync()
{
	frameComplete = true;
	while (true)
	{
		Sleep(5);
		while (!quit && frameComplete) {}
		if (quit)
			return;

		std::unique_lock<std::mutex> lock(stateMtx);
		DoIteration();
	}
}

void Nes::DoIteration()
{
	if (cart)
	{
		if (running)
		{
			if (emulationSpeed >= 0)
			{
				offDisplay = false;
				ppu->ClearCurrentSpriteNumbers();
				for (int i = 0; i < emulationSpeed; i++)
				{
					do
					{
						Clock();
					} while (!ppu->IsBeginningFrame());
				}
			}
			else
			{
				if (emuStep == 0)
				{
					emuStep = -emulationSpeed;
					offDisplay = false;
					ppu->ClearCurrentSpriteNumbers();
					do
					{
						Clock();
					} while (!ppu->IsBeginningFrame());
				}
				emuStep--;
			}
		}
	}
	else if (!offDisplay)
	{
		for (int x = 0; x < Ppu::DRAWABLE_WIDTH; x++)
			for (int y = 0; y < Ppu::DRAWABLE_HEIGHT; y++)
				DrawPixel(x + drawXOffset, y + drawYOffset, Ppu::GetPowerOffScreen()[y * Ppu::DRAWABLE_WIDTH + x]);
		offDisplay = true;
	}
	frameComplete = true;
}

int Nes::GetEmulationSpeed() const
{
	return emulationSpeed;
}

void Nes::SetEmulationSpeed(int speed)
{
	if (speed != -8 && speed != -4
		&& speed != -2 && speed != 0
		&& speed != 1 && speed != 2
		&& speed != 4 && speed != 8)
	{
		emulationSpeed = 1;
	}
	else
	{
		emulationSpeed = speed;
	}

	if (apu)
	{
		if (speed < 0)
			apu->SetEmulationSpeed(1.0f / (float)-speed);
		else
			apu->SetEmulationSpeed((float)speed);
	}
}

bool Nes::NotRunning() const
{
	return !running || emulationSpeed == 0;
}

void Nes::Reset()
{
	std::unique_lock<std::mutex> lock(stateMtx);
	cart->Reset();
	cpu->Reset();
	ppu->Reset();
	apu->Reset();
}

void Nes::Reposition(int x, int y)
{
	drawXOffset = x;
	drawYOffset = y;
	if (ppu)
		ppu->Reposition(x, y);
}

void Nes::RemoveCartridge()
{
	std::unique_lock<std::mutex> lock(stateMtx);
	cpu.reset();
	ppu.reset();
	apu.reset();
	cart.reset();
}

void Nes::InsertCartridge(std::shared_ptr<Cartridge> cart)
{
	std::unique_lock<std::mutex> lock(stateMtx);
	this->cart = cart;

	cpu = std::make_unique<Cpu>(*this);
	ppu = std::make_unique<Ppu>(*this, cart);
	apu = std::make_unique<Apu>(*this);
	SetEmulationSpeed(emulationSpeed);

	ppu->Reposition(drawXOffset, drawYOffset);
	clockNumber = 0;
	std::memset(ram, 0, std::size(ram));

	cart->Reset();
	ppu->Reset();
	apu->Reset();
}

void Nes::SetController(int port, std::unique_ptr<Controller> controller)
{
	std::unique_lock<std::mutex> lock(stateMtx);
	if (controller)
		controllers[port] = std::move(controller);
	else
		controllers[port] = std::make_unique<Controller>();
}

void Nes::UnplugController(int port)
{
	SetController(port, nullptr);
}

void Nes::Clock()
{
	ppu->Clock();
	apu->Clock();
	if (clockNumber == 3 || clockNumber == 6)
	{
		if (clockNumber == 6)
			clockNumber = 0;
		if (dmaMode)
			ClockDMA();
		else
			cpu->Clock();
	}
	clockNumber++;

	if (ppu->CheckNmi())
		cpu->Nmi();

	bool irq = false;
	if (cart->GetMapper().GetIrq())
	{
		cart->GetMapper().ClearIrq();
		irq = true;
	}
	if (apu->GetIrq())
		irq = true;

	if (irq)
		cpu->Irq();

	if (controllerLatch & 1)
		for (size_t i = 0; i < std::size(controllers); i++)
			controllers[i]->SetState();
}

void Nes::ClockCpuInstruction()
{
	do
	{
		Clock();
	} while (!cpu->InstructionComplete());
	do
	{
		Clock();
	} while (cpu->InstructionComplete());
}

void Nes::ClockFrame()
{
	do
	{
		Clock();
	} while (!ppu->IsBeginningFrame());
}

void Nes::CpuWrite(uint16_t addr, uint8_t data)
{
	if (cart->CpuWrite(addr, data))
	{
	}
	else if (addr < 0x2000)
	{
		addr &= 0x7FF;
		ram[addr] = data;
	}
	else if (addr >= 0x2000 && addr < 0x4000)
	{
		ppu->WriteFromCpu(addr, data);
	}
	else if ((addr >= 0x4000 && addr < 0x4014) || addr == 0x4015 || addr == 0x4017)
	{
		apu->WriteFromCpu(addr, data);
	}
	else if (addr == 0x4014)
	{
		dmaAddr = data << 8;
		dmaMode = true;
	}
	else if (addr == 0x4016)
	{
		controllerLatch = data & 1;
	}
}

uint8_t Nes::CpuRead(uint16_t addr, bool readonly)
{
	uint8_t data;
	if (cart->CpuRead(addr, data, readonly))
	{
	}
	else if (addr < 0x2000)
	{
		addr &= 0x7FF;
		data = ram[addr];
	}
	else if (addr >= 0x2000 && addr < 0x4000)
	{
		data = ppu->ReadFromCpu(addr, readonly);
	}
	else if (addr == 0x4015)
	{
		data = apu->ReadFromCpu(addr, readonly);
	}
	else if (addr == 0x4016 || addr == 0x4017)
	{
		data = controllers[addr & 1]->Read() + 0x40;
	}
	return data;
}

void Nes::ClockDMA()
{
	if (!dmaReady)
	{
		if (clockNumber % 2 == 1)
			dmaReady = true;
	}
	else
	{
		if (clockNumber % 2 == 0)
		{
			dmaData = CpuRead(dmaAddr);
		}
		else
		{
			reinterpret_cast<uint8_t*>(oam)[dmaAddr & 0xFF] = dmaData;
			dmaAddr++;
			if ((dmaAddr & 0xFF) == 0)
			{
				dmaMode = false;
				dmaReady = false;
			}
		}
	}
}

bool Nes::GetCurrentSprites(int scanline, uint8_t spriteSize, ObjectAttributeMemory out[8], int& spriteCount, bool& sprite0Loaded, std::array<bool, 64>& currentSpriteNumbers) const
{
	spriteCount = 0;
	sprite0Loaded = false;
	std::memset(out, 0xFF, 32);
	if (scanline >= Ppu::DRAWABLE_HEIGHT)
		return false;

	for (int i = 0; i < 64; i++)
	{
		int difference = scanline - oam[i].y;
		if (difference >= 0 && difference < 8 * (spriteSize + 1))
		{
			if (spriteCount < 8)
			{
				if (i == 0)
					sprite0Loaded = true;
				out[spriteCount] = oam[i];
				spriteCount++;

				currentSpriteNumbers[i] = true;
			}
			else
			{
				return true;
			}
		}
	}
	return false;
}

uint8_t Nes::ReadOAM() const
{
	return reinterpret_cast<const uint8_t*>(oam)[oamAddr];
}

void Nes::SetOAMAddr(uint8_t addr)
{
	oamAddr = addr;
}

void Nes::WriteOAM(uint8_t data)
{
	reinterpret_cast<uint8_t*>(oam)[oamAddr] = data;
	oamAddr++;
}

bool Nes::SaveSRam() const
{
	if (!cart)
		return true;
	return cart->SaveSRam();
}
