#include "SerializationBuffer.h"

#include <cstdio>
#include <cstring>

SPacket::SPacket()
	: mBuffer(new char[PACKET_SIZE])
	, mPayloadPtr(mBuffer + PACKET_HEADER_SIZE)
	, mReadPos(mPayloadPtr)
	, mWritePos(mPayloadPtr)
	, mCapacity(PACKET_SIZE)
	, mHeaderSize(0)
	, mSize(PACKET_HEADER_SIZE)
{
}

SPacket::SPacket(size_t capacity)
	: mHeaderSize(0)
	, mSize(PACKET_HEADER_SIZE)
{
	if (capacity < PACKET_SIZE)
	{
		capacity = PACKET_SIZE;
	}
	else if (capacity > PACKET_MAX_SIZE)
	{
		capacity = PACKET_MAX_SIZE;
	}

	mBuffer = new char[capacity];
	mPayloadPtr = mBuffer + PACKET_HEADER_SIZE;
	mReadPos = mPayloadPtr;
	mWritePos = mPayloadPtr;
	mCapacity = capacity;
}

SPacket::SPacket(const SPacket& other)
	: mBuffer(new char[other.mCapacity])
	, mPayloadPtr(mBuffer + PACKET_HEADER_SIZE)
	, mReadPos(mPayloadPtr)
	, mWritePos(mPayloadPtr + other.mSize)
	, mCapacity(other.mCapacity)
	, mSize(other.mSize)
	, mHeaderSize(other.mHeaderSize)
{
	memcpy(mBuffer, other.mBuffer, mSize);
}

SPacket::~SPacket(void)
{
	delete[] mBuffer;
	mBuffer = nullptr;
}

void SPacket::Clear(void)
{
	mSize = PACKET_HEADER_SIZE;
	mReadPos = mPayloadPtr;
	mWritePos = mPayloadPtr;
	mHeaderSize = 0;
	_isEncoded = 0;

	return;
}

size_t SPacket::Capacity(void)
{
	return mCapacity - PACKET_HEADER_SIZE;
}

size_t SPacket::Size(void)
{
	return mSize;
}

size_t SPacket::GetHeaderSize(void)
{
	return mHeaderSize;
}

size_t SPacket::GetPayloadSize(void)
{
	return mSize - PACKET_HEADER_SIZE;
}

bool SPacket::Reserve(size_t capacity)
{
	char* newBuffer;

	if (mCapacity > capacity)
	{
		return false;
	}

	if (capacity > PACKET_MAX_SIZE)
	{
		return false;
	}

	newBuffer = new char[capacity];

	memcpy(newBuffer, mBuffer, mSize);

	mCapacity = capacity;

	delete[] mBuffer;

	mBuffer = newBuffer;
	mPayloadPtr = mBuffer + PACKET_HEADER_SIZE;
	mReadPos = mPayloadPtr;
	mWritePos = mPayloadPtr + mSize;

	return true;
}

char* SPacket::GetBufferPtr(void)
{
	return mBuffer;
}

char* SPacket::GetPayloadPtr(void)
{
	return mPayloadPtr;
}

size_t SPacket::MoveReadPos(size_t size)
{
	if (size > mSize)
	{
		size = mSize;
	}

	mSize -= size;
	mReadPos += size;

	return size;
}

size_t SPacket::MoveWritePos(size_t size)
{
	size_t freeSize = mCapacity - mSize;

	if (size > freeSize)
	{
		size = freeSize;
	}

	mSize += size;
	mWritePos += size;

	return size;
}

size_t SPacket::SetHeaderData(void* header, size_t size)
{
	memcpy(mBuffer, header, size);
	mHeaderSize = size;

	return mHeaderSize;
}

size_t SPacket::GetHeaderData(void* outHeader, size_t size)
{
	if (size > mHeaderSize)
	{
		size = mHeaderSize;
	}

	memcpy(outHeader, mBuffer, size);

	return size;
}

void SPacket::SetPayloadData(void* data, size_t size)
{
	memcpy(mWritePos, data, size);
}

size_t SPacket::GetPayloadData(void* outData, size_t size)
{
	if (size > mSize - PACKET_HEADER_SIZE)
	{
		size = mSize - PACKET_HEADER_SIZE;
	}

	memcpy(outData, mReadPos, size);

	return size;
}

