/**
 * @file http_auth_async.cpp
 * @brief Asynchronous HTTP authentication implementation.
 *
 * Implements the async HTTP helpers declared in http_auth_async.h using
 * the internal ThemisDB HTTP client abstraction.
 */

#include "auth/http_auth_async.h"
#include <stdexcept>
#include <regex>
#include <algorithm>

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <arpa/inet.h>
#endif

// For HTTP requests (libcurl integration)
#include <curl/curl.h>

namespace themis {
namespace auth {

// ===========================================================================
// Helper: Validate URL format
// ===========================================================================

void AsyncHTTPAuth::validateURL(const std::string& url)
{
    // Must be absolute HTTP(S) URL
    if (url.empty()) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_CONFIG_INVALID,
                         "URL cannot be empty",
                         "AsyncHTTPAuth::validateURL received empty URL");
    }

    if (url.find("http://") != 0 && url.find("https://") != 0) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_CONFIG_INVALID,
                         "URL must start with http:// or https://",
                         "AsyncHTTPAuth::validateURL received non-http(s) URL");
    }

    // Very basic validation: must have at least host
    if (url.length() < 12) {  // "https://x.co" = 12 chars
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_CONFIG_INVALID,
                         "URL appears malformed",
                         "AsyncHTTPAuth::validateURL rejected too-short URL");
    }
}

// ===========================================================================
// Constructor / Destructor
// ===========================================================================

AsyncHTTPAuth::AsyncHTTPAuth(const HTTPAuthConfig& config)
    : config_(config)
{
    // Spawn worker thread pool with 4-16 threads for HTTP operations
    worker_pool_ = std::make_unique<AuthWorkerThreadPool>(4, 16);
}

AsyncHTTPAuth::~AsyncHTTPAuth()
{
    // Worker pool is automatically shut down when destroyed
}

// ===========================================================================
// Async HTTP methods
// ===========================================================================

std::future<HTTPAuthResponse> AsyncHTTPAuth::getAsync(
    const std::string& url,
    const std::vector<std::pair<std::string, std::string>>& headers)
{
    // Validate input synchronously on caller's thread
    validateURL(url);
    
    // Make copies to capture in lambda
    std::string url_copy = url;
    auto headers_copy = headers;
    
    // Dispatch to worker pool
    return worker_pool_->submit([this, url_copy, headers_copy]() {
        return performGet(url_copy, headers_copy);
    });
}

std::future<HTTPAuthResponse> AsyncHTTPAuth::postAsync(
    const std::string& url,
    const std::string& body,
    const std::string& content_type,
    const std::vector<std::pair<std::string, std::string>>& headers)
{
    // Validate input synchronously on caller's thread
    validateURL(url);

    if (body.empty()) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_CONFIG_INVALID,
                         "Request body cannot be empty",
                         "AsyncHTTPAuth::postAsync received empty request body");
    }

    // Make copies to capture in lambda
    std::string url_copy = url;
    std::string body_copy = body;
    std::string ct_copy = content_type;
    auto headers_copy = headers;
    
    // Dispatch to worker pool
    return worker_pool_->submit([this, url_copy, body_copy, ct_copy, headers_copy]() {
        return performPost(url_copy, body_copy, ct_copy, headers_copy);
    });
}

std::future<bool> AsyncHTTPAuth::checkConnectivityAsync(const std::string& url)
{
    // Validate input synchronously on caller's thread
    validateURL(url);
    
    std::string url_copy = url;
    
    // Dispatch to worker pool
    return worker_pool_->submit([this, url_copy]() {
        return performConnectivityCheck(url_copy);
    });
}

// ===========================================================================
// Synchronous HTTP operations (run on worker threads)
// ===========================================================================

namespace {

// RAII helper for CURL handle
class CURLHandle {
public:
    CURLHandle() : handle_(curl_easy_init())
    {
        if (!handle_) {
            throw std::runtime_error("Failed to initialize CURL");
        }
    }
    
    ~CURLHandle()
    {
        if (handle_) {
            curl_easy_cleanup(handle_);
        }
    }
    
    CURL* get() const { return handle_; }
    
    CURLHandle(const CURLHandle&) = delete;
    CURLHandle& operator=(const CURLHandle&) = delete;
    
private:
    CURL* handle_;
};

// Callback for curl to accumulate response data
static size_t writecallback(void* contents, size_t size, size_t nmemb, std::string* s)
{
    size_t newlen = size * nmemb;
    s->append((char*)contents, newlen);
    return newlen;
}

} // anonymous namespace

