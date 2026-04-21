#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cerr << "there are not enough parameters" << std::endl;
		return 1;
	}
	std::string filePath = argv[1];
	int port = std::stoi(argv[2]);

	std::ifstream file(filePath, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::cerr << "cannot open file" << std::endl;
		return 1;
	}
	std::streamsize fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(static_cast<size_t>(fileSize));
	if (!file.read(buffer.data(), fileSize)) {
		std::cerr << "failed to read file content" << std::endl;
	}
	file.close();

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed." << std::endl;
		return 1;
	}
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		std::cerr << "creation failed" << std::endl;
		WSACleanup();
		return 1;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(static_cast<unsigned short>(port));
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

	std::cout << "connect to server on port:" << port << "..." << std::endl;
	if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "connect failed" << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	std::cout << "connext success" << std::endl;

	int sendSize = static_cast<int>(fileSize);
	
	if (send(sock, reinterpret_cast<char*>(&sendSize), sizeof(sendSize), 0) == SOCKET_ERROR) {
		std::cerr << "failed sent" << std::endl;
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	int totalSent = 0;
	char* dataPtr = buffer.data();
	
	while (totalSent < sendSize) {
		int res = send(sock, dataPtr + totalSent, sendSize - totalSent, 0);

		if (res == SOCKET_ERROR) {
			std::cerr << "failed sent part with error" << WSAGetLastError() << std::endl;
			closesocket(sock);
			WSACleanup();
			return 1;
		}

		totalSent += res;
		std::cout << "Progress sent:" << totalSent << "/" << sendSize << std::endl;
	}
	
	std::cout << "file sent success< start analyzing" << std::endl;

	char result[256] = { 0 };
	int bytesReceived = recv(sock, result, sizeof(result) - 1, 0);
	if (bytesReceived > 0) {
		std::cout << result << std::endl;
	}
	else {
		std::cout << "connection corrupted from server side" << std::endl;
	}

	closesocket(sock);
	WSACleanup();

	std::cout << "Press enter to exit..." << std::endl;

	std::cin.get();
	return 0;
}