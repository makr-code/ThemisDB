#include "utils/http_client_pool.h"
#include <stdexcept>
#include <sstream>
#include <regex>

namespace themis {
namespace utils {

// ============================================================================
// URL Parsing
// ============================================================================

URLComponents parseURL(const std::string& url) {
    URLComponents components;
    
    // Regex: (https?)://([^:/]+)(?::(\d+))?(/.*)?
    std::regex url_regex(R"((https?)://([^:/]+)(?::(\d+))?(/.*)?)", std::regex::icase);
    std::smatch matches;
    
    if (!std::regex_match(url, matches, url_regex)) {
        throw std::invalid_argument("Invalid URL format: " + url);
    }
    
    components.protocol = matches[1].str();
    components.host = matches[2].str();
    
    if (matches[3].matched) {
        components.port = matches[3].str();
    } else {
        // Default ports
        components.port = (components.protocol == "https") ? "443" : "80";
    }
    
    if (matches[4].matched) {
        components.path = matches[4].str();
    } else {
        components.path = "/";
    }
    
    return components;
}

// ============================================================================
// HTTPClientPool Implementation
// ============================================================================

HTTPClientPool::HTTPClientPool(const Config& config)
    : config_(config)
{
}

HTTPClientPool::~HTTPClientPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
    }
    cv_.notify_all();
    clear();
}

std::future<HTTPResponse> HTTPClientPool::post(
    const std::string& url,
    const json& body,
    const std::unordered_map<std::string, std::string>& headers
) {
    return std::async(std::launch::async, [this, url, body, headers]() {
        auto client = acquireConnection();
        
        try {
            auto response = client->post(url, body, headers);
            releaseConnection(client);
            return response;
        } catch (...) {
            releaseConnection(client);
            throw;
        }
    });
}

std::future<HTTPResponse> HTTPClientPool::get(
    const std::string& url,
    const std::unordered_map<std::string, std::string>& headers
) {
    return std::async(std::launch::async, [this, url, headers]() {
        auto client = acquireConnection();
        
        try {
            auto response = client->get(url, headers);
            releaseConnection(client);
            return response;
        } catch (...) {
            releaseConnection(client);
            throw;
        }
    });
}

std::shared_ptr<HTTPClient> HTTPClientPool::acquireConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait for available connection or create new
    if (available_clients_.empty()) {
        if (all_clients_.size() < config_.max_connections) {
            auto client = createClient();
            all_clients_.push_back(client);
            return client;
        } else {
            // Wait for connection to become available
            cv_.wait(lock, [this] { 
                return !available_clients_.empty() || shutdown_; 
            });
            
            if (shutdown_) {
                throw std::runtime_error("HTTPClientPool is shutting down");
            }
        }
    }
    
    auto client = available_clients_.front();
    available_clients_.pop();
    return client;
}

void HTTPClientPool::releaseConnection(std::shared_ptr<HTTPClient> client) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        available_clients_.push(client);
    }
    cv_.notify_one();
}

std::shared_ptr<HTTPClient> HTTPClientPool::createClient() {
    return std::make_shared<BeastHTTPClient>(config_);
}

HTTPClientPool::Stats HTTPClientPool::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats stats;
    stats.total_connections = all_clients_.size();
    stats.available_connections = available_clients_.size();
    stats.in_use_connections = stats.total_connections - stats.available_connections;
    
    return stats;
}

void HTTPClientPool::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    while (!available_clients_.empty()) {
        available_clients_.pop();
    }
    all_clients_.clear();
}

// ============================================================================
// BeastHTTPClient Implementation
// ============================================================================

BeastHTTPClient::BeastHTTPClient(const HTTPClientPool::Config& config)
    : config_(config)
    , ssl_ctx_(ssl::context::tlsv12_client)
{
    // Configure SSL context
    ssl_ctx_.set_default_verify_paths();
    ssl_ctx_.set_verify_mode(ssl::verify_peer);
}

