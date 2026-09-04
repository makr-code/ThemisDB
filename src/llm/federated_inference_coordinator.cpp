/**
 * @file federated_inference_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/federated_inference_coordinator.h"
#include <spdlog/spdlog.h>
#include <future>
#include <thread>
#include <chrono>
#include <algorithm>

namespace themis::llm {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

FederatedInferenceCoordinator::FederatedInferenceCoordinator(
    sharding::RemoteExecutor*                executor,
    std::shared_ptr<sharding::ShardTopology> topology,
    const Config&                            config)
    : executor_(executor)
    , topology_(std::move(topology))
    , config_(config)
{}

void FederatedInferenceCoordinator::addStaticShard(const std::string&         instance_id,
                                                    const sharding::ShardInfo& shard) {
    std::lock_guard<std::mutex> lock(static_shards_mutex_);
    static_shards_[instance_id] = shard;
}

// ---------------------------------------------------------------------------
// IFederatedInferenceBackend::execute()
// ---------------------------------------------------------------------------

std::vector<FanOutInstanceResult> FederatedInferenceCoordinator::execute(
    const std::vector<std::string>& instance_ids,
    const InferenceRequest&         request) {

    if (instance_ids.empty()) {
        return {};
    }

    // Resolve all shards up front so per-instance futures do not need locks.
    struct DispatchItem {
        std::string instance_id = {};
        std::optional<sharding::ShardInfo> shard;
    };
    std::vector<DispatchItem> items = {};

    items.reserve(instance_ids.size());
    for (const auto& id : instance_ids) {
        items.push_back({id, resolveShard(id)});
    }

    // Fan-out: launch one async task per instance.
    std::vector<std::future<FanOutInstanceResult>> futures;
    futures.reserve(items.size());

    for (const auto& item : items) {
        if (!item.shard.has_value()) {
            // Cannot resolve → fail immediately without consuming a thread.
            futures.push_back(
                std::async(std::launch::deferred, [item]() -> FanOutInstanceResult {
                    FanOutInstanceResult r;
                    r.instance_id = item.instance_id;
                    r.success     = false;
                    r.error       = "Unknown instance_id '" + item.instance_id +
                                    "': not found in static registry or topology";
                    r.attempts    = 0;
                    return r;
                }));
        } else {
            const sharding::ShardInfo shard_copy = *item.shard;
            const std::string         id_copy    = item.instance_id;
            futures.push_back(
                std::async(std::launch::async,
                           [this, id_copy, shard_copy, &request]() -> FanOutInstanceResult {
                               return dispatchToInstance(id_copy, shard_copy, request);
                           }));
        }
    }

    // Fan-in: collect results.  We wait at most per_instance_timeout_ms for
    // each future (accounting for elapsed time).
    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(config_.per_instance_timeout_ms);

    std::vector<FanOutInstanceResult> results = {};

    results.reserve(futures.size());

    for (size_t i = 0; i <static_cast<int>(futures.size()); ++i) {
        const auto& item = items[i];
        auto&        fut  = futures[i];

        const auto now_remaining = deadline - std::chrono::steady_clock::now();
        if (now_remaining <= std::chrono::milliseconds::zero()) {
            // Budget exhausted: mark remaining instances as timed-out.
            FanOutInstanceResult r;
            r.instance_id = item.instance_id;
            r.success     = false;
            r.error       = "Timeout waiting for fan-out response from '" +
                            item.instance_id + "'";
            r.attempts    = 0;
            results.push_back(std::move(r));
            continue;
        }

        const auto status = fut.wait_for(now_remaining);
        if (status == std::future_status::ready) {
            results.push_back(fut.get());
        } else {
            FanOutInstanceResult r;
            r.instance_id = item.instance_id;
            r.success     = false;
            r.error       = "Timeout waiting for fan-out response from '" +
                            item.instance_id + "' (per_instance_timeout_ms=" +
                            std::to_string(config_.per_instance_timeout_ms) + ")";
            r.attempts    = 0;
            results.push_back(std::move(r));
        }
    }

    // Log summary.
    const size_t successes =
        static_cast<size_t>(std::count_if(results.begin(), results.end(),
                                          [](const FanOutInstanceResult& r) { return r.success; }));
    spdlog::info("FederatedInferenceCoordinator: fan-out complete — {}/{} instances succeeded",
                 successes,static_cast<int>(results.size()));

    return results;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

FanOutInstanceResult FederatedInferenceCoordinator::dispatchToInstance(
    const std::string&         instance_id,
    const sharding::ShardInfo& shard,
    const InferenceRequest&    request) {

    FanOutInstanceResult result;
    result.instance_id = instance_id;

    const nlohmann::json body = buildRequestBody(request);

    uint32_t attempt = 0;
    while (true) {
        ++attempt;
        result.attempts = static_cast<int>(attempt);

        if (executor_ == nullptr) {
            result.success = false;
            result.error   = "No RemoteExecutor attached to FederatedInferenceCoordinator";
            return result;
        }

        const auto remote_result =
            executor_->post(shard, config_.inference_endpoint, body);

        if (remote_result.success) {
            result.response = parseResponse(remote_result.data, instance_id);
            result.success  = true;
            spdlog::debug("FederatedInferenceCoordinator: instance '{}' succeeded "
                          "(attempt {})", instance_id, attempt);
            return result;
        }

        // Transient check: HTTP 5xx or empty data → retry; 4xx → permanent failure.
        const bool permanent = (remote_result.http_status >= 400 &&
                                remote_result.http_status < 500);
        if (permanent || attempt > config_.max_retries) {
            result.success = false;
            result.error   = "Instance '" + instance_id + "' failed after " +
                             std::to_string(attempt) + " attempt(s): " +
                             remote_result.error + " (HTTP " +
                             std::to_string(remote_result.http_status) + ")";
            spdlog::warn("FederatedInferenceCoordinator: {}", result.error);
            return result;
        }

        // Exponential back-off before retry.
        const uint32_t delay_ms = config_.retry_base_delay_ms * (1u << (attempt - 1));
        spdlog::debug("FederatedInferenceCoordinator: instance '{}' attempt {} failed "
                      "({}); retrying in {} ms",
                      instance_id, attempt, remote_result.error, delay_ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
}

std::optional<sharding::ShardInfo>
FederatedInferenceCoordinator::resolveShard(const std::string& instance_id) const {
    {
        std::lock_guard<std::mutex> lock(static_shards_mutex_);
        const auto it = static_shards_.find(instance_id);
        if (it != static_shards_.end()) {
            return it->second;
        }
    }
    if (topology_) {
        return topology_->getShard(instance_id);
    }
    return std::nullopt;
}

nlohmann::json FederatedInferenceCoordinator::buildRequestBody(const InferenceRequest& req) {
    nlohmann::json body;
    body["prompt"]      = req.prompt;
    body["max_tokens"]  = req.max_tokens;
    body["temperature"] = req.temperature;
    if (!req.model_id.empty()) {
        body["model_id"] = req.model_id;
    }
    if (!req.request_id.empty()) {
        body["request_id"] = req.request_id;
    }
    if (!req.trace_id.empty()) {
        body["trace_id"]  = req.trace_id;
        body["span_id"]   = req.span_id;
    }
    if (!req.metadata.empty()) {
        body["metadata"] = req.metadata;
    }
    return body;
}

InferenceResponse FederatedInferenceCoordinator::parseResponse(const nlohmann::json& data,
                                                               const std::string& instance_id) {
    InferenceResponse resp;
    resp.success = true;
    if (data.contains("text") && data["text"].is_string()) {
        resp.text = data["text"].get<std::string>();
    }
    if (data.contains("request_id") && data["request_id"].is_string()) {
        resp.request_id = data["request_id"].get<std::string>();
    }
    if (data.contains("model_id") && data["model_id"].is_string()) {
        resp.model_id = data["model_id"].get<std::string>();
    }
    if (data.contains("tokens_generated") && data["tokens_generated"].is_number_integer()) {
        resp.tokens_generated = data["tokens_generated"].get<int>();
    }
    if (data.contains("inference_time_ms") && data["inference_time_ms"].is_number()) {
        resp.inference_time_ms = data["inference_time_ms"].get<float>();
    }
    // Tag the response with the instance that produced it.
    resp.metadata["source_instance_id"] = instance_id;
    return resp;
}

} // namespace themis::llm

