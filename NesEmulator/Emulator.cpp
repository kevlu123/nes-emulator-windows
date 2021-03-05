#include <algorithm>
#include <fstream>
#include "Emulator.h"
#include "Timer.h"
#include "resource.h"
#include <stack>
#include <filesystem>
#include "EmuFileException.h"
#include "DebugLogger.h"

#pragma warning(suppress : 26451)

void Emulator::AudioRequest(float* out)
{
	for (int i = 0; i < Audio::SAMPLES_PER_BLOCK; i++)
		out[i] = 0.0f;

	if (em == nullptr)
		return;

	int count = (int)em->neses.size();
	for (auto& nes : em->neses)
	{
		auto apu = nes->apu;
		if (apu)
			apu->GetSamples(out);
	}

	if (count)
	{
		float m = 1.0f / (float)count;
		for (int i = 0; i < Audio::SAMPLES_PER_BLOCK; i++)
			out[i] *= m;
	}
}

/* 130 - 148 Controller config
 * 149       Null
 * 150 - 199 Misc
 * 200 - 209 Recent roms
 * 210 - 219 Save slots
 * 220 - 229 Load slots
 * 230 - 249 Controllers
 * 300 - xxx Neses
*/

constexpr int IDM_OFFSET_RECENTROMS = 200;
constexpr int IDM_OFFSET_SAVE = 210;
constexpr int IDM_OFFSET_LOAD = 220;
constexpr int IDM_OFFSET_CTRL = 230;
constexpr int IDM_OFFSET_NES = 300;
constexpr int IDM_SIZE_NES = 100;

constexpr int IDM_NESOFFSET_OPENROM = 0;
constexpr int IDM_NESOFFSET_OPENRECENT = 1;
constexpr int IDM_NESOFFSET_RELOAD = 6 + 5;
constexpr int IDM_NESOFFSET_SAVE = 7 + 5;
constexpr int IDM_NESOFFSET_LOAD = 17 + 5;
constexpr int IDM_NESOFFSET_CLOSE = 27 + 5;
constexpr int IDM_NESOFFSET_CTRL = 28 + 5;
constexpr int IDM_NESOFFSET_SPEED = 36 + 5;
constexpr int IDM_NESOFFSET_MUTE = 44 + 5;
constexpr int IDM_NESOFFSET_REMOVE = 99;

constexpr int CCF_OFFSET = 130;

size_t Emulator::MAX_EMULATORS = 1;

static constexpr size_t GetNesMenuID(size_t nesNumber, size_t id)
{
	return nesNumber * IDM_SIZE_NES + IDM_OFFSET_NES + id;
}

enum
{
	CCF_RIGHT = 130,
	CCF_LEFT,
	CCF_DOWN,
	CCF_UP,
	CCF_START,
	CCF_SELECT,
	CCF_B,
	CCF_A,
	CCF_CANCEL,
	CCF_OK,
	CCF_APPLY,
	CCF_DELETE,
	CCF_DEFAULT,
	CCF_RESET,
	CCF_ENABLE,

	IDM_NULL = 149,
	IDM_FILE_OPENROM = 150,
	IDM_FILE_RELOAD,
	IDM_FILE_CLOSEROM,
	IDM_FILE_SAVESRAMNOW,
	IDM_FILE_ABOUT,
	IDM_FILE_EXIT,

	IDM_CTRL_ADDCTRL,

	IDM_EMU_PAUSE,
	IDM_EMU_ENABLEAUDIO,
	IDM_EMU_PAUSEONFOCUSLOST,
	IDM_EMU_EXTERNALINPUT,
	IDM_EMU_ADDNES,

	IDM_DBG_MEMDUMP,
	//IDM_DBG_DUMPALL,
	IDM_DBG_BG,
	IDM_DBG_FG,
	IDM_DBG_DISPNONE,
	IDM_DBG_DISPRAM,
	IDM_DBG_DISPCPU,
	IDM_DBG_DISPPPU,
	//IDM_DBG_DISPAPU,
	IDM_DBG_STEPFRAME,
	IDM_DBG_STEPSCANLINE,
	IDM_DBG_STEPCPU,
	IDM_DBG_STEPCYCLE,

	IDM_WND_FIT,
	IDM_WND_FULLSCREEN,
};

Emulator* Emulator::em;
const std::wstring Emulator::INI_FILENAME = L"ini";
const std::wstring Emulator::SAVE_DIR = L"save_states";
const std::wstring Emulator::SAVE_FILENAME = Emulator::SAVE_DIR + L"/sav";

Emulator::Emulator() :
	title(L"Nes Emulator - "),
	horizontalPixelCount(Ppu::DRAWABLE_WIDTH),
	verticalPixelCount(Ppu::DRAWABLE_HEIGHT),
	vsync(false),
	createNewGfx(false)
{
	static int dummyInit = []
	{
		MAX_EMULATORS = std::thread::hardware_concurrency();
		if (MAX_EMULATORS == 0)
			MAX_EMULATORS = 1;

		WNDCLASSEXW wc{};
		wc.cbSize = sizeof(WNDCLASSEXW);
		wc.lpfnWndProc = Emulator::WndProc;
		wc.lpszClassName = L"Window";
		wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
		wc.hIcon = wc.hIconSm = (HICON)LoadImage(
			GetModuleHandle(NULL),
			MAKEINTRESOURCEW(IDI_ICON1),
			IMAGE_ICON,
			32, 32,
			NULL
		);
		RegisterClassExW(&wc);
		return 0;
	}();

	recentRoms.resize(MAX_RECENTROMS);
	ctrllrs.timer.Reset(false);
	LoadExternalData();
	RepositionNeses();
	FitWindow();

	// Listen to shortcut keys
	Key keys[]
	{
		Key::Ctrl, Key::Alt, Key::Shift, Key::Enter, Key::Escape, Key::LeftMouseButton,
		Key::Num0, Key::Num1, Key::Num2, Key::Num3, Key::Num4,
		Key::Num5, Key::Num6, Key::Num7, Key::Num8, Key::Num9,
		Key::F1, Key::F2, Key::F3, Key::F4, Key::F5, Key::F6,
		Key::F7, Key::F8, Key::F9, Key::F10, Key::F11,
		Key::W, Key::O, Key::P, Key::M, Key::N, Key::R,
		Key::A, Key::B, Key::C, Key::D, Key::E, Key::F,
		Key::LeftArrow, Key::RightArrow, Key::DownArrow, Key::UpArrow,
	};
	for (Key key : keys)
		input.ListenToKey(key);
}

void Emulator::FitWindow()
{
	int desktopWidth = GetSystemMetrics(SM_CXFULLSCREEN);
	int desktopHeight = GetSystemMetrics(SM_CYFULLSCREEN);
	int rows, cols;
	if (neses.size() == 2 || debug != DebugState::None)
	{
		rows = 1;
		cols = 2;
	}
	else
	{
		rows = (neses.size() > 1) + 1;
		cols = ((int)neses.size() + 1) / 2;
	}
	int pxWidth = desktopWidth / (cols * Ppu::DRAWABLE_WIDTH);
	int pxHeight = desktopHeight / (rows * Ppu::DRAWABLE_HEIGHT);
	int pxSize = pxWidth < pxHeight / Graphics::PIXEL_ASPECT ? pxWidth : pxHeight;
	restoredWidth = pxSize * cols * Ppu::DRAWABLE_WIDTH - 2;
	restoredHeight = (int)(pxSize * rows * Ppu::DRAWABLE_HEIGHT / Graphics::PIXEL_ASPECT);

	MakeWindow(WndState::Restored, hWnd, restoredWidth, restoredHeight);
}

void Emulator::MakeWindow(WndState wndState, HWND oldHwnd, int clientWidth, int clientHeight)
{
	if (oldHwnd)
	{
		fullscreenTransitioning = true;
		DestroyWindow(oldHwnd);
	}

	if (wndState == WndState::Fullscreen)
	{
		RECT scrRect;
		GetClientRect(GetDesktopWindow(), &scrRect);
		clientWidth = scrRect.right;
		clientHeight = scrRect.bottom;
	}

	RECT rc{};
	rc.right = clientWidth;
	rc.bottom = clientHeight;
	const auto STYLE = wndState == WndState::Fullscreen ? (WS_POPUP) : (WS_OVERLAPPEDWINDOW);
	AdjustWindowRectEx(
		&rc,
		STYLE,
		wndState != WndState::Fullscreen,
		NULL
	);
	hWnd = CreateWindowExW(
		NULL,
		L"Window",
		(title + L"60 fps").c_str(),
		STYLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		NULL,
		NULL,
		NULL,
		nullptr
	);
	input.SetHwnd(hWnd);
	ShowWindow(hWnd, wndState == WndState::Maximized ? SW_MAXIMIZE : SW_RESTORE);
	SetFocus(hWnd);
	focus = true;
	this->wndState = wndState;
	UpdateMenu();
	createNewGfx = true;
}

