/**
 * @file query_profiler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/query_profiler.h"
#include <fstream>
#include <algorithm>
#include <mutex>
#include <sstream>
#include <iomanip>

namespace themis {
namespace observability {

/**
 * @brief To string.
 * @param[in] phase Input parameter.
 * @return Pointer to the result.
 * @details Implements to_string without additional internal calls.
 */
const char* to_string(QueryPhase phase) {
    switch (phase) {
        case QueryPhase::PARSE: return "PARSE";
        case QueryPhase::VALIDATE: return "VALIDATE";
        case QueryPhase::OPTIMIZE: return "OPTIMIZE";
        case QueryPhase::PLAN: return "PLAN";
        case QueryPhase::EXECUTE: return "EXECUTE";
        case QueryPhase::FETCH_RESULTS: return "FETCH_RESULTS";
        default: return "UNKNOWN";
    }
}

/**
 * @brief To string.
 * @param[in] type Input parameter.
 * @return Pointer to the result.
 * @details Implements to_string without additional internal calls.
 */
const char* to_string(OperatorType type) {
    switch (type) {
        case OperatorType::SCAN: return "SCAN";
        case OperatorType::INDEX_SCAN: return "INDEX_SCAN";
        case OperatorType::FILTER: return "FILTER";
        case OperatorType::PROJECT: return "PROJECT";
        case OperatorType::AGGREGATE: return "AGGREGATE";
        case OperatorType::JOIN: return "JOIN";
        case OperatorType::SORT: return "SORT";
        case OperatorType::LIMIT: return "LIMIT";
        case OperatorType::SUBQUERY: return "SUBQUERY";
        case OperatorType::VECTOR_SEARCH: return "VECTOR_SEARCH";
        case OperatorType::GRAPH_TRAVERSE: return "GRAPH_TRAVERSE";
        default: return "UNKNOWN";
    }
}

json OperatorStats::toJSON() const {
    return json{
        {"type", to_string(type)},
        {"name", name},
        {"duration_us", duration.count()},
        {"rows_processed", rows_processed},
        {"bytes_processed", bytes_processed},
        {"disk_reads", disk_reads},
        {"cache_hits", cache_hits},
        {"cache_misses", cache_misses},
        {"details", details}
    };
}

json QueryProfile::toJSON() const {
    json phases_json = json::object();
    for (const auto& [phase, duration] : phase_timings) {
        phases_json[to_string(phase)] = duration.count();
    }
    
    json operators_json = json::array();
    for (const auto& op : operator_stats) {
        operators_json.push_back(op.toJSON());
    }
    
    auto time_t_val = std::chrono::system_clock::to_time_t(start_time);
    
    return json{
        {"query_id", query_id},
        {"query_text", query_text},
        {"start_time", time_t_val},
        {"total_duration_us", total_duration.count()},
        {"phase_timings", phases_json},
        {"operator_stats", operators_json},
        {"resource_usage", {
            {"peak_memory_bytes", peak_memory_bytes},
            {"total_disk_io_bytes", total_disk_io_bytes},
            {"total_network_bytes", total_network_bytes}
        }},
        {"optimization", {
            {"used_index", used_index},
            {"used_cache", used_cache},
            {"indexes_used", indexes_used},
            {"warnings", warnings},
            {"hints", optimization_hints}
        }},
        {"results", {
            {"rows", result_rows},
            {"bytes", result_bytes}
        }}
    };
}

std::string QueryProfile::toSummary() const {
    std::ostringstream oss = {};
    oss << "Query: " << query_id << "\n";
    oss << "Duration: " << std::fixed << std::setprecision(2) 
        << (total_duration.count() / 1000.0) << " ms\n";
    oss << "Rows: " << result_rows << "\n";
    
    if (used_index) {
        oss << "Indexes used: ";
        for (size_t i = 0; i < indexes_used.size(); ++i) {
            if (i > 0) {
              oss << ", ";
            }
            oss << indexes_used[i];
        }
        oss << "\n";
    }
    
    if (!warnings.empty()) {
        oss << "Warnings:\n";
        for (const auto& warning : warnings) {
            oss << "  - " << warning << "\n";
        }
    }
    
    if (!optimization_hints.empty()) {
        oss << "Optimization hints:\n";
        for (const auto& hint : optimization_hints) {
            oss << "  - " << hint << "\n";
        }
    }
    
    return oss.str();
}

// QueryProfiler::Impl
class QueryProfiler::Impl {
public:
    QueryProfilerConfig config;
    std::unordered_map<std::string, std::shared_ptr<QueryProfile>> profiles;
    mutable std::mutex mutex;
    
