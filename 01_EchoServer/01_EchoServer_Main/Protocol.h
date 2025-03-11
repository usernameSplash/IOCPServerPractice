#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#define SERVER_ADDRESS L"0.0.0.0"
#define SERVER_PORT 11850

#define SESSION_MAX 5000

typedef struct EchoPacketHeader
{
	short _len;
} EchoPacketHeader;