
#include "ChatServer.h"

bool ChatServer::Initialize(void)
{
	InitializeSRWLock(&_playerMapLock);

	_playerPool = new ObjectPool<Player>(PLAYER_MAX / CHUNK_SIZE, false);

	for (int y = 0; y < REGION_Y_NUM; ++y)
	{
		for (int x = 0; x < REGION_X_NUM; ++x)
		{
			_regions[y][x]._y = y;
			_regions[y][x]._x = x;

			if (y > 0)
			{
				_regions[y - 1][x]._aroundRegions.push_back(&_regions[y][x]);

				if (x > 0)
				{
					_regions[y - 1][x - 1]._aroundRegions.push_back(&_regions[y][x]);
				}

				if (x < REGION_X_NUM - 1)
				{
					_regions[y - 1][x + 1]._aroundRegions.push_back(&_regions[y][x]);
				}
			}

			if (y < REGION_Y_NUM - 1)
			{
				_regions[y + 1][x]._aroundRegions.push_back(&_regions[y][x]);

				if (x > 0)
				{
					_regions[y + 1][x - 1]._aroundRegions.push_back(&_regions[y][x]);
				}

				if (x < REGION_X_NUM - 1)
				{
					_regions[y + 1][x + 1]._aroundRegions.push_back(&_regions[y][x]);
				}
			}

			if (x > 0)
			{
				_regions[y][x - 1]._aroundRegions.push_back(&_regions[y][x]);
			}

			if (x < REGION_X_NUM - 1)
			{
				_regions[y][x + 1]._aroundRegions.push_back(&_regions[y][x]);
			}
		}
	}

	_monitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, NULL);
	if (_monitorThread == NULL)
	{
		wprintf(L"# Begin Monitor Thread Failed\n");
		return false;
	}

	//_updateThread = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, this, 0, nullptr);
	//if (_updateThread == NULL)
	//{
	//	wprintf(L"# Create Update Thread is Failed\n");
	//	return false;
	//}

	//_timeoutThread = (HANDLE)_beginthreadex(NULL, 0, TimeoutThread, this, 0, nullptr);
	//if (_timeoutThread == NULL)
	//{
	//	wprintf(L"# Create Timeout Thread is Failed\n");
	//	return false;
	//}

	int threadNum = 1;
	int concurrentThreadNum = 1;

	if (IServer::Initialize(SERVER_ADDRESS, SERVER_PORT, threadNum, concurrentThreadNum, true, true, SESSION_MAX) == false)
	{
		Terminate();
		return false;
	}

	wprintf(L"# Chat Server Start\n");

	return true;
}

void ChatServer::Terminate()
{
	IServer::Terminate();

	_isAlive = false;

	if (_monitorThread != NULL)
	{
		WaitForSingleObject(_monitorThread, INFINITE);
		CloseHandle(_monitorThread);
		_monitorThread = NULL;
	}

	//if (_updateThread != NULL) { ... }

	if (_timeoutThread != NULL)
	{
		WaitForSingleObject(_timeoutThread, INFINITE);
		CloseHandle(_timeoutThread);
		_timeoutThread = NULL;
	}

	wprintf(L"# Chat Server Terminate\n");

	return;
}

void ChatServer::OnInitialize(void)
{
	wprintf(L"# OnInitialize - Network Library Start\n");
	return;
}

bool ChatServer::OnConnectionRequest(const wchar_t* ip, const short port)
{
	return true;
}

void ChatServer::OnAccept(const SessionID sessionId)
{
	AcquireSRWLockExclusive(&_playerMapLock);
	if (_playersMap.size() >= PLAYER_MAX)
	{
		DisconnectSession(sessionId);
		ReleaseSRWLockExclusive(&_playerMapLock);
		return;
	}

	Player* player = _playerPool->Alloc();
	player->Initialize(sessionId, _playerIDProvider++);

	bool insertResult = _playersMap.insert({ sessionId, player }).second;
	if (insertResult == false)
	{
		wprintf(L"# Player Insert is Failed : SessionID is Already Exist(%llu)\n", sessionId);
	}

	ReleaseSRWLockExclusive(&_playerMapLock);

	return;
}

