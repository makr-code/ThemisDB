/**
 * @file tensor_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"  // TTTrain, TTCore
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// TensorSearchResult — result from a compressed-domain ANN query
// ============================================================================

struct TensorSearchResult {
    int64_t id;         ///< User-assigned vector ID
    float   distance;   ///< Approximate distance (smaller = closer)
    float   tt_norm;    ///< Frobenius norm of the stored TT-train
};

// ============================================================================
// TensorIndexStats — runtime diagnostics for cost-model and monitoring
// ============================================================================

struct TensorIndexStats {
    size_t  num_vectors      = 0;  ///< Total entries in the index
    size_t  dim              = 0;  ///< Logical vector dimension
    size_t  avg_tt_rank      = 0;  ///< Mean TT rank across all stored cores
    size_t  storage_bytes    = 0;  ///< Estimated bytes consumed by TT cores
    double  avg_compress_ratio = 0.0; ///< Average compression vs. float32 flat
    double  avg_search_ms    = 0.0;   ///< Rolling average search latency (ms)
    size_t  total_searches   = 0;     ///< Lifetime search count
};

// ============================================================================
// ITensorIndex — interface
// ============================================================================

/**
 * @brief Uniform interface for TT-based approximate-nearest-neighbour indexes.
 *
 * All implementations must be thread-safe for concurrent reads (search,
 * innerProduct, norm).  Writes (add, remove) must be serialised externally
 * or handled internally with a shared_mutex.
 */
class ITensorIndex {
public:
    virtual ~ITensorIndex() = default;

    // ------------------------------------------------------------------
    // Write path
    // ------------------------------------------------------------------

    /**
     * @brief Add a vector stored as a Tensor-Train to the index.
     *
     * @param id     User-assigned identifier (must be unique).
     * @param train  TT-compressed representation produced by TensorTrainDecomposer.
     * @return true on success; false if id already exists or train is invalid.
     */
    [[nodiscard]] virtual bool add(int64_t id,
                                   const storage::TTTrain& train) = 0;

    /**
     * @brief Add a flat float vector by compressing it internally.
     *
     * Convenience overload — calls TensorTrainDecomposer::decompose() with
     * the index's default rank / epsilon settings before delegating to
     * add(id, train).
     *
     * @param id     User-assigned identifier.
     * @param vector Pointer to dim float values.
     * @param dim    Vector dimension.
     * @return true on success.
     */
    [[nodiscard]] virtual bool addFlat(int64_t id,
                                       const float* vector,
                                       size_t dim) = 0;

    /**
     * @brief Remove a vector by id.  Returns false if not found.
     */
    virtual bool remove(int64_t id) = 0;

    // ------------------------------------------------------------------
    // Read path — all may be called concurrently
    // ------------------------------------------------------------------

    /**
     * @brief Approximate k-nearest-neighbour search in the compressed domain.
     *
     * Distance computation runs entirely on TT cores: O(d·r²) per candidate.
     * The caller does NOT need to decompress the stored vectors.
     *
     * @param query  TT-train of the query vector.
     * @param k      Number of results requested.
     * @return       Up to k results sorted by ascending distance.
     */
    virtual std::vector<TensorSearchResult> search(
        const storage::TTTrain& query, int k) const = 0;

    /**
     * @brief Flat-vector query convenience overload.
     *
     * Compresses @p query internally, then delegates to search(train, k).
     */
    virtual std::vector<TensorSearchResult> searchFlat(
        const float* query, size_t dim, int k) const = 0;

    /**
     * @brief Inner product of two stored TT-trains in compressed domain.
     *
     * Complexity: O(d · r³)  (TT-inner-product algorithm, Holtz 2012).
     *
     * @param id_a  ID of the first stored vector.
     * @param id_b  ID of the second stored vector.
     * @return      Inner product value, or std::nullopt if either ID not found.
     */
    virtual std::optional<float> innerProduct(int64_t id_a,
                                               int64_t id_b) const = 0;

    /**
     * @brief Frobenius norm of a stored TT-train, computed without reconstruction.
     *
     * Uses the identity ‖T‖_F = sqrt(<T,T>_TT) in O(d·r³).
     *
     * @param id  Vector ID.
     * @return    Norm value, or std::nullopt if id not found.
     */
    virtual std::optional<float> norm(int64_t id) const = 0;

    /**
     * @brief Retrieve the raw TT-train for an entry.
     *
     * @return Pointer to internal storage (valid until next mutation).
     *         Returns nullptr if not found.
     */
    virtual const storage::TTTrain* get(int64_t id) const = 0;

    // ------------------------------------------------------------------
    // Persistence
    // ------------------------------------------------------------------

    /**
     * @brief Persist the index to a RocksDB-backed storage path.
     *
     * Key schema: `__ttidx__:<index_name>:<id>:<core_k>`
     */
    virtual bool save(const std::string& path) const = 0;

    /**
     * @brief Load a previously persisted index.
     */
    virtual bool load(const std::string& path) = 0;

    // ------------------------------------------------------------------
    // Diagnostics
    // ------------------------------------------------------------------

    [[nodiscard]] virtual size_t          size()  const = 0;
    [[nodiscard]] virtual TensorIndexStats stats() const = 0;
};

} // namespace tensor
} // namespace themis

