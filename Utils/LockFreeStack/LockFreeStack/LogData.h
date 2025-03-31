#pragma once

#include <Windows.h>

#define LOG_ARRAY_LEN 100000
#define JOB_PUSH 0
#define JOB_POP -1
#define JOB_PUSH_ENTER 0xeeeeeeeeeeeeeeee
#define JOB_POP_ENTER  0xdddddddddddddddd

struct LogData
{
	LONG64 _idx;
	LONG64 _threadId;
	LONG64 _jobType;
	void* _nodePtr;
	void* _nextPtr;
};

extern volatile __int64 g_pushCnt;
extern volatile __int64 g_popCnt;

extern volatile __int64 g_logIndex;
extern LogData g_logArray[];