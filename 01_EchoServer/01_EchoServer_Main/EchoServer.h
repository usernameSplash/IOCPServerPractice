#pragma once

#include "Server.h"
#include "ContentsProtocol.h"

class EchoServer : public IServer
{
public:
	EchoServer() 
	{

	};

	~EchoServer()
	{
	
	}

public:
	bool Initialize(void);
	void Terminate(void);

private:
	virtual void OnInitialize(void) override;
	virtual bool OnConnectionRequest(const wchar_t* ip, const short port) override;
	virtual void OnAccept(const SessionID sessionId) override;
	virtual void OnRelease(const SessionID sessionId) override;
	virtual void OnRecv(const SessionID sessionId, SPacket* packet) override;
	virtual void OnError(const int errorCode, const wchar_t* errorMsg) override;

private:
	static unsigned int WINAPI MonitorThread(void* arg);

public:
	inline bool IsAlive(void) const
	{
		return _isAlive;
	}

private:
	bool _isAlive = true;
	HANDLE _monitorThread = NULL;
};