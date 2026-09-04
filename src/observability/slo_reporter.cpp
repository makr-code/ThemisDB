/**
 * @file slo_reporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/slo_reporter.h"
#include "observability/metrics_collector.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace observability {

// ---------------------------------------------------------------------------
// SloStatus::toJson
// ---------------------------------------------------------------------------

json SloStatus::toJson() const {
    json alerts_arr = json::array();
    for (const auto& alert : active_burn_rate_alerts) {
        alerts_arr.push_back({
            {"level",        alert.severity},
            {"burn_rate",    alert.burn_rate},
            {"window_hours", alert.window_hours},
            {"message",      alert.message}
        });
    }

    return json{
        {"name",                    name},
        {"objective",               objective},
        {"current_sli",             current_sli},
        {"error_budget_total",      error_budget_total},
        {"error_budget_remaining",  error_budget_remaining},
        {"total_requests",          total_requests},
        {"error_requests",          error_requests},
        {"burn_rate",               burn_rate},
        {"slo_met",                 slo_met},
        {"active_burn_rate_alerts", alerts_arr}
    };
}

// ---------------------------------------------------------------------------
// SloReporter – construction/destruction
// ---------------------------------------------------------------------------

SloReporter::SloReporter() : SloReporter(Config{}) {}
SloReporter::SloReporter(const Config& config) : config_(config) {}
SloReporter::~SloReporter() = default;

// ---------------------------------------------------------------------------
// registerSlo / record
// ---------------------------------------------------------------------------

void SloReporter::registerSlo(const SloDefinition& slo) {
    std::lock_guard<std::mutex> lk(mutex_);
    SloState& state = slos_[slo.name];
    state.def = slo;
    // Preserve existing samples when replacing a definition.
}

void SloReporter::record(const std::string& slo_name, bool good_request,
                          std::chrono::system_clock::time_point timestamp) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = slos_.find(slo_name);
    if (it == slos_.end()) return;  // unknown SLO – silently ignored

    SloState& state = it->second;
    state.samples.push_back({timestamp, good_request});

    // Evict oldest samples if we exceed max_samples_per_slo.
    while (state.samples.size() > config_.max_samples_per_slo) {
        state.samples.pop_front();
    }
}

// ---------------------------------------------------------------------------
// getStatus / getAllStatuses
// ---------------------------------------------------------------------------

SloStatus SloReporter::getStatus(const std::string& slo_name) const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = slos_.find(slo_name);
    if (it == slos_.end()) {
        throw std::out_of_range("SloReporter: unknown SLO name '" + slo_name + "'");
    }
    SloState state_copy = it->second;  // copy to expire outside the loop
    auto now = std::chrono::system_clock::now();
    expireSamples(state_copy, now);
    return computeStatus(state_copy);
}

std::vector<SloStatus> SloReporter::getAllStatuses() const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto now = std::chrono::system_clock::now();
    std::vector<SloStatus> result = {};

    result.reserve(slos_.size());
    for (const auto& [name, state] : slos_) {
        SloState state_copy = state;
        expireSamples(state_copy, now);
        result.push_back(computeStatus(state_copy));
    }
    return result;
}

// ---------------------------------------------------------------------------
// publishMetrics
// ---------------------------------------------------------------------------

void SloReporter::publishMetrics() const {
    auto statuses = getAllStatuses();
    auto& mc = MetricsCollector::getInstance();
    for (const auto& s : statuses) {
        std::map<std::string, std::string> labels = {{"slo", s.name}};
        mc.setGauge("themis_slo_current_sli",            s.current_sli,             labels);
        mc.setGauge("themis_slo_error_budget_remaining",  s.error_budget_remaining,   labels);
        mc.setGauge("themis_slo_burn_rate",               s.burn_rate,                labels);
        mc.setGauge("themis_slo_met",                     s.slo_met ? 1.0 : 0.0,     labels);
        mc.setGauge("themis_slo_total_requests",
                    static_cast<double>(s.total_requests), labels);
        mc.setGauge("themis_slo_error_requests",
                    static_cast<double>(s.error_requests),  labels);

        // Publish per-alert-level burn-rate gauges
        mc.setGauge("themis_slo_burn_rate_fast",
                    computeBurnRateLevelPublish(s, BurnRateLevel::FAST),   labels);
        mc.setGauge("themis_slo_burn_rate_medium",
                    computeBurnRateLevelPublish(s, BurnRateLevel::MEDIUM), labels);
        mc.setGauge("themis_slo_burn_rate_slow",
                    computeBurnRateLevelPublish(s, BurnRateLevel::SLOW),   labels);
    }
}

// ---------------------------------------------------------------------------
// generateReport / generateReportJson
// ---------------------------------------------------------------------------

std::string SloReporter::generateReport() const {
    auto statuses = getAllStatuses();
    std::ostringstream oss = {};
    oss << "=== ThemisDB SLO Compliance Report ===\n\n";
    oss << "SLOs evaluated: " << statuses.size() << "\n\n";

    for (const auto& s : statuses) {
        oss << "--- " << s.name << " ---\n";
        oss << std::fixed << std::setprecision(4);
        oss << "  SLO Target:           " << (s.objective * 100.0) << " %\n";
        oss << "  Current SLI:          " << (s.current_sli * 100.0) << " %\n";
        oss << "  Status:               " << (s.slo_met ? "MET ✅" : "VIOLATED ❌") << "\n";
        oss << "  Error Budget Total:   " << (s.error_budget_total * 100.0) << " %\n";
        oss << "  Error Budget Left:    "
            << (s.error_budget_remaining * 100.0) << " %\n";
        oss << "  Burn Rate:            " << std::setprecision(2)
            << s.burn_rate << "×\n";
        oss << "  Requests (window):    " << s.total_requests
            << " total, " << s.error_requests << " errors\n";

        if (!s.active_burn_rate_alerts.empty()) {
            oss << "  ⚠ Burn-Rate Alerts:\n";
            for (const auto& alert : s.active_burn_rate_alerts) {
                oss << "    [" << alert.severity << "] " << alert.message << "\n";
            }
        }
        oss << "\n";
    }
    return oss.str();
}

json SloReporter::generateReportJson() const {
    auto statuses = getAllStatuses();
    json arr = json::array();
    for (const auto& s : statuses) {
        arr.push_back(s.toJson());
    }

    auto ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
    return json{
        {"generated_at_ms", ts_ms},
        {"slo_count",       statuses.size()},
        {"slos",            arr}
    };
}

// ---------------------------------------------------------------------------
// clear / sloCount
// ---------------------------------------------------------------------------

void SloReporter::clear() {
    std::lock_guard<std::mutex> lk(mutex_);
    slos_.clear();
}

size_t SloReporter::sloCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return slos_.size();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

/*static*/
void SloReporter::expireSamples(SloState& state,
                                 std::chrono::system_clock::time_point now) {
    const auto cutoff = now - state.def.window;
    while (!state.samples.empty() && state.samples.front().ts < cutoff) {
        state.samples.pop_front();
    }
}

