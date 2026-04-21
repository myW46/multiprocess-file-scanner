#pragma once
#include <winsock2.h>
#include <vector>
#include <string>
#include "Stats.hpp"
#include "ConfigManager.hpp"

class Worker {
public:
	Worker(SOCKET clientSocket, Stats& stats, const std::vector<Signature>& signatures);
	~Worker();

	void run();
private:
	SOCKET s;
	Stats& globalStats;
	const std::vector<Signature>& signs;

	void analyze(const std::vector<char>& data);
	void sendResponse(const std::string& message);
};

