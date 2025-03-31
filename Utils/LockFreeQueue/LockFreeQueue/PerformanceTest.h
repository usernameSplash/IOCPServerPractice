#pragma once

#include "LockFreeQueue.h"

#include <Windows.h>
#include <process.h>
#include <queue>

#pragma comment(lib, "winmm.lib")

#define THREAD_NUM 10
#define REPEAT_NUM 3000000
#define LOOP_NUM 3

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

unsigned int WINAPI LockFreeQueueTest(void* arg);
unsigned int WINAPI LockQueueTest(void* arg);

void StartTest(void);