/**
 * @file http_metrics_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>

namespace themis::rag::judge {

/**
 * @brief HTTP request method
 */
enum class HTTPMethod {
    GET,
    POST,
    PUT,
    DELETE_
};

/**
 * @brief HTTP response
 */
struct HTTPResponse {
    int status_code = 0;                  ///< HTTP status code
    std::string body;                 ///< Response body
    std::unordered_map<std::string, std::string> headers;  ///< Response headers
    bool success;                     ///< Whether request succeeded
    std::string error_message;        ///< Error message if failed
    std::chrono::milliseconds latency;  ///< Request latency
};

/**
 * @brief HTTP metrics client configuration
 */
struct HTTPMetricsClientConfig {
    std::string endpoint_url;         ///< Base URL for metrics endpoint
    int timeout_ms = 5000;            ///< Request timeout (5 seconds)
    int max_retries = 3;              ///< Maximum retry attempts
    int retry_backoff_ms = 1000;      ///< Initial retry backoff (exponential)
    bool verify_ssl = true;           ///< Verify SSL certificates
    std::string auth_token;           ///< Optional authentication token
    std::string user_agent = "ThemisDB-QC/1.0";  ///< User agent string
    
    // Advanced options
    bool enable_compression = true;   ///< Enable gzip compression
    int max_batch_size = 100;         ///< Maximum metrics per batch
    int connection_pool_size = 4;     ///< HTTP connection pool size
};

/**
 * @brief Quality metric payload
 */
struct QualityMetricPayload {
    std::string query;                ///< User query
    double faithfulness_score;        ///< Faithfulness score
    double relevance_score;           ///< Relevance score
    double completeness_score;        ///< Completeness score
    double coherence_score;           ///< Coherence score
    double overall_score;             ///< Overall quality score
    std::string decision;             ///< QC decision (ACCEPT/REJECT/WARN/RETRY)
    int latency_ms;                   ///< QC processing latency
    std::string mode;                 ///< QC mode (FAST/BALANCED/THOROUGH)
    std::string timestamp;            ///< ISO 8601 timestamp
    std::unordered_map<std::string, std::string> metadata;  ///< Additional metadata
};

/**
 * @brief HTTP Metrics Client
 * 
 * Sends quality control metrics to HTTP endpoints for continuous learning.
 * Features:
 * - Batched metric uploads
 * - Automatic retry with exponential backoff
 * - Connection pooling
 * - SSL/TLS support
 * - Authentication (Bearer token)
 * - Request/response logging
 */
class HTTPMetricsClient {
public:
    /**
     * @brief Construct client with configuration
     */
    explicit HTTPMetricsClient(const HTTPMetricsClientConfig& config);
    
    /**
     * @brief Destructor
     */
    ~HTTPMetricsClient();
    
    /**
     * @brief Send single metric
     * @param metric Quality metric to send
     * @return HTTP response
     */
    HTTPResponse sendMetric(const QualityMetricPayload& metric);
    
    /**
     * @brief Send batch of metrics
     * @param metrics Vector of quality metrics
     * @return HTTP response
     */
    HTTPResponse sendMetricsBatch(const std::vector<QualityMetricPayload>& metrics);
    
    /**
     * @brief Send raw JSON payload
     * @param json_payload JSON string
     * @return HTTP response
     */
    HTTPResponse sendRawPayload(const std::string& json_payload);
    
    /**
     * @brief Perform HTTP request
     * @param method HTTP method
     * @param path URL path (relative to base URL)
     * @param body Request body
     * @param headers Additional headers
     * @return HTTP response
     */
    HTTPResponse request(
        HTTPMethod method,
        const std::string& path,
        const std::string& body = "",
        const std::unordered_map<std::string, std::string>& headers = {}
    );
    
    /**
     * @brief Check if endpoint is reachable
     * @return true if endpoint responds to health check
     */
    bool isEndpointHealthy();
    
    /**
     * @brief Get client statistics
     */
    struct Statistics {
        size_t requests_sent = 0;
        size_t requests_succeeded = 0;
        size_t requests_failed = 0;
        size_t metrics_sent = 0;
        size_t retries_attempted = 0;
        std::chrono::milliseconds total_latency{0};
        std::chrono::milliseconds avg_latency{0};
    };
    
    Statistics getStatistics() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();
    
    /**
     * @brief Get configuration
     */
    const HTTPMetricsClientConfig& getConfig() const { return config_; }
    
    /**
     * @brief Set request callback (for logging/monitoring)
     */
    using RequestCallback = std::function<void(const std::string& method, const std::string& url, int status_code, int64_t latency_ms)>;
    void setRequestCallback(RequestCallback callback);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    HTTPMetricsClientConfig config_;
    Statistics stats_;
    mutable std::shared_mutex stats_mutex_;
    RequestCallback request_callback_;
    mutable std::mutex callback_mutex_;  // Protect request_callback_ access
    
    HTTPResponse requestWithRetry(
        HTTPMethod method,
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers
    );
    
    std::string serializeMetric(const QualityMetricPayload& metric);
    std::string serializeMetricsBatch(const std::vector<QualityMetricPayload>& metrics);
    
    void updateStatistics(const HTTPResponse& response, size_t metrics_count = 1);
};

/**
 * @brief Factory for creating pre-configured HTTP clients
 */
class HTTPMetricsClientFactory {
public:
    /**
     * @brief Create client for local development
     */
    static std::shared_ptr<HTTPMetricsClient> createLocalClient(const std::string& endpoint = "http://localhost:8080/metrics");
    
    /**
     * @brief Create client for production with authentication
     */
    static std::shared_ptr<HTTPMetricsClient> createProductionClient(
        const std::string& endpoint,
        const std::string& auth_token
    );
    
    /**
     * @brief Create client with custom configuration
     */
    static std::shared_ptr<HTTPMetricsClient> createCustomClient(const HTTPMetricsClientConfig& config);
};

} // namespace themis::rag::judge