    explicit Impl(const QueryProfilerConfig& cfg) : config(cfg) {}
    
    /**
     * @brief Cleanup old.
     * @details Calls: std::chrono::system_clock::now(), push_back(), erase(), size(), std::sort(), begin(), end().
     */
    void cleanup_old() {
        auto now = std::chrono::system_clock::now();
        std::vector<std::string> to_remove;
        
        for (const auto& [id, profile] : profiles) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                now - profile->start_time);
            if (age > config.retention_duration) {
                to_remove.push_back(id);
            }
        }
        
        for (const auto& id : to_remove) {
            profiles.erase(id);
        }
        
        // Keep only max_profiles_retained most recent
        if (profiles.size() > config.max_profiles_retained) {
            std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> times;
            for (const auto& [id, profile] : profiles) {
                times.push_back({id, profile->start_time});
            }
            std::sort(times.begin(), times.end(),
                     [](const auto& a, const auto& b) { return a.second < b.second; });
            
            size_t to_delete = profiles.size() - config.max_profiles_retained;
            for (size_t i = 0; i < to_delete; ++i) {
                profiles.erase(times[i].first);
            }
        }
    }
};

// QueryProfiler implementation
QueryProfiler::QueryProfiler(const QueryProfilerConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

QueryProfiler::~QueryProfiler() = default;

/**
 * @brief Start query.
 * @param[in] query_id Identifier of the query.
 * @param[in] query_text Input parameter.
 * @return Return value.
 * @details Calls: lock(), std::chrono::system_clock::now(), cleanup_old_profiles().
 */
std::string QueryProfiler::start_query(const std::string& query_id, 
                                      const std::string& query_text) {
    if (!impl_->config.enabled) {
        return query_id;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto profile = std::make_shared<QueryProfile>();
    profile->query_id = query_id;
    profile->query_text = query_text;
    profile->start_time = std::chrono::system_clock::now();
    
    impl_->profiles[query_id] = profile;
    
    cleanup_old_profiles();
    
    return query_id;
}

/**
 * @brief End query.
 * @param[in] query_id Identifier of the query.
 * @details Calls: lock(), find(), end(), std::chrono::system_clock::now(), log_slow_query().
 */
void QueryProfiler::end_query(const std::string& query_id) {
    if (!impl_->config.enabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto it = impl_->profiles.find(query_id);
    if (it != impl_->profiles.end()) {
        auto& profile = it->second;
        auto now = std::chrono::system_clock::now();
        profile->total_duration = std::chrono::duration_cast<std::chrono::microseconds>(
            now - profile->start_time);
        
        if (impl_->config.log_slow_queries && 
            profile->total_duration >= std::chrono::duration_cast<std::chrono::microseconds>(
                impl_->config.slow_query_threshold)) {
            log_slow_query(*profile);
        }
    }
}

/**
 * @brief Record phase.
 * @param[in] query_id Identifier of the query.
 * @param[in] phase Input parameter.
 * @param[in] duration Input parameter.
 * @details Calls: lock(), find(), end().
 */
void QueryProfiler::record_phase(const std::string& query_id, QueryPhase phase,
                                std::chrono::microseconds duration) {
    if (!impl_->config.enabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto it = impl_->profiles.find(query_id);
    if (it != impl_->profiles.end()) {
        it->second->phase_timings[phase] = duration;
    }
}

/**
 * @brief Record operator.
 * @param[in] query_id Identifier of the query.
 * @param[in] stats Input parameter.
 * @details Calls: lock(), find(), end(), push_back().
 */
void QueryProfiler::record_operator(const std::string& query_id, 
                                   const OperatorStats& stats) {
    if (!impl_->config.enabled || !impl_->config.collect_operator_stats) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto it = impl_->profiles.find(query_id);
    if (it != impl_->profiles.end()) {
        it->second->operator_stats.push_back(stats);
    }
}

/**
 * @brief Record index usage.
 * @param[in] query_id Identifier of the query.
 * @param[in] index_name Name of the index.
 * @details Calls: lock(), find(), end(), push_back().
 */
void QueryProfiler::record_index_usage(const std::string& query_id, 
                                      const std::string& index_name) {
    if (!impl_->config.enabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto it = impl_->profiles.find(query_id);
    if (it != impl_->profiles.end()) {
        it->second->used_index = true;
        it->second->indexes_used.push_back(index_name);
    }
}

/**
 * @brief Record cache usage.
 * @param[in] query_id Identifier of the query.
 * @param[in] cache_hit Input parameter.
 * @details Calls: lock(), find(), end().
 */
void QueryProfiler::record_cache_usage(const std::string& query_id, bool cache_hit) {
    if (!impl_->config.enabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto it = impl_->profiles.find(query_id);
    if (it != impl_->profiles.end()) {
        if (cache_hit) {
            it->second->used_cache = true;
        }
    }
}

/**
 * @brief Add hint.
 * @param[in] query_id Identifier of the query.
 * @param[in] hint Input parameter.
 * @details Calls: lock(), find(), end(), push_back().
 */
void QueryProfiler::add_hint(const std::string& query_id, const std::string& hint) {
    if (!impl_->config.enabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto it = impl_->profiles.find(query_id);
    if (it != impl_->profiles.end()) {
        it->second->optimization_hints.push_back(hint);
    }
}

/**
 * @brief Add warning.
 * @param[in] query_id Identifier of the query.
 * @param[in] warning Input parameter.
 * @details Calls: lock(), find(), end(), push_back().
 */
void QueryProfiler::add_warning(const std::string& query_id, const std::string& warning) {
    if (!impl_->config.enabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto it = impl_->profiles.find(query_id);
    if (it != impl_->profiles.end()) {
        it->second->warnings.push_back(warning);
    }
}

std::shared_ptr<QueryProfile> QueryProfiler::get_profile(const std::string& query_id) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto it = impl_->profiles.find(query_id);
    if (it != impl_->profiles.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<QueryProfile>> QueryProfiler::get_all_profiles() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    std::vector<std::shared_ptr<QueryProfile>> result;
    result.reserve(impl_->profiles.size());
    for (const auto& [_, profile] : impl_->profiles) {
        result.push_back(profile);
    }
    return result;
}

std::vector<std::shared_ptr<QueryProfile>> QueryProfiler::get_slow_queries(
    std::chrono::milliseconds threshold) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    std::vector<std::shared_ptr<QueryProfile>> result;
    auto threshold_us = std::chrono::duration_cast<std::chrono::microseconds>(threshold);
    
    for (const auto& [_, profile] : impl_->profiles) {
        if (profile->total_duration >= threshold_us) {
            result.push_back(profile);
        }
    }
    
    std::sort(result.begin(), result.end(),
             [](const auto& a, const auto& b) {
                 return a->total_duration > b->total_duration;
             });
    
    return result;
}

std::vector<std::shared_ptr<QueryProfile>> QueryProfiler::get_top_queries(size_t limit) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    std::vector<std::shared_ptr<QueryProfile>> result;
    result.reserve(impl_->profiles.size());
    for (const auto& [_, profile] : impl_->profiles) {
        result.push_back(profile);
    }
    
    std::sort(result.begin(), result.end(),
             [](const auto& a, const auto& b) {
                 return a->total_duration > b->total_duration;
             });
    
    if (result.size() > limit) {
        result.resize(limit);
    }
    
    return result;
}

/**
 * @brief Clear.
 * @details Calls: lock().
 */
void QueryProfiler::clear() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->profiles.clear();
}

void QueryProfiler::export_to_json(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    json profiles_json = json::array();
    for (const auto& [_, profile] : impl_->profiles) {
        profiles_json.push_back(profile->toJSON());
    }
    
    /**
     * @brief File.
     * @param[in] filename Input parameter.
     * @return Return value.
     */
    std::ofstream file(filename);
    file << profiles_json.dump(2);
}

QueryProfilerConfig QueryProfiler::get_config() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->config;
}

/**
 * @brief Set config.
 * @param[in] config Input parameter.
 * @details Calls: lock().
 */
void QueryProfiler::set_config(const QueryProfilerConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config = config;
}

/**
 * @brief Enable.
 * @details Calls: lock().
 */
void QueryProfiler::enable() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config.enabled = true;
}

/**
 * @brief Disable.
 * @details Calls: lock().
 */
void QueryProfiler::disable() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config.enabled = false;
}

bool QueryProfiler::is_enabled() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->config.enabled;
}

json QueryProfiler::get_statistics() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    size_t total_queries = impl_->profiles.size();
    std::chrono::microseconds total_duration{0};
    size_t total_rows = 0;
    size_t queries_with_index = 0;
    size_t queries_with_cache = 0;
    
    for (const auto& [_, profile] : impl_->profiles) {
        total_duration += profile->total_duration;
        total_rows += profile->result_rows;
        if (profile->used_index) {
          queries_with_index++;
        }
        if (profile->used_cache) {
          queries_with_cache++;
        }
    }
    
    double avg_duration = total_queries > 0 ? 
        static_cast<double>(total_duration.count()) / total_queries : 0.0;
    
    return json{
        {"total_queries", total_queries},
        {"avg_duration_us", avg_duration},
        {"total_duration_us", total_duration.count()},
        {"total_rows", total_rows},
        {"queries_with_index", queries_with_index},
        {"queries_with_cache", queries_with_cache},
        {"index_usage_pct", total_queries > 0 ? 
            (100.0 * queries_with_index / total_queries) : 0.0},
        {"cache_usage_pct", total_queries > 0 ? 
            (100.0 * queries_with_cache / total_queries) : 0.0}
    };
}

