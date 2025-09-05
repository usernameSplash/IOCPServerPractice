
#include "Server.h"

#include <cstdio>

IServer::IServer()
{
	// To do...
}

bool IServer::Initialize(const wchar_t* IP, const short port, const int numOfWorkerThread, const int numOfConcurrentWorkerThread, const bool nagle, const bool zeroCopy, const int numSessionMax)
{
	wcsncpy(_IP, IP, 16);
	_port = port;
	_numOfWorkerThread = numOfWorkerThread;
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
	_networkThreads = new HANDLE[_numOfWorkerThread];
	for (int iCnt = 0; iCnt < _numOfWorkerThread; ++iCnt)
	{
		_networkThreads[iCnt] = (HANDLE)_beginthreadex(NULL, 0, NetworkThread, this, 0, NULL);
		if (_networkThreads[iCnt] == NULL)
		{
			wprintf(L"# Begin Network Thread Failed\n");
			return false;
		}
	}

	for (int iCnt = 0; iCnt < numSessionMax; ++iCnt)
	{
		_sessionIndexStack.push(iCnt);
		_sessionArray[iCnt] = new Session;
	}

	InitializeSRWLock(&_sessionIndexStackLock);

	//_sessionMap.reserve(numSessionMax);
	//InitializeSRWLock(&_sessionMapLock);

	_isInitialized = true;

	OnInitialize();

	return true;
}

void IServer::Terminate(void)
{
	_isActive = false;

	closesocket(_listenSocket);

	//std::unordered_map<SessionID, Session*>::iterator iter = _sessionMap.begin();

	//for (; iter != _sessionMap.end();)
	//{
	//	Session* session = iter->second;
	//	_sessionMap.erase(iter);
	//	closesocket(session->_clientSocket);
	//	delete session;
	//}

	for (int iCnt = 0; iCnt < _numSessionMax; ++iCnt)
	{
		closesocket(_sessionArray[iCnt]->_clientSocket);
		delete _sessionArray[iCnt];
	}

	for (int iCnt = 0; iCnt < _numOfWorkerThread; ++iCnt)
	{
		PostQueuedCompletionStatus(_networkIOCP, 0, 0, 0);
	}

	WaitForSingleObject(_acceptThread, INFINITE);
	WaitForMultipleObjects(_numOfWorkerThread, _networkThreads, TRUE, INFINITE);

	CloseHandle(_networkIOCP);
	CloseHandle(_acceptThread);

	for (int iCnt = 0; iCnt < _numOfWorkerThread; ++iCnt)
	{
		CloseHandle(_networkThreads[iCnt]);
	}

	delete[] _networkThreads;

	WSACleanup();
}

bool IServer::DisconnectSession(const SessionID sessionId)
{
	unsigned short idx = Session::GetIndexNumFromId(sessionId);
	Session* session = _sessionArray[idx];

	if (session->_isActive == false)
	{
		return false; // already disconnected
	}

	session->_isActive = false;
	// Todo..

	return true;
}

