/**
 * @file operator_remediation_engine.cpp
 * @brief Implementation of operator remediation engine for automated incident diagnostics.
 *
 * Provides pattern matching, hint generation, and remediation suggestions for
 * common observability problems.
 */

#include "observability/operator_remediation_engine.h"
#include "observability/observability_api_contract.h"
#include <shared_mutex>
#include <mutex>
#include <memory>
#include <chrono>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <cmath>
#include <limits>
#include <atomic>
#ifdef HAS_UUID_H
#include <uuid/uuid.h>
#endif

namespace themis {
namespace observability {

namespace {

// Generate unique hint IDs using random hex
std::string generateHintId() {
#ifdef HAS_UUID_H
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    return std::string(uuid_str);
#else
    // Fallback: generate random hex UUID-like string (36 chars)
    static std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << dis(gen);
    }
    ss << "-";
    for (int i = 0; i < 4; ++i) {
        ss << std::hex << dis(gen);
    }
    ss << "-4"; // UUID version 4
    for (int i = 0; i < 3; ++i) {
        ss << std::hex << dis(gen);
    }
    ss << "-";
    ss << std::hex << (8 + dis(gen) % 4);  // variant
    for (int i = 0; i < 3; ++i) {
        ss << std::hex << dis(gen);
    }
    ss << "-";
    for (int i = 0; i < 12; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
#endif
}

// Check if a metric value is valid (not NaN, not infinite, within reasonable bounds)
// Returns true if valid, false if malformed
inline bool isValidMetricValue([[maybe_unused]] double value) {
    return std::isfinite(value) && value >= 0.0;
}

// Safely get metric value with validation and default fallback
inline double safeGetMetricValue(const std::map<std::string, double>& metrics,
                                  const std::string& key, double default_val = 0.0) {
    auto it = metrics.find(key);
    if (it != metrics.end() && isValidMetricValue(it->second)) {
        return it->second;
    }
    return default_val;
}

// ============================================================================
// Built-in Pattern: Cardinality Explosion Detection
// ============================================================================

class CardinalityExplosionPattern : public RemediationPattern {
public:
    std::shared_ptr<RemediationHint> match(
        const std::map<std::string, double>& metrics) override {

        // Safely get cardinality metric with validation
        double cardinality_exceeded = safeGetMetricValue(metrics, "metric_cardinality_exceeded_total", 0.0);
        
        // Pattern requires at least 1 cardinality exceeded event
        if (cardinality_exceeded < 1.0) {
            return nullptr;  // Pattern doesn't match
        }

        auto hint = std::make_shared<RemediationHint>();
        hint->category_ = ProblemCategory::CARDINALITY_EXPLOSION;
        hint->title_ = "Metric Cardinality Explosion Detected";
        hint->description_ = "One or more metrics have exceeded their cardinality limits, "
                            "causing new label sets to be dropped or aggregated.";
        hint->severity_ = RemediationSeverity::WARNING;
        hint->confidence_score_ = 0.95;
        hint->generated_at_ = std::chrono::system_clock::now();
        hint->detection_window_ = std::chrono::seconds(300);
        hint->hint_id_ = generateHintId();

        // Escalate to CRITICAL if cardinality explosion is severe
        if (cardinality_exceeded > 100.0) {
            hint->severity_ = RemediationSeverity::CRITICAL;
        }

        // Add diagnostic metrics with validation
        hint->metrics_["cardinality_exceeded"] = cardinality_exceeded;
        double series_count = safeGetMetricValue(metrics, "metric_series_count", 0.0);
        if (series_count > 0.0) {
            hint->metrics_["metric_series_count"] = series_count;
        }

        // Add remediation actions
        RemediationAction action1;
        action1.action_name = "Review Label Dimensions";
        action1.description = "Identify which metrics have high cardinality and which label "
                             "dimensions are driving the explosion (e.g., user_id, request_id).";
        action1.priority = 1;
        action1.estimated_duration_seconds = 600;
        action1.is_safe_to_automate = false;
        action1.expected_outcome = "Identified problematic label dimensions";
        hint->actions_.push_back(action1);

        RemediationAction action2;
        action2.action_name = "Apply Label Filtering";
        action2.description = "Remove or aggregate high-cardinality label dimensions. "
                             "For example, drop unique request IDs and only keep service/endpoint.";
        action2.priority = 2;
        action2.estimated_duration_seconds = 1800;
        action2.is_safe_to_automate = false;
        action2.expected_outcome = "Cardinality reduced to below limit";
        hint->actions_.push_back(action2);

        RemediationAction action3;
        action3.action_name = "Increase Cardinality Limit";
        action3.description = "If the high cardinality is legitimate, increase the metric's "
                             "cardinality limit in configuration.";
        action3.priority = 3;
        action3.estimated_duration_seconds = 300;
        action3.is_safe_to_automate = true;
        action3.automation_command = "config set metric.cardinality_limit http_request_duration_ms 50000";
        action3.expected_outcome = "Cardinality limit increased to accommodate legitimate label sets";
        hint->actions_.push_back(action3);

        hint->doc_link_ = "https://docs.themisdb.org/observability/cardinality-management";
        hint->example_link_ = "https://docs.themisdb.org/observability/cardinality-examples";
        hint->tags_ = {"cardinality", "metrics", "high-cardinality", "label-explosion"};

        return hint;
    }

