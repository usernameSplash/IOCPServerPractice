#pragma once

#include "Server.h"
#include "ContentsProtocol.h"
#include "TLSPool.h"

#include "Player.h"
#include "Region.h"

#include <unordered_map>
#include <synchapi.h>
#pragma comment(lib, "Synchronization.lib")

using namespace std;

class ChatServer : public IServer
{
public:
	ChatServer()
	{
		
	};

	~ChatServer()
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
	//static unsigned int WINAPI UpdateThread(void* arg);
	static unsigned int WINAPI TimeoutThread(void* arg);

private:
	void SendUnicast(SessionID sessionId, SPacket* packet);
	void SendMulticastAroundRegion(Region region, SPacket* packet);

private:
	void HANDLE_REQ_Login(Player* player, SPacket* packet);
	void HANDLE_REQ_RegionMove(Player* player, SPacket* packet);
	void HANDLE_REQ_Message(Player* player, SPacket* packet);
	void HANDLE_REQ_Heartbeat(Player* player);

	void UNMARSHAL_REQ_Login(SPacket* packet, __int64& accountNum, ID& id, Nickname& nickname, SessionKey& sessionKey);
	void UNMARSHAL_REQ_RegionMove(SPacket* packet, __int64& accountNum, WORD& regionX, WORD& regionY);
	void UNMARSHAL_REQ_Message(SPacket* packet, __int64& accountNum, WORD& msgLen, WCHAR message[MSG_LEN]);
	
	void MARSHAL_RES_LOGIN(SPacket* packet, BYTE status, __int64 accountNum);
	void MARSHAL_RES_RegionMove(SPacket* packet, __int64 accountNum, WORD regionX, WORD regionY);
	void MARSHAL_RES_Message(SPacket* packet, __int64 accountNum, ID id, Nickname nickname, WORD msgLen, WCHAR message[MSG_LEN]);


public:
	inline bool IsAlive(void) const
	{
		return _isAlive;
	}

private:
	bool _isAlive = true;
	HANDLE _monitorThread = NULL;
	HANDLE _updateThread = NULL;
	HANDLE _timeoutThread = NULL;

private:
	Region _regions[REGION_Y_NUM][REGION_X_NUM];
	unordered_map<SessionID, Player*> _playersMap;
	ObjectPool<Player>* _playerPool;

	SRWLOCK _playerMapLock;
	
	unsigned __int64 _playerIDProvider = 0;
private:
	long _waitSignal = 0;
};