void ChatServer::OnRelease(const SessionID sessionId)
{
	AcquireSRWLockExclusive(&_playerMapLock);
	unordered_map<unsigned __int64, Player*>::iterator iter = _playersMap.find(sessionId);

	if (iter == _playersMap.end())
	{
		wprintf(L"# Release Player Failed : No Session Id(%llu)\n", sessionId);
		ReleaseSRWLockExclusive(&_playerMapLock);
		return;
	}

	Player* player = iter->second;
	_playersMap.erase(iter);
	ReleaseSRWLockExclusive(&_playerMapLock);

	if (player->_regionY < REGION_Y_NUM && player->_regionX < REGION_X_NUM)
	{
		Region* region = &_regions[player->_regionY][player->_regionX];

		AcquireSRWLockExclusive(&region->_lock);
		for (auto playerIter = region->_players.begin(); playerIter != region->_players.end(); ++playerIter)
		{
			if (player == (*playerIter))
			{
				region->_players.erase(playerIter);
				break;
			}
		}
		ReleaseSRWLockExclusive(&region->_lock);
	}

	_playerPool->Free(player);

	return;
}

void ChatServer::OnRecv(const SessionID sessionId, SPacket* packet)
{
	AcquireSRWLockShared(&_playerMapLock);
	unordered_map<unsigned __int64, Player*>::iterator iter = _playersMap.find(sessionId);

	Player* player;
	if (iter == _playersMap.end())
	{
		player = nullptr;
	}
	else
	{
		player = iter->second;
	}
		ReleaseSRWLockShared(&_playerMapLock);

	if (player == nullptr)
	{
		DisconnectSession(sessionId);
		return;
	}

	player->_lastRecvTime = timeGetTime();

	short type;
	*packet >> type;

	switch (type)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		HANDLE_REQ_Login(player, packet);
		break;

	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		HANDLE_REQ_RegionMove(player, packet);
		break;

	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		HANDLE_REQ_Message(player, packet);
		break;

	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		HANDLE_REQ_Heartbeat(player);
		break;

	default:
		wprintf(L"# Wrong Message Type : %d\n", type);
		break;
	}

	return;
}

void ChatServer::OnError(const int errorCode, const wchar_t* errorMsg)
{
	// Todo: Handle Error defined by User
}

unsigned int WINAPI ChatServer::MonitorThread(void* arg)
{
	ChatServer* instance = (ChatServer*)arg;

	DWORD threadId = GetCurrentThreadId();

	wprintf(L"# Monitor Thread Start : %d\n", threadId);

	while (instance->IsActive())
	{
		Sleep(UPDATE_TIMEOUT);

		instance->UpdateMonitoringData();

		wprintf(L"\nESC : Quit\n");
		wprintf(L"===================================================\n");
		wprintf(L"Server IP : %s, Port : %d\n", instance->_IP, instance->_port);
		wprintf(L"===================================================\n");
		wprintf(L" [Network Statistics]\n");
		wprintf(L"  Session Count : %d\n\n", instance->GetSessionCount());

		wprintf(L"  Accept Total : %d\n", instance->GetAcceptTotal());
		wprintf(L"  Disconnect Total : %d\n\n", instance->GetDisconnectTotal());

		wprintf(L"  Accept TPS : %d\n", instance->GetAcceptTPS());
		wprintf(L"  Disconnect TPS : %d\n\n", instance->GetDisconnectTPS());

		wprintf(L"  Recv Total : %lld messages\n", instance->GetRecvTotal());
		wprintf(L"  Send Total : %lld messages\n\n", instance->GetSendTotal());

		wprintf(L"  Recv TPS : %d messages/s\n", instance->GetRecvTPS());
		wprintf(L"  Send TPS : %d messages/s\n\n", instance->GetSendTPS());
	}

	wprintf(L"# Monitor Thread End : %d\n", threadId);

	return 0;
}

unsigned int WINAPI ChatServer::TimeoutThread(void* arg)
{
	ChatServer* instance = (ChatServer*)arg;

	while (instance->IsActive())
	{
		AcquireSRWLockExclusive(&instance->_playerMapLock);
		for (auto iter = instance->_playersMap.begin(); iter != instance->_playersMap.end();)
		{
			Player* player = iter->second;
			if ((timeGetTime() - player->_lastRecvTime) >= TIMEOUT_FREQ)
			{
				instance->DisconnectSession(player->_sessionId);
				iter = instance->_playersMap.erase(iter);
			}
			else
			{
				++iter;
			}
		}
		ReleaseSRWLockExclusive(&instance->_playerMapLock);

		Sleep(TIMEOUT_FREQ);
	}

	return 0;
}

