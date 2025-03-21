#pragma once

#define __DEFAULT_POOL_SIZE__ 100

#include <Windows.h>
#include <new.h>

template<typename T>
class LockFreePool
{
private:
	struct Node
	{
		T _data;
		__int64 _next;
	};

public:
	template<typename... Types>
	LockFreePool(int size, bool placementNew = false, Types... args);
	~LockFreePool();

public:
	void Free(T* data);
	template<typename...Types>
	T* Alloc(Types... args);

private:
	inline __int64 SetNode(__int64 id, void* ptr)
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
	bool _bPlacementNew;

	__int64 _top = NULL;
	volatile __int64 _id = 0;
};

template<typename T>
template<typename... Types>
LockFreePool<T>::LockFreePool(int size, bool placementNew, Types... args)
	: _bPlacementNew(placementNew)
{
	if (size <= 0)
	{
		size = __DEFAULT_POOL_SIZE__;
	}

	if (_bPlacementNew)
	{
		for (int iCnt = 0; iCnt < size; ++iCnt)
		{
			Node* node = new Node;
			node->_next = _top;

			__int64 id = InterlockedIncrement64(&_id);
			__int64 newTop = SetNode(id, node);

			_top = newTop;
		}
	}
	else
	{
		for (int iCnt = 0; iCnt < size; ++iCnt)
		{
			Node* node = new Node;
			node->_next = _top;

			new (&(node->_data)) T(args...);

			__int64 id = InterlockedIncrement64(&_id);
			__int64 newTop = SetNode(id, node);

			_top = newTop;
		}
	}
}

template<typename T>
LockFreePool<T>::~LockFreePool()
{
	if (_top == NULL)
	{
		return;
	}

	if (_bPlacementNew)
	{
		do
		{
			Node* topNode = (Node*)GetAddress(_top);
			__int64 next = topNode->_next;
			
			delete topNode;

			_top = next;
		} while (_top != NULL);
	}
	else
	{
		do
		{
			Node* topNode = (Node*)GetAddress(_top);
			__int64 next = topNode->_next;

			(topNode->_data).~T();
			delete topNode;

			_top = next;
		} while (_top != NULL);
	}
}

template<typename T>
void LockFreePool<T>::Free(T* data)
{
	__int64 id = InterlockedIncrement64(&_id);
	Node* newTopNode = (Node*)data;
	__int64 newTop = SetNode(id, data);
	__int64 tempTop;

	if (_bPlacementNew)
	{
		data->~T();
	}

	do
	{
		tempTop = _top;
		newTopNode->_next = tempTop;

	} while (InterlockedCompareExchange64(&_top, newTop, tempTop) != tempTop);

	return;
};

template<typename T>
template<typename... Types>
T* LockFreePool<T>::Alloc(Types... args)
{
	__int64 tempTop;
	Node* tempTopNode;
	__int64 next;

	do
	{
		tempTop = _top;

		if (tempTop == NULL)
		{
			Node* newNode = new Node;
			if (_bPlacementNew)
			{
				new (&(newNode->_data)) T(args...);
			}

			return &(newNode->_data);
		}

		tempTopNode = (Node*)GetAddress(tempTop);
		next = tempTopNode->_next;

	} while (InterlockedCompareExchange64(&_top, next, tempTop) != tempTop);

	if (_bPlacementNew)
	{
		new (&(tempTopNode->_data)) T(args...);
	}

	return &(tempTopNode->_data);
}