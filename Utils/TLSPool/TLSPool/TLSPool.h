#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <Windows.h>
#include <new.h>

#define CHUNK_SIZE 2000
#define THREAD_MAX 20

template<typename T>
class ObjectPool
{
private:
	struct Chunk
	{
		T* _objects[CHUNK_SIZE];
		__int64 _next = 0;
	};

private:
	class TLSPool
	{
	public:
		TLSPool(ObjectPool* globalPool, bool placementNew);
		~TLSPool(void);

	public:
		template<typename... Types>
		inline void AcquireChunk(Types... args);
		inline void ReleaseChunk(void);
		template<typename... Types>
		inline void CreateChunk(Types... args);

	public:
		template<typename... Types>
		inline T* Alloc(Types... args);
		inline void Free(T* obj);

	private:
		ObjectPool<T>* _globalPool;		// master Pool
		bool _bPlacementNew;
		Chunk* _chunk = nullptr;
		Chunk* _freeChunk = nullptr;
		int _chunkIdx = 0;
		int _freeIdx = 0;
	};

public:
	template<typename... Types>
	ObjectPool(int chunkNum, bool placementNew, Types... arg);
	~ObjectPool();

public:
	template<typename... Types>
	inline void Initialize(Types... args);
	template<typename... Types>
	inline T* Alloc(Types... args);
	inline void Free(T* obj);

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

private:
	__int64 _top = 0;	// top pointer of bucket stack 
	volatile __int64 _id = 0;

private:
	volatile long _poolNum = 0;

private:
	thread_local static TLSPool* _tlsPool = nullptr;

	friend class TLSPool;
};


#pragma region DefineTLSPoolFunctions

template <typename T>
ObjectPool<T>::TLSPool::TLSPool(ObjectPool* globalPool, bool placementNew)
	: _globalPool(globalPool)
	, _bPlacementNew(placementNew)
{
	_freeChunk = new Chunk;
}

template <typename T>
ObjectPool<T>::TLSPool::~TLSPool(void)
{
	if (_bPlacementNew)
	{
		for (int iCnt = 0; iCnt < CHUNK_SIZE; ++iCnt)
		{
			free(_freeChunk->_objects[iCnt]);
		}
		delete _freeChunk;

		for (int iCnt = 0; iCnt < CHUNK_SIZE; ++iCnt)
		{
			free(_chunk->_objects[iCnt]);
		}
		delete _chunk;
	}
	else
	{
		for (int iCnt = 0; iCnt < CHUNK_SIZE; ++iCnt)
		{
			delete _freeChunk->_objects[iCnt];
		}
		delete _freeChunk;

		for (int iCnt = 0; iCnt < CHUNK_SIZE; ++iCnt)
		{
			delete _chunk->_objects[iCnt];
		}
		delete _chunk;
	}
}

template<typename T>
template<typename... Types>
inline void ObjectPool<T>::TLSPool::AcquireChunk(Types... args)
{
	__int64 tempTop;
	Chunk* tempTopChunk;
	__int64 tempTopNext;

	while (true)
	{
		tempTop = _globalPool->_top;

		if (tempTop == NULL)
		{
			CreateChunk(args...);
			break;
		}

		tempTopChunk = (Chunk*)ObjectPool<T>::GetAddress(tempTop);
		tempTopNext = tempTopChunk->_next;

		if (InterlockedCompareExchange64(&_globalPool->_top, tempTopNext, tempTop) == tempTop)
		{
			_chunk = tempTopChunk;
			break;
		}
	}

	return;
}

template<typename T>
inline void ObjectPool<T>::TLSPool::ReleaseChunk(void)
{
	__int64 id = InterlockedIncrement64(&_globalPool->_id);
	__int64 newTop = ObjectPool<T>::SetNodeValue(id, _freeChunk);

	while (true)
	{
		__int64 tempTop = _globalPool->_top;
		_freeChunk->_next = tempTop;

		if (InterlockedCompareExchange64(&_globalPool->_top, newTop, tempTop) == tempTop)
		{
			_freeChunk = new Chunk;
			break;
		}
	}

	return;
}

