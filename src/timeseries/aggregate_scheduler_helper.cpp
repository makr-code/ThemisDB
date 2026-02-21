/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aggregate_scheduler_helper.cpp                     ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     51                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
