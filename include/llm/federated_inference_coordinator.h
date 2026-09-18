/**
 * @file federated_inference_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class FederatedInferenceCoordinator final : public IFederatedInferenceBackend {
public:
    struct Config {
        uint32_t max_retries = 2;
        uint32_t retry_base_delay_ms = 50;
        uint32_t per_instance_timeout_ms = 5000;
        std::string inference_endpoint = "/api/v1/inference/generate";
    };

    FederatedInferenceCoordinator(sharding::RemoteExecutor*                executor,
                                  std::shared_ptr<sharding::ShardTopology>  topology);
    FederatedInferenceCoordinator(sharding::RemoteExecutor*                executor,
                                  std::shared_ptr<sharding::ShardTopology>  topology,
                                  const Config&                             config);

    ~FederatedInferenceCoordinator() override = default;

    /**
     * @brief Add Static Shard.
     * @param[in] instance_id Identifier of the instance.
     * @param[in] shard Input parameter.
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

    /**
     * @brief Dispatch To Instance.
     * @param[in] instance_id Identifier of the instance.
     * @param[in] shard Input parameter.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    FanOutInstanceResult dispatchToInstance(const std::string&           instance_id,
                                            const sharding::ShardInfo&   shard,
                                            const InferenceRequest&       request);

    /**
     * @brief Resolve Shard.
     * @param[in] instance_id Identifier of the instance.
     * @return Return value.
     */
    std::optional<sharding::ShardInfo> resolveShard(const std::string& instance_id) const;

    /**
     * @brief Build Request Body.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    static nlohmann::json buildRequestBody(const InferenceRequest& request);

    /**
     * @brief Parse Response.
     * @param[in] data Input parameter.
     * @param[in] instance_id Identifier of the instance.
     * @return Return value.
     */
    static InferenceResponse parseResponse(const nlohmann::json& data,
                                           const std::string&    instance_id);
};

} // namespace themis::llm