void Emulator::CreateControllerConfigWindow()
{
	EnableWindow(hWnd, false);

	constexpr auto STYLE = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	constexpr int WIDTH = 600;
	static constexpr int HEIGHT = 300;
	constexpr int NAME_LABEL_WIDTH = 95;
	constexpr int CHECKBOX_WIDTH = 70;
	static constexpr int BORDER = 12;
	static constexpr int BUTTON_WIDTH = 75;

	RECT rc{};
	rc.right = WIDTH;
	rc.bottom = HEIGHT;
	AdjustWindowRectEx(
		&rc,
		STYLE,
		false,
		NULL
	);
	ctrllrs.hWnd = CreateWindowExW(
		NULL,
		L"Window",
		L"Configure Controllers",
		STYLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top,
		hWnd,
		NULL,
		NULL,
		nullptr
	);
	ShowWindow(ctrllrs.hWnd, SW_RESTORE);

	HDC hdc = GetDC(ctrllrs.hWnd);
	LOGFONTW lf{};
	lf.lfWeight = FW_THIN;
	lf.lfHeight = 16;
	const wchar_t fontName[] = L"Segoe UI";
	std::memcpy(lf.lfFaceName, fontName, sizeof(fontName));
	ctrllrs.hFont.reset(new HFONT(CreateFontIndirectW(&lf)), [](HFONT* hf) { DeleteObject(*hf); });

	auto SetFont = [&](HWND hWnd)
	{
		SendMessageW(hWnd, WM_SETFONT, (WPARAM)*ctrllrs.hFont, (LPARAM)true);
	};
	auto KeyButton = [&](int x, int y, int w, size_t id, std::wstring text, std::wstring lbl, HWND* outHwnd)
	{
		constexpr int anchX = 20;
		constexpr int anchY = (HEIGHT - BORDER) / 2;
		SetFont(*outHwnd = CreateWindowExW(
			NULL,
			L"Button",
			text.c_str(),
			WS_CHILD | WS_VISIBLE | BS_FLAT | BS_TEXT | BS_CENTER | BS_VCENTER,
			anchX + x, anchY + y,
			w, 25,
			ctrllrs.hWnd,
			(HMENU)id,
			NULL,
			nullptr
		));
		SetFont(CreateWindowExW(
			NULL,
			L"Static",
			lbl.c_str(),
			WS_CHILD | WS_VISIBLE | SS_CENTER,
			anchX + x, anchY + y - 20,
			w, 20,
			ctrllrs.hWnd,
			(HMENU)IDM_NULL,
			NULL,
			nullptr
		));
	};
	auto UtilButton = [&](int x, int w, size_t id, std::wstring text, HWND* out = nullptr)
	{
		HWND wnd;
		SetFont((out ? *out : wnd) = CreateWindowExW(
			NULL,
			L"Button",
			text.c_str(),
			WS_CHILD | WS_VISIBLE | BS_FLAT | BS_TEXT | BS_CENTER | BS_VCENTER,
			x, HEIGHT - BORDER - 20,
			w, 20,
			ctrllrs.hWnd,
			(HMENU)id,
			NULL,
			nullptr
		));
	};

	KeyButton(  0,  0, 75, CCF_LEFT,   Input::KeyToString(ctrllrs.keys.KeyLeft()),   L"Left",   &ctrllrs.keysHwnd[1]);
	KeyButton(150,  0, 75, CCF_RIGHT,  Input::KeyToString(ctrllrs.keys.KeyRight()),  L"Right",  &ctrllrs.keysHwnd[0]);
	KeyButton( 75,-50, 75, CCF_UP,     Input::KeyToString(ctrllrs.keys.KeyUp()),     L"Up",     &ctrllrs.keysHwnd[3]);
	KeyButton( 75, 50, 75, CCF_DOWN,   Input::KeyToString(ctrllrs.keys.KeyDown()),   L"Down",   &ctrllrs.keysHwnd[2]);
	KeyButton(231, 50, 75, CCF_SELECT, Input::KeyToString(ctrllrs.keys.KeySelect()), L"Select", &ctrllrs.keysHwnd[5]);
	KeyButton(318, 50, 75, CCF_START,  Input::KeyToString(ctrllrs.keys.KeyStart()),  L"Start",  &ctrllrs.keysHwnd[4]);
	KeyButton(400,  0, 75, CCF_B,      Input::KeyToString(ctrllrs.keys.KeyB()),      L"B",      &ctrllrs.keysHwnd[6]);
	KeyButton(485,  0, 75, CCF_A,      Input::KeyToString(ctrllrs.keys.KeyA()),      L"A",      &ctrllrs.keysHwnd[7]);

	HWND del;
	UtilButton(WIDTH - 1 * (BORDER + BUTTON_WIDTH), BUTTON_WIDTH, CCF_APPLY,  L"Apply",  &ctrllrs.applyWnd);
	UtilButton(WIDTH - 2 * (BORDER + BUTTON_WIDTH), BUTTON_WIDTH, CCF_CANCEL, L"Cancel");
	UtilButton(WIDTH - 3 * (BORDER + BUTTON_WIDTH), BUTTON_WIDTH, CCF_OK,     L"Ok",     &ctrllrs.okWnd);
	UtilButton(WIDTH - 4 * (BORDER + BUTTON_WIDTH), BUTTON_WIDTH, CCF_DELETE, L"Delete", &del);

	UtilButton(BORDER		   , 110, CCF_DEFAULT, L"Reset to Default");
	UtilButton(2 * BORDER + 110,  75, CCF_RESET,   L"Clear");

	SetFont(CreateWindowExW(
		NULL,
		L"Static",
		L"Controller Name",
		WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
		BORDER, BORDER,
		NAME_LABEL_WIDTH, 20,
		ctrllrs.hWnd,
		(HMENU)IDM_NULL,
		NULL,
		nullptr
	));
	SetFont(ctrllrs.renameWnd = CreateWindowExW(
		NULL,
		L"Edit",
		ctrllrs.keys.name.c_str(),
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		NAME_LABEL_WIDTH + BORDER, BORDER,
		WIDTH - 3 * BORDER - NAME_LABEL_WIDTH - CHECKBOX_WIDTH, 20,
		ctrllrs.hWnd,
		(HMENU)IDM_NULL,
		NULL,
		nullptr
	));
	SetFont(ctrllrs.waitingHwnd = CreateWindowExW(
		NULL,
		L"Static",
		L"",
		WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_CENTER,
		BORDER, HEIGHT - 2 * (BORDER + 20),
		125, 20,
		ctrllrs.hWnd,
		(HMENU)IDM_NULL,
		NULL,
		nullptr
	));
	SetFont(ctrllrs.enabledHwnd = CreateWindowExW(
		NULL,
		L"Button",
		L"Enabled",
		WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
		WIDTH - BORDER - CHECKBOX_WIDTH, BORDER,
		CHECKBOX_WIDTH, 20,
		ctrllrs.hWnd,
		(HMENU)CCF_ENABLE,
		NULL,
		nullptr
	));
	CheckDlgButton(ctrllrs.hWnd, CCF_ENABLE, ctrllrs.keys.enabled ? BST_CHECKED : BST_UNCHECKED);

	EnableWindow(ctrllrs.applyWnd, false);
	EnableWindow(ctrllrs.okWnd, true);
	EnableWindow(del, controllers.size() > 1);
	ctrllrs.validName = true;

	ReleaseDC(ctrllrs.hWnd, hdc);
}

void Emulator::DestroyControllerConfigWindow()
{
	ctrllrs.creating = false;
	EnableWindow(hWnd, true);
	SetFocus(hWnd);
	DestroyWindow(ctrllrs.hWnd);
	ctrllrs.hWnd = NULL;
}

