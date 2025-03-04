
#include "Server.h"

#include <cstdio>

IServer::IServer()
{
	// To do...
}

bool IServer::Initialize(wchar_t* IP, short port, int numOfWorkerThread, int numOfConcurrentWorkerThread, bool nagle, bool zeroCopy, int numSessionMax)
{
	wcsncpy(_IP, IP, 16);
	_port = port;
	_numSessionMax = numSessionMax;

	WSADATA wsa;
	int startRet = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (startRet != 0)
	{
		wprintf(L"# WSAStartup Failed\n");
		return false;
	}

	_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_listenSocket == INVALID_SOCKET)
	{
		wprintf(L"# Invalid Listen Socket\n");
		return false;
	}

	// set Nagle
	if (nagle)
	{
		linger lingerOption;
		lingerOption.l_onoff = 1;
		lingerOption.l_linger = 0;
		int setSockOptRet = setsockopt(_listenSocket, SOL_SOCKET, SO_LINGER, (const char*)&lingerOption, sizeof(lingerOption));
		if (setSockOptRet == SOCKET_ERROR)
		{
			wprintf(L"# Setting Linger Option Failed\n");
			return false;
		}
	}

	// set Zero Copy
	if (zeroCopy)
	{
		int sendBufSize = 0;
		int setSockOptRet = setsockopt(_listenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufSize, sizeof(sendBufSize));
		if (setSockOptRet == SOCKET_ERROR)
		{
			wprintf(L"# Setting Send Buffer Size to Zero Failed\n");
			return false;
		}
	}

	// bind Listen Socket
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(_port);
	int bindRet = bind(_listenSocket, (const sockaddr*)&addr, sizeof(addr));
	if (bindRet == SOCKET_ERROR)
	{
		wprintf(L"# Binding Listen Socket Failed\n");
		return false;
	}

	// start listen
	int listenRet = listen(_listenSocket, SOMAXCONN);
	if(listenRet == SOCKET_ERROR)
	{
		wprintf(L"# Listen Failed\n");
		return false;
	}

	// create IOCP
	_networkIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numOfConcurrentWorkerThread);
	if (_networkIOCP == NULL)
	{
		wprintf(L"# Create Network IOCP Failed\n");
		return false;
	}

	// create accept thread
	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL);
	if (_acceptThread == NULL)
	{
		wprintf(L"# Begin Accept Thread Failed\n");
		return false;
	}

	// create network threads
	_networkThreads = new HANDLE[numOfWorkerThread];
	for (int iCnt = 0; iCnt < numOfWorkerThread; ++iCnt)
	{
		_networkThreads[iCnt] = (HANDLE)_beginthreadex(NULL, 0, NetworkThread, this, 0, NULL);
		if (_networkThreads[iCnt] == NULL)
		{
			wprintf(L"# Begin Network Thread Failed\n");
			return false;
		}
	}

	OnInitialize();

	return true;
}

void IServer::Terminate(void)
{

}

bool IServer::DisconnectSession(const __int64 sessionId)
{

}

bool IServer::SendPacket(const __int64 sessionId, SPacket* packet)
{

}

unsigned int WINAPI IServer::AcceptThread(void* arg)
{

}

unsigned int WINAPI IServer::NetworkThread(void* arg)
{
	
}

void IServer::HandleRecv(Session* session, int recvByte)
{

}

void IServer::HandleSend(Session* session, int sendByte)
{

}

void IServer::RecvPost(Session* session)
{

}

void IServer::SendPost(Session* session)
{

}

