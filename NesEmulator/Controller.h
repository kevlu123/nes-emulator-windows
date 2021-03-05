#pragma once
#include "Input.h"
#include <string>
#include <memory>

class Nes;

struct ControllerTemplate
{
	ControllerTemplate();
	ControllerTemplate(const std::wstring& name, Key a, Key b, Key select, Key start, Key up, Key down, Key left, Key right);
	Key& KeyA();
	Key& KeyB();
	Key& KeySelect();
	Key& KeyStart();
	Key& KeyUp();
	Key& KeyDown();
	Key& KeyLeft();
	Key& KeyRight();
	std::wstring name;
	Key keyMappings[8];
	bool enabled;
};

class Controller
{
public:
	Controller();
	Controller(const std::unique_ptr<ControllerTemplate>& tmplate, Input& input);
	~Controller();
	void SetState();
	uint8_t Read();
	void UpdateKeyMappings();
	const ControllerTemplate* tmplate;
private:
	Input* input;
	Key keyMappings[8];
	uint8_t state;
};
