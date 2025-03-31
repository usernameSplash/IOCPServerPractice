#include "PerformanceTest.h"

static LockFreeQueue<int> s_lockFreeQueue;
static std::queue<int> s_lockQueue;
static LARGE_INTEGER s_freq;
static SRWLOCK s_queueLock;

static double s_LockFreeTotalDuration[THREAD_NUM] = { 0, };
static double s_LockFreeEnqueueDuration[THREAD_NUM] = { 0, };
static double s_LockFreeDequeueDuration[THREAD_NUM] = { 0, };

static double s_LockTotalDuration[THREAD_NUM] = { 0, };
static double s_LockEnqueueDuration[THREAD_NUM] = { 0, };
static double s_LockDequeueDuration[THREAD_NUM] = { 0, };

unsigned int WINAPI LockFreeQueueTest(void* arg)
{
	LARGE_INTEGER total_start;
	LARGE_INTEGER total_end;
	LARGE_INTEGER start;
	LARGE_INTEGER end;

	int index = (int)arg;
	
	QueryPerformanceCounter(&total_start);

	for (int iCnt = 0; iCnt < REPEAT_NUM; ++iCnt)
	{
		for (int jCnt = 0; jCnt < LOOP_NUM; ++jCnt)
		{
			QueryPerformanceCounter(&start);
			s_lockFreeQueue.Enqueue(index);
			QueryPerformanceCounter(&end);

			s_LockFreeEnqueueDuration[index] += (end.QuadPart - start.QuadPart) / (double)s_freq.QuadPart;
		}

		for (int jCnt = 0; jCnt < LOOP_NUM; ++jCnt)
		{
			QueryPerformanceCounter(&start);
			s_lockFreeQueue.Dequeue();
			QueryPerformanceCounter(&end);
			s_LockFreeDequeueDuration[index] += (end.QuadPart - start.QuadPart) / (double)s_freq.QuadPart;
		}
	}

	QueryPerformanceCounter(&total_end);

	s_LockFreeTotalDuration[index] = (total_end.QuadPart - total_start.QuadPart) / (double)s_freq.QuadPart;

	return 0;
}

unsigned int WINAPI LockQueueTest(void* arg)
{
	LARGE_INTEGER total_start;
	LARGE_INTEGER total_end;
	LARGE_INTEGER start;
	LARGE_INTEGER end;

	int index = (int)arg;

	QueryPerformanceCounter(&total_start);

	for (int iCnt = 0; iCnt < REPEAT_NUM; ++iCnt)
	{
		for (int jCnt = 0; jCnt < LOOP_NUM; ++jCnt)
		{
			QueryPerformanceCounter(&start);
			AcquireSRWLockExclusive(&s_queueLock);
			s_lockQueue.push(index);
			ReleaseSRWLockExclusive(&s_queueLock);
			QueryPerformanceCounter(&end);

			s_LockEnqueueDuration[index] += (end.QuadPart - start.QuadPart) / (double)s_freq.QuadPart;
		}

		for (int jCnt = 0; jCnt < LOOP_NUM; ++jCnt)
		{
			QueryPerformanceCounter(&start);
			AcquireSRWLockExclusive(&s_queueLock);
			s_lockQueue.pop();
			ReleaseSRWLockExclusive(&s_queueLock);
			QueryPerformanceCounter(&end);
			s_LockDequeueDuration[index] += (end.QuadPart - start.QuadPart) / (double)s_freq.QuadPart;
		}
	}

	QueryPerformanceCounter(&total_end);

	s_LockTotalDuration[index] = (total_end.QuadPart - total_start.QuadPart) / (double)s_freq.QuadPart;

	return 0;
}

void StartTest(void)
{
	QueryPerformanceFrequency(&s_freq);

	InitializeSRWLock(&s_queueLock);

	HANDLE lockFreeThreads[THREAD_NUM];
	HANDLE lockThreads[THREAD_NUM];

	for (int iCnt = 0; iCnt < THREAD_NUM; ++iCnt)
	{
		lockFreeThreads[iCnt] = (HANDLE)_beginthreadex(NULL, 0, LockFreeQueueTest, (void*)iCnt, 0, NULL);
	}

	WaitForMultipleObjects(THREAD_NUM, lockFreeThreads, TRUE, INFINITE);

	for (int iCnt = 0; iCnt < THREAD_NUM; ++iCnt)
	{
		lockThreads[iCnt] = (HANDLE)_beginthreadex(NULL, 0, LockQueueTest, (void*)iCnt, 0, NULL);
	}

	WaitForMultipleObjects(THREAD_NUM, lockThreads, TRUE, INFINITE);

	double lockFreeTotalAvg = 0;
	double lockFreeEnqAvg = 0;
	double lockFreeDeqAvg = 0;
	double lockTotalAvg = 0;
	double lockEnqAvg = 0;
	double lockDeqAvg = 0;

	for (int iCnt = 0; iCnt < THREAD_NUM; ++iCnt)
	{
		lockFreeTotalAvg += s_LockFreeTotalDuration[iCnt];
		lockFreeEnqAvg += s_LockFreeEnqueueDuration[iCnt];
		lockFreeDeqAvg += s_LockFreeDequeueDuration[iCnt];

		lockTotalAvg += s_LockTotalDuration[iCnt];
		lockEnqAvg += s_LockEnqueueDuration[iCnt];
		lockDeqAvg += s_LockDequeueDuration[iCnt];
	}

	lockFreeTotalAvg /= THREAD_NUM;
	lockFreeEnqAvg /= THREAD_NUM;
	lockFreeDeqAvg /= THREAD_NUM;
	lockTotalAvg /= THREAD_NUM;
	lockEnqAvg /= THREAD_NUM;
	lockDeqAvg /= THREAD_NUM;

	wprintf(L"<Lock Free Queue>\n\n");
	wprintf(L"Total Duration : %lf\n", lockFreeTotalAvg);
	wprintf(L"Enqueue Duration : %lf\n", lockFreeEnqAvg);
	wprintf(L"Dequeue Duration : %lf\n\n\n", lockFreeDeqAvg);

	wprintf(L"<Lock Queue>\n\n");
	wprintf(L"Total Duration : %lf\n", lockTotalAvg);
	wprintf(L"Enqueue Duration : %lf\n", lockEnqAvg);
	wprintf(L"Dequeue Duration : %lf\n\n\n", lockDeqAvg);
}