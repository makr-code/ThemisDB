/**
 * @file wal_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

class AuthMiddleware;

// Forward declarations
class RocksDBWrapper;

namespace sharding {
class WALApplier;
class WALManager;
class ReplicationCoordinator;
}

namespace server {

/**
 * @brief Handler for Write-Ahead Log (WAL) Operations
 * 
 * This handler manages all WAL-related endpoints:
 * - POST /api/v1/wal/apply - Apply WAL entries for replication
 * 
 * Features:
 * - WAL entry application
 * - Replication support
 * - HMAC authentication
 * - Transaction replay
 * - Metrics tracking
 * 
 * Extracted from http_server.cpp (~220 lines) to improve maintainability.
 * 
 * Note: Governance headers are not applied by this handler. If needed for
 * compliance, they should be applied at a middleware layer or in the 
 * HttpServer before delegating to this handler.
 */
class WALApiHandler {
public:
    /**
     * @brief Construct a new WAL API Handler
     * 
     * @param storage Storage backend
     * @param wal_applier WAL applier for entry processing
     * @param wal_manager WAL manager
     * @param replication_coordinator Replication coordinator
     * @param auth Authentication/authorization middleware
     * @param wal_shared_secret Shared secret for X-WAL-Auth header
     * @param wal_hmac_secret HMAC secret for X-WAL-HMAC verification
     */
    WALApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<sharding::WALApplier> wal_applier,
        std::shared_ptr<sharding::WALManager> wal_manager,
        std::shared_ptr<sharding::ReplicationCoordinator> replication_coordinator,
        std::shared_ptr<themis::AuthMiddleware> auth,
        const std::string& wal_shared_secret = "",
        const std::string& wal_hmac_secret = ""
    );

    /**
     * @brief Handle POST /api/v1/wal/apply request
     * @param req HTTP request with WAL entries to apply
     * @return HTTP response with application status
     */
    http::response<http::string_body> handleApply(const http::request<http::string_body>& req);

    /**
     * @brief Get metrics for monitoring
     */
    uint64_t getApplySuccessCount() const { return wal_apply_success_.load(std::memory_order_relaxed); }
    uint64_t getApplyFailCount() const { return wal_apply_fail_.load(std::memory_order_relaxed); }
    uint64_t getApplyLatencyLe50ms() const { return wal_apply_latency_le_50ms_.load(std::memory_order_relaxed); }
    uint64_t getApplyLatencyLe200ms() const { return wal_apply_latency_le_200ms_.load(std::memory_order_relaxed); }
    uint64_t getApplyLatencyLe1000ms() const { return wal_apply_latency_le_1000ms_.load(std::memory_order_relaxed); }
    uint64_t getApplyLatencyGt1000ms() const { return wal_apply_latency_gt_1000ms_.load(std::memory_order_relaxed); }
    uint64_t getApplyLatencySumUs() const { return wal_apply_latency_sum_us_.load(std::memory_order_relaxed); }
    uint64_t getApplyLatencyCount() const { return wal_apply_latency_count_.load(std::memory_order_relaxed); }
    std::string getLastAppliedLsn() const { 
        /**
         * @brief TBD: Describe lock.
         * @param[in] wal_metrics_mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock<std::shared_mutex> lock(wal_metrics_mutex_);
        return wal_last_applied_lsn_; 
    }

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<sharding::WALApplier> wal_applier_;
    std::shared_ptr<sharding::WALManager> wal_manager_;
    std::shared_ptr<sharding::ReplicationCoordinator> replication_coordinator_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    
    // Authentication secrets
    std::string wal_shared_secret_;
    std::string wal_hmac_secret_;
    
    // Metrics
    std::atomic<uint64_t> wal_apply_success_{0};
    std::atomic<uint64_t> wal_apply_fail_{0};
    std::atomic<uint64_t> wal_apply_latency_le_50ms_{0};
    std::atomic<uint64_t> wal_apply_latency_le_200ms_{0};
    std::atomic<uint64_t> wal_apply_latency_le_1000ms_{0};
    std::atomic<uint64_t> wal_apply_latency_gt_1000ms_{0};
    std::atomic<uint64_t> wal_apply_latency_sum_us_{0};
    std::atomic<uint64_t> wal_apply_latency_count_{0};
    mutable std::shared_mutex wal_metrics_mutex_;
    std::string wal_last_applied_lsn_;

    /**
     * @brief Helper methods
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    /**
     * @brief TBD: Describe makeResponse.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
    
    /**
     * @brief TBD: Describe recordLatency.
     * @param[in] elapsed_us Input parameter.
     */
    void recordLatency(int64_t elapsed_us);
    /**
     * @brief TBD: Describe hmacSha256Hex.
     * @param[in] key Input parameter.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::string hmacSha256Hex(const std::string& key, const std::string& data);
    /**
     * @brief TBD: Describe timingSafeEqual.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return True on success.
     */
    bool timingSafeEqual(const std::string& a, const std::string& b);
};

} // namespace server
} // namespace themis
