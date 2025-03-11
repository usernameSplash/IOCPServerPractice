
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

	_sessionMap.reserve(numSessionMax);
	InitializeSRWLock(&_sessionMapLock);

	OnInitialize();

	return true;
}

void IServer::Terminate(void)
{

}

bool IServer::DisconnectSession(const SessionID sessionId)
{
	AcquireSRWLockShared(&_sessionMapLock);
	
	std::unordered_map<SessionID, Session*>::iterator iter = _sessionMap.find(sessionId);
	if (iter == _sessionMap.end())
	{
		ReleaseSRWLockShared(&_sessionMapLock);
		return false;
	}

	Session* session = iter->second;

	AcquireSRWLockExclusive(&session->_lock);

	ReleaseSRWLockShared(&_sessionMapLock);

	session->_isActive = false;
	// Todo..

	ReleaseSRWLockExclusive(&session->_lock);
}

bool IServer::SendPacket(const SessionID sessionId, SPacket* packet)
{
	AcquireSRWLockShared(&_sessionMapLock);

	std::unordered_map<SessionID, Session*>::iterator iter = _sessionMap.find(sessionId);
	if (iter == _sessionMap.end())
	{
		ReleaseSRWLockShared(&_sessionMapLock);
		return false;
	}

	Session* session = iter->second;

	AcquireSRWLockExclusive(&session->_lock);

	ReleaseSRWLockShared(&_sessionMapLock);

	// Todo..

	ReleaseSRWLockExclusive(&session->_lock);
}

