#pragma once
#include <windows.h>
#include <string>
#include <vector>

struct Threat {
	char name[256];
	int count;
};

struct SharedStats {
	long long totalFiles;
	long long totalThreats;
	int uniqueThreatCount;
	Threat threats[100];

};

class Stats {
public:
	Stats(bool isServer);
	~Stats();

	void incrementFiles();
	void incrementThreat(const std::string& threatName);

	SharedStats getCurrentStats();

private:
	HANDLE hMapFile;
	HANDLE hMutex;
	SharedStats* pStats;

	void lock();
	void unlock();
};