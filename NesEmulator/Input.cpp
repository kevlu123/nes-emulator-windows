#include "Input.h"
#include <Windows.h>
#include <unordered_map>
#include "EmuFileException.h"
#include "Emulator.h"
#include "Ppu.h"

Input::Input() :
	keys{}
{
}

void Input::GetLocalCursorPos(int& x, int& y)
{
	if (hWnd == NULL)
		throw EmuFileException("hWnd not set");

	// Get pixel pos
	POINT pt;
	::GetCursorPos(&pt);
	::ScreenToClient(hWnd, &pt);

	// Get window size
	RECT rc;
	::GetClientRect(hWnd, &rc);
	POINT tl;
	tl.x = rc.left;
	tl.y = rc.top;
	::ScreenToClient(hWnd, &tl);
	POINT br;
	br.x = rc.right;
	br.y = rc.bottom;
	::ScreenToClient(hWnd, &br);
	float clientWidth = static_cast<float>(br.x - tl.x);
	float clientHeight = static_cast<float>(br.y - tl.y);
	float horizontalPixelCount = static_cast<float>(Ppu::DRAWABLE_WIDTH + Emulator::DEBUG_WIDTH);
	float verticalPixelCount = static_cast<float>(Ppu::DRAWABLE_HEIGHT);

	// Convert to virtual pos
	if (clientWidth / horizontalPixelCount >= Graphics::PIXEL_ASPECT * clientHeight / verticalPixelCount)
	{
		// Narrow
		pt.x = (LONG)((float)pt.x - ((clientWidth - clientHeight / verticalPixelCount * Graphics::PIXEL_ASPECT * horizontalPixelCount) / 2.0f));
		pt.y = (LONG)((float)pt.y / (clientHeight / verticalPixelCount * 8));
		pt.x = (LONG)((float)pt.x / (clientHeight / verticalPixelCount * Graphics::PIXEL_ASPECT * 8));
	}
	else
	{
		// Wide
		pt.y = (LONG)((float)pt.y - ((clientHeight - clientWidth / horizontalPixelCount / Graphics::PIXEL_ASPECT * verticalPixelCount) / 2.0f));
		pt.x = (LONG)((float)pt.x / (clientWidth / horizontalPixelCount * 8));
		pt.y = (LONG)((float)pt.y / (clientWidth / horizontalPixelCount / Graphics::PIXEL_ASPECT * 8));
	}

	pt.x -= Ppu::DRAWABLE_WIDTH / 8;
	x = pt.x;
	y = pt.y;
}

void Input::SetHwnd(HWND hWnd)
{
	this->hWnd = hWnd;
}

void Input::UpdateKeys(bool focus)
{
	for (const auto& k : listening)
	{
		int i = static_cast<int>(k.first);
		bool heldBefore = GetKey((Key)i);
		bool heldNow = focus && (GetAsyncKeyState(i) & 0x8000);
		if (heldNow)
		{
			if (heldBefore)
				keys[i] = KeyPhase::Held;
			else
				keys[i] = KeyPhase::Begin;
		}
		else
		{
			keys[i] = KeyPhase::NotHeld;
		}
	}
}

bool Input::GetKey(Key key) const
{
	return keys[(int)key] == KeyPhase::Begin || keys[(int)key] == KeyPhase::Held;
}

bool Input::GetKeyDown(Key key) const
{
	return keys[(int)key] == KeyPhase::Begin;
}

Key Input::GetKeyPressed() const
{
	for (const auto& p : listening)
	{
		Key key = p.first;
		if (key != Key::Ctrl && key != Key::Alt && key != Key::Shift && GetKeyDown(key))
			return key;
	}
	return Key::None;
}

int Input::GetHexKeyPressed() const
{
	for (int k = 0; k < 10; k++)
		if (GetKeyDown(static_cast<Key>((int)Key::Num0 + k)))
			return k;
	for (int k = 0; k < 6; k++)
		if (GetKeyDown(static_cast<Key>((int)Key::A + k)))
			return k + 10;
	return -1;
}

