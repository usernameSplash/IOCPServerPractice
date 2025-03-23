
#include "EchoClient.h"

#include <cstdio>

bool EchoClient::ClientInitialize(void)
{
	_contentsThread = (HANDLE)_beginthreadex(NULL, 0, ContentsThread, this, 0, NULL);
	if (_contentsThread == NULL)
	{
		wprintf(L"# Begin Contents Thraed Failed\n");
		return false;
	}

	_echoEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (_echoEvent == NULL)
	{
		wprintf(L"# Create Event Failed\n");
		return false;
	}

	if (IClient::NetworkInitialize(SERVER_ADDRESS, SERVER_PORT, 4, 2, true, true) == false)
	{
		return false;
	}

	return true;
}
void EchoClient::ClientTerminate(void)
{
	IClient::NetworkTerminate();

	SetEvent(_echoEvent);

	_isAlive = false;

	return;
}

unsigned int WINAPI EchoClient::ContentsThread(void* arg)
{
	EchoClient* instance = (EchoClient*)arg;

	while(true)
	{
		if (instance->IsInitialize())
		{
			break;
		}
		Sleep(1000);
	}

	while (true)
	{
		__int64 sendData;
		SPacket sendPacket;

		wprintf(L"\n\n=======================\n");
		wprintf(L"Input Integer Value : ");
		wscanf(L"%lld", &sendData);

		if (instance->IsActive() == false)
		{
			break;
		}

		sendPacket << sendData;

		instance->SendPacket(&sendPacket);

		WaitForSingleObject(instance->_echoEvent, INFINITE);
	}

	return 0;
}

void EchoClient::OnInitialize(void)
{
	wprintf(L"# Echo Client Initialize\n");
}

void EchoClient::OnConnect(void)
{
	wprintf(L"# Connect on Server\n");
}

void EchoClient::OnDisconnect(void)
{
	wprintf(L"# Disconnect from Server\n");
}

void EchoClient::OnRecv(SPacket* packet)
{
	__int64 echoData;
	(*packet) >> echoData;

	wprintf(L"# Message From Server : %lld\n", echoData);
	SetEvent(_echoEvent);
}