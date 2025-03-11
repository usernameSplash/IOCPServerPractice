#include "SerializationBuffer.h"

#include <cstdio>
#include <cstring>

SPacket::SPacket()
	: mBuffer(new char[BUFFER_DEFAULT_SIZE])
	, mPayloadPtr(mBuffer + sizeof(BUFFER_HEADER_MAX_SIZE))
	, mReadPos(mPayloadPtr)
	, mWritePos(mPayloadPtr)
	, mCapacity(BUFFER_DEFAULT_SIZE)
	, mHeaderSize(0)
	, mSize(BUFFER_HEADER_MAX_SIZE)
{
}

SPacket::SPacket(size_t capacity)
	: mHeaderSize(0)
	, mSize(BUFFER_HEADER_MAX_SIZE)
{
	if (capacity < BUFFER_MINIMUM_SIZE)
	{
		capacity = BUFFER_MINIMUM_SIZE;
	}
	else if (capacity > BUFFER_MAX_SIZE)
	{
		capacity = BUFFER_MAX_SIZE;
	}

	mBuffer = new char[capacity];
	mPayloadPtr = mBuffer + BUFFER_HEADER_MAX_SIZE;
	mReadPos = mPayloadPtr;
	mWritePos = mPayloadPtr;
	mCapacity = capacity;
}

SPacket::SPacket(const SPacket& other)
	: mBuffer(new char[other.mCapacity])
	, mPayloadPtr(mBuffer + BUFFER_HEADER_MAX_SIZE)
	, mReadPos(mPayloadPtr)
	, mWritePos(mPayloadPtr + other.mSize)
	, mCapacity(other.mCapacity)
	, mSize(other.mSize)
{
	memcpy(mBuffer, other.mBuffer, mSize);
}

SPacket::~SPacket(void)
{
	delete mBuffer;
	mBuffer = nullptr;
}

void SPacket::Clear(void)
{
	mSize = BUFFER_HEADER_MAX_SIZE;
	mReadPos = mPayloadPtr;
	mWritePos = mPayloadPtr;
	return;
}

size_t SPacket::Capacity(void)
{
	return mCapacity - BUFFER_HEADER_MAX_SIZE;
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
	return mSize - BUFFER_HEADER_MAX_SIZE;
}

bool SPacket::Reserve(size_t capacity)
{
	char* newBuffer;

	if (mCapacity > capacity)
	{
		return false;
	}

	if (capacity > BUFFER_MAX_SIZE)
	{
		return false;
	}

	newBuffer = new char[capacity];

	memcpy(newBuffer, mBuffer, mSize);

	mCapacity = capacity;

	delete mBuffer;

	mBuffer = newBuffer;
	mPayloadPtr = mBuffer + BUFFER_HEADER_MAX_SIZE;
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

void SPacket::SetHeaderData(void* header, size_t size)
{
	memcpy(mBuffer, header, size);
}

void SPacket::GetHeaderData(void* outHeader)
{
	memcpy(outHeader, mBuffer, mHeaderSize);
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
	memcpy(mPayloadPtr, data, size);
}

void SPacket::GetPayloadData(void* outData)
{
	memcpy(outData, mPayloadPtr, (mSize - BUFFER_HEADER_MAX_SIZE));
}

size_t SPacket::GetPayloadData(void* outData, size_t size)
{
	if (size > mSize - BUFFER_HEADER_MAX_SIZE)
	{
		size = mSize - BUFFER_HEADER_MAX_SIZE;
	}

	memcpy(outData, mPayloadPtr, size);

	return size;
}


SPacket& SPacket::operator=(SPacket& rhs)
{
	if (this != &rhs)
	{
		delete mBuffer;

		mBuffer = new char[rhs.mCapacity];
		mPayloadPtr = mBuffer + BUFFER_HEADER_MAX_SIZE;
		mReadPos = mPayloadPtr;
		mWritePos = mPayloadPtr;
		mCapacity = rhs.mCapacity;
		mSize = rhs.mSize;
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
