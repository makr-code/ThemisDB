/**
 * @file hardware_profile.h
 * @brief Hardware profile definitions for deployment sizing decisions.
 *
 * Describes the hardware envelope (memory, storage, accelerators) of a
 * ThemisDB deployment tier and drives routing, compression, and index
 * selection choices throughout the retrieval and evaluation stacks.
 *
 * Planned in: docs/EPIC2_HARDWARE_PROFILES.md
 * Sub-issue:   #5437
 */

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::evaluation {

/// GPU/NPU accelerator family.
enum class AcceleratorFamily {
    None,
    NvidiaAmpere,
    NvidiaHopper,
    AmdRDNA,
    AppleANE,
    IntelArc,
    Custom,
};

/// Storage tier available to an index shard.
enum class StorageTier {
    DRAM,     ///< In-memory (latency < 1 µs)
    NVMeSSD,  ///< Local NVMe (latency < 100 µs)
    NetworkSSD, ///< EBS / remote block (latency < 1 ms)
    ObjectStore, ///< S3 / GCS (latency < 100 ms)
};

/// A concrete hardware profile snapshot.
struct HardwareProfile {
    std::string  id;                ///< e.g. "gpu-a100-80gb-nvme-4tb"
    std::string  display_name;

    // Memory
    std::uint64_t dram_bytes     = 0;
    std::uint64_t vram_bytes     = 0;

    // Storage
    StorageTier  primary_storage = StorageTier::NVMeSSD;
    std::uint64_t storage_bytes  = 0;

    // Accelerator
    AcceleratorFamily accelerator = AcceleratorFamily::None;
    std::uint32_t     accelerator_count = 0;

    // Network
    std::uint64_t network_bandwidth_bps = 0; ///< Egress bandwidth

    // Derived capacity hints (populated by the sizing engine)
    std::uint64_t max_hnsw_vectors  = 0; ///< Estimated max in-memory HNSW
    std::uint64_t max_diskann_vectors = 0;
    bool          supports_hot_path   = false;
};

/**
 * @brief Hardware profile registry and sizing engine.
 *
 * Manages a catalogue of hardware profiles, resolves the profile for the
 * current node, and exposes sizing recommendations.
 */
class IHardwareProfileRegistry {
public:
    virtual ~IHardwareProfileRegistry() = default;

    /// Detect the profile of the local machine.
    virtual HardwareProfile detectLocal() const = 0;

    /// Look up a named profile.
    virtual std::optional<HardwareProfile> lookup(const std::string& id) const = 0;

    /// Register a custom profile.
    virtual void registerProfile(HardwareProfile profile) = 0;

    /// Return all known profile IDs.
    virtual std::vector<std::string> listProfiles() const = 0;

    /// Recommend the best ANN backend for a given profile and dataset size.
    virtual std::string recommendAnnBackend(const HardwareProfile& p,
                                             std::uint64_t dataset_vectors) const = 0;
};

/// Factory: create a hardware profile registry pre-populated with stock profiles.
std::unique_ptr<IHardwareProfileRegistry> makeHardwareProfileRegistry();

} // namespace themis::evaluation
