/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cdc_admin.h                                        ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     216                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include "cdc/changefeed.h"
#include "cdc/cdc_metrics.h"
#include "nlohmann/json.hpp"

namespace themis {
namespace cdc {

// Forward declarations
class TenantBufferManager;

/**
 * Result of a purge operation
 */
struct PurgeResult {
    uint64_t events_deleted = 0;
    uint64_t elapsed_time_ms = 0;
    
    nlohmann::json toJson() const {
        return {
            {"events_deleted", events_deleted},
            {"elapsed_time_ms", elapsed_time_ms}
        };
    }
};

/**
 * Health status of CDC components
 */
struct HealthStatus {
    bool is_healthy = true;
    std::string message;
    
    // Component health
    bool changefeed_healthy = true;
    bool buffer_healthy = true;
    bool retention_healthy = true;
    
    // Metrics
    double buffer_utilization = 0.0;  // 0.0 to 1.0
    uint64_t error_count = 0;
    uint64_t error_rate_per_sec = 0;
    
    nlohmann::json toJson() const {
        return {
            {"is_healthy", is_healthy},
            {"message", message},
            {"components", {
                {"changefeed", changefeed_healthy},
                {"buffer", buffer_healthy},
                {"retention", retention_healthy}
            }},
            {"metrics", {
                {"buffer_utilization", buffer_utilization},
                {"error_count", error_count},
                {"error_rate_per_sec", error_rate_per_sec}
            }}
        };
    }
};

/**
 * Comprehensive diagnostics information
 */
struct DiagnosticsInfo {
    Changefeed::Watermarks watermarks;
    CDCMetrics metrics;
    HealthStatus health;
    
    // Additional stats
    uint64_t total_events = 0;
    uint64_t buffer_size = 0;
    std::chrono::system_clock::time_point uptime_start;
    
    nlohmann::json toJson() const {
        auto now = std::chrono::system_clock::now();
        auto uptime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - uptime_start).count();
        
        return {
            {"watermarks", {
                {"low_watermark", watermarks.low_watermark},
                {"high_watermark", watermarks.high_watermark},
                {"oldest_timestamp_ms", watermarks.oldest_timestamp_ms},
                {"newest_timestamp_ms", watermarks.newest_timestamp_ms}
            }},
            {"metrics", metrics.toJson()},
            {"health", health.toJson()},
            {"stats", {
                {"total_events", total_events},
                {"buffer_size", buffer_size},
                {"uptime_ms", uptime_ms}
            }}
        };
    }
};

/**
 * CDC Admin API for operational tasks
 * 
 * Provides administrative operations for CDC:
 * - Purge: Delete events by range, timestamp, or tenant
 * - Replay: Get events from a specific sequence
 * - Health Check: Check system health status
 * - Diagnostics: Export complete diagnostics info
 */
class CDCAdmin {
public:
    /**
     * Create admin interface for a changefeed
     */
    explicit CDCAdmin(Changefeed* changefeed);
    
    /**
     * Create admin interface for tenant buffer manager
     */
    explicit CDCAdmin(TenantBufferManager* tenant_manager);
    
    ~CDCAdmin() = default;
    
    // Purge operations
    
    /**
     * Purge all events from the changefeed
     * WARNING: This deletes all data!
     */
    PurgeResult purgeAll();
    
    /**
     * Purge events in a sequence range (inclusive)
     * @param start_sequence First sequence to delete (inclusive)
     * @param end_sequence Last sequence to delete (inclusive)
     */
    PurgeResult purgeBySequenceRange(uint64_t start_sequence, uint64_t end_sequence);
    
    /**
     * Purge events older than a timestamp
     * @param before_timestamp_ms Delete events with timestamp < this value
     */
    PurgeResult purgeByTimestamp(uint64_t before_timestamp_ms);
    
    /**
     * Purge all events for a specific tenant
     * Only works with TenantBufferManager
     */
    PurgeResult purgeTenant(const std::string& tenant_id);
    
    // Replay operations
    
    /**
     * Replay events starting from a specific sequence
     * @param from_sequence Start from this sequence (inclusive)
     * @param limit Maximum number of events to return (0 = no limit)
     * @return Vector of change events
     */
    std::vector<Changefeed::ChangeEvent> replayFromSequence(
        uint64_t from_sequence, 
        uint64_t limit = 0);
    
    // Health & diagnostics
    
    /**
     * Perform health check on CDC components
     * @return Health status with component details
     */
    HealthStatus healthCheck();
    
    /**
     * Get comprehensive diagnostics information
     * @return Complete diagnostics including metrics, watermarks, health
     */
    DiagnosticsInfo getDiagnostics();
    
private:
    Changefeed* changefeed_;
    TenantBufferManager* tenant_manager_;
    std::chrono::system_clock::time_point creation_time_;
    
    // Helper methods
    uint64_t countEventsInRange(uint64_t start, uint64_t end);
    void validateSequenceRange(uint64_t start, uint64_t end);
};

}  // namespace cdc
}  // namespace themis
