/**
 * @file tensor_artifact_classes.cc
 * @brief Tensor artifact class registry implementation stub.
 *
 * Skeleton: in-memory registry with lifecycle transitions.
 * Replace with durable store in sub-issue #5429.
 */

#include "distributed_tensor/include/tensor_artifact_classes.h"

namespace themis::distributed_tensor {

namespace {

class ArtifactClassRegistryImpl final : public IArtifactClassRegistry {
public:
    std::string registerRaw(RawTensorArtifact artifact) override {
        std::string id = artifact.meta.id;
        artifact.meta.state = ArtifactState::Active;
        metadata_[id] = artifact.meta;
        return id;
    }

    std::string registerSharded(ShardedArtifact artifact) override {
        std::string id = artifact.meta.id;
        artifact.meta.state = ArtifactState::Active;
        metadata_[id] = artifact.meta;
        return id;
    }

    std::optional<ArtifactMetadata> lookupMetadata(
        const std::string& id) const override {
        auto it = metadata_.find(id);
        if (it == metadata_.end()) return std::nullopt;
        return it->second;
    }

    bool transitionState(const std::string& id,
                          ArtifactState target) override {
        auto it = metadata_.find(id);
        if (it == metadata_.end()) return false;
        it->second.state = target;
        return true;
    }

    std::vector<std::string> listByState(
        ArtifactState state) const override {
        std::vector<std::string> result;
        for (const auto& [id, meta] : metadata_)
            if (meta.state == state) result.push_back(id);
        return result;
    }

private:
    std::unordered_map<std::string, ArtifactMetadata> metadata_;
};

} // namespace

std::unique_ptr<IArtifactClassRegistry> makeArtifactClassRegistry() {
    return std::make_unique<ArtifactClassRegistryImpl>();
}

} // namespace themis::distributed_tensor
