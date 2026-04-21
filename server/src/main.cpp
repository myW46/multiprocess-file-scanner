#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <fstream>
#include <thread>
#include <atomic>


#include "json.hpp"

#include "Stats.hpp"
#include "ConfigManager.hpp"
#include "Worker.hpp"
#include "ProcessManager.hpp"
#pragma comment(lib, "ws2_32.lib")
using json = nlohmann::json;

std::atomic<bool> g_running(true);

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    if (fdwCtrlType == CTRL_C_EVENT || fdwCtrlType == CTRL_CLOSE_EVENT) {
        std::cout << "\n[Server] shutdown signal received" << std::endl;
        g_running = false;
        return TRUE;
    }
    return FALSE;
}

void statsSendThread(Stats& stats) {
    const char* pipeName = "\\\\.\\pipe\\ScannerStatsPipe";
    while (g_running) {
        HANDLE hPipe = CreateNamedPipeA(
            pipeName,
            PIPE_ACCESS_OUTBOUND,
            PIPE_TYPE_BYTE | PIPE_WAIT,
            1, 0, 0, 0, NULL
        );
        if (hPipe == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            SharedStats current = stats.getCurrentStats();

            json j;
            j["total_files"] = current.totalFiles;
            j["total_threats"] = current.totalThreats;
            j["details"] = json::array();

            for (int i = 0; i < current.uniqueThreatCount; i++) {
                j["details"].push_back({
                    {"name",std::string(current.threats[i].name)},
                    {"count", current.threats[i].count}
                    });
            }

            std::string response = j.dump();
            DWORD written;
            WriteFile(hPipe, response.c_str(), (DWORD)response.length(), &written, NULL);
        }

        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }
    if (argc > 3 && std::string(argv[1]) == "--worker") {
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            return 1;
        }
        try {
            HANDLE hPipe = (HANDLE)(uintptr_t)std::stoull(argv[3]);
          

          
            WSAPROTOCOL_INFOA protoInfo;
            DWORD bytesRead;
            if (!ReadFile(hPipe, &protoInfo, sizeof(protoInfo), &bytesRead, NULL)) {
                DWORD error = GetLastError();
               
                CloseHandle(hPipe);
                return 1;
            }
            CloseHandle(hPipe);
            SOCKET s = WSASocketA(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, &protoInfo, 0, 0);
            if (s == INVALID_SOCKET) {
                std::cerr << "[Worker] WSASocketA failed, error: " << WSAGetLastError() << std::endl;
                return 1;
            }
            
            Stats stats(false);
            auto signatures = ConfigManager::load(argv[2]);
            Worker worker(s, stats, signatures);
            worker.run();
            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << "[Worker] Exception caught: " << e.what() << std::endl;
            return 1;
        }
        catch (...) {
            std::cerr << "[Worker] Unknown exception caught" << std::endl;
            return 1;
        }
    }
    if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
        return 1;
    }

    std::string configPath = argv[1];
    int port = (argc >= 3) ? std::stoi(argv[2]) : 8080;

    Stats* globalStats = nullptr;
    try {
        globalStats = new Stats(true);
        std::cout << "shared memmory initializer" << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    std::vector<HANDLE> workerHandles;
    std::vector<Signature> Signatures;
    
    try {
        Signatures = ConfigManager::load(configPath);
        std::cout << "[Server] config loaded. pattern nums:" << Signatures.size() << std::endl;
    }catch (const std::exception& e) { 
        std::cerr << e.what()<<std::endl;
        return 1;
    } 
    
    
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed with error: " << iResult << std::endl;
        return 1;
    }

   
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Socket error: " << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; 
    serverAddr.sin_port = htons(port);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::thread t(statsSendThread, std::ref(*globalStats));
    t.detach();


    listen(listenSocket, SOMAXCONN);
    std::cout << "Server start on port:" << port << std::endl;
    while (g_running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenSocket, &readfds);

        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(0, &readfds, NULL, NULL, &timeout);

        if (activity == SOCKET_ERROR && g_running) {
            std::cerr << "Select error: " << WSAGetLastError() << std::endl;
            break;
        }

        if (activity > 0 && FD_ISSET(listenSocket, &readfds)) {
            SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
            if (clientSocket != INVALID_SOCKET) {
                HANDLE hProcess = ProcessManager::launchWorker(clientSocket, configPath);
                if (hProcess) {
                    workerHandles.push_back(hProcess);
                }
                closesocket(clientSocket);
            }
        }
    }

    std::cout << "[Server] Waiting for workers to finish..." << std::endl;
    for (HANDLE h : workerHandles) {
        WaitForSingleObject(h, INFINITE);
        CloseHandle(h);
    }

    if (t.joinable()) t.join();

    delete globalStats;
    closesocket(listenSocket);
    WSACleanup();
    std::cout << "[Server] cleanup complete" << std::endl;

    return 0;
}

