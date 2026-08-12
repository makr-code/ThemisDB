/**
 * @file performance_analyzer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=22, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/performance_analyzer.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <unordered_map>

namespace themis {
namespace observability {

// Helper functions
const char* to_string(IssueSeverity severity) {
    switch (severity) {
        case IssueSeverity::INFO: return "INFO";
        case IssueSeverity::WARNING: return "WARNING";
        case IssueSeverity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

const char* to_string(IssueCategory category) {
    switch (category) {
        case IssueCategory::QUERY_OPTIMIZATION: return "QUERY_OPTIMIZATION";
        case IssueCategory::INDEX_USAGE: return "INDEX_USAGE";
        case IssueCategory::CACHE_EFFICIENCY: return "CACHE_EFFICIENCY";
        case IssueCategory::STORAGE_AMPLIFICATION: return "STORAGE_AMPLIFICATION";
        case IssueCategory::RESOURCE_USAGE: return "RESOURCE_USAGE";
        case IssueCategory::SLOW_OPERATIONS: return "SLOW_OPERATIONS";
        default: return "UNKNOWN";
    }
}

json PerformanceIssue::toJSON() const {
    json recs = json::array();
    for (const auto& rec : recommendations) {
        recs.push_back(rec);
    }
    
    return json{
        {"severity", to_string(severity)},
        {"category", to_string(category)},
        {"title", title},
        {"description", description},
        {"recommendations", recs},
        {"metrics", metrics}
    };
}

json PerformanceAnalysis::toJSON() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(timestamp);
    
    json issues_json = json::array();
    for (const auto& issue : issues) {
        issues_json.push_back(issue.toJSON());
    }
    
    return json{
        {"timestamp", time_t_val},
        {"issues", issues_json},
        {"summary_metrics", summary_metrics},
        {"recommendations", recommendations}
    };
}

std::string PerformanceAnalysis::toReport() const {
    std::ostringstream oss;
    
    oss << "=== ThemisDB Performance Analysis Report ===\n\n";
    
    auto time_t_val = std::chrono::system_clock::to_time_t(timestamp);
    char time_buf[100];
    std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t_val));
    oss << "Generated: " << time_buf << "\n\n";
    
    // Summary
    oss << "--- Summary ---\n";
    size_t critical = 0, warning = 0, info = 0;
    for (const auto& issue : issues) {
        switch (issue.severity) {
            case IssueSeverity::CRITICAL: critical++; break;
            case IssueSeverity::WARNING: warning++; break;
            case IssueSeverity::INFO: info++; break;
        }
    }
    oss << "Total Issues: " << issues.size() << "\n";
    oss << "  Critical: " << critical << "\n";
    oss << "  Warning: " << warning << "\n";
    oss << "  Info: " << info << "\n\n";
    
    // Issues
    oss << "--- Issues ---\n\n";
    for (size_t i = 0; i < issues.size(); ++i) {
        const auto& issue = issues[i];
        oss << i + 1 << ". [" << to_string(issue.severity) << "] " 
            << issue.title << "\n";
        oss << "   Category: " << to_string(issue.category) << "\n";
        oss << "   " << issue.description << "\n";
        
        if (!issue.recommendations.empty()) {
            oss << "   Recommendations:\n";
            for (const auto& rec : issue.recommendations) {
                oss << "   - " << rec << "\n";
            }
        }
        oss << "\n";
    }
    
    return oss.str();
}

// PerformanceAnalyzer::Impl
class PerformanceAnalyzer::Impl {
public:
    PerformanceAnalyzerConfig config;
    
    explicit Impl(const PerformanceAnalyzerConfig& cfg) : config(cfg) {}
};

// PerformanceAnalyzer implementation
PerformanceAnalyzer::PerformanceAnalyzer(const PerformanceAnalyzerConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

PerformanceAnalyzer::~PerformanceAnalyzer() = default;

PerformanceAnalysis PerformanceAnalyzer::analyze(const QueryProfiler& query_profiler,
                                                const StorageProfiler& storage_profiler) {
    PerformanceAnalysis analysis;
    analysis.timestamp = std::chrono::system_clock::now();
    
    // Collect issues from different analyzers
    if (impl_->config.analyze_queries) {
        auto query_issues = analyze_queries(query_profiler);
        analysis.issues.insert(analysis.issues.end(), 
                             query_issues.begin(), query_issues.end());
    }
    
    if (impl_->config.analyze_storage) {
        auto storage_issues = analyze_storage(storage_profiler);
        analysis.issues.insert(analysis.issues.end(),
                             storage_issues.begin(), storage_issues.end());
    }
    
    if (impl_->config.analyze_cache) {
        auto cache_issues = analyze_cache(query_profiler, storage_profiler);
        analysis.issues.insert(analysis.issues.end(),
                             cache_issues.begin(), cache_issues.end());
    }
    
    if (impl_->config.analyze_indexes) {
        auto index_issues = analyze_indexes(query_profiler);
        analysis.issues.insert(analysis.issues.end(),
                             index_issues.begin(), index_issues.end());
    }
    
    // Generate summary metrics
    analysis.summary_metrics = json{
        {"total_issues", analysis.issues.size()},
        {"critical_issues", std::count_if(analysis.issues.begin(), analysis.issues.end(),
            [](const auto& i) { return i.severity == IssueSeverity::CRITICAL; })},
        {"warning_issues", std::count_if(analysis.issues.begin(), analysis.issues.end(),
            [](const auto& i) { return i.severity == IssueSeverity::WARNING; })},
        {"info_issues", std::count_if(analysis.issues.begin(), analysis.issues.end(),
            [](const auto& i) { return i.severity == IssueSeverity::INFO; })}
    };
    
    // Generate recommendations
    if (impl_->config.generate_recommendations) {
        analysis.recommendations = generate_recommendations(analysis.issues);
    }
    
    return analysis;
}

std::vector<PerformanceIssue> PerformanceAnalyzer::analyze_queries(
    const QueryProfiler& query_profiler) {
    std::vector<PerformanceIssue> issues;
    
    // Check slow queries
    auto slow_query_issue = check_slow_queries(query_profiler);
    if (!slow_query_issue.title.empty()) {
        issues.push_back(slow_query_issue);
    }
    
    // Check full scans
    auto full_scan_issue = check_full_scans(query_profiler);
    if (!full_scan_issue.title.empty()) {
        issues.push_back(full_scan_issue);
    }
    
    return issues;
}

std::vector<PerformanceIssue> PerformanceAnalyzer::analyze_storage(
    const StorageProfiler& storage_profiler) {
    std::vector<PerformanceIssue> issues;
    
    // Check write amplification
    auto write_amp_issue = check_write_amplification(storage_profiler);
    if (!write_amp_issue.title.empty()) {
        issues.push_back(write_amp_issue);
    }
    
    // Check read amplification
    auto read_amp_issue = check_read_amplification(storage_profiler);
    if (!read_amp_issue.title.empty()) {
        issues.push_back(read_amp_issue);
    }
    
    // Check slow storage operations
    auto slow_op_issue = check_slow_storage_ops(storage_profiler);
    if (!slow_op_issue.title.empty()) {
        issues.push_back(slow_op_issue);
    }
    
    return issues;
}

std::vector<PerformanceIssue> PerformanceAnalyzer::analyze_cache(
    const QueryProfiler& query_profiler,
    const StorageProfiler& storage_profiler) {
    std::vector<PerformanceIssue> issues;
    
    auto cache_issue = check_cache_hit_rate(query_profiler, storage_profiler);
    if (!cache_issue.title.empty()) {
        issues.push_back(cache_issue);
    }
    
    return issues;
}

std::vector<PerformanceIssue> PerformanceAnalyzer::analyze_indexes(
    const QueryProfiler& query_profiler) {
    std::vector<PerformanceIssue> issues;
    
    auto index_issue = check_index_usage(query_profiler);
    if (!index_issue.title.empty()) {
        issues.push_back(index_issue);
    }
    
    return issues;
}

json PerformanceAnalyzer::generate_recommendations(
    const std::vector<PerformanceIssue>& issues) {
    json recommendations = json::array();
    
    std::unordered_map<IssueCategory, size_t> category_counts;
    for (const auto& issue : issues) {
        category_counts[issue.category]++;
    }
    
    // Generate high-level recommendations based on issue patterns
    if (category_counts[IssueCategory::INDEX_USAGE] > 0) {
        recommendations.push_back("Review and create missing indexes for frequently queried columns");
    }
    
    if (category_counts[IssueCategory::CACHE_EFFICIENCY] > 0) {
        recommendations.push_back("Increase cache size or improve cache key design");
    }
    
    if (category_counts[IssueCategory::STORAGE_AMPLIFICATION] > 0) {
        recommendations.push_back("Tune RocksDB compaction settings to reduce amplification");
    }
    
    if (category_counts[IssueCategory::SLOW_OPERATIONS] > 0) {
        recommendations.push_back("Optimize slow queries and consider query rewrites");
    }
    
    return recommendations;
}

PerformanceAnalyzerConfig PerformanceAnalyzer::get_config() const {
    return impl_->config;
}

void PerformanceAnalyzer::set_config(const PerformanceAnalyzerConfig& config) {
    impl_->config = config;
}

void PerformanceAnalyzer::export_analysis(const PerformanceAnalysis& analysis,
                                         const std::string& filename) const {
    std::ofstream file(filename);
    file << analysis.toJSON().dump(2);
}

void PerformanceAnalyzer::export_html_report(const PerformanceAnalysis& analysis,
                                            const std::string& filename) const {
    std::ofstream file(filename);
    
    file << generate_html_header();
    file << "<h1>ThemisDB Performance Analysis Report</h1>\n";
    
    // Summary section
    file << "<div class='summary'>\n";
    file << "<h2>Summary</h2>\n";
    file << "<p>Total Issues: " << analysis.issues.size() << "</p>\n";
    file << "</div>\n";
    
    // Issues section
    file << "<div class='issues'>\n";
    file << "<h2>Issues</h2>\n";
    for (const auto& issue : analysis.issues) {
        file << generate_html_issue_section(issue);
    }
    file << "</div>\n";
    
    file << generate_html_footer();
}

// Private helper methods
PerformanceIssue PerformanceAnalyzer::check_slow_queries(const QueryProfiler& query_profiler) {
    auto slow_queries = query_profiler.get_slow_queries(impl_->config.slow_query_threshold);
    
    if (slow_queries.empty()) {
        return PerformanceIssue{};
    }
    
    PerformanceIssue issue;
    issue.severity = slow_queries.size() > 10 ? IssueSeverity::CRITICAL : IssueSeverity::WARNING;
    issue.category = IssueCategory::SLOW_OPERATIONS;
    issue.title = "Slow Queries Detected";
    issue.description = "Found " + std::to_string(slow_queries.size()) + 
                       " queries exceeding threshold of " + 
                       std::to_string(impl_->config.slow_query_threshold.count()) + " ms";
    issue.recommendations = {
        "Review slow query execution plans",
        "Add appropriate indexes",
        "Consider query caching",
        "Optimize query predicates"
    };
    issue.metrics = json{
        {"count", slow_queries.size()},
        {"threshold_ms", impl_->config.slow_query_threshold.count()}
    };
    
    return issue;
}

PerformanceIssue PerformanceAnalyzer::check_full_scans(const QueryProfiler& query_profiler) {
    auto stats = query_profiler.get_statistics();

    // Derive a full-scan proxy from queries that ran without index support.
    // QueryProfiler does not expose an explicit full_scan_count field yet;
    // queries_with_index == 0 means the executor fell back to a sequential scan.
    // Note: this is an approximation — partial-index or covering-index usage
    // still sets `used_index = true`, so the proxy may undercount.  If the
    // counters are inconsistent (queries_with_index > total_queries) we treat
    // the situation conservatively as zero full scans rather than underflowing.
    const size_t total_queries      = stats.value("total_queries",      static_cast<size_t>(0));
    const size_t queries_with_index = stats.value("queries_with_index", static_cast<size_t>(0));
    const size_t full_scan_proxy    = (queries_with_index <= total_queries)
                                      ? (total_queries - queries_with_index)
                                      : 0u;

    if (full_scan_proxy == 0 || full_scan_proxy < impl_->config.max_full_scan_threshold) {
        return PerformanceIssue{};
    }

    PerformanceIssue issue;
    issue.severity  = (full_scan_proxy > impl_->config.max_full_scan_threshold * 10)
                      ? IssueSeverity::CRITICAL
                      : IssueSeverity::WARNING;
    issue.category  = IssueCategory::QUERY_OPTIMIZATION;
    issue.title     = "High Full-Scan Query Rate";
    issue.description =
        std::to_string(full_scan_proxy) + " of " + std::to_string(total_queries) +
        " recent queries ran without index support (potential full-table scans).";
    issue.recommendations = {
        "Add indexes on frequently filtered columns",
        "Review query predicates with EXPLAIN",
        "Ensure statistics are up-to-date",
        "Consider partial or composite indexes"
    };
    issue.metrics = json{
        {"total_queries",      total_queries},
        {"queries_with_index", queries_with_index},
        {"full_scan_proxy",    full_scan_proxy},
        {"threshold",          impl_->config.max_full_scan_threshold}
    };
    return issue;
}

PerformanceIssue PerformanceAnalyzer::check_index_usage(const QueryProfiler& query_profiler) {
    auto stats = query_profiler.get_statistics();
    double index_usage_pct = stats.value("index_usage_pct", 0.0);
    
    if (index_usage_pct >= impl_->config.index_usage_threshold) {
        return PerformanceIssue{};
    }
    
    PerformanceIssue issue;
    issue.severity = index_usage_pct < 25.0 ? IssueSeverity::CRITICAL : IssueSeverity::WARNING;
    issue.category = IssueCategory::INDEX_USAGE;
    issue.title = "Low Index Usage";
    issue.description = "Only " + std::to_string(index_usage_pct) + 
                       "% of queries use indexes";
    issue.recommendations = {
        "Identify frequently queried columns",
        "Create indexes on filter predicates",
        "Review query patterns",
        "Use EXPLAIN to analyze query plans"
    };
    issue.metrics = json{
        {"index_usage_pct", index_usage_pct},
        {"threshold_pct", impl_->config.index_usage_threshold}
    };
    
    return issue;
}

PerformanceIssue PerformanceAnalyzer::check_cache_hit_rate(
    const QueryProfiler& query_profiler,
    const StorageProfiler& storage_profiler) {
    (void)query_profiler;
    auto cache_metrics = storage_profiler.get_cache_metrics();
    
    if (cache_metrics.empty() || !cache_metrics.contains("operation_cache")) {
        return PerformanceIssue{};
    }
    
    double hit_rate = cache_metrics["operation_cache"]["hit_rate_pct"];
    
    if (hit_rate >= impl_->config.cache_hit_rate_threshold) {
        return PerformanceIssue{};
    }
    
    PerformanceIssue issue;
    issue.severity = hit_rate < 50.0 ? IssueSeverity::CRITICAL : IssueSeverity::WARNING;
    issue.category = IssueCategory::CACHE_EFFICIENCY;
    issue.title = "Low Cache Hit Rate";
    issue.description = "Cache hit rate is " + std::to_string(hit_rate) + "%";
    issue.recommendations = {
        "Increase cache size",
        "Review cache eviction policy",
        "Analyze access patterns",
        "Consider warming up cache"
    };
    issue.metrics = cache_metrics;
    
    return issue;
}

PerformanceIssue PerformanceAnalyzer::check_write_amplification(
    const StorageProfiler& storage_profiler) {
    auto amp_metrics = storage_profiler.get_amplification_metrics();
    
    if (amp_metrics.empty()) {
        return PerformanceIssue{};
    }
    
    double write_amp = amp_metrics.value("write_amplification", 0.0);
    
    if (write_amp <= impl_->config.write_amplification_threshold) {
        return PerformanceIssue{};
    }
    
    PerformanceIssue issue;
    issue.severity = write_amp > 20.0 ? IssueSeverity::CRITICAL : IssueSeverity::WARNING;
    issue.category = IssueCategory::STORAGE_AMPLIFICATION;
    issue.title = "High Write Amplification";
    issue.description = "Write amplification factor is " + std::to_string(write_amp);
    issue.recommendations = {
        "Increase memtable size",
        "Adjust compaction settings",
        "Use larger SST file sizes",
        "Consider leveled compaction"
    };
    issue.metrics = amp_metrics;
    
    return issue;
}

PerformanceIssue PerformanceAnalyzer::check_read_amplification(
    const StorageProfiler& storage_profiler) {
    auto amp_metrics = storage_profiler.get_amplification_metrics();
    
    if (amp_metrics.empty()) {
        return PerformanceIssue{};
    }
    
    double read_amp = amp_metrics.value("read_amplification", 0.0);
    
    if (read_amp <= impl_->config.read_amplification_threshold) {
        return PerformanceIssue{};
    }
    
    PerformanceIssue issue;
    issue.severity = read_amp > 10.0 ? IssueSeverity::CRITICAL : IssueSeverity::WARNING;
    issue.category = IssueCategory::STORAGE_AMPLIFICATION;
    issue.title = "High Read Amplification";
    issue.description = "Read amplification factor is " + std::to_string(read_amp);
    issue.recommendations = {
        "Increase block cache size",
        "Enable bloom filters",
        "Optimize compaction strategy",
        "Consider point lookups vs range scans"
    };
    issue.metrics = amp_metrics;
    
    return issue;
}

PerformanceIssue PerformanceAnalyzer::check_slow_storage_ops(
    const StorageProfiler& storage_profiler) {
    auto slow_ops = storage_profiler.get_slow_operations(impl_->config.slow_storage_op_threshold);
    
    if (slow_ops.empty()) {
        return PerformanceIssue{};
    }
    
    PerformanceIssue issue;
    issue.severity = slow_ops.size() > 100 ? IssueSeverity::CRITICAL : IssueSeverity::WARNING;
    issue.category = IssueCategory::SLOW_OPERATIONS;
    issue.title = "Slow Storage Operations";
    issue.description = "Found " + std::to_string(slow_ops.size()) + 
                       " storage operations exceeding threshold";
    issue.recommendations = {
        "Check disk I/O performance",
        "Review RocksDB configuration",
        "Consider SSD storage",
        "Optimize batch operations"
    };
    issue.metrics = json{
        {"count", slow_ops.size()},
        {"threshold_ms", impl_->config.slow_storage_op_threshold.count()}
    };
    
    return issue;
}

std::string PerformanceAnalyzer::generate_html_header() const {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <title>ThemisDB Performance Analysis Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .summary { background-color: #f0f0f0; padding: 15px; margin: 20px 0; }
        .issue { border: 1px solid #ddd; padding: 15px; margin: 10px 0; }
        .critical { border-left: 5px solid #d9534f; }
        .warning { border-left: 5px solid #f0ad4e; }
        .info { border-left: 5px solid #5bc0de; }
        h1, h2, h3 { color: #333; }
    </style>
</head>
<body>
)";
}

std::string PerformanceAnalyzer::generate_html_footer() const {
    return "</body>\n</html>\n";
}

std::string PerformanceAnalyzer::generate_html_issue_section(const PerformanceIssue& issue) const {
    std::ostringstream oss;
    
    std::string severity_class;
    switch (issue.severity) {
        case IssueSeverity::CRITICAL: severity_class = "critical"; break;
        case IssueSeverity::WARNING: severity_class = "warning"; break;
        case IssueSeverity::INFO: severity_class = "info"; break;
    }
    
    oss << "<div class='issue " << severity_class << "'>\n";
    oss << "<h3>" << issue.title << "</h3>\n";
    oss << "<p><strong>Severity:</strong> " << to_string(issue.severity) << "</p>\n";
    oss << "<p><strong>Category:</strong> " << to_string(issue.category) << "</p>\n";
    oss << "<p>" << issue.description << "</p>\n";
    
    if (!issue.recommendations.empty()) {
        oss << "<p><strong>Recommendations:</strong></p>\n<ul>\n";
        for (const auto& rec : issue.recommendations) {
            oss << "<li>" << rec << "</li>\n";
        }
        oss << "</ul>\n";
    }
    
    oss << "</div>\n";
    
    return oss.str();
}

} // namespace observability
} // namespace themis

