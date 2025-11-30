#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <future>
#include <chrono>
#include <unordered_map>
#include <cstddef>
#include <nlohmann/json.hpp>

// Boost.Beast for HTTP client
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

using json = nlohmann::json;

namespace themis {
namespace utils {

/**
 * @brief HTTP Response structure
 */
struct HTTPResponse {
    int status_code = 0;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    
    bool isSuccess() const { return status_code >= 200 && status_code < 300; }
};

/**
 * @brief URL components for parsing
 */
struct URLComponents {
    std::string protocol;  // http or https
    std::string host;
    std::string port;
    std::string path;
    
    bool is_https() const { return protocol == "https"; }
};

/**
 * @brief Parse URL into components
 */
URLComponents parseURL(const std::string& url);

/**
 * @brief HTTP client interface
 */
class HTTPClient {
public:
    virtual ~HTTPClient() = default;
    
    virtual HTTPResponse post(
        const std::string& url,
        const json& body,
        const std::unordered_map<std::string, std::string>& headers = {}
    ) = 0;
    
    virtual HTTPResponse get(
        const std::string& url,
        const std::unordered_map<std::string, std::string>& headers = {}
    ) = 0;
};

/**
 * @brief HTTP Connection Pool for reusing TCP connections
 * 
 * Provides:
 * - Connection pooling (reduce TCP handshake overhead)
 * - Keep-Alive support
 * - Configurable timeouts
 * - Thread-safe access
 */
class HTTPClientPool {
public:
    struct Config {
        size_t max_connections = 50;              ///< Max pooled connections
        std::chrono::seconds idle_timeout{30};    ///< Connection idle timeout
        std::chrono::seconds connect_timeout{5};  ///< Connection timeout
        std::chrono::seconds request_timeout{30}; ///< Request timeout
        bool enable_keepalive = true;             ///< HTTP Keep-Alive
    };
    
    explicit HTTPClientPool(const Config& config);
    ~HTTPClientPool();
    
    /**
     * @brief Execute POST request using pooled connection
     * @param url Target URL
     * @param body JSON request body
     * @param headers Additional HTTP headers
     * @return Future with HTTP response
     */
    std::future<HTTPResponse> post(
        const std::string& url,
        const json& body,
        const std::unordered_map<std::string, std::string>& headers = {}
    );
    
    /**
     * @brief Execute GET request using pooled connection
     * @param url Target URL
     * @param headers Additional HTTP headers
     * @return Future with HTTP response
     */
    std::future<HTTPResponse> get(
        const std::string& url,
        const std::unordered_map<std::string, std::string>& headers = {}
    );
    
    /**
     * @brief Get pool statistics
     */
    struct Stats {
        size_t total_connections = 0;
        size_t available_connections = 0;
        size_t in_use_connections = 0;
    };
    
    Stats getStats() const;
    
    /**
     * @brief Clear all pooled connections
     */
    void clear();
    
private:
    /**
     * @brief Get connection from pool (or create new)
     */
    std::shared_ptr<HTTPClient> acquireConnection();
    
    /**
     * @brief Return connection to pool
     */
    void releaseConnection(std::shared_ptr<HTTPClient> client);
    
    /**
     * @brief Create new HTTP client instance
     */
    std::shared_ptr<HTTPClient> createClient();
    
    Config config_;
    std::queue<std::shared_ptr<HTTPClient>> available_clients_;
    std::vector<std::shared_ptr<HTTPClient>> all_clients_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool shutdown_ = false;
};

/**
 * @brief Concrete HTTP client implementation using Boost.Beast
 */
class BeastHTTPClient : public HTTPClient {
public:
    explicit BeastHTTPClient(const HTTPClientPool::Config& config);
    ~BeastHTTPClient() override;
    
    HTTPResponse post(
        const std::string& url,
        const json& body,
        const std::unordered_map<std::string, std::string>& headers = {}
    ) override;
    
    HTTPResponse get(
        const std::string& url,
        const std::unordered_map<std::string, std::string>& headers = {}
    ) override;
    
private:
    HTTPResponse execute(
        http::verb method,
        const std::string& url,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers
    );
    
    HTTPClientPool::Config config_;
    net::io_context ioc_;
    ssl::context ssl_ctx_;
};

} // namespace utils
} // namespace themis
