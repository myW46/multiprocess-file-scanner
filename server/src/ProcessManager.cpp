#include "ProcessManager.hpp"
#include <iostream>
#include <sstream>

std::vector<HANDLE> ProcessManager::workerHandles;
std::mutex ProcessManager::vectorMutex;

HANDLE ProcessManager::launchWorker(SOCKET clientSocket, const std::string& configPath) {

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        std::cerr << "[ProcessManager] CreatePipe failed, error: " << GetLastError() << std::endl;
        return NULL;
    }
    SetHandleInformation(hWritePipe, HANDLE_FLAG_INHERIT, 0);
 

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
   

    std::stringstream ss;
    ss << "\"" << exePath << "\" --worker \"" << configPath << "\" " << (uintptr_t)hReadPipe;
    std::string cmd = ss.str();

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hReadPipe;
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    std::cout << "[ProcessManager] Creating worker process..." << std::endl;
    if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        DWORD error = GetLastError();
        std::cerr << "[ProcessManager] CreateProcessA failed, error: " << error << std::endl;

        LPSTR messageBuffer = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
        std::cerr << "[ProcessManager] Error details: " << messageBuffer << std::endl;
        LocalFree(messageBuffer);

        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return NULL;
    }
    
    WSAPROTOCOL_INFOA protocolInfo;
    if (WSADuplicateSocketA(clientSocket, pi.dwProcessId, &protocolInfo) == SOCKET_ERROR) {
        DWORD error = WSAGetLastError();
        std::cerr << "[ProcessManager] WSADuplicateSocketA failed, error: " << error << std::endl;

        LPSTR messageBuffer = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
        std::cerr << "[ProcessManager] Winsock error details: " << messageBuffer << std::endl;
        LocalFree(messageBuffer);

        TerminateProcess(pi.hProcess, 1);
        //CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return pi.hProcess;
    }

    DWORD written;

    if (!WriteFile(hWritePipe, &protocolInfo, sizeof(protocolInfo), &written, NULL)) {
        DWORD error = GetLastError();
        std::cerr << "[ProcessManager] WriteFile failed, error: " << error << std::endl;

        LPSTR messageBuffer = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
        std::cerr << "[ProcessManager] WriteFile error details: " << messageBuffer << std::endl;
        LocalFree(messageBuffer);

        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }

    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    CloseHandle(pi.hThread);

    closesocket(clientSocket);
   
    std::lock_guard<std::mutex> lock(vectorMutex);
    workerHandles.push_back(pi.hProcess);

    return pi.hProcess;
}

void ProcessManager::waitForAll() {
	std::lock_guard<std::mutex> lock(vectorMutex);
	if (workerHandles.empty()) return;

	std::cout << "[parrent] waiting for" << workerHandles.size() << "workers to finished" << std::endl;
	WaitForMultipleObjects((DWORD)workerHandles.size(), workerHandles.data(), TRUE, INFINITE);

	for (HANDLE h : workerHandles) CloseHandle(h);
	workerHandles.clear();
}
