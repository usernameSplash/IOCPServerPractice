#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <Windows.h>
#include <new.h>
#include <cstring>

#define CHUNK_SIZE 2000
#define THREAD_MAX 20

template<typename T>
class ObjectPool
{
private:
	struct Chunk
	{
		T* _objects[CHUNK_SIZE] = { nullptr, };
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
		ObjectPool<T>* _globalPool;
		bool _bPlacementNew;
		Chunk* _chunk = nullptr;
		Chunk* _freeChunk = nullptr;
		Chunk* _emptyChunk = nullptr;
		int _chunkIdx = CHUNK_SIZE;		// SubPool 맨 처음 초기화 시 Chunk를 모두 소진한 것과 동일하게 처리하기 위한 초기 값
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
	inline static __int64 SetNodeValue(__int64 id, void* ptr)
	{
		__int64 retVal = (id << 47) | (__int64)ptr;
		return retVal;
	}

	inline static __int64 GetAddress(__int64 val)
	{
		return (val & ADDRESS_MASK);
	}

	inline static  __int64 GetID(__int64 val)
	{
		return (val & KEY_MASK) >> 47;
	}

	inline void PushEmptyChunk(Chunk* chunk);
	inline Chunk* PopEmptyChunk(void);

private:
	static constexpr unsigned __int64 ADDRESS_MASK = 0x00007fffffffffff;
	static constexpr unsigned __int64 KEY_MASK = 0xffff800000000000;

private:
	bool _bPlacementNew;

private:
	volatile __int64 _top = 0;
	volatile __int64 _emptyTop = 0;
	volatile __int64 _id = 0;

private:
	volatile long _poolNum = 0;
	volatile long _chunkTotal = 0;
	volatile long _chunkInGlobal = 0;

public:
	inline long GetChunkTotal(void) const
	{
		return _chunkTotal;
	}

	inline long GetChunkInGlobal(void) const
	{
		return _chunkInGlobal;
	}

	inline long GetCapacity(void) const
	{
		return _chunkTotal * CHUNK_SIZE;
	}

	inline long GetGlobalFree(void) const
	{
		return _chunkInGlobal * CHUNK_SIZE;
	}

private:
	thread_local static TLSPool* _tlsPool;

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
	// 아직 사용자에게 넘기지 않은 개체만 정리한다.
	//   _chunk      : [_chunkIdx, CHUNK_SIZE)  아직 꺼내지 않은 것
	//   _freeChunk  : [0, _freeIdx)            반환되어 쌓인 것
	//   _emptyChunk : 개체가 없다 — 껍데기만 해제
	if (_chunk != nullptr)
	{
		for (int iCnt = _chunkIdx; iCnt < CHUNK_SIZE; ++iCnt)
		{
			if (_bPlacementNew)
			{
				free(_chunk->_objects[iCnt]);
			}
			else
			{
				delete _chunk->_objects[iCnt];
			}
		}
		delete _chunk;
	}

	if (_freeChunk != nullptr)
	{
		for (int iCnt = 0; iCnt < _freeIdx; ++iCnt)
		{
			if (_bPlacementNew)
			{
				free(_freeChunk->_objects[iCnt]);
			}
			else
			{
				delete _freeChunk->_objects[iCnt];
			}
		}
		delete _freeChunk;
	}

	delete _emptyChunk;
}

template<typename T>
template<typename... Types>
inline void ObjectPool<T>::TLSPool::AcquireChunk(Types... args)
{
	__int64 tempTop;
	Chunk* tempTopChunk;
	__int64 tempTopNext;

	// 다 쓴 껍데기를 놓아 준다 — 로컬 캐시가 비어 있으면 로컬, 이미 차 있으면 전역으로
	if (_chunk != nullptr)
	{
		memset(_chunk->_objects, 0, sizeof(_chunk->_objects));

		if (_emptyChunk == nullptr)
		{
			_emptyChunk = _chunk;
		}
		else
		{
			_globalPool->PushEmptyChunk(_chunk);
		}

		_chunk = nullptr;
	}

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
			InterlockedDecrement(&_globalPool->_chunkInGlobal);
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
			InterlockedIncrement(&_globalPool->_chunkInGlobal);

			// 새 반환용 껍데기 : 로컬 캐시 -> 전역 빈 스택 -> 정말 없을 때만 new
			if (_emptyChunk != nullptr)
			{
				_freeChunk = _emptyChunk;
				_emptyChunk = nullptr;
			}
			else
			{
				Chunk* emptyChunk = _globalPool->PopEmptyChunk();
				_freeChunk = (emptyChunk != nullptr) ? emptyChunk : new Chunk;
			}

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
	InterlockedIncrement(&_globalPool->_chunkTotal);

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
inline void ObjectPool<T>::PushEmptyChunk(Chunk* chunk)
{
	__int64 id = InterlockedIncrement64(&_id);
	__int64 newTop = SetNodeValue(id, chunk);

	while (true)
	{
		__int64 tempTop = _emptyTop;
		chunk->_next = tempTop;

		if (InterlockedCompareExchange64(&_emptyTop, newTop, tempTop) == tempTop)
		{
			break;
		}
	}

	return;
}

template<typename T>
inline typename ObjectPool<T>::Chunk* ObjectPool<T>::PopEmptyChunk(void)
{
	while (true)
	{
		__int64 tempTop = _emptyTop;

		if (tempTop == NULL)
		{
			return nullptr;
		}

		Chunk* tempTopChunk = (Chunk*)GetAddress(tempTop);
		__int64 tempTopNext = tempTopChunk->_next;

		if (InterlockedCompareExchange64(&_emptyTop, tempTopNext, tempTop) == tempTop)
		{
			return tempTopChunk;
		}
	}
}

template<typename T>
template<typename... Types>
ObjectPool<T>::ObjectPool(int chunkNum, bool placementNew, Types... args)
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
			++_chunkTotal;
			++_chunkInGlobal;
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
			++_chunkTotal;
			++_chunkInGlobal;
		}
	}
}

template<typename T>
ObjectPool<T>::~ObjectPool(void)
{
	// 빈 껍데기 스택 정리 — 개체는 없고 껍데기만 있다
	__int64 emptyCur = _emptyTop;
	while (GetAddress(emptyCur) != 0)
	{
		Chunk* chunk = (Chunk*)GetAddress(emptyCur);
		__int64 next = chunk->_next;
		delete chunk;
		emptyCur = next;
	}
	_emptyTop = 0;

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
	// _tlsPool->AcquireChunk(args...); // Free만 발생하는 thread의 경우 Alloc용 Chunk가 불필요하므로 삭제

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