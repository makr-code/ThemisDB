/**
 * @file vec_knn.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

/**
 * @brief ============================================================================ SIMD-accelerated pairwise distance kernel (PERF-D3) Compile-time dispatch: AVX-512 → AVX2 → NEON → scalar ============================================================================
 * @param[in] a Input parameter.
 * @param[in] b Input parameter.
 * @param[in] dim Input parameter.
 * @return Return value.
 * @note Exception safety: noexcept.
 */

float simd_l2_sq(const float* a, const float* b, std::size_t dim) noexcept;

/**
 * @brief Simd batch l2 sq.
 * @param[in] query Input parameter.
 * @param[in] database Input parameter.
 * @param[in] n Input parameter.
 * @param[in] dim Input parameter.
 * @param[in,out] out Input/output parameter.
 * @note Exception safety: noexcept.
 */
void simd_batch_l2_sq(const float* query,
                      const float* database,
                      std::size_t n,
                      std::size_t dim,
                      float*      out) noexcept;

// ============================================================================
// DistanceCache – memoisation for repeated batch pairs (PERF-D3)
// ============================================================================

class DistanceCache {
public:
    explicit DistanceCache(std::size_t max_entries = 65536);
    ~DistanceCache() = default;

    // Non-copyable, movable
    DistanceCache(const DistanceCache&) = delete;
    DistanceCache& operator=(const DistanceCache&) = delete;
    DistanceCache(DistanceCache&&) noexcept;
    DistanceCache& operator=(DistanceCache&&) noexcept;

    /**
     * @brief Get.
     * @param[in] pk_a Input parameter.
     * @param[in] pk_b Input parameter.
     * @param[in,out] out Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool get(const std::string& pk_a, const std::string& pk_b, float& out) const;

    /**
     * @brief Put.
     * @param[in] pk_a Input parameter.
     * @param[in] pk_b Input parameter.
     * @param[in] value Input parameter.
     */
    void put(const std::string& pk_a, const std::string& pk_b, float value);

    /**
     * @brief Invalidate.
     * @param[in] pk Input parameter.
     */
    void invalidate(const std::string& pk);

    /**
     * @brief Clear.
     */
    void clear();

    /**
     * @brief Size.
     * @return Return value.
     */
    std::size_t size() const;

    std::size_t hits()   const { return hits_.load(std::memory_order_relaxed); }
    std::size_t misses() const { return misses_.load(std::memory_order_relaxed); }

private:
    struct Entry {
        std::string key;
        float       value;
    };

    /**
     * @brief Make Key.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
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

struct VecKnnPipelineConfig {
    std::size_t batch_size    = 32;

    std::size_t num_threads   = 0;

    bool        enable_cache  = true;

    std::size_t cache_entries = 65536;

    std::string vector_field  = "embedding";
};

struct VecKnnInsertResult {
    bool        ok            = true;
    std::string message;
    std::size_t inserted      = 0;  ///< successfully inserted entities
    std::size_t failed        = 0;  ///< entities that could not be inserted
};

class VecKnnInsertPipeline {
public:
  using AddBatchBridgeFn = std::function<VecKnnInsertResult(
    VectorIndexManager&,
    const std::vector<BaseEntity>&,
    std::string_view)>;

  using ExtractVectorBridgeFn = std::function<std::optional<std::vector<float>>(
    const BaseEntity&,
    std::string_view)>;

  /**
   * @brief Set Add Batch Bridge Fn.
   * @param[in] fn Input parameter.
   */
  static void setAddBatchBridgeFn(AddBatchBridgeFn fn);
  /**
   * @brief Clear Add Batch Bridge Fn.
   */
  static void clearAddBatchBridgeFn();

  /**
   * @brief Set Extract Vector Bridge Fn.
   * @param[in] fn Input parameter.
   */
  static void setExtractVectorBridgeFn(ExtractVectorBridgeFn fn);
  /**
   * @brief Clear Extract Vector Bridge Fn.
   */
  static void clearExtractVectorBridgeFn();

    explicit VecKnnInsertPipeline(VecKnnPipelineConfig config = {});
    ~VecKnnInsertPipeline();

    // Non-copyable
    VecKnnInsertPipeline(const VecKnnInsertPipeline&) = delete;
    VecKnnInsertPipeline& operator=(const VecKnnInsertPipeline&) = delete;

    VecKnnInsertResult insertBatch(VectorIndexManager&                  index,
                                   const std::vector<BaseEntity>&       entities,
                                   std::string_view                     vectorField = "");

    /**
     * @brief Compute Distances.
     * @param[in] query_vectors Input parameter.
     * @param[in] numQueries Input parameter.
     * @param[in] db_vectors Input parameter.
     * @param[in] numDB Input parameter.
     * @param[in] dim Input parameter.
     * @return Return value.
     */
    std::vector<float> computeDistances(const float* query_vectors,
                                        std::size_t  numQueries,
                                        const float* db_vectors,
                                        std::size_t  numDB,
                                        std::size_t  dim) const;

    /**
     * @brief Cache.
     * @return Return value.
     * @details Implements cache without additional internal calls.
     */
    DistanceCache& cache() { return *cache_; }
    const DistanceCache& cache() const { return *cache_; }

    const VecKnnPipelineConfig& config() const { return config_; }
    /**
     * @brief Set Batch Size.
     * @param[in] sz Input parameter.
     */
    void setBatchSize(std::size_t sz);
    /**
     * @brief Set Thread Count.
     * @param[in] n Input parameter.
     */
    void setThreadCount(std::size_t n);
    /**
     * @brief Enable Distance Cache.
     * @param[in] enable Input parameter.
     */
    void enableDistanceCache(bool enable);

    std::size_t totalInserted() const { return total_inserted_.load(); }
    std::size_t totalFailed()   const { return total_failed_.load(); }

private:
    VecKnnPipelineConfig          config_;
    std::unique_ptr<DistanceCache> cache_;
    std::atomic<std::size_t>      total_inserted_{0};
    std::atomic<std::size_t>      total_failed_{0};

    mutable std::mutex            index_mtx_;
};

} // namespace acceleration
} // namespace themis
