/**
 * @file performance_analyzer.h
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
#include <memory>
#include <nlohmann/json.hpp>
#include "observability/query_profiler.h"
#include "observability/storage_profiler.h"

namespace themis {
namespace observability {

using json = nlohmann::json;

/**
 * @brief Performance issue severity
 */
enum class IssueSeverity {
    INFO,
    WARNING,
    CRITICAL
};

/**
 * @brief Performance issue category
 */
enum class IssueCategory {
    QUERY_OPTIMIZATION,
    INDEX_USAGE,
    CACHE_EFFICIENCY,
    STORAGE_AMPLIFICATION,
    RESOURCE_USAGE,
    SLOW_OPERATIONS
};

/**
 * @brief Performance issue
 */
struct PerformanceIssue {
    IssueSeverity severity;
    IssueCategory category;
    std::string title;
    std::string description;
    std::vector<std::string> recommendations;
    json metrics;
    
    json toJSON() const;
};

/**
 * @brief Performance analysis result
 */
struct PerformanceAnalysis {
    std::chrono::system_clock::time_point timestamp;
    std::vector<PerformanceIssue> issues;
    json summary_metrics;
    json recommendations;
    
    json toJSON() const;
    std::string toReport() const;
};

/**
 * @brief Performance analyzer configuration
 */
struct PerformanceAnalyzerConfig {
    // Thresholds
    std::chrono::milliseconds slow_query_threshold{1000};
    std::chrono::milliseconds slow_storage_op_threshold{100};
    double cache_hit_rate_threshold = 80.0; // %
    double index_usage_threshold = 50.0; // %
    double write_amplification_threshold = 10.0;
    double read_amplification_threshold = 5.0;
    size_t max_full_scan_threshold = 1000; // rows
    
    // Analysis settings
    bool analyze_queries = true;
    bool analyze_storage = true;
    bool analyze_cache = true;
    bool analyze_indexes = true;
    bool generate_recommendations = true;
};

/**
 * @brief Performance analyzer
 * 
 * Analyzes query and storage profiles to identify performance
 * issues and provide optimization recommendations.
 */
class PerformanceAnalyzer {
public:
    explicit PerformanceAnalyzer(const PerformanceAnalyzerConfig& config = 
                                PerformanceAnalyzerConfig{});
    ~PerformanceAnalyzer();
    
    // Disable copy
    PerformanceAnalyzer(const PerformanceAnalyzer&) = delete;
    PerformanceAnalyzer& operator=(const PerformanceAnalyzer&) = delete;
    
    /**
     * @brief Analyze performance using profilers
     * @param query_profiler Query profiler instance
     * @param storage_profiler Storage profiler instance
     * @return Performance analysis result
     */
    PerformanceAnalysis analyze(const QueryProfiler& query_profiler,
                               const StorageProfiler& storage_profiler);
    
    /**
     * @brief Analyze query performance
     * @param query_profiler Query profiler instance
     * @return Vector of identified issues
     */
    std::vector<PerformanceIssue> analyze_queries(const QueryProfiler& query_profiler);
    
    /**
     * @brief Analyze storage performance
     * @param storage_profiler Storage profiler instance
     * @return Vector of identified issues
     */
    std::vector<PerformanceIssue> analyze_storage(const StorageProfiler& storage_profiler);
    
    /**
     * @brief Analyze cache performance
     * @param query_profiler Query profiler instance
     * @param storage_profiler Storage profiler instance
     * @return Vector of identified issues
     */
    std::vector<PerformanceIssue> analyze_cache(const QueryProfiler& query_profiler,
                                                const StorageProfiler& storage_profiler);
    
    /**
     * @brief Analyze index usage
     * @param query_profiler Query profiler instance
     * @return Vector of identified issues
     */
    std::vector<PerformanceIssue> analyze_indexes(const QueryProfiler& query_profiler);
    
    /**
     * @brief Generate optimization recommendations
     * @param issues Identified issues
     * @return JSON with recommendations
     */
    json generate_recommendations(const std::vector<PerformanceIssue>& issues);
    
    /**
     * @brief Get configuration
     */
    PerformanceAnalyzerConfig get_config() const;
    
    /**
     * @brief Set configuration
     */
    void set_config(const PerformanceAnalyzerConfig& config);
    
    /**
     * @brief Export analysis to JSON file
     * @param analysis Analysis result
     * @param filename Output file path
     */
    void export_analysis(const PerformanceAnalysis& analysis, 
                        const std::string& filename) const;
    
    /**
     * @brief Export analysis to HTML report
     * @param analysis Analysis result
     * @param filename Output file path
     */
    void export_html_report(const PerformanceAnalysis& analysis,
                           const std::string& filename) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    PerformanceIssue check_slow_queries(const QueryProfiler& query_profiler);
    PerformanceIssue check_full_scans(const QueryProfiler& query_profiler);
    PerformanceIssue check_index_usage(const QueryProfiler& query_profiler);
    PerformanceIssue check_cache_hit_rate(const QueryProfiler& query_profiler,
                                         const StorageProfiler& storage_profiler);
    PerformanceIssue check_write_amplification(const StorageProfiler& storage_profiler);
    PerformanceIssue check_read_amplification(const StorageProfiler& storage_profiler);
    PerformanceIssue check_slow_storage_ops(const StorageProfiler& storage_profiler);
    
    std::string generate_html_header() const;
    std::string generate_html_footer() const;
    std::string generate_html_issue_section(const PerformanceIssue& issue) const;
};

// Helper functions
const char* to_string(IssueSeverity severity);
const char* to_string(IssueCategory category);

} // namespace observability
} // namespace themis