void ChatServer::SendUnicast(SessionID sessionId, SPacket* packet)
{
	packet->AddUseCount(1);
	if (SendPacket(sessionId, packet) == false)
	{
		SPacket::Free(packet);
}
	SPacket::Free(packet);
}

void ChatServer::SendMulticastAroundRegion(Region* region, SPacket* packet)
{
	vector<SessionID> sendId;

	for (auto iter = region->_aroundRegions.begin(); iter != region->_aroundRegions.end(); ++iter)
	{
		AcquireSRWLockShared(&(*iter)->_lock);
		for (auto playerIter = (*iter)->_players.begin(); playerIter != (*iter)->_players.end(); ++playerIter)
		{
			sendId.push_back((*playerIter)->_sessionId);
		}
		ReleaseSRWLockShared(&(*iter)->_lock);
	}

	AcquireSRWLockShared(&region->_lock);
	for (auto playerIter = region->_players.begin(); playerIter != region->_players.end(); ++playerIter)
	{
		sendId.push_back((*playerIter)->_sessionId);
	}
	ReleaseSRWLockShared(&region->_lock);

	packet->AddUseCount((long)sendId.size());

	for (auto sendIdIter = sendId.begin(); sendIdIter != sendId.end(); ++sendIdIter)
	{
		if (SendPacket(*sendIdIter, packet) == false)
		{
			SPacket::Free(packet);
		}
	}

	SPacket::Free(packet);

	return;
}

void ChatServer::HANDLE_REQ_Login(Player* player, SPacket* packet)
{
	__int64 accountNum;

	UNMARSHAL_REQ_Login(packet, accountNum, player->_id, player->_nickname, player->_sessionKey);

	if (accountNum < 0)
	{
		wprintf(L"# Login is Failed : AccountNum is Wrong(%llu)\n", player->_sessionId);
		DisconnectSession(player->_sessionId);
		return;
	}

	BYTE status = 1;

	player->_accountNumber = accountNum;
	
	SPacket* sendPacket = SPacket::Alloc();
	MARSHAL_RES_LOGIN(sendPacket, status, player->_accountNumber);
	SendUnicast(player->_sessionId, sendPacket);
}

void ChatServer::HANDLE_REQ_RegionMove(Player* player, SPacket* packet)
{
	__int64 accountNum;
	WORD regionX;
	WORD regionY;

	UNMARSHAL_REQ_RegionMove(packet, accountNum, regionX, regionY);

	if (accountNum != player->_accountNumber)
	{
		wprintf(L"# Handle Move is Failed, Account Num is Wrong : %lld, %lld", accountNum, player->_accountNumber);
		DisconnectSession(player->_sessionId);
		return;
	}

	//wprintf(L"# Move (%d, %d) -> (%d, %d)\n", player->_regionY, player->_regionX, regionY, regionX);

	if (regionY >= REGION_Y_NUM || regionX >= REGION_X_NUM)
	{
		DisconnectSession(player->_sessionId);
		return;
	}

	if (player->_regionX < REGION_X_NUM && player->_regionY < REGION_Y_NUM)
	{
		Region* region = &_regions[player->_regionY][player->_regionX];

		AcquireSRWLockExclusive(&region->_lock);

		for (auto playerIter = region->_players.begin(); playerIter != region->_players.end(); ++playerIter)
		{
			if (*playerIter == player)
			{
				region->_players.erase(playerIter);
				break;
			}
		}

		ReleaseSRWLockExclusive(&region->_lock);
	}

	player->_regionX = regionX;
	player->_regionY = regionY;

	AcquireSRWLockExclusive(&_regions[regionY][regionX]._lock);
	_regions[regionY][regionX]._players.push_back(player);
	ReleaseSRWLockExclusive(&_regions[regionY][regionX]._lock);


	SPacket* sendPacket = SPacket::Alloc();

	MARSHAL_RES_RegionMove(sendPacket, player->_accountNumber, player->_regionX, player->_regionY);
	SendUnicast(player->_sessionId, sendPacket);

	return;
}