void Input::ListenToKey(Key key)
{
	if (key == Key::None)
		return;
	for (size_t i = 0; i < listening.size(); i++)
		if (listening[i].first == key)
		{
			listening[i].second++;
			return;
		}
	listening.push_back(std::make_pair(key, 1));
}

void Input::StopListeningToKey(Key key)
{
	if (key == Key::None)
		return;
	for (size_t i = 0; i < listening.size(); i++)
	{
		auto& entry = listening[i];
		if (entry.first == key)
		{
			entry.second--;
			if (entry.second == 0)
			{
				keys[static_cast<int>(key)] = KeyPhase::NotHeld;
				listening.erase(listening.begin() + i);
			}
			return;
		}
	}
	throw std::logic_error("Attempted to stop listening to a key that was not being listened to");
}

void Input::ListenAll(bool listen)
{
	if (listen)
		for (int i = 0; i < 255; i++)
			ListenToKey((Key)i);
	else
		for (int i = 0; i < 255; i++)
			StopListeningToKey((Key)i);
}

Key Input::StringToKey(const std::wstring& s)
{
	static std::unordered_map<std::wstring, Key> m = []()
	{
		std::unordered_map<std::wstring, Key> m;
		m.insert(std::make_pair<std::wstring, Key>(L"LMB", Key::LeftMouseButton));
		m.insert(std::make_pair<std::wstring, Key>(L"RMB", Key::RightMouseButton));
		m.insert(std::make_pair<std::wstring, Key>(L"MMB", Key::MiddleMouseButton));
		m.insert(std::make_pair<std::wstring, Key>(L"MB0", Key::ExtraMouseButtonA));
		m.insert(std::make_pair<std::wstring, Key>(L"MB1", Key::ExtraMouseButtonB));
		m.insert(std::make_pair<std::wstring, Key>(L"Backspace", Key::BackSpace));
		m.insert(std::make_pair<std::wstring, Key>(L"Tab", Key::Tab));
		m.insert(std::make_pair<std::wstring, Key>(L"Enter", Key::Enter));
		m.insert(std::make_pair<std::wstring, Key>(L"Shift", Key::Shift));
		m.insert(std::make_pair<std::wstring, Key>(L"Ctrl", Key::Ctrl));
		m.insert(std::make_pair<std::wstring, Key>(L"Alt", Key::Alt));
		m.insert(std::make_pair<std::wstring, Key>(L"Capslock", Key::CapsLock));
		m.insert(std::make_pair<std::wstring, Key>(L"Esc", Key::Escape));
		m.insert(std::make_pair<std::wstring, Key>(L"Space", Key::SpaceBar));
		m.insert(std::make_pair<std::wstring, Key>(L"Left", Key::LeftArrow));
		m.insert(std::make_pair<std::wstring, Key>(L"Up", Key::UpArrow));
		m.insert(std::make_pair<std::wstring, Key>(L"Right", Key::RightArrow));
		m.insert(std::make_pair<std::wstring, Key>(L"Down", Key::DownArrow));
		m.insert(std::make_pair<std::wstring, Key>(L"Ins", Key::Insert));
		m.insert(std::make_pair<std::wstring, Key>(L"Del", Key::Delete));
		m.insert(std::make_pair<std::wstring, Key>(L"0", Key::Num0));
		m.insert(std::make_pair<std::wstring, Key>(L"1", Key::Num1));
		m.insert(std::make_pair<std::wstring, Key>(L"2", Key::Num2));
		m.insert(std::make_pair<std::wstring, Key>(L"3", Key::Num3));
		m.insert(std::make_pair<std::wstring, Key>(L"4", Key::Num4));
		m.insert(std::make_pair<std::wstring, Key>(L"5", Key::Num5));
		m.insert(std::make_pair<std::wstring, Key>(L"6", Key::Num6));
		m.insert(std::make_pair<std::wstring, Key>(L"7", Key::Num7));
		m.insert(std::make_pair<std::wstring, Key>(L"8", Key::Num8));
		m.insert(std::make_pair<std::wstring, Key>(L"9", Key::Num9));
		m.insert(std::make_pair<std::wstring, Key>(L"A", Key::A));
		m.insert(std::make_pair<std::wstring, Key>(L"B", Key::B));
		m.insert(std::make_pair<std::wstring, Key>(L"C", Key::C));
		m.insert(std::make_pair<std::wstring, Key>(L"D", Key::D));
		m.insert(std::make_pair<std::wstring, Key>(L"E", Key::E));
		m.insert(std::make_pair<std::wstring, Key>(L"F", Key::F));
		m.insert(std::make_pair<std::wstring, Key>(L"G", Key::G));
		m.insert(std::make_pair<std::wstring, Key>(L"H", Key::H));
		m.insert(std::make_pair<std::wstring, Key>(L"I", Key::I));
		m.insert(std::make_pair<std::wstring, Key>(L"J", Key::J));
		m.insert(std::make_pair<std::wstring, Key>(L"K", Key::K));
		m.insert(std::make_pair<std::wstring, Key>(L"L", Key::L));
		m.insert(std::make_pair<std::wstring, Key>(L"M", Key::M));
		m.insert(std::make_pair<std::wstring, Key>(L"N", Key::N));
		m.insert(std::make_pair<std::wstring, Key>(L"O", Key::O));
		m.insert(std::make_pair<std::wstring, Key>(L"P", Key::P));
		m.insert(std::make_pair<std::wstring, Key>(L"Q", Key::Q));
		m.insert(std::make_pair<std::wstring, Key>(L"R", Key::R));
		m.insert(std::make_pair<std::wstring, Key>(L"S", Key::S));
		m.insert(std::make_pair<std::wstring, Key>(L"T", Key::T));
		m.insert(std::make_pair<std::wstring, Key>(L"U", Key::U));
		m.insert(std::make_pair<std::wstring, Key>(L"V", Key::V));
		m.insert(std::make_pair<std::wstring, Key>(L"W", Key::W));
		m.insert(std::make_pair<std::wstring, Key>(L"X", Key::X));
		m.insert(std::make_pair<std::wstring, Key>(L"Y", Key::Y));
		m.insert(std::make_pair<std::wstring, Key>(L"Z", Key::Z));
		m.insert(std::make_pair<std::wstring, Key>(L"LWnd", Key::LeftWindows));
		m.insert(std::make_pair<std::wstring, Key>(L"RWnd", Key::RightWindows));
		m.insert(std::make_pair<std::wstring, Key>(L"Num0", Key::Numpad0));
		m.insert(std::make_pair<std::wstring, Key>(L"Num1", Key::Numpad1));
		m.insert(std::make_pair<std::wstring, Key>(L"Num2", Key::Numpad2));
		m.insert(std::make_pair<std::wstring, Key>(L"Num3", Key::Numpad3));
		m.insert(std::make_pair<std::wstring, Key>(L"Num4", Key::Numpad4));
		m.insert(std::make_pair<std::wstring, Key>(L"Num5", Key::Numpad5));
		m.insert(std::make_pair<std::wstring, Key>(L"Num6", Key::Numpad6));
		m.insert(std::make_pair<std::wstring, Key>(L"Num7", Key::Numpad7));
		m.insert(std::make_pair<std::wstring, Key>(L"Num8", Key::Numpad8));
		m.insert(std::make_pair<std::wstring, Key>(L"Num9", Key::Numpad9));
		m.insert(std::make_pair<std::wstring, Key>(L"Num*", Key::NumpadAsterisk));
		m.insert(std::make_pair<std::wstring, Key>(L"Num+", Key::NumpadPlus));
		m.insert(std::make_pair<std::wstring, Key>(L"Num-", Key::NumpadMinus));
		m.insert(std::make_pair<std::wstring, Key>(L"Num.", Key::NumpadDot));
		m.insert(std::make_pair<std::wstring, Key>(L"Num/", Key::NumpadSlash));
		m.insert(std::make_pair<std::wstring, Key>(L"F1", Key::F1));
		m.insert(std::make_pair<std::wstring, Key>(L"F2", Key::F2));
		m.insert(std::make_pair<std::wstring, Key>(L"F3", Key::F3));
		m.insert(std::make_pair<std::wstring, Key>(L"F4", Key::F4));
		m.insert(std::make_pair<std::wstring, Key>(L"F5", Key::F5));
		m.insert(std::make_pair<std::wstring, Key>(L"F6", Key::F6));
		m.insert(std::make_pair<std::wstring, Key>(L"F7", Key::F7));
		m.insert(std::make_pair<std::wstring, Key>(L"F8", Key::F8));
		m.insert(std::make_pair<std::wstring, Key>(L"F9", Key::F9));
		m.insert(std::make_pair<std::wstring, Key>(L"F10", Key::F10));
		m.insert(std::make_pair<std::wstring, Key>(L"F11", Key::F11));
		m.insert(std::make_pair<std::wstring, Key>(L"F12", Key::F12));
		m.insert(std::make_pair<std::wstring, Key>(L"F13", Key::F13));
		m.insert(std::make_pair<std::wstring, Key>(L"F14", Key::F14));
		m.insert(std::make_pair<std::wstring, Key>(L"F15", Key::F15));
		m.insert(std::make_pair<std::wstring, Key>(L"F16", Key::F16));
		m.insert(std::make_pair<std::wstring, Key>(L"F17", Key::F17));
		m.insert(std::make_pair<std::wstring, Key>(L"F18", Key::F18));
		m.insert(std::make_pair<std::wstring, Key>(L"F19", Key::F19));
		m.insert(std::make_pair<std::wstring, Key>(L"F20", Key::F20));
		m.insert(std::make_pair<std::wstring, Key>(L"F21", Key::F21));
		m.insert(std::make_pair<std::wstring, Key>(L"F22", Key::F22));
		m.insert(std::make_pair<std::wstring, Key>(L"F23", Key::F23));
		m.insert(std::make_pair<std::wstring, Key>(L"F24", Key::F24));
		m.insert(std::make_pair<std::wstring, Key>(L"Numlock", Key::NumLock));
		m.insert(std::make_pair<std::wstring, Key>(L"Scrlock", Key::ScrollLock));
		m.insert(std::make_pair<std::wstring, Key>(L"LShift", Key::LeftShift));
		m.insert(std::make_pair<std::wstring, Key>(L"RShift", Key::RightShift));
		m.insert(std::make_pair<std::wstring, Key>(L"LCtrl", Key::LeftCtrl));
		m.insert(std::make_pair<std::wstring, Key>(L"RCtrl", Key::RightCtrl));
		m.insert(std::make_pair<std::wstring, Key>(L"LAlt", Key::LeftAlt));
		m.insert(std::make_pair<std::wstring, Key>(L"RAlt", Key::RightAlt));
		m.insert(std::make_pair<std::wstring, Key>(L"VolMute", Key::VolumeMute));
		m.insert(std::make_pair<std::wstring, Key>(L"VolDown", Key::VolumeDown));
		m.insert(std::make_pair<std::wstring, Key>(L"VolUp", Key::VolumeUp));
		m.insert(std::make_pair<std::wstring, Key>(L";", Key::SemiColonOEM));
		m.insert(std::make_pair<std::wstring, Key>(L"+", Key::PlusOEM));
		m.insert(std::make_pair<std::wstring, Key>(L",", Key::CommaOEM));
		m.insert(std::make_pair<std::wstring, Key>(L"-", Key::MinusOEM));
		m.insert(std::make_pair<std::wstring, Key>(L".", Key::DotOEM));
		m.insert(std::make_pair<std::wstring, Key>(L"/", Key::SlashOEM));
		m.insert(std::make_pair<std::wstring, Key>(L"`", Key::TildeOEM));
		m.insert(std::make_pair<std::wstring, Key>(L"[", Key::OpenSquareBracketOEM));
		m.insert(std::make_pair<std::wstring, Key>(L"\\", Key::BackSlashOEM));
		m.insert(std::make_pair<std::wstring, Key>(L"]", Key::CloseSquareBracketOEM));
		m.insert(std::make_pair<std::wstring, Key>(L"\"", Key::QuoteOEM));
		return m;
	}();
	if (m.find(s) == m.end())
		return Key::None;
	return m[s];
}

