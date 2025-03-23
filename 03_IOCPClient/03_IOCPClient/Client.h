#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#pragma comment(lib, "ws2_32.lib")

#include "ClientSession.h"
#include "SerializationBuffer.h"
#include "Protocol.h"

#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <process.h>

class IClient
{
public:
	IClient();
	virtual	~IClient() {}

protected:
	bool NetworkInitialize(const wchar_t* IP, const short port, const int numOfWorkerThread, const int numOfConcurrentWorkerThread, const bool nagle, const bool zeroCopy);
	void NetworkTerminate(void);

public:
	bool Disconnect(void);
	bool SendPacket(SPacket* packet);

private:
	static unsigned int WINAPI NetworkThread(void* arg);

	void HandleRecv(int recvByte);
	void HandleSend(int sendByte);
	void HandleRelease(void);

	void RecvPost(void);
	void SendPost(void);

protected:
	virtual void OnInitialize(void) = 0;
	virtual void OnConnect(void) = 0;
	virtual void OnDisconnect(void) = 0;
	virtual void OnRecv(SPacket* packet) = 0;

public:
	inline bool IsActive(void)
	{
		return _isActive;
	}
	inline bool IsInitialize(void)
	{
		return _isInitialize;
	}

private:
	wchar_t _IP[16] = { 0, };
	short port = 0;
	int _numOfWorkerThread = 0;

	ClientSession* _clientSession = NULL;

	HANDLE _networkIOCP = NULL;
	HANDLE* _networkThreads = { 0, };

private:
	bool _isActive = true;
	bool _isInitialize = false;

	volatile long _recvCnt = 0;
	volatile long _sendCnt = 0;

	long _recvTPS = 0;
	long _sendTPS = 0;

	long long _recvTotal = 0;
	long long _sendTotal = 0;
};