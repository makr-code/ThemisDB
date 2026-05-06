/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_network_storage_engine.h                    ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor_network_storage_engine.h
 * @brief RocksDB-backed storage for Tensor-Train compressed tensors.
 *
 * `TensorNetworkStorageEngine` stores multi-dimensional tensors as TT-chains
 * (optionally quantised) in RocksDB using a structured key schema.  It
 * provides a transparent compression/decompression layer so that callers
 * interact only with raw `std::vector<float>` / mode-size pairs.
 *
 * ### Key schema
 * ```
 * __ttn__:<tenant>:<collection>:<field>:meta:<version>   → QuantizedTrain header
 * __ttn__:<tenant>:<collection>:<field>:G<k>:<version>   → QuantizedCore k bytes
 * ```
 * The `__ttn__` prefix namespaces tensor-network keys away from regular data.
 *
 * ### Versioning
 * Each `put()` call increments a monotonic version counter.  Past versions
 * can be retrieved via `getVersion()`.  `compact()` removes all versions
 * below a retention threshold.
 *
 * ### StorageLayoutAdvisor integration
 * The advisor recognises `LayoutType::TENSOR_NETWORK` as a new layout hint.
 * Collections with high float64 aggregation patterns and low point-lookup
 * ratios are candidates for TT compression.
 *
 * ### Thread safety
 * All public methods are protected by an internal `std::shared_mutex`:
 * - `get()` / `getVersion()` use shared (read) locks.
 * - `put()` / `remove()` / `compact()` use exclusive (write) locks.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"
#include "storage/tt_quantizer.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Forward-declare RocksDBWrapper so callers that only use InMemoryTensorBackend
// do not pull in the heavy RocksDB headers.
namespace themis { class RocksDBWrapper; }

namespace themis {
namespace storage {

// ============================================================================
// TensorFieldKey — logical identifier for one stored tensor field
// ============================================================================

/**
 * @brief Logical address of a tensor stored in the engine.
 */
struct TensorFieldKey {
    std::string tenant;
    std::string collection;
    std::string field;

    bool operator==(const TensorFieldKey& o) const noexcept {
        return tenant == o.tenant && collection == o.collection && field == o.field;
    }
};

struct TensorFieldKeyHash {
    std::size_t operator()(const TensorFieldKey& k) const noexcept;
};

// ============================================================================
// TensorStorageConfig
// ============================================================================

/**
 * @brief Configuration for TensorNetworkStorageEngine.
 */
struct TensorStorageConfig {
    /// TT-decomposition parameters applied on `put()`.
    TensorTrainConfig tt_config;

    /// Quantisation type applied after TT-decomposition.
    QuantizationType quant_type = QuantizationType::INT8;

    /// Number of old versions to retain per field (0 = keep all).
    std::size_t version_retention = 3;

    /**
     * @brief Minimum compression ratio to actually store in TT format.
     *
     * When the achieved ratio < min_compression_ratio the engine falls back
     * to storing the raw float32 data (QuantizationType::NONE, rank = full).
     *
     * Default: 2.0 (must compress at least 2×).
     */
    double min_compression_ratio = 2.0;
};

// ============================================================================
// TensorStorageStats
// ============================================================================

/**
 * @brief Per-field storage statistics.
 */
struct TensorStorageStats {
    std::size_t current_version   = 0;
    double      compression_ratio = 1.0;
    double      achieved_eps      = 0.0;
    std::size_t tt_max_rank       = 0;
    std::size_t compressed_bytes  = 0;
    std::size_t dense_elements    = 0;
    std::string quant_type;
};

// ============================================================================
// ITensorStorageBackend — abstraction for RocksDB / in-memory (testing)
// ============================================================================

/**
 * @brief Minimal KV-store interface used by TensorNetworkStorageEngine.
 *
 * Concrete implementations: `RocksDBTensorBackend` (production) and
 * `InMemoryTensorBackend` (unit tests).
 */
class ITensorStorageBackend {
public:
    virtual ~ITensorStorageBackend() = default;

    virtual bool put(const std::string& key,
                     const std::vector<uint8_t>& value) = 0;

    virtual std::optional<std::vector<uint8_t>>
    get(const std::string& key) const = 0;

    virtual bool del(const std::string& key) = 0;

    /// Iterate over all keys with the given prefix.
    virtual std::vector<std::string>
    listKeys(const std::string& prefix) const = 0;
};

// ============================================================================
// InMemoryTensorBackend — testing implementation
// ============================================================================

class InMemoryTensorBackend final : public ITensorStorageBackend {
public:
    bool put(const std::string& key,
             const std::vector<uint8_t>& value) override;

    std::optional<std::vector<uint8_t>>
    get(const std::string& key) const override;

    bool del(const std::string& key) override;

    std::vector<std::string>
    listKeys(const std::string& prefix) const override;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<uint8_t>> store_;
};

// ============================================================================
// RocksDBTensorBackend — production implementation backed by RocksDBWrapper
// ============================================================================

/**
 * @brief Production `ITensorStorageBackend` backed by `RocksDBWrapper`.
 *
 * Routes every KV operation to the shared `RocksDBWrapper` instance.  The
 * key namespace used by TensorNetworkStorageEngine (`__ttn__:…`) and by
 * TensorCoreStorageBridge (`__ttcore__:…`) is already distinct from all other
 * RocksDB keys so no additional column-family isolation is required.
 *
 * ### Thread safety
 * Delegated to `RocksDBWrapper` which is internally thread-safe for concurrent
 * reads and writes.
 *
 * ### Stub resolution
 * - STUB #148: adalora_tt_bridge store()/loadAdapter() now durable via RocksDB
 * - STUB #160: TensorCoreStorageBridge now durable across process restarts
 */
class RocksDBTensorBackend final : public ITensorStorageBackend {
public:
    /**
     * @brief Construct with a shared RocksDBWrapper.
     * @throws std::invalid_argument when db is null.
     */
    explicit RocksDBTensorBackend(std::shared_ptr<RocksDBWrapper> db);

