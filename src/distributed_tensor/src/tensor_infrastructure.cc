/**
 * @file tensor_infrastructure.cc
 * @brief Node registry and stripe transport implementation stubs.
 *
 * Skeleton: in-memory registry and no-op transport.
 * Replace with gRPC-based transport in sub-issue #5435.
 */

#include "distributed_tensor/include/tensor_infrastructure.h"

namespace themis::distributed_tensor {

namespace {

class NodeRegistryImpl final : public INodeRegistry {
public:
    explicit NodeRegistryImpl(
        const std::vector<std::pair<std::string, std::string>>& endpoints) {
        for (const auto& [key, ep] : endpoints) registerNode(key, ep);
    }

    void registerNode(const std::string& shard_key,
                       const std::string& endpoint) override {
        endpoints_[shard_key] = endpoint;
        health_[shard_key]    = {.shard_key = shard_key,
                                  .health    = NodeHealth::Unknown};
    }

    void deregisterNode(const std::string& shard_key) override {
        endpoints_.erase(shard_key);
        health_.erase(shard_key);
    }

    std::optional<std::string> endpoint(
        const std::string& shard_key) const override {
        auto it = endpoints_.find(shard_key);
        if (it == endpoints_.end()) return std::nullopt;
        return it->second;
    }

    NodeHealthRecord healthOf(const std::string& shard_key) const override {
        auto it = health_.find(shard_key);
        if (it == health_.end()) return {.shard_key = shard_key,
                                         .health    = NodeHealth::Unknown};
        return it->second;
    }

    std::vector<std::string> healthyNodes(NodeHealth min_health) const override {
        std::vector<std::string> result;
        for (const auto& [key, rec] : health_) {
            if (static_cast<int>(rec.health) <=
                static_cast<int>(min_health))
                result.push_back(key);
        }
        return result;
    }

private:
    std::unordered_map<std::string, std::string>        endpoints_;
    std::unordered_map<std::string, NodeHealthRecord>   health_;
};

class StripeTransportImpl final : public IStripeTransport {
public:
    StripeTransportImpl(std::shared_ptr<INodeRegistry> registry,
                         TransportConfig cfg)
        : registry_(std::move(registry)), cfg_(std::move(cfg)) {}

    bool writeStripe(const std::string& /*shard_key*/,
                     const std::string& /*artifact_id*/,
                     std::uint32_t /*stripe_index*/,
                     const std::vector<std::uint8_t>& /*data*/) override {
        // TODO(#5435): Implement gRPC stripe write.
        return false;
    }

    std::vector<std::uint8_t> readStripe(
        const std::string& /*shard_key*/,
        const std::string& /*artifact_id*/,
        std::uint32_t /*stripe_index*/) override {
        // TODO(#5435): Implement gRPC stripe read.
        return {};
    }

    bool deleteStripe(const std::string& /*shard_key*/,
                       const std::string& /*artifact_id*/,
                       std::uint32_t /*stripe_index*/) override {
        // TODO(#5435): Implement gRPC stripe delete.
        return false;
    }

    void onHealthChange(HealthCallback cb) override {
        health_cb_ = std::move(cb);
    }

private:
    std::shared_ptr<INodeRegistry> registry_;
    TransportConfig                cfg_;
    HealthCallback                 health_cb_;
};

} // namespace

std::unique_ptr<INodeRegistry> makeNodeRegistry(
    const std::vector<std::pair<std::string, std::string>>& shard_endpoints) {
    return std::make_unique<NodeRegistryImpl>(shard_endpoints);
}

std::unique_ptr<IStripeTransport> makeStripeTransport(
    std::shared_ptr<INodeRegistry> registry, const TransportConfig& cfg) {
    return std::make_unique<StripeTransportImpl>(std::move(registry), cfg);
}

} // namespace themis::distributed_tensor
