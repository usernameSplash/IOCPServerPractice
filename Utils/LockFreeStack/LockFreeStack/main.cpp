
#include "LogData.h"
#include "LockFreeStack.h"
#include "CrashDump.h"

#include <cstdio>
#include <Windows.h>
#include <process.h>

#define THREAD_NUM 4
#define REPEAT_NUM 10000
#define LOOP_NUM 100000

unsigned int WINAPI ThreadProc(void* arg);

unsigned int WINAPI PushProc(void* arg);
unsigned int WINAPI PopProc(void* arg);

unsigned int WINAPI PoolTestProc(void* arg);

void Test1(void);
void Test2(void);
void Test3(void);

CCrashDump dump;

static LockFreeStack<int> s_stack;
static LockFreeStack<int> s_stack2;

static LockFreePool<int> s_pool(0);

int wmain(void)
{
	//for (int i = 0; i < 100; ++i)
	//{
		Test1();
	//}
	//Test2();

	//Test3();

	return 0;
}

void Test1(void)
{
	HANDLE threads[THREAD_NUM] = { 0, };

	for (int iCnt = 0; iCnt < THREAD_NUM; ++iCnt)
	{
		threads[iCnt] = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, NULL, 0, NULL);
		if (threads[iCnt] == NULL)
		{
			__debugbreak();
			return;
		}
	}

	WaitForMultipleObjects(THREAD_NUM, threads, TRUE, INFINITE);

	for (int iCnt = 0; iCnt < THREAD_NUM; ++iCnt)
	{
		CloseHandle(threads[iCnt]);
	}

	__int64 result = s_stack._top;

	if (result != 0)
	{
		__debugbreak();
	}

	wprintf(L"Stack Top Node : 0x%p\n", result);
}

void Test2(void)
{
	HANDLE pushThreads[THREAD_NUM] = { 0, };
	HANDLE popThreads[THREAD_NUM] = { 0, };

	for (int iCnt = 0; iCnt < THREAD_NUM; ++iCnt)
	{
		pushThreads[iCnt] = (HANDLE)_beginthreadex(NULL, 0, PushProc, NULL, 0, NULL);

		if (pushThreads[iCnt] == NULL)
		{
			__debugbreak();
			return;
		}
	}

	WaitForMultipleObjects(THREAD_NUM, pushThreads, TRUE, INFINITE);

	for (int iCnt = 0; iCnt < THREAD_NUM; ++iCnt)
	{
		CloseHandle(pushThreads[iCnt]);
	}

	wprintf(L"\nPush End\n");

	for (int iCnt = 0; iCnt < THREAD_NUM; ++iCnt)
	{
		popThreads[iCnt] = (HANDLE)_beginthreadex(NULL, 0, PopProc, NULL, 0, NULL);

		if (popThreads[iCnt] == NULL)
		{
			__debugbreak();
			return;
		}
	}

	WaitForMultipleObjects(THREAD_NUM, popThreads, TRUE, INFINITE);

	for (int iCnt = 0; iCnt < THREAD_NUM; ++iCnt)
	{
		CloseHandle(popThreads[iCnt]);
	}

	wprintf(L"Stack Top Node : %p\n", s_stack2._top);

	return;
}

void Test3(void)
{
	HANDLE threads[THREAD_NUM] = { 0, };

	for (int iCnt = 0; iCnt < THREAD_NUM; ++iCnt)
	{
		threads[iCnt] = (HANDLE)_beginthreadex(NULL, 0, PoolTestProc, NULL, 0, NULL);
		if (threads[iCnt] == NULL)
		{
			__debugbreak();
			return;
		}
	}

	WaitForMultipleObjects(THREAD_NUM, threads, TRUE, INFINITE);

	for (int iCnt = 0; iCnt < THREAD_NUM; ++iCnt)
	{
		CloseHandle(threads[iCnt]);
	}

	__int64 result = s_pool._top;

	if (result != 0)
	{
		__debugbreak();
	}

	wprintf(L"Pool Top Node : 0x%p\n", result);
}


unsigned int WINAPI ThreadProc(void* arg)
{
	DWORD threadId = GetCurrentThreadId();
	wprintf(L"%d Thread Start\n", threadId);

	//for(int repeatCnt = 0; repeatCnt < REPEAT_NUM; ++repeatCnt)
	while(true)
	{
		for (int iCnt = 0; iCnt < LOOP_NUM; ++iCnt)
		{
			s_stack.Push(threadId);
		}

		for (int iCnt = 0; iCnt < LOOP_NUM; ++iCnt)
		{
			s_stack.Pop();
		}
	}

	return 0;
}

unsigned int WINAPI PushProc(void* arg)
{
	wprintf(L"%d Push Thread Start\n", GetCurrentThreadId());

	for (int repeatCnt = 0; repeatCnt < 10; ++repeatCnt)
	{
		for (int iCnt = 0; iCnt < LOOP_NUM; ++iCnt)
		{
			s_stack2.Push(iCnt);
		}
	}

	return 0;
}

unsigned int WINAPI PopProc(void* arg)
{
	wprintf(L"%d Pop Thread Start\n", GetCurrentThreadId());

	for (int repeatCnt = 0; repeatCnt < 10; ++repeatCnt)
	{
		for (int iCnt = 0; iCnt < LOOP_NUM; ++iCnt)
		{
			s_stack2.Pop();
		}
	}
	return 0;
}

unsigned int WINAPI PoolTestProc(void* arg)
{
	wprintf(L"%d Pool Test Thread Start\n", GetCurrentThreadId());

	int** arr = new int*[100000];

	for (int iCnt = 0; iCnt < LOOP_NUM; ++iCnt)
	{
		arr[iCnt] = s_pool.Alloc();
	}

	for (int repeatCnt = 0; repeatCnt < REPEAT_NUM; ++repeatCnt)
	{
		for (int iCnt = 0; iCnt < LOOP_NUM; ++iCnt)
		{
			s_pool.Free(arr[iCnt]);
		}

		for (int iCnt = 0; iCnt < LOOP_NUM; ++iCnt)
		{
			arr[iCnt] = s_pool.Alloc();
		}
	}
	
	delete[] arr;

	return 0;
}