void ChatServer::HANDLE_REQ_Message(Player* player, SPacket* packet)
{
	__int64 accountNum;
	WORD msgLen;
	WCHAR message[MSG_LEN];

	if (UNMARSHAL_REQ_Message(packet, accountNum, msgLen, message) == false)
	{
		wprintf(L"# Handle Message is Failed, Message Length is Wrong : %u\n", msgLen);
		DisconnectSession(player->_sessionId);
		return;
	}

	if (accountNum != player->_accountNumber)
	{
		wprintf(L"# Handle Message is Failed, Account Num is Wrong : %lld, %lld\n", accountNum, player->_accountNumber);
		DisconnectSession(player->_sessionId);
		return;
	}

	if (player->_regionX >= REGION_X_NUM || player->_regionY >= REGION_Y_NUM)
	{
		wprintf(L"# Handle Message is Failed, Player is Not in Any Region : %llu\n", player->_sessionId);
		DisconnectSession(player->_sessionId);
		return;
	}

	SPacket* sendPacket = SPacket::Alloc();

	MARSHAL_RES_Message(sendPacket, player->_accountNumber, player->_id, player->_nickname, msgLen, message);
	SendMulticastAroundRegion(&_regions[player->_regionY][player->_regionX], sendPacket);

	return;
}

void ChatServer::HANDLE_REQ_Heartbeat(Player* player)
{
	player->_lastRecvTime = timeGetTime();

	return;
}

void ChatServer::UNMARSHAL_REQ_Login(SPacket* packet, __int64& accountNum, ID& id, Nickname& nickname, SessionKey& sessionKey)
{
	(*packet) >> accountNum;
	packet->GetPayloadData((char*)id, sizeof(ID));
	packet->MoveReadPos(sizeof(ID));
	id[ID_LEN - 1] = L'\0';

	packet->GetPayloadData((char*)nickname, sizeof(WCHAR) * NICKNAME_LEN);
	packet->MoveReadPos(sizeof(Nickname));
	nickname[NICKNAME_LEN - 1] = L'\0';

	packet->GetPayloadData((char*)sessionKey, sizeof(char) * SESSION_KEY_LEN);
	packet->MoveReadPos(sizeof(SessionKey));

	return;
}

void ChatServer::UNMARSHAL_REQ_RegionMove(SPacket* packet, __int64& accountNum, WORD& regionX, WORD& regionY)
{
	(*packet) >> accountNum;
	(*packet) >> regionX;
	(*packet) >> regionY;

	return;
}

bool ChatServer::UNMARSHAL_REQ_Message(SPacket* packet, __int64& accountNum, WORD& msgLen, WCHAR message[MSG_LEN])
{
	(*packet) >> accountNum;
	(*packet) >> msgLen;

	if (msgLen > sizeof(WCHAR) * MSG_LEN || msgLen > packet->GetPayloadSize())
	{
		return false;
	}

	packet->GetPayloadData((char*)message, msgLen);
	packet->MoveReadPos(msgLen);

	return true;
}

void ChatServer::MARSHAL_RES_LOGIN(SPacket* packet, BYTE status, __int64 accountNum)
{
	WORD type = en_PACKET_CS_CHAT_RES_LOGIN;
	(*packet) << type;
	(*packet) << status;
	(*packet) << accountNum;
	
	return;
}

void ChatServer::MARSHAL_RES_RegionMove(SPacket* packet, __int64 accountNum, WORD regionX, WORD regionY)
{
	WORD type = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
	(*packet) << type;
	(*packet) << accountNum;
	(*packet) << regionX;
	(*packet) << regionY;

	return;
}

void ChatServer::MARSHAL_RES_Message(SPacket* packet, __int64 accountNum, ID id, Nickname nickname, WORD msgLen, WCHAR message[MSG_LEN])
{
	WORD type = en_PACKET_CS_CHAT_RES_MESSAGE;
	(*packet) << type;
	(*packet) << accountNum;
	
	packet->SetPayloadData(id, sizeof(ID));
	packet->MoveWritePos(sizeof(ID));

	packet->SetPayloadData(nickname, sizeof(Nickname));
	packet->MoveWritePos(sizeof(Nickname));

	(*packet) << msgLen;

	packet->SetPayloadData(message, msgLen);
	packet->MoveWritePos(msgLen);

	return;
}
