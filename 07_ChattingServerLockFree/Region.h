#pragma once

#include "ContentsProtocol.h"

#include <vector>
#include <synchapi.h>

using namespace std;

class Player;
class Region
{
public:
	Region()
	{
		InitializeSRWLock(&_lock);
	}

public:
	vector<Region*> _aroundRegions;
	vector<Player*> _players;
	WORD _x;
	WORD _y;
	SRWLOCK _lock;
};