BeastHTTPClient::~BeastHTTPClient() {
}

HTTPResponse BeastHTTPClient::post(
    const std::string& url,
    const json& body,
    const std::unordered_map<std::string, std::string>& headers
) {
    return execute(http::verb::post, url, body.dump(), headers);
}

HTTPResponse BeastHTTPClient::get(
    const std::string& url,
    const std::unordered_map<std::string, std::string>& headers
) {
    return execute(http::verb::get, url, "", headers);
}

HTTPResponse BeastHTTPClient::execute(
    http::verb method,
    const std::string& url,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers
) {
    try {
        auto components = parseURL(url);
        
        // Resolve host
        tcp::resolver resolver(ioc_);
        auto const results = resolver.resolve(components.host, components.port);
        
        HTTPResponse response;
        
        if (components.is_https()) {
            // HTTPS request with SSL
            beast::ssl_stream<beast::tcp_stream> stream(ioc_, ssl_ctx_);
            
            // Set SNI Hostname (many HTTPS servers require this)
            if (!SSL_set_tlsext_host_name(stream.native_handle(), components.host.c_str())) {
                throw std::runtime_error("Failed to set SNI hostname");
            }
            
            // Set timeout for connection
            beast::get_lowest_layer(stream).expires_after(config_.connect_timeout);
            
            // Connect
            beast::get_lowest_layer(stream).connect(results);
            
            // Perform SSL handshake
            stream.handshake(ssl::stream_base::client);
            
            // Build HTTP request
            http::request<http::string_body> req{method, components.path, 11};
            req.set(http::field::host, components.host);
            req.set(http::field::user_agent, "ThemisDB-HTTPClientPool/1.0");
            
            // Add custom headers
            for (const auto& [key, value] : headers) {
                req.set(key, value);
            }
            
            // Add body for POST/PUT
            if (method == http::verb::post || method == http::verb::put) {
                req.body() = body;
                req.set(http::field::content_type, "application/json");
                req.prepare_payload();
            }
            
            // Set timeout for request
            beast::get_lowest_layer(stream).expires_after(config_.request_timeout);
            
            // Send request
            http::write(stream, req);
            
            // Receive response
            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);
            
            // Extract response
            response.status_code = static_cast<int>(res.result_int());
            response.body = res.body();
            
            for (auto const& field : res) {
                response.headers[std::string(field.name_string())] = std::string(field.value());
            }
            
            // Graceful shutdown
            beast::error_code ec;
            stream.shutdown(ec);
            // Ignore "stream truncated" error on shutdown
            if (ec && ec != beast::errc::not_connected) {
                // Log warning but don't throw
            }
            
        } else {
            // HTTP request (no SSL)
            beast::tcp_stream stream(ioc_);
            
            // Set timeout
            stream.expires_after(config_.connect_timeout);
            
            // Connect
            stream.connect(results);
            
            // Build HTTP request
            http::request<http::string_body> req{method, components.path, 11};
            req.set(http::field::host, components.host);
            req.set(http::field::user_agent, "ThemisDB-HTTPClientPool/1.0");
            
            // Add custom headers
            for (const auto& [key, value] : headers) {
                req.set(key, value);
            }
            
            // Add body for POST/PUT
            if (method == http::verb::post || method == http::verb::put) {
                req.body() = body;
                req.set(http::field::content_type, "application/json");
                req.prepare_payload();
            }
            
            // Set timeout for request
            stream.expires_after(config_.request_timeout);
            
            // Send request
            http::write(stream, req);
            
            // Receive response
            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);
            
            // Extract response
            response.status_code = static_cast<int>(res.result_int());
            response.body = res.body();
            
            for (auto const& field : res) {
                response.headers[std::string(field.name_string())] = std::string(field.value());
            }
            
            // Graceful shutdown
            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        }
        
        return response;
        
    } catch (const std::exception& e) {
        throw std::runtime_error("HTTP request failed: " + std::string(e.what()));
    }
}

} // namespace utils
} // namespace themis
