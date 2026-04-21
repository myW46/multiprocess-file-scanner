#pragma once
#include <winsock2.h>
#include <windows.h>

#include <vector>
#include <string>
#include <mutex>

class ProcessManager {
public:
	static HANDLE launchWorker(SOCKET clientSocket, const std::string& configPath);
	static void waitForAll();
	static void cleanupFinished();
private:
	static std::vector<HANDLE> workerHandles;
	static std::mutex vectorMutex;
};
