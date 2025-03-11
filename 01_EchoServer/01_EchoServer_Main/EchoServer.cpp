
#include "EchoServer.h"

bool EchoServer::Initialize(void)
{
	wprintf(L"# Echo Server Start\n");

	if (IServer::Initialize(SERVER_ADDRESS, SERVER_PORT, 16, 8, true, true, SESSION_MAX) == false)
	{
		return false;
	}

	return true;
}

void EchoServer::Terminate(void)
{
	IServer::Terminate();

	_isAlive = false;

	wprintf(L"# Echo Server Terminate\n");

	return;
}