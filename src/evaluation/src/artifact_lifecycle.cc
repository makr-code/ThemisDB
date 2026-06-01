/**
 * @file artifact_lifecycle.cc
 * @brief Artifact lifecycle manager implementation stub.
 *
 * Skeleton: in-memory freshness tracking with TTL-based staleness.
 * Replace with persistent store and rebuild orchestration in sub-issue #5442.
 */

#include "evaluation/include/artifact_lifecycle.h"

namespace themis::evaluation {

namespace {

class ArtifactLifecycleImpl final : public IArtifactLifecycle {
public:
    void upsert(ArtifactRecord record) override {
        records_[record.id] = std::move(record);
    }

    std::optional<ArtifactRecord> lookup(const std::string& id) const override {
        auto it = records_.find(id);
        if (it == records_.end()) return std::nullopt;
        return it->second;
    }

    ArtifactFreshness evaluate(const std::string& id) const override {
        auto it = records_.find(id);
        if (it == records_.end()) return ArtifactFreshness::Missing;

        const auto& rec = it->second;
        auto now = std::chrono::system_clock::now();

        if (rec.freshness == ArtifactFreshness::Stale ||
            rec.freshness == ArtifactFreshness::Expired) {
            return rec.freshness;
        }
        if (now > rec.expires_at) return ArtifactFreshness::Expired;
        return ArtifactFreshness::Fresh;
    }

    void invalidate(const std::string& id, StalenessTrigger trigger) override {
        auto it = records_.find(id);
        if (it == records_.end()) return;
        it->second.freshness = ArtifactFreshness::Stale;
        it->second.staleness_trigger = trigger;
    }

    bool triggerRebuild(const std::string& id) override {
        auto it = records_.find(id);
        if (it == records_.end()) return false;
        if (rebuild_cb_) rebuild_cb_(it->second);
        return true;
    }

    void registerPolicy(ArtifactPolicy policy) override {
        policies_[policy.artifact_type] = std::move(policy);
    }

    void onRebuild(RebuildCallback cb) override {
        rebuild_cb_ = std::move(cb);
    }

private:
    std::unordered_map<std::string, ArtifactRecord>  records_;
    std::unordered_map<std::string, ArtifactPolicy>  policies_;
    RebuildCallback rebuild_cb_;
};

} // namespace

std::unique_ptr<IArtifactLifecycle> makeArtifactLifecycle() {
    return std::make_unique<ArtifactLifecycleImpl>();
}

} // namespace themis::evaluation