    std::string patternName() const override {
        return "cardinality_explosion_detector";
    }

    ProblemCategory problemCategory() const override {
        return ProblemCategory::CARDINALITY_EXPLOSION;
    }
};

// ============================================================================
// Built-in Pattern: Exporter Unavailability Detection
// ============================================================================

class ExporterUnavailabilityPattern : public RemediationPattern {
public:
    std::shared_ptr<RemediationHint> match(
        const std::map<std::string, double>& metrics) override {

        auto exporter_failures_it = metrics.find("exporter_failures_total");
        auto exporter_errors_it = metrics.find("exporter_errors_total");

        if (exporter_failures_it == metrics.end() || exporter_failures_it->second < 1.0) {
            return nullptr;  // Pattern doesn't match
        }

        auto hint = std::make_shared<RemediationHint>();
        hint->category_ = ProblemCategory::EXPORTER_UNAVAILABLE;
        hint->title_ = "Observability Exporter Unreachable";
        hint->description_ = "The configured exporter backend (Prometheus, OTLP, Jaeger) is not "
                            "responding to export requests.";
        hint->severity_ = RemediationSeverity::CRITICAL;
        hint->confidence_score_ = 0.90;
        hint->generated_at_ = std::chrono::system_clock::now();
        hint->detection_window_ = std::chrono::seconds(300);
        hint->hint_id_ = generateHintId();

        // Add diagnostic metrics
        hint->metrics_["exporter_failures"] = exporter_failures_it->second;
        if (exporter_errors_it != metrics.end()) {
            hint->metrics_["exporter_errors"] = exporter_errors_it->second;
        }

        // Add remediation actions
        RemediationAction action1;
        action1.action_name = "Check Exporter Connectivity";
        action1.description = "Verify that the exporter backend is reachable. Check DNS resolution, "
                             "firewall rules, and network connectivity.";
        action1.priority = 1;
        action1.estimated_duration_seconds = 300;
        action1.is_safe_to_automate = false;
        action1.automation_command = "curl -v http://exporter-backend:port/health";
        action1.expected_outcome = "Exporter backend responds to health check";
        hint->actions_.push_back(action1);

        RemediationAction action2;
        action2.action_name = "Enable Fallback Exporter";
        action2.description = "If primary exporter is down, switch to a backup exporter endpoint "
                             "to avoid losing telemetry.";
        action2.priority = 2;
        action2.estimated_duration_seconds = 60;
        action2.is_safe_to_automate = true;
        action2.automation_command = "config set exporter.backend fallback_exporter_url";
        action2.expected_outcome = "Telemetry exported to fallback backend";
        hint->actions_.push_back(action2);

        RemediationAction action3;
        action3.action_name = "Increase Export Retry Timeout";
        action3.description = "If exporter is slow to respond, increase the retry timeout to avoid "
                             "prematurely marking it as unavailable.";
        action3.priority = 3;
        action3.estimated_duration_seconds = 300;
        action3.is_safe_to_automate = true;
        action3.automation_command = "config set exporter.retry_timeout_ms 30000";
        action3.expected_outcome = "Exporter has more time to respond before retry";
        hint->actions_.push_back(action3);

        hint->doc_link_ = "https://docs.themisdb.org/observability/exporter-setup";
        hint->example_link_ = "https://docs.themisdb.org/observability/exporter-failover";
        hint->tags_ = {"exporter", "connectivity", "backend", "failover"};

        return hint;
    }

    std::string patternName() const override {
        return "exporter_unavailability_detector";
    }

