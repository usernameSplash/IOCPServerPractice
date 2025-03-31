
#include "PerformanceTest.h"

LockFreeQueue<int> g_queue;

unsigned int WINAPI EnqueueTest(void* arg);
unsigned int WINAPI DequeueTest(void* arg);

int wmain(void)
{
	//StartTest();

	HANDLE enqueueThreads[3];
	HANDLE dequeueThreads[3];

	for (int iCnt = 0; iCnt < 3; ++iCnt)
	{
		enqueueThreads[iCnt] = (HANDLE)_beginthreadex(NULL, 0, EnqueueTest, (void*)iCnt, 0, NULL);
	}

	for (int iCnt = 0; iCnt < 3; ++iCnt)
	{
		dequeueThreads[iCnt] = (HANDLE)_beginthreadex(NULL, 0, DequeueTest, NULL, 0, NULL);
	}

	WaitForMultipleObjects(3, dequeueThreads, TRUE, INFINITE);

	return 0;
}

unsigned int WINAPI EnqueueTest(void* arg)
{
	int index = (int)arg;

	index += 1;

	for (int iCnt = 0; iCnt < 10000; ++iCnt)
	{
		g_queue.Enqueue(index);
	}

	g_queue.Enqueue(-1);

	return 0;
}

unsigned int WINAPI DequeueTest(void* arg)
{
	int sum = 0;

	while(true)
	{
		int x = g_queue.Dequeue();
		
		if (x == -1)
		{
			break;
		}

		sum += x;
	}

	wprintf(L"Thread %d - sum : %d\n", GetCurrentThreadId(), sum);

	return 0;
}