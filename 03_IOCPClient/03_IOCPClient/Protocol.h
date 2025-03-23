#pragma once

#define SERVER_ADDRESS L"127.0.0.1"
#define SERVER_PORT 6000 // 11850

typedef struct EchoPacketHeader
{
	short _len;
} EchoPacketHeader;