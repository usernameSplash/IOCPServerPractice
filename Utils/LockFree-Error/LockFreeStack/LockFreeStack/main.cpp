#include "LogData.h"
#include "LockFreeStack.h"

#include <cstdio>
#include <Windows.h>
#include <process.h>

#define THREAD_NUM 10
#define LOOP_NUM 100000

unsigned int WINAPI ThreadProc(void* arg);

unsigned int WINAPI PushProc(void* arg);
unsigned int WINAPI PopProc(void* arg);

void Test1(void);
void Test2(void);


static LockFreeStack<int> s_stack;
static LockFreeStack<int> s_stack2;

int wmain(void)
{
	Test1();
	//Test2();

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

	wprintf(L"Stack Top Node : %p\n", s_stack._top);
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


unsigned int WINAPI ThreadProc(void* arg)
{
	wprintf(L"%d Thread Start\n", GetCurrentThreadId());

	for(int repeatCnt = 0; repeatCnt < 10000; ++repeatCnt)
	{
		for (int iCnt = 0; iCnt < LOOP_NUM; ++iCnt)
		{
			s_stack.Push(iCnt);
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