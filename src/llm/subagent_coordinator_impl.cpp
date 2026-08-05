/**
 * @file subagent_coordinator_impl.cpp
 * @brief Implementation of SubagentCoordinator — orchestrates parallel inference
 *        across multiple subagents with merge strategies and failure handling.
 */

#include "llm/subagent_coordinator.h"
#include "llm/subagent_factory.h"
#include "llm/subagent.h"
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <future>
#include <mutex>
#include <numeric>
#include <thread>

namespace themis {
namespace llm {

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
                
                SubagentInferenceResult inference_result;
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
        
        // Check if coordination succeeded based on merge strategy
        bool merge_success = false;
        switch (config.strategy) {
            case SubagentMergeStrategy::FIRST_WIN:
                // First successful result wins
                for (const auto& coord_result : result.per_subagent_results) {
                    if (coord_result.success) {
                        result.merged_output = coord_result.output;
                        merge_success = true;
                        break;
                    }
                }
                break;

            case SubagentMergeStrategy::ALL_SUCCEED:
                // All must succeed
                merge_success = (result.num_failed == 0 && result.num_successful > 0);
                if (merge_success) {
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
                // Find result with highest quality_score
                {
                    float best_score = -1.0f;
                    for (const auto& coord_result : result.per_subagent_results) {
                        if (coord_result.success && coord_result.quality_score > best_score) {
                            best_score = coord_result.quality_score;
                            result.merged_output = coord_result.output;
                            merge_success = true;
                        }
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
                break;

            case SubagentMergeStrategy::CUSTOM:
                // Use custom merge function
                if (config.custom_merge_fn) {
                    auto merge_result = config.custom_merge_fn(result.per_subagent_results);
                    if (merge_result) {
                        result.merged_output = merge_result.value();
                        merge_success = true;
                    } else {
                        diagnostics_.merge_failed = true;
                        diagnostics_.merge_error = merge_result.error();
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

        // Build summary
        result.summary = "Coordination completed: " +
                        std::to_string(result.num_successful) + " successes, " +
                        std::to_string(result.num_failed) + " failures, " +
                        std::to_string(result.total_latency_ms) + "ms";

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
        
        std::vector<SubagentCoordinatorAggregateResult> results;
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
        return make_unexpected("Factory is null");
    }
    return make_expected<std::unique_ptr<SubagentCoordinator>>(
        std::make_unique<SubagentCoordinatorImpl>(factory)
    );
}

} // namespace llm
} // namespace themis
