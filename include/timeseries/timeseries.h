/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            timeseries.h                                       ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     173                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 3043a1a008  2026-04-12  docs(perf): aktualisiere Performance-Erwartungswerte und ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cstdint>
#include <nlohmann/json.hpp>

// Forward declarations
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {

/**
 * @brief Time-Series Storage MVP (Sprint B)
 * 
 * Simple time-series storage for metrics and events with range queries.
 * 
 * Key Schema: ts:{metric}:{entity}:{timestamp_ms}
 * Value: double (for simple metrics) or JSON (for complex events)
 * 
 * Features:
 * - Put data points with timestamp
 * - Range queries (from_time, to_time)
 * - Basic aggregations: min, max, avg, sum, count
 * - Optional bucketing/downsampling
 * 
 * MVP Scope (no compression yet):
 * - Raw storage in RocksDB
 * - Range scans via prefix iteration
 * - In-memory aggregation
 * - Follow-up: Gorilla compression, retention policies
 */
class TimeSeriesStore {
public:
    struct DataPoint {
        int64_t timestamp_ms;
        double value;
        nlohmann::json metadata; // Optional additional data
        
        nlohmann::json toJson() const;
        static DataPoint fromJson(const nlohmann::json& j);
    };
    
    struct RangeQuery {
        int64_t from_ms = 0;
        int64_t to_ms = INT64_MAX;
        size_t limit = 1000;        // Max points to return
        bool descending = false;     // Latest first
    };
    
    struct Aggregation {
        double min = 0.0;
        double max = 0.0;
        double avg = 0.0;
        double sum = 0.0;
        size_t count = 0;
        
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Construct TimeSeriesStore
     * @param db RocksDB TransactionDB instance (not owned)
     * @param cf Optional column family handle (nullptr = default CF)
     */
    explicit TimeSeriesStore(rocksdb::TransactionDB* db,
                            rocksdb::ColumnFamilyHandle* cf = nullptr);
    
    ~TimeSeriesStore() = default;
    
    /**
     * @brief Put a data point
     * @param metric Metric name (e.g., "cpu_usage", "request_count")
     * @param entity Entity ID (e.g., "server-1", "user-123")
     * @param point Data point with timestamp and value
     * @return true on success
     */
    bool put(std::string_view metric, 
             std::string_view entity,
             const DataPoint& point);
    
    /**
     * @brief Query data points in time range (all data)
     * @param metric Metric name
     * @param entity Entity ID
     * @return Vector of data points
     */
    std::vector<DataPoint> query(std::string_view metric,
                                  std::string_view entity) const;
    
    /**
     * @brief Query data points in time range
     * @param metric Metric name
     * @param entity Entity ID
     * @param query Range query parameters
     * @return Vector of data points
     */
    std::vector<DataPoint> query(std::string_view metric,
                                  std::string_view entity,
                                  const RangeQuery& query) const;
    
    /**
     * @brief Aggregate data points in time range (all data)
     * @param metric Metric name
     * @param entity Entity ID
     * @return Aggregation result
     */
    Aggregation aggregate(std::string_view metric,
                         std::string_view entity) const;
    
    /**
     * @brief Aggregate data points in time range
     * @param metric Metric name
     * @param entity Entity ID
     * @param query Range query parameters
     * @return Aggregation result
     */
    Aggregation aggregate(std::string_view metric,
                         std::string_view entity,
                         const RangeQuery& query) const;
    
    /**
     * @brief Delete old data points (retention policy)
     * @param metric Metric name
     * @param entity Entity ID  
     * @param before_ms Delete all points older than this timestamp
     * @return Number of points deleted
     */
    size_t deleteOldPoints(std::string_view metric,
                           std::string_view entity,
                           int64_t before_ms);
    
    /**
     * @brief Get latest value for metric/entity
     * @param metric Metric name
     * @param entity Entity ID
     * @return Latest data point, or nullopt if not found
     */
    std::optional<DataPoint> getLatest(std::string_view metric,
                                       std::string_view entity) const;

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_;

  rocksdb::ColumnFamilyHandle* resolveColumnFamily() const;
    
    static constexpr const char* KEY_PREFIX = "ts:";
    
    std::string makeKey(std::string_view metric, 
                       std::string_view entity,
                       int64_t timestamp_ms) const;
    
    std::string makePrefix(std::string_view metric,
                          std::string_view entity) const;
};

} // namespace themis