unsigned int WINAPI IServer::AcceptThread(void* arg)
{
	IServer* instance = (IServer*)arg;
	SessionID idProvider = 0;
	DWORD threadId = GetCurrentThreadId();

	int addrLen = sizeof(SOCKADDR_IN);

	wprintf(L"# Accept Thread Start : %d\n", threadId);

	while (true)
	{
		SOCKET clientSocket;
		SOCKADDR_IN clientAddr;

		clientSocket = accept(instance->_listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

		if (instance->_isActive == false)
		{
			break;
		}

		if (clientSocket == INVALID_SOCKET)
		{
			__debugbreak();
			instance->_isActive = false;
			break;
		}

#pragma region validate_client_connection
		/*
		wchar_t clientIp[16];
		short port;
		
		InetNtopW(AF_INET, &clientAddr.sin_addr.s_addr, clientIp, sizeof(clientIp));
		port = ntohs(clientAddr.sin_port);

		if (instance->OnConnectionRequest(clientIp, port) == false)
		{
			closesocket(clientSocket);
			continue;
		}
		*/
#pragma endregion

#pragma region create_new_session
		SessionID clientId = idProvider++;

		Session* newSession = new Session;
		newSession->Initialize(clientId, clientSocket, clientAddr);

		AcquireSRWLockExclusive(&instance->_sessionMapLock);
		instance->_sessionMap.insert(std::pair<SessionID, Session*>(clientId, newSession));
		ReleaseSRWLockExclusive(&instance->_sessionMapLock);

		InterlockedIncrement(&instance->_sessionCnt);
#pragma endregion

		instance->_acceptCnt++;
		instance->OnAccept(clientId);

		CreateIoCompletionPort((HANDLE)clientSocket, instance->_networkIOCP, (ULONG_PTR)newSession, 0);

		instance->RecvPost(newSession);
	}

	wprintf(L"# Accept Thread End : %d\n", threadId);

	return 0;
}

unsigned int WINAPI IServer::NetworkThread(void* arg)
{
	IServer* instance = (IServer*)arg;
	DWORD threadId = GetCurrentThreadId();

	wprintf(L"# Network Thread Start : %d\n", threadId);

	while (true)
	{
		DWORD transferredByte;
		Session* completionSession;
		MyOverlapped* overlapped;

		int gqcsRet = GetQueuedCompletionStatus((HANDLE)instance->_networkIOCP, &transferredByte, (PULONG_PTR)completionSession, (LPOVERLAPPED*)overlapped, INFINITE);

		if (instance->_isActive == false)
		{
			break;
		}

		if (overlapped->_type == eOverlappedType::RELEASE)
		{
			instance->HandleRelease(completionSession);
			continue;
		}

		AcquireSRWLockExclusive(&completionSession->_lock);

		if (gqcsRet == 0 || transferredByte == 0)
		{
			if (gqcsRet == 0)
			{
				DWORD errorCode = GetLastError();

				if (errorCode != WSAECONNABORTED || errorCode != WSAECONNRESET)
				{
					wprintf(L"# GQCS Returned Zero : %d\n", gqcsRet);
				}
			}
		}
		else
		{
			switch (overlapped->_type)
			{
			case eOverlappedType::RECV:
				{
					instance->HandleRecv(completionSession, transferredByte);
					break;
				}
			case eOverlappedType::SEND:
				{
					instance->HandleSend(completionSession, transferredByte);
					break;
				}
			}
		}

		if (InterlockedDecrement(&completionSession->_ioCount) == 0)
		{
			PostQueuedCompletionStatus((HANDLE)instance->_networkIOCP, 1, (ULONG_PTR)completionSession, (LPOVERLAPPED)&completionSession->_releaseOvl);
		}

		ReleaseSRWLockExclusive(&completionSession->_lock);
	}
}

// recv data to messages & call OnRecv
void IServer::HandleRecv(Session* session, int recvByte)
{
	session->_recvBuffer.MoveRear(recvByte);

	int bufferSize = recvByte;

	while (bufferSize > 0)
	{
		if (bufferSize <= sizeof(EchoPacketHeader))
		{
			break;
		}

		EchoPacketHeader header;
		session->_recvBuffer.Peek((char*)&header, sizeof(header));

		if (bufferSize < sizeof(header) + header._len)
		{
			break;
		}

		session->_recvBuffer.Dequeue(sizeof(header));
		
		SPacket packet;
		session->_recvBuffer.Peek((char*)packet.GetPayloadPtr(), header._len);
		session->_recvBuffer.Dequeue(header._len);

		OnRecv(session->_sessionId, &packet);
	}

	RecvPost(session);
}

void IServer::HandleSend(Session* session, int sendByte)
{
	session->_sendBuffer.MoveFront(sendByte);

	long prevSendFlag = InterlockedDecrement(&session->_sendStatus);
	
	if (prevSendFlag == 0)
	{
		SendPost(session);
	}
}

void IServer::HandleRelease(Session* session)
{
	SessionID id = session->_sessionId;

	AcquireSRWLockExclusive(&_sessionMapLock);

	std::unordered_map<SessionID, Session*>::iterator iter = _sessionMap.find(id);
	if (iter == _sessionMap.end())
	{
		ReleaseSRWLockExclusive(&_sessionMapLock);
		return;
	}

	_sessionMap.erase(iter);

	ReleaseSRWLockExclusive(&_sessionMapLock);

	AcquireSRWLockExclusive(&session->_lock);
	ReleaseSRWLockExclusive(&session->_lock);

	closesocket(session->_clientSocket);

	delete session;

	InterlockedIncrement(&_disconnectCnt);
	OnRelease(id);

	return;
}

void IServer::RecvPost(Session* session)
{
	if (session->_isActive == false)
	{
		return;
	}

	DWORD flag = 0;

	InterlockedIncrement(&session->_ioCount);
	ZeroMemory(&session->_recvOvl._ovl, sizeof(WSAOVERLAPPED));

	int freeSize = session->_recvBuffer.Capacity() - session->_recvBuffer.Size();

	WSABUF wsabuf[2];

	wsabuf[0].buf = session->_recvBuffer.GetRearBufferPtr();
	wsabuf[0].len = session->_recvBuffer.DirectEnqueueSize();
	wsabuf[1].buf = session->_recvBuffer.GetBufferPtr();
	wsabuf[1].len = freeSize - wsabuf[0].len;


	int recvRet = WSARecv(session->_clientSocket, wsabuf, 2, NULL, &flag, (LPWSAOVERLAPPED)&session->_recvOvl, NULL);

	if (recvRet == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();

		if (errorCode != ERROR_IO_PENDING)
		{
			if (errorCode != WSAECONNRESET || errorCode != WSAECONNABORTED)
			{
				wprintf(L"# WSARecv Error in Session %d\n", session->_sessionId);
			}
		}
	}

	return;
}


void IServer::SendPost(Session* session)
{
	if (session->_isActive == false)
	{
		return;
	}

	if (InterlockedExchange(&session->_sendStatus, 1) == 1)
	{
		return;
	}

	InterlockedIncrement(&session->_ioCount);
	ZeroMemory(&session->_sendOvl._ovl, sizeof(WSAOVERLAPPED));

	int bufferSize = session->_sendBuffer.Size();

	WSABUF wsabuf[2];

	wsabuf[0].buf = session->_sendBuffer.GetFrontBufferPtr();
	wsabuf[0].len = session->_sendBuffer.DirectDequeueSize();
	wsabuf[1].buf = session->_sendBuffer.GetBufferPtr();
	wsabuf[1].len = bufferSize - wsabuf[0].len;


	int sendRet = WSASend(session->_clientSocket, wsabuf, 2, NULL, 0, (LPWSAOVERLAPPED)&session->_sendOvl, NULL);

	if (sendRet == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();

		if (errorCode != ERROR_IO_PENDING)
		{
			if (errorCode != WSAECONNRESET || errorCode != WSAECONNABORTED)
			{
				wprintf(L"# WSASend Error in Session %d\n", session->_sessionId);
			}
		}
	}
}

