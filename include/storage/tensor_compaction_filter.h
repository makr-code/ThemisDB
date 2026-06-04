/**
 * @file tensor_compaction_filter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"
#include "storage/tt_quantizer.h"

#include <functional>
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
     * @brief Injectable recompress backend for compaction.
     *
     * Signature: `TTTrain fn(const TTTrain&, const TensorTrainConfig&)`
     */
    using RecompressFn = std::function<TTTrain(const TTTrain&,
                                               const TensorTrainConfig&)>;

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

    /**
     * @brief Inject a recompress backend (e.g. LAPACK dgesdd wrapper).
     *
     * When set, filterTTCore()/filterTTNMeta() delegate recompression to this
     * callback instead of using TensorTrainDecomposer::recompress().
     */
    static void setRecompressFn(RecompressFn fn);

    /** @brief Remove a previously injected recompress backend. */
    static void clearRecompressFn();

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

