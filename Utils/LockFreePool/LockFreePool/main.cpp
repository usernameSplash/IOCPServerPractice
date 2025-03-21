
#include "LockFreePool.h"
#include <cstdio>

int wmain(void)
{
	LockFreePool<int> pool (1);

	auto a = pool.Alloc();
	int* b = pool.Alloc();
	*a = 1;
	*b = 2;

	wprintf(L"a: %d\n", *a);
	wprintf(L"b: %d\n", *b);

	pool.Free(a);
	pool.Free(b);

	int* c = pool.Alloc();
	int* d = pool.Alloc();

	wprintf(L"c: %d\n", *c);
	wprintf(L"d: %d\n", *d);

	return 0;
}