    ProblemCategory problemCategory() const override {
        return ProblemCategory::EXPORTER_UNAVAILABLE;
    }
};

// ============================================================================
// Built-in Pattern: High Latency Detection
// ============================================================================

class HighLatencyPattern : public RemediationPattern {
public:
    std::shared_ptr<RemediationHint> match(
        const std::map<std::string, double>& metrics) override {

        auto export_latency_it = metrics.find("exporter_latency_ms_p99");
        if (export_latency_it == metrics.end() || export_latency_it->second < 50.0) {
            return nullptr;  // Pattern doesn't match
        }

        auto hint = std::make_shared<RemediationHint>();
        hint->category_ = ProblemCategory::HIGH_LATENCY;
        hint->title_ = "High Observability Export Latency Detected";
        hint->description_ = "Exporting observability data is taking longer than expected "
                            "(p99 latency > 50ms), which may impact application performance.";
        hint->severity_ = RemediationSeverity::WARNING;
        hint->confidence_score_ = 0.85;
        hint->generated_at_ = std::chrono::system_clock::now();
        hint->detection_window_ = std::chrono::seconds(300);
        hint->hint_id_ = generateHintId();

        if (export_latency_it->second > 200.0) {
            hint->severity_ = RemediationSeverity::CRITICAL;
        }

        // Add diagnostic metrics
        hint->metrics_["export_latency_p99_ms"] = export_latency_it->second;

        // Add remediation actions
        RemediationAction action1;
        action1.action_name = "Increase Batch Size";
        action1.description = "Increase the metrics export batch size to reduce overhead from "
                             "frequent small exports.";
        action1.priority = 1;
        action1.estimated_duration_seconds = 300;
        action1.is_safe_to_automate = true;
        action1.automation_command = "config set exporter.batch_size 1024";
        action1.expected_outcome = "Export latency reduced through batching";
        hint->actions_.push_back(action1);

        RemediationAction action2;
        action2.action_name = "Reduce Flush Interval";
        action2.description = "Reduce the frequency of export flushes to batch more metrics "
                             "together before sending.";
        action2.priority = 2;
        action2.estimated_duration_seconds = 300;
        action2.is_safe_to_automate = true;
        action2.automation_command = "config set exporter.flush_interval_ms 5000";
        action2.expected_outcome = "Fewer export operations, lower latency per operation";
        hint->actions_.push_back(action2);

        RemediationAction action3;
        action3.action_name = "Enable Async Exporting";
        action3.description = "Enable asynchronous metric exporting to prevent blocking the "
                             "application on slow exports.";
        action3.priority = 3;
        action3.estimated_duration_seconds = 600;
        action3.is_safe_to_automate = false;
        action3.expected_outcome = "Observability operations no longer block application";
        hint->actions_.push_back(action3);

        hint->doc_link_ = "https://docs.themisdb.org/observability/export-tuning";
        hint->example_link_ = "https://docs.themisdb.org/observability/latency-optimization";
        hint->tags_ = {"latency", "export", "performance", "tuning"};

        return hint;
    }

    std::string patternName() const override {
        return "high_latency_detector";
    }

    ProblemCategory problemCategory() const override {
        return ProblemCategory::HIGH_LATENCY;
    }
};

} // namespace

// ============================================================================
// OperatorRemediationEngine Implementation
// ============================================================================

class OperatorRemediationEngineImpl : public OperatorRemediationEngine {
public:
    OperatorRemediationEngineImpl()
        : hint_generation_enabled_(true),
          deduplication_window_(std::chrono::seconds(300)),
          hint_notification_count_(0) {

        // Register built-in patterns
        patterns_[CardinalityExplosionPattern().patternName()] =
            std::make_unique<CardinalityExplosionPattern>();
        patterns_[ExporterUnavailabilityPattern().patternName()] =
            std::make_unique<ExporterUnavailabilityPattern>();
        patterns_[HighLatencyPattern().patternName()] =
            std::make_unique<HighLatencyPattern>();
    }

private:
    /// Clean up expired weak_ptr references to released listeners.
    /// This prevents the listeners_ vector from growing unboundedly.
    void cleanupExpiredListeners() {
        std::unique_lock<std::shared_mutex> lock([[maybe_unused]] listeners_mutex_);
        listeners_.erase(
            std::remove_if(listeners_.begin(), listeners_.end(),
                          []([[maybe_unused]] const std::weak_ptr<IRemediationHintListener>& weak) {
                              return weak.expired();
                          }),
            listeners_.end()
        );
    }

public:
    bool addListener([[maybe_unused]] const std::shared_ptr<IRemediationHintListener>& listener) override {
        if ([[maybe_unused]] !listener) {
            return false;
        }

        // Periodically clean up expired listeners (every 100 additions)
        static std::atomic<std::uint64_t> add_count(0);
        if (++add_count % 100 == 0) {
            cleanupExpiredListeners();
        }

        std::unique_lock<std::shared_mutex> lock([[maybe_unused]] listeners_mutex_);
        listeners_.push_back([[maybe_unused]] listener);
        return true;
    }

