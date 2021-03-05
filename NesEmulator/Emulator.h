#pragma once
#include <map>
#include <memory>
#include "Graphics.h"
#include "Nes.h"
#include "Input.h"
#include "Audio.h"

class Emulator
{
	friend void DrawPixel(int x, int y, uint8_t c);
public:
	Emulator();
	Emulator(const Emulator&) = delete;
	Emulator& operator=(const Emulator&) = delete;
	bool Run(std::wstring cmdArgs);
	static constexpr int DEBUG_WIDTH = 256;
private:
	enum class WndState { Restored, Maximized, Fullscreen };
	enum class DebugState { None, Ram, Cpu, Ppu, /*Apu*/ };
	int horizontalPixelCount;
	int verticalPixelCount;
	bool focus = true;
	HWND hWnd = NULL;
	bool vsync;
	const std::wstring title;
	bool fullscreenTransitioning = false;
	bool resized = false;
	WndState wndState = WndState::Restored;
	std::wstring exeDir;
	std::wstring sramDir;
	bool createNewGfx;
	bool unlimitedEmulators = false;
	FpsTimer fpsTimer;
	Timer frameTimer;
	long long frameCount = 0;
	int fps = 60;
	std::shared_ptr<HMENU> hMenu;
	bool showMenu = true;
	bool menuTransitioning = false;
	bool pauseOnFocusLost = true;
	bool captureExternalInput = false;
	bool sramSavePending = false;
	void Update();
	void SaveIni() const;
	void MakeWindow(WndState wndState, HWND oldhWnd, int clientWidth, int clientHeight);
	void FitWindow();
	void LoadExternalData();
	void UpdateMenu();
	void CheckKeyboardShortcuts();
	void CheckSaveStates();
	void RepositionNeses();
	void SetFullscreenState(bool fullscreen);
	static bool OpenROMDialog(std::wstring& outFile);
	void OpenROM(int nes, const std::wstring& filename);
	void OpenAllROM(const std::wstring& filename);
	void SaveFile(const std::vector<uint8_t>& bytes) const;
	void InsertRecentRom(std::wstring rom);
	void DeleteRecentRom(std::wstring rom);
	void ExitDebug();
	bool Debuggable() const;
	void ChangeDebugState(DebugState ds);
	void CreateAudio();
	static bool StringToUint(std::wstring s, unsigned int& out);
	static std::wstring GetFilenameFromPath(const std::wstring& path);
	static void AudioRequest(float* out);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static Emulator* em;
	static const std::wstring INI_FILENAME;
	static const std::wstring SAVE_FILENAME;
	static const std::wstring SAVE_DIR;
	static constexpr size_t MAX_RECENTROMS = 10;
	static constexpr size_t MAX_SAVES = 10;
	static constexpr size_t MAX_CONTROLLERS = 8;
	static constexpr size_t MAX_CONTROLLER_PORTS = 2;
	static size_t MAX_EMULATORS;
	std::vector<std::wstring> recentRoms;
	std::wstring saveStates[MAX_SAVES];
	std::vector<std::unique_ptr<ControllerTemplate>> controllers;

	WndState prevWndState = WndState::Restored;
	int restoredWidth;
	int restoredHeight;

	struct
	{
		HWND hWnd = NULL;
		ControllerTemplate keys;
		int curCtrllr = -1;
		int curKey = -1;
		bool creating = false;
		std::shared_ptr<HFONT> hFont;
		HWND keysHwnd[8];
		HWND renameWnd = NULL;
		HWND applyWnd = NULL;
		HWND waitingHwnd = NULL;
		HWND enabledHwnd = NULL;
		HWND okWnd = NULL;
		Timer timer;
		int timePoint = 0;
		bool validName = true;
	} ctrllrs{};
	void CreateControllerConfigWindow();
	void DestroyControllerConfigWindow();

	// Audio
	std::atomic_bool multithreaded = true;
	std::atomic_bool mute = false;

	// Devices
	static constexpr size_t MAX_NESES = 8;
	std::vector<std::unique_ptr<Nes>> neses;
	std::unique_ptr<Graphics> gfx;
	std::unique_ptr<Audio> audio;
	Input input;
	std::unique_ptr<Nes>& MainNes();
	const std::unique_ptr<Nes>& MainNes() const;

	// Debugging
	DebugState debug = DebugState::None;
	unsigned int debugPage = 0;
	int ramAddr = -1;
	int ppuPaletteSelect = -1;
	int ppuPatternSelect = -1;
	int ppuPatternColour = 0;
	static std::string HexString(uint32_t n, int digits = 4);
	static std::string IntToString(int n);
	std::vector<std::string> Disassemble(uint32_t addr, int instructionCount);
	void DrawRam();
	void DrawCpu();
	void DrawPpu();
	//void DrawApu();
};
