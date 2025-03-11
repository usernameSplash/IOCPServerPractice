#pragma once

#include "Server.h"

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
	virtual void OnError(const int errorCode, wchar_t* errorMsg) override;

public:
	inline bool IsAlive(void) const
	{
		return _isAlive;
	}

private:
	bool _isAlive = true;
};