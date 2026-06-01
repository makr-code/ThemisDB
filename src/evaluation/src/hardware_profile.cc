/**
 * @file hardware_profile.cc
 * @brief Hardware profile registry implementation stub.
 *
 * Skeleton: local-detection heuristic and stock profile catalogue.
 * Replace with production probe logic in sub-issue #5437.
 */

#include "evaluation/include/hardware_profile.h"

namespace themis::evaluation {

namespace {

class HardwareProfileRegistryImpl final : public IHardwareProfileRegistry {
public:
    HardwareProfileRegistryImpl() {
        // Pre-populate stock profiles (placeholder values).
        registerProfile(HardwareProfile{
            .id = "cpu-only-16gb",
            .display_name = "CPU-only / 16 GiB DRAM",
            .dram_bytes = 16ULL << 30,
            .primary_storage = StorageTier::NVMeSSD,
            .storage_bytes   = 500ULL << 30,
            .accelerator = AcceleratorFamily::None,
        });
        registerProfile(HardwareProfile{
            .id = "gpu-a100-80gb-nvme-4tb",
            .display_name = "NVIDIA A100 80 GiB + 4 TiB NVMe",
            .dram_bytes  = 512ULL << 30,
            .vram_bytes  = 80ULL << 30,
            .primary_storage = StorageTier::NVMeSSD,
            .storage_bytes   = 4ULL << 40,
            .accelerator = AcceleratorFamily::NvidiaAmpere,
            .accelerator_count = 1,
            .supports_hot_path = true,
        });
    }

    HardwareProfile detectLocal() const override {
        // TODO(#5437): Probe /proc/meminfo, nvidia-smi, lspci etc.
        return lookup("cpu-only-16gb").value_or(HardwareProfile{});
    }

    std::optional<HardwareProfile> lookup(const std::string& id) const override {
        auto it = profiles_.find(id);
        if (it == profiles_.end()) return std::nullopt;
        return it->second;
    }

    void registerProfile(HardwareProfile profile) override {
        profiles_[profile.id] = std::move(profile);
    }

    std::vector<std::string> listProfiles() const override {
        std::vector<std::string> ids;
        ids.reserve(profiles_.size());
        for (const auto& [id, _] : profiles_) ids.push_back(id);
        return ids;
    }

    std::string recommendAnnBackend(const HardwareProfile& p,
                                     std::uint64_t /*dataset_vectors*/) const override {
        return p.supports_hot_path ? "hnsw" : "diskann";
    }

private:
    std::unordered_map<std::string, HardwareProfile> profiles_;
};

} // namespace

std::unique_ptr<IHardwareProfileRegistry> makeHardwareProfileRegistry() {
    return std::make_unique<HardwareProfileRegistryImpl>();
}

} // namespace themis::evaluation
