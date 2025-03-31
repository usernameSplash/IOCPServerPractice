#pragma once

#include "LockFreeQueue.h"

#include <Windows.h>
#include <process.h>
#include <queue>

#pragma comment(lib, "winmm.lib")

#define THREAD_NUM 10
#define REPEAT_NUM 3000000
#define LOOP_NUM 3

unsigned int WINAPI LockFreeQueueTest(void* arg);
unsigned int WINAPI LockQueueTest(void* arg);

void StartTest(void);