    bool put(const std::string& key,
             const std::vector<uint8_t>& value) override;

    std::optional<std::vector<uint8_t>>
    get(const std::string& key) const override;

    bool del(const std::string& key) override;

    /// Returns all keys that start with @p prefix (sorted lexicographically).
    std::vector<std::string>
    listKeys(const std::string& prefix) const override;

private:
    std::shared_ptr<RocksDBWrapper> db_;
};

// ============================================================================
// TensorNetworkStorageEngine
// ============================================================================

/**
 * @brief Storage engine for TT-compressed tensors.
 *
 * ### Usage
 * @code
 * TensorStorageConfig cfg;
 * cfg.tt_config.eps  = 0.01;
 * cfg.quant_type = QuantizationType::NF4;
 *
 * auto backend = std::make_shared<InMemoryTensorBackend>();
 * TensorNetworkStorageEngine engine(backend, cfg);
 *
 * // Store a 4×4×4×4 attention matrix
 * std::vector<float> weights(256, 0.5f);
 * engine.put({"myorg", "models", "layer0_attn"}, weights, {4,4,4,4});
 *
 * // Retrieve (reconstructed)
 * auto result = engine.get({"myorg", "models", "layer0_attn"});
 * @endcode
 */
class TensorNetworkStorageEngine {
public:
    /**
     * @brief Construct with a storage backend and configuration.
     *
     * @param backend  Shared pointer to the KV-store backend.
     * @param cfg      Storage configuration.
     * @throws std::invalid_argument if backend is null.
     */
    explicit TensorNetworkStorageEngine(
        std::shared_ptr<ITensorStorageBackend> backend,
        const TensorStorageConfig& cfg = {});

    ~TensorNetworkStorageEngine() = default;

    // ─── Write ────────────────────────────────────────────────────────────

    /**
     * @brief Compress and store a tensor field.
     *
     * Decomposes `data` into TT format, quantises the cores, and persists
     * each core under the structured key schema.
     *
     * @param key        Logical field address.
     * @param data       Flat row-major float32 tensor.
     * @param mode_sizes Shape of the tensor (∏ mode_sizes == data.size()).
     * @return True on success.
     * @throws std::invalid_argument on shape mismatch.
     */
    bool put(const TensorFieldKey&            key,
             const std::vector<float>&        data,
             const std::vector<std::size_t>&  mode_sizes);

    // ─── Read ─────────────────────────────────────────────────────────────

    /**
     * @brief Retrieve and reconstruct the latest version of a tensor field.
     *
     * @return Reconstructed float32 tensor, or std::nullopt if not found.
     */
    std::optional<std::vector<float>>
    get(const TensorFieldKey& key) const;

    /**
     * @brief Retrieve a specific version.
     */
    std::optional<std::vector<float>>
    getVersion(const TensorFieldKey& key, std::size_t version) const;

    /**
     * @brief Retrieve in compressed (QuantizedTrain) form — avoids decompression.
     */
    std::optional<QuantizedTrain>
    getCompressed(const TensorFieldKey& key) const;

    // ─── Delete / compact ─────────────────────────────────────────────────

    /**
     * @brief Remove the latest version of a field.
     */
    bool remove(const TensorFieldKey& key);

    /**
     * @brief Delete all versions below `keep_versions` newest ones.
     */
    void compact(const TensorFieldKey& key);

    // ─── Metadata ─────────────────────────────────────────────────────────

    /**
     * @brief Statistics for the latest version of a field.
     *
     * @return std::nullopt if field not found.
     */
    std::optional<TensorStorageStats>
    stats(const TensorFieldKey& key) const;

    /**
     * @brief Configuration used by this engine.
     */
    const TensorStorageConfig& config() const noexcept { return cfg_; }

private:
    std::shared_ptr<ITensorStorageBackend> backend_;
    TensorStorageConfig  cfg_;
    TensorTrainDecomposer decomposer_;
    TTQuantizer          quantizer_;

    mutable std::shared_mutex rw_mutex_;

    // ─── Internal key building ────────────────────────────────────────────

    static std::string makeMetaKey(const TensorFieldKey& k, std::size_t ver);
    static std::string makeCoreKey(const TensorFieldKey& k,
                                   std::size_t core_idx, std::size_t ver);
    static std::string makePrefix(const TensorFieldKey& k);

    // ─── Version tracking (in-memory cache for speed) ─────────────────────

    mutable std::unordered_map<TensorFieldKey,
                               std::size_t,
                               TensorFieldKeyHash> version_cache_;

    std::size_t currentVersion(const TensorFieldKey& k) const;
    void incrementVersion(const TensorFieldKey& k);

    // ─── Persistence helpers ──────────────────────────────────────────────

    bool persistQuantizedTrain(const TensorFieldKey& key,
                               const QuantizedTrain& qtrain,
                               std::size_t version);

    std::optional<QuantizedTrain>
    loadQuantizedTrain(const TensorFieldKey& key, std::size_t version) const;
};

} // namespace storage
} // namespace themis
