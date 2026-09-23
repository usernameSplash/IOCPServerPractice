#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <Windows.h>
#include <WinSock2.h>
#include <iphlpapi.h>
#include <psapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")

class SystemMonitor
{
public:
	SystemMonitor(void)
	{
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		_numOfProcessors = systemInfo.dwNumberOfProcessors;
		_process = GetCurrentProcess();

		_lastProcessTime.QuadPart = 0;
		_lastProcessKernel.QuadPart = 0;
		_lastProcessUser.QuadPart = 0;
		_lastTotalIdle.QuadPart = 0;
		_lastTotalKernel.QuadPart = 0;
		_lastTotalUser.QuadPart = 0;

		Update();
	}

	void Update(void)
	{
		UpdateProcessCPU();
		UpdateTotalCPU();
		UpdateMemory();
		UpdateNetwork();
	}

public:
	inline float GetProcessCPU(void) const { return _processCPU; }
	inline float GetProcessKernelCPU(void) const { return _processKernelCPU; }
	inline float GetProcessUserCPU(void) const { return _processUserCPU; }

	inline float GetTotalCPU(void) const { return _totalCPU; }
	inline float GetTotalKernelCPU(void) const { return _totalKernelCPU; }
	inline float GetTotalUserCPU(void) const { return _totalUserCPU; }

	inline long GetProcessMemoryMB(void) const { return _processMemoryMB; }
	inline long GetNonpagedMB(void) const { return _nonpagedMB; }
	inline long GetAvailableMB(void) const { return _availableMB; }
	inline long GetCommitMB(void) const { return _commitMB; }

	inline long GetNetworkRecvKBps(void) const { return _recvKBps; }
	inline long GetNetworkSendKBps(void) const { return _sendKBps; }

private:
	void UpdateProcessCPU(void)
	{
		ULARGE_INTEGER nowTime;
		ULARGE_INTEGER creationTime;
		ULARGE_INTEGER exitTime;
		ULARGE_INTEGER kernelTime;
		ULARGE_INTEGER userTime;

		GetSystemTimeAsFileTime((LPFILETIME)&nowTime);
		if (GetProcessTimes(_process, (LPFILETIME)&creationTime, (LPFILETIME)&exitTime, (LPFILETIME)&kernelTime, (LPFILETIME)&userTime) == FALSE)
		{
			return;
		}

		if (_lastProcessTime.QuadPart != 0)
		{
			ULONGLONG timeDiff = nowTime.QuadPart - _lastProcessTime.QuadPart;
			ULONGLONG kernelDiff = kernelTime.QuadPart - _lastProcessKernel.QuadPart;
			ULONGLONG userDiff = userTime.QuadPart - _lastProcessUser.QuadPart;

			if (timeDiff > 0)
			{
				_processCPU = (float)((kernelDiff + userDiff) / (double)_numOfProcessors / (double)timeDiff * 100.0);
				_processKernelCPU = (float)(kernelDiff / (double)_numOfProcessors / (double)timeDiff * 100.0);
				_processUserCPU = (float)(userDiff / (double)_numOfProcessors / (double)timeDiff * 100.0);
			}
		}

		_lastProcessTime = nowTime;
		_lastProcessKernel = kernelTime;
		_lastProcessUser = userTime;
	}

	void UpdateTotalCPU(void)
	{
		ULARGE_INTEGER idleTime;
		ULARGE_INTEGER kernelTime;
		ULARGE_INTEGER userTime;

		if (GetSystemTimes((LPFILETIME)&idleTime, (LPFILETIME)&kernelTime, (LPFILETIME)&userTime) == FALSE)
		{
			return;
		}

		if (_lastTotalKernel.QuadPart != 0)
		{
			ULONGLONG idleDiff = idleTime.QuadPart - _lastTotalIdle.QuadPart;
			ULONGLONG kernelDiff = kernelTime.QuadPart - _lastTotalKernel.QuadPart;
			ULONGLONG userDiff = userTime.QuadPart - _lastTotalUser.QuadPart;
			ULONGLONG total = kernelDiff + userDiff;

			if (total > 0)
			{
				_totalCPU = (float)((double)(total - idleDiff) / total * 100.0);
				_totalKernelCPU = (float)((double)(kernelDiff - idleDiff) / total * 100.0);
				_totalUserCPU = (float)((double)userDiff / total * 100.0);
			}
		}

		_lastTotalIdle = idleTime;
		_lastTotalKernel = kernelTime;
		_lastTotalUser = userTime;
	}