LRESULT CALLBACK Emulator::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (!em)
		return DefWindowProcW(hWnd, msg, wParam, lParam);

	switch (msg)
	{
	case WM_DESTROY:
		if (hWnd == em->hWnd)
			PostQuitMessage(0);
		break;
	case WM_CLOSE:
		if (hWnd == em->ctrllrs.hWnd)
		{
			SendMessageW(hWnd, WM_COMMAND, CCF_CANCEL, NULL);
		}
		break;
	case WM_SETFOCUS:
		if (!em->fullscreenTransitioning && hWnd == em->hWnd && em->gfx)
		{
			em->gfx->Shade(true);
		}
		break;
	case WM_KILLFOCUS:
		if (!em->fullscreenTransitioning && hWnd == em->hWnd && em->gfx && em->pauseOnFocusLost)
		{
			em->gfx->Shade(false);
			em->gfx->Present();
		}
		break;
	case WM_LBUTTONDOWN:
		em->menuTransitioning = true;
		if (!em->showMenu && em->wndState == WndState::Fullscreen && HIWORD(lParam) == 0)
		{
			em->showMenu = true;
			SetMenu(em->hWnd, *em->hMenu);
		}
		else if (em->showMenu && em->wndState == WndState::Fullscreen && HIWORD(lParam) > 15)
		{
			em->showMenu = false;
			SetMenu(em->hWnd, NULL);
		}
		break;
	case WM_SIZE:
		if (!em->fullscreenTransitioning && em->ctrllrs.hWnd != hWnd && !em->menuTransitioning)
		{
			if (em->wndState != WndState::Fullscreen)
			{
				if (wParam == SIZE_RESTORED  && em->wndState == WndState::Restored)
				{
					// Resize
					em->restoredWidth = LOWORD(lParam);
					em->restoredHeight = HIWORD(lParam);
					em->resized = true;
				}
				if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED && em->wndState != WndState::Restored)
				{
					// Maximize toggle
					em->wndState = wParam == SIZE_RESTORED ? WndState::Restored : WndState::Maximized;
					em->createNewGfx = true;
				}
			}
		}
		if (em->hWnd == hWnd)
		{
			em->menuTransitioning = false;
		}
		break;
	case WM_EXITSIZEMOVE:
		if (em->resized)
		{
			em->resized = false;
			em->createNewGfx = true;
		}
		break;
	case WM_SYSKEYDOWN:
		if ((Key)wParam == Key::F4 && (lParam & (1 << 29)))
			DestroyWindow(em->hWnd);
		if ((Key)wParam == Key::F10)
			return 0;
		break;
	case WM_COMMAND:
		int id = LOWORD(wParam);
		bool handled = true;

		switch (HIWORD(wParam))
		{
		case EN_CHANGE:
		{
			bool valid = true;
			wchar_t buf[128];
			GetWindowTextW(em->ctrllrs.renameWnd, buf, (int)std::size(buf));
			std::wstring trimmed = buf;
			while (trimmed.size() && (trimmed.front() == L' ' || trimmed.front() == L'\t'))
				trimmed = trimmed.substr(1);
			while (trimmed.size() && (trimmed.back() == L' ' || trimmed.back() == L'\t'))
				trimmed = trimmed.substr(0, trimmed.size() - 1);

			if (trimmed.size())
			{
				for (const auto& controller : em->controllers)
					if (&em->controllers[em->ctrllrs.curCtrllr] != &controller && controller->name == trimmed)
					{
						valid = false;
						break;
					}
			}
			else
			{
				valid = false;
			}
			
			if (valid)
				em->ctrllrs.keys.name = trimmed;
			EnableWindow(em->ctrllrs.okWnd, valid);
			EnableWindow(em->ctrllrs.applyWnd, valid);
			em->ctrllrs.validName = valid;
			break;
		}
		default:
			handled = false;
			break;
		}

		if (!handled)
		{
			handled = true;
			switch (id)
			{
			case IDM_FILE_OPENROM:
			{
				std::wstring filename;
				if (OpenROMDialog(filename))
				{
					em->OpenAllROM(filename);
					em->SaveIni();
				}
				break;
			}
			case IDM_FILE_ABOUT:
				em->menuTransitioning = true;
				MessageBoxW(
					em->hWnd,
					L"All credits to Kev!",
					L"About",
					MB_OK | MB_ICONINFORMATION
				);
				break;
			case IDM_FILE_EXIT:
				DestroyWindow(hWnd);
				break;
			case IDM_FILE_CLOSEROM:
				for (auto& nes : em->neses)
					nes->RemoveCartridge();
				em->UpdateMenu();
				break;
			case IDM_FILE_RELOAD:
				for (auto& nes : em->neses)
					if (nes->cart)
						nes->Reset();
				break;
			case IDM_FILE_SAVESRAMNOW:
				em->sramSavePending = true;
				break;
			case IDM_EMU_ADDNES:
			{
				bool createNes = true;
				if (em->neses.size() == MAX_EMULATORS)
				{
					createNes = false;
					if (MessageBoxW(
						em->hWnd,
						L"You have reached the maximum recommended Neses for your system.\n"
						L"Adding another Nes may cause significant lag.\n"
						L"Are you sure you want to continue?",
						L"Nes Limited Reached",
						MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2
					) == IDYES)
					{
						createNes = true;
					}
				}
				if (createNes)
				{
					em->createNewGfx = true;
					em->ExitDebug();
					em->neses.push_back(std::make_unique<Nes>(em->sramDir));
					em->neses.back()->running = em->MainNes()->running.load();
					em->neses.back()->StartAsync();
					em->RepositionNeses();
					em->UpdateMenu();
					em->SaveIni();
				}
				break;
			}
			case IDM_EMU_ENABLEAUDIO:
				if (em->audio)
					em->audio.reset();
				else
					em->audio = std::make_unique<Audio>(Emulator::AudioRequest);
				em->SaveIni();
				break;
			case IDM_EMU_PAUSE:
				if (em->MainNes()->running)
					for (auto& nes : em->neses)
						nes->running = false;
				else
					for (auto& nes : em->neses)
						nes->running = true;
				em->SaveIni();
				break;
			case IDM_EMU_PAUSEONFOCUSLOST:
				em->pauseOnFocusLost = !em->pauseOnFocusLost;
				em->SaveIni();
				break;
			case IDM_EMU_EXTERNALINPUT:
				em->captureExternalInput = !em->captureExternalInput;
				em->SaveIni();
				break;
			case IDM_WND_FIT:
				if (em->wndState != WndState::Fullscreen)
					em->FitWindow();
				break;
			case IDM_WND_FULLSCREEN:
				em->SetFullscreenState(em->wndState != WndState::Fullscreen);
				break;
			case IDM_CTRL_ADDCTRL:
				if (em->controllers.size() < MAX_CONTROLLERS)
				{
					em->menuTransitioning = true;
					for (size_t i = 0; i < MAX_CONTROLLERS; i++)
					{
						bool contains = false;
						for (size_t j = 0; j < em->controllers.size(); j++)
							if (em->controllers[j]->name == L"Controller " + std::to_wstring(i + 1))
							{
								contains = true;
								break;
							}
						if (!contains)
						{
							em->ctrllrs.keys.name = L"Controller " + std::to_wstring(i + 1);
							break;
						}
					}
					for (auto& key : em->ctrllrs.keys.keyMappings)
						key = Key::None;
					em->ctrllrs.curCtrllr = (int)em->controllers.size();
					em->controllers.push_back(std::make_unique<ControllerTemplate>(em->ctrllrs.keys));
					em->ctrllrs.creating = true;
					em->ctrllrs.keys.enabled = true;
					em->ctrllrs.validName = true;
					em->CreateControllerConfigWindow();
				}
				break;
			case CCF_CANCEL:
				if (em->ctrllrs.creating)
					em->controllers.erase(em->controllers.end() - 1);
				em->DestroyControllerConfigWindow();
				break;
			case CCF_APPLY:
				if (em->ctrllrs.validName)
				{
					*em->controllers[em->ctrllrs.curCtrllr] = em->ctrllrs.keys;
					for (size_t i = 0; i < em->neses.size(); i++)
						for (size_t n = 0; n < MAX_CONTROLLER_PORTS; n++)
							if (em->neses[i]->controllers[n]->tmplate == em->controllers[em->ctrllrs.curCtrllr].get())
								em->neses[i]->controllers[n]->UpdateKeyMappings();
					em->ctrllrs.creating = false;
					EnableWindow(em->ctrllrs.applyWnd, false);
					em->SaveIni();
				}
				break;
			case CCF_OK:
				if (em->ctrllrs.validName)
				{
					SendMessageW(hWnd, WM_COMMAND, CCF_APPLY, NULL);
					em->DestroyControllerConfigWindow();
				}
				break;
			case CCF_DEFAULT:
			{
				auto SetKey = [](Key& key, Key k, HWND hWnd)
				{
					key = k;
					SetWindowTextW(hWnd, Input::KeyToString(k).c_str());
				};
				SetKey(em->ctrllrs.keys.KeyRight(), Key::D, em->ctrllrs.keysHwnd[0]);
				SetKey(em->ctrllrs.keys.KeyLeft(), Key::A, em->ctrllrs.keysHwnd[1]);
				SetKey(em->ctrllrs.keys.KeyDown(), Key::S, em->ctrllrs.keysHwnd[2]);
				SetKey(em->ctrllrs.keys.KeyUp(), Key::W, em->ctrllrs.keysHwnd[3]);
				SetKey(em->ctrllrs.keys.KeyStart(), Key::Enter, em->ctrllrs.keysHwnd[4]);
				SetKey(em->ctrllrs.keys.KeySelect(), Key::RightShift, em->ctrllrs.keysHwnd[5]);
				SetKey(em->ctrllrs.keys.KeyB(), Key::SemiColonOEM, em->ctrllrs.keysHwnd[6]);
				SetKey(em->ctrllrs.keys.KeyA(), Key::QuoteOEM, em->ctrllrs.keysHwnd[7]);
				SetFocus(em->ctrllrs.hWnd);
				if (em->ctrllrs.validName)
					EnableWindow(em->ctrllrs.applyWnd, true);
				break;
			}
			case CCF_RESET:
			{
				for (int i = 0; i < 8; i++)
				{
					em->ctrllrs.keys.keyMappings[i] = Key::None;
					SetWindowTextW(em->ctrllrs.keysHwnd[i], Input::KeyToString(Key::None).c_str());
				}
				SetFocus(em->ctrllrs.hWnd);
				if (em->ctrllrs.validName)
					EnableWindow(em->ctrllrs.applyWnd, true);
				break;
			}
			case CCF_DELETE:
				if (em->controllers.size() > 1)
				{
					const auto tmplate = em->controllers[em->ctrllrs.curCtrllr].get();
					for (size_t i = 0; i < em->neses.size(); i++)
						for (size_t n = 0; n < MAX_CONTROLLER_PORTS; n++)
							if (em->neses[i]->controllers[n]->tmplate == tmplate)
								em->neses[i]->UnplugController((int)n);
					em->controllers.erase(em->controllers.begin() + em->ctrllrs.curCtrllr);
					em->DestroyControllerConfigWindow();
					em->SaveIni();
				}
				break;
			case CCF_ENABLE:
			{
				bool checked = IsDlgButtonChecked(hWnd, CCF_ENABLE) == BST_CHECKED;
				SetFocus(em->ctrllrs.hWnd);
				CheckDlgButton(hWnd, CCF_ENABLE, checked ? BST_UNCHECKED : BST_CHECKED);
				em->ctrllrs.keys.enabled = !checked;
				if (em->ctrllrs.validName)
					EnableWindow(em->ctrllrs.applyWnd, true);
				break;
			}
			case IDM_DBG_BG:
				if (em->Debuggable())
				{
					em->MainNes()->masterBg = !em->MainNes()->masterBg;
					em->SaveIni();
				}
				break;
			case IDM_DBG_FG:
				if (em->Debuggable())
				{
					em->MainNes()->masterFg = !em->MainNes()->masterFg;
					em->SaveIni();
				}
				break;
			case IDM_DBG_DISPNONE:
				if (em->Debuggable())
					em->ChangeDebugState(DebugState::None);
				break;
			case IDM_DBG_DISPRAM:
				if (em->Debuggable())
					em->ChangeDebugState(DebugState::Ram);
				break;
			case IDM_DBG_DISPCPU:
				if (em->Debuggable())
					em->ChangeDebugState(DebugState::Cpu);
				break;
			case IDM_DBG_DISPPPU:
				if (em->Debuggable())
					em->ChangeDebugState(DebugState::Ppu);
				break;
			//case IDM_DBG_DISPAPU:
			//	if (em->Debuggable())
			//		em->ChangeDebugState(DebugState::Apu);
			//	break;
			case IDM_DBG_MEMDUMP:
				if (em->Debuggable() && em->MainNes()->cart)
				{
					std::vector<uint8_t> vec(em->MainNes()->ram, em->MainNes()->ram + std::size(em->MainNes()->ram));
					em->SaveFile(vec);
				}
				break;
			//case IDM_DBG_DUMPALL:
			//	break;
			case IDM_DBG_STEPCYCLE:
				if (em->Debuggable() && em->MainNes()->cart && em->MainNes()->NotRunning())
				{
					em->MainNes()->Clock();
					em->MainNes()->frameComplete = true;
				}
				break;
			case IDM_DBG_STEPCPU:
				if (em->Debuggable() && em->MainNes()->cart && em->MainNes()->NotRunning())
				{
					em->MainNes()->ClockCpuInstruction();
					em->MainNes()->frameComplete = true;
				}
				break;
			case IDM_DBG_STEPSCANLINE:
				if (em->Debuggable() && em->MainNes()->cart && em->MainNes()->NotRunning())
				{
					for (int i = 0; i < Ppu::DOT_COUNT; i++)
						em->MainNes()->Clock();
					em->MainNes()->frameComplete = true;
				}
				break;
			case IDM_DBG_STEPFRAME:
				if (em->Debuggable() && em->MainNes()->cart && em->MainNes()->NotRunning())
				{
					em->MainNes()->ClockFrame();
					em->MainNes()->frameComplete = true;
				}
				break;
			default:
				int nesNum = (id - IDM_OFFSET_NES) / IDM_SIZE_NES;
				int subId = (id - IDM_OFFSET_NES) % IDM_SIZE_NES;
				auto InRange = [](unsigned int val, unsigned int offset, unsigned int range = 1)
				{
					if (val < IDM_OFFSET_NES)
						return false;
					val = (val - IDM_OFFSET_NES) % IDM_SIZE_NES;
					return val >= offset && val < offset + range;
				};
				auto IsControllerButton = [](unsigned int val)
				{
					if (val < IDM_OFFSET_NES)
						return false;
					val = (val - IDM_OFFSET_NES) % IDM_SIZE_NES;
					return val >= IDM_NESOFFSET_CTRL && val < IDM_NESOFFSET_CTRL + em->controllers.size() * MAX_CONTROLLER_PORTS;
				};
				if (id >= IDM_OFFSET_RECENTROMS && id < IDM_OFFSET_RECENTROMS + MAX_RECENTROMS)
				{
					// Recent ROMS (all)
					auto& filename = em->recentRoms[id - IDM_OFFSET_RECENTROMS];
					em->OpenAllROM(filename);
					em->SaveIni();
				}
				else if (InRange(id, IDM_NESOFFSET_OPENRECENT, MAX_RECENTROMS))
				{
					// Recent ROMS
					auto& filename = em->recentRoms[subId - IDM_NESOFFSET_OPENRECENT];
					em->OpenROM(nesNum, filename);
					em->SaveIni();
				}
				else if (InRange(id, IDM_NESOFFSET_CLOSE))
				{
					// Close
					em->neses[nesNum]->RemoveCartridge();
				}
				else if (InRange(id, IDM_NESOFFSET_RELOAD))
				{
					// Reload
					if (em->neses[nesNum]->cart)
						em->neses[nesNum]->Reset();
				}
				else if (InRange(id, IDM_NESOFFSET_MUTE))
				{
					// Mute
					em->neses[nesNum]->mute = !em->neses[nesNum]->mute;
				}
				else if (InRange(id, IDM_NESOFFSET_OPENROM))
				{
					// Open ROM
					std::wstring filename;
					if (OpenROMDialog(filename))
					{
						em->OpenROM(nesNum, filename);
						em->SaveIni();
					}
				}
				else if (id >= IDM_OFFSET_LOAD && id < IDM_OFFSET_LOAD + MAX_SAVES)
				{
					// Load all
					if (!em->saveStates[id - IDM_OFFSET_LOAD].empty())
						for (auto& nes : em->neses)
							nes->saveLoadState = -(id - IDM_OFFSET_LOAD + 1);
				}
				else if (InRange(id, IDM_NESOFFSET_SAVE, MAX_SAVES))
				{
					// Save
					if (em->neses[nesNum]->cart)
						em->neses[nesNum]->saveLoadState = subId - IDM_NESOFFSET_SAVE + 1;
				}
				else if (InRange(id, IDM_NESOFFSET_LOAD, MAX_SAVES))
				{
					// Load
					if (!em->saveStates[subId - IDM_NESOFFSET_LOAD].empty())
						em->neses[nesNum]->saveLoadState = -(subId - IDM_NESOFFSET_LOAD + 1);
				}
				else if (InRange(id, IDM_NESOFFSET_SPEED, 8))
				{
					// Speed
					int speed = 0;
					switch (subId - IDM_NESOFFSET_SPEED)
					{
					case 0: speed = 0; break;
					case 1: speed = -8; break;
					case 2: speed = -4; break;
					case 3: speed = -2; break;
					case 4: speed = 1; break;
					case 5: speed = 2; break;
					case 6: speed = 4; break;
					case 7: speed = 8; break;
					}
					em->neses[nesNum]->SetEmulationSpeed(speed);
					em->SaveIni();
				}
				else if (InRange(id, IDM_NESOFFSET_REMOVE))
				{
					// Delete nes
					em->createNewGfx = true;
					em->neses.erase(em->neses.begin() + nesNum);
					if (em->neses.size() == 1)
						em->MainNes()->StopAsync();
					em->RepositionNeses();
					em->SaveIni();
				}
				else if (id >= IDM_OFFSET_CTRL && id < IDM_OFFSET_CTRL + MAX_CONTROLLERS)
				{
					int num = id - IDM_OFFSET_CTRL;
					em->ctrllrs.curCtrllr = num;
					em->ctrllrs.keys = *em->controllers[num];
					em->CreateControllerConfigWindow();
				}
				else if (IsControllerButton(id))
				{
					int menuNum = id;
					id = (id - IDM_OFFSET_NES) % IDM_SIZE_NES - IDM_NESOFFSET_CTRL;
					int port = static_cast<int>(id / em->controllers.size());
					int ctrllrTemplate = id % em->controllers.size();
					MENUITEMINFOW menuInfo;
					menuInfo.cbSize = sizeof(MENUITEMINFOW);
					menuInfo.fMask = MIIM_STATE;
					GetMenuItemInfoW(GetMenu(em->hWnd), menuNum, false, &menuInfo);
					if (menuInfo.fState & MFS_CHECKED)
					{
						// Remove controller
						em->neses[nesNum]->UnplugController(port);
					}
					else
					{
						// Add controller
						em->neses[nesNum]->SetController(port, std::make_unique<Controller>(
							em->controllers[ctrllrTemplate],
							em->input
							));
					}
				}
				else if (id >= CCF_OFFSET && id < CCF_OFFSET + 8 && em->ctrllrs.timePoint == 0)
				{
					// CCF keys
					em->ctrllrs.curKey = id - CCF_OFFSET;
					EnableWindow(em->ctrllrs.keysHwnd[em->ctrllrs.curKey], false);
					em->ctrllrs.timePoint = 3;
					SetWindowTextW(em->ctrllrs.waitingHwnd, L"Waiting for key... 3");
					em->ctrllrs.timer.Reset(true);
					em->input.ListenAll(true);
				}
				else
				{
					handled = false;
				}
			}
		}
		if (handled)
			em->UpdateMenu();
		break;
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void Emulator::ExitDebug()
{
	debug = DebugState::None;
	MainNes()->masterBg = true;
	MainNes()->masterFg = true;
}

