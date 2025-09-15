#include "../../include/core/WalletService.h"
#include <iostream>
#include <sstream>

// Static instance for console handler
static WalletService* g_walletService = nullptr;

WalletService::WalletService()
    : baseUrl_("http://localhost:8080")
    , daemonPath_("")
    , hSession_(nullptr)
    , hConnect_(nullptr)
    , connected_(false)
    , daemonRunning_(false) {

    // Set global instance for console handler
    g_walletService = this;

    // Initialize daemon process info
    ZeroMemory(&daemonProcess_, sizeof(PROCESS_INFORMATION));

    // Set default daemon path (relative to executable)
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string exeDir = std::string(exePath);
    size_t lastSlash = exeDir.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        exeDir = exeDir.substr(0, lastSlash);
        daemonPath_ = exeDir + "\\..\\..\\..\\..\\go-wallet\\wallet.exe";
    }

    std::cout << "🚀 WalletService initialized" << std::endl;
    std::cout << "📍 Daemon path: " << daemonPath_ << std::endl;

    // Check if daemon file exists
    if (GetFileAttributesA(daemonPath_.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::cerr << "❌ Go daemon executable not found at: " << daemonPath_ << std::endl;
        std::cerr << "❌ Error code: " << GetLastError() << std::endl;
    } else {
        std::cout << "✅ Go daemon executable found" << std::endl;
    }

    // Set up console control handler
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    // Try to connect to existing daemon first
    if (initializeConnection()) {
        std::cout << "✅ Connected to existing Go daemon" << std::endl;
    } else {
        // If connection fails, start our own daemon
        std::cout << "⚠️ No existing daemon found, starting new one..." << std::endl;
        if (startDaemon()) {
            // Wait a moment for daemon to start
            Sleep(2000);
            initializeConnection();
        }
    }
}

WalletService::~WalletService() {
    std::cout << "🛑 WalletService destructor called - shutting down daemon..." << std::endl;
    stopDaemon();
    cleanupConnection();
}

bool WalletService::initializeConnection() {
    // Initialize WinHTTP session
    hSession_ = WinHttpOpen(L"BitcoinBrowser/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS,
                           0);

    if (!hSession_) {
        std::cerr << "❌ Failed to initialize WinHTTP session. Error: " << GetLastError() << std::endl;
        return false;
    }

    // Parse URL
    URL_COMPONENTS urlComp = {0};
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwSchemeLength = -1;
    urlComp.dwHostNameLength = -1;
    urlComp.dwUrlPathLength = -1;
    urlComp.dwExtraInfoLength = -1;

    std::wstring wideUrl(baseUrl_.begin(), baseUrl_.end());
    if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &urlComp)) {
        std::cerr << "❌ Failed to parse URL: " << baseUrl_ << std::endl;
        return false;
    }

    // Extract hostname and port
    std::wstring hostname(urlComp.lpszHostName, urlComp.dwHostNameLength);
    INTERNET_PORT port = urlComp.nPort;
    if (port == 0) {
        port = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? 443 : 80;
    }

    // Connect to server
    hConnect_ = WinHttpConnect(hSession_, hostname.c_str(), port, 0);
    if (!hConnect_) {
        std::cerr << "❌ Failed to connect to Go daemon at " << baseUrl_ << std::endl;
        return false;
    }

    connected_ = true;
    std::cout << "✅ Connected to Go wallet daemon at " << baseUrl_ << std::endl;
    return true;
}

void WalletService::cleanupConnection() {
    if (hConnect_) {
        WinHttpCloseHandle(hConnect_);
        hConnect_ = nullptr;
    }
    if (hSession_) {
        WinHttpCloseHandle(hSession_);
        hSession_ = nullptr;
    }
    connected_ = false;
}

bool WalletService::isConnected() {
    return connected_;
}

void WalletService::setBaseUrl(const std::string& url) {
    if (baseUrl_ != url) {
        cleanupConnection();
        baseUrl_ = url;
        initializeConnection();
    }
}

nlohmann::json WalletService::makeHttpRequest(const std::string& method, const std::string& endpoint, const std::string& body) {
    if (!connected_) {
        std::cerr << "❌ Not connected to Go daemon" << std::endl;
        return nlohmann::json::object();
    }

    // Convert endpoint to wide string
    std::wstring wideEndpoint(endpoint.begin(), endpoint.end());

    // Create request
    HINTERNET hRequest = WinHttpOpenRequest(hConnect_,
                                           std::wstring(method.begin(), method.end()).c_str(),
                                           wideEndpoint.c_str(),
                                           nullptr,
                                           WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES,
                                           0);

    if (!hRequest) {
        std::cerr << "❌ Failed to create HTTP request. Error: " << GetLastError() << std::endl;
        return nlohmann::json::object();
    }

    // Set headers
    std::string contentType = "application/json";
    std::wstring wideContentType(contentType.begin(), contentType.end());
    WinHttpAddRequestHeaders(hRequest,
                           std::wstring(L"Content-Type: " + wideContentType).c_str(),
                           -1,
                           WINHTTP_ADDREQ_FLAG_ADD);

    // Send request
    BOOL result = WinHttpSendRequest(hRequest,
                                   WINHTTP_NO_ADDITIONAL_HEADERS,
                                   0,
                                   body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.c_str(),
                                   body.length(),
                                   body.length(),
                                   0);

    if (!result) {
        std::cerr << "❌ Failed to send HTTP request. Error: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hRequest);
        return nlohmann::json::object();
    }

    // Receive response
    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        std::cerr << "❌ Failed to receive HTTP response. Error: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hRequest);
        return nlohmann::json::object();
    }

    // Read response body
    std::string responseBody = readResponse(hRequest);
    WinHttpCloseHandle(hRequest);

    // Parse JSON response
    try {
        return nlohmann::json::parse(responseBody);
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to parse JSON response: " << e.what() << std::endl;
        std::cerr << "Response body: " << responseBody << std::endl;
        return nlohmann::json::object();
    }
}

