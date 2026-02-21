/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aggregate_scheduler_helper.cpp                     ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:43:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     56                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 35e35b6dc  2025-11-30  Sharding Phase 2-3: Auto-Rebalancing komplett (Load Detec... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    agg.total_refreshes = 0;
    agg.failed_refreshes = 0;
    agg.avg_refresh_time_ms = 0.0;
    
    aggregates_[id] = std::move(agg);
    
    THEMIS_INFO("Registered aggregate '{}' with refresh interval {}ms", id, refresh_interval.count());
    
    return id;
}

} // namespace themis