/**
 * @brief Cleanup old profiles.
 * @details Calls: cleanup_old().
 */
void QueryProfiler::cleanup_old_profiles() {
    impl_->cleanup_old();
}

/**
 * @brief Log slow query.
 * @param[in] profile Input parameter.
 * @details Implements log_slow_query without additional internal calls.
 */
void QueryProfiler::log_slow_query(const QueryProfile& profile) {
    (void)profile;
    // Log slow query (could be integrated with logging system)
    // For now, just a placeholder
}

// ScopedQueryProfile implementation
ScopedQueryProfile::ScopedQueryProfile(QueryProfiler& profiler, 
                                     const std::string& query_id,
                                     const std::string& query_text)
    : profiler_(profiler), query_id_(query_id) {
    profiler_.start_query(query_id_, query_text);
}

ScopedQueryProfile::~ScopedQueryProfile() {
    profiler_.end_query(query_id_);
}

/**
 * @brief Record phase.
 * @param[in] phase Input parameter.
 * @param[in] duration Input parameter.
 * @details Implements record_phase without additional internal calls.
 */
void ScopedQueryProfile::record_phase(QueryPhase phase, 
                                     std::chrono::microseconds duration) {
    profiler_.record_phase(query_id_, phase, duration);
}

/**
 * @brief Record operator.
 * @param[in] stats Input parameter.
 * @details Implements record_operator without additional internal calls.
 */
