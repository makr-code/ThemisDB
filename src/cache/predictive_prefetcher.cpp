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
#include <algorithm>
#include <utility>

namespace themis {
namespace cache {

PredictivePrefetcher::PredictivePrefetcher(const Config& config)
    : config_(config) {}

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
        }
    }

    last_fingerprint_[session_key] = fingerprint;
}

std::vector<std::string> PredictivePrefetcher::getPrefetchCandidates(
    const std::string& fingerprint,
    const std::string& /*tenant_id*/) const {
    if (fingerprint.empty()) return {};

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = transitions_.find(fingerprint);
    if (it == transitions_.end() || it->second.empty()) {
        return {};
    }

    const auto& successors = it->second;

    // Calculate total transitions from this source key
    uint64_t total = 0;
    for (const auto& [key, count] : successors) {
        total += count;
    }

    // Collect candidates meeting the threshold criteria
    std::vector<std::pair<uint32_t, std::string>> candidates;
    candidates.reserve(successors.size());

    for (const auto& [to, count] : successors) {
        if (count < config_.min_transition_count) continue;
        if (total > 0 && config_.min_confidence > 0.0) {
            double confidence = static_cast<double>(count) / static_cast<double>(total);
            if (confidence < config_.min_confidence) continue;
        }
        candidates.emplace_back(count, to);
    }

    if (candidates.empty()) return {};

    // Sort descending by transition count
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<std::string> result;
    const size_t limit = std::min(candidates.size(), config_.max_predictions);
    result.reserve(limit);
    for (size_t i = 0; i < limit; ++i) {
        result.push_back(std::move(candidates[i].second));
    }

    return result;
}

void PredictivePrefetcher::recordPrefetchHit() {
    std::lock_guard<std::mutex> lock(mutex_);
    prefetch_hits_++;
}

void PredictivePrefetcher::recordCandidatesGenerated() {
    std::lock_guard<std::mutex> lock(mutex_);
    candidates_generated_++;
}

void PredictivePrefetcher::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    transitions_.clear();
    ordered_keys_.clear();
    last_fingerprint_.clear();
    total_transitions_recorded_ = 0;
    candidates_generated_ = 0;
    prefetch_hits_ = 0;
}

nlohmann::json PredictivePrefetcher::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json j;
    j["tracked_keys"]               = transitions_.size();
    j["total_transitions_recorded"] = total_transitions_recorded_;
    j["candidates_generated"]       = candidates_generated_;
    j["prefetch_hits"]              = prefetch_hits_;
    j["max_tracked_keys"]           = config_.max_tracked_keys;
    j["max_predictions"]            = config_.max_predictions;
    j["min_transition_count"]       = config_.min_transition_count;
    j["min_confidence"]             = config_.min_confidence;
    return j;
}

} // namespace cache
} // namespace themis