template<typename T>
template<typename... Types>
inline void ObjectPool<T>::TLSPool::CreateChunk(Types... args)
{
	_chunk = new Chunk;

	if (_bPlacementNew)
	{
		for (int iCnt = 0; iCnt < CHUNK_SIZE; ++iCnt)
		{
			_chunk->_objects[iCnt] = (T*)malloc(sizeof(T));
		}
	}
	else
	{
		for (int iCnt = 0; iCnt < CHUNK_SIZE; ++iCnt)
		{
			_chunk->_objects[iCnt] = new T(args...);
		}
	}
}

template<typename T>
template<typename... Types>
inline T* ObjectPool<T>::TLSPool::Alloc(Types... args)
{
	if (_chunkIdx >= CHUNK_SIZE)
	{
		AcquireChunk(args...);
		_chunkIdx = 0;
	}

	T* obj = _chunk->_objects[_chunkIdx];
	_chunkIdx++;

	if (_bPlacementNew)
	{
		new (obj) T(args...);
	}

	return obj;
}

template<typename T>
inline void ObjectPool<T>::TLSPool::Free(T* obj)
{
	if (_freeIdx >= CHUNK_SIZE)
	{
		ReleaseChunk();
		_freeIdx = 0;
	}

	_freeChunk->_objects[_freeIdx] = obj;
	_freeIdx++;

	if (_bPlacementNew)
	{
		obj->~T();
	}
}
#pragma endregion

#pragma region DefineGlobalPoolFunctions

template<typename T>
template<typename... Types>
ObjectPool<T>::ObjectPool(int chunkNum, bool placementNew, Types... arg)
	: _bPlacementNew(placementNew)
{
	if (chunkNum <= 0)
	{
		return;
	}


	if (placementNew)
	{
		for (int iCnt = 0; iCnt < chunkNum; ++iCnt)
		{
			Chunk* chunk = new Chunk;
			chunk->_next = _top;

			for (int jCnt = 0; jCnt < CHUNK_SIZE; ++jCnt)
			{
				chunk->_objects[jCnt] = (T*)malloc(sizeof(T));
			}

			__int64 id = InterlockedIncrement64(&_id);
			__int64 newTop = SetNodeValue(id, chunk);
			_top = newTop;
		}
	}
	else
	{
		for (int iCnt = 0; iCnt < chunkNum; ++iCnt)
		{
			Chunk* chunk = new Chunk;
			chunk->_next = _top;

			for (int jCnt = 0; jCnt < CHUNK_SIZE; ++jCnt)
			{
				chunk->_objects[jCnt] = new T(args...);
			}

			__int64 id = InterlockedIncrement64(&_id);
			__int64 newTop = SetNodeValue(id, chunk);
			_top = newTop;
		}
	}
}

template<typename T>
ObjectPool<T>::~ObjectPool(void)
{
	if (_top == NULL)
	{
		return;
	}

	if (_bPlacementNew)
	{
		Chunk* topChunk = (Chunk*)GetAddress(_top);

		while (true)
		{
			__int64 next = topChunk->_next;

			for (int iCnt = 0; iCnt < CHUNK_SIZE; ++iCnt)
			{
				free(topChunk->_objects[iCnt]);
			}

			delete topChunk;

			if (next == NULL)
			{
				break;
			}

			topChunk = (Chunk*)GetAddress(next);
		}
	}
	else
	{
		Chunk* topChunk = (Chunk*)GetAddress(_top);

		while (true)
		{
			__int64 next = topChunk->_next;

			for (int iCnt = 0; iCnt < CHUNK_SIZE; ++iCnt)
			{
				delete topChunk->_objects[iCnt];
			}

			delete topChunk;

			if (next == NULL)
			{
				break;
			}

			topChunk = (Chunk*)GetAddress(next);
		}
	}
}

template<typename T>
template<typename... Types>
inline void ObjectPool<T>::Initialize(Types... args)
{
	_tlsPool = new TLSPool(this, _bPlacementNew);
	_tlsPool->AcquireChunk(args...);

	return;
}

template<typename T>
template<typename... Types>
inline T* ObjectPool<T>::Alloc(Types... args)
{
	if (_tlsPool == nullptr)
	{
		Initialize(args...);
	}

	return _tlsPool->Alloc(args...);
}

template<typename T>
inline void ObjectPool<T>::Free(T* obj)
{
	if (_tlsPool == nullptr)
	{
		Initialize();
	}

	_tlsPool->Free(obj);

	return;
}

template<typename T>
thread_local typename ObjectPool<T>::TLSPool* ObjectPool<T>::_tlsPool = nullptr;

#pragma endregion