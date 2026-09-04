/**
 * @file http_metrics_client.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/http_metrics_client.h"
#include "utils/logger.h"
#include "utils/retry_policy.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <thread>

using json = nlohmann::json;

namespace themis::rag::judge {

// ═══════════════════════════════════════════════════════════
// HTTP Metrics Client Implementation
// ═══════════════════════════════════════════════════════════

struct HTTPMetricsClient::Impl {
    std::unique_ptr<httplib::Client> http_client;
    
    Impl(const std::string& base_url, const HTTPMetricsClientConfig& config) {
        http_client = std::make_unique<httplib::Client>(base_url);
        
        // Configure timeouts
        http_client->set_read_timeout(config.timeout_ms / 1000, (config.timeout_ms % 1000) * 1000);
        http_client->set_write_timeout(config.timeout_ms / 1000, (config.timeout_ms % 1000) * 1000);
        http_client->set_connection_timeout(config.timeout_ms / 1000, (config.timeout_ms % 1000) * 1000);
        
        // Enable compression if requested
        if (config.enable_compression) {
            http_client->set_compress(true);
        }
        
    }
};

HTTPMetricsClient::HTTPMetricsClient(const HTTPMetricsClientConfig& config)
    : config_(config) {
    
    if (config_.endpoint_url.empty()) {
        throw std::invalid_argument("Endpoint URL cannot be empty");
    }
    
    impl_ = std::make_unique<Impl>(config_.endpoint_url, config_);
    
    THEMIS_INFO("HTTP Metrics Client initialized: {}", config_.endpoint_url);
}

HTTPMetricsClient::~HTTPMetricsClient() = default;

HTTPResponse HTTPMetricsClient::sendMetric(const QualityMetricPayload& metric) {
    std::string json_payload = serializeMetric(metric);
    return sendRawPayload(json_payload);
}

HTTPResponse HTTPMetricsClient::sendMetricsBatch(const std::vector<QualityMetricPayload>& metrics) {
    if (metrics.empty()) {
        HTTPResponse response;
        response.status_code = 400;
        response.success = false;
        response.error_message = "Empty metrics batch";
        return response;
    }
    
    // Split into batches if needed
    if (metrics.size() > static_cast<size_t>(config_.max_batch_size)) {
        THEMIS_WARN("Batch size {} exceeds max {}, splitting", metrics.size(), config_.max_batch_size);
        
        HTTPResponse last_response;
        for (size_t i = 0; i < metrics.size(); i += config_.max_batch_size) {
            size_t end = std::min(i + config_.max_batch_size, metrics.size());
            std::vector<QualityMetricPayload> batch(metrics.begin() + i, metrics.begin() + end);
            last_response = sendMetricsBatch(batch);
            
            if (!last_response.success) {
                return last_response;  // Return first failure
            }
        }
        return last_response;
    }
    
    std::string json_payload = serializeMetricsBatch(metrics);
    auto response = sendRawPayload(json_payload);
    updateStatistics(response, metrics.size());
    return response;
}

HTTPResponse HTTPMetricsClient::sendRawPayload(const std::string& json_payload) {
    std::unordered_map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    
    if (!config_.auth_token.empty()) {
        headers["Authorization"] = "Bearer " + config_.auth_token;
    }
    
    return requestWithRetry(HTTPMethod::POST, "/metrics", json_payload, headers);
}

HTTPResponse HTTPMetricsClient::request(
    HTTPMethod method,
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers) {
    
    return requestWithRetry(method, path, body, headers);
}

HTTPResponse HTTPMetricsClient::requestWithRetry(
    HTTPMethod method,
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers) {

    const themis::utils::RetryConfig retry_cfg{
        /* max_attempts       */ static_cast<uint32_t>(config_.max_retries + 1),
        /* initial_backoff_ms */ static_cast<uint32_t>(config_.retry_backoff_ms),
        /* max_backoff_ms     */ 30'000u,
        /* multiplier         */ 2.0,
        /* jitter_fraction    */ 0.0,
    };
    themis::utils::ExponentialBackoff backoff(retry_cfg);

    HTTPResponse response;

    for (int attempt = 0; attempt <= config_.max_retries; ++attempt) {
        auto start = std::chrono::steady_clock::now();

        // Prepare headers
        httplib::Headers http_headers;
        for (const auto& [key, value] : headers) {
            http_headers.emplace(key, value);
        }
        http_headers.emplace("User-Agent", config_.user_agent);

        // Perform request
        httplib::Result result;
        switch (method) {
            case HTTPMethod::GET:
                result = impl_->http_client->Get(path.c_str(), http_headers);
                break;
            case HTTPMethod::POST:
                result = impl_->http_client->Post(path.c_str(), http_headers, body, "application/json");
                break;
            case HTTPMethod::PUT:
                result = impl_->http_client->Put(path.c_str(), http_headers, body, "application/json");
                break;
            case HTTPMethod::DELETE_:
                result = impl_->http_client->Delete(path.c_str(), http_headers);
                break;
            default:
                response.success = false;
                response.error_message = "Unsupported HTTP method";
                return response;
        }

        auto end = std::chrono::steady_clock::now();
        response.latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        response.headers.clear();

        if (result) {
            response.status_code = result->status;
            response.body        = result->body;
            for (const auto& [key, value] : result->headers) {
                response.headers[key] = value;
            }
            response.success = (result->status >= 200 && result->status < 300);

            if (response.success) {
                THEMIS_DEBUG("HTTP {} {} succeeded with status {}",
                               method == HTTPMethod::POST ? "POST" : "GET", path, result->status);
                break; // done
            }

            if (attempt < config_.max_retries) {
                THEMIS_WARN("Request failed with status {}, retrying in {}ms (attempt {}/{})",
                              result->status, backoff.current_delay_ms(),
                              attempt + 1, config_.max_retries);
                {
                    std::unique_lock<std::shared_mutex> lock(stats_mutex_);
                    stats_.retries_attempted++;
                }
                backoff.wait();
            } else {
                response.error_message = "HTTP " + std::to_string(result->status);
                THEMIS_ERROR("HTTP {} {} failed with status {}",
                               method == HTTPMethod::POST ? "POST" : "GET", path, result->status);
            }
        } else {
            response.success       = false;
            response.status_code   = 0;
            response.error_message = httplib::to_string(result.error());

            if (attempt < config_.max_retries) {
                THEMIS_WARN("Request failed: {}, retrying in {}ms (attempt {}/{})",
                              response.error_message, backoff.current_delay_ms(),
                              attempt + 1, config_.max_retries);
                {
                    std::unique_lock<std::shared_mutex> lock(stats_mutex_);
                    stats_.retries_attempted++;
                }
                backoff.wait();
            } else {
                THEMIS_ERROR("HTTP {} {} failed: {}",
                               method == HTTPMethod::POST ? "POST" : "GET", path, response.error_message);
            }
        }
    }

    // Call callback if set
    if ([[maybe_unused]] request_callback_) {
        std::string method_str = {};
        switch (method) {
            case HTTPMethod::GET:    method_str = "GET";    break;
            case HTTPMethod::POST:   method_str = "POST";   break;
            case HTTPMethod::PUT:    method_str = "PUT";    break;
            case HTTPMethod::DELETE_: method_str = "DELETE"; break;
        }
        
        // Safely access callback with mutex protection
        {
            std::lock_guard<std::mutex> lock([[maybe_unused]] callback_mutex_);
            if ([[maybe_unused]] request_callback_) {  // Double-check pattern
                request_callback_(method_str, path, response.status_code, response.latency.count());
            }
        }
    }

    updateStatistics(response, 1);
    return response;
}

