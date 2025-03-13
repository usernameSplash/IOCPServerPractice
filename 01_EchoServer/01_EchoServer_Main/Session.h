#pragma once

#include "Protocol.h"
#include "RingBuffer.h"
#include "SerializationBuffer.h"

#include <WinSock2.h>
#include <Windows.h>

using SessionID = unsigned __int64;

enum class eOverlappedType
{
	RECV = 0,
	SEND = 1,
	RELEASE = 2,
};

class MyOverlapped
{
public:
	WSAOVERLAPPED _ovl;
	eOverlappedType _type;
};

class Session
{

public:
	Session(void);
	~Session(void);

public:
	void Initialize(const SessionID id, const SOCKET socket, const SOCKADDR_IN addr);

private:
	bool _isActive = false;

	SessionID _sessionId = ULLONG_MAX;
	SOCKET _clientSocket = INVALID_SOCKET;
	SOCKADDR_IN _clientAddr;

	MyOverlapped _recvOvl;
	MyOverlapped _sendOvl;
	MyOverlapped _releaseOvl;

	MyDataStructure::RingBuffer _recvBuffer;
	MyDataStructure::RingBuffer _sendBuffer;

	volatile long _ioCount = 0;
	volatile long _sendStatus = 0;

	//SRWLOCK _lock;
	CRITICAL_SECTION _lock;

	friend class IServer;
};