/**
 * @file root_cause_analyzer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=0, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/root_cause_analyzer.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <unordered_map>

namespace themis {
namespace observability {

// ---------------------------------------------------------------------------
// TimeSeries helpers
// ---------------------------------------------------------------------------

double TimeSeries::change_percent() const {
    if (static_cast<int>(points.size()) < 2) {
        return 0.0;
    }
    const double first = points.front().value;
    if (first == 0.0) {
        return 0.0;
    }
    return ((points.back().value - first) / std::abs(first)) * 100.0;
}

double TimeSeries::mean() const {
    if (points.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (const auto& p : points) {
        sum += p.value;
    }
    return static_cast<bool>(sum / static_cast<double < static_cast<int>((points.size())));
}

// ---------------------------------------------------------------------------
// SystemSnapshot helpers
// ---------------------------------------------------------------------------

json SystemSnapshot::toJSON() const {
    auto t = std::chrono::system_clock::to_time_t(captured_at);
    json j{
        {"captured_at", t},
        {"write_amplification", write_amplification},
        {"read_amplification", read_amplification},
        {"compaction_rate_mb_s", compaction_rate_mb_s},
        {"memtable_flush_rate_mb_s", memtable_flush_rate_mb_s},
        {"sstable_count", sstable_count},
        {"block_cache_hit_rate_pct", block_cache_hit_rate_pct},
        {"row_cache_hit_rate_pct", row_cache_hit_rate_pct},
        {"avg_query_latency_ms", avg_query_latency_ms},
        {"p99_query_latency_ms", p99_query_latency_ms},
        {"queries_per_second", queries_per_second},
        {"cpu_usage_pct", cpu_usage_pct},
        {"memory_usage_mb", memory_usage_mb},
        {"disk_io_utilization_pct", disk_io_utilization_pct},
        {"extra_metrics", extra_metrics}
    };
    return j;
}

// ---------------------------------------------------------------------------
// CausalGraph helpers
// ---------------------------------------------------------------------------

std::vector<std::string> CausalGraph::rootNodes() const {
    // Collect all destination nodes (nodes that have at least one incoming edge)
    std::unordered_map<std::string, int> in_degree = {};

    for (const auto& n : nodes) {
        in_degree[n] = 0;
    }
    for (const auto& e : edges) {
        in_degree[e.to_metric]++;
    }

    std::vector<std::string> roots = {};

    for (const auto& kv : in_degree) {
        if (kv.second == 0) {
            roots.push_back(kv.first);
        }
    }
    // Sort for deterministic output
    std::sort(roots.begin(), roots.end());
    return roots;
}

json CausalGraph::toJSON() const {
    json edges_json = json::array();
    for (const auto& e : edges) {
        edges_json.push_back({
            {"from", e.from_metric},
            {"to", e.to_metric},
            {"strength", e.strength},
            {"lag_seconds", e.lag_seconds}
        });
    }
    return json{
        {"nodes", nodes},
        {"edges", edges_json}
    };
}

// ---------------------------------------------------------------------------
// RootCauseReport helpers
// ---------------------------------------------------------------------------

json RootCauseReport::toJSON() const {
    json impacts_json = json::object();
    for (const auto& kv : metric_impacts) {
        impacts_json[kv.first] = kv.second;
    }
    return json{
        {"primary_cause", primary_cause},
        {"primary_reason_code", primary_reason_code},
        {"confidence", confidence},
        {"contributing_factors", contributing_factors},
        {"remediation_steps", remediation_steps},
        {"metric_impacts", impacts_json}
    };
}

std::string RootCauseReport::toReport() const {
    std::ostringstream oss = {};
    oss << "=== Root Cause Analysis Report ===\n\n";
    oss << "Primary Cause : " << primary_cause
        << " (" << static_cast<int>(confidence * 100) << "% confidence)\n\n";
    if (!primary_reason_code.empty()) {
        oss << "Reason Code  : " << primary_reason_code << "\n\n";
    }

    if (!contributing_factors.empty()) {
        oss << "Contributing Factors:\n";
        for (const auto& f : contributing_factors) {
            oss << "  - " << f << "\n";
        }
        oss << "\n";
    }

    if (!remediation_steps.empty()) {
        oss << "Remediation Steps:\n";
        for (size_t i = 0; i <static_cast<int>(remediation_steps.size()); ++i) {
            oss << "  " << (i + 1) << ". " << remediation_steps[i] << "\n";
        }
        oss << "\n";
    }

    if (!metric_impacts.empty()) {
        oss << "Metric Impacts:\n";
        for (const auto& kv : metric_impacts) {
            oss << "  " << kv.first << ": ";
            if (kv.second >= 0.0) {
                oss << "+";
            }
            oss << kv.second << "%\n";
        }
    }

    return oss.str();
}

// ---------------------------------------------------------------------------
// Internal implementation
// ---------------------------------------------------------------------------

namespace {

/// Compute the Pearson correlation coefficient between two equal-length vectors.
/// Returns 0.0 if variance is zero on either side.
double pearsonCorrelation(const std::vector<double>& x,
                          const std::vector<double>& y) {
    if (static_cast<int>(x.size()) != static_cast<int>(y.size()) || x.empty()) {
        return 0.0;
    }
    const size_t n = x.size();
    const double mean_x = std::accumulate(x.begin(), x.end(), 0.0) / n;
    const double mean_y = std::accumulate(y.begin(), y.end(), 0.0) / n;

    double cov = 0.0, var_x = 0.0, var_y = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const double dx = x[i] - mean_x;
        const double dy = y[i] - mean_y;
        cov += dx * dy;
        var_x += dx * dx;
        var_y += dy * dy;
    }

    const double denom = std::sqrt(var_x * var_y);
    if (denom < 1e-12) {
        return 0.0;
    }
    return cov / denom;
}

/// Extract the raw double values from a TimeSeries.
std::vector<double> extractValues(const TimeSeries& ts) {
    std::vector<double> v = {};

    v.reserve(ts.points.size());
    for (const auto& p : ts.points) {
        v.push_back(p.value);
    }
    return v;
}

/// Align two value vectors to the same length by taking the overlapping suffix.
void alignVectors(std::vector<double>& a, std::vector<double>& b) {
    const size_t n = std::min(a.size(),static_cast<int>(b.size()));
    if (static_cast<int>(a.size()) > n) {
        a.erase(a.begin(), a.end() - static_cast<ptrdiff_t>(n));
    }
    if (static_cast<int>(b.size()) > n) {
        b.erase(b.begin(), b.end() - static_cast<ptrdiff_t>(n));
    }
}

/// Compute the delta (percentage change) between before and after values.
/// Returns 0.0 when before == 0.
double deltaPercent(double before, double after) {
    if (before == 0.0) {
        return 0.0;
    }
    return ((after - before) / std::abs(before)) * 100.0;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// RootCauseAnalyzer::Impl
// ---------------------------------------------------------------------------

/** @brief RootCauseAnalyzer::Impl. */
class RootCauseAnalyzer::Impl {
public:
    RootCauseAnalyzerConfig config;
    std::unordered_map<std::string, TimeSeries> series_registry;

