/**
 * @file http_client_pool.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: http_client_pool.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <queue>
#include <deque>
#include <condition_variable>
#include <future>
#include <chrono>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <cstddef>
#include <nlohmann/json.hpp>

#ifdef HAVE_BOOST_BEAST
// Boost.Beast for HTTP client
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/post.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
#endif

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
 * - Optimized for high-concurrency with reduced lock contention
 */
class HTTPClientPool {
public:
    struct Config {
        size_t max_connections = 50;              ///< Max pooled connections
        std::chrono::seconds idle_timeout{30};    ///< Connection idle timeout
        std::chrono::seconds connect_timeout{5};  ///< Connection timeout
        std::chrono::seconds request_timeout{30}; ///< Request timeout
        std::chrono::seconds acquire_timeout{10}; ///< Timeout for acquiring connection
        bool enable_keepalive = true;             ///< HTTP Keep-Alive
        size_t io_threads = 4;                    ///< Number of I/O threads
        size_t lock_stripes = 8;                  ///< Number of lock stripes for reduced contention
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
        size_t stale_connections_removed = 0;
        size_t acquire_timeouts = 0;
        size_t requests_served = 0;
        size_t connections_created = 0;
        size_t connections_reused = 0;
        
        /**
         * @brief Calculate connection reuse rate (0.0 - 1.0)
         */
        double getReuseRate() const {
            size_t total = connections_created + connections_reused;
            if (total == 0) return 0.0;
            return static_cast<double>(connections_reused) / static_cast<double>(total);
        }
    };
    
    Stats getStats() const;
    
    /**
     * @brief Clear all pooled connections
     */
    void clear();
    
    /**
     * @brief Warm up pool with minimum connections
     * 
     * Pre-creates connections to reduce cold-start latency.
     * Useful for production deployments.
     * 
     * @param num_connections Number of connections to pre-create
     */
    void warmup(size_t num_connections);
    
private:
    /**
     * @brief Pooled connection with metadata
     */
    struct PooledConnection {
        std::shared_ptr<HTTPClient> client;
        std::chrono::steady_clock::time_point last_used;
        bool in_use = false;
        
        bool isStale(std::chrono::seconds timeout) const {
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration_cast<std::chrono::seconds>(now - last_used) > timeout;
        }
    };
    
    /**
     * @brief Lock stripe for reduced contention
     */
    struct LockStripe {
        std::mutex mutex;
        std::condition_variable cv;
        std::deque<std::shared_ptr<PooledConnection>> connections;
    };
    
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
    
    /**
     * @brief Remove stale connections from pool
     */
    void pruneStaleConnections();
    
    /**
     * @brief Get stripe index for load distribution
     */
    size_t getStripeIndex() const;
    
    Config config_;
    std::vector<std::unique_ptr<LockStripe>> stripes_;
    std::shared_ptr<net::io_context> io_context_;
    std::vector<std::thread> io_threads_;
    std::atomic<size_t> total_connections_{0};
    std::atomic<size_t> requests_served_{0};
    std::atomic<size_t> stale_removed_{0};
    std::atomic<size_t> acquire_timeouts_{0};
    std::atomic<size_t> connections_created_{0};
    std::atomic<size_t> connections_reused_{0};
    std::atomic<bool> shutdown_{false};
    mutable std::atomic<size_t> round_robin_{0};
};

/**
 * @brief Concrete HTTP client implementation using Boost.Beast
 */
class BeastHTTPClient : public HTTPClient {
public:
    BeastHTTPClient(const HTTPClientPool::Config& config, std::shared_ptr<net::io_context> ioc);
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
    std::shared_ptr<net::io_context> ioc_;
    ssl::context ssl_ctx_;
};

} // namespace utils
} // namespace themis
