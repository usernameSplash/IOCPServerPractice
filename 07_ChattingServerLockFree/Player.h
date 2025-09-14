#pragma once

#include "LockFreeQueue.h"
#include "TLSPool.h"
#include "ContentsProtocol.h"
#include "Protocol.h"

class Player
{
public:
	inline void Initialize(SessionID sessionId, unsigned __int64 playerId)
	{
		_sessionId = sessionId;
		_playerId = playerId;

		_accountNumber = MAXULONGLONG;

		_id[0] = L'\0';
		_nickname[0] = L'\0';
		_sessionKey[0] = '\0';

		_regionX = -1;
		_regionY = -1;
		_lastRecvTime = timeGetTime();
	}

public:
	SessionID _sessionId;
	unsigned __int64 _playerId;
	__int64 _accountNumber;

	ID _id;
	Nickname _nickname;
	SessionKey _sessionKey;

	WORD _regionX;
	WORD _regionY;
	DWORD _lastRecvTime;
};