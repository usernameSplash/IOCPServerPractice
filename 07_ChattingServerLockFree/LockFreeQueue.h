#pragma once

#include <Windows.h>

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
	static_assert(alignof(Node) % 2 == 0, "LSB of Node(next) is Used As The Empty Flag.");

public:
	LockFreeQueue(void)
	{
		_qid = InterlockedIncrement64(&s_qidProvider);

		Node* dummy = _nodePool->Alloc();
		dummy->_value = 0;
		dummy->_next = GetEmptyNext();

		__int64 tag = InterlockedIncrement64(&_id);
		_head = SetNodeValue(tag, dummy);
		_tail = _head;
	}

	~LockFreeQueue(void)
	{
		Node* node = GetNode(_head);
		while (node != nullptr)
		{
			Node* next = IsLinked(node->_next) ? GetNode(node->_next) : nullptr;
			_nodePool->Free(node);
			node = next;
		}
	}

public:
	void Enqueue(T data);
	T Dequeue(void);

	inline __int64 Size(void) const
	{
		return _size;
	}

private:
	inline static __int64 SetNodeValue(__int64 id, Node* node)
	{
		return (__int64)(((unsigned __int64)id << 47) | ((unsigned __int64)node & ADDRESS_MASK));
	}

	inline __int64 GetEmptyNext(void) const
	{
		return (__int64)(((unsigned __int64)_qid << 1) | 1);
	}

	inline static bool IsLinked(__int64 next)
	{
		return ((unsigned __int64)next & 1) == 0;
	}

	inline static Node* GetNode(__int64 word)
	{
		return (Node*)((unsigned __int64)word & ADDRESS_MASK);
	}

	inline static __int64 GetQID(__int64 word)
	{
		return (__int64)((unsigned __int64)word >> 1);
	}

private:
	static constexpr unsigned __int64 ADDRESS_MASK = 0x00007fffffffffff;

private:
	volatile __int64 _head = 0;
	volatile __int64 _tail = 0;
	volatile __int64 _id = 0;	// ABA 방지용 Node 태그
	volatile __int64 _size = 0;

private:
	volatile __int64 _qid;		// 해당 Queue에서 발급된 Object인지 식별하기 위한 Queue의 ID
								// 다른 Thread에서 나의 tail을 Dequeue하고 다른 Queue에 Enqueue한 경우를 탐지하기 위해 존재

private:
	static volatile __int64 s_qidProvider;
	static ObjectPool<Node>* _nodePool;
};

template <typename T>
void LockFreeQueue<T>::Enqueue(T data)
{
	Node* newNode = _nodePool->Alloc();
	newNode->_value = data;
	newNode->_next = GetEmptyNext();

	__int64 id = InterlockedIncrement64(&_id);
	__int64 newTail = SetNodeValue(id, newNode);

	while (true)
	{
		__int64 tail = _tail;
		Node* tailNode = GetNode(tail);
		__int64 next = tailNode->_next;

		if (IsLinked(next))
		{
			InterlockedCompareExchange64(&_tail, next, tail);
			continue;
		}

		if (GetQID(next) != _qid)
		{
			continue;
		}

		if (InterlockedCompareExchange64(&tailNode->_next, newTail, next) == next)
		{
			InterlockedCompareExchange64(&_tail, newTail, tail);
			InterlockedIncrement64(&_size);
			break;
		}
	}
}

template <typename T>
T LockFreeQueue<T>::Dequeue(void)
{
	while (true)
	{
		__int64 tempHead = _head;
		__int64 tempTail = _tail;
		Node* headNode = GetNode(tempHead);
		__int64 tempHeadNext = headNode->_next;

		if (tempHead != _head)
		{
			continue;
		}

		if (_size == 0)
		{
			return 0;
		}

		if (IsLinked(tempHeadNext) == false)
		{
			return 0;
		}

		if (tempHead == tempTail)
		{
			InterlockedCompareExchange64(&_tail, tempHeadNext, tempTail);
			continue;
		}

		Node* dataNode = GetNode(tempHeadNext);
		T data = dataNode->_value;

		if (InterlockedCompareExchange64(&_head, tempHeadNext, tempHead) == tempHead)
		{
			// Decrement Before Free : _size can be under-reported
			// but over-report is never allowed.

			InterlockedDecrement64(&_size);
			_nodePool->Free(headNode);
			return data;
		}
	}
}

template <typename T>
volatile __int64 LockFreeQueue<T>::s_qidProvider = 0;

template <typename T>
ObjectPool<typename LockFreeQueue<T>::Node>* LockFreeQueue<T>::_nodePool = new ObjectPool<LockFreeQueue<T>::Node>(10, false);
