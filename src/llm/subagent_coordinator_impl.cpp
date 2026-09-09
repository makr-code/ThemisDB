/**
 * @file subagent_coordinator_impl.cpp
 * @brief Implementation of SubagentCoordinator â€” orchestrates parallel inference
 *        across multiple subagents with merge strategies and failure handling.
 */

#include "llm/subagent_coordinator.h"
#include "llm/subagent_factory.h"
#include "llm/subagent.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <future>
#include <limits>
#include <mutex>
#include <numeric>
#include <thread>
#include <unordered_map>

namespace themis {
namespace llm {

namespace {

std::string trim(const std::string& value) {
    auto begin = std::find_if_not(value.begin(), value.end(),
                                  [](unsigned char c) { return std::isspace(c) != 0; });
    auto end = std::find_if_not(value.rbegin(), value.rend(),
                                [](unsigned char c) { return std::isspace(c) != 0; }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::string normalizeKey(const std::string& output) {
    const auto trimmed = trim(output);
    if (trimmed.empty()) {
        return {};
    }
    try {
        return nlohmann::json::parse(trimmed).dump();
    } catch (...) {
    }

    std::string normalized;
    normalized.reserve(trimmed.size());
    bool previous_space = false;
    for (unsigned char c : trimmed) {
        const bool is_space = std::isspace(c) != 0;
        if (is_space) {
            if (!previous_space) {
                normalized.push_back(' ');
            }
            previous_space = true;
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(c)));
        previous_space = false;
    }
    return normalized;
}

float computeQualityScore(const SubagentCoordinatorResult& result) {
    if (!result.success) {
        return 0.0f;
    }

    float score = 0.5f;
    const auto normalized = normalizeKey(result.output);
    if (!normalized.empty()) {
        score += std::min(0.3f, static_cast<float>(normalized.size()) / 400.0f);
    }
    if (result.latency_ms > 0) {
        score += std::max(0.0f, 0.2f - (static_cast<float>(result.latency_ms) / 5000.0f));
    }
    return std::clamp(score, 0.0f, 1.0f);
}

} // namespace

/** @brief Subagent coordinator implementation detail. */
class SubagentCoordinatorImpl : public SubagentCoordinator {
public:
    explicit SubagentCoordinatorImpl(std::shared_ptr<SubagentFactory> factory)
        : factory_(factory)
        , stats_()
        , diagnostics_() {}

