#pragma once

#include <Windows.h>

#define LOG_ARRAY_LEN 100000
#define JOB_PUSH 0
#define JOB_POP -1

struct LogData
{
	LONG64 _idx;
	LONG64 _threadId;
	LONG64 _jobType;
	void* _nodePtr;
	void* _nextPtr;
};

extern long g_pushCnt;
extern long g_popCnt;

extern long g_logIndex;
extern LogData g_logArray[];