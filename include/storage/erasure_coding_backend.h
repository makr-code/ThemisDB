/**
 * @file erasure_coding_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Erasure Coding Backend for Blob Storage
 *
 * Implements space-efficient redundancy using Reed-Solomon erasure codes.
 * Supports RS(10,4), RS(6,3), RS(4,2) and arbitrary (k,m) configurations.
 *
 * Benefits over mirroring:
 *   - RS(10,4): 40% overhead vs 200% for 2× mirroring
 *   - RS(6,3):  50% overhead, tolerates 3 simultaneous node failures
 *   - RS(4,2):  50% overhead, fast encode/decode for smaller blobs
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "sharding/redundancy_strategy.h"
#include "storage/blob_redundancy_manager.h"

namespace themisdb {
namespace storage {

/**
 * Encoded blob: the individual shards produced by ErasureCodingBackend::encode().
 *
 * Each shard carries its absolute index within the stripe (0 … k+m-1) so that
 * the decoder can reconstruct the original data even when only a subset of
 * shards is available.
 */
struct EncodedShard {
    uint32_t  shard_index  = 0;      ///< Position in the stripe (0-based)
    bool      is_parity    = false;  ///< true for parity shards (index >= data_shards)
    uint64_t  original_size = 0;     ///< Original blob size (bytes) before padding
    std::vector<uint8_t> data;       ///< Shard payload
};

/**
 * ErasureCodingBackend
 *
 * Self-contained backend that encodes blobs into Reed-Solomon stripes and
 * reconstructs them from any k-of-(k+m) available shards.  Storage of the
 * encoded shards is handled by the caller; this class provides only the
 * encode/decode logic and an optional in-memory shard store for testing.
 *
 * Thread-safety: all public methods are thread-safe.
 *
 * Usage example (RS 10+4):
 * @code
 *   ErasureCodingConfig cfg;
 *   cfg.data_shards   = 10;
 *   cfg.parity_shards = 4;
 *
 *   ErasureCodingBackend backend(cfg);
 *   backend.put("blob-123", data);           // encode + store internally
 *   auto recovered = backend.get("blob-123"); // reconstruct from shards
 * @endcode
 */
class ErasureCodingBackend {
public:
    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * Construct with an explicit erasure coding configuration.
     * @throws std::invalid_argument if data_shards < 2 or parity_shards < 1.
     */
    explicit ErasureCodingBackend(const ErasureCodingConfig& config);

    ~ErasureCodingBackend();

    ErasureCodingBackend(const ErasureCodingBackend&)            = delete;
    ErasureCodingBackend& operator=(const ErasureCodingBackend&) = delete;

    ErasureCodingBackend(ErasureCodingBackend&&)            = delete;
    ErasureCodingBackend& operator=(ErasureCodingBackend&&) = delete;

    // -----------------------------------------------------------------------
    // Low-level encode / decode  (caller manages shard storage)
    // -----------------------------------------------------------------------

    /**
     * Encode @p data into (data_shards + parity_shards) EncodedShard objects.
     *
     * The original data is zero-padded to a multiple of data_shards before
     * splitting; each shard therefore has the same size.  The original byte
     * count is stored in EncodedShard::original_size so that trailing padding
     * can be stripped on decode.
     *
     * @param blob_id   Identifier used only for logging.
     * @param data      Raw blob bytes to encode.
     * @return          Vector of (data_shards + parity_shards) shards.
     */
    std::vector<EncodedShard> encode(
        const std::string&          blob_id,
        const std::vector<uint8_t>& data
    ) const;

    /**
     * Decode the original blob from a subset of shards.
     *
     * Reconstruction succeeds as long as at least @c data_shards shards are
     * available (i.e. up to @c parity_shards simultaneous failures are
     * tolerated).
     *
     * @param blob_id        Identifier used for logging / error messages.
     * @param shards         Map of shard_index → EncodedShard for every
     *                       available (non-erased) shard.
     * @param original_size  Expected original blob size in bytes.  If zero,
     *                       the value stored in the first available shard is
     *                       used.
     * @return               Reconstructed original data.
     * @throws std::runtime_error if fewer than data_shards shards are present.
     */
    std::vector<uint8_t> decode(
        const std::string&                         blob_id,
        const std::map<uint32_t, EncodedShard>&    shards,
        uint64_t                                   original_size = 0
    ) const;

    // -----------------------------------------------------------------------
    // High-level put / get  (uses internal in-memory shard store)
    // -----------------------------------------------------------------------

    /**
     * Encode @p data and store all shards in the internal shard store.
     *
     * Any previously stored shards for @p blob_id are replaced.
     */
    void put(const std::string& blob_id, const std::vector<uint8_t>& data);

    /**
     * Retrieve a blob from the internal shard store.
     *
     * Returns std::nullopt if the blob is unknown or if too many shards are
     * missing for reconstruction (more than parity_shards failures).
     */
    std::optional<std::vector<uint8_t>> get(const std::string& blob_id) const;

    /**
     * Remove all shards for @p blob_id from the internal shard store.
     */
    void remove(const std::string& blob_id);

    /**
     * Simulate a shard failure by removing one shard from the internal store.
     *
     * Intended for testing fault-tolerance scenarios.
     *
     * @param blob_id     Target blob.
     * @param shard_index Index of the shard to drop (0 … total_shards-1).
     * @return            true if the shard was present and removed.
     */
    bool dropShard(const std::string& blob_id, uint32_t shard_index);

    /**
     * Return the number of healthy (available) shards for @p blob_id in
     * the internal store, or 0 if the blob is not known.
     */
    uint32_t availableShardCount(const std::string& blob_id) const;

    // -----------------------------------------------------------------------
    // Configuration accessors
    // -----------------------------------------------------------------------

    const ErasureCodingConfig& config()      const noexcept { return config_; }
    uint32_t  dataShards()    const noexcept { return config_.data_shards;   }
    uint32_t  parityShards()  const noexcept { return config_.parity_shards; }
    uint32_t  totalShards()   const noexcept { return config_.totalShards(); }

    /**
     * Storage overhead factor relative to the raw data size.
     * e.g. RS(4,2) → 1.5×, RS(10,4) → 1.4×, RS(6,3) → 1.5×
     */
    double storageOverhead() const noexcept {
        return 1.0 / config_.storageEfficiency();
    }

    /**
     * Maximum number of simultaneous shard failures the configuration can
     * recover from (equals parity_shards).
     */
    uint32_t faultTolerance() const noexcept { return config_.parity_shards; }

    /**
     * Return true if the backend can still reconstruct a blob given that
     * @p failed_shards shards are unavailable.
     */
    bool canRecover(uint32_t failed_shards) const noexcept {
        return failed_shards <= config_.parity_shards;
    }

private:
    // Per-blob entry in the internal shard store
    struct BlobEntry {
        std::map<uint32_t, std::vector<uint8_t>> chunks; ///< chunk_index → data
        uint64_t original_size = 0;
    };

    ErasureCodingConfig                      config_;
    std::unique_ptr<themis::sharding::ErasureCoder> coder_;

    mutable std::mutex                       store_mutex_;
    std::map<std::string, BlobEntry>         store_;   ///< blob_id → BlobEntry
};

} // namespace storage
} // namespace themisdb
