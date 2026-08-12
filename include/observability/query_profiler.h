/**
 * @file query_profiler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace themis {
namespace observability {

using json = nlohmann::json;

/**
 * @brief Query execution phase
 */
enum class QueryPhase {
    PARSE,
    VALIDATE,
    OPTIMIZE,
    PLAN,
    EXECUTE,
    FETCH_RESULTS
};

/**
 * @brief Operator types in query execution
 */
enum class OperatorType {
    SCAN,
    INDEX_SCAN,
    FILTER,
    PROJECT,
    AGGREGATE,
    JOIN,
    SORT,
    LIMIT,
    SUBQUERY,
    VECTOR_SEARCH,
    GRAPH_TRAVERSE,
    UNKNOWN
};

/**
 * @brief Statistics for a single operator
 */
struct OperatorStats {
    OperatorType type;
    std::string name;
    std::chrono::microseconds duration{0};
    size_t rows_processed = 0;
    size_t bytes_processed = 0;
    size_t disk_reads = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    std::string details;
    
    json toJSON() const;
};

/**
 * @brief Query execution profile
 */
struct QueryProfile {
    std::string query_id;
    std::string query_text;
    std::chrono::system_clock::time_point start_time;
    std::chrono::microseconds total_duration{0};
    
    // Phase timings
    std::unordered_map<QueryPhase, std::chrono::microseconds> phase_timings;
    
    // Operator statistics
    std::vector<OperatorStats> operator_stats;
    
    // Resource usage
    size_t peak_memory_bytes = 0;
    size_t total_disk_io_bytes = 0;
    size_t total_network_bytes = 0;
    
    // Optimization info
    bool used_index = false;
    bool used_cache = false;
    std::vector<std::string> indexes_used;
    std::vector<std::string> warnings;
    std::vector<std::string> optimization_hints;
    
    // Result metadata
    size_t result_rows = 0;
    size_t result_bytes = 0;
    
    json toJSON() const;
    std::string toSummary() const;
};

/**
 * @brief Query profiler configuration
 */
struct QueryProfilerConfig {
    bool enabled = true;
    bool profile_all_queries = false;
    bool collect_operator_stats = true;
    bool collect_memory_stats = true;
    bool collect_io_stats = true;
    size_t max_profiles_retained = 1000;
    std::chrono::seconds retention_duration{3600}; // 1 hour
    bool log_slow_queries = true;
    std::chrono::milliseconds slow_query_threshold{1000}; // 1 second
};

/**
 * @brief Query execution profiler
 * 
 * Profiles query execution, collecting timing, resource usage,
 * and operator statistics for performance analysis.
 */
class QueryProfiler {
public:
    explicit QueryProfiler(const QueryProfilerConfig& config = QueryProfilerConfig{});
    ~QueryProfiler();
    
    // Disable copy
    QueryProfiler(const QueryProfiler&) = delete;
    QueryProfiler& operator=(const QueryProfiler&) = delete;
    
    /**
     * @brief Start profiling a query
     * @param query_id Unique query identifier
     * @param query_text Query text
     * @return Profile handle
     */
    std::string start_query(const std::string& query_id, const std::string& query_text);
    
    /**
     * @brief End profiling a query
     * @param query_id Query identifier
     */
    void end_query(const std::string& query_id);
    
    /**
     * @brief Record phase timing
     * @param query_id Query identifier
     * @param phase Query phase
     * @param duration Phase duration
     */
    void record_phase(const std::string& query_id, QueryPhase phase, 
                     std::chrono::microseconds duration);
    
    /**
     * @brief Record operator statistics
     * @param query_id Query identifier
     * @param stats Operator statistics
     */
    void record_operator(const std::string& query_id, const OperatorStats& stats);
    
    /**
     * @brief Record index usage
     * @param query_id Query identifier
     * @param index_name Index name
     */
    void record_index_usage(const std::string& query_id, const std::string& index_name);
    
