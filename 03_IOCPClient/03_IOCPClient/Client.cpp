#include "Client.h"

#include <cstdio>

IClient::IClient()
{

}

bool IClient::NetworkInitialize(const wchar_t* IP, const short port, const int numOfWorkerThread, const int numOfConcurrentWorkerThread, const bool nagle, const bool zeroCopy)
{
	WSADATA wsa;
	int startRet = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (startRet != 0)
	{
		wprintf(L"# WSAStartup Failed\n");
		return false;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		wprintf(L"# Create Socket Failed\n");
		return false;
	}

	_clientSession = new ClientSession(sock);

	if (nagle)
	{
		linger lingerOption;
		lingerOption.l_onoff = 1;
		lingerOption.l_linger = 0;
		int setSockOptRet = setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&lingerOption, sizeof(lingerOption));
		if (setSockOptRet == SOCKET_ERROR)
		{
			wprintf(L"# Setting Nagle Option Failed\n");
			return false;
		}
	}

	if (zeroCopy)
	{
		int sendBufSize = 0;
		int setSockOptRet = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufSize, sizeof(sendBufSize));
		if (setSockOptRet == SOCKET_ERROR)
		{
			wprintf(L"# Setting Send Buffer Size to Zero Failed\n");
			return false;
		}
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	InetPtonW(AF_INET, IP, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(port);

	int connectRet = connect(_clientSession->_sock, (const sockaddr*)&serverAddr, sizeof(serverAddr));
	if (connectRet == SOCKET_ERROR)
	{
		wprintf(L"# Connect Failed\n");
		return false;
	}

	_networkIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numOfConcurrentWorkerThread);
	if (_networkIOCP == NULL)
	{
		wprintf(L"# Create Network IOCP Failed\n");
		return false;
	}

	_networkThreads = new HANDLE[numOfWorkerThread];
	for (int iCnt = 0; iCnt < numOfWorkerThread; ++iCnt)
	{
		_networkThreads[iCnt] = (HANDLE)_beginthreadex(NULL, 0, NetworkThread, this, 0, NULL);
		if (_networkThreads[iCnt] == NULL)
		{
			wprintf(L"Begin Network Therad Failed\n");
			return false;
		}
	}


	CreateIoCompletionPort((HANDLE)_clientSession->_sock, _networkIOCP, (ULONG_PTR)0, 0);
	RecvPost();

	_isInitialize = true;

	return true;
}

void IClient::NetworkTerminate(void)
{
	_isActive = false;

	if (_clientSession != NULL)
	{
		if (_clientSession->_isActive)
		{
			closesocket(_clientSession->_sock);
			_clientSession->_isActive = false;
		}

		delete _clientSession;
	}

	for (int iCnt = 0; iCnt < _numOfWorkerThread; ++iCnt)
	{
		PostQueuedCompletionStatus(_networkIOCP, 0, 0, 0);
	}

	WaitForMultipleObjects(_numOfWorkerThread, _networkThreads, TRUE, INFINITE);

	CloseHandle(_networkIOCP);

	for (int iCnt = 0; iCnt < _numOfWorkerThread; ++iCnt)
	{
		CloseHandle(_networkThreads[iCnt]);
	}

	delete[] _networkThreads;

	WSACleanup();
}

bool IClient::Disconnect(void)
{
	_clientSession->_isActive = false;

	return true;
}

bool IClient::SendPacket(SPacket* packet)
{
	EchoPacketHeader header;
	header._len = (short)packet->GetPayloadSize();

	packet->SetHeaderData(&header, sizeof(EchoPacketHeader));

	AcquireSRWLockExclusive(&_clientSession->_sendBufferLock);
	_clientSession->_sendBuffer.Enqueue(packet->GetBufferPtr(), sizeof(EchoPacketHeader) + header._len);
	ReleaseSRWLockExclusive(&_clientSession->_sendBufferLock);

	SendPost();

	return true;
}

unsigned int WINAPI IClient::NetworkThread(void* arg)
{
	IClient* instance = (IClient*)arg;
	DWORD threadId = GetCurrentThreadId();

	wprintf(L"# Network Thread Start : %d\n", threadId);

	while (true)
	{
		DWORD transferredByte;
		ULONG_PTR completionKey;
		MyOverlapped* overlapped;

		int gqcsRet = GetQueuedCompletionStatus((HANDLE)instance->_networkIOCP, &transferredByte, &completionKey, (LPOVERLAPPED*)&overlapped, INFINITE);

		if (instance->_isActive == false)
		{
			break;
		}

		if (overlapped->_type == eOverlappedType::RELEASE)
		{
			instance->HandleRelease();
			break;
		}

		if (gqcsRet == 0 || transferredByte == 0)
		{
			if (gqcsRet == 0)
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
		}
		else
		{
			switch (overlapped->_type)
			{
			case eOverlappedType::RECV:
				{
					instance->HandleRecv(transferredByte);
					break;
				}
			case eOverlappedType::SEND:
				{
					instance->HandleSend(transferredByte);
					break;
				}
			}
		}
	}

	return 0;
}

