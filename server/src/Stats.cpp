#include "Stats.hpp"
#include <iostream>

const char* MAP_NAME = "Local\\ScannerSharedStats";
const char* MUTEX_NAME = "Local\\ScannerStatsMutex";

Stats::Stats(bool isServer) {
	if (isServer) {
		hMapFile = CreateFileMapping(
			INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
			sizeof(SharedStats),
			MAP_NAME
		);

		hMutex = CreateMutexA(NULL, FALSE, MUTEX_NAME);

	}
	else {
		hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MAP_NAME);
		hMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, MUTEX_NAME);
	}

	if (!hMapFile || !hMutex) {
		throw std::runtime_error("failed to initialize shared memory");
	}

	pStats =(SharedStats*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedStats));
	if (!pStats) {
		CloseHandle(hMapFile);
		CloseHandle(hMutex);
		throw std::runtime_error("Failed to map view of file");
	}
	if (isServer) {
		memset(pStats, 0, sizeof(SharedStats));
	}

}

void Stats::lock() { WaitForSingleObject(hMutex, INFINITE); }
void Stats::unlock() { ReleaseMutex(hMutex);}

void Stats::incrementThreat(const std::string& threatName) {
	lock();
	pStats->totalThreats++;

	bool found = false;

	for (int i = 0; i < pStats->uniqueThreatCount; i++) {
		if (strcmp(pStats->threats[i].name, threatName.c_str()) == 0) {
			pStats->threats[i].count++;
			found = true;
			break;
		}
	}
	if (!found && pStats->uniqueThreatCount < 100) {
		int idx = pStats->uniqueThreatCount;
		strncpy_s(pStats->threats[idx].name, threatName.c_str(), 256);
		pStats->threats[idx].count=1;
		pStats->uniqueThreatCount++;
	}
	unlock();
}
void Stats::incrementFiles() {
	lock();
	pStats->totalFiles++;
	unlock();
}

SharedStats Stats::getCurrentStats() {
	lock();
	SharedStats copy = *pStats;
	unlock();
	return copy;
}

Stats::~Stats() {
	if (pStats) UnmapViewOfFile(pStats);
	if (hMapFile) CloseHandle(hMapFile);
	if (hMutex) CloseHandle(hMutex);
}
