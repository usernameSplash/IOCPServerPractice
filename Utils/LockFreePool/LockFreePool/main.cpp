
#include "LockFreePool.h"
#include <cstdio>

int* arr[10000];

int wmain(void)
{
	LockFreePool<int> pool (10000);

	//for (int a = 0; a < 10; ++a)
	//{
	//	for (int iCnt = 0; iCnt < 10000; ++iCnt)
	//	{
	//		arr[iCnt] = pool.Alloc();
	//	}
	//	for (int iCnt = 0; iCnt < 10000; ++iCnt)
	//	{
	//		pool.Free(arr[iCnt]);
	//	}
	//}

	while (true)
	{
		for (int iCnt = 0; iCnt < 10000; ++iCnt)
		{
			arr[iCnt] = pool.Alloc();
		}
		for (int iCnt = 0; iCnt < 10000; ++iCnt)
		{
			pool.Free(arr[iCnt]);
		}
	}

	return 0;
}