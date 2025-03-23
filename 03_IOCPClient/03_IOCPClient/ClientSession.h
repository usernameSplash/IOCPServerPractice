#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include "RingBuffer.h"

#include <WinSock2.h>
#include <Windows.h>

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

class ClientSession
{
public:
	ClientSession(SOCKET clientSocket);
	~ClientSession() {};

public:

private:
	bool _isActive = true;

	SOCKET _sock;

	MyOverlapped _recvOvl;
	MyOverlapped _sendOvl;
	MyOverlapped _releaseOvl;

	MyDataStructure::RingBuffer _recvBuffer;
	MyDataStructure::RingBuffer _sendBuffer;

	SRWLOCK _sendBufferLock;

	volatile long _ioCount = 0;
	volatile long _sendStatus = 0;

	DWORD _lastRecvTime = 0;
	DWORD _lastSendTime = 0;

	friend class IClient;
};