    SubagentCoordinatorAggregateResult inferMultiple(
        const std::vector<std::string>& subagent_ids,
        const InferenceRequest& request,
        const SubagentCoordinatorConfig& config) override {
        
        auto start_time = std::chrono::steady_clock::now();
        SubagentCoordinatorAggregateResult result;
        result.strategy = config.strategy;

        // Validate subagent IDs
        std::vector<std::shared_ptr<Subagent>> subagents;
        for (const auto& id : subagent_ids) {
            auto subagent = factory_->getSubagent(id);
            if (!subagent) {
                result.success = false;
                result.summary = "Subagent not found: " + id;
                stats_.failed_coordinations++;
                return result;
            }
            subagents.push_back(subagent);
        }

        // Fan-out: Submit requests to all subagents asynchronously
        auto fanout_start = std::chrono::steady_clock::now();
        std::vector<std::future<SubagentInferenceResult>> futures;
        std::vector<std::string> submitted_ids;

        for (size_t i = 0; i < subagents.size(); ++i) {
            futures.push_back(subagents[i]->inferAsync(request, config.correlation_context));
            submitted_ids.push_back(subagent_ids[i]);
            stats_.total_subagent_requests++;
        }

        auto fanout_end = std::chrono::steady_clock::now();
        diagnostics_.fan_out_latency = 
            std::chrono::duration_cast<std::chrono::milliseconds>(fanout_end - fanout_start);

        // Fan-in: Collect results from all subagents
        auto fanin_start = std::chrono::steady_clock::now();
        
        // Determine timeout
        int actual_timeout_ms = config.timeout_ms > 0 ? config.timeout_ms : 30000;
        if (config.max_total_latency_ms > 0) {
            actual_timeout_ms = config.max_total_latency_ms;
        }

        auto deadline = std::chrono::steady_clock::now() +
                       std::chrono::milliseconds(actual_timeout_ms);

        for (size_t i = 0; i < futures.size(); ++i) {
            auto remaining = deadline - std::chrono::steady_clock::now();
            if (remaining.count() <= 0) {
                SubagentCoordinatorResult coord_result;
                coord_result.subagent_id = submitted_ids[i];
                coord_result.success = false;
                coord_result.error = "Timeout waiting for result";
                result.per_subagent_results.push_back(coord_result);
                result.num_failed++;
                stats_.total_subagent_failures++;
                continue;
            }

            try {
                auto status = futures[i].wait_for(
                    std::chrono::duration_cast<std::chrono::milliseconds>(remaining));
                
                SubagentInferenceResult inference_result = {};
                if (status == std::future_status::ready) {
                    inference_result = futures[i].get();
                } else {
                    inference_result.success = false;
                    inference_result.error = "Timeout waiting for result";
                }

                SubagentCoordinatorResult coord_result;
                coord_result.subagent_id = submitted_ids[i];
                coord_result.success = inference_result.success;
                coord_result.output = inference_result.output;
                coord_result.error = inference_result.error;
                coord_result.tokens_consumed = inference_result.tokens_consumed;
                coord_result.latency_ms = inference_result.latency_ms;
                coord_result.trace_id = inference_result.trace_id;
                coord_result.quality_score = computeQualityScore(coord_result);

                result.per_subagent_results.push_back(coord_result);

                if (inference_result.success) {
                    result.num_successful++;
                    stats_.total_subagent_successes++;
                } else {
                    result.num_failed++;
                    stats_.total_subagent_failures++;
                }
                result.total_tokens_consumed += inference_result.tokens_consumed;

            } catch (const std::exception& ex) {
                SubagentCoordinatorResult coord_result;
                coord_result.subagent_id = submitted_ids[i];
                coord_result.success = false;
                coord_result.error = ex.what();
                result.per_subagent_results.push_back(coord_result);
                result.num_failed++;
                stats_.total_subagent_failures++;
            }
        }

        auto fanin_end = std::chrono::steady_clock::now();
        diagnostics_.fan_in_latency = 
            std::chrono::duration_cast<std::chrono::milliseconds>(fanin_end - fanin_start);

        // Merge results
        auto merge_start = std::chrono::steady_clock::now();
        std::string merge_decision = "none";
        
        // Check if coordination succeeded based on merge strategy
        bool merge_success = false;
        switch (config.strategy) {
            case SubagentMergeStrategy::FIRST_WIN:
                // First successful result wins
                for (const auto& coord_result : result.per_subagent_results) {
                    if (coord_result.success) {
                        result.merged_output = coord_result.output;
                        merge_success = true;
                        merge_decision = "first_win winner=" + coord_result.subagent_id;
                        break;
                    }
                }
                break;

            case SubagentMergeStrategy::ALL_SUCCEED:
                // All must succeed
                merge_success = (result.num_failed == 0 && result.num_successful > 0);
                if (merge_success) {
                    merge_decision = "all_succeed merged=" +
                                     std::to_string(result.num_successful);
                    // Concatenate all outputs
                    for (const auto& coord_result : result.per_subagent_results) {
                        if (!result.merged_output.empty()) {
                            result.merged_output += "\n---\n";
                        }
                        result.merged_output += coord_result.output;
                    }
                }
                break;

            case SubagentMergeStrategy::BEST_SCORE:
                // Find result with highest quality_score, deterministic tie-breaks.
                {
                    float best_score = -1.0f;
                    int best_latency = std::numeric_limits<int>::max();
                    std::string best_id;
                    for (const auto& coord_result : result.per_subagent_results) {
                        if (!coord_result.success) {
                            continue;
                        }
                        if (coord_result.quality_score > best_score ||
                            (std::fabs(coord_result.quality_score - best_score) < 1e-6f &&
                             (coord_result.latency_ms < best_latency ||
                              (coord_result.latency_ms == best_latency &&
                               coord_result.subagent_id < best_id)))) {
                            best_score = coord_result.quality_score;
                            best_latency = coord_result.latency_ms;
                            best_id = coord_result.subagent_id;
                            result.merged_output = coord_result.output;
                            merge_success = true;
                        }
                    }
                    if (merge_success) {
                        merge_decision = "best_score winner=" + best_id +
                                         " quality=" + std::to_string(best_score);
                    }
                }
                break;

            case SubagentMergeStrategy::ENSEMBLE:
                // Combine all successful outputs
                for (const auto& coord_result : result.per_subagent_results) {
                    if (coord_result.success) {
                        if (!result.merged_output.empty()) {
                            result.merged_output += " ";
                        }
                        result.merged_output += coord_result.output;
                        merge_success = true;
                    }
                }
                if (merge_success) {
                    merge_decision = "ensemble contributors=" +
                                     std::to_string(result.num_successful);
                }
                break;

            case SubagentMergeStrategy::MAJORITY_VOTE:
                // Tally semantically-normalized outputs with score-aware tie-breaks.
                {
                    struct VoteBucket {
                        size_t votes = 0;
                        float total_quality = 0.0f;
                        std::string representative;
                    };
                    std::unordered_map<std::string, VoteBucket> tally = {};

                    for (const auto& coord_result : result.per_subagent_results) {
                        if (coord_result.success) {
                            const auto key = normalizeKey(coord_result.output);
                            if (key.empty()) {
                                continue;
                            }
                            auto& bucket = tally[key];
                            bucket.votes++;
                            bucket.total_quality += coord_result.quality_score;
                            if (bucket.representative.empty()) {
                                bucket.representative = coord_result.output;
                            }
                        }
                    }
                    size_t best_votes = 0;
                    float best_avg_quality = -1.0f;
                    std::string best_key;
                    for (const auto& [key, bucket] : tally) {
                        if (bucket.votes == 0) {
                            continue;
                        }
                        const float avg_quality = bucket.total_quality /
                                                  static_cast<float>(bucket.votes);
                        if (bucket.votes > best_votes ||
                            (bucket.votes == best_votes &&
                             (avg_quality > best_avg_quality ||
                              (std::fabs(avg_quality - best_avg_quality) < 1e-6f &&
                               key < best_key)))) {
                            best_votes = bucket.votes;
                            best_avg_quality = avg_quality;
                            best_key = key;
                            result.merged_output = bucket.representative;
                            merge_success = true;
                        }
                    }
                    if (merge_success) {
                        merge_decision =
                            "majority_vote winner_votes=" + std::to_string(best_votes) +
                            " avg_quality=" + std::to_string(best_avg_quality) +
                            " unique_candidates=" + std::to_string(tally.size());
                    }
                }
                break;

            case SubagentMergeStrategy::CUSTOM:
                // Use custom merge function
                if (config.custom_merge_fn) {
                    auto merge_result = config.custom_merge_fn(result.per_subagent_results);
                    if (merge_result) {
                        result.merged_output = merge_result.value();
                        merge_success = true;
                        merge_decision = "custom success";
                    } else {
                        diagnostics_.merge_failed = true;
                        diagnostics_.merge_error = "Custom merge function failed";
                        merge_decision = "custom failed";
                    }
                }
                break;
        }

        auto merge_end = std::chrono::steady_clock::now();
        diagnostics_.merge_latency = 
            std::chrono::duration_cast<std::chrono::milliseconds>(merge_end - merge_start);

        // Determine overall success
        if (config.fail_on_any_error) {
            result.success = merge_success && result.num_failed == 0;
        } else {
            result.success = merge_success;
        }

        auto end_time = std::chrono::steady_clock::now();
        result.total_latency_ms = 
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        // Build summary (also stored in diagnostics for getLastDiagnostics())
        result.summary = "Coordination completed: " +
                        std::to_string(result.num_successful) + " successes, " +
                        std::to_string(result.num_failed) + " failures, " +
                        std::to_string(result.total_latency_ms) + "ms" +
                        ", merge=" + merge_decision;
        diagnostics_.summary = result.summary;

        if (result.success) {
            stats_.successful_coordinations++;
        } else {
            stats_.failed_coordinations++;
        }
        stats_.total_coordinations++;

        return result;
    }

    std::vector<SubagentCoordinatorAggregateResult> inferMultipleBatch(
        const std::vector<std::string>& subagent_ids,
        const std::vector<InferenceRequest>& requests,
        const SubagentCoordinatorConfig& config) override {
        
        std::vector<SubagentCoordinatorAggregateResult> results = {};

        for (const auto& request : requests) {
            results.push_back(inferMultiple(subagent_ids, request, config));
        }
        return results;
    }

    CoordinationDiagnostics getLastDiagnostics() override {
        return diagnostics_;
    }

    CoordinatorStats getStats() override {
        return stats_;
    }

    void resetStats() override {
        stats_ = CoordinatorStats{};
    }

private:
    std::shared_ptr<SubagentFactory> factory_;
    CoordinatorStats stats_;
    CoordinationDiagnostics diagnostics_;
};

SubagentResult<std::unique_ptr<SubagentCoordinator>> SubagentCoordinator::create(
    std::shared_ptr<SubagentFactory> factory) {
    if (!factory) {
        return tl::make_unexpected(std::string("Factory is null"));
    }
    return make_expected<std::unique_ptr<SubagentCoordinator>>(
        std::make_unique<SubagentCoordinatorImpl>(factory)
    );
}

} // namespace llm
} // namespace themis
