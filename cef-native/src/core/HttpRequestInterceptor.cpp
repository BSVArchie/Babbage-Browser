#include "../../include/core/HttpRequestInterceptor.h"
#include "include/wrapper/cef_helpers.h"
#include "include/cef_urlrequest.h"
#include "include/cef_request.h"
#include "include/cef_request_context.h"
#include "include/cef_browser.h"
#include "include/cef_task.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <mutex>
#include <condition_variable>

// Simple logging function
void LogDebug(const std::string& message) {
    std::ofstream debugLog("debug_output.log", std::ios::app);
    if (debugLog.is_open()) {
        debugLog << "[HTTP] " << message << std::endl;
        debugLog.close();
    }
}

// Forward declaration
class AsyncHTTPClient;

// Async Resource Handler for managing wallet HTTP requests
class AsyncWalletResourceHandler : public CefResourceHandler {
public:
    AsyncWalletResourceHandler(const std::string& method,
                              const std::string& endpoint,
                              const std::string& body)
        : method_(method), endpoint_(endpoint), body_(body),
          responseOffset_(0), requestCompleted_(false) {
        std::cout << "🌐 AsyncWalletResourceHandler constructor called for " << method << " " << endpoint << std::endl;
        LogDebug("🌐 AsyncWalletResourceHandler constructor called for " + method + " " + endpoint);
    }

    bool Open(CefRefPtr<CefRequest> request,
              bool& handle_request,
              CefRefPtr<CefCallback> callback) override {
        CEF_REQUIRE_IO_THREAD();

        std::cout << "🌐 AsyncWalletResourceHandler::Open called" << std::endl;
        LogDebug("🌐 AsyncWalletResourceHandler::Open called");

        handle_request = true;

        // Start async HTTP request to Go daemon
        std::cout << "🌐 About to start async HTTP request..." << std::endl;
        startAsyncHTTPRequest();
        std::cout << "🌐 Async HTTP request started" << std::endl;

        // Don't call callback->Continue() yet - wait for HTTP response
        return true;
    }

    void GetResponseHeaders(scoped_refptr<CefResponse> response,
                           int64_t& response_length,
                           CefString& redirectUrl) override {
        CEF_REQUIRE_IO_THREAD();

        std::cout << "🌐 AsyncWalletResourceHandler::GetResponseHeaders called" << std::endl;

        response->SetStatus(200);
        response->SetStatusText("OK");
        response->SetMimeType("application/json");
        response->SetHeaderByName("Access-Control-Allow-Origin", "*", true);
        response->SetHeaderByName("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS", true);
        response->SetHeaderByName("Access-Control-Allow-Headers", "Content-Type, Authorization", true);
        response->SetHeaderByName("Access-Control-Max-Age", "86400", true);

        response_length = static_cast<int64_t>(responseData_.length());
    }

    bool ReadResponse(void* data_out,
                     int bytes_to_read,
                     int& bytes_read,
                     CefRefPtr<CefCallback> callback) override {
        CEF_REQUIRE_IO_THREAD();

        std::cout << "🌐 AsyncWalletResourceHandler::ReadResponse called, completed: " << requestCompleted_ << std::endl;

        if (!requestCompleted_) {
            // Store callback for later use
            readCallback_ = callback;
            return true; // Wait for HTTP response
        }

        // Send response data to frontend
        if (responseOffset_ >= responseData_.length()) {
            bytes_read = 0;
            return false; // No more data
        }

        int remaining = static_cast<int>(responseData_.length() - responseOffset_);
        bytes_read = (bytes_to_read < remaining) ? bytes_to_read : remaining;
        memcpy(data_out, responseData_.c_str() + responseOffset_, bytes_read);
        responseOffset_ += bytes_read;

        return true;
    }

    void Cancel() override {
        CEF_REQUIRE_IO_THREAD();
        std::cout << "🌐 AsyncWalletResourceHandler::Cancel called" << std::endl;

        if (urlRequest_) {
            urlRequest_->Cancel();
            urlRequest_ = nullptr;
        }
    }

