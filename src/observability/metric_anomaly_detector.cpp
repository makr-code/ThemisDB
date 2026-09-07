/**
 * @file metric_anomaly_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/metric_anomaly_detector.h"
#include "observability/metrics_collector.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace observability {

// ---------------------------------------------------------------------------
// MetricAnomaly::toJson
// ---------------------------------------------------------------------------

json MetricAnomaly::toJson() const {
    auto ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                     timestamp.time_since_epoch())
                     .count();
    return json{
        {"metric_name",     metric_name},
        {"score",           score},
        {"is_anomaly",      is_anomaly},
        {"observed_value",  observed_value},
        {"severity",        severity},
        {"timestamp_ms",    ts_ms},
        {"description",     description}
    };
}

// ---------------------------------------------------------------------------
// StreamState construction
// ---------------------------------------------------------------------------

MetricAnomalyDetector::StreamState::StreamState(const MonitoredMetric& cfg)
    : config(cfg)
    , sad([&] {
          themisdb::analytics::StreamingAnomalyDetector::Config sc;
          sc.method            = cfg.method;
          sc.threshold         = cfg.threshold;
          sc.window_size       = cfg.window_size;
          sc.auto_train        = true;
          sc.auto_train_after  = cfg.auto_train_after;
          sc.retrain_on_window = cfg.retrain_on_window;
          return sc;
      }()) {}

// ---------------------------------------------------------------------------
// monitor / unmonitor
// ---------------------------------------------------------------------------

void MetricAnomalyDetector::monitor(const MonitoredMetric& config) {
    std::lock_guard<std::mutex> lk(mutex_);
    streams_[config.name] = std::make_unique<StreamState>(config);
}

void MetricAnomalyDetector::unmonitor(const std::string& metric_name) {
    std::lock_guard<std::mutex> lk(mutex_);
    streams_.erase(metric_name);
}

// ---------------------------------------------------------------------------
// observe
// ---------------------------------------------------------------------------

std::optional<MetricAnomaly> MetricAnomalyDetector::observe(
    const std::string& metric_name,
    double value,
    std::chrono::system_clock::time_point timestamp)
{
    std::unique_lock<std::mutex> lk(mutex_);
    auto it = streams_.find(metric_name);
    if (it == streams_.end()) {
      return std::nullopt;
    }

    StreamState& state = *it->second;
    ++state.points_seen;

    // Build a DataPoint with a single numeric feature.
    themisdb::analytics::DataPoint dp;
    dp.id = metric_name;
    dp.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          timestamp.time_since_epoch())
                          .count();
    dp.set("value", value);

    auto raw = state.sad.process(dp);  // may return nullopt during warm-up

    if (!raw) {
      return std::nullopt;
    }

    // Enrich the result into a MetricAnomaly.
    MetricAnomaly ma;
    ma.metric_name    = metric_name;
    ma.score          = raw->score;
    ma.is_anomaly     = raw->is_anomaly;
    ma.observed_value = value;
    ma.severity       = scoreSeverity(raw->score);
    ma.timestamp      = timestamp;
    ma.description    = raw->description;
    if (ma.description.empty() && ma.is_anomaly) {
        std::ostringstream desc = {};
        desc << std::fixed << std::setprecision(4)
             << "Anomaly on '" << metric_name
             << "': observed=" << value
             << " score=" << raw->score
             << " [" << ma.severity << "]";
        ma.description = desc.str();
    }

    if (ma.is_anomaly) {
        state.anomalies.push_back(ma);
    }

    // Copy callback before releasing lock to avoid holding lock during call.
    AnomalyCallback cb = callback_;
    lk.unlock();

    if (cb && ma.is_anomaly) {
        cb(ma);
    }

    return ma;
}

// ---------------------------------------------------------------------------
// getAnomalies / getAllAnomalies
// ---------------------------------------------------------------------------

std::vector<MetricAnomaly>
MetricAnomalyDetector::getAnomalies(const std::string& metric_name) const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = streams_.find(metric_name);
    if (it == streams_.end()) {
        throw std::out_of_range("MetricAnomalyDetector: unknown metric '" +
                                metric_name + "'");
    }
    return it->second->anomalies;
}

std::vector<MetricAnomaly> MetricAnomalyDetector::getAllAnomalies() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<MetricAnomaly> result = {};

    for (const auto& [name, state] : streams_) {
        result.insert(result.end(),
                      state->anomalies.begin(),
                      state->anomalies.end());
    }
    return result;
}

// ---------------------------------------------------------------------------
// clearAnomalies
// ---------------------------------------------------------------------------

void MetricAnomalyDetector::clearAnomalies(const std::string& metric_name) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = streams_.find(metric_name);
    if (it == streams_.end()) {
        throw std::out_of_range("MetricAnomalyDetector: unknown metric '" +
                                metric_name + "'");
    }
    it->second->anomalies.clear();
    it->second->sad.clearAnomalies();
}

void MetricAnomalyDetector::clearAllAnomalies() {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& [name, state] : streams_) {
        state->anomalies.clear();
        state->sad.clearAnomalies();
    }
}

// ---------------------------------------------------------------------------
// setCallback
// ---------------------------------------------------------------------------

void MetricAnomalyDetector::setCallback(AnomalyCallback cb) {
    std::lock_guard<std::mutex> lk(mutex_);
    callback_ = std::move(cb);
}

// ---------------------------------------------------------------------------
// publishMetrics
// ---------------------------------------------------------------------------

void MetricAnomalyDetector::publishMetrics() const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto& mc = MetricsCollector::getInstance();

    for (const auto& [name, state] : streams_) {
        std::map<std::string, std::string> labels = {{"metric", name}};

        auto stats = state->sad.getWindowStats();

        // Latest score and anomaly flag from the raw history.
        auto raw_anomalies = state->sad.getAnomalies();
        double latest_score    = 0.0;
        double latest_detected = 0.0;
        if (!raw_anomalies.empty()) {
            const auto& last = raw_anomalies.back();
            latest_score    = last.score;
            latest_detected = last.is_anomaly ? 1.0 : 0.0;
        }

        mc.setGauge("themis_anomaly_score",       latest_score,                         labels);
        mc.setGauge("themis_anomaly_detected",    latest_detected,                      labels);
        mc.setGauge("themis_anomaly_total",
                    static_cast<double>(state->anomalies.size()),                        labels);
        mc.setGauge("themis_anomaly_window_size",
                    static_cast<double>(stats.window_size),                              labels);
        mc.setGauge("themis_anomaly_points_seen",
                    static_cast<double>(state->points_seen),                             labels);
    }
}

// ---------------------------------------------------------------------------
// generateReport / generateReportJson
// ---------------------------------------------------------------------------

std::string MetricAnomalyDetector::generateReport() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::ostringstream oss = {};
    oss << "=== ThemisDB Metric Anomaly Detection Report ===\n\n";
    oss << "Monitored metrics: " <<static_cast<int>(streams_.size()) << "\n\n";

    for (const auto& [name, state] : streams_) {
        auto stats = state->sad.getWindowStats();
        oss << "--- " << name << " ---\n";
        oss << "  Method:          "
            << themisdb::analytics::anomalyMethodName(state->config.method) << "\n";
        oss << "  Threshold:       " << state->config.threshold << "\n";
        oss << "  Points seen:     " << state->points_seen << "\n";
        oss << "  Window size:     " << stats.window_size << "\n";
        oss << "  Trained:         " << (stats.trained ? "yes" : "no (warming up)") << "\n";
        oss << "  Anomaly count:   " << state->anomalies.size() << "\n";
        if (!state->anomalies.empty()) {
            oss << "  Recent anomalies:\n";
            size_t start = state->anomalies.size() > 5
                           ? state->anomalies.size() - 5 : 0;
            for (size_t i = start; i < state->anomalies.size(); ++i) {
                const auto& a = state->anomalies[i];
                oss << std::fixed << std::setprecision(4)
                    << "    [" << a.severity << "] score=" << a.score
                    << " value=" << a.observed_value << "\n";
            }
        }
        oss << "\n";
    }
    return oss.str();
}

json MetricAnomalyDetector::generateReportJson() const {
    std::lock_guard<std::mutex> lk(mutex_);
    json metrics_arr = json::array();

    for (const auto& [name, state] : streams_) {
        auto stats = state->sad.getWindowStats();
        json anomalies_arr = json::array();
        for (const auto& a : state->anomalies) {
            anomalies_arr.push_back(a.toJson());
        }
        metrics_arr.push_back({
            {"metric_name",    name},
            {"method",         themisdb::analytics::anomalyMethodName(state->config.method)},
            {"threshold",      state->config.threshold},
            {"points_seen",    state->points_seen},
            {"window_size",    stats.window_size},
            {"trained",        stats.trained},
            {"anomaly_count",  state->anomalies.size()},
            {"anomalies",      anomalies_arr}
        });
    }

    auto ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
    return json{
        {"generated_at_ms", ts_ms},
        {"monitored_count",static_cast<int>(streams_.size())},
        {"metrics",         metrics_arr}
    };
}

// ---------------------------------------------------------------------------
// monitoredCount / monitoredNames
// ---------------------------------------------------------------------------

size_t MetricAnomalyDetector::monitoredCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return streams_.size();
}

std::vector<std::string> MetricAnomalyDetector::monitoredNames() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<std::string> names = {};

    names.reserve(streams_.size());
    for (const auto& [name, _] : streams_) {
        names.push_back(name);
    }
    return names;
}

// ---------------------------------------------------------------------------
// scoreSeverity
// ---------------------------------------------------------------------------

/*static*/
std::string MetricAnomalyDetector::scoreSeverity(double score) noexcept {
    if (score >= 0.9) {
      return "critical";
    }
    if (score >= 0.75) {
      return "high";
    }
    if (score >= 0.6) {
      return "medium";
    }
    return "low";
}

} // namespace observability
} // namespace themis
