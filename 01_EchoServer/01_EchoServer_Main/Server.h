#pragma once

#include "Session.h"

#include <Windows.h>
#include <process.h>

class SPacket;

class IServer
{
public:
	IServer();
	~IServer() {};

protected:
	bool Initialize(wchar_t* IP, short port, int numOfWorkerThread, int numOfConcurrentWorkerThread, bool nagle, bool zeroCopy, int numSessionMax);

public:
	void Terminate(void);

protected:
	// Called by Contents Code (Require Something to Network Library)
	bool DisconnectSession(const __int64 sessionId);
	bool SendPacket(const __int64 sessionId, SPacket* packet);

protected:
	// Called by Network Library Code
	virtual void OnInitialize(void) = 0;
	virtual bool OnConnectionRequest(const wchar_t* ip, const short port) = 0;
	virtual void OnAccept(const __int64 sessionId) = 0;
	virtual void OnRelease(const __int64 sessionId) = 0;
	virtual void OnRecv(const __int64 sessionId, SPacket* packet) = 0;
	//virtual void OnSend(const __int64 sessionId, const int sendByte) = 0;
	virtual void OnError(const int errorCode, wchar_t* errorMsg) = 0;

private:
	// Thread Procedure
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);
	//static unsigned int WINAPI ReleaseThread(void* arg);

private:
	void HandleRecv(Session* session, int recvByte);
	void HandleSend(Session* session, int sendByte);

private:
	// Wrapper of WSA IO Function
	void RecvPost(Session* session);
	void SendPost(Session* session);

protected:
	// Functions for Monitoring
	inline long GetSessionCount(void)
	{
		return _sessionCnt;
	}

	inline long GetAcceptTotal(void)
	{
		return _acceptTotal;
	}

	inline long GetDisconnectTotal(void)
	{
		return _disconnectTotal;
	}

	inline long GetAcceptTPS(void)
	{
		return _acceptTPS;
	}

	inline long GetDisconnectTPS(void)
	{
		return _disconnectTPS;
	}

	inline long GetRecvTPS(void)
	{
		return _recvTPS;
	}

	inline long GetSendTPS(void)
	{
		return _sendTPS;
	}


// Variables
private:
	wchar_t _IP[16];
	short _port;
	int _numSessionMax;
	SOCKET _listenSocket;
	HANDLE _networkIOCP;
	HANDLE _acceptThread;
	HANDLE* _networkThreads;

private:
	volatile long _acceptCnt = 0;
	volatile long _disconnectCnt = 0;
	volatile long _recvCnt = 0;
	volatile long _sendCnt = 0;
	volatile long _sessionCnt = 0;

	long _acceptTPS = 0;
	long _disconnectTPS = 0;
	long _recvTPS = 0;
	long _sendTPS = 0;

	long _acceptTotal = 0;
	long _disconnectTotal = 0;
};