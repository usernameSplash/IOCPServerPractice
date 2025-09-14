#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#define SERVER_ADDRESS L"0.0.0.0"
#define SERVER_PORT 12001 //11850

#define SESSION_MAX 18000

#define PACKET_CODE 0x77
#define PACKET_KEY 0x32

#pragma pack(push, 1)
typedef struct NetPacketHeader
{
	unsigned char _code = PACKET_CODE;
	unsigned short _len = 0;
	unsigned char _randKey = 0;
	unsigned char _checkSum = 0;
} NetPacketHeader;
#pragma pack(pop)

#define PACKET_HEADER_SIZE sizeof(NetPacketHeader)
#define PACKET_SIZE 1024
#define PACKET_MAX_SIZE 8192

using SessionID = unsigned __int64;
