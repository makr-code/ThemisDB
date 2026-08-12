/**
 * @file federated_inference_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/i_federated_inference_backend.h"
#include "sharding/remote_executor.h"
#include "sharding/shard_topology.h"
#include <chrono>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace themis::llm {

/**
 * @brief Production fan-out coordinator for cross-shard inference.
 *
 * `FederatedInferenceCoordinator` implements `IFederatedInferenceBackend` by
 * using `sharding::RemoteExecutor` to POST inference requests to the
 * `/api/v1/inference/generate` endpoint on every requested shard instance.
 *
 * ### Fan-out / Fan-in behaviour
 * 1. For each `instance_id` in the request list the coordinator looks up the
 *    `ShardInfo` from the topology registry provided at construction time.
 * 2. It dispatches each POST in its own `std::async` task (parallel fan-out).
 * 3. Each instance dispatch is retried up to `config_.max_retries` times with
 *    exponential back-off on transient failures (HTTP 5xx, network error).
 * 4. Once all futures resolve (or the `per_instance_timeout` elapses) the
 *    results are collected and returned as a vector of `FanOutInstanceResult`.
 *
 * ### Partial failures
 * - An instance that fails after all retries records `success = false` and
 *   a descriptive `error` string.  **The coordinator never silently drops
 *   failures** — every element of `instance_ids` appears exactly once in the
 *   returned vector.
 * - The caller (`InferenceEngineEnhanced`) decides the merge strategy (first
 *   successful win, majority vote, …).
 *
 * ### Thread safety
 * `execute()` is safe to call concurrently from multiple worker threads.
 */
class FederatedInferenceCoordinator final : public IFederatedInferenceBackend {
public:
    struct Config {
        /// Maximum number of retry attempts per instance on transient failure.
        uint32_t max_retries = 2;
        /// Base delay (ms) for exponential back-off between retries.
        uint32_t retry_base_delay_ms = 50;
        /// Per-instance wall-clock timeout (ms) for the entire attempt budget.
        uint32_t per_instance_timeout_ms = 5000;
        /// Inference endpoint path on the remote shard.
        std::string inference_endpoint = "/api/v1/inference/generate";
    };

    /**
     * @param executor   Pointer to the mTLS remote executor.  Not owned.
     * @param topology   Shard topology registry used to resolve instance_id →
     *                   ShardInfo.
     * @param config     Coordinator configuration.
     */
    FederatedInferenceCoordinator(sharding::RemoteExecutor*                executor,
                                  std::shared_ptr<sharding::ShardTopology>  topology,
                                  const Config&                             config = {});

    ~FederatedInferenceCoordinator() override = default;

    /**
     * @brief Inject a static shard registry for testing.
     *
     * When `topology` is null (e.g. in unit tests), callers can register
     * ShardInfo objects directly via this map.  If a shard_id is present in
     * the static map it takes priority over the topology lookup.
     */
    void addStaticShard(const std::string& instance_id,
                        const sharding::ShardInfo& shard);

    // IFederatedInferenceBackend
    std::vector<FanOutInstanceResult> execute(
        const std::vector<std::string>& instance_ids,
        const InferenceRequest&         request) override;

private:
    sharding::RemoteExecutor*                executor_;
    std::shared_ptr<sharding::ShardTopology> topology_;
    Config                                   config_;

    mutable std::mutex static_shards_mutex_;
    std::unordered_map<std::string, sharding::ShardInfo> static_shards_;

    /// Dispatch one instance with retries; returns a single FanOutInstanceResult.
    FanOutInstanceResult dispatchToInstance(const std::string&           instance_id,
                                            const sharding::ShardInfo&   shard,
                                            const InferenceRequest&       request);

    /// Resolve instance_id to ShardInfo (static map first, then topology).
    std::optional<sharding::ShardInfo> resolveShard(const std::string& instance_id) const;

    /// Build the POST body from an InferenceRequest.
    static nlohmann::json buildRequestBody(const InferenceRequest& request);

    /// Parse a remote response JSON into an InferenceResponse.
    static InferenceResponse parseResponse(const nlohmann::json& data,
                                           const std::string&    instance_id);
};

} // namespace themis::llm
