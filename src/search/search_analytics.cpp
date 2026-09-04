/**
 * @file search_analytics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/search_analytics.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

SearchAnalytics::SearchAnalytics(const Config& config) : config_(config) {
    if (config_.max_events == 0) {
        throw std::invalid_argument("SearchAnalytics: max_events must be > 0");
    }
    events_.reserve(std::min(config_.max_events, size_t{1024}));
}

// ============================================================================
// Event recording
// ============================================================================

void SearchAnalytics::record(const std::string& query,
                              size_t result_count,
                              double latency_ms) {
    SearchEvent ev;
    ev.query        = query;
    ev.ts           = std::chrono::system_clock::now();
    ev.result_count = result_count;
    ev.latency_ms   = latency_ms;
    ev.is_zero_result = (result_count == 0);

    std::lock_guard<std::mutex> lock(mu_);
    if (events_.size() >= config_.max_events) {
        // Evict the oldest entry
        events_.erase(events_.begin());
    }
    const bool is_zero = ev.is_zero_result;
    events_.push_back(std::move(ev));

    if (is_zero) {
        THEMIS_WARN("SearchAnalytics: zero-result query recorded: '{}'", query);
    }
}

// ============================================================================
// Querying
// ============================================================================

std::vector<SearchEvent> SearchAnalytics::getZeroResultQueries(size_t limit) const {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<SearchEvent> result;
    for (auto it = events_.rbegin(); it != events_.rend(); ++it) {
        if (it->is_zero_result) {
            result.push_back(*it);
            if (result.size() >= limit) break;
        }
    }
    return result; // most-recent first
}

std::vector<SearchEvent> SearchAnalytics::getRecentEvents(size_t limit) const {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<SearchEvent> result;
    size_t n = std::min(limit, events_.size());
    result.reserve(n);
    for (auto it = events_.rbegin(); it != events_.rend() && result.size() < n; ++it) {
        result.push_back(*it);
    }
    return result; // most-recent first
}

SearchMetrics SearchAnalytics::computeMetrics() const {
    std::lock_guard<std::mutex> lock(mu_);
    SearchMetrics m;
    if (events_.empty()) return m;

    m.total_queries = events_.size();
    std::map<std::string, size_t> query_freq;
    std::vector<double> latencies;
    latencies.reserve(events_.size());

    for (const auto& ev : events_) {
        if (ev.is_zero_result) ++m.zero_result_queries;
        latencies.push_back(ev.latency_ms);
        query_freq[ev.query]++;
    }

    // Average latency
    m.avg_latency_ms = std::accumulate(latencies.begin(), latencies.end(), 0.0)
                       / static_cast<double>(latencies.size());

    // Percentiles
    std::vector<double> sorted_lat = latencies;
    std::sort(sorted_lat.begin(), sorted_lat.end());
    auto percentile = [&]([[maybe_unused]] double p) -> double {
        size_t idx = static_cast<size_t>(p * static_cast<double>(sorted_lat.size() - 1));
        return sorted_lat[idx];
    };
    m.p95_latency_ms = percentile(0.95);
    m.p99_latency_ms = percentile(0.99);

    // Zero-result rate
    m.zero_result_rate = static_cast<double>(m.zero_result_queries)
                         / static_cast<double>(m.total_queries);

    // Top queries (up to 20 by frequency)
    std::vector<std::pair<std::string, size_t>> freq_vec(query_freq.begin(), query_freq.end());
    std::partial_sort(freq_vec.begin(),
                      freq_vec.begin() + std::min(size_t{20}, freq_vec.size()),
                      freq_vec.end(),
                      [](const auto& a, const auto& b) { return a.second > b.second; });
    for (size_t i = 0; i < std::min(size_t{20}, freq_vec.size()); ++i) {
        m.top_queries[freq_vec[i].first] = freq_vec[i].second;
    }

    return m;
}

std::vector<std::pair<std::string, size_t>>
SearchAnalytics::getTopQueries(size_t limit) const {
    std::lock_guard<std::mutex> lock(mu_);
    if (events_.empty() || limit == 0) return {};

    std::map<std::string, size_t> freq;
    for (const auto& ev : events_) {
        freq[ev.query]++;
    }

    std::vector<std::pair<std::string, size_t>> result(freq.begin(), freq.end());
    size_t n = std::min(limit, result.size());
    std::partial_sort(result.begin(), result.begin() + static_cast<std::ptrdiff_t>(n),
                      result.end(),
                      [](const auto& a, const auto& b) {
                          return a.second != b.second ? a.second > b.second
                                                      : a.first < b.first;
                      });
    result.resize(n);
    return result;
}

// ============================================================================
// Utilities
// ============================================================================

size_t SearchAnalytics::eventCount() const {
    std::lock_guard<std::mutex> lock(mu_);
    return events_.size();
}

void SearchAnalytics::clear() {
    std::lock_guard<std::mutex> lock(mu_);
    events_.clear();
}

} // namespace themis