/*static*/
SloStatus SloReporter::computeStatus(const SloState& state) {
    SloStatus s;
    s.name      = state.def.name;
    s.objective = state.def.objective;
    s.error_budget_total = 1.0 - s.objective;

    const auto& samples = state.samples;
    s.total_requests = static_cast<uint64_t>(samples.size());
    uint64_t good = 0;
    for (const auto& sample : samples) {
        if (sample.good) {
          ++good;
        }
    }
    s.error_requests = s.total_requests - good;

    if (s.total_requests > 0) {
        s.current_sli = static_cast<double>(good) /
                        static_cast<double>(s.total_requests);
    } else {
        // No data: assume full compliance.
        s.current_sli = 1.0;
    }

    s.slo_met = (s.current_sli >= s.objective);

    // Error budget remaining: what fraction of the allowed error rate is unused.
    if (s.error_budget_total > 0.0) {
        double actual_error_rate = 1.0 - s.current_sli;
        double budget_consumed_fraction = actual_error_rate / s.error_budget_total;
        s.error_budget_remaining = std::max(0.0, 1.0 - budget_consumed_fraction);
    } else {
        s.error_budget_remaining = 1.0;
    }

    // Compute overall burn rate (error rate / allowed error rate).
    const double allowed_error_rate = s.error_budget_total;
    if (allowed_error_rate > 0.0 && s.total_requests > 0) {
        double actual_error_rate = 1.0 - s.current_sli;
        s.burn_rate = actual_error_rate / allowed_error_rate;
    } else {
        s.burn_rate = 0.0;
    }

    // Multi-window burn-rate alerts.
    // Each level checks a short evaluation window against a threshold multiplier.
    auto now = std::chrono::system_clock::now();

    struct LevelSpec {
        BurnRateLevel level;
        std::chrono::seconds window;
        double hours = {};
    };

    static const LevelSpec kLevels[] = {
        {BurnRateLevel::FAST,   std::chrono::hours(1),  1.0},
        {BurnRateLevel::MEDIUM, std::chrono::hours(6),  6.0},
        {BurnRateLevel::SLOW,   std::chrono::hours(24), 24.0},
    };

    for (const auto& spec : kLevels) {
        double threshold = burnRateMultiplier(spec.level);
        double rate = computeBurnRate(samples, spec.window, now, allowed_error_rate);
        if (rate > threshold) {
            BurnRateAlert alert;
            alert.level        = spec.level;
            alert.burn_rate    = rate;
            alert.window_hours = spec.hours;
            alert.severity     = burnRateSeverity(spec.level);
            std::ostringstream msg = {};
            msg << std::fixed << std::setprecision(1)
                << "SLO '" << s.name << "' burn rate " << rate
                << "× (threshold " << threshold << "×) over "
                << spec.hours << "h window — " << alert.severity;
            alert.message = msg.str();
            s.active_burn_rate_alerts.push_back(std::move(alert));
        }
    }

    return s;
}

