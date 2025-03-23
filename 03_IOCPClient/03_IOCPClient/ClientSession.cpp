
#include "ClientSession.h"

ClientSession::ClientSession(SOCKET clientSocket)
	: _sock(clientSocket)
	, _recvBuffer(20000)
	, _sendBuffer(20000)
{
	ZeroMemory(&_recvOvl._ovl, sizeof(WSAOVERLAPPED));
	_recvOvl._type = eOverlappedType::RECV;

	ZeroMemory(&_sendOvl._ovl, sizeof(WSAOVERLAPPED));
	_sendOvl._type = eOverlappedType::SEND;

	ZeroMemory(&_releaseOvl._ovl, sizeof(WSAOVERLAPPED));
	_releaseOvl._type = eOverlappedType::RELEASE;

	InitializeSRWLock(&_sendBufferLock);
}