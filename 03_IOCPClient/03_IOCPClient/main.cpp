
#include "EchoClient.h"

#include <Windows.h>
#include <timeapi.h>

#pragma comment(lib, "winmm.lib")

EchoClient g_client;

int wmain(void)
{
	timeBeginPeriod(1);

	if (g_client.ClientInitialize() == false)
	{
		return 0;
	}

	while (g_client.IsAlive())
	{
		Sleep(1000);

		if (GetAsyncKeyState(VK_ESCAPE))
		{
			g_client.ClientTerminate();
		}
	}

	return 0;
}