#pragma once
#include <vector>
#include <cstdint>
#include "Timer.h"
#include <Windows.h>
#include <string>

enum class Key : uint8_t;
enum class KeyPhase : uint8_t { NotHeld, Begin, Held };

class Input
{
public:
	static Key StringToKey(const std::wstring& s);
	static const std::wstring& KeyToString(Key k);
	Input();
	void SetHwnd(HWND hWnd);
	void GetLocalCursorPos(int& x, int& y); // Assumes debug window dimensions
	void UpdateKeys(bool focus);
	bool GetKey(Key key) const;
	bool GetKeyDown(Key key) const;
	Key GetKeyPressed() const;
	int GetHexKeyPressed() const;
	void ListenToKey(Key key);
	void StopListeningToKey(Key key);
	void ListenAll(bool listen);
private:
	HWND hWnd = NULL;
	KeyPhase keys[256];
	std::vector<std::pair<Key, int>> listening;
};

enum class Key : uint8_t
{
	None = 0,
	LeftMouseButton = 1,
	RightMouseButton,
	MiddleMouseButton = 4,
	ExtraMouseButtonA,
	ExtraMouseButtonB,
	BackSpace = 8,
	Tab,
	Enter = 0xD,
	Shift = 0x10,
	Ctrl,
	Alt,
	CapsLock = 0x14,
	Escape = 0x1B,
	SpaceBar = 0x20,
	LeftArrow = 0x25,
	UpArrow,
	RightArrow,
	DownArrow,
	Insert = 0x2D,
	Delete,
	Num0 = 0x30,
	Num1,
	Num2,
	Num3,
	Num4,
	Num5,
	Num6,
	Num7,
	Num8,
	Num9,
	A = 0x41,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	I,
	J,
	K,
	L,
	M,
	N,
	O,
	P,
	Q,
	R,
	S,
	T,
	U,
	V,
	W,
	X,
	Y,
	Z,
	LeftWindows,
	RightWindows,
	Numpad0 = 0x60,
	Numpad1,
	Numpad2,
	Numpad3,
	Numpad4,
	Numpad5,
	Numpad6,
	Numpad7,
	Numpad8,
	Numpad9,
	NumpadAsterisk,
	NumpadPlus,
	NumpadMinus = 0x6D,
	NumpadDot,
	NumpadSlash,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12,
	F13,
	F14,
	F15,
	F16,
	F17,
	F18,
	F19,
	F20,
	F21,
	F22,
	F23,
	F24,
	NumLock = 0x90,
	ScrollLock,
	LeftShift = 0xA0,
	RightShift,
	LeftCtrl,
	RightCtrl,
	LeftAlt,
	RightAlt,
	VolumeMute = 0xAD,
	VolumeDown,
	VolumeUp,
	SemiColonOEM = 0xBA,
	PlusOEM,
	CommaOEM,
	MinusOEM,
	DotOEM,
	SlashOEM,
	TildeOEM,
	OpenSquareBracketOEM = 0xDB,
	BackSlashOEM,
	CloseSquareBracketOEM,
	QuoteOEM,
};
