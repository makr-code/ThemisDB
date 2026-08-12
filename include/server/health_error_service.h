/**
 * @file health_error_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

namespace themis {
namespace server {

// Forward declarations
class ErrorApiHandler;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

/**
 * @brief Lightweight health and error introspection service on alternate port
 * 
 * This service provides error diagnostics and health checks on a separate port
 * to ensure observability even when the main HTTP server experiences issues.
 * 
 * Features:
 * - Runs on separate port (default: 9090) independent of main server
 * - Exposes error introspection endpoints from ErrorApiHandler
 * - Provides basic health check endpoints
 * - Minimal dependencies (ErrorRegistry only, no RocksDB/LLM)
 * - Non-blocking operation in separate thread
 */
class HealthErrorService {
public:
    /**
     * @brief Configuration for health/error service
     */
    struct Config {
        std::string bind_address = "127.0.0.1";  // Localhost only by default for security
        uint16_t port = 9090;
        bool enabled = true;
        
        Config() = default;
        Config(const std::string& addr, uint16_t p, bool en = true)
            : bind_address(addr), port(p), enabled(en) {}
    };

    /**
     * @brief Construct health/error service with configuration
     * @param config Service configuration
     */
    explicit HealthErrorService(const Config& config);
    
    /**
     * @brief Destructor - ensures graceful shutdown
     */
    ~HealthErrorService();

    /**
     * @brief Start the service (non-blocking, runs in separate thread)
     */
    void start();

    /**
     * @brief Stop the service gracefully
     */
    void stop();

    /**
     * @brief Check if service is running
     * @return true if service is running, false otherwise
     */
    bool isRunning() const { return running_.load(); }

    /**
     * @brief Get service uptime in seconds
     * @return Uptime in seconds
     */
    int64_t getUptimeSeconds() const;

private:
    /**
     * @brief Main service loop (runs in separate thread)
     */
    void run();

    /**
     * @brief Handle incoming HTTP connection with enforced read/write timeouts
     * @param raw_socket Connected socket; ownership transferred; wrapped in
     *        beast::tcp_stream with 10-second per-operation timeout to prevent
     *        slow-client thread starvation (Phase 8.2 remediation).
     */
    void handleConnection(tcp::socket raw_socket);

    /**
     * @brief Route and handle HTTP request
     * @param req HTTP request
     * @return HTTP response
     */
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle /health endpoint
     * @return HTTP response with health status
     */
    http::response<http::string_body> handleHealth();

    /**
     * @brief Handle /health/components endpoint
     * @return HTTP response with component health details
     */
    http::response<http::string_body> handleHealthComponents();

    Config config_;
    std::unique_ptr<net::io_context> ioc_;
    std::unique_ptr<tcp::acceptor> acceptor_;
    std::unique_ptr<std::thread> service_thread_;
    std::unique_ptr<ErrorApiHandler> error_handler_;
    std::atomic<bool> running_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace server
} // namespace themis
