#include <windows.h>
#include <iostream>
#include <string>
#include "json.hpp"

using json = nlohmann::json;

const char* RESET = "\033[0m";
const char* BOLD = "\033[1m";
const char* RED = "\033[31m";
const char* GREEN = "\033[32m";
const char* CYAN = "\033[36m";


void print_border(int width) {
	std::cout << "+" << std::string(width, '-') << "+" << std::endl;


}
int main() {
	const char* pipeName = "\\\\.\\pipe\\ScannerStatsPipe";

	HANDLE hPipe = CreateFileA(
		pipeName,
		GENERIC_READ,
		0, NULL,
		OPEN_EXISTING,
		0, NULL
		);
	if (hPipe == INVALID_HANDLE_VALUE) {
		std::cerr <<RED<< "Error:"<< RESET << "Could not connected to stats pipe";
		return 1;
	}
	char buffer[4096];
	DWORD bytesRead;
	if (!ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
		std::cerr << RED << "Error: " << RESET << "Failed to read data from Pipe." << std::endl;
		CloseHandle(hPipe);
		return 1;
	}
	buffer[bytesRead] = '\0';
	CloseHandle(hPipe);
	try {
		auto data = json::parse(buffer);
		std::cout << "\n" << BOLD << CYAN << "CURRENT STATISTICS" << RESET << "\n";
		print_border(40);
		std::cout << "Total files scanned:" << std::setw(20) << data["total_files"].get<int>() << std::endl;
		int threats = data["total_threats"].get<int>();
		std::cout << "Threat Detected:    " << (threats > 0 ? RED : GREEN)
			<< std::setw(20) << threats << RESET << std::endl;

		print_border(40);
		std::cout << "Threat by types:" << std::endl;
		if(data.contains("details") && data["details"].is_array()) {
			for (const auto& item : data["details"]) {
				std::cout << "" << std::left << std::setw(25) << item["name"].get<std::string>()
					<< ":" << std::right << std::setw(11) << item["count"].get<int>() << std::endl;
			}
		}
		else {
			std::cout << "No specific threats recorded" << std::endl;
		}
		print_border(40);
		std::cout << std::endl;
	}
	catch (const json::parse_error& e) {
		std::cerr << RED << "JSON Parse Error: " << RESET << e.what() << std::endl;
	}
	return 0;
}