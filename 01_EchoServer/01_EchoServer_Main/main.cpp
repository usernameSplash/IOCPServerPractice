
#include "EchoServer.h"

#include <cstdio>
#include <Windows.h>
#include <timeapi.h>

#pragma comment(lib, "winmm.lib")

EchoServer g_service;

int wmain(void)
{
	timeBeginPeriod(1);

	if (g_service.Initialize())
	{
		return 0;
	}

	while (g_service.IsAlive())
	{
		Sleep(1000);

		if (GetAsyncKeyState(VK_ESCAPE))
		{
			g_service.Terminate();
		}
	}
	return 0;
}