#include "LogData.h"

volatile __int64 g_pushCnt = 0;
volatile __int64 g_popCnt = 0;

volatile __int64 g_logIndex = 0;
LogData g_logArray[LOG_ARRAY_LEN];