bool Emulator::Debuggable() const
{
	return neses.size() == 1;
}

void Emulator::ChangeDebugState(DebugState ds)
{
	if (debug != DebugState::None)
		for (int y = 0; y < Ppu::DRAWABLE_HEIGHT; y++)
			for (int x = 0; x < DEBUG_WIDTH; x++)
				gfx->Draw(Ppu::DRAWABLE_WIDTH + x, y, 0x3F);

	if ((debug == DebugState::None) != (ds == DebugState::None))
		createNewGfx = true;

	if (debug == DebugState::Ram && ds == DebugState::Ram)
		debugPage = (debugPage + 1) % 8;
	if (debug == DebugState::Cpu && ds == DebugState::Cpu)
		debugPage = (debugPage + 1) % 2;
	if (debug == DebugState::Ppu && ds == DebugState::Ppu)
		debugPage = (debugPage + 1) % 3;

	if (debug != ds)
		debugPage = 0;

	debug = ds;
	ramAddr = ppuPaletteSelect = -1;
	SaveIni();
}

void Emulator::UpdateMenu()
{
	std::stack<HMENU> menus;
	hMenu.reset(new HMENU(CreateMenu()), [](HMENU* hm) { DestroyMenu(*hm); });
	menus.push(*hMenu);

	auto SubMenu = [&](const wchar_t* text, ULONG flags = NULL)
	{
		HMENU hm = CreateMenu();
		AppendMenuW(menus.top(), MF_POPUP | flags, (UINT_PTR)hm, text);
		menus.push(hm);
	};
	auto EndSubMenu = [&]()
	{
		menus.pop();
	};
	auto NewMenu = [&](const wchar_t* text, size_t id, bool enabled = true, ULONG flags = NULL)
	{
		AppendMenuW(menus.top(), MF_STRING | flags | (enabled ? MF_ENABLED : MF_DISABLED), (UINT_PTR)id, text);
	};
	auto NewSeparator = [&]()
	{
		AppendMenuW(menus.top(), MF_SEPARATOR, IDM_NULL, NULL);
	};

	bool romOpen = false;
	for (const auto& nes : neses)
		if (nes->cart)
		{
			romOpen = true;
			break;
		}

	// Create menus
	SubMenu(L"File");
	{
		NewMenu(L"Open ROM...\tCtrl+O", IDM_FILE_OPENROM);
		SubMenu(L"Open Recent ROM");
		{
			for (size_t i = 0; i < MAX_RECENTROMS; i++)
			{
				std::wstring text = (i >= 10 ? L"" : L" ") + std::to_wstring(i + 1) + L". ";
				if (!recentRoms[i].empty())
					text += recentRoms[i].c_str();
				NewMenu(text.c_str(), IDM_OFFSET_RECENTROMS + i, !recentRoms[i].empty());
			}
			EndSubMenu();
		}
		NewMenu(L"Close ROM\tCtrl+W", IDM_FILE_CLOSEROM, romOpen);
		NewSeparator();
		NewMenu(L"Save SRAM Now\tF5", IDM_FILE_SAVESRAMNOW, romOpen);
		NewSeparator();
		NewMenu(L"Reset\tCtrl+R", IDM_FILE_RELOAD, romOpen);
		NewSeparator();
		SubMenu(L"Save State", neses.size() > 1 ? MF_DISABLED : MF_ENABLED);
		{
			for (size_t i = 0; i < MAX_SAVES; i++)
			{
				std::wstring text = (i >= 10 ? L"" : L" ") + std::to_wstring(i + 1) + L". ";
				if (!saveStates[i].empty())
					text += saveStates[i].c_str();
				text += L"\tAlt+" + std::to_wstring((i + 1) % 10);
				NewMenu(text.c_str(), GetNesMenuID(0, IDM_NESOFFSET_SAVE) + i, romOpen);
			}
			EndSubMenu();
		}
		SubMenu(L"Load State");
		{
			for (size_t i = 0; i < MAX_SAVES; i++)
			{
				std::wstring text = (i >= 10 ? L"" : L" ") + std::to_wstring(i + 1) + L". ";
				if (!saveStates[i].empty())
					text += saveStates[i].c_str();
				text += L"\tCtrl+" + std::to_wstring((i + 1) % 10);
				NewMenu(text.c_str(), IDM_OFFSET_LOAD + i, !saveStates[i].empty());
			}
			EndSubMenu();
		}
		NewSeparator();
		NewMenu(L"About...", IDM_FILE_ABOUT);
		NewSeparator();
		NewMenu(L"Exit\tAlt+F4", IDM_FILE_EXIT);
		EndSubMenu();
	}
	SubMenu(L"Controllers");
	{
		for (size_t i = 0; i < controllers.size(); i++)
			NewMenu((controllers[i]->name + L"...").c_str(), IDM_OFFSET_CTRL + i);
		NewSeparator();
		NewMenu(L"Add Controller...", IDM_CTRL_ADDCTRL, controllers.size() < MAX_CONTROLLERS);
		EndSubMenu();
	}
	SubMenu(L"Emulation");
	{
		NewMenu(L"Enable Audio\tCtrl+M", IDM_EMU_ENABLEAUDIO, true, audio ? MF_CHECKED : MF_UNCHECKED);
		NewMenu(L"Pause\tCtrl+P", IDM_EMU_PAUSE, true, !MainNes()->running ? MF_CHECKED : MF_UNCHECKED);
		NewMenu(L"Pause On Focus Loss", IDM_EMU_PAUSEONFOCUSLOST, true, pauseOnFocusLost ? MF_CHECKED : MF_UNCHECKED);
		NewMenu(L"Capture External Input", IDM_EMU_EXTERNALINPUT, true, captureExternalInput ? MF_CHECKED : MF_UNCHECKED);
		NewSeparator();
		NewMenu(L"Add Emulator\tCtrl+N", IDM_EMU_ADDNES, neses.size() < MAX_EMULATORS);
		EndSubMenu();
	}
	SubMenu(L"Debug", Debuggable() ? MF_ENABLED : MF_DISABLED);
	{
		NewMenu(L"Dump RAM...\tF7", IDM_DBG_MEMDUMP);
		//NewMenu(L"Dump State...\tF7", IDM_DBG_DUMPALL);
		NewSeparator();
		SubMenu(L"Display");
		{
			NewMenu(L"None\tF1", IDM_DBG_DISPNONE, true, debug == DebugState::None ? MF_CHECKED : MF_UNCHECKED);
			NewMenu(L"RAM\tF2", IDM_DBG_DISPRAM, true, debug == DebugState::Ram ? MF_CHECKED : MF_UNCHECKED);
			NewMenu(L"Cpu\tF3", IDM_DBG_DISPCPU, true, debug == DebugState::Cpu ? MF_CHECKED : MF_UNCHECKED);
			NewMenu(L"Ppu\tF4", IDM_DBG_DISPPPU, true, debug == DebugState::Ppu ? MF_CHECKED : MF_UNCHECKED);
			//NewMenu(L"Apu\tF5", IDM_DBG_DISPAPU, true, debug == DebugState::Apu ? MF_CHECKED : MF_UNCHECKED);
			EndSubMenu();
		}
		NewSeparator();
		NewMenu(L"Enable Background\tF8", IDM_DBG_BG, true, MainNes()->masterBg ? MF_CHECKED : MF_UNCHECKED);
		NewMenu(L"Enable Foreground\tF9", IDM_DBG_FG, true, MainNes()->masterFg ? MF_CHECKED : MF_UNCHECKED);
		NewSeparator();
		NewMenu(L"Step Frame\tF10", IDM_DBG_STEPFRAME, MainNes()->NotRunning());
		NewMenu(L"Step Scanline\tShift+F10", IDM_DBG_STEPSCANLINE, MainNes()->NotRunning());
		NewMenu(L"Step Cpu\tF11", IDM_DBG_STEPCPU, MainNes()->NotRunning());
		NewMenu(L"Step Cycle\tShift+F11", IDM_DBG_STEPCYCLE, MainNes()->NotRunning());
		EndSubMenu();
	}
	SubMenu(L"Window");
	{
		NewMenu(L"Auto Resize\tF6", IDM_WND_FIT, wndState != WndState::Fullscreen);
		NewMenu(L"Fullscreen\tAlt+Enter", IDM_WND_FULLSCREEN, true, wndState == WndState::Fullscreen ? MF_CHECKED : MF_UNCHECKED);
		EndSubMenu();
	}
	for (size_t nes = 0; nes < neses.size(); nes++)
	{
		bool specificRomOpen = static_cast<bool>(neses[nes]->cart);

		if (neses.size() == 1)
			SubMenu(L"Nes");
		else
			SubMenu((L"Nes " + std::to_wstring(nes + 1)).c_str());

		{
			NewMenu(L"Open ROM...", GetNesMenuID(nes, IDM_NESOFFSET_OPENROM));
			SubMenu(L"Open Recent ROM");
			{
				for (size_t i = 0; i < MAX_RECENTROMS; i++)
				{
					std::wstring text = (i >= 10 ? L"" : L" ") + std::to_wstring(i + 1) + L". ";
					if (!recentRoms[i].empty())
						text += recentRoms[i].c_str();
					NewMenu(text.c_str(), GetNesMenuID(nes, IDM_NESOFFSET_OPENRECENT) + i, !recentRoms[i].empty());
				}
				EndSubMenu();
			}
			NewMenu(L"Close ROM", GetNesMenuID(nes, IDM_NESOFFSET_CLOSE), specificRomOpen);
			NewSeparator();
			SubMenu(L"Save State");
			{
				for (size_t i = 0; i < MAX_SAVES; i++)
				{
					std::wstring text = (i >= 10 ? L"" : L" ") + std::to_wstring(i + 1) + L". ";
					if (!saveStates[i].empty())
						text += saveStates[i].c_str();
					NewMenu(text.c_str(), GetNesMenuID(nes, IDM_NESOFFSET_SAVE) + i, specificRomOpen);
				}
				EndSubMenu();
			}
			SubMenu(L"Load State");
			{
				for (size_t i = 0; i < MAX_SAVES; i++)
				{
					std::wstring text = (i >= 10 ? L"" : L" ") + std::to_wstring(i + 1) + L". ";
					if (!saveStates[i].empty())
						text += saveStates[i].c_str();
					NewMenu(text.c_str(), GetNesMenuID(nes, IDM_NESOFFSET_LOAD) + i, !saveStates[i].empty());
				}
				EndSubMenu();
			}
			NewSeparator();
			NewMenu(L"Reset", GetNesMenuID(nes, IDM_NESOFFSET_RELOAD), specificRomOpen);
			NewSeparator();
			for (size_t k = 0; k < 2; k++)
			{
				SubMenu((L"Controller " + std::to_wstring(k + 1)).c_str());
				{
					for (size_t i = 0; i < controllers.size(); i++)
					{
						NewMenu(
							controllers[i]->name.c_str(),
							GetNesMenuID(nes, IDM_NESOFFSET_CTRL + controllers.size() * k + i),
							true,
							neses[nes]->controllers[k]->tmplate == controllers[i].get() ? MF_CHECKED : MF_UNCHECKED
						);
					}
					EndSubMenu();
				}
			}
			NewSeparator();
			SubMenu(L"Speed");
			{
				for (size_t k = 0; k < 8; k++)
				{
					int speed = 0;
					std::wstring name;
					switch (k)
					{
					case 0: speed = 0;  name = L"Freeze"; break;
					case 1: speed = -8; name = L"x1/8"; break;
					case 2: speed = -4; name = L"x1/4"; break;
					case 3: speed = -2; name = L"x1/2"; break;
					case 4: speed = 1;  name = L"x1"; break;
					case 5: speed = 2;  name = L"x2"; break;
					case 6: speed = 4;  name = L"x4"; break;
					case 7: speed = 8;  name = L"x8"; break;
					}

					NewMenu(
						name.c_str(),
						GetNesMenuID(nes, IDM_NESOFFSET_SPEED + k),
						true,
						neses[nes]->GetEmulationSpeed() == speed ? MF_CHECKED : MF_UNCHECKED
					);
				}
				EndSubMenu();
			}
			NewMenu(L"Mute", GetNesMenuID(nes, IDM_NESOFFSET_MUTE), true, neses[nes]->mute ? MF_CHECKED : MF_UNCHECKED);
			NewSeparator();
			NewMenu(L"Delete Nes", GetNesMenuID(nes, IDM_NESOFFSET_REMOVE), neses.size() > 1);
			EndSubMenu();
		}
	}

	SetMenu(hWnd, wndState == WndState::Fullscreen ? NULL : menus.top());
	showMenu = wndState != WndState::Fullscreen;
}

