/**
 * @file vision_resource_monitor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=17, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/vision_resource_monitor.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <sstream>
#include <iomanip>
#include <algorithm>


namespace themis::llm {

// =====================================================
// VisionResourceUsage Implementation
// =====================================================

double VisionResourceUsage::getMemoryUtilization([[maybe_unused]] size_t limit_mb) const {
    if (limit_mb == 0) {
        return 0.0;
    }
    return (static_cast<double>(current_memory_mb) / static_cast<double>(limit_mb)) * 100.0;
}

double VisionResourceUsage::getVRAMUtilization([[maybe_unused]] size_t limit_mb) const {
    if (limit_mb == 0) {
        return 0.0;
    }
    return (static_cast<double>(current_vram_mb) / static_cast<double>(limit_mb)) * 100.0;
}

double VisionResourceUsage::getRequestUtilization([[maybe_unused]] size_t limit) const {
    if (limit == 0) {
        return 0.0;
    }
    return (static_cast<double>(active_requests) / static_cast<double>(limit)) * 100.0;
}

// =====================================================
// RateLimiter Implementation
// =====================================================

RateLimiter::RateLimiter(size_t rate_per_minute, size_t burst_size)
    : capacity_(burst_size)
    , refill_rate_(rate_per_minute)
    , tokens_(burst_size)
    , last_refill_(std::chrono::steady_clock::now())
{
}

bool RateLimiter::tryAcquire() {
    std::scoped_lock<std::mutex> lock(mutex_);
    
    refillTokens();
    
    if (tokens_.load(std::memory_order_acquire) > 0) {
        tokens_.fetch_sub(1, std::memory_order_release);
        return true;
    }
    
    return false;
}

size_t RateLimiter::availableTokens() const {
    std::scoped_lock<std::mutex> lock(mutex_);
    // mutable last_refill_ und tokens_ erlauben Update in const-Methode
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill_);
    if (elapsed.count() > 0) {
        auto tokens_to_add = static_cast<size_t>((refill_rate_ * elapsed.count()) / 60000);
        if (tokens_to_add > 0) {
            size_t current = tokens_.load(std::memory_order_relaxed);
            size_t new_tokens = std::min(capacity_, current + tokens_to_add);
            tokens_.store(new_tokens, std::memory_order_release);
            last_refill_ = now;
        }
    }
    return tokens_.load(std::memory_order_acquire);
}

std::chrono::milliseconds RateLimiter::timeUntilNextToken() const {
    std::scoped_lock<std::mutex> lock(mutex_);
    
    if (tokens_.load(std::memory_order_acquire) > 0) {
        return std::chrono::milliseconds(0);
    }
    
    // Calculate time for next token based on refill rate
    auto tokens_per_ms = static_cast<double>(refill_rate_) / 60000.0; // per millisecond
    if (tokens_per_ms <= 0) {
        return std::chrono::milliseconds(60000); // 1 minute
    }
    
    auto ms_per_token = 1.0 / tokens_per_ms;
    return std::chrono::milliseconds(static_cast<int64_t>(ms_per_token));
}

void RateLimiter::reset() {
    std::scoped_lock<std::mutex> lock(mutex_);
    tokens_ = capacity_;
    last_refill_ = std::chrono::steady_clock::now();
}

void RateLimiter::refillTokens() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill_);
    
    if (elapsed.count() > 0) {
        // Calculate tokens to add based on elapsed time
        auto tokens_to_add = static_cast<size_t>((refill_rate_ * elapsed.count()) / 60000); // 60000ms = 1 minute
        
        if (tokens_to_add > 0) {
            size_t current = tokens_.load();
            size_t new_tokens = std::min(capacity_, current + tokens_to_add);
            tokens_.store(new_tokens);
            last_refill_ = now;
        }
    }
}

// =====================================================
// QuotaTracker Implementation
// =====================================================

QuotaTracker::QuotaTracker(const VisionResourceQuota& quota)
    : quota_config_(quota)
{
}

bool QuotaTracker::hasQuotaAvailable(const std::string& user_id) const {
    if (!quota_config_.enabled) {
        return true;
    }
    
    std::scoped_lock<std::mutex> lock(mutex_);
    
    auto it = user_quotas_.find(user_id);
    if (it == user_quotas_.end()) {
        return true; // New user, has quota
    }
    
    const auto& quota = it->second;
    
    // Check daily quota
    if (quota.daily_requests_used >= quota_config_.daily_requests) {
        return false;
    }
    
    // Check monthly quota
    if (quota.monthly_requests_used >= quota_config_.monthly_requests) {
        return false;
    }
    
    return true;
}

bool QuotaTracker::consumeQuota(const std::string& user_id, 
                                size_t requests,
                                std::chrono::milliseconds inference_time,
                                size_t vram_mb_seconds) {
    if (!quota_config_.enabled) {
        return true;
    }
    
    std::scoped_lock<std::mutex> lock(mutex_);
    
    auto& quota = user_quotas_[user_id];
    
    // Initialize if new user
    if (quota.last_reset.time_since_epoch().count() == 0) {
        quota.last_reset = std::chrono::steady_clock::now();
    }
    
    // Check if we can consume
    if (quota.daily_requests_used + requests > quota_config_.daily_requests) {
        if (quota_config_.enforcement == "hard") {
            return false;
        }
        spdlog::warn("User {} exceeded daily request quota (soft limit)", user_id);
    }
    
    if (quota.monthly_requests_used + requests > quota_config_.monthly_requests) {
        if (quota_config_.enforcement == "hard") {
            return false;
        }
        spdlog::warn("User {} exceeded monthly request quota (soft limit)", user_id);
    }
    
    // Consume quota
    quota.daily_requests_used += requests;
    quota.monthly_requests_used += requests;
    quota.inference_minutes_used += std::chrono::duration_cast<std::chrono::minutes>(inference_time).count();
    quota.vram_hours_used += vram_mb_seconds / 3600; // Convert to hours
    
    return true;
}

QuotaTracker::QuotaRemaining QuotaTracker::getRemainingQuota(const std::string& user_id) const {
    std::scoped_lock<std::mutex> lock(mutex_);
    
    auto it = user_quotas_.find(user_id);
    if (it == user_quotas_.end()) {
        // New user, full quota available
        return QuotaRemaining{
            .daily_requests_remaining = quota_config_.daily_requests,
            .monthly_requests_remaining = quota_config_.monthly_requests,
            .inference_minutes_remaining = quota_config_.total_inference_minutes,
            .vram_hours_remaining = quota_config_.total_vram_hours
        };
    }
    
    const auto& quota = it->second;
    
    return QuotaRemaining{
        .daily_requests_remaining = quota_config_.daily_requests > quota.daily_requests_used ? 
            quota_config_.daily_requests - quota.daily_requests_used : 0,
        .monthly_requests_remaining = quota_config_.monthly_requests > quota.monthly_requests_used ? 
            quota_config_.monthly_requests - quota.monthly_requests_used : 0,
        .inference_minutes_remaining = quota_config_.total_inference_minutes > quota.inference_minutes_used ? 
            quota_config_.total_inference_minutes - quota.inference_minutes_used : 0,
        .vram_hours_remaining = quota_config_.total_vram_hours > quota.vram_hours_used ? 
            quota_config_.total_vram_hours - quota.vram_hours_used : 0
    };
}

void QuotaTracker::resetQuotas() {
    std::scoped_lock<std::mutex> lock(mutex_);
    
    for (auto& pair : user_quotas_) {
        if (quota_config_.reset_period == "daily") {
            pair.second.daily_requests_used = 0;
        } else if (quota_config_.reset_period == "monthly") {
            pair.second.daily_requests_used = 0;
            pair.second.monthly_requests_used = 0;
        }
        pair.second.last_reset = std::chrono::steady_clock::now();
    }
    
    spdlog::info("Vision quotas reset (period: {})", quota_config_.reset_period);
}

// =====================================================
// VisionResourceMonitor Implementation
// =====================================================

VisionResourceMonitor::VisionResourceMonitor(std::shared_ptr<VisionConfig> config)
    : config_(config)
{
    usage_.start_time = std::chrono::steady_clock::now();
    usage_.min_inference_time_ms = std::numeric_limits<double>::max();
}

VisionResourceMonitor::~VisionResourceMonitor() {
    shutdown();
}

bool VisionResourceMonitor::initialize() {
    spdlog::info("Initializing vision resource monitor");
    
    // Initialize rate limiter
    const auto& rate_limits = config_->getRateLimits();
    if (rate_limits.enabled) {
        global_rate_limiter_ = std::make_unique<RateLimiter>(
            rate_limits.requests_per_minute,
            rate_limits.burst_size
        );
        spdlog::info("  - Global rate limiter: {} req/min, burst: {}", 
                    rate_limits.requests_per_minute, rate_limits.burst_size);
    }
    
    // Initialize quota tracker
    const auto& quota = config_->getResourceQuota();
    if (quota.enabled) {
        quota_tracker_ = std::make_unique<QuotaTracker>(quota);
        spdlog::info("  - Quota tracking: {} daily, {} monthly requests",
                    quota.daily_requests, quota.monthly_requests);
    }
    
    // Start background threads
    running_ = true;
    
    if (config_->getMonitoringConfig().enabled) {
        metrics_thread_ = std::thread(&VisionResourceMonitor::metricsCollectionLoop, this);
        spdlog::info("  - Metrics collection thread started");
    }
    
    if (quota_tracker_) {
        quota_reset_thread_ = std::thread(&VisionResourceMonitor::quotaResetLoop, this);
        spdlog::info("  - Quota reset thread started");
    }
    
    spdlog::info("Vision resource monitor initialized successfully");
    return true;
}

void VisionResourceMonitor::shutdown() {
    if (running_) {
        spdlog::info("Shutting down vision resource monitor");
        running_ = false;
        
        if (metrics_thread_.joinable()) {
            // thread_join_no_timeout (W4): bounded join via joinThreadWithin
            if (!themis::utils::joinThreadWithin(metrics_thread_)) {
                THEMIS_WARN("[VisionResourceMonitor] thread did not finish within shutdown deadline; detaching.");
            }
        }
        
        if (quota_reset_thread_.joinable()) {
            // thread_join_no_timeout (W4): bounded join via joinThreadWithin
            if (!themis::utils::joinThreadWithin(quota_reset_thread_)) {
                THEMIS_WARN("[VisionResourceMonitor] thread did not finish within shutdown deadline; detaching.");
            }
        }
        
        spdlog::info("Vision resource monitor shutdown complete");
    }
}

bool VisionResourceMonitor::canAcceptRequest(const std::string& user_id, size_t estimated_memory_mb) {
    const auto& limits = config_->getResourceLimits();
    
    // Check global rate limit
    if (global_rate_limiter_ && !global_rate_limiter_->tryAcquire()) {
        logAuditEvent("rate_limit_exceeded", user_id, "", "Global rate limit", false);
        return false;
    }
    
    // Check per-user rate limit
    auto* user_limiter = getUserRateLimiter(user_id);
    if (user_limiter && !user_limiter->tryAcquire()) {
        logAuditEvent("rate_limit_exceeded", user_id, "", "User rate limit", false);
        return false;
    }
    
    // Check quota
    if (quota_tracker_ && !quota_tracker_->hasQuotaAvailable(user_id)) {
        logAuditEvent("quota_exceeded", user_id, "", "User quota exhausted", false);
        return false;
    }
    
    // Check resource limits
    std::scoped_lock<std::mutex> lock(usage_mutex_);
    
    if (usage_.active_requests >= limits.max_concurrent_requests) {
        logAuditEvent("resource_limit_exceeded", user_id, "", "Max concurrent requests", false);
        return false;
    }
    
    if (usage_.queued_requests >= limits.max_queue_size) {
        logAuditEvent("resource_limit_exceeded", user_id, "", "Queue full", false);
        return false;
    }
    
    if (estimated_memory_mb > 0 && 
        usage_.current_memory_mb + estimated_memory_mb > limits.max_memory_mb) {
        logAuditEvent("resource_limit_exceeded", user_id, "", "Memory limit", false);
        return false;
    }
    
    return true;
}

uint64_t VisionResourceMonitor::startRequest(const std::string& user_id, const std::string& model_id) {
    uint64_t request_id = next_request_id_++;
    
    {
        std::scoped_lock<std::mutex> lock(requests_mutex_);
        RequestInfo info;
        info.request_id = request_id;
        info.user_id = user_id;
        info.model_id = model_id;
        info.start_time = std::chrono::steady_clock::now();
        info.memory_allocated_mb = 0;
        active_requests_[request_id] = info;
    }
    
    {
        std::scoped_lock<std::mutex> lock(usage_mutex_);
        usage_.active_requests++;
        usage_.total_requests++;
    }
    
    logAuditEvent("request_started", user_id, model_id, "Request ID: " + std::to_string(request_id), true);
    
    return request_id;
}

void VisionResourceMonitor::completeRequest(uint64_t request_id, bool success,
                                           std::chrono::milliseconds inference_time,
                                           size_t memory_used_mb) {
    RequestInfo info;
    
    {
        std::scoped_lock<std::mutex> lock(requests_mutex_);
        auto it = active_requests_.find(request_id);
        if (it != active_requests_.end()) {
            info = it->second;
            active_requests_.erase(it);
        } else {
            spdlog::warn("Attempted to complete unknown request: {}", request_id);
            return;
        }
    }
    
    {
        std::scoped_lock<std::mutex> lock(usage_mutex_);
        if (usage_.active_requests > 0) {
            usage_.active_requests--;
        }
        
        if (success) {
            usage_.successful_requests++;
        } else {
            usage_.failed_requests++;
        }
        
        // Update timing statistics
        const double time_ms = std::chrono::duration<double, std::milli>(inference_time).count();
        if (usage_.successful_requests == 1) {
            usage_.avg_inference_time_ms = time_ms;
            usage_.min_inference_time_ms = time_ms;
            usage_.max_inference_time_ms = time_ms;
        } else {
            // Running average
            const double successful_requests = static_cast<double>(usage_.successful_requests);
            usage_.avg_inference_time_ms =
                ((usage_.avg_inference_time_ms * (successful_requests - 1.0)) + time_ms) /
                successful_requests;
            usage_.min_inference_time_ms = std::min(usage_.min_inference_time_ms, time_ms);
            usage_.max_inference_time_ms = std::max(usage_.max_inference_time_ms, time_ms);
        }
    }
    
    // Update quota
    if (quota_tracker_) {
        quota_tracker_->consumeQuota(info.user_id, 1, inference_time, memory_used_mb);
    }
    
    logAuditEvent(success ? "request_completed" : "request_failed", 
                 info.user_id, info.model_id, 
                 "Time: " + std::to_string(inference_time.count()) + "ms", success);
}

void VisionResourceMonitor::rejectRequest(const std::string& user_id, const std::string& reason) {
    {
        std::scoped_lock<std::mutex> lock(usage_mutex_);
        usage_.rejected_requests++;
    }
    
    logAuditEvent("request_rejected", user_id, "", reason, false);
}

void VisionResourceMonitor::updateMemoryUsage([[maybe_unused]] size_t memory_mb) {
    std::scoped_lock<std::mutex> lock(usage_mutex_);
    usage_.current_memory_mb = memory_mb;
    usage_.peak_memory_mb = std::max(usage_.peak_memory_mb, memory_mb);
}

void VisionResourceMonitor::updateVRAMUsage([[maybe_unused]] size_t vram_mb) {
    std::scoped_lock<std::mutex> lock(usage_mutex_);
    usage_.current_vram_mb = vram_mb;
    usage_.peak_vram_mb = std::max(usage_.peak_vram_mb, vram_mb);
}

void VisionResourceMonitor::registerModelLoad(const std::string& model_id, size_t memory_mb, size_t vram_mb) {
    size_t model_count = 0;
    
    {
        std::scoped_lock<std::mutex> lock(models_mutex_);
        ModelInfo info;
        info.model_id = model_id;
        info.memory_mb = memory_mb;
        info.vram_mb = vram_mb;
        info.load_time = std::chrono::steady_clock::now();
        loaded_models_[model_id] = info;
        model_count = loaded_models_.size();
    }
    
    {
        std::scoped_lock<std::mutex> lock(usage_mutex_);
        usage_.loaded_models = model_count;
    }
    
    updateMemoryUsage(usage_.current_memory_mb + memory_mb);
    updateVRAMUsage(usage_.current_vram_mb + vram_mb);
    
    logAuditEvent("model_loaded", "", model_id, 
                 "Memory: " + std::to_string(memory_mb) + "MB, VRAM: " + std::to_string(vram_mb) + "MB", 
                 true);
}

void VisionResourceMonitor::registerModelUnload(const std::string& model_id) {
    size_t memory_mb = 0;
    size_t vram_mb = 0;
    size_t model_count = 0;
    
    {
        std::scoped_lock<std::mutex> lock(models_mutex_);
        auto it = loaded_models_.find(model_id);
        if (it != loaded_models_.end()) {
            memory_mb = it->second.memory_mb;
            vram_mb = it->second.vram_mb;
            loaded_models_.erase(it);
        }
        model_count = loaded_models_.size();
    }
    
    {
        std::scoped_lock<std::mutex> lock(usage_mutex_);
        usage_.loaded_models = model_count;
    }
    
    if (memory_mb > 0 && usage_.current_memory_mb >= memory_mb) {
        updateMemoryUsage(usage_.current_memory_mb - memory_mb);
    }
    
    if (vram_mb > 0 && usage_.current_vram_mb >= vram_mb) {
        updateVRAMUsage(usage_.current_vram_mb - vram_mb);
    }
    
    logAuditEvent("model_unloaded", "", model_id, "", true);
}

VisionResourceUsage VisionResourceMonitor::getResourceUsage() const {
    std::scoped_lock<std::mutex> lock(usage_mutex_);
    return usage_;
}

VisionResourceMonitor::RateLimiterStats VisionResourceMonitor::getRateLimiterStats() const {
    RateLimiterStats stats{};
    
    if (global_rate_limiter_) {
        stats.available_tokens = global_rate_limiter_->availableTokens();
        stats.time_until_next_token = global_rate_limiter_->timeUntilNextToken();
    }
    
    {
        std::scoped_lock<std::mutex> lock(usage_mutex_);
        stats.total_requests = usage_.total_requests;
        stats.rejected_requests = usage_.rejected_requests;
    }
    
    return stats;
}

QuotaTracker::QuotaRemaining VisionResourceMonitor::getUserQuota(const std::string& user_id) const {
    if (quota_tracker_) {
        return quota_tracker_->getRemainingQuota(user_id);
    }
    return QuotaTracker::QuotaRemaining{
        .daily_requests_remaining = 0,
        .monthly_requests_remaining = 0,
        .inference_minutes_remaining = 0,
        .vram_hours_remaining = 0
    };
}

std::string VisionResourceMonitor::exportMetrics() const {
    std::stringstream ss = {};
    constexpr uint64_t kBytesPerMb = 1024 * 1024;
    
    auto usage = getResourceUsage();
    // Prometheus format
    ss << "# HELP themisdb_vision_requests_total Total number of vision requests\n";
    ss << "# TYPE themisdb_vision_requests_total counter\n";
    ss << "themisdb_vision_requests_total " << usage.total_requests << "\n\n";
    
    ss << "# HELP themisdb_vision_requests_active Active vision requests\n";
    ss << "# TYPE themisdb_vision_requests_active gauge\n";
    ss << "themisdb_vision_requests_active " << usage.active_requests << "\n\n";
    
    ss << "# HELP themisdb_vision_memory_bytes Vision memory usage in bytes\n";
    ss << "# TYPE themisdb_vision_memory_bytes gauge\n";
     ss << "themisdb_vision_memory_bytes "
         << (static_cast<uint64_t>(usage.current_memory_mb) * kBytesPerMb)
         << "\n\n";
    
    ss << "# HELP themisdb_vision_vram_bytes Vision VRAM usage in bytes\n";
    ss << "# TYPE themisdb_vision_vram_bytes gauge\n";
     ss << "themisdb_vision_vram_bytes "
         << (static_cast<uint64_t>(usage.current_vram_mb) * kBytesPerMb)
         << "\n\n";
    
    ss << "# HELP themisdb_vision_models_loaded Number of loaded vision models\n";
    ss << "# TYPE themisdb_vision_models_loaded gauge\n";
    ss << "themisdb_vision_models_loaded " << usage.loaded_models << "\n\n";
    
    ss << "# HELP themisdb_vision_inference_duration_seconds Inference duration\n";
    ss << "# TYPE themisdb_vision_inference_duration_seconds summary\n";
    ss << "themisdb_vision_inference_duration_seconds{quantile=\"0.5\"} " << (usage.avg_inference_time_ms / 1000.0) << "\n";
    ss << "themisdb_vision_inference_duration_seconds{quantile=\"0.9\"} " << (usage.max_inference_time_ms / 1000.0) << "\n";
     ss << "themisdb_vision_inference_duration_seconds_sum "
         << ((usage.avg_inference_time_ms * static_cast<double>(usage.successful_requests)) / 1000.0)
         << "\n";
    ss << "themisdb_vision_inference_duration_seconds_count " << usage.successful_requests << "\n\n";
    
    return ss.str();
}

std::vector<VisionResourceMonitor::AuditEntry> VisionResourceMonitor::getAuditLog([[maybe_unused]] size_t max_entries) const {
    std::scoped_lock<std::mutex> lock(audit_mutex_);
    
    std::vector<AuditEntry> entries;
    auto q = audit_log_;
    
    while (!q.empty() && static_cast<int>(entries.size()) < max_entries) {
        entries.push_back(q.front());
        q.pop();
    }
    
    return entries;
}

bool VisionResourceMonitor::isHealthy() const {
    return getHealthStatus().healthy;
}

VisionResourceMonitor::HealthStatus VisionResourceMonitor::getHealthStatus() const {
    HealthStatus status{};
    status.healthy = true;
    status.status = "healthy";
    
    const auto& limits = config_->getResourceLimits();
    auto usage = getResourceUsage();
    
    status.memory_utilization_percent = usage.getMemoryUtilization(limits.max_memory_mb);
    status.vram_utilization_percent = usage.getVRAMUtilization(limits.max_vram_mb);
    status.request_utilization_percent = usage.getRequestUtilization(limits.max_concurrent_requests);
    
    // Check thresholds
    if (status.memory_utilization_percent > 90.0) {
        status.healthy = false;
        status.status = "unhealthy";
        status.issues.push_back("Memory utilization critical: " + std::to_string(static_cast<int>(status.memory_utilization_percent)) + "%");
    } else if (status.memory_utilization_percent > 80.0) {
        status.status = "degraded";
        status.issues.push_back("Memory utilization high: " + std::to_string(static_cast<int>(status.memory_utilization_percent)) + "%");
    }
    
    if (status.vram_utilization_percent > 90.0) {
        status.healthy = false;
        status.status = "unhealthy";
        status.issues.push_back("VRAM utilization critical: " + std::to_string(static_cast<int>(status.vram_utilization_percent)) + "%");
    } else if (status.vram_utilization_percent > 80.0) {
        status.status = "degraded";
        status.issues.push_back("VRAM utilization high: " + std::to_string(static_cast<int>(status.vram_utilization_percent)) + "%");
    }
    
    if (status.request_utilization_percent > 95.0) {
        status.healthy = false;
        status.status = "unhealthy";
        status.issues.push_back("Request capacity critical: " + std::to_string(static_cast<int>(status.request_utilization_percent)) + "%");
    }
    
    return status;
}

void VisionResourceMonitor::metricsCollectionLoop() {
    const auto& config = config_->getMonitoringConfig();
    auto interval = config.collect_interval;
    
    spdlog::debug("Metrics collection loop started (interval: {}s)", interval.count());
    
    while (running_) {
        std::this_thread::sleep_for(interval);
        
        if (!running_) {
            break;
        }
        
        // Collect and log metrics
        auto usage = getResourceUsage();
        const auto& limits = config_->getResourceLimits();
        
        spdlog::debug("Vision metrics: active={}, total={}, memory={}MB/{}/MB, vram={}MB/{}MB",
                     usage.active_requests, usage.total_requests,
                     usage.current_memory_mb, limits.max_memory_mb,
                     usage.current_vram_mb, limits.max_vram_mb);
    }
    
    spdlog::debug("Metrics collection loop stopped");
}

void VisionResourceMonitor::quotaResetLoop() {
    spdlog::debug("Quota reset loop started");
    
    while (running_) {
        // Sleep for 1 hour
        std::this_thread::sleep_for(std::chrono::hours(1));
        
        if (!running_) {
            break;
        }
        
        // Check if it's time to reset quotas
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        const auto& quota = config_->getResourceQuota();
        
        // Reset at midnight for daily quotas
        if (quota.reset_period == "daily" && tm.tm_hour == 0 && tm.tm_min < 5) {
            quota_tracker_->resetQuotas();
        }
        
        // Reset on first day of month for monthly quotas
        if (quota.reset_period == "monthly" && tm.tm_mday == 1 && tm.tm_hour == 0 && tm.tm_min < 5) {
            quota_tracker_->resetQuotas();
        }
    }
    
    spdlog::debug("Quota reset loop stopped");
}

void VisionResourceMonitor::logAuditEvent(const std::string& event_type, const std::string& user_id,
                                         const std::string& model_id, const std::string& details, bool success) {
    if (!config_->isAuditEnabled()) {
        return;
    }
    
    std::scoped_lock<std::mutex> lock(audit_mutex_);
    
    AuditEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.event_type = event_type;
    entry.user_id = user_id;
    entry.model_id = model_id;
    entry.details = details;
    entry.success = success;
    
    audit_log_.push(entry);
    
    // Limit audit log size
    while (static_cast<int>(audit_log_.size()) > MAX_AUDIT_ENTRIES) {
        audit_log_.pop();
    }
    
    // Also log to regular logger
    if (success) {
        spdlog::debug("Vision audit: {} - user={}, model={}, details={}", 
                     event_type, user_id, model_id, details);
    } else {
        spdlog::warn("Vision audit: {} - user={}, model={}, details={}", 
                    event_type, user_id, model_id, details);
    }
}

RateLimiter* VisionResourceMonitor::getUserRateLimiter(const std::string& user_id) {
    const auto& rate_limits = config_->getRateLimits();
    if (!rate_limits.enabled) {
        return nullptr;
    }
    
    std::scoped_lock<std::mutex> lock(rate_limiter_mutex_);
    
    auto it = user_rate_limiters_.find(user_id);
    if (it != user_rate_limiters_.end()) {
        return it->second.get();
    }
    
    // Create new rate limiter for user
    auto limiter = std::make_unique<RateLimiter>(
        rate_limits.requests_per_minute,
        rate_limits.burst_size
    );
    auto* ptr = limiter.get();
    user_rate_limiters_[user_id] = std::move(limiter);
    
    return ptr;
}

} // namespace themis::llm

