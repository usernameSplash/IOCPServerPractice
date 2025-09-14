#pragma once

#include <Windows.h>
#include <process.h>

//#include "LogData.h"
//#include "LockFreePool.h"
#include "TLSPool.h"

//#define __LOCKFREESTACK_DEBUG__

template<typename T>
class LockFreeStack
{
private:
	struct Node
	{
		T _value = 0;
		__int64 _next = 0;
	};

public:
	LockFreeStack()
	{
	}

	~LockFreeStack()
	{
		while (true)
		{
			if (_top == NULL)
			{
				break;
			}

			Node* curNode = (Node*)GetAddress(_top);

			_nodePool->Free(curNode);
			_top = curNode->_next;
		} while (_top != NULL);
	}

	void Push(T data);
	T Pop(void);

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

public:
	__int64 _top = NULL;
	volatile __int64 _id = 0;

private:
	//LockFreePool<Node> _nodePool;
	static ObjectPool<Node>* _nodePool;
};

template<typename T>
void LockFreeStack<T>::Push(T data)
{
	__int64 id = InterlockedIncrement64(&_id);
	Node* newNode;
	__int64 newTop;

	newNode = _nodePool->Alloc();
	newNode->_value = data;

	newTop = SetNodeValue(id, newNode);

	__int64 tempTop;

#ifdef __LOCKFREESTACK_DEBUG__
	__int64 idx = InterlockedIncrement64(&g_logIndex);
	g_logArray[idx % LOG_ARRAY_LEN]._idx = idx;
	g_logArray[idx % LOG_ARRAY_LEN]._threadId = GetCurrentThreadId();
	g_logArray[idx % LOG_ARRAY_LEN]._jobType = JOB_PUSH_ENTER;
	g_logArray[idx % LOG_ARRAY_LEN]._nodePtr = (void*)newNode;
	g_logArray[idx % LOG_ARRAY_LEN]._nextPtr = NULL;
#endif

	do
	{
		tempTop = _top;
		newNode->_next = tempTop;
	} while (InterlockedCompareExchange64(&_top, newTop, tempTop) != tempTop);

#ifdef __LOCKFREESTACK_DEBUG__
	if ((__int64)newNode == GetAddress(tempTop))
	{
		__debugbreak();
	}

	InterlockedIncrement64(&g_pushCnt);
	__int64 idx2 = InterlockedIncrement64(&g_logIndex);
	g_logArray[idx2 % LOG_ARRAY_LEN]._idx = idx2;
	g_logArray[idx2 % LOG_ARRAY_LEN]._threadId = GetCurrentThreadId();
	g_logArray[idx2 % LOG_ARRAY_LEN]._jobType = JOB_PUSH;
	g_logArray[idx2 % LOG_ARRAY_LEN]._nodePtr = (void*)newNode;
	g_logArray[idx2 % LOG_ARRAY_LEN]._nextPtr = (void*)GetAddress(tempTop);
#endif 

}

template<typename T>
T LockFreeStack<T>::Pop(void)
{
	__int64 tempTop;
	Node* tempTopNode;
	__int64 next;

#ifdef __LOCKFREESTACK_DEBUG__
	__int64 idx = InterlockedIncrement64(&g_logIndex);
	g_logArray[idx % LOG_ARRAY_LEN]._idx = idx;
	g_logArray[idx % LOG_ARRAY_LEN]._threadId = GetCurrentThreadId();
	g_logArray[idx % LOG_ARRAY_LEN]._jobType = JOB_POP_ENTER;
	g_logArray[idx % LOG_ARRAY_LEN]._nodePtr = NULL;
	g_logArray[idx % LOG_ARRAY_LEN]._nextPtr = NULL;
#endif

	do
	{
		tempTop = _top;
		tempTopNode = (Node*)GetAddress(tempTop);
		next = tempTopNode->_next;
	} while (InterlockedCompareExchange64(&_top, next, tempTop) != tempTop);

	T data;
	data = tempTopNode->_value;
	
#ifdef __LOCKFREESTACK_DEBUG__
	if ((__int64)tempTopNode == GetAddress(next))
	{
		__debugbreak();
	}

	InterlockedIncrement64(&g_popCnt);
	__int64 idx2 = InterlockedIncrement64(&g_logIndex);
	g_logArray[idx2 % LOG_ARRAY_LEN]._idx = idx2;
	g_logArray[idx2 % LOG_ARRAY_LEN]._threadId = GetCurrentThreadId();
	g_logArray[idx2 % LOG_ARRAY_LEN]._jobType = JOB_POP;
	g_logArray[idx2 % LOG_ARRAY_LEN]._nodePtr = (void*)tempTopNode;
	g_logArray[idx2 % LOG_ARRAY_LEN]._nextPtr = (void*)GetAddress(next);
#endif

	_nodePool->Free(tempTopNode);

	return data;
}

template <typename T>
ObjectPool<typename LockFreeStack<T>::Node>* LockFreeStack<T>::_nodePool = new ObjectPool<LockFreeStack<T>::Node>(10, false);