/**
 * @file vision_resource_monitor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct VisionResourceUsage {
    /**
     * @brief Vision Resource Usage.
     * @return Return value.
     */
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
     * @brief Get Memory Utilization.
     * @param[in] limit_mb Input parameter.
     * @return Return value.
     */
    double getMemoryUtilization(size_t limit_mb) const;
    /**
     * @brief Get VRAMUtilization.
     * @param[in] limit_mb Input parameter.
     * @return Return value.
     */
    double getVRAMUtilization(size_t limit_mb) const;
    /**
     * @brief Get Request Utilization.
     * @param[in] limit Input parameter.
     * @return Return value.
     */
    double getRequestUtilization(size_t limit) const;
};

class RateLimiter {
public:
    /**
     * @brief Rate Limiter.
     * @return Return value.
     */
    virtual ~RateLimiter() = default;
    RateLimiter(size_t rate_per_minute, size_t burst_size);
    
    /**
     * @brief Try Acquire.
     * @return True when the operation succeeds.
     */
    bool tryAcquire();
    
    /**
     * @brief Available Tokens.
     * @return Return value.
     */
    size_t availableTokens() const;
    
    /**
     * @brief Time Until Next Token.
     * @return Return value.
     */
    std::chrono::milliseconds timeUntilNextToken() const;
    
    /**
     * @brief Reset the modification detection flag.
     */
    void reset();
    
private:
    /**
     * @brief Refill Tokens.
     */
    void refillTokens();
    
    size_t capacity_ = 0;           ///< Bucket capacity (burst size)
    size_t refill_rate_ = 0;        ///< Tokens per minute
    mutable std::atomic<size_t> tokens_; ///< Available tokens
    mutable std::chrono::steady_clock::time_point last_refill_;
    mutable std::mutex mutex_;
};

class QuotaTracker {
public:
    /**
     * @brief Quota Tracker.
     * @return Return value.
     */
    virtual ~QuotaTracker() = default;
    QuotaTracker(const VisionResourceQuota& quota);
    
    /**
     * @brief Has Quota Available.
     * @param[in] user_id Identifier of the user.
     * @return True when the operation succeeds.
     */
    bool hasQuotaAvailable(const std::string& user_id) const;
    
    bool consumeQuota(const std::string& user_id, 
                     size_t requests = 1,
                     std::chrono::milliseconds inference_time = std::chrono::milliseconds(0),
                     size_t vram_mb_seconds = 0);
    
    struct QuotaRemaining {
        size_t daily_requests_remaining = 0;
        size_t monthly_requests_remaining = 0;
        size_t inference_minutes_remaining = 0;
        size_t vram_hours_remaining = 0;
    };
    /**
     * @brief Get Remaining Quota.
     * @param[in] user_id Identifier of the user.
     * @return Return value.
     */
    QuotaRemaining getRemainingQuota(const std::string& user_id) const;
    
    /**
     * @brief Reset Quotas.
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

class VisionResourceMonitor {
public:
    /**
     * @brief Vision Resource Monitor.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit VisionResourceMonitor(std::shared_ptr<VisionConfig> config);
    ~VisionResourceMonitor();
    
    /**
     * @brief Initialize.
     * @return True when the operation succeeds.
     */
    bool initialize();
    
    /**
     * @brief Shutdown.
     */
    void shutdown();
    
    // =====================================================
    // Request Management
    // =====================================================
    
    bool canAcceptRequest(const std::string& user_id, size_t estimated_memory_mb = 0);
    
    /**
     * @brief Start Request.
     * @param[in] user_id Identifier of the user.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    uint64_t startRequest(const std::string& user_id, const std::string& model_id);
    
    void completeRequest(uint64_t request_id, bool success, 
                        std::chrono::milliseconds inference_time,
                        size_t memory_used_mb = 0);
    
    /**
     * @brief Reject Request.
     * @param[in] user_id Identifier of the user.
     * @param[in] reason Input parameter.
     */
    void rejectRequest(const std::string& user_id, const std::string& reason);
    
    // =====================================================
    // Resource Tracking
    // =====================================================
    
    /**
     * @brief Update Memory Usage.
     * @param[in] memory_mb Input parameter.
     */
    void updateMemoryUsage(size_t memory_mb);
    
    /**
     * @brief Update VRAMUsage.
     * @param[in] vram_mb Input parameter.
     */
    void updateVRAMUsage(size_t vram_mb);
    
    /**
     * @brief Register Model Load.
     * @param[in] model_id Identifier of the model.
     * @param[in] memory_mb Input parameter.
     * @param[in] vram_mb Input parameter.
     */
    void registerModelLoad(const std::string& model_id, size_t memory_mb, size_t vram_mb);
    
    /**
     * @brief Register Model Unload.
     * @param[in] model_id Identifier of the model.
     */
    void registerModelUnload(const std::string& model_id);
    
    // =====================================================
    // Monitoring & Statistics
    // =====================================================
    
    /**
     * @brief Get Resource Usage.
     * @return Return value.
     */
    VisionResourceUsage getResourceUsage() const;
    
    struct RateLimiterStats {
        size_t available_tokens = 0;
        std::chrono::milliseconds time_until_next_token{0};
        uint64_t total_requests = 0;
        uint64_t rejected_requests = 0;
    };
    /**
     * @brief Get Rate Limiter Stats.
     * @return Return value.
     */
    RateLimiterStats getRateLimiterStats() const;
    
    /**
     * @brief Get User Quota.
     * @param[in] user_id Identifier of the user.
     * @return Return value.
     */
    QuotaTracker::QuotaRemaining getUserQuota(const std::string& user_id) const;
    
    /**
     * @brief Export Metrics.
     * @return Return value.
     */
    std::string exportMetrics() const;
    
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
     * @brief Is Healthy.
     * @return True when the operation succeeds.
     */
    bool isHealthy() const;
    
    struct HealthStatus {
        bool healthy = false;
        std::string status;  // "healthy", "degraded", "unhealthy"
        std::vector<std::string> issues;
        double memory_utilization_percent = 0.0;
        double vram_utilization_percent = 0.0;
        double request_utilization_percent = 0.0;
    };
    /**
     * @brief Get Health Status.
     * @return Return value.
     */
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
    /**
     * @brief Metrics Collection Loop.
     */
    void metricsCollectionLoop();
    /**
     * @brief Quota Reset Loop.
     */
    void quotaResetLoop();
    /**
     * @brief Log Audit Event.
     * @param[in] event_type Input parameter.
     * @param[in] user_id Identifier of the user.
     * @param[in] model_id Identifier of the model.
     * @param[in] details Input parameter.
     * @param[in] success Input parameter.
     */
    void logAuditEvent(const std::string& event_type, const std::string& user_id,
                      const std::string& model_id, const std::string& details, bool success);
    /**
     * @brief Get User Rate Limiter.
     * @param[in] user_id Identifier of the user.
     * @return Pointer to the result.
     */
    RateLimiter* getUserRateLimiter(const std::string& user_id);
};

} // namespace themis::llm
