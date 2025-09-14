#include "RingBuffer.h"

#include <cstdio>
#include <cstring>

namespace MyDataStructure
{
	RingBuffer::RingBuffer(void)
		: mBuffer(new char[CAPACITY])
		, mFront(0)
		, mRear(0)
		, mCapacity(CAPACITY)
		, mSize(0)
	{
	}

	RingBuffer::RingBuffer(size_t capacity)
		: mBuffer(new char[capacity])
		, mFront(0)
		, mRear(0)
		, mCapacity(capacity)
		, mSize(0)
	{
	}

	void RingBuffer::Reserve(size_t capacity)
	{
		char* newBuffer;

		if (mCapacity >= capacity)
		{
			return;
		}

		newBuffer = new char[capacity];
		for (size_t iCnt = 0; iCnt < mSize; iCnt++)
		{
			newBuffer[iCnt] = mBuffer[iCnt];
		}

		delete[] mBuffer;
		mBuffer = newBuffer;

		mCapacity = capacity;
	}

	size_t RingBuffer::Capacity(void)
	{
		return mCapacity;
	}

	size_t RingBuffer::Size(void)
	{
		return mSize;
	}

	size_t RingBuffer::Enqueue(char* srcData, size_t size)
	{
		// I made the mistake of getting a direct size for the old size instead of getting a direct size for the new(bigger) size
		if (mCapacity < (mSize + size))
		{
			// if new capacity is larger than 8 times its origin, enqueuing is refused
			if ((mSize + size) > (CAPACITY << 3))
			{
				wprintf(L"Enqueue Failed\n");
				return 0;
			}

			size_t tempCapacity = mCapacity;
			while (tempCapacity < (mSize + size))
			{
				tempCapacity <<= 1;
			}

			if (tempCapacity > (CAPACITY << 3))
			{
				tempCapacity = (CAPACITY << 3);
			}

			Reserve(tempCapacity);
		}

		const size_t DIRECT_ENQUEUE_SIZE = DirectEnqueueSize();
		size_t result = 0;
		//size_t srcDataIdx;

		if (DIRECT_ENQUEUE_SIZE < size)
		{
			memcpy((mBuffer + mRear), srcData, DIRECT_ENQUEUE_SIZE);
			memcpy(mBuffer, srcData + DIRECT_ENQUEUE_SIZE, size - DIRECT_ENQUEUE_SIZE);
		}
		else
		{
			memcpy(mBuffer + mRear, srcData, size);
		}

		result = size;
		mSize += result;
		mRear = (mRear + size) % mCapacity;

		return result;
	}

	size_t RingBuffer::Dequeue(size_t size)
	{
		const size_t DIRECT_DEQUEUE_SIZE = DirectDequeueSize();

		// if Dequeuing size is bigger than stored size, dequeuing size is decreased by stored size.
		if (mSize < size)
		{
			size = mSize;
		}

		mFront = (mFront + size) % mCapacity;
		mSize -= size;

		return size;
	}

	size_t RingBuffer::Peek(char* dstData, size_t size)
	{
		const size_t DIRECT_DEQUEUE_SIZE = DirectDequeueSize();
		size_t result = 0;
		//size_t dstDataIdx;

		char* peekPtr = mBuffer + mFront;

		if (mSize < size)
		{
			size = mSize;
		}

		if (DIRECT_DEQUEUE_SIZE < size)
		{
			memcpy(dstData, peekPtr, DIRECT_DEQUEUE_SIZE);

			peekPtr = mBuffer;
			
			memcpy(dstData + DIRECT_DEQUEUE_SIZE, peekPtr, size - DIRECT_DEQUEUE_SIZE);
		}
		else
		{
			memcpy(dstData, peekPtr, size);
		}

		result = size;

		return result;
	}

	size_t RingBuffer::DirectEnqueueSize(void)
	{
		if (mRear >= mFront)
		{
			return mCapacity - mRear;
		}
		else
		{
			return mFront - mRear;
		}
	}

	size_t RingBuffer::DirectDequeueSize(void)
	{
		if (mRear >= mFront)
		{
			return mRear - mFront;
		}
		else
		{
			return mCapacity - mFront;
		}
	}

	void RingBuffer::ClearBuffer(void)
	{
		mFront = 0;
		mRear = 0;
		mSize = 0;
	}

	size_t RingBuffer::MoveFront(size_t size)
	{
		if (mSize < size)
		{
			size = mSize;
		}

		mFront = (mFront + size) % mCapacity;

		mSize -= size;

		return size;
	}

	size_t RingBuffer::MoveRear(size_t size)
	{
		if ((mCapacity - mSize) < size)
		{
			size = (mCapacity - mSize);
		}

		mRear = (mRear + size) % mCapacity;

		mSize += size;

		return size;
	}

	char* RingBuffer::GetBufferPtr(void)
	{
		return mBuffer;
	}

	char* RingBuffer::GetFrontBufferPtr(void)
	{
		return mBuffer + mFront;
	}

	char* RingBuffer::GetRearBufferPtr(void)
	{
		return mBuffer + mRear;
	}
}