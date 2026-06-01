/**
 * @file federated_summaries.cc
 * @brief Federated Summaries implementation stub.
 *
 * Skeleton: factory and minimal concrete class.  Replace with production
 * cross-shard fan-out and merging logic in sub-issue #5427.
 */

#include "retrieval/include/federated_summaries.h"

namespace themis::retrieval {

namespace {

class FederatedSummariesImpl final : public IFederatedSummaries {
public:
    explicit FederatedSummariesImpl(FederatedSummariesConfig cfg)
        : cfg_(std::move(cfg)) {}

    FederatedResult query(const FederatedQuery& /*fq*/) override {
        // TODO(#5427): Implement cross-shard fan-out and summary merging.
        FederatedResult result;
        for (const auto& [key, _] : shards_) {
            result.shard_availability[key] = false; // Stub: not yet connected
        }
        return result;
    }

    void registerShard(const std::string& key,
                        const std::string& endpoint) override {
        shards_[key] = endpoint;
    }

    void deregisterShard(const std::string& key) override {
        shards_.erase(key);
    }

    std::unordered_map<std::string, bool> shardHealth() const override {
        std::unordered_map<std::string, bool> health;
        for (const auto& [key, _] : shards_) {
            health[key] = false; // TODO(#5427): Probe each shard.
        }
        return health;
    }

private:
    FederatedSummariesConfig cfg_;
    std::unordered_map<std::string, std::string> shards_; // key → endpoint
};

} // namespace

std::unique_ptr<IFederatedSummaries> makeFederatedSummaries(
    const FederatedSummariesConfig& cfg) {
    return std::make_unique<FederatedSummariesImpl>(cfg);
}

} // namespace themis::retrieval
