
#include "Session.h"

Session::Session()
	: _recvBuffer(20000)
	, _sendBuffer(20000)
{
	ZeroMemory(&_clientAddr, sizeof(_clientAddr));

	ZeroMemory(&_recvOvl._ovl, sizeof(WSAOVERLAPPED));
	_recvOvl._type = eOverlappedType::RECV;

	ZeroMemory(&_sendOvl._ovl, sizeof(WSAOVERLAPPED));
	_sendOvl._type = eOverlappedType::SEND;

	ZeroMemory(&_releaseOvl._ovl, sizeof(WSAOVERLAPPED));
	_releaseOvl._type = eOverlappedType::RELEASE;

	InitializeSRWLock(&_sendBufferLock);
}

Session::~Session(void)
{

}

void Session::Initialize(const unsigned __int64 idNum, const unsigned short index, const SOCKET socket, const SOCKADDR_IN addr)
{
	unsigned __int64 idx = (unsigned __int64)index;
	unsigned __int64 id = idNum;

	idx <<= 48;
	id &= s_idMask;

	_isActive = true;

	_sessionId = (idx | id);
	_clientSocket = socket;
	_clientAddr = addr;

	_ioCount = 0;
	_sendStatus = 0;

	_lastRecvTime = 0;
	_lastSendTime = 0;

	_recvCnt = 0;
	_sendCnt = 0;
}

void Session::Terminate(void)
{
	_isActive = false;
	closesocket(_clientSocket);

	_recvBuffer.ClearBuffer();
	_sendBuffer.ClearBuffer();
}

unsigned __int64 Session::GetIdNumFromId(const SessionID sessionId)
{
	return (sessionId & s_idMask);
}

short Session::GetIndexNumFromId(const SessionID sessionId)
{
	return (short)((sessionId & s_idxMask) >> 48);
}