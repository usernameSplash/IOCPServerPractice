#pragma once
#pragma comment(lib, "ws2_32.lib")

#include "Session.h"

#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <WS2tcpip.h>
#include <unordered_map>


class SPacket;

class IServer
{
public:
	IServer();
	virtual ~IServer() {};

protected:
	bool Initialize(const wchar_t* IP, short port, int numOfWorkerThread, int numOfConcurrentWorkerThread, bool nagle, bool zeroCopy, int numSessionMax);
	void Terminate(void);

protected:
	// Called by Contents Code (Require Something to Network Library)
	bool DisconnectSession(const SessionID sessionId);
	bool SendPacket(const SessionID sessionId, SPacket* packet);

protected:
	// Called by Network Library Code
	virtual void OnInitialize(void) = 0;
	virtual bool OnConnectionRequest(const wchar_t* ip, const short port) = 0;
	virtual void OnAccept(const SessionID sessionId) = 0;
	virtual void OnRelease(const SessionID sessionId) = 0;
	virtual void OnRecv(const SessionID sessionId, SPacket* packet) = 0;
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
	void HandleRelease(Session* session);

private:
	// Wrapper of WSA IO Function
	void RecvPost(Session* session);
	void SendPost(Session* session);

protected:
	// Functions for Monitoring
	inline long GetSessionCount(void) const
	{
		return _sessionCnt;
	}

	inline long GetAcceptTotal(void) const
	{
		return _acceptTotal;
	}

	inline long GetDisconnectTotal(void) const
	{
		return _disconnectTotal;
	}

	inline long GetAcceptTPS(void) const
	{
		return _acceptTPS;
	}

	inline long GetDisconnectTPS(void) const
	{
		return _disconnectTPS;
	}

	inline long GetRecvTPS(void) const
	{
		return _recvTPS;
	}

	inline long GetSendTPS(void) const
	{
		return _sendTPS;
	}

// Variables
private:
	wchar_t _IP[16] = { 0, };
	short _port = 0;
	int _numOfWorkerThread = 0;
	int _numSessionMax = SESSION_MAX;
	SOCKET _listenSocket = INVALID_SOCKET;
	HANDLE _networkIOCP = NULL;
	HANDLE _acceptThread = NULL;
	HANDLE* _networkThreads = { 0, };

	std::unordered_map<SessionID, Session*> _sessionMap;
	SRWLOCK _sessionMapLock;

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

	bool _isActive = true;
};