void Emulator::LoadExternalData()
{
	// Get current directory
	wchar_t dirChars[256];
	CreateAudio();
	if (GetModuleFileNameW(NULL, dirChars, (DWORD)std::size(dirChars)))
	{
		exeDir = dirChars;
		exeDir = std::filesystem::path(exeDir).parent_path().wstring() + L"/";
		//exeDir = L"c:/users/kevlu/desktop/";
		sramDir = exeDir + L"/sram/";

		// Read ini
		std::wifstream ini(exeDir + INI_FILENAME);
		if (ini.is_open())
		{
			std::vector<std::wstring> lines;
			std::wstring line;
			while (std::getline(ini, line))
				lines.push_back(std::move(line));

			// Check if unlimited emulators is allowed
			for (size_t n = 1; n < lines.size(); n++)
				if (lines[n - 1] == L"[no emulator limit]")
				{
					unlimitedEmulators = true;
					break;
				}

			// Recent roms
			for (size_t n = 1; n < lines.size(); n++)
				if (lines[n - 1] == L"[recent roms]")
				{
					for (size_t i = 0; i + n < lines.size() && i < MAX_RECENTROMS; i++)
					{
						if (lines[i + n].size() && lines[i + n].front() == L'[')
							break;
						recentRoms[i] = lines[i + n];
					}
					break;
				}

			// Controllers
			for (size_t n = 1; n + 9 < lines.size() && controllers.size() < MAX_CONTROLLERS; n++)
				if (lines[n - 1] == L"[controller]")
				{
					std::wstring name = lines[n];
					Key keys[8]{};
					for (size_t i = 0; i < 8; i++)
						keys[i] = Input::StringToKey(lines[n + i + 2]);
					controllers.push_back(std::make_unique<ControllerTemplate>(
						name,
						keys[7], keys[6], keys[5], keys[4],
						keys[3], keys[2], keys[1], keys[0]
					));
					controllers.back()->enabled = lines[n + 1] == L"enabled";
				}

			// Neses
			for (size_t n = 1; n + 3 < lines.size(); n++)
				if (lines[n - 1] == L"[nes]")
				{
					auto nes = std::make_unique<Nes>(sramDir);

					// Controllers
					unsigned int ctrl0;
					StringToUint(lines[n], ctrl0);
					if (ctrl0 - 1 >= controllers.size())
						ctrl0 = 0;
					unsigned int ctrl1;
					StringToUint(lines[n + 1], ctrl1);
					if (ctrl1 - 1 >= controllers.size())
						ctrl1 = 0;
					if (ctrl0)
						nes->SetController(0, std::make_unique<Controller>(controllers[ctrl0 - 1], input));
					if (ctrl1)
						nes->SetController(1, std::make_unique<Controller>(controllers[ctrl1 - 1], input));

					// Speed
					unsigned int speed = 0;
					if (StringToUint(lines[n + 2], speed))
						nes->SetEmulationSpeed((int)speed - 8);

					// Mute
					nes->mute = lines[n + 3] == L"mute";

					neses.push_back(std::move(nes));
					if (neses.size() >= MAX_EMULATORS && !unlimitedEmulators)
						break;
				}

			// Global data
			for (size_t n = 1; n + 7 < lines.size(); n++)
				if (lines[n - 1] == L"[global]")
				{
					for (auto& nes : neses)
						nes->running = lines[n] != L"paused";
					if (lines[n + 1] == L"audio_disabled")
						audio.reset();
					pauseOnFocusLost = lines[n + 2] == L"pause_on_focus_lost";
					captureExternalInput = lines[n + 3] == L"external_input";

					// Debug fg bg
					MainNes()->masterBg = lines[n + 4] == L"bg";
					MainNes()->masterFg = lines[n + 5] == L"fg";

					// Debug state
					unsigned int debugState = 0;
					if (neses.size() == 1 && StringToUint(lines[n + 6], debugState) && debugState < 5)
						ChangeDebugState((DebugState)debugState);

					StringToUint(lines[n + 7], debugPage);
				}
		}

		// Get save states
		for (size_t i = 0; i < MAX_SAVES; i++)
		{
			saveStates[i] = L"";
			std::ifstream f(exeDir + SAVE_FILENAME + std::to_wstring(i));

			if (!f.is_open())
				continue;

			size_t strLen = 0;
			f.read((char*)&strLen, sizeof(strLen));
			if (f.eof() || f.fail() || f.bad() || strLen > 255)
				continue;

			std::wstring str;
			str.resize(strLen);
			f.read((char*)str.data(), strLen * sizeof(wchar_t));
			if (f.eof() || f.fail() || f.bad())
				continue;

			saveStates[i] = str;
		}
	}

	// Default controller
	if (controllers.empty())
	{
		controllers.push_back(std::make_unique<ControllerTemplate>(
			L"Default Controller",
			Key::QuoteOEM, Key::SemiColonOEM, Key::RightShift, Key::Enter,
			Key::W, Key::S, Key::A, Key::D
		));

		if (!neses.empty())
			MainNes()->SetController(0, std::make_unique<Controller>(controllers.front(), input));
	}

	// Default nes
	if (neses.empty())
	{
		neses.push_back(std::make_unique<Nes>(sramDir));
		MainNes()->SetController(0, std::make_unique<Controller>(controllers.front(), input));
	}

	if (neses.size() > 1)
		for (size_t i = 1; i < neses.size(); i++)
			neses[i]->StartAsync();
}

bool Emulator::OpenROMDialog(std::wstring& outFile)
{
	OPENFILENAMEW diagDesc{};
	diagDesc.lStructSize = sizeof(diagDesc);
	diagDesc.hwndOwner = em->hWnd;
	diagDesc.lpstrFilter = L"Nes ROMS\0*.nes\0All files\0*.*\0";
	wchar_t filename[256] = { 0 };
	diagDesc.lpstrFile = filename;
	diagDesc.nMaxFile = (DWORD)std::size(filename);
	diagDesc.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
	diagDesc.nFilterIndex = 1;
	bool res = GetOpenFileNameW(&diagDesc);
	outFile = filename;
	return res;
}

void Emulator::CreateAudio()
{
	try
	{
		audio = std::make_unique<Audio>(Emulator::AudioRequest);
	}
	catch (AudioException&)
	{
		MessageBoxW(
			hWnd,
			L"Failed to initialise audio. Emulation will continue "
			L"normally without audio.",
			L"Audio Error",
			MB_OK | MB_ICONINFORMATION
		);
	}
}

void Emulator::SaveFile(const std::vector<uint8_t>& bytes) const
{
	OPENFILENAMEW diagDesc{};
	diagDesc.lStructSize = sizeof(diagDesc);
	diagDesc.hwndOwner = em->hWnd;
	diagDesc.lpstrFilter = L"All files\0*.*\0";
	wchar_t filename[256] = L"ram";
	diagDesc.lpstrFile = filename;
	diagDesc.nMaxFile = (DWORD)std::size(filename);
	diagDesc.Flags = OFN_DONTADDTORECENT | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_OVERWRITEPROMPT;
	diagDesc.nFilterIndex = 1;

	if (GetSaveFileNameW(&diagDesc))
	{
		std::ofstream f(filename, std::ios::binary);
		if (f.is_open())
			f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());

		if (!f.is_open() || f.bad() || f.fail())
		{
			MessageBoxW(
				hWnd,
				(std::wstring(L"Could not write to file ") + filename).c_str(),
				L"File Error",
				MB_ICONERROR | MB_OK
			);
		}
	}
}

void Emulator::OpenAllROM(const std::wstring& filename)
{
	try
	{
		for (size_t i = 0; i < neses.size(); i++)
		{
			neses[i]->InsertCartridge(std::make_shared<Cartridge>(
				sramDir + GetFilenameFromPath(filename) + L".sram",
				filename)
			);
		}
		InsertRecentRom(filename);
	}
	catch (EmuFileException& ex)
	{
		DeleteRecentRom(filename);
		MessageBoxA(hWnd, ex.what(), "File Error", MB_OK | MB_ICONERROR);
	}
}

void Emulator::OpenROM(int nes, const std::wstring& filename)
{
	try
	{
		neses[nes]->InsertCartridge(std::make_shared<Cartridge>(
			sramDir + GetFilenameFromPath(filename) + L".sram",
			filename)
		);
		InsertRecentRom(filename);
	}
	catch (EmuFileException& ex)
	{
		DeleteRecentRom(filename);
		MessageBoxA(hWnd, ex.what(), "File Error", MB_OK | MB_ICONERROR);
	}
}

std::wstring Emulator::GetFilenameFromPath(const std::wstring& path)
{
	return path.substr(path.find_last_of(L"/\\") + 1);
}

bool Emulator::StringToUint(std::wstring s, unsigned int& out)
{
	out = 0;
	if (s.size() > 3)
		return false;
	while (s.size())
	{
		if (s.front() < L'0' || s.front() > L'9')
		{
			out = 0;
			return false;
		}
		out = out * 10 + s.front() - L'0';
		s = s.substr(1);
	}
	return true;
}

void Emulator::SetFullscreenState(bool fullscreen)
{
	if (fullscreen)
	{
		prevWndState = wndState;
		MakeWindow(WndState::Fullscreen, hWnd, 0, 0);
	}
	else
	{
		MakeWindow(prevWndState, hWnd, restoredWidth, restoredHeight);
	}
}

void Emulator::InsertRecentRom(std::wstring rom)
{
	DeleteRecentRom(rom);
	recentRoms.insert(recentRoms.begin(), rom);
	recentRoms.pop_back();
}

void Emulator::DeleteRecentRom(std::wstring rom)
{
	if (rom.empty())
		return;

	for (size_t i = 0; i < MAX_RECENTROMS; i++)
		if (recentRoms[i] == rom)
		{
			recentRoms.erase(recentRoms.begin() + i);
			recentRoms.emplace_back();
			return;
		}
}

