/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_compaction_filter.h                         ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-06                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Phase 2 (Q4 2026)                                        ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor_compaction_filter.h
 * @brief RocksDB compaction filter that re-compresses stored TT-core values.
 *
 * `TensorCompactionFilter` integrates with the RocksDB background compaction
 * pipeline to automatically reduce TT-core bond dimensions when a tighter
 * reconstruction error is acceptable.  It targets two key namespaces:
 *
 * - `__ttcore__:<tenant>:<file_id>:<chunk_id>` — raw `TTTrain::serialize()`
 *   bytes stored by `TensorCoreStorageBridge`.
 * - `__ttn__:<tenant>:<collection>:&lt;field&gt;:meta:<ver>` — `QuantizedTrain`
 *   header bytes stored by `TensorNetworkStorageEngine`.
 *
 * For each matching key the filter:
 *  1. Deserialises the TT-core value.
 *  2. Calls `TensorTrainDecomposer::recompress()` with the configured ε.
 *  3. If the new train is smaller (fewer total parameters), serialises and
 *     replaces the value; otherwise keeps the original unchanged.
 *
 * ### Design constraints
 * - Opt-in: disabled by default.  Register on a column family via
 *   `rocksdb::ColumnFamilyOptions::compaction_filter`.
 * - Copy-on-success: original data is preserved on any deserialization error.
 * - Never increases rank: the filter only reduces or maintains bond dimensions.
 * - Thread-safe: `TensorTrainDecomposer` and `TTQuantizer` are stateless.
 *
 * ### STUB/SIMULATION NOTE
 * Purpose: background rank-reduction during RocksDB compaction.
 * Activation: register filter on CF options; gated by `THEMIS_ENABLE_TENSOR_COMPACTION`.
 * Production Delta: `recompress()` uses heuristic rank truncation until
 *   LAPACK SVD (THEMIS_USE_LAPACK_SVD) is available for optimal rounding.
 * Removal Plan: No removal planned — permanent component.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"
#include "storage/tt_quantizer.h"

#include <rocksdb/compaction_filter.h>
#include <string>

namespace themis {
namespace storage {

// ============================================================================
// TensorCompactionFilter
// ============================================================================

/**
 * @brief RocksDB compaction filter for TT-core re-compression.
 *
 * Targets `__ttcore__:` (raw TTTrain) and `__ttn__:...:meta:` (QuantizedTrain)
 * key namespaces.
 *
 * ### Usage
 * @code
 * rocksdb::ColumnFamilyOptions cf_opts;
 * cf_opts.compaction_filter =
 *     new TensorCompactionFilter(1e-4, QuantizationType::INT8);
 * @endcode
 */
class TensorCompactionFilter final : public rocksdb::CompactionFilter {
public:
    /**
     * @brief Construct filter with compression parameters.
     *
     * @param epsilon    Reconstruction error tolerance ε ∈ (0, 1].
     *                   Values below the original tensor ε will have no effect.
     *                   Default: 1e-4 (tight but lossless for most embeddings).
     * @param quant_type Quantisation type to apply when re-serialising
     *                   QuantizedTrain values (has no effect on raw TTTrain keys).
     *                   Default: INT8.
     */
    explicit TensorCompactionFilter(
        double         epsilon   = 1e-4,
        QuantizationType quant_type = QuantizationType::INT8) noexcept;

    ~TensorCompactionFilter() override = default;

    const char* Name() const override { return "TensorCompactionFilter"; }

    /**
     * @brief Compaction callback: inspect one key–value pair.
     *
     * Returns:
     * - `kKeep`        — key is not a TT-core key, or deserialization failed,
     *                    or recompression did not reduce size.
     * - `kChangeValue` — recompressed value is smaller; `*new_value` is set.
     */
    Decision FilterV2(int                       level,
                      const rocksdb::Slice&     key,
                      ValueType                 value_type,
                      const rocksdb::Slice&     existing_value,
                      std::string*              new_value,
                      std::string*              skip_until) const override;

private:
    double           epsilon_;
    QuantizationType quant_type_;
    TensorTrainDecomposer decomposer_;
    TTQuantizer           quantizer_;

    // --- Key classification helpers -----------------------------------------

    /// Returns true for `__ttcore__:` prefix (raw TTTrain values).
    static bool isTTCoreKey(const rocksdb::Slice& key) noexcept;

    /// Returns true for `__ttn__:...:meta:` suffix (QuantizedTrain headers).
    static bool isTTNMetaKey(const rocksdb::Slice& key) noexcept;

    // --- Per-format handlers ------------------------------------------------

    /// Process a raw TTTrain value.  Returns true and sets *new_bytes if
    /// recompression produced a smaller result.
    bool filterTTCore(const rocksdb::Slice& value,
                      std::string*          new_bytes) const;

    /// Process a QuantizedTrain meta value.  Returns true and sets *new_bytes
    /// if recompression produced a smaller result.
    bool filterTTNMeta(const rocksdb::Slice& value,
                       std::string*          new_bytes) const;
};

} // namespace storage
} // namespace themis
