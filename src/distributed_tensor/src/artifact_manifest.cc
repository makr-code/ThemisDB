/**
 * @file artifact_manifest.cc
 * @brief Manifest store implementation stub.
 *
 * Skeleton: in-memory manifest with snapshot support.
 * Replace with distributed consensus store in sub-issue #5430.
 */

#include "distributed_tensor/include/artifact_manifest.h"

namespace themis::distributed_tensor {

namespace {

class ManifestStoreImpl final : public IManifestStore {
public:
    bool commit(ManifestEntry entry) override {
        entry.committed_at = std::chrono::system_clock::now();
        entries_[entry.artifact_id] = std::move(entry);
        return true;
    }

    std::optional<ManifestEntry> lookup(
        const std::string& artifact_id) const override {
        auto it = entries_.find(artifact_id);
        if (it == entries_.end()) return std::nullopt;
        return it->second;
    }

    bool reportPartialLoss(
        const std::string& artifact_id,
        const std::vector<std::string>& /*missing_shard_keys*/) override {
        auto it = entries_.find(artifact_id);
        if (it == entries_.end()) return false;
        it->second.status = ManifestEntryStatus::PartialLoss;
        return true;
    }

    bool updateStatus(const std::string& artifact_id,
                       ManifestEntryStatus status) override {
        auto it = entries_.find(artifact_id);
        if (it == entries_.end()) return false;
        it->second.status = status;
        return true;
    }

    ManifestSnapshot snapshot() const override {
        ManifestSnapshot snap;
        snap.snapshot_id = "snap-" + std::to_string(++epoch_);
        snap.epoch       = epoch_;
        snap.taken_at    = std::chrono::system_clock::now();
        for (const auto& [_, e] : entries_) snap.entries.push_back(e);
        return snap;
    }

    bool restore(const ManifestSnapshot& snap) override {
        entries_.clear();
        for (const auto& e : snap.entries) entries_[e.artifact_id] = e;
        epoch_ = snap.epoch;
        return true;
    }

    std::vector<std::string> listByStatus(
        ManifestEntryStatus status) const override {
        std::vector<std::string> ids;
        for (const auto& [id, e] : entries_)
            if (e.status == status) ids.push_back(id);
        return ids;
    }

private:
    std::unordered_map<std::string, ManifestEntry> entries_;
    mutable std::uint64_t epoch_ = 0;
};

} // namespace

std::unique_ptr<IManifestStore> makeManifestStore() {
    return std::make_unique<ManifestStoreImpl>();
}

} // namespace themis::distributed_tensor
