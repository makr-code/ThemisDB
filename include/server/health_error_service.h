/**
 * @file health_error_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class HealthErrorService {
public:
    struct Config {
        std::string bind_address = "127.0.0.1";  // Localhost only by default for security
        uint16_t port = 9090;
        bool enabled = true;
        
        Config() = default;
        Config(const std::string& addr, uint16_t p, bool en = true)
            : bind_address(addr), port(p), enabled(en) {}
    };

    /**
     * @brief Health Error Service.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit HealthErrorService(const Config& config);
    
    ~HealthErrorService();

    /**
     * @brief Start.
     */
    void start();

    /**
     * @brief Stop.
     */
    void stop();

    bool isRunning() const { return running_.load(); }

    /**
     * @brief Get Uptime Seconds.
     * @return Return value.
     */
    int64_t getUptimeSeconds() const;

private:
    /**
     * @brief Run.
     */
    void run();

    /**
     * @brief Handle Connection.
     * @param[in] raw_socket Input parameter.
     */
    void handleConnection(tcp::socket raw_socket);

    /**
     * @brief Handle Request.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRequest(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Health.
     * @return Return value.
     */
    http::response<http::string_body> handleHealth();

    /**
     * @brief Handle Health Components.
     * @return Return value.
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