    // Called by AsyncHTTPClient when HTTP response is received
    void onHTTPResponseReceived(const std::string& data) {
        std::cout << "🌐 AsyncWalletResourceHandler received HTTP response: " << data << std::endl;
        LogDebug("🌐 AsyncWalletResourceHandler received HTTP response: " + data);

        responseData_ = data;
        requestCompleted_ = true;

        LogDebug("🌐 About to call readCallback_->Continue()");
        // Now we can continue with the response
        if (readCallback_) {
            readCallback_->Continue();
            LogDebug("🌐 readCallback_->Continue() called successfully");
        }
    }

    // Static method to create CefURLRequest on UI thread (called by URLRequestCreationTask)
    static void createURLRequestOnUIThread(AsyncWalletResourceHandler* handler,
                                          CefRefPtr<CefRequest> httpRequest,
                                          AsyncHTTPClient* client,
                                          CefRefPtr<CefRequestContext> context);

private:
    void startAsyncHTTPRequest();

    // Request data
    std::string method_;
    std::string endpoint_;
    std::string body_;

    // Response management
    std::string responseData_;
    size_t responseOffset_;
    bool requestCompleted_;

    // CEF request management
    CefRefPtr<CefURLRequest> urlRequest_;
    CefRefPtr<CefCallback> readCallback_;

    IMPLEMENT_REFCOUNTING(AsyncWalletResourceHandler);
    DISALLOW_COPY_AND_ASSIGN(AsyncWalletResourceHandler);
};

// Async HTTP Client for handling CEF URL requests
class AsyncHTTPClient : public CefURLRequestClient {
public:
    explicit AsyncHTTPClient(AsyncWalletResourceHandler* parent)
        : parent_(parent), completed_(false) {
        LogDebug("🌐 AsyncHTTPClient constructor called");
    }

    void OnRequestComplete(CefRefPtr<CefURLRequest> request) override {
        std::lock_guard<std::mutex> lock(mutex_);
        completed_ = true;

        LogDebug("🌐 AsyncHTTPClient::OnRequestComplete called, response size: " + std::to_string(responseData_.length()));

        // Notify parent handler that HTTP request completed
        if (parent_) {
            parent_->onHTTPResponseReceived(responseData_);
        }
    }

    void OnUploadProgress(CefRefPtr<CefURLRequest> request, int64_t current, int64_t total) override {
        // Not needed for our use case
    }

    void OnDownloadProgress(CefRefPtr<CefURLRequest> request, int64_t current, int64_t total) override {
        // Not needed for our use case
    }

    void OnDownloadData(CefRefPtr<CefURLRequest> request, const void* data, size_t data_length) override {
        std::lock_guard<std::mutex> lock(mutex_);
        responseData_.append(static_cast<const char*>(data), data_length);
        LogDebug("🌐 AsyncHTTPClient::OnDownloadData received " + std::to_string(data_length) + " bytes");
    }

    bool GetAuthCredentials(bool isProxy,
                           const CefString& host,
                           int port,
                           const CefString& realm,
                           const CefString& scheme,
                           CefRefPtr<CefAuthCallback> callback) override {
        return false; // No authentication needed
    }

private:
    AsyncWalletResourceHandler* parent_;
    std::mutex mutex_;
    bool completed_;
    std::string responseData_;

    IMPLEMENT_REFCOUNTING(AsyncHTTPClient);
    DISALLOW_COPY_AND_ASSIGN(AsyncHTTPClient);
};

// Task class for creating CefURLRequest on UI thread
class URLRequestCreationTask : public CefTask {
public:
    URLRequestCreationTask(AsyncWalletResourceHandler* handler,
                          CefRefPtr<CefRequest> httpRequest,
                          AsyncHTTPClient* client,
                          CefRefPtr<CefRequestContext> context)
        : handler_(handler), httpRequest_(httpRequest), client_(client), context_(context) {}

