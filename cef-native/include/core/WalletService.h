#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <winhttp.h>
#include <thread>
#include <atomic>

class WalletService {
public:
    WalletService();
    ~WalletService();

    // API Methods
    bool isHealthy();
    nlohmann::json getIdentity();
    bool markBackedUp();

    // Connection management
    bool isConnected();
    void setBaseUrl(const std::string& url);

    // Daemon process management
    bool startDaemon();
    void stopDaemon();
    bool isDaemonRunning();
    void setDaemonPath(const std::string& path);

private:
    std::string baseUrl_;
    std::string daemonPath_;
    HINTERNET hSession_;
    HINTERNET hConnect_;
    bool connected_;

    // Process management
    PROCESS_INFORMATION daemonProcess_;
    std::atomic<bool> daemonRunning_;
    std::thread monitorThread_;

    // HTTP helper methods
    nlohmann::json makeHttpRequest(const std::string& method, const std::string& endpoint, const std::string& body = "");
    bool initializeConnection();
    void cleanupConnection();
    std::string readResponse(HINTERNET hRequest);

    // Daemon management helpers
    bool createDaemonProcess();
    void monitorDaemon();
    void cleanupDaemonProcess();

    // Console control handler
    static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType);
};
