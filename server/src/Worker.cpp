#include "Worker.hpp"
#include <iostream>
#include <vector>
#include<set>

#include "AhoCorasick.hpp"

Worker::Worker(SOCKET clientSocket, Stats& stats, const std::vector<Signature>& signatures)
    : s(clientSocket), globalStats(stats), signs(signatures) {}
Worker::~Worker(){
    if (s != INVALID_SOCKET) {
        closesocket(s);
    }
}


void Worker::run() {
  
    int fileSize = 0;
    int bytesReceived = recv(s, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);
 

    if (bytesReceived != sizeof(fileSize)) {
        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "Failed to receive file size: " << WSAGetLastError() << std::endl;
            sendResponse("error: failed to receive file size");
        }
        else {
            sendResponse("error: incomplete file size data");
        }
        return;
    }

    std::vector<char> buffer(fileSize);
    int totalRead = 0;

    while (totalRead < fileSize) {
        bytesReceived = recv(s, buffer.data() + totalRead, fileSize - totalRead, 0);

        if (bytesReceived <= 0) {
            if (bytesReceived == SOCKET_ERROR) {
                std::cerr << "Failed to receive file data: " << WSAGetLastError() << std::endl;
                sendResponse("error: failed to receive file data");
            }
            else {
                sendResponse("error: connection closed while receiving file");
            }
            return;
        }

        totalRead += bytesReceived;
        std::cout << "Received: " << totalRead << "/" << fileSize << " bytes" << std::endl;
    }

    std::cout << "File received successfully, analyzing..." << std::endl;
    globalStats.incrementFiles();
    analyze(buffer);
}

void Worker::analyze(const std::vector<char>& data){
    AhoCorasick ac;
    ac.build(this->signs);

    std::vector<std::string> matches = ac.search(data);
    if (matches.empty()) {
        sendResponse("Clean");
        return;
    }
    std::set<std::string> uniqueThreats(matches.begin(), matches.end());
    std::string report = "Find viruses:";
    bool first = true;

    for (const auto& threatName : uniqueThreats) {
        globalStats.incrementThreat(threatName);

        if (!first) report += ", ";
        report += threatName;
        first = false;
    }
    sendResponse(report);

}

void Worker::sendResponse(const std::string& message) {
    send(s, message.c_str(), (int)message.length(), 0);
}