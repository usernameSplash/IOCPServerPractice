
#include "EchoServer.h"

bool EchoServer::Initialize(void)
{
	wprintf(L"# Echo Server Start\n");

	_monitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, NULL);
	if (_monitorThread == NULL)
	{
		wprintf(L"# Begin Monitor Thread Failed\n");
		return false;
	}

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

void EchoServer::OnInitialize(void)
{
	wprintf(L"# OnInitialize - Network Library Start\n");
	return;
}

bool EchoServer::OnConnectionRequest(const wchar_t* ip, const short port)
{
	return true;
}

void EchoServer::OnAccept(const SessionID sessionId)
{
	// Accept handle in Contents
	//wprintf(L"# OnAccept\n");
	return;
}

void EchoServer::OnRelease(const SessionID sessionId)
{
	// Release handle in Contents
}

void EchoServer::OnRecv(const SessionID sessionId, SPacket* packet)
{
	// Todo: Categorize by message type

	SessionID itselfId = sessionId;

	EchoData echoData;
	(*packet) >> echoData;

	//wprintf(L"%lld\n", echoData);

	SPacket sendPacket;
	sendPacket << echoData;

	SendPacket(itselfId, &sendPacket);
}

void EchoServer::OnError(const int errorCode, const wchar_t* errorMsg)
{
	// Todo: Handle Error defined by User
}

unsigned int WINAPI EchoServer::MonitorThread(void* arg)
{
	EchoServer* instance = (EchoServer*)arg;

	DWORD threadId = GetCurrentThreadId();

	wprintf(L"# Monitor Thread Start : %d\n", threadId);

	while (instance->IsActive())
	{
		Sleep(UPDATE_TIMEOUT);

		instance->UpdateMonitoringData();

		wprintf(L"\nESC : Quit\n");
		wprintf(L"===================================================\n");
		wprintf(L"Server IP : %s, Port : %d\n", instance->_IP, instance->_port);
		wprintf(L"===================================================\n");
		wprintf(L" [Network Statistics]\n");
		wprintf(L"  Session Count : %d\n\n", instance->GetSessionCount());

		wprintf(L"  Accept Total : %d\n", instance->GetAcceptTotal());
		wprintf(L"  Disconnect Total : %d\n\n", instance->GetDisconnectTotal());
		
		wprintf(L"  Accept TPS : %d\n", instance->GetAcceptTPS());
		wprintf(L"  Disconnect TPS : %d\n\n", instance->GetDisconnectTPS());

		wprintf(L"  Recv TPS : %d\n", instance->GetRecvTPS());
		wprintf(L"  Send TPS : %d\n\n", instance->GetSendTPS());
	}

	wprintf(L"# Monitor Thread End : %d\n", threadId);

	return 0;
}