/**
 * @file vision_resource_monitor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/vision_config.h"
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <queue>
#include <thread>

namespace themis::llm {

/**
 * @brief Resource usage statistics
 */
struct VisionResourceUsage {
    virtual ~VisionResourceUsage() = default;
    size_t current_memory_mb = 0;
    size_t peak_memory_mb = 0;
    size_t current_vram_mb = 0;
    size_t peak_vram_mb = 0;
    size_t active_requests = 0;
    size_t queued_requests = 0;
    size_t loaded_models = 0;
    std::chrono::steady_clock::time_point start_time;
    
    // Statistics
    uint64_t total_requests = 0;
    uint64_t successful_requests = 0;
    uint64_t failed_requests = 0;
    uint64_t rejected_requests = 0;
    
    // Timing statistics
    double avg_inference_time_ms = 0.0;
    double max_inference_time_ms = 0.0;
    double min_inference_time_ms = 0.0;
    
    /**
     * @brief Get current resource utilization as percentage
     */
    double getMemoryUtilization(size_t limit_mb) const;
    double getVRAMUtilization(size_t limit_mb) const;
    double getRequestUtilization(size_t limit) const;
};

/**
 * @brief Rate limiter token bucket implementation
 */
class RateLimiter {
public:
    virtual ~RateLimiter() = default;
    RateLimiter(size_t rate_per_minute, size_t burst_size);
    
    /**
     * @brief Try to acquire a token
     * @return true if token acquired, false if rate limit exceeded
     */
    bool tryAcquire();
    
    /**
     * @brief Get number of available tokens
     */
    size_t availableTokens() const;
    
    /**
     * @brief Get time until next token available
     */
    std::chrono::milliseconds timeUntilNextToken() const;
    
    /**
     * @brief Reset the rate limiter
     */
    void reset();
    
private:
    void refillTokens();
    
    size_t capacity_ = 0;           ///< Bucket capacity (burst size)
    size_t refill_rate_ = 0;        ///< Tokens per minute
    mutable std::atomic<size_t> tokens_; ///< Available tokens
    mutable std::chrono::steady_clock::time_point last_refill_;
    mutable std::mutex mutex_;
};

/**
 * @brief Per-user quota tracker
 */
class QuotaTracker {
public:
    virtual ~QuotaTracker() = default;
    QuotaTracker(const VisionResourceQuota& quota);
    
    /**
     * @brief Check if user has quota available
     */
    bool hasQuotaAvailable(const std::string& user_id) const;
    
    /**
     * @brief Consume quota for a request
     */
    bool consumeQuota(const std::string& user_id, 
                     size_t requests = 1,
                     std::chrono::milliseconds inference_time = std::chrono::milliseconds(0),
                     size_t vram_mb_seconds = 0);
    
    /**
     * @brief Get remaining quota for a user
     */
    struct QuotaRemaining {
        size_t daily_requests_remaining = 0;
        size_t monthly_requests_remaining = 0;
        size_t inference_minutes_remaining = 0;
        size_t vram_hours_remaining = 0;
    };
    QuotaRemaining getRemainingQuota(const std::string& user_id) const;
    
    /**
     * @brief Reset quotas (called on reset period)
     */
    void resetQuotas();
    
private:
    struct UserQuota {
        size_t daily_requests_used = 0;
        size_t monthly_requests_used = 0;
        size_t inference_minutes_used = 0;
        size_t vram_hours_used = 0;
        std::chrono::steady_clock::time_point last_reset;
    };
    
    VisionResourceQuota quota_config_;
    std::unordered_map<std::string, UserQuota> user_quotas_;
    mutable std::mutex mutex_;
};

/**
 * @brief Vision resource monitor and manager
 * 
 * Monitors and enforces resource limits, rate limits, and quotas for vision processing.
 * Provides observability through metrics and audit logging.
 */
class VisionResourceMonitor {
public:
    explicit VisionResourceMonitor(std::shared_ptr<VisionConfig> config);
    ~VisionResourceMonitor();
    
    /**
     * @brief Initialize the monitor
     */
    bool initialize();
    
    /**
     * @brief Shutdown the monitor
     */
    void shutdown();
    
    // =====================================================
    // Request Management
    // =====================================================
    
    /**
     * @brief Check if a request can be accepted
     * @param user_id User identifier for quota tracking
     * @param estimated_memory_mb Estimated memory requirement
     * @return true if request can be accepted, false otherwise
     */
    bool canAcceptRequest(const std::string& user_id, size_t estimated_memory_mb = 0);
    