void IClient::HandleRecv(int recvByte)
{
	_clientSession->_recvBuffer.MoveRear(recvByte);

	int bufferSize = recvByte;
	int cnt = 0;

	while (bufferSize > 0)
	{
		EchoPacketHeader header;

		if (bufferSize <= sizeof(header))
		{
			break;
		}

		_clientSession->_recvBuffer.Peek((char*)&header, sizeof(header));

		if (bufferSize < sizeof(header) + header._len)
		{
			break;
		}

		_clientSession->_recvBuffer.Dequeue(sizeof(header));

		SPacket packet;
		_clientSession->_recvBuffer.Peek(packet.GetPayloadPtr(), header._len);
		_clientSession->_recvBuffer.Dequeue(header._len);

		packet.MoveWritePos(header._len);

		bufferSize = bufferSize - (sizeof(header) + header._len);

		cnt++;

		OnRecv(&packet);
	}

	_clientSession->_lastRecvTime = timeGetTime();
	_recvCnt += cnt;

	RecvPost();
}

void IClient::HandleSend(int sendByte)
{
	_sendCnt += sendByte / 10;

	_clientSession->_sendBuffer.MoveFront(sendByte);
	_clientSession->_lastSendTime = timeGetTime();

	InterlockedExchange(&_clientSession->_sendStatus, 0);
	SendPost();
}

void IClient::HandleRelease(void)
{
	_clientSession->_isActive = false;
	closesocket(_clientSession->_sock);
	
	delete _clientSession;
	_clientSession = NULL;

	//OnDisconnect();
}

void IClient::RecvPost(void)
{
	if (_clientSession->_isActive == false)
	{
		return;
	}

	DWORD flag = 0;

	InterlockedIncrement(&_clientSession->_ioCount);
	ZeroMemory(&_clientSession->_recvOvl._ovl, sizeof(WSAOVERLAPPED));

	int freeSize = (int)_clientSession->_sendBuffer.Capacity() - (int)_clientSession->_recvBuffer.Size();

	WSABUF wsabuf[2];

	wsabuf[0].buf = _clientSession->_recvBuffer.GetRearBufferPtr();
	wsabuf[0].len = (ULONG)_clientSession->_recvBuffer.DirectEnqueueSize();
	wsabuf[1].buf = _clientSession->_recvBuffer.GetBufferPtr();
	wsabuf[1].len = freeSize - wsabuf[0].len;

	int recvRet = WSARecv(_clientSession->_sock, wsabuf, 2, NULL, &flag, (LPOVERLAPPED)&_clientSession->_recvOvl, NULL);

	if (recvRet == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();

		if (errorCode != ERROR_IO_PENDING)
		{
			if (errorCode != WSAECONNRESET && errorCode != WSAECONNABORTED && errorCode != ERROR_NETNAME_DELETED)
			{
				wprintf(L"(Error) WSARecv Error, errorCode : %d\n", errorCode);
			}
			if (InterlockedDecrement(&_clientSession->_ioCount) == 0)
			{
				PostQueuedCompletionStatus(_networkIOCP, 1, 0, (LPOVERLAPPED)&_clientSession->_releaseOvl);
			}
		}
	}

	return;
}

void IClient::SendPost(void)
{
	if (_clientSession->_isActive == false)
	{
		return;
	}

	if (_clientSession->_sendBuffer.Size() == 0)
	{
		return;
	}

	if (InterlockedExchange(&_clientSession->_sendStatus, 1) == 1)
	{
		return;
	}

	if (_clientSession->_sendBuffer.Size() == 0)
	{
		InterlockedExchange(&_clientSession->_sendStatus, 0);
		return;
	}

	AcquireSRWLockExclusive(&_clientSession->_sendBufferLock);
	int bufferSize = (int)_clientSession->_sendBuffer.Size();

	WSABUF wsabuf[2];
	wsabuf[0].buf = _clientSession->_sendBuffer.GetFrontBufferPtr();
	wsabuf[0].len = (ULONG)_clientSession->_sendBuffer.DirectDequeueSize();
	wsabuf[1].buf = _clientSession->_sendBuffer.GetBufferPtr();
	wsabuf[1].len = bufferSize - wsabuf[0].len;
	ReleaseSRWLockExclusive(&_clientSession->_sendBufferLock);

	int sendRet = WSASend(_clientSession->_sock, wsabuf, 2, NULL, 0, (LPOVERLAPPED)&_clientSession->_sendOvl, NULL);

	if (sendRet == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();

		if (errorCode != ERROR_IO_PENDING)
		{
			if (errorCode != WSAECONNRESET && errorCode != WSAECONNABORTED && errorCode != ERROR_NETNAME_DELETED)
			{
				wprintf(L"(Error) WSASend Error, errorCode : %d\n", errorCode);
			}

			InterlockedExchange(&_clientSession->_sendStatus, 0);

			if (InterlockedDecrement(&_clientSession->_ioCount) == 0)
			{
				PostQueuedCompletionStatus(_networkIOCP, 1, 0, (LPOVERLAPPED)&_clientSession->_releaseOvl);
			}
		}
	}

	return;
}