    /**
     * @brief Record cache usage
     * @param query_id Query identifier
     * @param cache_hit Whether it was a cache hit
     */
    void record_cache_usage(const std::string& query_id, bool cache_hit);
    
    /**
     * @brief Add optimization hint
     * @param query_id Query identifier
     * @param hint Optimization hint
     */
    void add_hint(const std::string& query_id, const std::string& hint);
    
    /**
     * @brief Add warning
     * @param query_id Query identifier
     * @param warning Warning message
     */
    void add_warning(const std::string& query_id, const std::string& warning);
    
    /**
     * @brief Get profile for a query
     * @param query_id Query identifier
     * @return Query profile or nullptr if not found
     */
    std::shared_ptr<QueryProfile> get_profile(const std::string& query_id) const;
    
    /**
     * @brief Get all profiles
     * @return Vector of all profiles
     */
    std::vector<std::shared_ptr<QueryProfile>> get_all_profiles() const;
    
    /**
     * @brief Get slow queries
     * @param threshold Slow query threshold
     * @return Vector of slow query profiles
     */
    std::vector<std::shared_ptr<QueryProfile>> get_slow_queries(
        std::chrono::milliseconds threshold) const;
    
    /**
     * @brief Get top queries by duration
     * @param limit Number of queries to return
     * @return Vector of top queries
     */
    std::vector<std::shared_ptr<QueryProfile>> get_top_queries(size_t limit = 10) const;
    
    /**
     * @brief Clear all profiles
     */
    void clear();
    
    /**
     * @brief Export profiles to JSON
     * @param filename Output file path
     */
    void export_to_json(const std::string& filename) const;
    
    /**
     * @brief Get configuration
     */
    QueryProfilerConfig get_config() const;
    
    /**
     * @brief Set configuration
     */
    void set_config(const QueryProfilerConfig& config);
    
    /**
     * @brief Enable profiling
     */
    void enable();
    
    /**
     * @brief Disable profiling
     */
    void disable();
    
    /**
     * @brief Check if profiling is enabled
     */
    bool is_enabled() const;
    
    /**
     * @brief Get statistics summary
     */
    json get_statistics() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    void cleanup_old_profiles();
    void log_slow_query(const QueryProfile& profile);
};

/**
 * @brief RAII helper for query profiling
 */
class ScopedQueryProfile {
public:
    ScopedQueryProfile(QueryProfiler& profiler, const std::string& query_id, 
                      const std::string& query_text);
    ~ScopedQueryProfile();
    
    // Disable copy
    ScopedQueryProfile(const ScopedQueryProfile&) = delete;
    ScopedQueryProfile& operator=(const ScopedQueryProfile&) = delete;
    
    void record_phase(QueryPhase phase, std::chrono::microseconds duration);
    void record_operator(const OperatorStats& stats);
    void add_hint(const std::string& hint);
    void add_warning(const std::string& warning);
    
private:
    QueryProfiler& profiler_;
    std::string query_id_;
};

/**
 * @brief RAII helper for operator profiling
 */
class ScopedOperatorProfile {
public:
    ScopedOperatorProfile(QueryProfiler& profiler, const std::string& query_id,
                         OperatorType type, const std::string& name);
    ~ScopedOperatorProfile();
    
    // Disable copy
    ScopedOperatorProfile(const ScopedOperatorProfile&) = delete;
    ScopedOperatorProfile& operator=(const ScopedOperatorProfile&) = delete;
    
    void record_rows(size_t count);
    void record_bytes(size_t count);
    void record_disk_read();
    void record_cache_hit();
    void record_cache_miss();
    void set_details(const std::string& details);
    
private:
    QueryProfiler& profiler_;
    std::string query_id_;
    OperatorStats stats_;
    std::chrono::high_resolution_clock::time_point start_;
};

// Helper functions
const char* to_string(QueryPhase phase);
const char* to_string(OperatorType type);

} // namespace observability
} // namespace themis
