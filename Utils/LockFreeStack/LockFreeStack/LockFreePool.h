#pragma once

#define __DEFAULT_POOL_SIZE__ 100

#define __POOL_DEBUG__

#include <Windows.h>
#include <new.h>
#include <unordered_map>

using namespace std;

template<typename T>
class LockFreePool
{
private:
	struct Node
	{
		T _data = 0;
		__int64 _next = 0;
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
	bool _bPlacementNew;

public:
	__int64 _top = NULL;
	volatile __int64 _id = 0;

private:
#ifdef __POOL_DEBUG__
	unordered_map<__int64, __int64> _nodeMap;
#endif
};

template<typename T>
template<typename... Types>
LockFreePool<T>::LockFreePool(int size, bool placementNew, Types... args)
	: _bPlacementNew(placementNew)
{
#ifdef __POOL_DEBUG__
	_nodeMap.reserve(1000000);
#endif

	if (size <= 0)
	{
		return;
	}

	if (_bPlacementNew)
	{
		for (int iCnt = 0; iCnt < size; ++iCnt)
		{
			Node* node = new Node;
			node->_next = _top;

			__int64 id = InterlockedIncrement64(&_id);
			__int64 newTop = SetNodeValue(id, node);

#ifdef __POOL_DEBUG__
			_nodeMap.insert(make_pair((__int64)node, 0));
#endif __POOL_DEBUG__

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
			__int64 newTop = SetNodeValue(id, node);

#ifdef __POOL_DEBUG__
			_nodeMap.insert(make_pair((__int64)node, 0));
#endif __POOL_DEBUG__

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
		while (true)
		{
			if (_top == NULL)
			{
				break;
			}

			Node* topNode = (Node*)GetAddress(_top);

			__int64 next = topNode->_next;

			delete topNode;

			_top = next;
		}
	}
	else
	{
		while (true)
		{
			if (_top == NULL)
			{
				break;
			}

			Node* topNode = (Node*)GetAddress(_top);

			__int64 next = topNode->_next;

			(topNode->_data).~T();
			delete topNode;

			_top = next;
		}
	}
}

template<typename T>
void LockFreePool<T>::Free(T* data)
{
	__int64 id = InterlockedIncrement64(&_id);
	Node* newTopNode = (Node*)data;
	__int64 newTop = SetNodeValue(id, data);
	__int64 tempTop;

#ifdef __POOL_DEBUG__
	__int64 mapVal = _nodeMap[(__int64)newTopNode];
	if (mapVal >= 2)
	{
		__debugbreak();
	}

	__int64 result = InterlockedDecrement64(&_nodeMap[(__int64)newTopNode]);
	if (result != 0)
	{
		__debugbreak();
	}
#endif

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
	Node* tempTopNode;
	__int64 tempTop;
	__int64 next;

	do
	{
		tempTop = _top;

		if (tempTop == NULL)
		{
			Node* newNode = new Node;
			newNode->_next = NULL;

			if (_bPlacementNew)
			{
				new (&(newNode->_data)) T(args...);
			}

#ifdef __POOL_DEBUG__
			_nodeMap[(__int64)newNode] = 1;
#endif __POOL_DEBUG__

			return &(newNode->_data);
		}

		tempTopNode = (Node*)GetAddress(tempTop);
		next = tempTopNode->_next;

	} while (InterlockedCompareExchange64(&_top, next, tempTop) != tempTop);

	if (_bPlacementNew)
	{
		new (&(tempTopNode->_data)) T(args...);
	}

#ifdef __POOL_DEBUG__
	__int64 mapVal = _nodeMap[(__int64)tempTopNode];
	if (mapVal >= 1)
	{
		__debugbreak();
	}

	__int64 result = InterlockedIncrement64(&_nodeMap[(__int64)tempTopNode]);
	if (result != 1)
	{
		__debugbreak();
	}
#endif __POOL_DEBUG__

	return &(tempTopNode->_data);
}