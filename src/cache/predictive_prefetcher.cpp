/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            predictive_prefetcher.cpp                          ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-03-16 04:13:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     160                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9a32a18e6  2026-02-24  feat(cache): implement predictive pre-fetching based on q... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "cache/predictive_prefetcher.h"
#include "storage/rocksdb_wrapper.h"
#include "observability/metrics_collector.h"
#include <algorithm>
#include <functional>
#include <utility>

namespace themis {
namespace cache {

static constexpr char PREFETCH_MODEL_PREFIX[] = "prefetch_model::";

PredictivePrefetcher::PredictivePrefetcher(const Config& config)
    : config_(config) {}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

bool PredictivePrefetcher::useMarkovModel(const std::string& tenant_id) const {
    if (!config_.enable_ab_test) {
        // No A/B test: always use time-of-day weighting when enabled.
        return config_.enable_time_of_day_weighting;
    }
    // A/B split: deterministic hash of tenant_id.
    // Group 0 → Markov model; group 1 → frequency baseline.
    return (std::hash<std::string>{}(tenant_id) % 2) == 0;
}

int PredictivePrefetcher::currentHour() {
    std::time_t t = std::time(nullptr);
    std::tm tm_local{};
#ifdef _WIN32
    localtime_s(&tm_local, &t);
#else
    localtime_r(&t, &tm_local);
#endif
    return tm_local.tm_hour;  // [0, 23]
}

void PredictivePrefetcher::emitMetrics() const {
    // Called while mutex_ is held – use the already-computed values.
    auto& mc = observability::MetricsCollector::getInstance();

    double hit_rate = 0.0;
    if (candidates_generated_ > 0) {
        hit_rate = static_cast<double>(prefetch_hits_) /
                   static_cast<double>(candidates_generated_);
    }
    mc.setGauge("cache.prefetch.hit_rate", hit_rate);
    mc.setGauge("cache.prefetch.overhead_bytes",
                static_cast<double>(overhead_bytes_));

    if (config_.enable_ab_test) {
        double markov_rate = ab_markov_generated_ > 0
            ? static_cast<double>(ab_markov_hits_) /
              static_cast<double>(ab_markov_generated_)
            : 0.0;
        double baseline_rate = ab_baseline_generated_ > 0
            ? static_cast<double>(ab_baseline_hits_) /
              static_cast<double>(ab_baseline_generated_)
            : 0.0;
        mc.setGauge("cache.prefetch.ab.markov_hit_rate",   markov_rate);
        mc.setGauge("cache.prefetch.ab.baseline_hit_rate", baseline_rate);
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void PredictivePrefetcher::recordQueryAccess(const std::string& fingerprint,
                                              const std::string& tenant_id) {
    if (fingerprint.empty()) return;

    std::lock_guard<std::mutex> lock(mutex_);

    const std::string session_key = tenant_id;  // empty string = global session

    auto it = last_fingerprint_.find(session_key);
    if (it != last_fingerprint_.end() && !it->second.empty() &&
        it->second != fingerprint) {
        const std::string& from = it->second;

        // Ensure source key exists; evict oldest if at capacity
        if (transitions_.find(from) == transitions_.end()) {
            if (ordered_keys_.size() >= config_.max_tracked_keys) {
                // Evict the oldest tracked source key
                const std::string& oldest = ordered_keys_.front();
                transitions_.erase(oldest);
                tod_buckets_.erase(oldest);
                ordered_keys_.erase(ordered_keys_.begin());
            }
            ordered_keys_.push_back(from);
            transitions_[from] = {};
        }

        auto& successors = transitions_[from];

        // Limit successors per key
        if (successors.size() < config_.max_successors_per_key ||
            successors.find(fingerprint) != successors.end()) {
            successors[fingerprint]++;
            total_transitions_recorded_++;

            // Time-of-day bookkeeping (always track if enabled, even for A/B baseline)
            if (config_.enable_time_of_day_weighting || config_.enable_ab_test) {
                int hour = currentHour();
                tod_buckets_[from][fingerprint][static_cast<size_t>(hour)]++;
            }
        }
    }

    last_fingerprint_[session_key] = fingerprint;
}

std::vector<std::string> PredictivePrefetcher::getPrefetchCandidates(
    const std::string& fingerprint,
    const std::string& tenant_id) const {
    if (fingerprint.empty()) return {};

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = transitions_.find(fingerprint);
    if (it == transitions_.end() || it->second.empty()) {
        return {};
    }

    const auto& successors = it->second;
    const bool use_tod = useMarkovModel(tenant_id);
    const int current_hour = use_tod ? currentHour() : 0;

    // Calculate total raw transitions from this source key
    uint64_t total = 0;
    for (const auto& [key, count] : successors) {
        total += count;
    }

    // Collect candidates meeting the threshold criteria
    std::vector<std::pair<double, std::string>> candidates;
    candidates.reserve(successors.size());

    // Time-of-day bucket lookup (if applicable)
    const auto tod_from_it = tod_buckets_.find(fingerprint);

    for (const auto& [to, count] : successors) {
        if (count < config_.min_transition_count) continue;
        if (total > 0 && config_.min_confidence > 0.0) {
            double confidence = static_cast<double>(count) / static_cast<double>(total);
            if (confidence < config_.min_confidence) continue;
        }

        double score = static_cast<double>(count);

        if (use_tod && tod_from_it != tod_buckets_.end()) {
            const auto tod_to_it = tod_from_it->second.find(to);
            if (tod_to_it != tod_from_it->second.end()) {
                // Laplace-smoothed time-of-day weight
                uint32_t hour_count =
                    tod_to_it->second[static_cast<size_t>(current_hour)];
                uint32_t total_tod = 0;
                for (uint32_t h : tod_to_it->second) total_tod += h;
                double tod_weight = (total_tod > 0)
                    ? static_cast<double>(hour_count + 1) /
                      static_cast<double>(total_tod + 24)
                    : (1.0 / 24.0);
                score *= (1.0 + tod_weight);
            }
        }

        candidates.emplace_back(score, to);
    }

    if (candidates.empty()) return {};

    // Sort descending by score
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<std::string> result;
    const size_t limit = std::min(candidates.size(), config_.max_predictions);
    result.reserve(limit);
    for (size_t i = 0; i < limit; ++i) {
        result.push_back(std::move(candidates[i].second));
    }

    // Update A/B generation counters
    if (config_.enable_ab_test) {
        if (use_tod) {
            const_cast<PredictivePrefetcher*>(this)->ab_markov_generated_++;
        } else {
            const_cast<PredictivePrefetcher*>(this)->ab_baseline_generated_++;
        }
    }

    return result;
}

void PredictivePrefetcher::recordPrefetchHit() {
    std::lock_guard<std::mutex> lock(mutex_);
    prefetch_hits_++;
    emitMetrics();
}

void PredictivePrefetcher::recordCandidatesGenerated() {
    std::lock_guard<std::mutex> lock(mutex_);
    candidates_generated_++;
}

void PredictivePrefetcher::recordOverheadBytes(uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    overhead_bytes_ += bytes;
    emitMetrics();
}

void PredictivePrefetcher::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    transitions_.clear();
    tod_buckets_.clear();
    ordered_keys_.clear();
    last_fingerprint_.clear();
    total_transitions_recorded_ = 0;
    candidates_generated_ = 0;
    prefetch_hits_ = 0;
    overhead_bytes_ = 0;
    ab_markov_hits_ = 0;
    ab_markov_generated_ = 0;
    ab_baseline_hits_ = 0;
    ab_baseline_generated_ = 0;
}

nlohmann::json PredictivePrefetcher::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json j;
    j["tracked_keys"]               = transitions_.size();
    j["total_transitions_recorded"] = total_transitions_recorded_;
    j["candidates_generated"]       = candidates_generated_;
    j["prefetch_hits"]              = prefetch_hits_;
    j["overhead_bytes"]             = overhead_bytes_;
    j["max_tracked_keys"]           = config_.max_tracked_keys;
    j["max_predictions"]            = config_.max_predictions;
    j["min_transition_count"]       = config_.min_transition_count;
    j["min_confidence"]             = config_.min_confidence;
    j["time_of_day_weighting"]      = config_.enable_time_of_day_weighting;
    j["ab_test_enabled"]            = config_.enable_ab_test;

    double hit_rate = 0.0;
    if (candidates_generated_ > 0) {
        hit_rate = static_cast<double>(prefetch_hits_) /
                   static_cast<double>(candidates_generated_);
    }
    j["hit_rate"] = hit_rate;

    if (config_.enable_ab_test) {
        j["ab"]["markov_hits"]       = ab_markov_hits_;
        j["ab"]["markov_generated"]  = ab_markov_generated_;
        j["ab"]["baseline_hits"]     = ab_baseline_hits_;
        j["ab"]["baseline_generated"]= ab_baseline_generated_;
    }
    return j;
}

// ---------------------------------------------------------------------------
// RocksDB persistence
// ---------------------------------------------------------------------------

void PredictivePrefetcher::saveModel(RocksDBWrapper* db) {
    if (!db) return;

    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& [from, successors] : transitions_) {
        for (const auto& [to, count] : successors) {
            // Build the RocksDB key
            std::string key = std::string(PREFETCH_MODEL_PREFIX)
                              + from + "::" + to;

            // Build the JSON payload
            nlohmann::json val;
            val["count"] = count;

            // Attach time-of-day histogram if available
            auto tod_from_it = tod_buckets_.find(from);
            if (tod_from_it != tod_buckets_.end()) {
                auto tod_to_it = tod_from_it->second.find(to);
                if (tod_to_it != tod_from_it->second.end()) {
                    val["tod"] = tod_to_it->second;  // array of 24 ints
                }
            }

            db->put(key, val.dump());
        }
    }
}

void PredictivePrefetcher::loadModel(RocksDBWrapper* db) {
    if (!db) return;

    std::lock_guard<std::mutex> lock(mutex_);

    db->scanPrefix(PREFETCH_MODEL_PREFIX,
        [&](std::string_view raw_key, std::string_view raw_value) {
            // Strip the prefix to get "from::to"
            std::string_view pair_view =
                raw_key.substr(sizeof(PREFETCH_MODEL_PREFIX) - 1);
            const std::string pair_str(pair_view);

            // Split on "::" separator
            const auto sep_pos = pair_str.find("::");
            if (sep_pos == std::string::npos) return;

            std::string from = pair_str.substr(0, sep_pos);
            std::string to   = pair_str.substr(sep_pos + 2);

            if (from.empty() || to.empty()) return;

            nlohmann::json val;
            try {
                val = nlohmann::json::parse(raw_value);
            } catch (...) {
                return;
            }

            if (!val.contains("count")) return;
            uint32_t count = val["count"].get<uint32_t>();

            // Ensure source key entry exists (no FIFO eviction during load)
            if (transitions_.find(from) == transitions_.end()) {
                if (ordered_keys_.size() < config_.max_tracked_keys) {
                    ordered_keys_.push_back(from);
                    transitions_[from] = {};
                } else {
                    return;  // Table full; skip
                }
            }

            auto& successors = transitions_[from];
            if (successors.size() < config_.max_successors_per_key ||
                successors.find(to) != successors.end()) {
                // Merge: take the max of persisted vs. in-memory count
                successors[to] = std::max(successors[to], count);
                total_transitions_recorded_ += count;
            }

            // Restore time-of-day histogram
            if (val.contains("tod") && val["tod"].is_array() &&
                val["tod"].size() == 24) {
                auto& arr = tod_buckets_[from][to];
                for (size_t h = 0; h < 24; ++h) {
                    arr[h] = std::max(arr[h], val["tod"][h].get<uint32_t>());
                }
            }
        });
}

} // namespace cache
} // namespace themis