void ScopedQueryProfile::record_operator(const OperatorStats& stats) {
    profiler_.record_operator(query_id_, stats);
}

/**
 * @brief Add hint.
 * @param[in] hint Input parameter.
 * @details Implements add_hint without additional internal calls.
 */
void ScopedQueryProfile::add_hint(const std::string& hint) {
    profiler_.add_hint(query_id_, hint);
}

/**
 * @brief Add warning.
 * @param[in] warning Input parameter.
 * @details Implements add_warning without additional internal calls.
 */
void ScopedQueryProfile::add_warning(const std::string& warning) {
    profiler_.add_warning(query_id_, warning);
}

// ScopedOperatorProfile implementation
ScopedOperatorProfile::ScopedOperatorProfile(QueryProfiler& profiler,
                                           const std::string& query_id,
                                           OperatorType type,
                                           const std::string& name)
    : profiler_(profiler), query_id_(query_id), 
      start_(std::chrono::high_resolution_clock::now()) {
    stats_.type = type;
    stats_.name = name;
}

ScopedOperatorProfile::~ScopedOperatorProfile() {
    auto end = std::chrono::high_resolution_clock::now();
    stats_.duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
    profiler_.record_operator(query_id_, stats_);
}

/**
 * @brief Record rows.
 * @param[in] count Input parameter.
 * @details Implements record_rows without additional internal calls.
 */
void ScopedOperatorProfile::record_rows(size_t count) {
    stats_.rows_processed += count;
}

/**
 * @brief Record bytes.
 * @param[in] count Input parameter.
 * @details Implements record_bytes without additional internal calls.
 */
void ScopedOperatorProfile::record_bytes(size_t count) {
    stats_.bytes_processed += count;
}

/**
 * @brief Record disk read.
 * @details Implements record_disk_read without additional internal calls.
 */
void ScopedOperatorProfile::record_disk_read() {
    stats_.disk_reads++;
}

/**
 * @brief Record cache hit.
 * @details Implements record_cache_hit without additional internal calls.
 */
void ScopedOperatorProfile::record_cache_hit() {
    stats_.cache_hits++;
}

/**
 * @brief Record cache miss.
 * @details Implements record_cache_miss without additional internal calls.
 */
void ScopedOperatorProfile::record_cache_miss() {
    stats_.cache_misses++;
}

/**
 * @brief Set details.
 * @param[in] details Input parameter.
 * @details Implements set_details without additional internal calls.
 */
void ScopedOperatorProfile::set_details(const std::string& details) {
    stats_.details = details;
}

} // namespace observability
} // namespace themis