void Emulator::CheckSaveStates()
{
	for (auto& nes : neses)
	{
		int state = nes->saveLoadState;
		if (state > 0)
		{
			// Save
			nes->saveLoadState = 0;
			int slot = state - 1;

			std::error_code ec;
			std::filesystem::create_directory(exeDir + SAVE_DIR, ec);

			std::ofstream f(exeDir + SAVE_FILENAME + std::to_wstring(slot), std::ios::binary);
			if (f.is_open())
			{
				auto bytes = nes->SaveState();
				f.write(reinterpret_cast<char*>(bytes.data()), bytes.size());
			}

			if (!f.is_open() || f.fail() || f.bad())
			{
				MessageBoxW(
					hWnd,
					(L"Could not write to file: " + SAVE_FILENAME + std::to_wstring(slot)).c_str(),
					L"File Error",
					MB_OK | MB_ICONERROR
				);
				return;
			}
			saveStates[slot] = nes->cart->filename;
			UpdateMenu();
		}
		else if (state < 0)
		{
			// Load
			nes->saveLoadState = 0;
			int slot = -state - 1;
			std::ifstream f(exeDir + SAVE_FILENAME + std::to_wstring(slot), std::ios::binary | std::ios::ate);

			std::vector<uint8_t> bytes;
			if (f.is_open())
			{
				size_t len = (size_t)f.tellg();
				f.seekg(0, std::ios::beg);
				bytes.resize(len);
				f.read(reinterpret_cast<char*>(bytes.data()), len);
			}

			if (!f.is_open() || f.fail() || f.bad())
			{
				MessageBoxW(
					hWnd,
					(L"Could not read from file: " + SAVE_FILENAME + std::to_wstring(slot)).c_str(),
					L"File Error",
					MB_OK | MB_ICONERROR
				);
				return;
			}

			try
			{
				nes->LoadState(saveStates[slot], bytes);
			}
			catch (EmuFileException&)
			{
				MessageBoxW(
					hWnd,
					(L"Invalid save state file: " + SAVE_FILENAME + std::to_wstring(slot)).c_str(),
					L"File Error",
					MB_OK | MB_ICONERROR
				);
				return;
			}
			UpdateMenu();
		}
	}
}

bool Emulator::Run(std::wstring cmdArgs)
{
	em = this;
	int prevFps = fps;
	MSG msg{};

	if (!cmdArgs.empty())
	{
		if (cmdArgs.size() >= 2 && cmdArgs.front() == '"' && cmdArgs.back() == '"')
			cmdArgs = cmdArgs.substr(1, cmdArgs.size() - 2);
		OpenROM(0, cmdArgs);
	}

	while (true)
	{
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			if (msg.message == WM_QUIT)
			{
				if (fullscreenTransitioning)
					fullscreenTransitioning = false;
				else
					goto end;
			}
		}

		long long expectedFrames = (long long)((double)frameTimer.GetElapsedSeconds() * 60.0);
		if (frameCount < expectedFrames)
		{
			frameCount++;
			if (expectedFrames - frameCount > 5)
				frameCount = expectedFrames - 5;

			focus = GetActiveWindow();
			input.UpdateKeys(focus || captureExternalInput);
			Update();

			if (prevFps != fps)
			{
				SetWindowTextW(hWnd, (title + std::to_wstring(fps) + L" fps").c_str());
				prevFps = fps;
			}
		}
		else
			Sleep(10);
	}
end:
	SaveIni();
	audio.reset();
	neses.clear();
	em = nullptr;
	return true;
}

void Emulator::SaveIni() const
{
	// Save ini
	std::wofstream f(exeDir + INI_FILENAME);

	// Global
	f << L"[global]" << std::endl;
	f << (neses[0]->running ? L"unpaused" : L"paused") << std::endl;
	f << (audio ? L"audio_enabled" : L"audio_disabled") << std::endl;
	f << (pauseOnFocusLost ? L"pause_on_focus_lost" : L"dont_pause_on_focus_lost") << std::endl;
	f << (captureExternalInput ? L"external_input" : L"internal_input") << std::endl;
	f << (MainNes()->masterBg ? L"bg" : L"no_bg") << std::endl;
	f << (MainNes()->masterFg ? L"fg" : L"no_fg") << std::endl;
	f << (int)debug << std::endl;
	f << debugPage << std::endl;

	// Recent ROMs
	f << L"[recent roms]" << std::endl;
	for (const auto& rom : recentRoms)
		f << rom << std::endl;

	// Controllers
	for (const auto& controller : controllers)
	{
		f << L"[controller]" << std::endl;
		f << controller->name << std::endl;
		f << (controller->enabled ? L"enabled" : L"disabled") << std::endl;
		for (size_t i = 0; i < 8; i++)
			f << Input::KeyToString(controller->keyMappings[i]) << std::endl;
	}

	// Neses
	for (const auto& nes : neses)
	{
		f << L"[nes]" << std::endl;
		for (size_t i = 0; i < std::size(nes->controllers); i++)
		{
			if (nes->controllers[i]->tmplate == nullptr)
			{
				f << 0 << std::endl;
			}
			else
			{
				for (size_t n = 0; n < controllers.size(); n++)
					if (controllers[n].get() == nes->controllers[i]->tmplate)
					{
						f << (n + 1) << std::endl;
						break;
					}
			}
		}
		f << (nes->GetEmulationSpeed() + 8) << std::endl;
		f << (nes->mute ? L"mute" : L"unmute") << std::endl;
	}
}

void Emulator::CheckKeyboardShortcuts()
{
	const auto& Notify = [&](int message) { SendMessageW(hWnd, WM_COMMAND, message, NULL); };
	Key key = input.GetKeyPressed();

	bool ctrl = input.GetKey(Key::Ctrl);
	bool alt = input.GetKey(Key::Alt);
	bool shift = input.GetKey(Key::Shift);

	if (ctrl && !alt && !shift) // Ctrl
	{
		// Load state
		if (key >= Key::Num1 && key <= Key::Num9)
			Notify(IDM_OFFSET_LOAD + (int)key - (int)Key::Num1);
		if (key == Key::Num0)
			Notify(IDM_OFFSET_LOAD + 9);

		// Close all
		if (key == Key::W)
			Notify(IDM_FILE_CLOSEROM);

		// Open all
		if (key == Key::O)
			Notify(IDM_FILE_OPENROM);

		// Reload all
		if (key == Key::R)
			Notify(IDM_FILE_RELOAD);

		// Mute
		if (key == Key::M)
			Notify(IDM_EMU_ENABLEAUDIO);

		// Pause
		if (key == Key::P)
			Notify(IDM_EMU_PAUSE);

		// New nes
		if (key == Key::N)
			Notify(IDM_EMU_ADDNES);
	}
	else if (!ctrl && alt && !shift) // Alt
	{
		// Save state
		if (key >= Key::Num1 && key <= Key::Num9)
			Notify(IDM_OFFSET_NES + IDM_NESOFFSET_SAVE + (int)key - (int)Key::Num1);
		if (key == Key::Num0)
			Notify(IDM_OFFSET_NES + IDM_NESOFFSET_SAVE + 9);
	}
	else if (!ctrl && !alt && shift) // Shift
	{
		// Step
		if (key == Key::F10)
			Notify(IDM_DBG_STEPSCANLINE);
		if (key == Key::F11)
			Notify(IDM_DBG_STEPCYCLE);
	}
	else if (!ctrl && !alt && !shift) // None
	{
		// Debug Disp
		if (key == Key::F1)
			Notify(IDM_DBG_DISPNONE);
		if (key == Key::F2)
			Notify(IDM_DBG_DISPRAM);
		if (key == Key::F3)
			Notify(IDM_DBG_DISPCPU);
		if (key == Key::F4)
			Notify(IDM_DBG_DISPPPU);

		if (key == Key::F5)
			Notify(IDM_FILE_SAVESRAMNOW);
		if (key == Key::F6)
			Notify(IDM_WND_FIT);

		// Dump
		if (key == Key::F7)
			Notify(IDM_DBG_MEMDUMP);
		//if (key == Key::F7)
		//	Notify(IDM_DBG_DUMPALL);

		// Bg
		if (key == Key::F8)
			Notify(IDM_DBG_BG);

		// Fg
		if (key == Key::F9)
			Notify(IDM_DBG_FG);

		// Step
		if (key == Key::F10)
			Notify(IDM_DBG_STEPFRAME);
		if (key == Key::F11)
			Notify(IDM_DBG_STEPCPU);
	}
}

void Emulator::RepositionNeses()
{
	int rows, cols;
	if (neses.size() == 2)
	{
		rows = 1;
		cols = 2;
		neses[0]->Reposition(0, 0);
		neses[1]->Reposition(Ppu::DRAWABLE_WIDTH, 0);
	}
	else
	{
		rows = (neses.size() > 1) + 1;
		cols = ((int)neses.size() + 1) / 2;
		for (size_t n = 0; n < neses.size(); n++)
		{
			neses[n]->Reposition(
				static_cast<int>(n % cols * Ppu::DRAWABLE_WIDTH),
				static_cast<int>(n / cols * Ppu::DRAWABLE_HEIGHT)
			);
		}
	}

	horizontalPixelCount = Ppu::DRAWABLE_WIDTH * cols;
	verticalPixelCount = Ppu::DRAWABLE_HEIGHT* rows;
}

void Emulator::Update()
{
	if (focus || !pauseOnFocusLost)
	{
		if (!ctrllrs.hWnd)
		{
			CheckKeyboardShortcuts();

			// Exit fullscreen
			if (wndState == WndState::Fullscreen && (input.GetKeyDown(Key::Enter) && input.GetKey(Key::Alt) || input.GetKeyDown(Key::Escape)))
				SetFullscreenState(false);
			// Enter fullscreen
			else if (wndState != WndState::Fullscreen && input.GetKeyDown(Key::Enter) && input.GetKey(Key::Alt))
				SetFullscreenState(true);

			bool ready = true;
			for (size_t i = 0; i < neses.size(); i++)
				if (!neses[i]->frameComplete)
				{
					ready = false;
					break;
				}

			if (ready)
			{
				fpsTimer.Update(fps);
				if (createNewGfx)
				{
					auto screenBuffer = gfx ? gfx->ReleaseBuffer() : std::vector<uint8_t>();
					em->gfx.reset();

					int h = em->horizontalPixelCount + ((!Debuggable() || debug == DebugState::None) ? 0 : DEBUG_WIDTH);
					int v = em->verticalPixelCount;
					em->gfx = std::make_unique<Graphics>(hWnd, h, v, em->vsync);
					
					if (screenBuffer.size() == h * v)
						for (int y = 0; y < v; y++)
							for (int x = 0; x < h; x++)
								gfx->Draw(x, y, screenBuffer[y * h + x]);
					
					createNewGfx = false;
					for (auto& nes : neses)
						nes->offDisplay = false;
				}
				gfx->Present();

				if (sramSavePending)
				{
					for (auto& nes : em->neses)
					{
						if (!nes->SaveSRam())
						{
							MessageBoxW(
								hWnd,
								(L"Failed to save SRAM for " + GetFilenameFromPath(nes->cart->filename)).c_str(),
								L"File Error",
								MB_OK
							);
						}
					}
					sramSavePending = false;
				}
				CheckSaveStates();

				for (auto& nes : neses)
					nes->frameComplete = false;

				MainNes()->DoIteration();
			}

			if (debug != DebugState::None)
			{
				switch (debug)
				{
				case DebugState::Ram:
					DrawRam();
					break;
				case DebugState::Cpu:
					DrawCpu();
					break;
				case DebugState::Ppu:
					DrawPpu();
					break;
				//case DebugState::Apu:
				//	DrawApu();
				//	break;
				}
			}
		}
		else
		{
			if (ctrllrs.curKey != -1)
			{
				Key key = input.GetKeyPressed();

				if (ctrllrs.timePoint == 3 && ctrllrs.timer.GetElapsedSeconds() >= 1.0f)
				{
					SetWindowTextW(ctrllrs.waitingHwnd, L"Waiting for key... 2");
					ctrllrs.timePoint = 2;
				}
				else if (ctrllrs.timePoint == 2 && ctrllrs.timer.GetElapsedSeconds() >= 2.0f)
				{
					SetWindowTextW(ctrllrs.waitingHwnd, L"Waiting for key... 1");
					ctrllrs.timePoint = 1;
				}
				else if (key != Key::None || ctrllrs.timer.GetElapsedSeconds() >= 3.0f)
				{
					HWND button = ctrllrs.keysHwnd[ctrllrs.curKey];
					if (key != Key::None)
					{
						ctrllrs.keys.keyMappings[ctrllrs.curKey] = key;
						if (em->ctrllrs.validName)
							EnableWindow(ctrllrs.applyWnd, true);
						SetWindowTextW(button, Input::KeyToString(key).c_str());
					}
					ctrllrs.curKey = -1;
					EnableWindow(button, true);
					ctrllrs.timer.Reset(false);
					SetWindowTextW(ctrllrs.waitingHwnd, L"");
					ctrllrs.timePoint = 0;
					input.ListenAll(false);
				}
			}
			else if (input.GetKeyDown(Key::Enter))
			{
				SendMessageW(ctrllrs.hWnd, WM_COMMAND, CCF_OK, NULL);
			}
		}
	}
}