HTTPAuthResponse AsyncHTTPAuth::performGet(
    const std::string& url,
    const std::vector<std::pair<std::string, std::string>>& headers)
{
    CURLHandle curl;
    std::string response_body;
    long http_code = 0;
    
    try {
        // Set URL
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        
        // Set timeout
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, (long)config_.request_timeout_seconds);
        curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 5L);
        
        // Set SSL options
        if (!config_.verify_ssl_certs) {
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);
        }
        
        // Add custom headers if provided
        struct curl_slist* slist = nullptr;
        for (const auto& [key, value] : headers) {
            std::string header = key + ": " + value;
            slist = curl_slist_append(slist, header.c_str());
        }
        if (slist) {
            curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, slist);
        }
        
        // Set response callback
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, writecallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_body);
        
        // Perform request with retry logic
        CURLcode res = CURLE_OK;
        int attempts = 0;
        
        while (attempts < config_.max_retries) {
            res = curl_easy_perform(curl.get());
            
            // Check for transient errors worth retrying
            if (res == CURLE_OPERATION_TIMEDOUT ||
                res == CURLE_COULDNT_RESOLVE_HOST ||
                res == CURLE_COULDNT_CONNECT) {
                ++attempts;
                if (attempts < config_.max_retries) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(config_.retry_backoff_ms * attempts));
                    response_body.clear();
                    continue;
                }
            }
            break;
        }
        
        if (slist) {
            curl_slist_free_all(slist);
        }
        
        if (res != CURLE_OK) {
            return HTTPAuthResponse::Failed(
                std::string("HTTP GET failed: ") + curl_easy_strerror(res));
        }
        
        // Get HTTP response code
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
        
        return HTTPAuthResponse::Success(http_code, response_body);
        
    } catch (const std::exception& e) {
        return HTTPAuthResponse::Failed(
            std::string("HTTP GET exception: ") + e.what());
    }
}

HTTPAuthResponse AsyncHTTPAuth::performPost(
    const std::string& url,
    const std::string& body,
    const std::string& content_type,
    const std::vector<std::pair<std::string, std::string>>& headers)
{
    CURLHandle curl;
    std::string response_body;
    long http_code = 0;
    
    try {
        // Set URL
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        
        // Set POST
        curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, body.c_str());
        
        // Set timeout
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, (long)config_.request_timeout_seconds);
        curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 5L);
        
        // Set SSL options
        if (!config_.verify_ssl_certs) {
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);
        }
        
        // Add headers
        struct curl_slist* slist = nullptr;
        
        // Add content-type header
        std::string ct_header = "Content-Type: " + content_type;
        slist = curl_slist_append(slist, ct_header.c_str());
        
        // Add custom headers
        for (const auto& [key, value] : headers) {
            std::string header = key + ": " + value;
            slist = curl_slist_append(slist, header.c_str());
        }
        
        if (slist) {
            curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, slist);
        }
        
        // Set response callback
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, writecallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_body);
        
        // Perform request with retry logic
        CURLcode res = CURLE_OK;
        int attempts = 0;
        
        while (attempts < config_.max_retries) {
            res = curl_easy_perform(curl.get());
            
            // Check for transient errors worth retrying
            if (res == CURLE_OPERATION_TIMEDOUT ||
                res == CURLE_COULDNT_RESOLVE_HOST ||
                res == CURLE_COULDNT_CONNECT) {
                ++attempts;
                if (attempts < config_.max_retries) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(config_.retry_backoff_ms * attempts));
                    response_body.clear();
                    continue;
                }
            }
            break;
        }
        
        if (slist) {
            curl_slist_free_all(slist);
        }
        
        if (res != CURLE_OK) {
            return HTTPAuthResponse::Failed(
                std::string("HTTP POST failed: ") + curl_easy_strerror(res));
        }
        
        // Get HTTP response code
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
        
        return HTTPAuthResponse::Success(http_code, response_body);
        
    } catch (const std::exception& e) {
        return HTTPAuthResponse::Failed(
            std::string("HTTP POST exception: ") + e.what());
    }
}

bool AsyncHTTPAuth::performConnectivityCheck(const std::string& url)
{
    CURLHandle curl;
    long http_code = 0;
    
    try {
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_NOBODY, 1L);  // HEAD request
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 3L);
        
        if (!config_.verify_ssl_certs) {
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);
        }
        
        CURLcode res = curl_easy_perform(curl.get());
        
        if (res != CURLE_OK) {
            return false;
        }
        
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
        
        // Consider any 2xx response as successful connectivity
        return (http_code >= 200 && http_code < 300);
        
    } catch (...) {
        return false;
    }
}

size_t AsyncHTTPAuth::threadCount() const
{
    return worker_pool_ ? worker_pool_->threadCount() : 0;
}

} // namespace auth
} // namespace themis