    void Execute() override {
        LogDebug("🌐 URLRequestCreationTask::Execute called on UI thread");
        AsyncWalletResourceHandler::createURLRequestOnUIThread(handler_, httpRequest_, client_, context_);
    }

private:
    AsyncWalletResourceHandler* handler_;
    CefRefPtr<CefRequest> httpRequest_;
    AsyncHTTPClient* client_;
    CefRefPtr<CefRequestContext> context_;

    IMPLEMENT_REFCOUNTING(URLRequestCreationTask);
    DISALLOW_COPY_AND_ASSIGN(URLRequestCreationTask);
};

// Implementation of AsyncWalletResourceHandler::startAsyncHTTPRequest
void AsyncWalletResourceHandler::startAsyncHTTPRequest() {
    std::cout << "🌐 Starting async HTTP request to: " << endpoint_ << std::endl;
    LogDebug("🌐 Starting async HTTP request to: " + endpoint_);

    // Create CEF HTTP request
    LogDebug("🌐 Creating CEF HTTP request");
    CefRefPtr<CefRequest> httpRequest = CefRequest::Create();
    std::string fullUrl = "http://localhost:8080" + endpoint_;
    httpRequest->SetURL(fullUrl);
    httpRequest->SetMethod(method_);

    LogDebug("🌐 Setting headers for request");
    // Set headers
    CefRequest::HeaderMap headers;
    headers.insert(std::make_pair("Content-Type", "application/json"));
    headers.insert(std::make_pair("Accept", "application/json"));
    httpRequest->SetHeaderMap(headers);

    // Set POST body if needed
    if (method_ == "POST" && !body_.empty()) {
        LogDebug("🌐 Setting POST body");
        CefRefPtr<CefPostData> postData = CefPostData::Create();
        CefRefPtr<CefPostDataElement> element = CefPostDataElement::Create();
        element->SetToBytes(body_.length(), body_.c_str());
        postData->AddElement(element);
        httpRequest->SetPostData(postData);
    }

    // Start async request
    LogDebug("🌐 About to create CefURLRequest");
    LogDebug("🌐 Creating AsyncHTTPClient");
    AsyncHTTPClient* client = new AsyncHTTPClient(this);
    LogDebug("🌐 AsyncHTTPClient created successfully");

    LogDebug("🌐 Getting global request context");
    CefRefPtr<CefRequestContext> context = CefRequestContext::GetGlobalContext();
    LogDebug("🌐 Global request context obtained");

    LogDebug("🌐 About to call CefURLRequest::Create");
    LogDebug("🌐 HTTP Request URL: " + httpRequest->GetURL().ToString());
    LogDebug("🌐 HTTP Request Method: " + httpRequest->GetMethod().ToString());

    CefRequest::HeaderMap requestHeaders;
    httpRequest->GetHeaderMap(requestHeaders);
    LogDebug("🌐 HTTP Request Headers count: " + std::to_string(requestHeaders.size()));

    try {
        LogDebug("🌐 Inside try block, about to create URL request");
        LogDebug("🌐 Posting task to UI thread for CefURLRequest creation");

        // Post task to UI thread - CefURLRequest::Create must be called from UI thread
        // Create a simple task that will call our method
        CefPostTask(TID_UI, new URLRequestCreationTask(this, httpRequest, client, context));
        LogDebug("🌐 Task posted to UI thread successfully");

    } catch (const std::exception& e) {
        LogDebug("🌐 Exception caught: " + std::string(e.what()));
    } catch (...) {
        LogDebug("🌐 Unknown exception caught");
    }
}

// Static method to create CefURLRequest on UI thread
void AsyncWalletResourceHandler::createURLRequestOnUIThread(AsyncWalletResourceHandler* handler,
                                                           CefRefPtr<CefRequest> httpRequest,
                                                           AsyncHTTPClient* client,
                                                           CefRefPtr<CefRequestContext> context) {
    LogDebug("🌐 createURLRequestOnUIThread called on UI thread");

    try {
        LogDebug("🌐 Creating CefURLRequest on UI thread");
        handler->urlRequest_ = CefURLRequest::Create(httpRequest, client, context);
        LogDebug("🌐 CefURLRequest created successfully on UI thread");
    } catch (const std::exception& e) {
        LogDebug("🌐 Exception in UI thread: " + std::string(e.what()));
    } catch (...) {
        LogDebug("🌐 Unknown exception in UI thread");
    }
}