bool HTTPMetricsClient::isEndpointHealthy() {
    auto response = request(HTTPMethod::GET, "/health");
    return response.success;
}

HTTPMetricsClient::Statistics HTTPMetricsClient::getStatistics() const {
    std::shared_lock<std::shared_mutex> lock(stats_mutex_);
    return stats_;
}

void HTTPMetricsClient::resetStatistics() {
    std::unique_lock<std::shared_mutex> lock(stats_mutex_);
    stats_ = Statistics();
}

void HTTPMetricsClient::setRequestCallback([[maybe_unused]] RequestCallback callback) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] callback_mutex_);
    request_callback_ = std::move([[maybe_unused]] callback);
}

void HTTPMetricsClient::updateStatistics(const HTTPResponse& response, size_t metrics_count) {
    std::unique_lock<std::shared_mutex> lock(stats_mutex_);
    
    stats_.requests_sent++;
    stats_.metrics_sent += metrics_count;
    
    if (response.success) {
        stats_.requests_succeeded++;
    } else {
        stats_.requests_failed++;
    }
    
    stats_.total_latency += response.latency;
    
    if (stats_.requests_sent > 0) {
        stats_.avg_latency = stats_.total_latency / stats_.requests_sent;
    }
}

std::string HTTPMetricsClient::serializeMetric(const QualityMetricPayload& metric) {
    json j;
    j["query"] = metric.query;
    j["faithfulness_score"] = metric.faithfulness_score;
    j["relevance_score"] = metric.relevance_score;
    j["completeness_score"] = metric.completeness_score;
    j["coherence_score"] = metric.coherence_score;
    j["overall_score"] = metric.overall_score;
    j["decision"] = metric.decision;
    j["latency_ms"] = metric.latency_ms;
    j["mode"] = metric.mode;
    j["timestamp"] = metric.timestamp;
    
    if (!metric.metadata.empty()) {
        j["metadata"] = metric.metadata;
    }
    
    return j.dump();
}

