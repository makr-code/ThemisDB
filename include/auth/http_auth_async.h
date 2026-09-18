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

/**
 * @brief HTTP response structure for authentication operations
 */
struct HTTPAuthResponse {
    int status_code{0};
    std::string body = {};
    bool success{false};
    std::string error_message;
    
    static HTTPAuthResponse Success(int code, const std::string& body_content)
    {
        HTTPAuthResponse r;
        r.status_code = code;
        r.body = body_content;
        r.success = (code >= 200 && code < 300);
        return r;
    }
    
    static HTTPAuthResponse Failed(const std::string& error)
    {
        HTTPAuthResponse r;
        r.success = false;
        r.error_message = error;
        return r;
    }
};

/**
 * @brief Configuration for async HTTP authentication
 */
struct HTTPAuthConfig {
    /// HTTP request timeout (seconds)
    int request_timeout_seconds{30};
    
    /// Maximum number of retries for transient failures
    int max_retries{3};
    
    /// Retry backoff delay (milliseconds)
    int retry_backoff_ms{100};
    
    /// Enable HTTP/2 support if available
    bool enable_http2{true};
    
    /// Certificate validation (disable only for testing)
    bool verify_ssl_certs{true};
};

/**
 * @brief Async HTTP authentication interface
 *
 * Wraps synchronous HTTP calls (OAuth token endpoints, OIDC discovery, SAML metadata)
 * in non-blocking operations using the AuthWorkerThreadPool.
 *
 * Performance target (auth roadmap v1.2.0):
 *   - OAuth token requests never block the caller's thread
 *   - OIDC discovery fetches run in background, cached results available immediately
 *   - P99 latency visible to callers ≤ 100 ms even when HTTP backend takes 500 ms
 *
 * Thread-safety: All public methods are safe to call concurrently.
 */
class AsyncHTTPAuth {
public:
    explicit AsyncHTTPAuth(const HTTPAuthConfig& config = HTTPAuthConfig());
    ~AsyncHTTPAuth();
    
    // Non-copyable, non-movable
    AsyncHTTPAuth(const AsyncHTTPAuth&) = delete;
    AsyncHTTPAuth& operator=(const AsyncHTTPAuth&) = delete;
    AsyncHTTPAuth(AsyncHTTPAuth&&) = delete;
    AsyncHTTPAuth& operator=(AsyncHTTPAuth&&) = delete;
    
    /**
     * @brief Perform an async HTTP GET request
     *
     * Dispatches the request to a worker thread. The caller receives a
     * std::future<HTTPAuthResponse> immediately and is never blocked by
     * network latency.
     *
     * @param url          Target URL (must be absolute)
     * @param headers      Optional HTTP headers to send
     * @return std::future<HTTPAuthResponse> — becomes ready when response arrives
     * @throws AuthException on invalid input (malformed URL)
     * @throws std::runtime_error if the thread pool is not running
     */
    std::future<HTTPAuthResponse> getAsync(
        const std::string& url,
        const std::vector<std::pair<std::string, std::string>>& headers = {});
    
    /**
     * @brief Perform an async HTTP POST request
     *
     * Dispatches the POST request to a worker thread with the given body
     * and content-type. The caller receives a std::future<HTTPAuthResponse>
     * immediately and is never blocked.
     *
     * @param url          Target URL (must be absolute)
     * @param body         Request body (JSON, form-encoded, etc.)
     * @param content_type MIME type of the body (e.g., "application/json")
     * @param headers      Optional additional HTTP headers
     * @return std::future<HTTPAuthResponse> — becomes ready when response arrives
     * @throws AuthException on invalid input
     * @throws std::runtime_error if the thread pool is not running
     */
    std::future<HTTPAuthResponse> postAsync(
        const std::string& url,
        const std::string& body,
        const std::string& content_type = "application/json",
        const std::vector<std::pair<std::string, std::string>>& headers = {});
    
    /**
     * @brief Check if HTTP connection is available (non-blocking)
     *
     * Performs a lightweight connectivity check by sending a HEAD request
     * to a given URL. Useful for validating auth provider availability
     * without blocking the caller's thread.
     *
     * @param url Target URL to check
     * @return std::future<bool> — true if reachable, false otherwise
     */
    std::future<bool> checkConnectivityAsync(const std::string& url);
    
    /**
     * @brief Return the configuration (after construction)
     */
    const HTTPAuthConfig& config() const { return config_; }
    
    /**
     * @brief Return the number of active worker threads
     */
    size_t threadCount() const;
    
private:
    HTTPAuthConfig config_;
    
    // Worker thread pool for dispatching HTTP operations
    std::unique_ptr<AuthWorkerThreadPool> worker_pool_;
    
    /**
     * @brief Perform the actual HTTP GET (called by worker thread)
     *
     * Performs retries on transient failures and respects the timeout.
     */
    HTTPAuthResponse performGet(
        const std::string& url,
        const std::vector<std::pair<std::string, std::string>>& headers);
    
    /**
     * @brief Perform the actual HTTP POST (called by worker thread)
     */
    HTTPAuthResponse performPost(
        const std::string& url,
        const std::string& body,
        const std::string& content_type,
        const std::vector<std::pair<std::string, std::string>>& headers);
    
    /**
     * @brief Perform the actual connectivity check (called by worker thread)
     */
    bool performConnectivityCheck(const std::string& url);
    
    /**
     * @brief Validate URL format (must be absolute HTTP/HTTPS)
     *
     * @throws AuthException if URL is invalid
     */
    static void validateURL(const std::string& url);
};

} // namespace auth
} // namespace themis