bool IServer::SendPacket(const SessionID sessionId, SPacket* packet)
{
	unsigned short idx = Session::GetIndexNumFromId(sessionId);
	Session* session = _sessionArray[idx];

	EchoPacketHeader header;
	header._len = (short)packet->GetPayloadSize();

	packet->SetHeaderData(&header, sizeof(header));

	AcquireSRWLockExclusive(&session->_sendBufferLock);
	session->_sendBuffer.Enqueue(packet->GetBufferPtr(), sizeof(header) + header._len);
	ReleaseSRWLockExclusive(&session->_sendBufferLock);

	SendPost(session);

	return true;
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

		if (clientSocket == INVALID_SOCKET)
		{
			if (instance->_isActive == false)
			{
				break;
			}

			int errorCode = WSAGetLastError();
			wprintf(L"# Accept Failed : %d\n", errorCode);
			//__debugbreak();
			instance->_isActive = false;
			break;
		}

		if (instance->_isActive == false)
		{
			break;
		}

	
		if (instance->_sessionCnt >= instance->_numSessionMax)
		{
			closesocket(clientSocket);
			//InterlockedIncrement(&instance->_disconnectCnt);
			continue;
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

		AcquireSRWLockExclusive(&instance->_sessionIndexStackLock);
		unsigned short newIndex = instance->_sessionIndexStack.top();
		instance->_sessionIndexStack.pop();
		ReleaseSRWLockExclusive(&instance->_sessionIndexStackLock);

		Session* newSession = instance->_sessionArray[newIndex];
		newSession->Initialize(clientId, newIndex, clientSocket, clientAddr);

		//AcquireSRWLockExclusive(&instance->_sessionMapLock);
		//instance->_sessionMap.insert(std::pair<SessionID, Session*>(clientId, newSession));
		//ReleaseSRWLockExclusive(&instance->_sessionMapLock);

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

		int gqcsRet = GetQueuedCompletionStatus((HANDLE)instance->_networkIOCP, &transferredByte, (PULONG_PTR)&completionSession, (LPOVERLAPPED*)&overlapped, INFINITE);

		if (instance->_isActive == false)
		{
			break;
		}

		if (overlapped->_type == eOverlappedType::RELEASE)
		{
			instance->HandleRelease(completionSession);
			continue;
		}

		if (gqcsRet == 0 || transferredByte == 0)
		{
			if (gqcsRet == 0)
			{
				DWORD errorCode = WSAGetLastError();

				if (errorCode != WSAECONNABORTED && errorCode != WSAECONNRESET && errorCode != ERROR_NETNAME_DELETED)
				{
					wprintf(L"# (Error) GQCS Returned Zero : %d\n", errorCode);
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
			PostQueuedCompletionStatus(instance->_networkIOCP, 1, (ULONG_PTR)completionSession, (LPOVERLAPPED)&completionSession->_releaseOvl);
		}
	}

	wprintf(L"# Network Thread End : %d\n", threadId);

	return 0;
}

// recv data to messages & call OnRecv
void IServer::HandleRecv(Session* session, int recvByte)
{
	session->_recvBuffer.MoveRear(recvByte);

	int bufferSize = session->_recvBuffer.Size();
	int cnt = 0;

	while (bufferSize > 0)
	{
		if (bufferSize <= sizeof(EchoPacketHeader))
		{
			break;
		}

		EchoPacketHeader header;
		session->_recvBuffer.Peek((char*)&header, sizeof(header));

		if (bufferSize < (sizeof(header) + header._len))
		{
			break;
		}

		session->_recvBuffer.Dequeue(sizeof(header));

		SPacket packet;
		session->_recvBuffer.Peek((char*)packet.GetPayloadPtr(), header._len);
		session->_recvBuffer.Dequeue(header._len);

		packet.MoveWritePos(header._len);

		bufferSize = bufferSize - (sizeof(header) + header._len);

		cnt++;

		OnRecv(session->_sessionId, &packet);
	}

	session->_lastRecvTime = timeGetTime();

	session->_recvCnt += cnt;

	InterlockedAdd(&_recvCnt, cnt);
	RecvPost(session);
}

void IServer::HandleSend(Session* session, int sendByte)
{
	//InterlockedIncrement(&_sendCnt);
	session->_sendCnt += sendByte / 10;
	InterlockedAdd(&_sendCnt, sendByte / 10); // size of each messages is always 10 in this echo server;

	AcquireSRWLockExclusive(&session->_sendBufferLock);
	session->_sendBuffer.MoveFront(sendByte);
	ReleaseSRWLockExclusive(&session->_sendBufferLock);

	session->_lastSendTime = timeGetTime();

	//long prevSendFlag = InterlockedDecrement(&session->_sendStatus);
	//
	//if (prevSendFlag == 0)
	//{
	//	SendPost(session);
	//}

	InterlockedExchange(&session->_sendStatus, 0);
	SendPost(session);
}

void IServer::HandleRelease(Session* session)
{
	SessionID id = session->_sessionId;
	short idx = Session::GetIndexNumFromId(id);

	session->_isActive = false;
	closesocket(session->_clientSocket);

	AcquireSRWLockExclusive(&_sessionIndexStackLock);
	_sessionIndexStack.push(idx);
	ReleaseSRWLockExclusive(&_sessionIndexStackLock);

	InterlockedDecrement(&_sessionCnt);
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

	int freeSize = (int)(session->_recvBuffer.Capacity() - session->_recvBuffer.Size());

	WSABUF wsabuf[2];

	wsabuf[0].buf = session->_recvBuffer.GetRearBufferPtr();
	wsabuf[0].len = (ULONG)session->_recvBuffer.DirectEnqueueSize();
	wsabuf[1].buf = session->_recvBuffer.GetBufferPtr();
	wsabuf[1].len = freeSize - wsabuf[0].len;


	int recvRet = WSARecv(session->_clientSocket, wsabuf, 2, NULL, &flag, (LPWSAOVERLAPPED)&session->_recvOvl, NULL);

	if (recvRet == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();

		if (errorCode != ERROR_IO_PENDING)
		{
			if (errorCode != WSAECONNRESET && errorCode != WSAECONNABORTED && errorCode != ERROR_NETNAME_DELETED)
			{
				wprintf(L"(Error) WSARecv Error, sessionId : %llu, errorCode : %d\n", Session::GetIdNumFromId(session->_sessionId), errorCode);
			}
			if (InterlockedDecrement(&session->_ioCount) == 0)
			{
				PostQueuedCompletionStatus(_networkIOCP, 1, (ULONG_PTR)session, (LPOVERLAPPED)&session->_releaseOvl);
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

	if (session->_sendBuffer.Size() == 0)
	{
		return;
	}

	if (InterlockedExchange(&session->_sendStatus, 1) == 1)
	{
		return;
	}

	if (session->_sendBuffer.Size() == 0)
	{
		InterlockedExchange(&session->_sendStatus, 0);
		return;
	}

	InterlockedIncrement(&session->_ioCount);
	ZeroMemory(&session->_sendOvl._ovl, sizeof(WSAOVERLAPPED));

	WSABUF wsabuf[2];

	AcquireSRWLockShared(&session->_sendBufferLock);
	int bufferSize = (int)(session->_sendBuffer.Size());

	wsabuf[0].buf = session->_sendBuffer.GetFrontBufferPtr();
	wsabuf[0].len = (ULONG)session->_sendBuffer.DirectDequeueSize();
	wsabuf[1].buf = session->_sendBuffer.GetBufferPtr();
	wsabuf[1].len = bufferSize - wsabuf[0].len;
	ReleaseSRWLockShared(&session->_sendBufferLock);

	int sendRet = WSASend(session->_clientSocket, wsabuf, 2, NULL, 0, (LPWSAOVERLAPPED)&session->_sendOvl, NULL);

	if (sendRet == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();

		if (errorCode != ERROR_IO_PENDING)
		{
			if (errorCode != WSAECONNRESET && errorCode != WSAECONNABORTED && errorCode != ERROR_NETNAME_DELETED)
			{
				wprintf(L"(Error) WSASend Error, sessionId : %llu, errorCode : %d\n", Session::GetIdNumFromId(session->_sessionId), errorCode);
			}
			
			InterlockedExchange(&session->_sendStatus, 0);

			if (InterlockedDecrement(&session->_ioCount) == 0)
			{
				PostQueuedCompletionStatus(_networkIOCP, 1, (ULONG_PTR)session, (LPOVERLAPPED)&session->_releaseOvl);
			}
		}
	}
}

void IServer::UpdateMonitoringData(void)
{
	_acceptTPS = _acceptCnt;
	_disconnectTPS = _disconnectCnt;
	_recvTPS = _recvCnt;
	_sendTPS = _sendCnt;

	_acceptTotal += _acceptCnt;
	_disconnectTotal += _disconnectCnt;
	_recvTotal = _recvTotal + (long long)_recvCnt;
	_sendTotal = _sendTotal + (long long)_sendCnt;

	_acceptCnt = 0;
	_disconnectCnt = 0;
	_recvCnt = 0;
	_sendCnt = 0;

	return;
}