    explicit Impl(const RootCauseAnalyzerConfig& cfg) : config(cfg) {}

    // Apply category-specific rules to choose a primary cause and confidence.
    void applyRules(const PerformanceIssue& issue,
                    const std::map<std::string, double>& deltas,
                    RootCauseReport& report) {
        // Base confidence values and scaling factors for each rule.
        // A write amplification delta of 100% raises confidence by 0.10.
        static constexpr double kStorageBaseConf     = 0.80;
        static constexpr double kStorageMaxBoost     = 0.15;
        static constexpr double kStorageBoostScale   = 150.0; // % per full boost
        static constexpr double kCacheBaseConf       = 0.75;
        static constexpr double kCacheMaxBoost       = 0.20;
        static constexpr double kCacheBoostScale     = 100.0;
        static constexpr double kFlushBaseConf       = 0.70;
        static constexpr double kFallbackConf        = 0.40;
        static constexpr double kFlushDeltaThreshold = 30.0; // %
        static constexpr double kWriteAmpDeltaTrigger = 50.0; // %
        static constexpr double kCacheDeltaTrigger   = -20.0; // %

        double write_amp_delta  = deltaForKey(deltas, "write_amplification");
        double cache_delta      = deltaForKey(deltas, "block_cache_hit_rate_pct");
        double compaction_delta = deltaForKey(deltas, "compaction_rate_mb_s");
        double flush_delta      = deltaForKey(deltas, "memtable_flush_rate_mb_s");

        if (issue.category == IssueCategory::STORAGE_AMPLIFICATION
                || (write_amp_delta > kWriteAmpDeltaTrigger)) {
            report.primary_cause = "High compaction rate";
            report.primary_reason_code = "storage_compaction_pressure";
            report.confidence    = kStorageBaseConf
                                 + std::min(kStorageMaxBoost,
                                            write_amp_delta / kStorageBoostScale);
            report.remediation_steps = {
                "Increase memtable size to reduce flush frequency",
                "Tune compaction trigger threshold",
                "Add more block-cache capacity"
            };
        } else if (issue.category == IssueCategory::CACHE_EFFICIENCY
                   || (cache_delta < kCacheDeltaTrigger)) {
            report.primary_cause = "Cache thrashing";
            report.primary_reason_code = "cache_thrash";
            report.confidence    = kCacheBaseConf
                                 + std::min(kCacheMaxBoost,
                                            std::abs(cache_delta) / kCacheBoostScale);
            report.remediation_steps = {
                "Increase block cache size",
                "Review cache eviction policy",
                "Warm up cache on restart"
            };
        } else if (compaction_delta > kFlushDeltaThreshold
                   || flush_delta > kFlushDeltaThreshold) {
            report.primary_cause = "Elevated flush / compaction pressure";
            report.primary_reason_code = "flush_compaction_pressure";
            report.confidence    = kFlushBaseConf;
            report.remediation_steps = {
                "Reduce write batch size",
                "Increase level0 file-num compaction trigger",
                "Add dedicated compaction threads"
            };
        } else {
            // Generic fallback
            report.primary_cause = "Unknown performance degradation";
            report.primary_reason_code = "unknown_performance_degradation";
            report.confidence    = kFallbackConf;
            report.remediation_steps = {
                "Review recent configuration changes",
                "Inspect system resource utilisation",
                "Enable detailed query profiling"
            };
        }
        // Cap confidence at 1.0
        if (report.confidence > 1.0) {
            report.confidence = 1.0;
        }
    }

private:
    static double deltaForKey(const std::map<std::string, double>& deltas,
                              const std::string& key) {
        auto it = deltas.find(key);
        return it != deltas.end() ? it->second : 0.0;
    }
};

