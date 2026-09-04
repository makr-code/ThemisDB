/**
 * @file health_error_service.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/health_error_service.h"
#include "server/error_api_handler.h"
#include "utils/error_registry.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>

namespace themis {
namespace server {

using json = nlohmann::json;

HealthErrorService::HealthErrorService(const Config& config)
    : config_(config)
    , ioc_(std::make_unique<net::io_context>())
    , error_handler_(std::make_unique<ErrorApiHandler>())
    , running_(false)
    , start_time_(std::chrono::steady_clock::now())
{
    if (!config_.enabled) {
        THEMIS_INFO("Health/Error service is disabled in configuration");
        return;
    }

    try {
        // Create TCP acceptor
        auto endpoint = tcp::endpoint(
            net::ip::make_address(config_.bind_address), 
            config_.port
        );
        acceptor_ = std::make_unique<tcp::acceptor>(*ioc_, endpoint);
        
        THEMIS_INFO("Health/Error service initialized on {}:{}", 
                   config_.bind_address, config_.port);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to initialize Health/Error service: {}", e.what());
        throw;
    }
}

HealthErrorService::~HealthErrorService() {
    stop();
}

void HealthErrorService::start() {
    if (!config_.enabled) {
        return;
    }

    if (running_.load()) {
        THEMIS_WARN("Health/Error service is already running");
        return;
    }

    running_.store(true);
    start_time_ = std::chrono::steady_clock::now();
    
    // Start service in separate thread
    service_thread_ = std::make_unique<std::thread>([this]() {
        run();
    });

    THEMIS_INFO("Health/Error service started on {}:{}", 
               config_.bind_address, config_.port);
}

void HealthErrorService::stop() {
    if (!running_.load()) {
        return;
    }

    THEMIS_INFO("Stopping Health/Error service...");
    
    running_.store(false);
    
    // Close acceptor to immediately reject new connections
    if (acceptor_) {
        beast::error_code ec;
        acceptor_->close(ec);
        if (ec) {
            THEMIS_DEBUG("Accept close error: {}", ec.message());
        }
    }

    // Stop the io_context
    if (ioc_) {
        ioc_->stop();
    }
    
    // Wait for service thread to finish
    if (service_thread_ && service_thread_->joinable()) {
        service_thread_->join();
    }
    
    THEMIS_INFO("Health/Error service stopped");
}

void HealthErrorService::run() {
    try {
        if (!acceptor_) {
            running_.store(false);
            return;
        }
        auto& acceptor = *acceptor_;
        while (running_.load()) {
            // Use synchronous accept with timeout
            beast::error_code ec;
            tcp::socket socket(*ioc_);
            
            // Set non-blocking mode for accept with timeout
            acceptor.non_blocking(true, ec);
            if (ec) {
                THEMIS_ERROR("Failed to set non-blocking mode: {}", ec.message());
                break;
            }
            
            // Try to accept connection (non-blocking)
            acceptor.accept(socket, ec);
            
            if (!ec && running_.load()) {
                // Connection accepted successfully
                handleConnection(std::move(socket));
            } else if (ec == boost::asio::error::would_block) {
                // No pending connection, sleep briefly and retry
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } else if (ec && running_.load()) {
                // Real error occurred (but only log if we're still running)
                THEMIS_DEBUG("Accept error: {}", ec.message());
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    } catch (const std::exception& e) {
        if (running_.load()) {
            THEMIS_ERROR("Health/Error service error: {}", e.what());
        }
    }
}

void HealthErrorService::handleConnection(tcp::socket raw_socket) {
    try {
        // Wrap socket in tcp_stream so we can enforce per-operation timeouts and
        // prevent a slow or malicious client from blocking the service thread
        // indefinitely (Phase 8.2 — No-Timeout / Blocking-No-Timeout remediation).
        beast::tcp_stream stream(std::move(raw_socket));
        stream.expires_after(std::chrono::seconds(10));

        beast::flat_buffer buffer;
        http::request<http::string_body> req;

        // Set buffer size limit to prevent memory exhaustion (1MB max)
        buffer.max_size(1024 * 1024);

        // Read HTTP request; deadline enforced by stream.expires_after() above.
        beast::error_code ec;
        http::read(stream, buffer, req, ec);

        if (ec) {
            THEMIS_DEBUG("Failed to read request: {}", ec.message());
            return;
        }

        // Check request size limit
        if (req.body().size() > 1024 * 1024) {
            // Request too large, send 413 Payload Too Large
            http::response<http::string_body> error_res{http::status::payload_too_large, 11};
            error_res.set(http::field::server, "ThemisDB-Health");
            error_res.set(http::field::content_type, "application/json");
            error_res.body() = R"({"status":"error","message":"Request too large"})";
            error_res.prepare_payload();
            stream.expires_after(std::chrono::seconds(5));
            http::write(stream, error_res, ec);
            return;
        }

        // Handle request and get response
        auto res = handleRequest([[maybe_unused]] req);

        // Send response; refresh deadline for write phase.
        stream.expires_after(std::chrono::seconds(10));
        http::write(stream, res, ec);

        // Graceful shutdown
        beast::error_code shutdown_ec;
        stream.socket().shutdown(tcp::socket::shutdown_send, shutdown_ec);
    } catch (const std::exception& e) {
        THEMIS_DEBUG("Connection handling error: {}", e.what());
    }
}

http::response<http::string_body> HealthErrorService::handleRequest(
    const http::request<http::string_body>& req) 
{
    // Extract path and query parameters
    std::string target_str = std::string(req.target());
    std::string path = target_str;
    std::string query_string = {};
    
    auto qpos = path.find('?');
    if (qpos != std::string::npos) {
        query_string = path.substr(qpos + 1);
        path = path.substr(0, qpos);
    }

    // Parse query parameters
    auto parse_query = [](const std::string& query) -> json {
        json params = json::object();
        if (query.empty()) {
          return params;
        }
        
        std::istringstream ss(query);
        std::string param = {};
        while (std::getline(ss, param, '&')) {
            auto eq = param.find('=');
            if (eq != std::string::npos) {
                std::string key = param.substr(0, eq);
                std::string value = param.substr(eq + 1);
                params[key] = value;
            }
        }
        return params;
    };

    // Route request
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "ThemisDB-Health");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    try {
        // Health endpoints
        if (path == "/health") {
            return handleHealth();
        } else if (path == "/health/components") {
            return handleHealthComponents();
        }
        // Error endpoints - delegate to ErrorApiHandler
        else if (path == "/errors") {
            Request handler_req;
            handler_req.method = std::string([[maybe_unused]] http::to_string(req.method()));
            handler_req.path = path;
            handler_req.query = parse_query([[maybe_unused]] query_string);
            
            Response handler_res;
            error_handler_->handleGetErrors(handler_req, handler_res);
            
            res.result([[maybe_unused]] static_cast<http::status>(handler_res.status_code));
            res.body() = handler_res.body.dump();
            res.prepare_payload();
            return res;
        } else if (path == "/errors/categories") {
            Request handler_req;
            handler_req.method = std::string([[maybe_unused]] http::to_string(req.method()));
            handler_req.path = path;
            
            Response handler_res;
            error_handler_->handleGetCategories(handler_req, handler_res);
            
            res.result([[maybe_unused]] static_cast<http::status>(handler_res.status_code));
            res.body() = handler_res.body.dump();
            res.prepare_payload();
            return res;
        } else if (path == "/errors/search") {
            Request handler_req;
            handler_req.method = std::string([[maybe_unused]] http::to_string(req.method()));
            handler_req.path = path;
            handler_req.query = parse_query([[maybe_unused]] query_string);
            
            Response handler_res;
            error_handler_->handleSearchErrors(handler_req, handler_res);
            
            res.result([[maybe_unused]] static_cast<http::status>(handler_res.status_code));
            res.body() = handler_res.body.dump();
            res.prepare_payload();
            return res;
        } else if (path.rfind("/errors/", 0) == 0) {
            // Extract error code: /errors/:code
            std::string code_str = path.substr(8); // Skip "/errors/"
            
            Request handler_req;
            handler_req.method = std::string([[maybe_unused]] http::to_string(req.method()));
            handler_req.path = path;
            handler_req.params["code"] = code_str;
            
            Response handler_res;
            error_handler_->handleGetError(handler_req, handler_res);
            
            res.result([[maybe_unused]] static_cast<http::status>(handler_res.status_code));
            res.body() = handler_res.body.dump();
            res.prepare_payload();
            return res;
        } else {
            // 404 Not Found
            res.result(http::status::not_found);
            json error_body = {
                {"status", "error"},
                {"message", "Endpoint not found"},
                {"available_endpoints", json::array({
                    "/health",
                    "/health/components",
                    "/errors",
                    "/errors/:code",
                    "/errors/categories",
                    "/errors/search"
                })}
            };
            res.body() = error_body.dump();
            res.prepare_payload();
            return res;
        }
    } catch (const std::exception& e) {
        res.result(http::status::internal_server_error);
        json error_body = {
            {"status", "error"},
            {"message", e.what()}
        };
        res.body() = error_body.dump();
        res.prepare_payload();
        return res;
    }
}

http::response<http::string_body> HealthErrorService::handleHealth() {
    http::response<http::string_body> res{http::status::ok, 11};
    res.set(http::field::server, "ThemisDB-Health");
    res.set(http::field::content_type, "application/json");
    
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now = {};
#ifdef _WIN32
    gmtime_s(&tm_now, &time_t_now);
#else
    gmtime_r(&time_t_now, &tm_now);
#endif
    
    std::ostringstream timestamp = {};
    timestamp << std::put_time(&tm_now, "%Y-%m-%dT%H:%M:%SZ");
    
    json health = {
        {"status", "healthy"},
        {"timestamp", timestamp.str()},
        {"uptime_seconds", getUptimeSeconds()},
        {"service", "health_error_service"},
        {"version", "1.0.0"}
    };
    
    res.body() = health.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body> HealthErrorService::handleHealthComponents() {
    http::response<http::string_body> res{http::status::ok, 11};
    res.set(http::field::server, "ThemisDB-Health");
    res.set(http::field::content_type, "application/json");
    
    json components = {
        {"health_error_service", {
            {"status", "healthy"},
            {"uptime_seconds", getUptimeSeconds()},
            {"port", config_.port},
            {"bind_address", config_.bind_address}
        }},
        {"error_registry", {
            {"status", "healthy"},
            {"type", "in-memory"},
            {"dependencies", "none"}
        }}
    };
    
    res.body() = components.dump();
    res.prepare_payload();
    return res;
}

int64_t HealthErrorService::getUptimeSeconds() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    return duration.count();
}

} // namespace server
} // namespace themis