std::string HTTPMetricsClient::serializeMetricsBatch(const std::vector<QualityMetricPayload>& metrics) {
    json j;
    j["metrics"] = json::array();
    
    for (const auto& metric : metrics) {
        json metric_json;
        metric_json["query"] = metric.query;
        metric_json["faithfulness_score"] = metric.faithfulness_score;
        metric_json["relevance_score"] = metric.relevance_score;
        metric_json["completeness_score"] = metric.completeness_score;
        metric_json["coherence_score"] = metric.coherence_score;
        metric_json["overall_score"] = metric.overall_score;
        metric_json["decision"] = metric.decision;
        metric_json["latency_ms"] = metric.latency_ms;
        metric_json["mode"] = metric.mode;
        metric_json["timestamp"] = metric.timestamp;
        
        if (!metric.metadata.empty()) {
            metric_json["metadata"] = metric.metadata;
        }
        
        j["metrics"].push_back(metric_json);
    }
    
    j["batch_size"] = metrics.size();
    j["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
    
    return j.dump();
}

// ═══════════════════════════════════════════════════════════
// HTTP Metrics Client Factory Implementation
// ═══════════════════════════════════════════════════════════

std::shared_ptr<HTTPMetricsClient> HTTPMetricsClientFactory::createLocalClient(const std::string& endpoint) {
    HTTPMetricsClientConfig config;
    config.endpoint_url = endpoint;
    config.verify_ssl = false;  // Disable SSL verification for local development
    config.max_retries = 1;     // Fewer retries for local
    config.timeout_ms = 3000;   // Shorter timeout
    
    return std::make_shared<HTTPMetricsClient>(config);
}

std::shared_ptr<HTTPMetricsClient> HTTPMetricsClientFactory::createProductionClient(
    const std::string& endpoint,
    const std::string& auth_token) {
    
    HTTPMetricsClientConfig config;
    config.endpoint_url = endpoint;
    config.auth_token = auth_token;
    config.verify_ssl = true;
    config.max_retries = 3;
    config.timeout_ms = 5000;
    config.enable_compression = true;
    
    return std::make_shared<HTTPMetricsClient>(config);
}

std::shared_ptr<HTTPMetricsClient> HTTPMetricsClientFactory::createCustomClient(const HTTPMetricsClientConfig& config) {
    return std::make_shared<HTTPMetricsClient>(config);
}

} // namespace themis::rag::judge
