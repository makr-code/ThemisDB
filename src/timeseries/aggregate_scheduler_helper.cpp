/**
 * @file aggregate_scheduler_helper.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "timeseries/aggregate_scheduler.h"
#include "timeseries/tsstore.h"
#include "timeseries/continuous_agg.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <sstream>

namespace themis {

std::string AggregateScheduler::registerAggregate(const AggConfig& config, std::chrono::milliseconds refresh_interval) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string id = generateAggregateId(config);
    
    ScheduledAggregate agg;
    agg.id = id;
    agg.config = config;
    agg.refresh_interval = refresh_interval;
    agg.last_refresh_ms = 0;
    agg.enabled = true;
    agg.use_incremental_refresh = true;
    agg.total_refreshes = 0;
    agg.failed_refreshes = 0;
    agg.avg_refresh_time_ms = 0.0;
    
    aggregates_[id] = std::move(agg);
    
    THEMIS_INFO("Registered aggregate '{}' with refresh interval {}ms", id, refresh_interval.count());
    
    return id;
}

void AggregateScheduler::backfill_range(const std::string& agg_id, int64_t start_ms, int64_t end_ms) {
    if (start_ms >= end_ms) {
        THEMIS_WARN("backfill_range: invalid range [{}, {}) for aggregate '{}' – start must be < end",
                    start_ms, end_ms, agg_id);
        return;
    }

    auto span = Tracer::startSpan("AggregateScheduler.backfill_range");
    span.setAttribute("aggregate_id", agg_id);
    span.setAttribute("start_ms", start_ms);
    span.setAttribute("end_ms", end_ms);

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = aggregates_.find(agg_id);
    if (it == aggregates_.end()) {
        THEMIS_WARN("backfill_range: unknown aggregate '{}'", agg_id);
        span.recordError("Unknown aggregate");
        return;
    }

    const AggConfig& cfg = it->second.config;

    THEMIS_INFO("backfill_range: running backfill for '{}' over [{}, {})", agg_id, start_ms, end_ms);

    try {
        // Backfill uses the standard full-range refresh so the watermark is NOT
        // advanced — callers can then decide whether to reset the watermark manually
        // or let the next scheduled incremental refresh carry on from where it left off.
        agg_manager_->refresh(cfg, start_ms, end_ms - 1);
        THEMIS_INFO("backfill_range: completed for '{}' over [{}, {})", agg_id, start_ms, end_ms);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        THEMIS_ERROR("backfill_range: failed for '{}': {}", agg_id, e.what());
    }
}

} // namespace themis