const std::wstring& Input::KeyToString(Key k)
{
	static std::unordered_map<Key, std::wstring> m = []()
	{
		std::unordered_map<Key, std::wstring> m;
		m.insert(std::make_pair<Key, std::wstring>(Key::LeftMouseButton, L"LMB"));
		m.insert(std::make_pair<Key, std::wstring>(Key::RightMouseButton, L"RMB"));
		m.insert(std::make_pair<Key, std::wstring>(Key::MiddleMouseButton, L"MMB"));
		m.insert(std::make_pair<Key, std::wstring>(Key::ExtraMouseButtonA, L"MB0"));
		m.insert(std::make_pair<Key, std::wstring>(Key::ExtraMouseButtonB, L"MB1"));
		m.insert(std::make_pair<Key, std::wstring>(Key::BackSpace, L"Backspace"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Tab, L"Tab"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Enter, L"Enter"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Shift, L"Shift"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Ctrl, L"Ctrl"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Alt, L"Alt"));
		m.insert(std::make_pair<Key, std::wstring>(Key::CapsLock, L"Capslock"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Escape, L"Esc"));
		m.insert(std::make_pair<Key, std::wstring>(Key::SpaceBar, L"Space"));
		m.insert(std::make_pair<Key, std::wstring>(Key::LeftArrow, L"Left"));
		m.insert(std::make_pair<Key, std::wstring>(Key::UpArrow, L"Up"));
		m.insert(std::make_pair<Key, std::wstring>(Key::RightArrow, L"Right"));
		m.insert(std::make_pair<Key, std::wstring>(Key::DownArrow, L"Down"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Insert, L"Ins"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Delete, L"Del"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Num0, L"0"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Num1, L"1"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Num2, L"2"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Num3, L"3"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Num4, L"4"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Num5, L"5"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Num6, L"6"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Num7, L"7"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Num8, L"8"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Num9, L"9"));
		m.insert(std::make_pair<Key, std::wstring>(Key::A, L"A"));
		m.insert(std::make_pair<Key, std::wstring>(Key::B, L"B"));
		m.insert(std::make_pair<Key, std::wstring>(Key::C, L"C"));
		m.insert(std::make_pair<Key, std::wstring>(Key::D, L"D"));
		m.insert(std::make_pair<Key, std::wstring>(Key::E, L"E"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F, L"F"));
		m.insert(std::make_pair<Key, std::wstring>(Key::G, L"G"));
		m.insert(std::make_pair<Key, std::wstring>(Key::H, L"H"));
		m.insert(std::make_pair<Key, std::wstring>(Key::I, L"I"));
		m.insert(std::make_pair<Key, std::wstring>(Key::J, L"J"));
		m.insert(std::make_pair<Key, std::wstring>(Key::K, L"K"));
		m.insert(std::make_pair<Key, std::wstring>(Key::L, L"L"));
		m.insert(std::make_pair<Key, std::wstring>(Key::M, L"M"));
		m.insert(std::make_pair<Key, std::wstring>(Key::N, L"N"));
		m.insert(std::make_pair<Key, std::wstring>(Key::O, L"O"));
		m.insert(std::make_pair<Key, std::wstring>(Key::P, L"P"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Q, L"Q"));
		m.insert(std::make_pair<Key, std::wstring>(Key::R, L"R"));
		m.insert(std::make_pair<Key, std::wstring>(Key::S, L"S"));
		m.insert(std::make_pair<Key, std::wstring>(Key::T, L"T"));
		m.insert(std::make_pair<Key, std::wstring>(Key::U, L"U"));
		m.insert(std::make_pair<Key, std::wstring>(Key::V, L"V"));
		m.insert(std::make_pair<Key, std::wstring>(Key::W, L"W"));
		m.insert(std::make_pair<Key, std::wstring>(Key::X, L"X"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Y, L"Y"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Z, L"Z"));
		m.insert(std::make_pair<Key, std::wstring>(Key::LeftWindows, L"LWnd"));
		m.insert(std::make_pair<Key, std::wstring>(Key::RightWindows, L"RWnd"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Numpad0, L"Num0"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Numpad1, L"Num1"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Numpad2, L"Num2"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Numpad3, L"Num3"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Numpad4, L"Num4"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Numpad5, L"Num5"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Numpad6, L"Num6"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Numpad7, L"Num7"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Numpad8, L"Num8"));
		m.insert(std::make_pair<Key, std::wstring>(Key::Numpad9, L"Num9"));
		m.insert(std::make_pair<Key, std::wstring>(Key::NumpadAsterisk, L"Num*"));
		m.insert(std::make_pair<Key, std::wstring>(Key::NumpadPlus, L"Num+"));
		m.insert(std::make_pair<Key, std::wstring>(Key::NumpadMinus, L"Num-"));
		m.insert(std::make_pair<Key, std::wstring>(Key::NumpadDot, L"Num."));
		m.insert(std::make_pair<Key, std::wstring>(Key::NumpadSlash, L"Num/"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F1, L"F1"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F2, L"F2"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F3, L"F3"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F4, L"F4"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F5, L"F5"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F6, L"F6"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F7, L"F7"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F8, L"F8"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F9, L"F9"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F10, L"F10"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F11, L"F11"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F12, L"F12"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F13, L"F13"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F14, L"F14"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F15, L"F15"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F16, L"F16"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F17, L"F17"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F18, L"F18"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F19, L"F19"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F20, L"F20"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F21, L"F21"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F22, L"F22"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F23, L"F23"));
		m.insert(std::make_pair<Key, std::wstring>(Key::F24, L"F24"));
		m.insert(std::make_pair<Key, std::wstring>(Key::NumLock, L"Numlock"));
		m.insert(std::make_pair<Key, std::wstring>(Key::ScrollLock, L"Scrlock"));
		m.insert(std::make_pair<Key, std::wstring>(Key::LeftShift, L"LShift"));
		m.insert(std::make_pair<Key, std::wstring>(Key::RightShift, L"RShift"));
		m.insert(std::make_pair<Key, std::wstring>(Key::LeftCtrl, L"LCtrl"));
		m.insert(std::make_pair<Key, std::wstring>(Key::RightCtrl, L"RCtrl"));
		m.insert(std::make_pair<Key, std::wstring>(Key::LeftAlt, L"LAlt"));
		m.insert(std::make_pair<Key, std::wstring>(Key::RightAlt, L"RAlt"));
		m.insert(std::make_pair<Key, std::wstring>(Key::VolumeMute, L"VolMute"));
		m.insert(std::make_pair<Key, std::wstring>(Key::VolumeDown, L"VolDown"));
		m.insert(std::make_pair<Key, std::wstring>(Key::VolumeUp, L"VolUp"));
		m.insert(std::make_pair<Key, std::wstring>(Key::SemiColonOEM, L";"));
		m.insert(std::make_pair<Key, std::wstring>(Key::PlusOEM, L"+"));
		m.insert(std::make_pair<Key, std::wstring>(Key::CommaOEM, L","));
		m.insert(std::make_pair<Key, std::wstring>(Key::MinusOEM, L"-"));
		m.insert(std::make_pair<Key, std::wstring>(Key::DotOEM, L"."));
		m.insert(std::make_pair<Key, std::wstring>(Key::SlashOEM, L"/"));
		m.insert(std::make_pair<Key, std::wstring>(Key::TildeOEM, L"`"));
		m.insert(std::make_pair<Key, std::wstring>(Key::OpenSquareBracketOEM, L"["));
		m.insert(std::make_pair<Key, std::wstring>(Key::BackSlashOEM, L"\\"));
		m.insert(std::make_pair<Key, std::wstring>(Key::CloseSquareBracketOEM, L"]"));
		m.insert(std::make_pair<Key, std::wstring>(Key::QuoteOEM, L"\""));
		return m;
	}();
	if (m.find(k) == m.end())
	{
		static std::wstring def = L"None";
		return def;
	}
	return m[k];
}
