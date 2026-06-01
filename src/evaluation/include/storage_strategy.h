/**
 * @file storage_strategy.h
 * @brief Storage strategy — quantization, mmap, and zero-copy loader policy.
 *
 * Provides the policy interface for choosing how tensor and adapter artifacts
 * are stored and loaded: full-precision, quantized, memory-mapped, or
 * zero-copy streamed.
 *
 * Planned in: src/evaluation/README.md (sub-issue 2.7 / #5443)
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::evaluation {

/// Supported storage and loading strategies.
enum class StorageMode {
    FullPrecision, ///< float32, no compression
    Quantized,     ///< int8 / int4 weights on disk
    Mmap,          ///< Memory-mapped file; zero-copy reads
    MmapQuantized, ///< Mmap with on-the-fly dequantisation
    Stream,        ///< Network-streamed; not resident in DRAM
};

/// Quantisation scheme for on-disk weights.
enum class QuantScheme {
    None,
    Int8,
    Int4,
    FP8,
    NF4, ///< Normalised float-4 (QLoRA style)
};

/// Descriptor for a stored artifact's storage parameters.
struct StorageDescriptor {
    std::string  artifact_id;
    StorageMode  mode  = StorageMode::FullPrecision;
    QuantScheme  quant = QuantScheme::None;
    std::string  path_or_uri;
    std::uint64_t size_bytes = 0;
    bool         pinned_in_memory = false; ///< Whether to lock pages
};

/// Recommendation produced by the storage strategy evaluator.
struct StorageRecommendation {
    StorageMode  recommended_mode;
    QuantScheme  recommended_quant;
    std::string  rationale;
    double       estimated_load_latency_ms = 0.0;
    double       estimated_memory_mb       = 0.0;
};

/**
 * @brief Storage strategy evaluator and loader interface.
 */
class IStorageStrategy {
public:
    virtual ~IStorageStrategy() = default;

    /// Recommend a storage mode for the given artifact size and hardware.
    virtual StorageRecommendation recommend(std::uint64_t artifact_bytes,
                                             std::uint64_t available_dram_bytes,
                                             bool has_nvme) const = 0;

    /// Load an artifact according to its descriptor into a byte buffer.
    virtual std::vector<std::uint8_t> load(const StorageDescriptor& desc) = 0;

    /// Memory-map a file and return a read-only view (zero-copy).
    /// Returns an empty vector if mmap is not supported on this platform.
    virtual std::vector<std::uint8_t> mmap(const std::string& path,
                                            std::uint64_t offset,
                                            std::uint64_t length) = 0;

    /// Release a previously mmap'd region.
    virtual void munmap(const std::string& path) = 0;

    /// Register a progress callback for streaming loads.
    using ProgressCallback = std::function<void(std::uint64_t loaded,
                                                 std::uint64_t total)>;
    virtual void onProgress(ProgressCallback cb) = 0;
};

/// Factory: create a storage strategy instance.
std::unique_ptr<IStorageStrategy> makeStorageStrategy();

} // namespace themis::evaluation