void DrawPixel(int x, int y, uint8_t c)
{
	Emulator::em->gfx->Draw(x, y, c);
}

const std::unique_ptr<Nes>& Emulator::MainNes() const
{
	return neses.front();
}

std::unique_ptr<Nes>& Emulator::MainNes()
{
	return neses.front();
}

std::string Emulator::HexString(uint32_t n, int digits)
{
	std::string s;
	for (int i = 0; i < digits; i++)
	{
		s = "0123456789ABCDEF"[n & 0xF] + s;
		n >>= 4;
	}
	return s;
}

std::string Emulator::IntToString(int n)
{
	if (n == 0)
		return "0";

	std::string s;
	bool neg = n < 0;
	if (neg)
		n = -n;
	while (n)
	{
		s = "0123456789"[n % 10] + s;
		n /= 10;
	}
	if (neg)
		s = '-' + s;
	return s;
}

std::vector<std::string> Emulator::Disassemble(uint32_t addr, int instructionCount)
{
	if (!MainNes()->cart)
		return { "" };

	std::vector<std::string> dasm;
	//addr = MainNes()->cart->mapper->MapCpuRead(addr, true);
	Cpu::Instruction instr{};

	for (int i = 0; i < instructionCount; i++, addr += instr.bytes)
	{
		auto Read = [&](uint16_t addr, int bytes = 1)
		{
			uint16_t val = 0;
			for (int i = 0; i < bytes; i++)
				val |= MainNes()->CpuRead(addr + i, true) << (i * 8);
			return val;
		};
		auto HexStringAddr = [](uint32_t n, int b = 1) { return '(' + HexString(n, 2 * b) + ')'; };
		auto HexString = [](uint32_t n, int b = 1) {return Emulator::HexString(n, b * 2); };

		const auto& cpu = MainNes()->cpu;
		instr = Cpu::instructions[Read(addr)];
		std::string s = HexString(addr, 2) + ' ';
		s += instr.opname + ' ';
		uint16_t immAddr = addr + 1;

		if (instr.addrmode == &Cpu::IMP)
			s += "                IMP";
		else if (instr.addrmode == &Cpu::ACC)
			s += "          " + HexStringAddr(cpu->ra) + "  ACC";
		else if (instr.addrmode == &Cpu::IMM)
			s += "# " + HexString(Read(immAddr)) + "            IMM";
		else if (instr.addrmode == &Cpu::ZRP)
			s += HexString(Read(immAddr)) + "        " + HexStringAddr(Read(Read(immAddr))) + "  ZRP";
		else if (instr.addrmode == &Cpu::ZPX)
			s += HexString(Read(immAddr)) + "+X      " + HexStringAddr(Read((Read(immAddr) + cpu->rx) & 0xFF)) + "  ZPX";
		else if (instr.addrmode == &Cpu::ZPY)
			s += HexString(Read(immAddr)) + "+Y      " + HexStringAddr(Read((Read(immAddr) + cpu->ry) & 0xFF)) + "  ZPY";
		else if (instr.addrmode == &Cpu::ABS)
		{
			if (instr.op == &Cpu::JMP || instr.op == &Cpu::JSR)
				s += HexString(Read(immAddr, 2), 2) + "            ABS";
			else
				s += HexString(Read(immAddr, 2), 2) + "      " + HexStringAddr(Read(Read(immAddr, 2))) + "  ABS";
		}
		else if (instr.addrmode == &Cpu::ABX)
			s += HexString(Read(immAddr, 2), 2) + "+X    " + HexStringAddr(Read(Read(immAddr, 2) + cpu->rx)) + "  ABX";
		else if (instr.addrmode == &Cpu::ABY)
			s += HexString(Read(immAddr, 2), 2) + "+Y    " + HexStringAddr(Read(Read(immAddr, 2) + cpu->ry)) + "  ABY";
		else if (instr.addrmode == &Cpu::IDX)
			s += '[' + HexString(Read(immAddr)) + "+X]    " + HexStringAddr(Read(Read((Read(immAddr) + cpu->rx) & 0xFF, 2))) + "  IDX";
		else if (instr.addrmode == &Cpu::IDY)
			s += '[' + HexString(Read(immAddr)) + "]+Y    " + HexStringAddr(Read(Read(Read(immAddr), 2) + cpu->ry)) + "  IDY";
		else if (instr.addrmode == &Cpu::IND)
			s += '[' + HexString(Read(immAddr, 2), 2) + "]  " + HexStringAddr(Read(Read(immAddr, 2), 2), 2) + "  IND";
		else if (instr.addrmode == &Cpu::REL)
		{
			int8_t offset = (int8_t)Read(immAddr);
			s += offset < 0 ? "#-" : "# ";
			if (offset < 0)
				offset = -offset;
			s += HexString((uint8_t)offset) + "    " + HexStringAddr(immAddr + (int8_t)Read(immAddr) + 1, 2) + "  REL";
		}

		s += ',' + Emulator::HexString(instr.cycles, 1) + "  ";
		if (s.size() != 32)
			throw "";
		dasm.push_back(std::move(s));
	}
	return dasm;
}

void Emulator::DrawRam()
{
	gfx->DrawString(Ppu::DRAWABLE_WIDTH + 10 * 8, 1 * 8, "RAM (page " + HexString(debugPage, 1) + ")", 0x30, 0x3F);
	gfx->DrawString(Ppu::DRAWABLE_WIDTH + 8 * 8, 3 * 8, "[F2 - Next page]", 0x10);
	for (int i = 0; i < 16; i++)
		gfx->DrawChar(Ppu::DRAWABLE_WIDTH + i * 16, Ppu::DRAWABLE_HEIGHT - 21 * 8, "0123456789ABCDEF"[i], 0x30);

	if (!MainNes()->cart)
	{
		for (int i = 0, y = 0; i < 16; i++, y++)
		{
			if (i % 4 == 0)
				y++;
			for (int x = 0; x < 16; x++)
			{
				gfx->DrawString(
					Ppu::DRAWABLE_WIDTH + x * 16,
					Ppu::DRAWABLE_HEIGHT - (20 - y) * 8,
					"--",
					(x % 2) ? 0x30 : 0x00, 0x3F
				);
			}
		}
		return;
	}

	int curX, curY;
	input.GetLocalCursorPos(curX, curY);
	curY -= 10;
	if (input.GetKeyDown(Key::LeftMouseButton))
	{
		if (curX >= 0 && curX < 32 && curY >= 0 && curY < 20 && curY % 5)
		{
			curY -= curY / 5 + 1;
			int newAddr = curY * 32 + curX;
			if (newAddr == ramAddr)
				ramAddr = -1;
			else
				ramAddr = newAddr;
		}
		else
		{
			ramAddr = -1;
		}
	}
	else if (ramAddr != -1)
	{

		if (input.GetKeyDown(Key::LeftArrow))
			ramAddr = (ramAddr - 1) % 0x200;
		if (input.GetKeyDown(Key::RightArrow))
			ramAddr = (ramAddr + 1) % 0x200;
		if (input.GetKeyDown(Key::DownArrow))
			ramAddr = (ramAddr + 32) % 0x200;
		if (input.GetKeyDown(Key::UpArrow))
			ramAddr = (ramAddr - 32) % 0x200;
		if (ramAddr < 0)
			ramAddr += 0x200;

		if (input.GetKeyDown(Key::Enter))
			ramAddr = -1;

		int val = input.GetHexKeyPressed();
		if (val != -1)
		{
			uint8_t& b = MainNes()->ram[debugPage * 256 + ramAddr / 2];
			b &= ramAddr % 2 ? 0xF0 : 0x0F;
			b |= ramAddr % 2 ? val : (val << 4);
			ramAddr = (ramAddr + 1) % 512;
		}
	}

	for (int i = 0, y = 0; i < 16; i++, y++)
	{
		if (i % 4 == 0)
			y++;
		for (int x = 0; x < 16; x++)
		{
			gfx->DrawString(
				Ppu::DRAWABLE_WIDTH + x * 16,
				Ppu::DRAWABLE_HEIGHT - (20 - y) * 8,
				HexString(MainNes()->ram[256 * debugPage + i * 16 + x], 2),
				(x % 2) ? 0x30 : 0x00, 0x3F
			);
		}
	}

	if (ramAddr != -1)
	{
		gfx->DrawChar(
			Ppu::DRAWABLE_WIDTH + ramAddr % 32 * 8,
			(11 + ramAddr / 32 + (ramAddr / 32) / 4) * 8,
			HexString(MainNes()->ram[debugPage * 256 + ramAddr / 2], 2)[ramAddr % 2],
			(ramAddr / 2 % 2) ? 0x30 : 0x00, 0x12
		);
	}
}

void Emulator::DrawCpu()
{
	switch (debugPage)
	{
	case 0:
	{
		struct
		{
			decltype(Cpu::status) status;
			uint32_t pc;
			uint8_t* ram;
			uint8_t ra, rx, ry, sp;
		} cpu{};
		uint8_t buf[0x102]{};
		cpu.ram = buf;
		if (MainNes()->cart)
		{
			cpu.status = MainNes()->cpu->status;
			cpu.pc = MainNes()->cpu->pc;
			cpu.ram = MainNes()->ram;
			cpu.ra = MainNes()->cpu->ra;
			cpu.rx = MainNes()->cpu->rx;
			cpu.ry = MainNes()->cpu->ry;
			cpu.sp = MainNes()->cpu->sp;
		}

		constexpr int asmLines = Ppu::DRAWABLE_HEIGHT / 8 - 6;
		const std::vector<std::string>& dasm = Disassemble(cpu.pc, asmLines);

		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 14 * 8, 8, "CPU");
		gfx->DrawString(Ppu::DRAWABLE_WIDTH, 5 * 8, "Addr Opcode       Data   Mod/Cyc");
		gfx->DrawString(Ppu::DRAWABLE_WIDTH, 6 * 8, dasm[0], 0x10, 0x12);
		for (int i = 1; i < (int)dasm.size(); i++)
			gfx->DrawString(Ppu::DRAWABLE_WIDTH, (i + 6) * 8, dasm[i], 0x10, 0x3F);

		gfx->DrawChar(Ppu::DRAWABLE_WIDTH + 0 * 8, 4 * 8, 'C', cpu.status.c ? 0x1A : 0x16);
		gfx->DrawChar(Ppu::DRAWABLE_WIDTH + 1 * 8, 4 * 8, 'Z', cpu.status.z ? 0x1A : 0x16);
		gfx->DrawChar(Ppu::DRAWABLE_WIDTH + 2 * 8, 4 * 8, 'I', cpu.status.i ? 0x1A : 0x16);
		gfx->DrawChar(Ppu::DRAWABLE_WIDTH + 3 * 8, 4 * 8, 'D', cpu.status.d ? 0x1A : 0x16);
		gfx->DrawChar(Ppu::DRAWABLE_WIDTH + 4 * 8, 4 * 8, 'B', cpu.status.b ? 0x1A : 0x16);
		gfx->DrawChar(Ppu::DRAWABLE_WIDTH + 5 * 8, 4 * 8, 'U', cpu.status.u ? 0x1A : 0x16);
		gfx->DrawChar(Ppu::DRAWABLE_WIDTH + 6 * 8, 4 * 8, 'V', cpu.status.v ? 0x1A : 0x16);
		gfx->DrawChar(Ppu::DRAWABLE_WIDTH + 7 * 8, 4 * 8, 'N', cpu.status.n ? 0x1A : 0x16);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 0 * 8, 3 * 8, "PC:", 0x10);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 4 * 8, 3 * 8, HexString(cpu.pc, 4), 0x30, 0x3F);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 11 * 8, 3 * 8, "SP:", 0x10);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 14 * 8, 3 * 8, HexString(cpu.sp, 2), 0x30, 0x3F);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 16 * 8, 3 * 8, '(' + HexString(cpu.ram[0x100 + cpu.sp + 1], 2) + ')', 0x30, 0x3F);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 12 * 8, 4 * 8, "A:", 0x10);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 14 * 8, 4 * 8, HexString(cpu.ra, 2), 0x30, 0x3F);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 16 * 8, 4 * 8, '(' + HexString(cpu.ram[cpu.ra], 2) + ')', 0x30, 0x3F);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 23 * 8, 3 * 8, "X:", 0x10);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 25 * 8, 3 * 8, HexString(cpu.rx, 2), 0x30, 0x3F);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 27 * 8, 3 * 8, '(' + HexString(cpu.ram[cpu.rx], 2) + ')', 0x30, 0x3F);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 23 * 8, 4 * 8, "Y:", 0x10);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 25 * 8, 4 * 8, HexString(cpu.ry, 2), 0x30, 0x3F);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH + 27 * 8, 4 * 8, '(' + HexString(cpu.ram[cpu.ry], 2) + ')', 0x30, 0x3F);
		break;
	}
	case 1:
	{
		if (!MainNes()->cart)
			return;

		gfx->DrawString(Ppu::DRAWABLE_WIDTH, 0 * 8, "PRG Size (KB): " + IntToString(MainNes()->cart->mapper->prgChunks * 16), 0x30, 0x3F);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH, 1 * 8, "CHR Size (KB): " + IntToString(MainNes()->cart->mapper->chrChunks *  8), 0x30, 0x3F);
		gfx->DrawString(Ppu::DRAWABLE_WIDTH, 2 * 8, "Mapper: " + IntToString(MainNes()->cart->mapper->mapperNumber) + "  ", 0x30, 0x3F);
		break;
	}
	}
}

