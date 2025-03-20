#pragma once

#include <Windows.h>
#include <process.h>

#include "LogData.h"

template<typename T>
class LockFreeStack
{
private:
	struct Node
	{
		T _value;
		Node* _next;
	};

public:
	LockFreeStack()
		: _top(nullptr)
	{
	}

	~LockFreeStack()
	{
		Node* curNode = _top;
		while (curNode != nullptr)
		{
			Node* deleteNode = curNode;
			curNode = curNode->_next;

			delete deleteNode;
		}
	}

	void Push(T data);
	T Pop(void);

public:
	Node* _top;
};

template<typename T>
void LockFreeStack<T>::Push(T data)
{
	Node* tempTop;
	Node* newNode;

	newNode = new Node;
	newNode->_value = data;

	do
	{
		tempTop = _top;
		newNode->_next = tempTop;
	} while (InterlockedCompareExchange64((LONG64*)&_top, (LONG64)newNode, (LONG64)tempTop) != (LONG64)tempTop);

	long idx = InterlockedIncrement(&g_logIndex);
	g_logArray[idx % LOG_ARRAY_LEN]._idx = idx;
	g_logArray[idx % LOG_ARRAY_LEN]._threadId = GetCurrentThreadId();
	g_logArray[idx % LOG_ARRAY_LEN]._jobType = JOB_PUSH;
	g_logArray[idx % LOG_ARRAY_LEN]._nodePtr = (void*)newNode;
	g_logArray[idx % LOG_ARRAY_LEN]._nextPtr = (void*)tempTop;
}

template<typename T>
T LockFreeStack<T>::Pop(void)
{
	Node* tempTop;
	Node* next;
	T data;

	do
	{
		tempTop = _top;
		next = tempTop->_next;
	} while (InterlockedCompareExchange64((LONG64*)&_top, (LONG64)next, (LONG64)tempTop) != (LONG64)tempTop);

	data = tempTop->_value;
	
	long idx = InterlockedIncrement(&g_logIndex);
	g_logArray[idx % LOG_ARRAY_LEN]._idx = idx;
	g_logArray[idx % LOG_ARRAY_LEN]._threadId = GetCurrentThreadId();
	g_logArray[idx % LOG_ARRAY_LEN]._jobType = JOB_POP;
	g_logArray[idx % LOG_ARRAY_LEN]._nodePtr = (void*)tempTop;
	g_logArray[idx % LOG_ARRAY_LEN]._nextPtr = (void*)next;

	delete tempTop;

	return data;
}