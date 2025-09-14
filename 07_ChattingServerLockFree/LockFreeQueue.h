#pragma once

#include <Windows.h>

//#include "LockFreePool.h"
//#include "LogData.h"
#include "TLSPool.h"

template <typename T>
class LockFreeQueue
{
private:
	struct Node
	{
		T _value = 0;
		__int64 _next = 0;
	};

public:
	LockFreeQueue(void)
	{
		__int64 id = InterlockedIncrement64(&_id);
		
		Node* newNode = _nodePool->Alloc();
		newNode->_next = NULL;
		
		_head = SetNodeValue(id, newNode);
		_tail = _head;
	}

	~LockFreeQueue(void)
	{
		// To do..
	}

public:
	void Enqueue(T data);
	T Dequeue(void);

public:
	inline __int64 Size(void)
	{
		return _size;
	}

private:
	inline __int64 SetNodeValue(__int64 id, void* ptr)
	{
		__int64 retVal = (id << 47) | (__int64)ptr;
		return retVal;
	}

	inline __int64 GetAddress(__int64 val)
	{
		return (val & ADDRESS_MASK);
	}

	inline __int64 GetID(__int64 val)
	{
		return (val & KEY_MASK) >> 47;
	}

private:
	static constexpr unsigned __int64 ADDRESS_MASK = 0x00007fffffffffff;
	static constexpr unsigned __int64 KEY_MASK = 0xffff800000000000;

private:
	__int64 _head = 0;
	__int64 _tail = 0;
	volatile __int64 _id = 0;
	__int64 _size = 0;

private:
	//LockFreePool<Node> _nodePool;
	static ObjectPool<Node>* _nodePool;
};

template <typename T>
void LockFreeQueue<T>::Enqueue(T data)
{
	__int64 id = InterlockedIncrement64(&_id);
	
	Node* newNode = _nodePool->Alloc();
	newNode->_value = data;
	newNode->_next = NULL;

	__int64 newTail = SetNodeValue(id, newNode);

	while (true)
	{
		__int64 tempTail = _tail;
		Node* tempTailNode = (Node*)GetAddress(tempTail);
		__int64 tempTailNext = tempTailNode->_next;
		Node* nextNode = (Node*)GetAddress(tempTailNext);

		if (tempTailNext != NULL)
		{
			InterlockedCompareExchange64(&_tail, tempTailNext, tempTail);
			continue;
		}
		else
		{
			if (InterlockedCompareExchange64(&tempTailNode->_next, newTail, tempTailNext) == tempTailNext)
			{
				InterlockedCompareExchange64(&_tail, newTail, tempTail);

				InterlockedIncrement64(&_size);
				break;
			}
		}
	}
}

template <typename T>
T LockFreeQueue<T>::Dequeue(void)
{
	T data;

	if (_size == 0)
	{
		return 0;
	}

	while (true)
	{
		__int64 tempTail = _tail;
		__int64 tempHead = _head;
		Node* tempHeadNode = (Node*)GetAddress(tempHead);
		__int64 tempHeadNext = tempHeadNode->_next;

		if (_size == 0)
		{
			return 0;
		}

		if (tempHeadNext == NULL)
		{
			return 0;
		}

		if (tempHead == tempTail)
		{
			InterlockedCompareExchange64(&_tail, tempHeadNext, tempTail);
			continue;
		}

		Node* dataNode = (Node*)GetAddress(tempHeadNext);
		data = dataNode->_value;

		if (InterlockedCompareExchange64(&_head, tempHeadNext, tempHead) == tempHead)
		{
			_nodePool->Free(tempHeadNode);
			InterlockedDecrement64(&_size);
			break;
		}
	}

	return data;
}

template <typename T>
ObjectPool<typename LockFreeQueue<T>::Node>* LockFreeQueue<T>::_nodePool = new ObjectPool<LockFreeQueue<T>::Node>(10, false);