bool SPacket::IsHeaderEmpty(void)
{
	return mHeaderSize == 0;
}

bool SPacket::Encode(NetPacketHeader& header, char* payload)
{
	if (InterlockedExchange(&_isEncoded, 1) == 1)
	{
		return false;
	}

	unsigned char checkSum = 0;

	for (int iCnt = 0; iCnt < header._len; ++iCnt)
	{
		checkSum += payload[iCnt];
	}

	char d = checkSum % 256;
	char p = d ^ (header._randKey + 1);
	char e = p ^ (PACKET_KEY + 1);
	header._checkSum = e;

	for (int iCnt = 0; iCnt < header._len; ++iCnt)
	{
		d = payload[iCnt];
		p = d ^ (p + header._randKey + 2 + iCnt);
		e = p ^ (e + PACKET_KEY + 2 + iCnt);
		payload[iCnt] = e;
	}

	return true;
}

bool SPacket::Decode(NetPacketHeader& header, char* payload)
{
	unsigned char checkSum = 0;

	char curE = header._checkSum;
	char curP = curE ^ (PACKET_KEY + 1);
	header._checkSum = curP ^ (header._randKey + 1);

	char prevE = curE;
	char prevP = curP;

	for (int iCnt = 0; iCnt < header._len; ++iCnt)
	{
		curE = payload[iCnt];
		curP = curE ^ (prevE + PACKET_KEY + 2 + iCnt);
		payload[iCnt] = curP ^ (prevP + header._randKey + 2 + iCnt);
		checkSum += payload[iCnt];

		prevE = curE;
		prevP = curP;
	}

	checkSum = checkSum % 256;

	if (header._checkSum != checkSum)
	{
		return false;
	}

	return true;
}

SPacket& SPacket::operator=(SPacket& rhs)
{
	if (this != &rhs)
	{
		delete mBuffer;

		mBuffer = new char[rhs.mCapacity];
		mPayloadPtr = mBuffer + PACKET_HEADER_SIZE;
		mReadPos = mPayloadPtr;
		mWritePos = mPayloadPtr;
		mCapacity = rhs.mCapacity;
		mSize = rhs.mSize;
		mHeaderSize = rhs.mHeaderSize;
	}

	return *this;
}