std::string WalletService::readResponse(HINTERNET hRequest) {
    std::string response;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;

    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
            break;
        }

        if (dwSize == 0) {
            break;
        }

        std::vector<char> buffer(dwSize + 1);
        if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
            break;
        }

        response.append(buffer.data(), dwDownloaded);
    } while (dwSize > 0);

    return response;
}

bool WalletService::isHealthy() {
    std::cout << "🔍 Checking Go daemon health..." << std::endl;

    auto response = makeHttpRequest("GET", "/health");

    if (response.contains("status") && response["status"] == "healthy") {
        std::cout << "✅ Go daemon is healthy" << std::endl;
        return true;
    } else {
        std::cerr << "❌ Go daemon health check failed" << std::endl;
        return false;
    }
}

nlohmann::json WalletService::getIdentity() {
    std::cout << "🔍 Getting identity from Go daemon..." << std::endl;

    auto response = makeHttpRequest("GET", "/identity/get");

    if (response.contains("address")) {
        std::cout << "✅ Identity retrieved successfully" << std::endl;
        std::cout << "📍 Address: " << response["address"].get<std::string>() << std::endl;
        return response;
    } else {
        std::cerr << "❌ Failed to get identity from Go daemon" << std::endl;
        return nlohmann::json::object();
    }
}

bool WalletService::markBackedUp() {
    std::cout << "🔍 Marking wallet as backed up..." << std::endl;

    auto response = makeHttpRequest("POST", "/identity/markBackedUp");

    if (response.contains("success") && response["success"] == true) {
        std::cout << "✅ Wallet marked as backed up successfully" << std::endl;
        return true;
    } else {
        std::cerr << "❌ Failed to mark wallet as backed up" << std::endl;
        return false;
    }
}

nlohmann::json WalletService::generateAddress() {
    std::cout << "🔍 Generating new address from Go daemon..." << std::endl;

    auto response = makeHttpRequest("GET", "/address/generate");

    if (response.contains("address")) {
        std::cout << "✅ Address generated successfully" << std::endl;
        std::cout << "📍 New Address: " << response["address"].get<std::string>() << std::endl;
        return response;
    } else {
        std::cerr << "❌ Failed to generate address from Go daemon" << std::endl;
        return nlohmann::json::object();
    }
}

// Daemon Process Management Methods

bool WalletService::startDaemon() {
    if (daemonRunning_) {
        std::cout << "🔄 Go daemon already running" << std::endl;
        return true;
    }

    std::cout << "🚀 Starting Go wallet daemon..." << std::endl;

    if (createDaemonProcess()) {
        daemonRunning_ = true;
        monitorThread_ = std::thread(&WalletService::monitorDaemon, this);
        std::cout << "✅ Go daemon started successfully" << std::endl;
        return true;
    } else {
        std::cerr << "❌ Failed to start Go daemon" << std::endl;
        return false;
    }
}

void WalletService::stopDaemon() {
    if (!daemonRunning_) {
        return;
    }

    std::cout << "🛑 Stopping Go wallet daemon..." << std::endl;

    daemonRunning_ = false;

    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }

    cleanupDaemonProcess();
    std::cout << "✅ Go daemon stopped" << std::endl;
}

bool WalletService::isDaemonRunning() {
    return daemonRunning_;
}

void WalletService::setDaemonPath(const std::string& path) {
    daemonPath_ = path;
}

bool WalletService::createDaemonProcess() {
    if (daemonPath_.empty()) {
        std::cerr << "❌ Daemon path not set" << std::endl;
        return false;
    }

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // Hide the daemon window

    ZeroMemory(&daemonProcess_, sizeof(PROCESS_INFORMATION));

    // Create the daemon process
    if (!CreateProcessA(
        daemonPath_.c_str(),    // Application name
        nullptr,                // Command line
        nullptr,                // Process security attributes
        nullptr,                // Thread security attributes
        FALSE,                  // Inherit handles
        CREATE_NO_WINDOW,       // Creation flags
        nullptr,                // Environment
        nullptr,                // Current directory
        &si,                    // Startup info
        &daemonProcess_)) {     // Process information

        std::cerr << "❌ Failed to create daemon process. Error: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}

void WalletService::monitorDaemon() {
    while (daemonRunning_) {
        if (daemonProcess_.hProcess) {
            DWORD exitCode;
            if (GetExitCodeProcess(daemonProcess_.hProcess, &exitCode)) {
                if (exitCode != STILL_ACTIVE) {
                    std::cerr << "⚠️ Go daemon process exited with code: " << exitCode << std::endl;
                    daemonRunning_ = false;
                    connected_ = false;
                    break;
                }
            }
        }

        // Check every 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void WalletService::cleanupDaemonProcess() {
    if (daemonProcess_.hProcess) {
        // Try to terminate gracefully first
        if (TerminateProcess(daemonProcess_.hProcess, 0)) {
            // Wait for process to exit
            WaitForSingleObject(daemonProcess_.hProcess, 5000);
        }

        CloseHandle(daemonProcess_.hProcess);
        CloseHandle(daemonProcess_.hThread);

        ZeroMemory(&daemonProcess_, sizeof(PROCESS_INFORMATION));
    }
}

// Console Control Handler Implementation
BOOL WINAPI WalletService::ConsoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            std::cout << "\n🛑 Console shutdown signal received - cleaning up daemon..." << std::endl;
            if (g_walletService) {
                g_walletService->stopDaemon();
            }
            return TRUE;
        default:
            return FALSE;
    }
}
