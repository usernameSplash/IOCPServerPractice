#pragma once

#define JOB_ENQ 0xeeeeeeeeeeeeeeee
#define JOB_DEQ 0xdddddddddddddddd

typedef struct LogData
{
	__int64 index;
	__int64 threadId;
	__int64 jobType;
	__int64 nodeValue;
	__int64 TailNode;
} LogData;

#define LOG_ARRAY_LEN 100000

extern __int64 g_LogIndex;
extern LogData g_LogArray[LOG_ARRAY_LEN];