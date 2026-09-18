/**
 * @file http_auth_async.h
 * @brief Asynchronous HTTP-based authentication utilities.
 *
 * Declares non-blocking HTTP helpers used by the auth module to validate
 * bearer tokens and OIDC discovery documents without stalling request threads.
 */

#pragma once

#include "auth/auth_error.h"
#include "auth/auth_worker_thread_pool.h"

#include <string>
#include <functional>
#include <future>
#include <memory>
#include <chrono>
#include <optional>
#include <vector>

namespace themis {
namespace auth {

struct HTTPAuthResponse {
    int status_code{0};
    std::string body = {};
    bool success{false};
    std::string error_message;
    
    /**
     * @brief Success.
     * @param[in] code Input parameter.
     * @param[in] body_content Input parameter.
     * @return Return value.
     */
    static HTTPAuthResponse Success(int code, const std::string& body_content)
    {
        HTTPAuthResponse r;
        r.status_code = code;
        r.body = body_content;
        r.success = (code >= 200 && code < 300);
        return r;
    }
    
    /**
     * @brief Failed.
     * @param[in] error Input parameter.
     * @return Return value.
     */
    static HTTPAuthResponse Failed(const std::string& error)
    {
        HTTPAuthResponse r;
        r.success = false;
        r.error_message = error;
        return r;
    }
};

struct HTTPAuthConfig {
    int request_timeout_seconds{30};
    
    int max_retries{3};
    
    int retry_backoff_ms{100};
    
    bool enable_http2{true};
    
    bool verify_ssl_certs{true};
};

class AsyncHTTPAuth {
public:
    explicit AsyncHTTPAuth(const HTTPAuthConfig& config = HTTPAuthConfig());
    ~AsyncHTTPAuth();
    
    // Non-copyable, non-movable
    AsyncHTTPAuth(const AsyncHTTPAuth&) = delete;
    AsyncHTTPAuth& operator=(const AsyncHTTPAuth&) = delete;
    AsyncHTTPAuth(AsyncHTTPAuth&&) = delete;
    AsyncHTTPAuth& operator=(AsyncHTTPAuth&&) = delete;
    
    std::future<HTTPAuthResponse> getAsync(
        const std::string& url,
        const std::vector<std::pair<std::string, std::string>>& headers = {});
    
    std::future<HTTPAuthResponse> postAsync(
        const std::string& url,
        const std::string& body,
        const std::string& content_type = "application/json",
        const std::vector<std::pair<std::string, std::string>>& headers = {});
    
    /**
     * @brief Check Connectivity Async.
     * @param[in] url Input parameter.
     * @return Return value.
     */
    std::future<bool> checkConnectivityAsync(const std::string& url);
    
    const HTTPAuthConfig& config() const { return config_; }
    
    /**
     * @brief Thread Count.
     * @return Return value.
     */
    size_t threadCount() const;
    
private:
    HTTPAuthConfig config_;
    
    // Worker thread pool for dispatching HTTP operations
    std::unique_ptr<AuthWorkerThreadPool> worker_pool_;
    
    HTTPAuthResponse performGet(
        const std::string& url,
        const std::vector<std::pair<std::string, std::string>>& headers);
    
    HTTPAuthResponse performPost(
        const std::string& url,
        const std::string& body,
        const std::string& content_type,
        const std::vector<std::pair<std::string, std::string>>& headers);
    
    /**
     * @brief Perform Connectivity Check.
     * @param[in] url Input parameter.
     * @return True when the operation succeeds.
     */
    bool performConnectivityCheck(const std::string& url);
    
    /**
     * @brief Validate URL.
     * @param[in] url Input parameter.
     */
    static void validateURL(const std::string& url);
};

} // namespace auth
} // namespace themis
