#pragma once

#include <Windows.h>

// Packet Managed by Serialized Buffer.
class SPacket
{
public:
	enum eBufferDefault
	{
		BUFFER_HEADER_MAX_SIZE = 64,
		BUFFER_MINIMUM_SIZE = 128,
		BUFFER_DEFAULT_SIZE = 128,
		BUFFER_MAX_SIZE = 1024,
	};

	SPacket();
	SPacket(size_t capacity);
	SPacket(const SPacket& other);
	virtual ~SPacket(void);

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

	void SetHeaderData(void* header, size_t size);
	void GetHeaderData(void* outHeader);
	size_t GetHeaderData(void* outHeader, size_t size);

	void SetPayloadData(void* data, size_t size);
	void GetPayloadData(void* outData);
	size_t GetPayloadData(void* outData, size_t size);

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
};