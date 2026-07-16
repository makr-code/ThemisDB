/**
 * @file vector_index_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace storage {

// ============================================================================
// DistanceMetric
// ============================================================================

/**
 * @brief Distance metric used by the vector index.
 */
enum class DistanceMetric : uint8_t {
    /// Euclidean distance (L2 norm squared).
    L2,
    /// Inner (dot) product.  For true cosine similarity pre-normalise vectors.
    DOT_PRODUCT,
    /// Cosine similarity: vectors are normalised internally.
    COSINE,
};

// ============================================================================
// VectorIndexConfig
// ============================================================================

/**
 * @brief Construction parameters for a vector index backend.
 */
struct VectorIndexConfig {
    /// Embedding dimensionality.  Must be > 0.
    std::size_t dim = 128;

    /// Distance metric to use for similarity search.
    DistanceMetric metric = DistanceMetric::L2;

    /// Maximum number of elements the index is expected to hold (hint).
    /// Backends may grow beyond this; it is used for pre-allocation.
    std::size_t max_elements = 100'000;

    /// HNSW-class parameter M — controls graph connectivity (default 16).
    /// Ignored by backends that do not use HNSW.
    std::size_t hnsw_M = 16;

    /// HNSW-class construction-time ef parameter (default 200).
    /// Higher values improve recall at the cost of build time.
    std::size_t hnsw_ef_construction = 200;
};

// ============================================================================
// KnnResult
// ============================================================================

/**
 * @brief Single element of a k-nearest-neighbour result set.
 */
struct KnnResult {
    /// Document / vector identifier (as supplied to `add()`).
    std::string id;

    /// Raw distance (metric-dependent; lower is more similar for L2).
    float distance = 0.0f;

    /// Normalised similarity score ∈ [0, 1] where 1 is most similar.
    /// For L2: score = 1 / (1 + distance).
    /// For DOT_PRODUCT / COSINE: score = (distance + 1) / 2 (clamped to [0,1]).
    float score = 0.0f;
};

// ============================================================================
// IVectorIndexBackend
// ============================================================================

/**
 * @brief Abstract ANN vector index backend interface.
 *
 * Implementations are expected to be thread-safe.
 */
class IVectorIndexBackend {
public:
    virtual ~IVectorIndexBackend() = default;

    /**
     * @brief Add (or replace) a vector in the index.
     *
     * If @p id already exists the entry is updated atomically.
     *
     * @param id       Unique document identifier.
     * @param embedding Embedding vector; length must equal `config().dim`.
     * @throws std::invalid_argument if `embedding.size() != config().dim`.
     */
    virtual void add(const std::string& id,
                     const std::vector<float>& embedding) = 0;

    /**
     * @brief Query the index for the k nearest neighbours.
     *
     * @param query Embedding vector; length must equal `config().dim`.
     * @param k     Number of nearest neighbours to return (≥ 1).
     * @return      Up to `min(k, size())` results, sorted by ascending distance.
     * @throws std::invalid_argument if `query.size() != config().dim`.
     */
    [[nodiscard]] virtual std::vector<KnnResult>
    search(const std::vector<float>& query, std::size_t k) const = 0;

    /**
     * @brief Remove a vector from the index.
     *
     * Silently ignores non-existent ids.
     *
     * @param id Document identifier to remove.
     */
    virtual void remove(const std::string& id) = 0;

    /**
     * @brief Number of vectors currently stored.
     */
    [[nodiscard]] virtual std::size_t size() const noexcept = 0;

    /**
     * @brief Configuration used when constructing this backend.
     */
    [[nodiscard]] virtual const VectorIndexConfig& config() const noexcept = 0;

    /**
     * @brief Human-readable backend name (e.g., "in_memory", "hnswlib", "faiss").
     */
    [[nodiscard]] virtual std::string name() const = 0;
};

// ============================================================================
// InMemoryVectorIndex
// ============================================================================

/**
 * @brief Brute-force in-memory vector index.
 *
 * Suitable for unit tests, small datasets (≤ 100 k vectors), and as a
 * reference implementation.  Linear scan O(n · dim) per query.
 *
 * Thread-safe: all public methods are protected by a single `std::mutex`.
 */
class InMemoryVectorIndex final : public IVectorIndexBackend {
public:
    /**
     * @brief Construct an in-memory index.
     * @param cfg Configuration; `cfg.dim` must be > 0.
     * @throws std::invalid_argument if `cfg.dim == 0`.
     */
    explicit InMemoryVectorIndex(const VectorIndexConfig& cfg);

    void add(const std::string& id,
             const std::vector<float>& embedding) override;

    [[nodiscard]] std::vector<KnnResult>
    search(const std::vector<float>& query, std::size_t k) const override;

    void remove(const std::string& id) override;

    [[nodiscard]] std::size_t size() const noexcept override;

    [[nodiscard]] const VectorIndexConfig& config() const noexcept override;

    [[nodiscard]] std::string name() const override { return "in_memory"; }

private:
    /// Compute raw distance between two embeddings according to `cfg_.metric`.
    float computeDistance(const std::vector<float>& a,
                          const std::vector<float>& b) const noexcept;

    /// Normalise a vector in-place (L2 norm); no-op if norm is zero.
    static void normalise(std::vector<float>& v) noexcept;

    /// Convert raw distance to a [0,1] similarity score.
    float toScore(float distance) const noexcept;

    VectorIndexConfig cfg_;
    /// id → normalised (for COSINE) or raw embedding
    std::unordered_map<std::string, std::vector<float>> vectors_;
    mutable std::mutex mutex_;
};

} // namespace storage
} // namespace themis