SPacket& SPacket::operator<<(unsigned char data)
{
	if (mPayloadPtr != mReadPos)
	{
#ifdef DEBUG
		wprintf(L"[SPacket] : Do Not Input Additional Data After Any Output Behavior\n");
#endif
		return *this;
	}

	if ((mSize + sizeof(data) > mCapacity))
	{
		if (!Reserve(mCapacity * 2))
		{
#ifdef DEBUG
			wprintf(L"[SPacket] : Input Error, Data : %u\n", data);
#endif
			return *this;
		}
	}

	memcpy(mWritePos, &data, sizeof(data));
	mSize += sizeof(data);
	mWritePos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator<<(char data)
{
	if (mPayloadPtr != mReadPos)
	{
#ifdef DEBUG
		wprintf(L"[SPacket] : Do Not Input Additional Data After Any Output Behavior\n");
#endif
		return *this;
	}

	if ((mSize + sizeof(data) > mCapacity))
	{
		if (!Reserve(mCapacity * 2))
		{
#ifdef DEBUG
			wprintf(L"[SPacket] : Input Error, Data : %u\n", data);
#endif
			return *this;
		}
	}

	memcpy(mWritePos, &data, sizeof(data));
	mSize += sizeof(data);
	mWritePos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator<<(unsigned short data)
{
	if (mPayloadPtr != mReadPos)
	{
#ifdef DEBUG
		wprintf(L"[SPacket] : Do Not Input Additional Data After Any Output Behavior\n");
#endif
		return *this;
	}

	if ((mSize + sizeof(data) > mCapacity))
	{
		if (!Reserve(mCapacity * 2))
		{
#ifdef DEBUG
			wprintf(L"[SPacket] : Input Error, Data : %u\n", data);
#endif
			return *this;
		}
	}

	memcpy(mWritePos, &data, sizeof(data));
	mSize += sizeof(data);
	mWritePos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator<<(short data)
{
	if (mPayloadPtr != mReadPos)
	{
#ifdef DEBUG
		wprintf(L"[SPacket] : Do Not Input Additional Data After Any Output Behavior\n");
#endif
		return *this;
	}

	if ((mSize + sizeof(data) > mCapacity))
	{
		if (!Reserve(mCapacity * 2))
		{
#ifdef DEBUG
			wprintf(L"[SPacket] : Input Error, Data : %u\n", data);
#endif
			return *this;
		}
	}

	memcpy(mWritePos, &data, sizeof(data));
	mSize += sizeof(data);
	mWritePos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator<<(unsigned int data)
{
	if (mPayloadPtr != mReadPos)
	{
#ifdef DEBUG
		wprintf(L"[SPacket] : Do Not Input Additional Data After Any Output Behavior\n");
#endif
		return *this;
	}

	if ((mSize + sizeof(data) > mCapacity))
	{
		if (!Reserve(mCapacity * 2))
		{
#ifdef DEBUG
			wprintf(L"[SPacket] : Input Error, Data : %u\n", data);
#endif
			return *this;
		}
	}

	memcpy(mWritePos, &data, sizeof(data));
	mSize += sizeof(data);
	mWritePos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator<<(int data)
{
	if (mPayloadPtr != mReadPos)
	{
#ifdef DEBUG
		wprintf(L"[SPacket] : Do Not Input Additional Data After Any Output Behavior\n");
#endif
		return *this;
	}

	if ((mSize + sizeof(data) > mCapacity))
	{
		if (!Reserve(mCapacity * 2))
		{
#ifdef DEBUG
			wprintf(L"[SPacket] : Input Error, Data : %u\n", data);
#endif
			return *this;
		}
	}

	memcpy(mWritePos, &data, sizeof(data));
	mSize += sizeof(data);
	mWritePos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator<<(unsigned long data)
{
	if (mPayloadPtr != mReadPos)
	{
#ifdef DEBUG
		wprintf(L"[SPacket] : Do Not Input Additional Data After Any Output Behavior\n");
#endif
		return *this;
	}

	if ((mSize + sizeof(data) > mCapacity))
	{
		if (!Reserve(mCapacity * 2))
		{
#ifdef DEBUG
			wprintf(L"[SPacket] : Input Error, Data : %u\n", data);
#endif
			return *this;
		}
	}

	memcpy(mWritePos, &data, sizeof(data));
	mSize += sizeof(data);
	mWritePos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator<<(long data)
{
	if (mPayloadPtr != mReadPos)
	{
#ifdef DEBUG
		wprintf(L"[SPacket] : Do Not Input Additional Data After Any Output Behavior\n");
#endif
		return *this;
	}

	if ((mSize + sizeof(data) > mCapacity))
	{
		if (!Reserve(mCapacity * 2))
		{
#ifdef DEBUG
			wprintf(L"[SPacket] : Input Error, Data : %u\n", data);
#endif
			return *this;
		}
	}

	memcpy(mWritePos, &data, sizeof(data));
	mSize += sizeof(data);
	mWritePos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator<<(unsigned long long data)
{
	if (mPayloadPtr != mReadPos)
	{
#ifdef DEBUG
		wprintf(L"[SPacket] : Do Not Input Additional Data After Any Output Behavior\n");
#endif
		return *this;
	}

	if ((mSize + sizeof(data) > mCapacity))
	{
		if (!Reserve(mCapacity * 2))
		{
#ifdef DEBUG
			wprintf(L"[SPacket] : Input Error, Data : %u\n", data);
#endif
			return *this;
		}
	}

	memcpy(mWritePos, &data, sizeof(data));
	mSize += sizeof(data);
	mWritePos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator<<(long long data)
{
	if (mPayloadPtr != mReadPos)
	{
#ifdef DEBUG
		wprintf(L"[SPacket] : Do Not Input Additional Data After Any Output Behavior\n");
#endif
		return *this;
	}

	if ((mSize + sizeof(data) > mCapacity))
	{
		if (!Reserve(mCapacity * 2))
		{
#ifdef DEBUG
			wprintf(L"[SPacket] : Input Error, Data : %u\n", data);
#endif
			return *this;
		}
	}

	memcpy(mWritePos, &data, sizeof(data));
	mSize += sizeof(data);
	mWritePos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator<<(float data)
{
	if (mPayloadPtr != mReadPos)
	{
#ifdef DEBUG
		wprintf(L"[SPacket] : Do Not Input Additional Data After Any Output Behavior\n");
#endif
		return *this;
	}

	if ((mSize + sizeof(data) > mCapacity))
	{
		if (!Reserve(mCapacity * 2))
		{
#ifdef DEBUG
			wprintf(L"[SPacket] : Input Error, Data : %u\n", data);
#endif
			return *this;
		}
	}

	memcpy(mWritePos, &data, sizeof(data));
	mSize += sizeof(data);
	mWritePos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator<<(double data)
{
	if (mPayloadPtr != mReadPos)
	{
#ifdef DEBUG
		wprintf(L"[SPacket] : Do Not Input Additional Data After Any Output Behavior\n");
#endif
		return *this;
	}

	if ((mSize + sizeof(data) > mCapacity))
	{
		if (!Reserve(mCapacity * 2))
		{
#ifdef DEBUG
			wprintf(L"[SPacket] : Input Error, Data : %u\n", data);
#endif
			return *this;
		}
	}

	memcpy(mWritePos, &data, sizeof(data));
	mSize += sizeof(data);
	mWritePos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator>>(unsigned char& data)
{
	if (sizeof(data) > mSize)
	{
		return *this;
	}

	data = *(unsigned char*)(mReadPos);
	mSize -= sizeof(data);
	mReadPos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator>>(char& data)
{
	if (sizeof(data) > mSize)
	{
		return *this;
	}

	data = *mReadPos;
	mSize -= sizeof(data);
	mReadPos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator>>(unsigned short& data)
{
	if (sizeof(data) > mSize)
	{
		return *this;
	}

	data = *(unsigned short*)(mReadPos);
	mSize -= sizeof(data);
	mReadPos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator>>(short& data)
{
	if (sizeof(data) > mSize)
	{
		return *this;
	}

	data = *(short*)(mReadPos);
	mSize -= sizeof(data);
	mReadPos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator>>(unsigned int& data)
{
	if (sizeof(data) > mSize)
	{
		return *this;
	}

	data = *(unsigned int*)(mReadPos);
	mSize -= sizeof(data);
	mReadPos += sizeof(data);

	return *this;
}


SPacket& SPacket::operator>>(int& data)
{
	if (sizeof(data) > mSize)
	{
		return *this;
	}

	data = *(int*)(mReadPos);
	mSize -= sizeof(data);
	mReadPos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator>>(unsigned long& data)
{
	if (sizeof(data) > mSize)
	{
		return *this;
	}

	data = *(unsigned long*)(mReadPos);
	mSize -= sizeof(data);
	mReadPos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator>>(long& data)
{
	if (sizeof(data) > mSize)
	{
		return *this;
	}

	data = *(long*)(mReadPos);
	mSize -= sizeof(data);
	mReadPos += sizeof(data);

	return *this;
}


SPacket& SPacket::operator>>(unsigned long long& data)
{
	if (sizeof(data) > mSize)
	{
		return *this;
	}

	data = *(unsigned long long*)(mReadPos);
	mSize -= sizeof(data);
	mReadPos += sizeof(data);

	return *this;
}


SPacket& SPacket::operator>>(long long& data)
{
	if (sizeof(data) > mSize)
	{
		return *this;
	}

	data = *(long long*)(mReadPos);
	mSize -= sizeof(data);
	mReadPos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator>>(float& data)
{
	if (sizeof(data) > mSize)
	{
		return *this;
	}

	data = *(float*)(mReadPos);
	mSize -= sizeof(data);
	mReadPos += sizeof(data);

	return *this;
}

SPacket& SPacket::operator>>(double& data)
{
	if (sizeof(data) > mSize)
	{
		return *this;
	}

	data = *(double*)(mReadPos);
	mSize -= sizeof(data);
	mReadPos += sizeof(data);

	return *this;
}

ObjectPool<SPacket> SPacket::s_packetPool = ObjectPool<SPacket>(10, false);