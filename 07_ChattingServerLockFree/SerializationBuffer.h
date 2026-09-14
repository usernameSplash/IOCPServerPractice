#pragma once

#include <Windows.h>

#include "TLSPool.h"
#include "Protocol.h"

// Packet Managed by Serialized Buffer.
class SPacket
{
public:
	enum eBufferDefault
	{
		BUFFER_HEADER_MAX_SIZE = 2,
		BUFFER_MINIMUM_SIZE = 10,
		BUFFER_DEFAULT_SIZE = 10,
		BUFFER_MAX_SIZE = 10,
	};

protected:
	SPacket();
	SPacket(size_t capacity);
	SPacket(const SPacket& other);
	virtual ~SPacket(void);

public:
	// Reference contract
	//  - Alloc 시 Usecount는 1
	//  - Recv path   : Library에서 Alloc, OnRecv에서 Recv용 Packet을 받고, 처리 후 Library에서 Free.
	//  - Send path   : Contents에서 Alloc, 보낼 횟수만큼 AddCount 설정, AddCount만큼 Contents에서 SendPost, 송신 몫은 Library에서 Free.
	//					SendPost 실패 시 Contents에서 Free. Send가 끝난 후 Contents의 Alloc에 대한 Free.

	static SPacket* Alloc()
	{
		SPacket* packet = s_packetPool.Alloc();
		packet->Clear();
		packet->_useCount = 1;
	
		return packet;
	}
	
	static bool Free(SPacket* packet)
	{
		long useCount = InterlockedDecrement(&packet->_useCount);

		if (useCount == 0)
		{
			s_packetPool.Free(packet);

			return true;
		}

		if (useCount < 0)
		{
			__debugbreak();
		}

		return false;
	}

	inline void AddUseCount(long useCount)
	{
		InterlockedAdd(&_useCount, useCount);
	}

	void Clear(void);
	size_t Capacity(void);
	size_t Size(void);
	size_t GetHeaderSize(void);
	size_t GetPayloadSize(void);

	bool Reserve(size_t size);

	char* GetBufferPtr(void);
	char* GetPayloadPtr(void);

	size_t MoveReadPos(size_t size);
	size_t MoveWritePos(size_t size);

	size_t SetHeaderData(void* header, size_t size);
	size_t GetHeaderData(void* outHeader, size_t size);

	size_t SetPayloadData(void* data, size_t size);
	size_t GetPayloadData(void* outData, size_t size);

	bool IsHeaderEmpty(void);

	bool Encode(NetPacketHeader& header, char* payload);
	bool Decode(NetPacketHeader& header, char* payload);

public:
	SPacket& operator=(SPacket& rhs);

	SPacket& operator<<(unsigned char data);
	SPacket& operator<<(char data);
	SPacket& operator<<(unsigned short data);
	SPacket& operator<<(short data);
	SPacket& operator<<(unsigned int data);
	SPacket& operator<<(int data);
	SPacket& operator<<(unsigned long data);
	SPacket& operator<<(long data);
	SPacket& operator<<(unsigned long long data);
	SPacket& operator<<(long long data);
	SPacket& operator<<(float data);
	SPacket& operator<<(double data);

	SPacket& operator>>(unsigned char& data);
	SPacket& operator>>(char& data);
	SPacket& operator>>(unsigned short& data);
	SPacket& operator>>(short& data);
	SPacket& operator>>(unsigned int& data);
	SPacket& operator>>(int& data);
	SPacket& operator>>(unsigned long& data);
	SPacket& operator>>(long& data);
	SPacket& operator>>(unsigned long long& data);
	SPacket& operator>>(long long& data);
	SPacket& operator>>(float& data);
	SPacket& operator>>(double& data);

protected:

	char* mBuffer;
	char* mPayloadPtr;
	char* mReadPos;
	char* mWritePos;

	size_t mCapacity;
	size_t mHeaderSize;
	size_t mSize;

	volatile long _useCount = 0;
	volatile long _isEncoded = 0;

	static ObjectPool<SPacket> s_packetPool;


	friend ObjectPool<SPacket>;
};