	void UpdateMemory(void)
	{
		PROCESS_MEMORY_COUNTERS_EX processMemory;
		processMemory.cb = sizeof(processMemory);
		if (GetProcessMemoryInfo(_process, (PROCESS_MEMORY_COUNTERS*)&processMemory, sizeof(processMemory)))
		{
			_processMemoryMB = (long)(processMemory.PrivateUsage >> 20);
		}

		PERFORMANCE_INFORMATION performanceInfo;
		performanceInfo.cb = sizeof(performanceInfo);
		if (GetPerformanceInfo(&performanceInfo, sizeof(performanceInfo)))
		{
			_nonpagedMB = (long)((performanceInfo.KernelNonpaged * performanceInfo.PageSize) >> 20);
			_availableMB = (long)((performanceInfo.PhysicalAvailable * performanceInfo.PageSize) >> 20);
			_commitMB = (long)((performanceInfo.CommitTotal * performanceInfo.PageSize) >> 20);
		}
	}

	void UpdateNetwork(void)
	{
		PMIB_IF_TABLE2 table = nullptr;
		if (GetIfTable2(&table) != NO_ERROR)
		{
			return;
		}

		ULONG64 inOctets = 0;
		ULONG64 outOctets = 0;

		for (ULONG iCnt = 0; iCnt < table->NumEntries; ++iCnt)
		{
			const MIB_IF_ROW2& row = table->Table[iCnt];

			if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK)
			{
				continue;
			}

			if (row.InterfaceAndOperStatusFlags.HardwareInterface == FALSE)
			{
				continue;
			}

			if (row.OperStatus != IfOperStatusUp)
			{
				continue;
			}

			inOctets += row.InOctets;
			outOctets += row.OutOctets;
		}

		FreeMibTable(table);

		ULONGLONG nowTick = GetTickCount64();

		if (_lastNetworkTick != 0 && nowTick > _lastNetworkTick)
		{
			double seconds = (nowTick - _lastNetworkTick) / 1000.0;
			_recvKBps = (long)((inOctets - _lastInOctets) / 1024.0 / seconds);
			_sendKBps = (long)((outOctets - _lastOutOctets) / 1024.0 / seconds);
		}

		_lastInOctets = inOctets;
		_lastOutOctets = outOctets;
		_lastNetworkTick = nowTick;
	}

private:
	HANDLE _process = NULL;
	int _numOfProcessors = 1;

	ULARGE_INTEGER _lastProcessTime;
	ULARGE_INTEGER _lastProcessKernel;
	ULARGE_INTEGER _lastProcessUser;

	ULARGE_INTEGER _lastTotalIdle;
	ULARGE_INTEGER _lastTotalKernel;
	ULARGE_INTEGER _lastTotalUser;

	float _processCPU = 0.0f;
	float _processKernelCPU = 0.0f;
	float _processUserCPU = 0.0f;

	float _totalCPU = 0.0f;
	float _totalKernelCPU = 0.0f;
	float _totalUserCPU = 0.0f;

	long _processMemoryMB = 0;
	long _nonpagedMB = 0;
	long _availableMB = 0;
	long _commitMB = 0;

	ULONG64 _lastInOctets = 0;
	ULONG64 _lastOutOctets = 0;
	ULONGLONG _lastNetworkTick = 0;
	long _recvKBps = 0;
	long _sendKBps = 0;
};
