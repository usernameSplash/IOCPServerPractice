#pragma once
#pragma comment(lib, "ws2_32.lib")

#include "Session.h"
#include "LockFreeStack.h"

#include <cstdio>
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <WS2tcpip.h>

class SPacket;

class IServer
{
public:
	IServer();
	virtual ~IServer() {};

protected:
	bool Initialize(const wchar_t* IP, const short port, const int numOfWorkerThread, const int numOfConcurrentWorkerThread, const bool nagle, const bool zeroCopy, const int numSessionMax);
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
	virtual void OnError(const int errorCode, const wchar_t* errorMsg) = 0;

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
	void UpdateMonitoringData(void);

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

	inline long long GetRecvTotal(void) const
	{
		return _recvTotal;
	}

	inline long long GetSendTotal(void) const
	{
		return _sendTotal;
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

	inline void CheckSessionResponseTime(void) const
	{
		if (_isInitialized == false)
		{
			return;
		}

		for (int iCnt = 0; iCnt < _numSessionMax; ++iCnt)
		{
			Session* session = _sessionArray[iCnt];

			if (session->_isActive)
			{
				DWORD sendTime = session->_lastSendTime;
				DWORD recvTime = session->_lastRecvTime;

				if (sendTime > recvTime)
				{
					if ((sendTime - recvTime) > 500)
					{
						wprintf(L"# The Response of session %lld is too late : %ums\n", Session::GetIdNumFromId(session->_sessionId), sendTime - recvTime);
					}
				}
			}
		}
	}

protected:
	inline bool IsActive(void) const
	{
		return _isActive;
	}

// Variables
protected:
	wchar_t _IP[16] = { 0, };
	short _port = 0;

private:
	int _numOfWorkerThread = 0;
	int _numSessionMax = SESSION_MAX;

	bool _nagle = false;
	bool _zeroCopy = false;

	SOCKET _listenSocket = INVALID_SOCKET;
	HANDLE _networkIOCP = NULL;
	HANDLE _acceptThread = NULL;
	HANDLE* _networkThreads = { 0, };

	LockFreeStack<unsigned short> _sessionIndexStack;

	Session* _sessionArray[SESSION_MAX] = { 0, };

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
	long long _recvTotal = 0;
	long long _sendTotal = 0;

	bool _isActive = true;
	bool _isInitialized = false;
};