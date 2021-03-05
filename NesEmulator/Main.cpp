#include "Emulator.h"
#include "Graphics.h"

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR cmdArgs, int)
{
#ifndef _DEBUG
	try
	{
#endif
		Emulator emu;
		return emu.Run(cmdArgs) ? 0 : 1;
#ifndef _DEBUG
	}
	catch (Graphics::GraphicsException& ex)
	{
		MessageBoxA(
			NULL,
			ex.what(),
			"Fatal Error D3D11",
			MB_OK | MB_ICONERROR
		);
		return -2;
	}
	catch (std::exception& ex)
	{
		MessageBoxA(
			NULL,
			ex.what(),
			"Fatal Error",
			MB_OK | MB_ICONERROR
		);
		return -1;
	}
	catch (...)
	{
		MessageBoxA(
			NULL,
			"No information",
			"Fatal Error",
			MB_OK | MB_ICONERROR
		);
		return -1;
	}
#endif
}