
#include "Session.h"

Session::Session()
	: _recvBuffer(4096)
	, _sendBuffer(4096)
{
	ZeroMemory(&_clientAddr, sizeof(_clientAddr));

	ZeroMemory(&_recvOvl._ovl, sizeof(WSAOVERLAPPED));
	_recvOvl._type = eOverlappedType::RECV;

	ZeroMemory(&_sendOvl._ovl, sizeof(WSAOVERLAPPED));
	_sendOvl._type = eOverlappedType::SEND;

	ZeroMemory(&_releaseOvl._ovl, sizeof(WSAOVERLAPPED));
	_releaseOvl._type = eOverlappedType::RELEASE;

	ZeroMemory(&_sendPostOvl._ovl, sizeof(WSAOVERLAPPED));
	_sendPostOvl._type = eOverlappedType::SENDPOST;

	InitializeSRWLock(&_lock);
}

void Session::Initialize(const SessionID id, const SOCKET socket, const SOCKADDR_IN addr)
{
	_sessionId = id;
	_clientSocket = socket;
	_clientAddr = addr;
}