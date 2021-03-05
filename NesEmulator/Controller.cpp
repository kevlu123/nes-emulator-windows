#include "Controller.h"

ControllerTemplate::ControllerTemplate() :
	keyMappings{ Key::None, Key::None, Key::None, Key::None, Key::None, Key::None, Key::None, Key::None },
	enabled(true)
{
}

ControllerTemplate::ControllerTemplate(const std::wstring& name, Key a, Key b, Key select, Key start, Key up, Key down, Key left, Key right) :
	name(name),
	keyMappings{ right, left, down, up, start, select, b, a },
	enabled(true)
{
}

Key& ControllerTemplate::KeyA()
{
	return keyMappings[7];
}

Key& ControllerTemplate::KeyB()
{
	return keyMappings[6];
}

Key& ControllerTemplate::KeySelect()
{
	return keyMappings[5];
}

Key& ControllerTemplate::KeyStart()
{
	return keyMappings[4];
}

Key& ControllerTemplate::KeyUp()
{
	return keyMappings[3];
}

Key& ControllerTemplate::KeyDown()
{
	return keyMappings[2];
}

Key& ControllerTemplate::KeyLeft()
{
	return keyMappings[1];
}

Key& ControllerTemplate::KeyRight()
{
	return keyMappings[0];
}

Controller::Controller(const std::unique_ptr<ControllerTemplate>& tmplate, Input& input) :
	input(&input),
	keyMappings{},
	state(0),
	tmplate(tmplate.get())
{
	std::memcpy(keyMappings, tmplate->keyMappings, 8 * sizeof(Key));
	for (Key key : keyMappings)
		input.ListenToKey(key);
}

Controller::Controller() :
	input(nullptr),
	state(0),
	tmplate(tmplate),
	keyMappings{}
{
}

Controller::~Controller()
{
	for (const auto& key : keyMappings)
		input->StopListeningToKey(key);
}

void Controller::SetState()
{
	state = 0;
	if (input != nullptr && tmplate->enabled && !(input->GetKey(Key::Alt) || input->GetKey(Key::Ctrl)))
	{
		for (size_t i = 0; i < std::size(keyMappings); i++)
		{
			state |= input->GetKey(keyMappings[i]) << i;
		}
	}
}

uint8_t Controller::Read()
{
	uint8_t res = state >> 7;
	state <<= 1;
	return res;
}

void Controller::UpdateKeyMappings()
{
	for (Key k : keyMappings)
		input->StopListeningToKey(k);
	std::memcpy(keyMappings, tmplate->keyMappings, 8 * sizeof(Key));
	for (Key key : keyMappings)
		input->ListenToKey(key);
}
