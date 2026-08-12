/**
 * @file vec_knn.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>
#include <optional>

// Forward declarations
namespace themis {
class BaseEntity;
class VectorIndexManager;
} // namespace themis

namespace themis {
namespace acceleration {

// ============================================================================
// SIMD-accelerated pairwise distance kernel (PERF-D3)
// Compile-time dispatch: AVX-512 → AVX2 → NEON → scalar
// ============================================================================

/// Compute squared L2 distance between two float vectors using the best
/// available SIMD instruction set.  Returns sum((a[i]-b[i])^2) without
/// the final sqrt – sufficient for ranking.
float simd_l2_sq(const float* a, const float* b, std::size_t dim) noexcept;

/// Batch version: compute squared L2 distance from one query to n database
/// vectors stored contiguously (n*dim floats).  Results written to out[n].
/// Uses unrolled AVX2/AVX-512 loops for maximum throughput.
void simd_batch_l2_sq(const float* query,
                      const float* database,
                      std::size_t n,
                      std::size_t dim,
                      float*      out) noexcept;

// ============================================================================
// DistanceCache – memoisation for repeated batch pairs (PERF-D3)
// ============================================================================

/// Thread-safe LRU-like distance cache.  Key = (pk_a, pk_b) pair, value =
/// pre-computed squared-L2 distance.  Avoids recomputing distances for vectors
/// that appear in overlapping batches.
class DistanceCache {
public:
    explicit DistanceCache(std::size_t max_entries = 65536);
    ~DistanceCache() = default;

    // Non-copyable, movable
    DistanceCache(const DistanceCache&) = delete;
    DistanceCache& operator=(const DistanceCache&) = delete;
    DistanceCache(DistanceCache&&) noexcept;
    DistanceCache& operator=(DistanceCache&&) noexcept;

    /// Look up cached distance.  Returns true and sets out if found.
    bool get(const std::string& pk_a, const std::string& pk_b, float& out) const;

    /// Store a distance.  Thread-safe.  Evicts oldest entries when full.
    void put(const std::string& pk_a, const std::string& pk_b, float value);

    /// Invalidate all cached entries for a given primary key.
    void invalidate(const std::string& pk);

    /// Remove all entries.
    void clear();

    /// Current number of cached entries.
    std::size_t size() const;

    /// Cache hit/miss statistics.
    std::size_t hits()   const { return hits_.load(std::memory_order_relaxed); }
    std::size_t misses() const { return misses_.load(std::memory_order_relaxed); }

private:
    struct Entry {
        std::string key;
        float       value;
    };

    static std::string makeKey(const std::string& a, const std::string& b);

    std::size_t               max_entries_;
    mutable std::mutex        mtx_;
    std::unordered_map<std::string, float> map_;
    std::deque<std::string>   order_; // insertion order for eviction (O(1) pop_front)
    mutable std::atomic<std::size_t> hits_{0};
    mutable std::atomic<std::size_t> misses_{0};
};

// ============================================================================
// VecKnnInsertPipeline – parallel batch insertion (PERF-D3)
// ============================================================================

/// Configuration for the parallel insert pipeline.
struct VecKnnPipelineConfig {
    /// Number of entities processed in a single parallel sub-batch.
    /// Tuned for cache line efficiency; default 32.
    std::size_t batch_size    = 32;

    /// Number of worker threads.  0 = std::thread::hardware_concurrency().
    std::size_t num_threads   = 0;

    /// Enable distance memoisation across overlapping batches.
    bool        enable_cache  = true;

    /// Maximum cached distance pairs.
    std::size_t cache_entries = 65536;

    /// Vector field name in BaseEntity.
    std::string vector_field  = "embedding";
};

/// Result of a batch insert operation.
struct VecKnnInsertResult {
    bool        ok            = true;
    std::string message;
    std::size_t inserted      = 0;  ///< successfully inserted entities
    std::size_t failed        = 0;  ///< entities that could not be inserted
};

/// Parallel batch insertion pipeline for VectorIndexManager (PERF-D3).
///
/// Uses a thread pool to submit sub-batches concurrently.  Each worker uses
/// SIMD-accelerated distance computation via simd_batch_l2_sq().  A shared
/// DistanceCache avoids duplicate distance computation for overlapping batches.
///
/// Thread-safety: multiple callers can call insertBatch() concurrently; the
/// underlying VectorIndexManager::addBatch() is serialised via a mutex.
class VecKnnInsertPipeline {
public:
  /// Bridge callback for batch insertion into a vector index.
  ///
  /// Return value follows `VecKnnInsertResult` semantics:
  /// `ok=true` and `inserted>0` for successful writes, `ok=false` with
  /// `failed>0` and message on failure.
  using AddBatchBridgeFn = std::function<VecKnnInsertResult(
    VectorIndexManager&,
    const std::vector<BaseEntity>&,
    std::string_view)>;

  /// Bridge callback for vector extraction from an entity field.
  ///
  /// Expected to return an empty optional when the field is missing or not
  /// vector-compatible.
  using ExtractVectorBridgeFn = std::function<std::optional<std::vector<float>>(
    const BaseEntity&,
    std::string_view)>;

  /// Installs a process-wide add-batch bridge for link profiles where
  /// VectorIndexManager write symbols are provided by another module.
  static void setAddBatchBridgeFn(AddBatchBridgeFn fn);
  /// Clears the add-batch bridge and restores fail-closed behavior.
  static void clearAddBatchBridgeFn();

  /// Installs a process-wide vector-extraction bridge for link profiles
  /// where BaseEntity conversion helpers are provided by another module.
  static void setExtractVectorBridgeFn(ExtractVectorBridgeFn fn);
  /// Clears the vector-extraction bridge and restores fail-closed behavior.
  static void clearExtractVectorBridgeFn();

    explicit VecKnnInsertPipeline(VecKnnPipelineConfig config = {});
    ~VecKnnInsertPipeline();

    // Non-copyable
    VecKnnInsertPipeline(const VecKnnInsertPipeline&) = delete;
    VecKnnInsertPipeline& operator=(const VecKnnInsertPipeline&) = delete;

    /// Insert entities into index in parallel batches.
    VecKnnInsertResult insertBatch(VectorIndexManager&                  index,
                                   const std::vector<BaseEntity>&       entities,
                                   std::string_view                     vectorField = "");

    /// Compute pairwise squared-L2 distances for a flat vector array.
    /// Useful for pre-warming the cache or standalone distance queries.
    /// query_vectors: numQueries * dim floats; db_vectors: numDB * dim floats.
    std::vector<float> computeDistances(const float* query_vectors,
                                        std::size_t  numQueries,
                                        const float* db_vectors,
                                        std::size_t  numDB,
                                        std::size_t  dim) const;

    /// Access the internal distance cache (for inspection or pre-warming).
    DistanceCache& cache() { return *cache_; }
    const DistanceCache& cache() const { return *cache_; }

    /// Runtime configuration.
    const VecKnnPipelineConfig& config() const { return config_; }
    void setBatchSize(std::size_t sz);
    void setThreadCount(std::size_t n);
    void enableDistanceCache(bool enable);

    /// Accumulated statistics across all insertBatch() calls.
    std::size_t totalInserted() const { return total_inserted_.load(); }
    std::size_t totalFailed()   const { return total_failed_.load(); }

private:
    VecKnnPipelineConfig          config_;
    std::unique_ptr<DistanceCache> cache_;
    std::atomic<std::size_t>      total_inserted_{0};
    std::atomic<std::size_t>      total_failed_{0};

    /// Serialise concurrent writes to the shared VectorIndexManager.
    mutable std::mutex            index_mtx_;
};

} // namespace acceleration
} // namespace themis