HttpRequestInterceptor::HttpRequestInterceptor() {
    std::cout << "🌐 HttpRequestInterceptor created" << std::endl;
}

HttpRequestInterceptor::~HttpRequestInterceptor() {
    std::cout << "🌐 HttpRequestInterceptor destroyed" << std::endl;
}

CefRefPtr<CefResourceHandler> HttpRequestInterceptor::GetResourceHandler(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefRequest> request) {

    CEF_REQUIRE_IO_THREAD();

    std::string url = request->GetURL().ToString();
    std::string method = request->GetMethod().ToString();

    std::cout << "🌐 HTTP Request intercepted: " << method << " " << url << std::endl;
    LogDebug("🌐 HTTP Request intercepted: " + method + " " + url);

    std::cout << "🌐 About to check if wallet endpoint..." << std::endl;

    // Check if this is a wallet endpoint
    if (!isWalletEndpoint(url)) {
        std::cout << "🌐 Not a wallet endpoint, allowing normal processing" << std::endl;
        return nullptr; // Let CEF handle it normally
    }

    std::cout << "🌐 Wallet endpoint detected, creating async handler" << std::endl;

    // Get request body
    std::string body;
    CefRefPtr<CefPostData> postData = request->GetPostData();
    if (postData) {
        std::cout << "🌐 Processing POST data..." << std::endl;
        CefPostData::ElementVector elements;
        postData->GetElements(elements);
        for (auto& element : elements) {
            if (element->GetType() == PDE_TYPE_BYTES) {
                size_t size = element->GetBytesCount();
                std::vector<char> buffer(size);
                element->GetBytes(size, buffer.data());
                body = std::string(buffer.data(), size);
            }
        }
    }

    // Extract endpoint from URL
    std::string endpoint;
    size_t pos = url.find("://");
    if (pos != std::string::npos) {
        pos = url.find("/", pos + 3);
        if (pos != std::string::npos) {
            endpoint = url.substr(pos);
        }
    }

    std::cout << "🌐 Extracted endpoint: " << endpoint << std::endl;

    if (!endpoint.empty()) {
        std::cout << "🌐 About to create AsyncWalletResourceHandler..." << std::endl;
        // Create and return async handler
        AsyncWalletResourceHandler* handler = new AsyncWalletResourceHandler(method, endpoint, body);
        std::cout << "🌐 AsyncWalletResourceHandler created successfully" << std::endl;
        return handler;
    }

    std::cout << "🌐 Could not extract endpoint from URL: " << url << std::endl;
    return nullptr;
}

void HttpRequestInterceptor::OnResourceRedirect(CefRefPtr<CefBrowser> browser,
                                               CefRefPtr<CefFrame> frame,
                                               CefRefPtr<CefRequest> request,
                                               CefRefPtr<CefResponse> response,
                                               CefString& new_url) {
    CEF_REQUIRE_IO_THREAD();
    std::cout << "🌐 Resource redirect: " << new_url.ToString() << std::endl;
}

bool HttpRequestInterceptor::OnResourceResponse(CefRefPtr<CefBrowser> browser,
                                              CefRefPtr<CefFrame> frame,
                                              CefRefPtr<CefRequest> request,
                                              CefRefPtr<CefResponse> response) {
    CEF_REQUIRE_IO_THREAD();
    return false;
}


bool HttpRequestInterceptor::isWalletEndpoint(const std::string& url) {
    // Check if URL contains wallet endpoints
    return (url.find("/brc100/") != std::string::npos ||
            url.find("/wallet/") != std::string::npos ||
            url.find("/transaction/") != std::string::npos);
}