/*static*/
double SloReporter::computeBurnRate(const std::deque<Sample>& samples,
                                     std::chrono::seconds window,
                                     std::chrono::system_clock::time_point now,
                                     double allowed_error_rate) noexcept {
    if (allowed_error_rate <= 0.0) {
      return 0.0;
    }
    const auto cutoff = now - window;
    uint64_t total = 0, good = 0;
    for (const auto& s : samples) {
        if (s.ts < cutoff) {
          continue;
        }
        ++total;
        if (s.good) {
          ++good;
        }
    }
    if (total == 0) {
      return 0.0;
    }
    double actual_error_rate = 1.0 -
        (static_cast<double>(good) / static_cast<double>(total));
    return actual_error_rate / allowed_error_rate;
}

// Helper used by publishMetrics to get the burn-rate value for a specific level.
// This is a free-function analogue of computeBurnRate that operates on a SloStatus.
double SloReporter::computeBurnRateLevelPublish(const SloStatus& s,
                                                 BurnRateLevel level) const {
    // We need raw samples – take the lock again through getStatus which copies
    // the state, but here we receive an already-computed status.  For the
    // per-window gauge values we re-derive from the status burn_rate scaled by
    // the level's burn-rate threshold ratio.  This is an approximation; for
    // exact per-window values call getStatus() which re-runs computeBurnRate.
    // In production, publishMetrics calls getAllStatuses() which already has
    // the per-alert info – extract them.
    for (const auto& alert : s.active_burn_rate_alerts) {
        if (alert.level == level) {
            return alert.burn_rate;
        }
    }
    return 0.0;
}

} // namespace observability
} // namespace themis
