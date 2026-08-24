/**
 * @file http_client_pool.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=13, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/http_client_pool.h"
#include "utils/thread_join_utils.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <sstream>
#include <regex>
#include <algorithm>

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
#ifdef HAVE_BOOST_BEAST
    , io_context_(std::make_shared<net::io_context>())
#endif
{
    // Initialize lock stripes
    stripes_.reserve(config_.lock_stripes);
    for (size_t i = 0; i < config_.lock_stripes; ++i) {
        stripes_.push_back(std::make_unique<LockStripe>());
    }
    
#ifdef HAVE_BOOST_BEAST
    // Start I/O thread pool
    io_threads_.reserve(config_.io_threads);
    for (size_t i = 0; i < config_.io_threads; ++i) {
        io_threads_.emplace_back([this]() {
            io_context_->run();
        });
    }
#endif
}

HTTPClientPool::~HTTPClientPool() {
    shutdown_.store(true);
    
    // Wake up all waiting threads
    for (auto& stripe : stripes_) {
        stripe->cv.notify_all();
    }
    
#ifdef HAVE_BOOST_BEAST
    // Stop io_context
    io_context_->stop();
#endif
    
    // Join I/O threads (bounded join; threads exit when io_context_ is stopped)
    for (auto& thread : io_threads_) {
        if (!joinThreadWithin(thread)) {
            spdlog::warn("HTTPClientPool: I/O thread did not exit within deadline; "
                         "a detached watcher will complete the join");
        }
    }
    
    clear();
}

std::future<HTTPResponse> HTTPClientPool::post(
    [[maybe_unused]] const std::string& url,
    [[maybe_unused]] const json& body,
    [[maybe_unused]] const std::unordered_map<std::string, std::string>& headers
) {
    auto promise = std::make_shared<std::promise<HTTPResponse>>();
    auto future = promise->get_future();
    
#ifdef HAVE_BOOST_BEAST
    // Post work to io_context instead of spawning new threads
    net::post(*io_context_, [this, url, body, headers, promise]() {
        std::shared_ptr<HTTPClient> client;
        try {
            client = acquireConnection();
            auto response = client->post(url, body, headers);
            releaseConnection(client);
            requests_served_++;
            promise->set_value(std::move(response));
        } catch (const std::exception &) {
            if (client) {
                releaseConnection(client);
            }
            promise->set_exception(std::current_exception());
        } catch (const std::string &) {
            if (client) {
                releaseConnection(client);
            }
            promise->set_exception(std::current_exception());
        } catch (const char *) {
            if (client) {
                releaseConnection(client);
            }
            promise->set_exception(std::current_exception());
        }
    });
#else
    promise->set_exception(std::make_exception_ptr(
        std::runtime_error("HTTPClientPool requires Boost.Beast (HAVE_BOOST_BEAST not defined)")));
#endif
    
    return future;
}

std::future<HTTPResponse> HTTPClientPool::get(
    [[maybe_unused]] const std::string& url,
    [[maybe_unused]] const std::unordered_map<std::string, std::string>& headers
) {
    auto promise = std::make_shared<std::promise<HTTPResponse>>();
    auto future = promise->get_future();
    
#ifdef HAVE_BOOST_BEAST
    // Post work to io_context instead of spawning new threads
    net::post(*io_context_, [this, url, headers, promise]() {
        std::shared_ptr<HTTPClient> client;
        try {
            client = acquireConnection();
            auto response = client->get(url, headers);
            releaseConnection(client);
            requests_served_++;
            promise->set_value(std::move(response));
        } catch (const std::exception &) {
            if (client) {
                releaseConnection(client);
            }
            promise->set_exception(std::current_exception());
        } catch (const std::string &) {
            if (client) {
                releaseConnection(client);
            }
            promise->set_exception(std::current_exception());
        } catch (const char *) {
            if (client) {
                releaseConnection(client);
            }
            promise->set_exception(std::current_exception());
        }
    });
#else
    promise->set_exception(std::make_exception_ptr(
        std::runtime_error("HTTPClientPool requires Boost.Beast (HAVE_BOOST_BEAST not defined)")));
#endif
    
    return future;
}

std::shared_ptr<HTTPClient> HTTPClientPool::acquireConnection() {
    // Use striped locking to reduce contention
    size_t stripe_idx = getStripeIndex();
    auto& stripe = stripes_[stripe_idx];
    
    std::unique_lock<std::mutex> lock(stripe->mutex);
    
    // Remove stale connections
    auto now = std::chrono::steady_clock::now();
    while (!stripe->connections.empty()) {
        auto& conn = stripe->connections.front();
        if (!conn->in_use && conn->isStale(config_.idle_timeout)) {
            stripe->connections.pop_front();
            total_connections_--;
            stale_removed_++;
        } else {
            break;
        }
    }
    
    // Try to find an available connection
    for (auto& conn : stripe->connections) {
        if (!conn->in_use) {
            conn->in_use = true;
            conn->last_used = now;
            connections_reused_.fetch_add(1, std::memory_order_relaxed);
            return conn->client;
        }
    }
    
    // Create new connection if under limit
    // Use atomic increment to avoid race condition
    size_t current_count = total_connections_.fetch_add(1);
    if (current_count < config_.max_connections) {
        auto client = createClient();
        auto pooled = std::make_shared<PooledConnection>();
        pooled->client = client;
        pooled->in_use = true;
        pooled->last_used = now;
        stripe->connections.push_back(pooled);
        connections_created_.fetch_add(1, std::memory_order_relaxed);
        return client;
    } else {
        // Exceeded limit, roll back the increment
        total_connections_--;
    }
    
    // Wait for connection with timeout
    auto deadline = now + config_.acquire_timeout;
    while (true) {
        if (shutdown_.load()) {
            throw std::runtime_error("HTTPClientPool is shutting down");
        }
        
        // Check for available connection
        for (auto& conn : stripe->connections) {
            if (!conn->in_use) {
                conn->in_use = true;
                conn->last_used = std::chrono::steady_clock::now();
                connections_reused_.fetch_add(1, std::memory_order_relaxed);
                return conn->client;
            }
        }
        
        // Wait with timeout
        auto wait_status = stripe->cv.wait_until(lock, deadline);
        if (wait_status == std::cv_status::timeout) {
            acquire_timeouts_++;
            throw std::runtime_error("Timeout waiting for available connection");
        }
    }
}

void HTTPClientPool::releaseConnection(std::shared_ptr<HTTPClient> client) {
    // Find the connection in any stripe and mark as not in use
    for (auto& stripe : stripes_) {
        std::lock_guard<std::mutex> lock(stripe->mutex);
        for (auto& conn : stripe->connections) {
            if (conn->client == client) {
                conn->in_use = false;
                conn->last_used = std::chrono::steady_clock::now();
                stripe->cv.notify_one();
                return;
            }
        }
    }
}

std::shared_ptr<HTTPClient> HTTPClientPool::createClient() {
#ifdef HAVE_BOOST_BEAST
    return std::make_shared<BeastHTTPClient>(config_, io_context_);
#else
    throw std::runtime_error("HTTPClientPool requires Boost.Beast (HAVE_BOOST_BEAST not defined)");
#endif
}

HTTPClientPool::Stats HTTPClientPool::getStats() const {
    Stats stats;
    stats.total_connections = total_connections_.load();
    stats.stale_connections_removed = stale_removed_.load();
    stats.acquire_timeouts = acquire_timeouts_.load();
    stats.requests_served = requests_served_.load();
    stats.connections_created = connections_created_.load();
    stats.connections_reused = connections_reused_.load();
    
    size_t in_use = 0;
    for (const auto& stripe : stripes_) {
        std::lock_guard<std::mutex> lock(stripe->mutex);
        for (const auto& conn : stripe->connections) {
            if (conn->in_use) {
                in_use++;
            }
        }
    }
    
    stats.in_use_connections = in_use;
    stats.available_connections = stats.total_connections - in_use;
    
    return stats;
}

void HTTPClientPool::clear() {
    for (auto& stripe : stripes_) {
        std::lock_guard<std::mutex> lock(stripe->mutex);
        stripe->connections.clear();
    }
    total_connections_.store(0);
}

void HTTPClientPool::pruneStaleConnections() {
    for (auto& stripe : stripes_) {
        std::lock_guard<std::mutex> lock(stripe->mutex);
        auto it = stripe->connections.begin();
        while (it != stripe->connections.end()) {
            if (!(*it)->in_use && (*it)->isStale(config_.idle_timeout)) {
                it = stripe->connections.erase(it);
                total_connections_--;
                stale_removed_++;
            } else {
                ++it;
            }
        }
    }
}

size_t HTTPClientPool::getStripeIndex() const {
    // Round-robin distribution across stripes
    return round_robin_.fetch_add(1, std::memory_order_relaxed) % stripes_.size();
}

void HTTPClientPool::warmup(size_t num_connections) {
    if (shutdown_.load()) {
        return;
    }
    
    // Cap at max connections
    num_connections = std::min(num_connections, config_.max_connections);
    
    // Distribute connections across stripes
    size_t connections_per_stripe = num_connections / stripes_.size();
    size_t remainder = num_connections % stripes_.size();
    
    for (size_t i = 0; i < stripes_.size(); ++i) {
        auto& stripe = stripes_[i];
        std::lock_guard<std::mutex> lock(stripe->mutex);
        
        size_t target_count = connections_per_stripe + (i < remainder ? 1 : 0);
        size_t current_count = stripe->connections.size();
        
        // Create additional connections up to target
        for (size_t j = current_count; j < target_count; ++j) {
            // Use atomic reserve like acquireConnection to avoid exceeding max
            size_t current_total = total_connections_.fetch_add(1);
            if (current_total >= config_.max_connections) {
                total_connections_.fetch_sub(1);  // Roll back
                break;
            }
            
            try {
                auto client = createClient();
                auto pooled = std::make_shared<PooledConnection>();
                pooled->client = client;
                pooled->in_use = false;
                pooled->last_used = std::chrono::steady_clock::now();
                
                stripe->connections.push_back(pooled);
                connections_created_.fetch_add(1, std::memory_order_relaxed);
            } catch (const std::exception &) {
                total_connections_.fetch_sub(1);  // Roll back on failure
                // Stop warmup on first failure to avoid cascading errors
                break;
            } catch (const std::string &) {
                total_connections_.fetch_sub(1);  // Roll back on failure
                // Stop warmup on first failure to avoid cascading errors
                break;
            } catch (const char *) {
                total_connections_.fetch_sub(1);  // Roll back on failure
                // Stop warmup on first failure to avoid cascading errors
                break;
            }
        }
    }
}

// ============================================================================
// BeastHTTPClient Implementation
// ============================================================================

#ifdef HAVE_BOOST_BEAST

BeastHTTPClient::BeastHTTPClient(const HTTPClientPool::Config& config, std::shared_ptr<net::io_context> ioc)
    : config_(config)
    , ioc_(ioc)
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
        tcp::resolver resolver(*ioc_);
        auto const results = resolver.resolve(components.host, components.port);
        
        HTTPResponse response;
        
        if (components.is_https()) {
            // HTTPS request with SSL
            beast::ssl_stream<beast::tcp_stream> stream(*ioc_, ssl_ctx_);
            
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
            beast::tcp_stream stream(*ioc_);
            
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

#endif // HAVE_BOOST_BEAST

} // namespace utils
} // namespace themis