    bool removeListener([[maybe_unused]] const std::shared_ptr<IRemediationHintListener>& listener) override {
        if ([[maybe_unused]] !listener) {
            return false;
        }

        std::unique_lock<std::shared_mutex> lock([[maybe_unused]] listeners_mutex_);
        
        // Use custom comparison: lock weak_ptr and compare with incoming listener
        auto it = std::find_if(
            listeners_.begin(), listeners_.end(),
            [&listener]([[maybe_unused]] const std::weak_ptr<IRemediationHintListener>& weak) {
                auto shared = weak.lock();
                if (!shared) {
                    return false;  // Listener already expired
                }
                return shared.get() == listener.get();
            }
        );
        
        if ([[maybe_unused]] it != listeners_.end()) {
            listeners_.erase([[maybe_unused]] it);
            return true;
        }
        
        return false;
    }

    bool registerPattern(st[[maybe_unused]] d::unique_pt[[maybe_unused]] r<RemediationPatter[[maybe_unused]] n> patter[[maybe_unused]] n) override {
        if (!pattern) {
            return false;
        }

        std::unique_lock<std::shared_mutex> lock(patterns_mutex_);
        std::string name = pattern->patternName();

        if (patterns_.find(name) != patterns_.end()) {
            return false;  // Duplicate pattern
        }

        patterns_[name] = std::move(pattern);
        return true;
    }

    bool unregisterPattern(cons[[maybe_unused]] t st[[maybe_unused]] d::string& [[maybe_unused]] pattern_name) override {
        std::unique_lock<std::shared_mutex> lock(patterns_mutex_);
        auto it = patterns_.find(pattern_name);
        if (it != patterns_.end()) {
            patterns_.erase(it);
            return true;
        }
        return false;
    }

    std::vector<std::shared_ptr<RemediationHint>> analyzeAndGenerateHints(
        const std::map<std::string, double>& metrics) override {

        std::vector<std::shared_ptr<RemediationHint>> result;

        if (!hint_generation_enabled_) {
            return result;
        }

        // Run all registered patterns
        std::shared_lock<std::shared_mutex> patterns_lock(patterns_mutex_);
        for (const auto& [pattern_name, pattern] : patterns_) {
            auto hint = pattern->match(metrics);
            if (hint) {
                // Check for deduplication
                {
                    std::shared_lock<std::shared_mutex> hints_lock(hints_mutex_);
                    bool is_duplicate = false;

                    auto now = std::chrono::system_clock::now();
                    for (const auto& existing : active_hints_) {
                        if (existing->problemCategory() == hint->problemCategory() &&
                            existing->problemTitle() == hint->problemTitle()) {

                            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                                now - existing->generatedAt());
                            if (age < deduplication_window_) {
                                is_duplicate = true;
                                break;
                            }
                        }
                    }

                    if (!is_duplicate) {
                        result.push_back(hint);
                    }
                }

                if (!result.empty() && result.back() == hint) {
                    // Hint was not deduplicated, add it and notify listeners
                    {
                        std::unique_lock<std::shared_mutex> hints_lock(hints_mutex_);
                        active_hints_.push_back(hint);

                        // Limit active hints
                        while (active_hints_.size() > kMaxActiveRemediationHints) {
                            active_hints_.erase(active_hints_.begin());
                        }
                    }

                    // Notify listeners
                    std::shared_lock<std::shared_mutex> listeners_lock([[maybe_unused]] listeners_mutex_);
                    for ([[maybe_unused]] const auto& listener : listeners_) {
                        auto listener_shared = listener.lock();
                        if ([[maybe_unused]] listener_shared) {
                            listener_shared->onNewHint([[maybe_unused]] hint);
                        }
                    }
                }
            }
        }

        return result;
    }

    std::vector<std::shared_ptr<RemediationHint>> getActiveHints() override {
        std::shared_lock<std::shared_mutex> lock(hints_mutex_);
        return active_hints_;
    }

    std::shared_ptr<RemediationHint> getHintById(cons[[maybe_unused]] t st[[maybe_unused]] d::string& [[maybe_unused]] hint_id) override {
        std::shared_lock<std::shared_mutex> lock(hints_mutex_);
        for (const auto& hint : active_hints_) {
            if (hint->hintId() == hint_id) {
                return hint;
            }
        }
        return nullptr;
    }