    /**
     * @brief Start tracking a request
     * @return Request tracking ID
     */
    uint64_t startRequest(const std::string& user_id, const std::string& model_id);
    
    /**
     * @brief Complete a request
     */
    void completeRequest(uint64_t request_id, bool success, 
                        std::chrono::milliseconds inference_time,
                        size_t memory_used_mb = 0);
    
    /**
     * @brief Reject a request (rate limit, quota, or resource limit exceeded)
     */
    void rejectRequest(const std::string& user_id, const std::string& reason);
    
    // =====================================================
    // Resource Tracking
    // =====================================================
    
    /**
     * @brief Update memory usage
     */
    void updateMemoryUsage(size_t memory_mb);
    
    /**
     * @brief Update VRAM usage
     */
    void updateVRAMUsage(size_t vram_mb);
    
    /**
     * @brief Register a model load
     */
    void registerModelLoad(const std::string& model_id, size_t memory_mb, size_t vram_mb);
    
    /**
     * @brief Register a model unload
     */
    void registerModelUnload(const std::string& model_id);
    
    // =====================================================
    // Monitoring & Statistics
    // =====================================================
    
    /**
     * @brief Get current resource usage
     */
    VisionResourceUsage getResourceUsage() const;
    
    /**
     * @brief Get rate limiter stats
     */
    struct RateLimiterStats {
        size_t available_tokens = 0;
        std::chrono::milliseconds time_until_next_token{0};
        uint64_t total_requests = 0;
        uint64_t rejected_requests = 0;
    };
    RateLimiterStats getRateLimiterStats() const;
    
    /**
     * @brief Get quota stats for a user
     */
    QuotaTracker::QuotaRemaining getUserQuota(const std::string& user_id) const;
    
    /**
     * @brief Export metrics (for Prometheus)
     */
    std::string exportMetrics() const;
    
    /**
     * @brief Get audit log entries
     */
    struct AuditEntry {
        std::chrono::system_clock::time_point timestamp;
        std::string event_type;
        std::string user_id;
        std::string model_id;
        std::string details;
        bool success = false;
    };
    std::vector<AuditEntry> getAuditLog(size_t max_entries = 100) const;
    
    // =====================================================
    // Lifecycle Management
    // =====================================================
    
    /**
     * @brief Check if system is healthy
     */
    bool isHealthy() const;
    
    /**
     * @brief Get health status details
     */
    struct HealthStatus {
        bool healthy = false;
        std::string status;  // "healthy", "degraded", "unhealthy"
        std::vector<std::string> issues;
        double memory_utilization_percent = 0.0;
        double vram_utilization_percent = 0.0;
        double request_utilization_percent = 0.0;
    };
    HealthStatus getHealthStatus() const;

private:
    // Configuration
    std::shared_ptr<VisionConfig> config_;
    
    // Resource tracking
    VisionResourceUsage usage_;
    mutable std::mutex usage_mutex_;
    
    // Rate limiting
    std::unique_ptr<RateLimiter> global_rate_limiter_;
    std::unordered_map<std::string, std::unique_ptr<RateLimiter>> user_rate_limiters_;
    mutable std::mutex rate_limiter_mutex_;
    
    // Quota tracking
    std::unique_ptr<QuotaTracker> quota_tracker_;
    
    // Request tracking
    struct RequestInfo {
        uint64_t request_id = 0;
        std::string user_id;
        std::string model_id;
        std::chrono::steady_clock::time_point start_time;
        size_t memory_allocated_mb = 0;
    };
    std::unordered_map<uint64_t, RequestInfo> active_requests_;
    std::atomic<uint64_t> next_request_id_{1};
    mutable std::mutex requests_mutex_;
    
    // Model tracking
    struct ModelInfo {
        std::string model_id;
        size_t memory_mb = 0;
        size_t vram_mb = 0;
        std::chrono::steady_clock::time_point load_time;
    };
    std::unordered_map<std::string, ModelInfo> loaded_models_;
    mutable std::mutex models_mutex_;
    
    // Audit logging
    std::queue<AuditEntry> audit_log_;
    mutable std::mutex audit_mutex_;
    static constexpr size_t MAX_AUDIT_ENTRIES = 10000;
    
    // Background tasks
    std::atomic<bool> running_{false};
    std::thread metrics_thread_;
    std::thread quota_reset_thread_;
    
    // Helper methods
    void metricsCollectionLoop();
    void quotaResetLoop();
    void logAuditEvent(const std::string& event_type, const std::string& user_id,
                      const std::string& model_id, const std::string& details, bool success);
    RateLimiter* getUserRateLimiter(const std::string& user_id);
};

} // namespace themis::llm
