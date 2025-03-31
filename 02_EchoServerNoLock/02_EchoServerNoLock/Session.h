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
	void Initialize(const unsigned __int64 idNum, const unsigned short index,  const SOCKET socket, const SOCKADDR_IN addr);

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
	SRWLOCK _sendBufferLock;

	//SPacket _recvPacket; // member recv buffer for temporary use in this test

	volatile long _ioCount = 0;
	volatile long _sendStatus = 0;

	DWORD _lastRecvTime = 0;
	DWORD _lastSendTime = 0;

	__int64 _recvCnt = 0;
	__int64 _sendCnt = 0;

private:
	const static unsigned __int64 s_idxMask = (unsigned __int64)0xffff << 48;
	const static unsigned __int64 s_idMask = ~s_idxMask;

	static unsigned __int64 GetIdNumFromId(const SessionID sessionId);
	static short GetIndexNumFromId(const SessionID sessionId);

	friend class IServer;
};