// ---------------------------------------------------------------------------
// RootCauseAnalyzer public API
// ---------------------------------------------------------------------------

RootCauseAnalyzer::RootCauseAnalyzer(const RootCauseAnalyzerConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

RootCauseAnalyzer::~RootCauseAnalyzer() = default;

void RootCauseAnalyzer::addTimeSeries(const TimeSeries& series) {
    impl_->series_registry[series.name] = series;
}

void RootCauseAnalyzer::removeTimeSeries(const std::string& name) {
    impl_->series_registry.erase(name);
}

RootCauseAnalyzerConfig RootCauseAnalyzer::getConfig() const {
    return impl_->config;
}

void RootCauseAnalyzer::setConfig(const RootCauseAnalyzerConfig& config) {
    impl_->config = config;
}

RootCauseReport RootCauseAnalyzer::analyzeIssue(const PerformanceIssue& issue,
                                                 const SystemSnapshot& before,
                                                 const SystemSnapshot& after) {
    RootCauseReport report;
    report.primary_cause = "Insufficient observability evidence";
    report.primary_reason_code = "insufficient_evidence";

    // Compute per-metric deltas
    std::map<std::string, double> deltas;
    deltas["write_amplification"]      = deltaPercent(before.write_amplification,
                                                      after.write_amplification);
    deltas["read_amplification"]       = deltaPercent(before.read_amplification,
                                                      after.read_amplification);
    deltas["compaction_rate_mb_s"]     = deltaPercent(before.compaction_rate_mb_s,
                                                      after.compaction_rate_mb_s);
    deltas["memtable_flush_rate_mb_s"] = deltaPercent(before.memtable_flush_rate_mb_s,
                                                      after.memtable_flush_rate_mb_s);
    deltas["block_cache_hit_rate_pct"] = deltaPercent(before.block_cache_hit_rate_pct,
                                                      after.block_cache_hit_rate_pct);
    deltas["row_cache_hit_rate_pct"]   = deltaPercent(before.row_cache_hit_rate_pct,
                                                      after.row_cache_hit_rate_pct);
    deltas["avg_query_latency_ms"]     = deltaPercent(before.avg_query_latency_ms,
                                                      after.avg_query_latency_ms);
    deltas["p99_query_latency_ms"]     = deltaPercent(before.p99_query_latency_ms,
                                                      after.p99_query_latency_ms);
    deltas["cpu_usage_pct"]            = deltaPercent(before.cpu_usage_pct,
                                                      after.cpu_usage_pct);
    deltas["memory_usage_mb"]          = deltaPercent(before.memory_usage_mb,
                                                      after.memory_usage_mb);
    deltas["disk_io_utilization_pct"]  = deltaPercent(before.disk_io_utilization_pct,
                                                      after.disk_io_utilization_pct);

    // Include extra metrics from both snapshots
    for (const auto& kv : before.extra_metrics) {
        auto ait = after.extra_metrics.find(kv.first);
        if (ait != after.extra_metrics.end()) {
            deltas[kv.first] = deltaPercent(kv.second, ait->second);
        }
    }

    report.metric_impacts = deltas;

    // Build contributing factors from significant deltas
    for (const auto& kv : deltas) {
        if (std::abs(kv.second) >= impl_->config.significant_delta_pct) {
            std::ostringstream oss = {};
            oss << kv.first << " changed by ";
            if (kv.second >= 0.0) {
                oss << "+";
            }
            oss << static_cast<int>(kv.second) << "%";
            report.contributing_factors.push_back(oss.str());
        }
    }

    // Sort factors by |delta| descending
    std::sort(report.contributing_factors.begin(),
              report.contributing_factors.end(),
              [&deltas](const std::string& a, const std::string& b) {
                  // Extract metric name (everything before " changed by")
                  auto extractMetric = [](const std::string& s) {
                      auto pos = s.find(" changed by");
                      return pos != std::string::npos ? s.substr(0, pos) : s;
                  };
                  const double da = std::abs(deltas.at(extractMetric(a)));
                  const double db = std::abs(deltas.at(extractMetric(b)));
                  return da > db;
              });

    // Apply heuristic rules to determine primary cause
    impl_->applyRules(issue, deltas, report);

    if (report.contributing_factors.empty()) {
        report.contributing_factors.push_back("No metric exceeded the configured significance threshold");
    }

    return report;
}

std::vector<CorrelatedMetric> RootCauseAnalyzer::findCorrelations(
        const std::string& metric_name) {
    auto target_it = impl_->series_registry.find(metric_name);
    if (target_it == impl_->series_registry.end()) {
        return {};
    }

    std::vector<double> target_vals = extractValues(target_it->second);

    std::vector<CorrelatedMetric> results = {};

    for (const auto& kv : impl_->series_registry) {
        if (kv.first == metric_name) {
            continue;
        }
        // Copy target_vals for each comparison to avoid progressive truncation
        // by alignVectors modifying the target across iterations.
        std::vector<double> target_copy = target_vals;
        std::vector<double> other_vals = extractValues(kv.second);
        alignVectors(target_copy, other_vals);
        if (static_cast<int>(target_copy.size()) < 2 || static_cast<int>(other_vals.size()) < 2) {
            continue;
        }

        const double r = pearsonCorrelation(target_copy, other_vals);
        if (std::abs(r) >= impl_->config.correlation_threshold) {
            CorrelatedMetric cm;
            cm.metric_name           = kv.first;
            cm.correlation_coefficient = r;
            cm.lag_seconds           = 0.0; // simple instantaneous correlation
            cm.description           = (r > 0 ? "Positively" : "Negatively")
                                      + std::string(" correlated with ")
                                      + metric_name;
            results.push_back(std::move(cm));
        }
    }

    // Sort by |r| descending
    std::sort(results.begin(), results.end(),
              [](const CorrelatedMetric& a, const CorrelatedMetric& b) {
                  return std::abs(a.correlation_coefficient)
                       > std::abs(b.correlation_coefficient);
              });

    if (static_cast<int>(results.size()) > impl_->config.max_correlations) {
        results.resize(impl_->config.max_correlations);
    }

    return results;
}

CausalGraph RootCauseAnalyzer::buildCausalGraph(
        const std::vector<TimeSeries>& metrics) {
    CausalGraph graph;

    for (const auto& ts : metrics) {
        graph.nodes.push_back(ts.name);
    }

    // Simple Granger-inspired heuristic: for each pair (A, B) check whether
    // lagging A by one step produces a stronger correlation with B than the
    // instantaneous correlation.  If the lagged correlation is stronger and
    // exceeds the threshold, add edge A→B.
    for (size_t i = 0; i <static_cast<int>(metrics.size()); ++i) {
        for (size_t j = 0; j <static_cast<int>(metrics.size()); ++j) {
            if (i == j) {
                continue;
            }
            std::vector<double> a_vals = extractValues(metrics[i]);
            std::vector<double> b_vals = extractValues(metrics[j]);

            if (static_cast<int>(a_vals.size()) < 3 || static_cast<int>(b_vals.size()) < 3) {
                continue;
            }

            // Instantaneous correlation
            std::vector<double> a_sync = a_vals;
            std::vector<double> b_sync = b_vals;
            alignVectors(a_sync, b_sync);
            const double r_inst = pearsonCorrelation(a_sync, b_sync);

            // Lagged correlation: drop the last element of A and the first of B
            std::vector<double> a_lag(a_vals.begin(), a_vals.end() - 1);
            std::vector<double> b_lag(b_vals.begin() + 1, b_vals.end());
            alignVectors(a_lag, b_lag);
            const double r_lag = pearsonCorrelation(a_lag, b_lag);

            // Edge A→B when the lag improves correlation AND exceeds threshold
            if (std::abs(r_lag) > std::abs(r_inst)
                    && std::abs(r_lag) >= impl_->config.correlation_threshold) {
                CausalEdge edge;
                edge.from_metric  = metrics[i].name;
                edge.to_metric    = metrics[j].name;
                edge.strength     = std::abs(r_lag);
                edge.lag_seconds  = 1.0; // one time-step lag
                graph.edges.push_back(std::move(edge));
            }
        }
    }

    return graph;
}

} // namespace observability
} // namespace themis
