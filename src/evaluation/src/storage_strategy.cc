/**
 * @file storage_strategy.cc
 * @brief Storage strategy implementation stub.
 *
 * Skeleton: recommendation heuristic and no-op loader.
 * Replace with mmap/zero-copy loader and quant engine in sub-issue #5443.
 */

#include "evaluation/include/storage_strategy.h"

#include <stdexcept>

namespace themis::evaluation {

namespace {

class StorageStrategyImpl final : public IStorageStrategy {
public:
    StorageRecommendation recommend(std::uint64_t artifact_bytes,
                                     std::uint64_t available_dram_bytes,
                                     bool has_nvme) const override {
        // Simple heuristic: prefer mmap if artifact > 25 % of DRAM.
        if (artifact_bytes > available_dram_bytes / 4) {
            return {
                .recommended_mode  = has_nvme ? StorageMode::Mmap
                                              : StorageMode::Stream,
                .recommended_quant = QuantScheme::Int8,
                .rationale = "Artifact too large for full DRAM residency",
                .estimated_memory_mb = static_cast<double>(artifact_bytes >> 20) * 0.5,
            };
        }
        return {
            .recommended_mode  = StorageMode::FullPrecision,
            .recommended_quant = QuantScheme::None,
            .rationale = "Artifact fits comfortably in DRAM",
            .estimated_memory_mb = static_cast<double>(artifact_bytes >> 20),
        };
    }

    std::vector<std::uint8_t> load(const StorageDescriptor& /*desc*/) override {
        // TODO(#5443): Implement file and network loaders.
        return {};
    }

    std::vector<std::uint8_t> mmap(const std::string& /*path*/,
                                    std::uint64_t /*offset*/,
                                    std::uint64_t /*length*/) override {
        // TODO(#5443): Implement POSIX mmap.
        return {};
    }

    void munmap(const std::string& /*path*/) override {
        // TODO(#5443): Release mmap region.
    }

    void onProgress(ProgressCallback cb) override {
        progress_cb_ = std::move(cb);
    }

private:
    ProgressCallback progress_cb_;
};

} // namespace

std::unique_ptr<IStorageStrategy> makeStorageStrategy() {
    return std::make_unique<StorageStrategyImpl>();
}

} // namespace themis::evaluation
