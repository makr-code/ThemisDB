/**
 * @file aggregate_scheduler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "timeseries/aggregate_scheduler.h"
#include "timeseries/continuous_agg.h"
#include "timeseries/tsstore.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <algorithm>
#include <sstream>

namespace themis {

AggregateScheduler::AggregateScheduler(TSStore* store)
    : AggregateScheduler(store, Config{}) {}

AggregateScheduler::AggregateScheduler(TSStore* store, const Config& config)
    : store_(store), config_(config) {
    
    if (!store_) {
        throw std::invalid_argument("AggregateScheduler: TSStore cannot be null");
    }
    
    agg_manager_ = std::make_unique<ContinuousAggregateManager>(store_);
    wm_store_ = std::make_unique<ContinuousAggWatermarkStore>(store_);
}

AggregateScheduler::~AggregateScheduler() {
    stop();
}

// ===== Lifecycle =====

void AggregateScheduler::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (running_.load()) {
        THEMIS_WARN("AggregateScheduler already running");
        return;
    }
    
    running_.store(true);
    scheduler_thread_ = std::thread(&AggregateScheduler::schedulerLoop, this);
    
    THEMIS_INFO("AggregateScheduler started with {} aggregates, check interval: {}s",
                aggregates_.size(), 
                config_.check_interval.count() / 1000);
}

void AggregateScheduler::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_.load()) {
            return;
        }
        running_.store(false);
    }
    
    cv_.notify_all();
    
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
    
    THEMIS_INFO("AggregateScheduler stopped. Total refreshes: {}, Failed: {}",
                total_refreshes_.load(), failed_refreshes_.load());
}

// ===== Aggregate Management =====

void AggregateScheduler::registerAggregate(const ScheduledAggregate& agg) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string id = agg.id;
    if (id.empty()) {
        id = generateAggregateId(agg.config);
    }
    
    aggregates_[id] = agg;
    aggregates_[id].id = id;
    
    THEMIS_INFO("Registered continuous aggregate: {} (metric={}, window={}ms, refresh_interval={}ms)",
                id, agg.config.metric, agg.config.window.size.count(), agg.refresh_interval.count());
}

void AggregateScheduler::unregisterAggregate(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = aggregates_.find(id);
    if (it != aggregates_.end()) {
        THEMIS_INFO("Unregistered continuous aggregate: {}", id);
        // Clean up persisted watermark
        if (wm_store_) {
            wm_store_->deleteWatermark(id);
        }
        aggregates_.erase(it);
    }
}

void AggregateScheduler::enableAggregate(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = aggregates_.find(id);
    if (it != aggregates_.end()) {
        it->second.enabled = true;
        THEMIS_INFO("Enabled continuous aggregate: {}", id);
    }
}

void AggregateScheduler::disableAggregate(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = aggregates_.find(id);
    if (it != aggregates_.end()) {
        it->second.enabled = false;
        THEMIS_INFO("Disabled continuous aggregate: {}", id);
    }
}

// ===== Manual Operations =====

void AggregateScheduler::refreshNow(const std::string& id) {
    auto span = Tracer::startSpan("AggregateScheduler.refreshNow");
    span.setAttribute("aggregate_id", id);
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = aggregates_.find(id);
    if (it == aggregates_.end()) {
        THEMIS_WARN("Cannot refresh unknown aggregate: {}", id);
        span.recordError("Unknown aggregate");
        return;
    }
    
    refreshAggregate(it->second);
}

void AggregateScheduler::refreshAll() {
    auto span = Tracer::startSpan("AggregateScheduler.refreshAll");
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = 0;
    for (auto& [id, agg] : aggregates_) {
        if (agg.enabled) {
            refreshAggregate(agg);
            count++;
        }
    }
    
    span.setAttribute("refreshed_count", static_cast<int64_t>(count));
    THEMIS_INFO("Manually refreshed {} aggregates", count);
}

// ===== Statistics =====

AggregateScheduler::Stats AggregateScheduler::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats stats;
    stats.registered_aggregates = aggregates_.size();
    stats.active_aggregates = std::count_if(aggregates_.begin(), aggregates_.end(),
        [](const auto& pair) { return pair.second.enabled; });
    stats.total_refreshes = total_refreshes_.load();
    stats.failed_refreshes = failed_refreshes_.load();
    stats.last_run = last_run_;
    stats.next_run = last_run_ + config_.check_interval;
    
    return stats;
}

std::vector<AggregateScheduler::ScheduledAggregate> AggregateScheduler::listAggregates() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ScheduledAggregate> result;
    result.reserve(aggregates_.size());
    
    for (const auto& [id, agg] : aggregates_) {
        result.push_back(agg);
    }
    
    return result;
}

void AggregateScheduler::setMetrics(std::shared_ptr<TimeSeriesMetrics> metrics) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_ = std::move(metrics);
}

// ===== Scheduler Loop =====

void AggregateScheduler::schedulerLoop() {
    THEMIS_INFO("AggregateScheduler loop started");
    
    while (running_.load()) {
        auto span = Tracer::startSpan("AggregateScheduler.tick");
        
        int64_t current_time_ms = getCurrentTimeMs();
        size_t refreshed_count = 0;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            last_run_ = std::chrono::system_clock::now();
            
            for (auto& [id, agg] : aggregates_) {
                if (!agg.enabled) {
                    continue;
                }
                
                if (needsRefresh(agg, current_time_ms)) {
                    // Catch up missed windows if configured.
                    // Skip for incremental refresh: refreshIncremental() already
                    // processes all missed windows since the last watermark, so calling
                    // catchUpMissedWindows() first would cause duplicate aggregate points.
                    if (config_.catch_up_missed_windows && agg.last_refresh_ms > 0
                            && !agg.use_incremental_refresh) {
                        catchUpMissedWindows(agg, current_time_ms);
                    }
                    
                    refreshAggregate(agg);
                    refreshed_count++;
                }
            }
        }
        
        span.setAttribute("refreshed_count", static_cast<int64_t>(refreshed_count));
        
        // Wait for next check interval or shutdown signal
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, config_.check_interval, [this] { return !running_.load(); });
    }
    
    THEMIS_INFO("AggregateScheduler loop stopped");
}

void AggregateScheduler::refreshAggregate(ScheduledAggregate& agg) {
    auto span = Tracer::startSpan("AggregateScheduler.refreshAggregate");
    span.setAttribute("aggregate_id", agg.id);
    span.setAttribute("metric", agg.config.metric);
    
    auto start = std::chrono::steady_clock::now();
    
    try {
        int64_t current_time_ms = getCurrentTimeMs();
        int64_t window_ms = agg.config.window.size.count();
        
        // Align window end to the last complete window boundary
        int64_t window_end = (current_time_ms / window_ms) * window_ms;

        span.setAttribute("window_end_ms", window_end);

        if (agg.use_incremental_refresh && wm_store_) {
            // Incremental path: read watermark, scan [watermark, window_end), advance watermark
            int64_t watermark_before = wm_store_->getWatermark(agg.id);
            span.setAttribute("watermark_ms", watermark_before);

            agg_manager_->refreshIncremental(agg.config, agg.id, window_end, *wm_store_);

            // Emit lag metric (how far behind the watermark is vs. now)
            double lag_ms = static_cast<double>(current_time_ms - window_end);
            if (metrics_) {
                metrics_->recordAggRefreshLag(agg.id, lag_ms < 0.0 ? 0.0 : lag_ms);
            }
        } else {
            // Full-scan fallback path (original behaviour)
            int64_t window_start = window_end - window_ms;
            span.setAttribute("window_start_ms", window_start);
            agg_manager_->refresh(agg.config, window_start, window_end);
        }
        
        agg.last_refresh_ms = current_time_ms;
        agg.total_refreshes++;
        total_refreshes_++;
        
        auto end = std::chrono::steady_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Update moving average of refresh time
        agg.avg_refresh_time_ms = (agg.avg_refresh_time_ms * (agg.total_refreshes - 1) + elapsed_ms) 
                                   / agg.total_refreshes;
        
        span.setAttribute("refresh_time_ms", elapsed_ms);

        // Emit per-aggregate latency metric
        if (metrics_) {
            metrics_->recordAggRefreshLatency(agg.id, elapsed_ms);
        }
        
        THEMIS_DEBUG("Refreshed aggregate {} (metric={}, window_end={}, time={:.2f}ms)",
                     agg.id, agg.config.metric, window_end, elapsed_ms);
        
    } catch (const std::exception& e) {
        agg.failed_refreshes++;
        failed_refreshes_++;
        
        span.recordError(e.what());
        THEMIS_ERROR("Failed to refresh aggregate {}: {}", agg.id, e.what());
    }
}

bool AggregateScheduler::needsRefresh(const ScheduledAggregate& agg, int64_t current_time_ms) const {
    if (agg.last_refresh_ms == 0) {
        return true;  // Never refreshed before
    }
    
    int64_t elapsed_ms = current_time_ms - agg.last_refresh_ms;
    return elapsed_ms >= agg.refresh_interval.count();
}

void AggregateScheduler::catchUpMissedWindows(ScheduledAggregate& agg, int64_t current_time_ms) {
    int64_t window_ms = agg.config.window.size.count();
    int64_t elapsed_ms = current_time_ms - agg.last_refresh_ms;
    
    // Calculate how many windows were missed
    size_t missed_windows = static_cast<size_t>(elapsed_ms / window_ms);
    
    if (missed_windows <= 1) {
        return;  // No catch-up needed (0 or 1 window is normal)
    }
    
    // Limit catch-up to prevent overwhelming the system
    missed_windows = std::min(missed_windows, config_.max_catch_up_windows);
    
    THEMIS_INFO("Catching up {} missed windows for aggregate {}", missed_windows - 1, agg.id);
    
    auto span = Tracer::startSpan("AggregateScheduler.catchUpMissedWindows");
    span.setAttribute("aggregate_id", agg.id);
    span.setAttribute("missed_windows", static_cast<int64_t>(missed_windows));
    
    // Refresh each missed window
    for (size_t i = 2; i <= missed_windows; i++) {
        int64_t window_end = current_time_ms - (missed_windows - i) * window_ms;
        int64_t window_start = window_end - window_ms;
        
        try {
            agg_manager_->refresh(agg.config, window_start, window_end);
            THEMIS_DEBUG("Caught up window [{}, {}] for aggregate {}", 
                        window_start, window_end, agg.id);
        } catch (const std::exception& e) {
            THEMIS_ERROR("Failed to catch up window for aggregate {}: {}", agg.id, e.what());
            break;  // Stop catch-up on first error
        }
    }
}

// ===== Helpers =====

int64_t AggregateScheduler::getCurrentTimeMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string AggregateScheduler::generateAggregateId(const AggConfig& config) const {
    std::ostringstream oss;
    oss << config.metric;
    if (config.entity.has_value()) {
        oss << ":" << *config.entity;
    }
    oss << ":" << config.window.size.count() << "ms";
    return oss.str();
}

} // namespace themis
