#pragma once

#include "Client.h"

class EchoClient : public IClient
{
public:
	EchoClient() {}
	virtual ~EchoClient() {}

public:
	bool ClientInitialize(void);
	void ClientTerminate(void);

private:
	static unsigned int WINAPI ContentsThread(void* arg);

protected:
	virtual void OnInitialize(void) override;
	virtual void OnConnect(void) override;
	virtual void OnDisconnect(void) override;
	virtual void OnRecv(SPacket* packet) override;

public:
	inline bool IsAlive(void) const
	{
		return _isAlive;
	}

private:
	bool _isAlive = true;
	HANDLE _contentsThread = NULL;

	HANDLE _echoEvent = NULL;
};