void Emulator::DrawPpu()
{
	auto GetTile = [&](uint8_t id, int table, uint8_t attr)
	{
		std::vector<uint8_t> pixels;
		for (int fineY = 0; fineY < 8; fineY++)
		{
			uint16_t addr = table * 0x1000;
			addr += id * 16;
			addr += fineY;
			uint8_t lo = MainNes()->ppu->PpuRead(addr, true);
			uint8_t hi = MainNes()->ppu->PpuRead(addr + 8, true);
			for (int bit = 7; bit >= 0; bit--)
			{
				uint8_t index = ((lo >> bit) & 1) | (((hi >> bit) & 1) << 1);
				int palette = index ? attr : 0;
				uint8_t colour = MainNes()->ppu->palettes[palette][index];
				pixels.push_back(colour);
			}
		}
		return pixels;
	};

	auto DrawTile = [&](int tileX, int tileY, uint8_t id, int table, uint8_t attr)
	{
		std::vector<uint8_t> pixels;
		if (!MainNes()->cart)
			for (int i = 0; i < 8 * 8; i++)
				pixels.push_back((tileX + tileY) % 2 ? 0x00 : 0x10);
		else
			pixels = GetTile(id, table, attr);

		for (int dy = 0; dy < 8; dy++)
			for (int dx = 0; dx < 8; dx++)
			{
				gfx->Draw(
					Ppu::DRAWABLE_WIDTH + tileX * 8 + dx,
					tileY * 8 + dy,
					pixels[dy * 8 + dx]
				);
			}
	};

	switch (debugPage)
	{
	case 0:
	{
		// Pattern tables
		for (int table = 0; table < 2; table++)
			for (int dy = 0; dy < 16; dy++)
				for (int dx = 0; dx < 16; dx++)
					DrawTile(dx + table * 16, dy, dy * 16 + dx, table, ppuPatternColour);
		if (ppuPatternSelect != -1)
		{
			gfx->DrawChar(
				Ppu::DRAWABLE_WIDTH + 8 * (ppuPatternSelect % 32),
				8 * (ppuPatternSelect / 32),
				'\x1',
				0
			);
		}

		// Modify pattern table
		int dispTile = 0;
		{
			int x, y;
			input.GetLocalCursorPos(x, y);
			if (x >= 0 && x < 32 && y >= 0 && y < 16)
			{
				int select = y * 32 + x;
				if (input.GetKeyDown(Key::LeftMouseButton))
				{
					if (ppuPatternSelect == select)
						ppuPatternSelect = -1;
					else
						ppuPatternSelect = select;
				}
				dispTile = select;
			}
			else
			{
				if (input.GetKeyDown(Key::LeftMouseButton))
					ppuPatternSelect = -1;
				dispTile = ppuPatternSelect;
			}
		}
		if (!MainNes()->cart)
			dispTile = ppuPatternSelect = -1;

		gfx->DrawString(
			Ppu::DRAWABLE_WIDTH,
			17 * 8,
			"Pattern Tables"
		);
		auto pixels = (dispTile == -1 || !MainNes()->cart) ? std::vector<uint8_t>() : GetTile(
			dispTile / 32 * 16 + dispTile % 16,
			dispTile % 32 >= 16,
			ppuPatternColour
		);
		for (int y = 0; y < 8; y++)
			for (int x = 0; x < 8; x++)
			{
				gfx->DrawChar(
					Ppu::DRAWABLE_WIDTH + x * 8,
					(16 + 2 + y) * 8,
					' ', 0,
					(dispTile == -1 || !MainNes()->cart) ? ((x + y) % 2 ? 0x00 : 0x10) : pixels[y * 8 + x]
				);
			}

		// Palettes
		gfx->DrawString(
			Ppu::DRAWABLE_WIDTH + 24 * 8,
			17 * 8,
			"Palettes"
		);
		for (int i = 0; i < 2; i++)
			for (int y = 0; y < 4; y++)
				for (int x = 0; x < 4; x++)
				{
					char ch = ' ';
					uint8_t colour = 0;

					int px = ppuPaletteSelect % 8;
					int py = ppuPaletteSelect / 8;
					if (ppuPaletteSelect != -1 && px == 4 * i + x && py == y)
						ch = '\x1';
					if (px == 0 && x % 4 == 0)
					{
						ch = '\x1';
						if (i || x || y)
							colour = 0x10;
					}

					uint8_t pal = (x + y) % 2 ? 0x00 : 0x10;
					if (MainNes()->cart)
						pal = MainNes()->ppu->palettes[x ? i * 4 + y : 0][x];
					gfx->DrawChar(
						Ppu::DRAWABLE_WIDTH + (i * 4 + x + 24) * 8,
						(16 + 2 + y) * 8,
						ch,
						colour, pal
					);
				}

		// Modify palette
		if (input.GetKeyDown(Key::LeftMouseButton) && MainNes()->cart)
		{
			int x, y;
			input.GetLocalCursorPos(x, y);
			x -= 24;
			y -= 18;
			if (x >= 0 && x < 8 && y >= 0 && y < 4)
			{
				int select = y * 8 + x;
				if (x % 4 == 0)
					select = 0;

				if (ppuPaletteSelect == select)
					ppuPaletteSelect = -1;
				else
					ppuPaletteSelect = select;

				ppuPatternColour = y + (x >= 4) * 4;
			}
			else if (ppuPaletteSelect != -1 && x >= 0 && x < 8 && y >= 4 && y < 12)
			{
				y -= 4;
				int px = ppuPaletteSelect % 8;
				int py = ppuPaletteSelect / 8;
				MainNes()->ppu->palettes[(px > 4) * 4 + py][px % 4] = y * 16 + x + (y >= 4) * 8;
				ppuPaletteSelect = -1;
			}
			else
			{
				ppuPaletteSelect = -1;
			}
		}
		for (int y = 0; y < 8; y++)
			for (int x = 0; x < 8; x++)
			{
				gfx->DrawChar(
					Ppu::DRAWABLE_WIDTH + 8 * (x + 24),
					(22 + y) * 8,
					' ',
					0, ppuPaletteSelect == -1 ? 0x3F : (y * 16 + x + (y >= 4) * 8)
				);
			}
		break;
	}
	case 1:
	{
		// Nametables
		if (!MainNes()->cart)
		{
			for (int table = 0; table < 4; table++)
				for (int dy = 0; dy < 16; dy++)
					for (int dx = 0; dx < 16; dx++)
						DrawTile(dx + (table % 2) * 16, (table / 2) * 15 + dy, dy * 16 + dx, (table % 2), ppuPatternColour);
			break;
		}

		for (int table = 0; table < 4; table++)
			for (int dy = 0; dy < 30; dy++)
				for (int dx = 0; dx < 32; dx++)
				{
					uint8_t id = MainNes()->ppu->PpuRead(0x2000 + 0x400 * table + dy * 32 + dx, true);
					uint16_t attrAddr = 0x23C0 + (table * 0b0000'0100'0000'0000) + ((dy >> 2) << 3) + (dx >> 2);
					uint8_t quadrant = ((dx & 2) >> 1) | (dy & 2);
					uint8_t paletteNumber = (MainNes()->ppu->PpuRead(attrAddr, true) >> (2 * quadrant)) & 0b0011;
					auto pixels = GetTile(id, MainNes()->ppu->ctrl.backgroundPatternTable, paletteNumber);
					for (int y = 0; y < 4; y++)
						for (int x = 0; x < 4; x++)
						{
							gfx->Draw(
								Ppu::DRAWABLE_WIDTH + (table % 2) * (Ppu::DRAWABLE_WIDTH  / 2) + dx * 4 + x,
													  (table / 2) * (Ppu::DRAWABLE_HEIGHT / 2) + dy * 4 + y,
								pixels[(y * 8 + x) * 2]
							);
						}
				}
		break;
	}
	case 2:
	{
		// Sprites
		for (int n = 0; n < 3; n++)
			gfx->DrawString(Ppu::DRAWABLE_WIDTH + n * 8 * 11 + 1, 0, " N X Y F ", 0x30, 0x3F);

		auto* nums = (bool)MainNes()->cart ? &MainNes()->ppu->currentSpriteNumbers : nullptr;
		for (int i = 0; i < 64; i++)
		{
			const ObjectAttributeMemory& oam = nums ? MainNes()->oam[i] : ObjectAttributeMemory{};
			if (nums)
			{
				DrawTile(
					i % 3 * 11,
					i / 3 + 1,
					oam.tileID,
					MainNes()->ppu->ctrl.spriteSize ? 0 : MainNes()->ppu->ctrl.spritePatternTable,
					oam.palette + 4
				);
			}
			else
			{
				DrawTile(
					i % 3 * 11,
					i / 3 + 1,
					0, 0, 0
				);
			}

			uint8_t bgCol = 0x3F;
			if (nums && (*nums)[i])
				bgCol = 0x12;

			gfx->DrawString(Ppu::DRAWABLE_WIDTH + (i % 3 * 11 + 1) * 8, (i / 3 + 1) * 8, HexString(oam.tileID, 2), 0x30, bgCol);
			gfx->DrawString(Ppu::DRAWABLE_WIDTH + (i % 3 * 11 + 3) * 8, (i / 3 + 1) * 8, HexString(oam.x, 2), 0x00, bgCol);
			gfx->DrawString(Ppu::DRAWABLE_WIDTH + (i % 3 * 11 + 5) * 8, (i / 3 + 1) * 8, HexString(oam.y, 2), 0x30, bgCol);
			gfx->DrawChar(Ppu::DRAWABLE_WIDTH + (i % 3 * 11 + 7) * 8, (i / 3 + 1) * 8, 'H', oam.flipHorizontally ? 0x1A : 0x16, bgCol);
			gfx->DrawChar(Ppu::DRAWABLE_WIDTH + (i % 3 * 11 + 8) * 8, (i / 3 + 1) * 8, 'V', oam.flipVertically ? 0x1A : 0x16, bgCol);
			gfx->DrawChar(Ppu::DRAWABLE_WIDTH + (i % 3 * 11 + 9) * 8, (i / 3 + 1) * 8, 'P', oam.priority ? 0x1A : 0x16, bgCol);
		}
		break;
	}
	}

}