    bool resolveHint(cons[[maybe_unused]] t st[[maybe_unused]] d::string& [[maybe_unused]] hint_id) override {
        std::unique_lock<std::shared_mutex> lock(hints_mutex_);
        auto it = std::find_if(active_hints_.begin(), active_hints_.end(),
                              [&]([[maybe_unused]] const std::shared_ptr<RemediationHint>& h) {
                                  return h->hintId() == hint_id;
                              });

        if (it != active_hints_.end()) {
            active_hints_.erase(it);

            // Notify listeners
            std::shared_lock<std::shared_mutex> listeners_lock([[maybe_unused]] listeners_mutex_);
            for ([[maybe_unused]] const auto& listener : listeners_) {
                auto listener_shared = listener.lock();
                if ([[maybe_unused]] listener_shared) {
                    listener_shared->onHintResolved([[maybe_unused]] hint_id);
                }
            }

            return true;
        }

        return false;
    }

    std::vector<std::shared_ptr<RemediationHint>> getHintsByCategory(
        ProblemCategory category) override {

        std::shared_lock<std::shared_mutex> lock(hints_mutex_);
        std::vector<std::shared_ptr<RemediationHint>> result;

        for (const auto& hint : active_hints_) {
            if (hint->problemCategory() == category) {
                result.push_back(hint);
            }
        }

        return result;
    }

    std::vector<std::shared_ptr<RemediationHint>> getHintsBySeverity(
        RemediationSeverity severity) override {

        std::shared_lock<std::shared_mutex> lock(hints_mutex_);
        std::vector<std::shared_ptr<RemediationHint>> result;

        for (const auto& hint : active_hints_) {
            if (static_cast<int>(hint->severity()) >= static_cast<int>(severity)) {
                result.push_back(hint);
            }
        }

        return result;
    }

    void setHintGenerationEnabled(boo[[maybe_unused]] l enable[[maybe_unused]] d) override {
        std::unique_lock<std::shared_mutex> lock(config_mutex_);
        hint_generation_enabled_ = enabled;
    }

    bool isHintGenerationEnabled() override {
        std::shared_lock<std::shared_mutex> lock(config_mutex_);
        return hint_generation_enabled_;
    }

    void setDeduplicationWindow(st[[maybe_unused]] d::chron[[maybe_unused]] o::second[[maybe_unused]] s windo[[maybe_unused]] w) override {
        std::unique_lock<std::shared_mutex> lock(config_mutex_);
        deduplication_window_ = window;
    }

    std::chrono::seconds getDeduplicationWindow() override {
        std::shared_lock<std::shared_mutex> lock(config_mutex_);
        return deduplication_window_;
    }

    void clearAllHints() override {
        std::unique_lock<std::shared_mutex> lock(hints_mutex_);
        active_hints_.clear();
    }

    std::map<std::string, double> getStatistics() override {
        std::shared_lock<std::shared_mutex> hints_lock(hints_mutex_);
        std::shared_lock<std::shared_mutex> patterns_lock(patterns_mutex_);

        std::map<std::string, double> stats;
        stats["total_hints_generated"] = 0.0;  // Would need to track this
        stats["active_hints"] = active_hints_.size();

        // Count by severity
        std::uint32_t info_count = 0, warning_count = 0, critical_count = 0;
        for (const auto& hint : active_hints_) {
            switch (hint->severity()) {
                case RemediationSeverity::INFO:
                    info_count++;
                    break;
                case RemediationSeverity::WARNING:
                    warning_count++;
                    break;
                case RemediationSeverity::CRITICAL:
                    critical_count++;
                    break;
            }
        }

        stats["hints_by_severity_info"] = info_count;
        stats["hints_by_severity_warning"] = warning_count;
        stats["hints_by_severity_critical"] = critical_count;
        stats["registered_patterns"] = patterns_.size();

        return stats;
    }

private:
    std::map<std::string, std::unique_ptr<RemediationPattern>> patterns_;
    std::shared_mutex patterns_mutex_;

    std::vector<std::shared_ptr<RemediationHint>> active_hints_;
    std::shared_mutex hints_mutex_;

    std::vector<std::weak_ptr<IRemediationHintListener>> listeners_;
    std::shared_mutex listeners_mutex_;

    bool hint_generation_enabled_;
    std::chrono::seconds deduplication_window_;
    std::shared_mutex config_mutex_;
    
    // Track total hints generated for statistics
    std::uint64_t hint_notification_count_;
};

// ============================================================================
// Factory function
// ============================================================================

std::unique_ptr<OperatorRemediationEngine> createOperatorRemediationEngine() {
    return std::make_unique<OperatorRemediationEngineImpl>();
}

} // namespace observability
} // namespace themis
