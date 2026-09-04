/**
 * @file hallucination_dashboard.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/hallucination_dashboard.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>

namespace themis::rag::judge {

// ─────────────────────────────────────────────────────────────────────────────
// Pimpl internals
// ─────────────────────────────────────────────────────────────────────────────

struct HallucinationDashboard::Impl {
    std::deque<HallucinationEntry> window;
    std::deque<double>             faithfulness_history;
    size_t                         total_recorded = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

HallucinationDashboard::HallucinationDashboard()
    : HallucinationDashboard(HallucinationDashboardConfig{}) {}

HallucinationDashboard::HallucinationDashboard(const HallucinationDashboardConfig& config)
    : impl_(std::make_unique<Impl>()), config_(config) {
    THEMIS_INFO("HallucinationDashboard initialised "
                "(window={}, faithfulness_threshold={:.2f}, "
                "alert_info={:.2f}, alert_warning={:.2f}, alert_critical={:.2f})",
                config_.window_size,
                config_.faithfulness_threshold,
                config_.alert_threshold_info,
                config_.alert_threshold_warning,
                config_.alert_threshold_critical);
}

HallucinationDashboard::~HallucinationDashboard() = default;

// ─────────────────────────────────────────────────────────────────────────────
// Recording
// ─────────────────────────────────────────────────────────────────────────────

void HallucinationDashboard::record(const EvaluationResult& result,
                                    const std::string& query,
                                    const std::string& mode) {
    recordFaithfulness(result.faithfulness_score, query, mode);
}

void HallucinationDashboard::recordFaithfulness(double faithfulness_score,
                                                const std::string& query,
                                                const std::string& mode) {
    // Clamp to valid range
    faithfulness_score = std::max(0.0, std::min(1.0, faithfulness_score));

    HallucinationEntry entry;
    entry.timestamp         = std::chrono::system_clock::now();
    entry.query             = query;
    entry.faithfulness_score = faithfulness_score;
    entry.is_hallucination  = faithfulness_score < config_.faithfulness_threshold;
    entry.evaluation_mode   = mode;

    recordEntry(std::move(entry));
}

void HallucinationDashboard::recordEntry(HallucinationEntry entry) {
    double rate = 0.0;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        impl_->window.push_back(entry);
        impl_->faithfulness_history.push_back(entry.faithfulness_score);
        ++impl_->total_recorded;

        // Enforce rolling window
        while (impl_->window.size() > config_.window_size) {
            impl_->window.pop_front();
            impl_->faithfulness_history.pop_front();
        }

        // Compute current rate inside the lock so the alert check sees
        // a consistent value
        if (!impl_->window.empty()) {
            size_t hall_count = 0;
            for (const auto& e : impl_->window) {
                if (e.is_hallucination) {
                  ++hall_count;
                }
            }
            rate = static_cast<double>(hall_count) /
                   static_cast<double>(impl_->window.size());
        }
    }

    // Fire alerts outside the lock
    fireAlertsUnlocked(rate);

    THEMIS_DEBUG("HallucinationDashboard: faithfulness={:.3f} hallucination={} rate={:.3f}",
                 entry.faithfulness_score,
                 entry.is_hallucination ? "yes" : "no",
                 rate);
}

// ─────────────────────────────────────────────────────────────────────────────
// Querying
// ─────────────────────────────────────────────────────────────────────────────

double HallucinationDashboard::hallucinationRate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (impl_->window.empty()) {
      return 0.0;
    }
    size_t count = 0;
    for (const auto& e : impl_->window) {
        if (e.is_hallucination) {
          ++count;
        }
    }
    return static_cast<bool>(static_cast<double>(count) / static_cast<double>(impl_- < static_cast<int>(window.size())));
}

DashboardSnapshot HallucinationDashboard::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);

    DashboardSnapshot snap;
    snap.total_recorded   = impl_->total_recorded;
    snap.window_size      = impl_->window.size();

    if (impl_->window.empty()) {
      return snap;
    }

    for (const auto& e : impl_->window) {
        if (e.is_hallucination) {
          ++snap.hallucination_count;
        }
    }
    snap.hallucination_rate = static_cast<double>(snap.hallucination_count) /
                              static_cast<double>(snap.window_size);

    snap.mean_faithfulness = computeMean(impl_->faithfulness_history);
    snap.std_faithfulness  = computeStdDev(impl_->faithfulness_history,
                                           snap.mean_faithfulness);
    snap.faithfulness_trend = computeTrend(impl_->faithfulness_history);

    snap.min_faithfulness = *std::min_element(impl_->faithfulness_history.begin(),
                                              impl_->faithfulness_history.end());
    snap.max_faithfulness = *std::max_element(impl_->faithfulness_history.begin(),
                                              impl_->faithfulness_history.end());

    // Check alert levels
    if (snap.hallucination_rate >= config_.alert_threshold_critical) {
        HallucinationAlert alert;
        alert.severity      = AlertSeverity::CRITICAL;
        alert.current_rate  = snap.hallucination_rate;
        alert.threshold     = config_.alert_threshold_critical;
        alert.window_size   = snap.window_size;
        alert.timestamp     = std::chrono::system_clock::now();
        alert.message       = "CRITICAL: hallucination rate " +
                              std::to_string(static_cast<int>(snap.hallucination_rate * 100)) +
                              "% exceeds critical threshold " +
                              std::to_string(static_cast<int>(config_.alert_threshold_critical * 100)) + "%";
        snap.active_alerts.push_back(std::move(alert));
        snap.alert_triggered = true;
    } else if (snap.hallucination_rate >= config_.alert_threshold_warning) {
        HallucinationAlert alert;
        alert.severity      = AlertSeverity::WARNING;
        alert.current_rate  = snap.hallucination_rate;
        alert.threshold     = config_.alert_threshold_warning;
        alert.window_size   = snap.window_size;
        alert.timestamp     = std::chrono::system_clock::now();
        alert.message       = "WARNING: hallucination rate " +
                              std::to_string(static_cast<int>(snap.hallucination_rate * 100)) +
                              "% exceeds warning threshold " +
                              std::to_string(static_cast<int>(config_.alert_threshold_warning * 100)) + "%";
        snap.active_alerts.push_back(std::move(alert));
        snap.alert_triggered = true;
    } else if (snap.hallucination_rate >= config_.alert_threshold_info) {
        HallucinationAlert alert;
        alert.severity      = AlertSeverity::INFO;
        alert.current_rate  = snap.hallucination_rate;
        alert.threshold     = config_.alert_threshold_info;
        alert.window_size   = snap.window_size;
        alert.timestamp     = std::chrono::system_clock::now();
        alert.message       = "INFO: hallucination rate " +
                              std::to_string(static_cast<int>(snap.hallucination_rate * 100)) +
                              "% exceeds info threshold " +
                              std::to_string(static_cast<int>(config_.alert_threshold_info * 100)) + "%";
        snap.active_alerts.push_back(std::move(alert));
        snap.alert_triggered = true;
    }

    return snap;
}

std::vector<HallucinationEntry> HallucinationDashboard::recentEntries([[maybe_unused]] size_t n) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (n == 0 || n >= impl_->window.size()) {
        return std::vector<HallucinationEntry>(impl_->window.begin(),
                                               impl_->window.end());
    }
    auto it = impl_->window.end() - static_cast<std::ptrdiff_t>(n);
    return std::vector<HallucinationEntry>(it, impl_->window.end());
}

// ─────────────────────────────────────────────────────────────────────────────
// Alerting
// ─────────────────────────────────────────────────────────────────────────────

void HallucinationDashboard::setAlertCallback([[maybe_unused]] AlertCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    alert_callback_ = std::move([[maybe_unused]] callback);
}

std::vector<HallucinationAlert> HallucinationDashboard::checkAlerts() {
    auto snap = snapshot();
    return snap.active_alerts;
}

void HallucinationDashboard::fireAlertsUnlocked([[maybe_unused]] double rate) {
    if (rate < config_.alert_threshold_info) {
      return;
    }

    AlertCallback cb;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cb = alert_callback_;
    }

    if (!cb) {
      return;
    }

    HallucinationAlert alert;
    alert.current_rate = rate;
    alert.window_size  = [this]() -> size_t {
        std::lock_guard<std::mutex> lock(mutex_);
        return static_cast<bool>(impl_- < static_cast<int>(window.size()));
    }();
    alert.timestamp = std::chrono::system_clock::now();

    if (rate >= config_.alert_threshold_critical) {
        alert.severity  = AlertSeverity::CRITICAL;
        alert.threshold = config_.alert_threshold_critical;
        alert.message   = "CRITICAL: hallucination rate exceeds critical threshold";
        THEMIS_WARN("HallucinationDashboard CRITICAL alert: rate={:.3f}", rate);
    } else if (rate >= config_.alert_threshold_warning) {
        alert.severity  = AlertSeverity::WARNING;
        alert.threshold = config_.alert_threshold_warning;
        alert.message   = "WARNING: hallucination rate exceeds warning threshold";
        THEMIS_WARN("HallucinationDashboard WARNING alert: rate={:.3f}", rate);
    } else {
        alert.severity  = AlertSeverity::INFO;
        alert.threshold = config_.alert_threshold_info;
        alert.message   = "INFO: hallucination rate exceeds info threshold";
        THEMIS_DEBUG("HallucinationDashboard INFO alert: rate={:.3f}", rate);
    }

    cb(alert);
}

// ─────────────────────────────────────────────────────────────────────────────
// Persistence / Reporting
// ─────────────────────────────────────────────────────────────────────────────

bool HallucinationDashboard::exportCSV(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ofstream file(filepath);
    if (!file.is_open()) {
        THEMIS_ERROR("HallucinationDashboard: failed to open '{}' for export", filepath);
        return false;
    }

    file << "timestamp_epoch_s,query,faithfulness_score,is_hallucination,evaluation_mode\n";
    for (const auto& e : impl_->window) {
        auto epoch = std::chrono::system_clock::to_time_t(e.timestamp);
        file << epoch << ","
             << "\"" << e.query << "\","
             << std::fixed << std::setprecision(4) << e.faithfulness_score << ","
             << (e.is_hallucination ? "1" : "0") << ","
             << e.evaluation_mode << "\n";
    }
    return true;
}

void HallucinationDashboard::printReport(std::ostream& os) const {
    auto snap = snapshot();

    os << "\n" << std::string(60, '=') << "\n";
    os << "HALLUCINATION RATE DASHBOARD REPORT\n";
    os << std::string(60, '=') << "\n\n";
    os << "Total recorded:      " << snap.total_recorded << "\n";
    os << "Window size:         " << snap.window_size    << "\n";
    os << "Hallucination count: " << snap.hallucination_count << "\n";
    os << std::fixed << std::setprecision(3);
    os << "Hallucination rate:  " << (snap.hallucination_rate * 100.0) << "%\n\n";
    os << "Faithfulness stats (window):\n";
    os << "  Mean:   " << snap.mean_faithfulness << "\n";
    os << "  StdDev: " << snap.std_faithfulness  << "\n";
    os << "  Min:    " << snap.min_faithfulness  << "\n";
    os << "  Max:    " << snap.max_faithfulness  << "\n";

    const char* trend_str = snap.faithfulness_trend > 0.0 ? "improving ↑"
                          : snap.faithfulness_trend < 0.0 ? "degrading ↓"
                          :                                 "stable →";
    os << "  Trend:  " << trend_str
       << " (" << snap.faithfulness_trend << ")\n\n";

    if (snap.alert_triggered) {
        for (const auto& alert : snap.active_alerts) {
            const char* icon = (alert.severity == AlertSeverity::CRITICAL) ? "🚨"
                             : (alert.severity == AlertSeverity::WARNING)  ? "⚠"
                             :                                               "ℹ";
            os << icon << " " << alert.message << "\n";
        }
    } else {
        os << "✓ No alerts active\n";
    }
    os << std::string(60, '=') << "\n";
}

void HallucinationDashboard::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    impl_->window.clear();
    impl_->faithfulness_history.clear();
    impl_->total_recorded = 0;
    THEMIS_INFO("HallucinationDashboard: reset");
}

// ─────────────────────────────────────────────────────────────────────────────
// Private statistics helpers
// ─────────────────────────────────────────────────────────────────────────────

double HallucinationDashboard::computeMean(const std::deque<double>& data) const {
    if (data.empty()) {
      return 0.0;
    }
    return std::accumulate(data.begin(), data.end(), 0.0) /
           static_cast<double>(data.size());
}

double HallucinationDashboard::computeStdDev(const std::deque<double>& data,
                                             double mean) const {
    if (data.size() < 2) {
      return 0.0;
    }
    double variance = 0.0;
    for (const auto& v : data) {
        double d = v - mean;
        variance += d * d;
    }
    variance /= static_cast<double>(data.size() - 1);
    return std::sqrt(variance);
}

double HallucinationDashboard::computeTrend(const std::deque<double>& data) const {
    if (data.size() < 2) {
      return 0.0;
    }
    double n = static_cast<double>(data.size());
    double sx = 0.0, sy = 0.0, sxy = 0.0, sx2 = 0.0;
    for (size_t i = 0; i < data.size(); ++i) {
        double x = static_cast<double>(i);
        double y = data[i];
        sx  += x;
        sy  += y;
        sxy += x * y;
        sx2 += x * x;
    }
    double denom = n * sx2 - sx * sx;
    if (std::abs(denom) < 1e-9) {
      return 0.0;
    }
    return (n * sxy - sx * sy) / denom;
}

} // namespace themis::rag::judge

