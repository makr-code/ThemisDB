/**
 * @file query_accelerator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "themis/gpu/graph_cache.h"

namespace themis {
namespace gpu {

/**
 * @brief GPU-accelerated database query operations.
 *
 * Provides the infrastructure layer for GPU query acceleration described in
 * `src/gpu/FUTURE_ENHANCEMENTS.md` v1.2.0.  Operations are dispatched to the
 * GPU path when the row count exceeds `Config::gpu_threshold_rows`; otherwise
 * they fall back to a CPU implementation so the interface is always usable
 * without real GPU hardware. GPU-side launch, synchronization, or timeout
 * failures are fail-closed: the operation degrades to CPU and `used_gpu`
 * remains `false` unless the GPU path completed successfully.
 *
 * Supported operations
 * --------------------
 * - **scan**      — parallel row scan with optional filter predicate
 * - **sort**      — sort rows by a numeric key extractor (ASC / DESC)
 * - **aggregate** — SUM / COUNT / MIN / MAX / AVG over a numeric column
 * - **hashJoin**  — hash join two row sets on matching uint64_t keys
 * - **dotProduct** — dot product of two float vectors (FP32/FP16/BF16 precision)
 * - **annSearch** — approximate k-nearest-neighbor vector similarity search
 *                   (GPU stub: cuVS/RAFT `ivf_flat` on CUDA; CPU brute-force fallback)
 *
 * Result Parity & Precision Guarantees
 * -------------------------------------
 *
 * **Deterministic CPU Fallback**: All GPU operations include deterministic CPU fallback
 * paths that execute when:
 * - No GPU device is available (returns same result as CPU-forced mode)
 * - CUDA/HIP memory allocation fails (automatic fallback via CHECKED_CUDA macro)
 * - CUDA/HIP memory copy operations fail (fallback triggered by exception)
 * - Kernel execution times out (exceeds 5-second SLA enforced by KernelSLAGuard)
 *
 * When fallback occurs:
 * - `used_gpu` field in result is set to `false`
 * - Result value matches CPU implementation within precision tolerance (see below)
 * - No partial/corrupted results are returned (fail-closed design)
 *
 * **Floating-Point Result Tolerance**:
 * GPU and CPU paths produce results within the following tolerances:
 * - **FP32 (exact)**: Relative error < 1e-5 (no quantization)
 * - **FP16 (quantized)**: Relative error < 1e-3 (due to 10-bit mantissa quantization)
 * - **BF16 (quantized)**: Relative error < 5e-3 (due to 7-bit mantissa quantization)
 * - **Integer operations** (scan, sort, aggregate COUNT, hashJoin): Exact match
 *
 * These tolerances account for:
 * 1. Host-side FP16/BF16 quantization simulation (CPU path)
 * 2. GPU Tensor Core precision loss (GPU path)
 * 3. Floating-point rounding differences between platforms
 * 4. Order-dependent accumulation (e.g., dot product, SUM may differ by 1 ULP)
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class GPUQueryAccelerator {
public:
    // -----------------------------------------------------------------------
    // Row (unit of data)
    // -----------------------------------------------------------------------
    struct Row {
        uint64_t             id   = 0;
        std::vector<uint8_t> data;   ///< serialised payload
    };

    // -----------------------------------------------------------------------
    // Precision mode for Tensor Core-style operations (FP16/BF16)
    // -----------------------------------------------------------------------
    enum class PrecisionMode {
        FP32,  ///< Full 32-bit float (default)
        FP16,  ///< IEEE 754 half-precision (16-bit); lossy, ~3.3 decimal digits
        BF16,  ///< bfloat16 (top 16 bits of FP32); same exponent range as FP32
    };

    // -----------------------------------------------------------------------
    // Aggregate function
    // -----------------------------------------------------------------------
    enum class AggFunc { SUM, COUNT, MIN, MAX, AVG };

    // -----------------------------------------------------------------------
    // Sort order
    // -----------------------------------------------------------------------
    enum class SortOrder { ASC, DESC };

    // -----------------------------------------------------------------------
    // Filter predicate type
    // -----------------------------------------------------------------------
    using FilterFn   = std::function<bool(const Row&)>;
    using KeyFn      = std::function<double(const Row&)>;
    using JoinKeyFn  = std::function<uint64_t(const Row&)>;

    // -----------------------------------------------------------------------
    // Result types
    // -----------------------------------------------------------------------
    struct ScanResult {
        std::vector<Row> rows;
        size_t           rows_scanned = 0;
        size_t           rows_passed  = 0;
        bool             used_gpu     = false;
    };

    struct SortResult {
        std::vector<Row> rows;
        bool             used_gpu = false;
    };

    struct AggResult {
        double value    = 0.0;
        size_t count    = 0;
        bool   used_gpu = false;
    };

    struct JoinResult {
        std::vector<std::pair<Row, Row>> pairs;
        bool used_gpu = false;
    };

    // -----------------------------------------------------------------------
    // Dot-product result (Tensor Core path)
    // -----------------------------------------------------------------------
    struct DotProductResult {
        double        value          = 0.0;   ///< Computed dot product
        PrecisionMode precision_used = PrecisionMode::FP32;
        bool          used_gpu       = false;
    };

    // -----------------------------------------------------------------------
    // ANN search result (cuVS/RAFT path)
    // -----------------------------------------------------------------------
    struct AnnNeighbor {
        size_t index    = 0;     ///< Index into the database vector set
        float  distance = 0.0f;  ///< Distance to the query vector
    };

    struct AnnResult {
        /// results[query_idx] holds the k nearest neighbors for that query,
        /// sorted ascending by distance.
        std::vector<std::vector<AnnNeighbor>> results;
        bool used_gpu = false;
    };

    // -----------------------------------------------------------------------
    // TopK result
    // -----------------------------------------------------------------------
    struct TopKResult {
        std::vector<Row> rows;     ///< Up to k rows, sorted by key in @p order
        bool             used_gpu = false;
    };

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------
    struct Stats {
        size_t   total_scans         = 0;
        size_t   total_sorts         = 0;
        size_t   total_aggregates    = 0;
        size_t   total_joins         = 0;
        size_t   total_dot_products  = 0;  ///< Dot-product operations completed
        size_t   total_ann_searches  = 0;  ///< ANN search operations completed
        size_t   total_topk          = 0;  ///< TopK partial-sort operations completed
        uint64_t rows_processed      = 0;
        uint64_t bytes_scanned       = 0;
        size_t   gpu_ops             = 0;
        size_t   cpu_fallback_ops    = 0;
        size_t   graph_cache_hits    = 0;   ///< Operations served via graph replay
        size_t   graph_cache_misses  = 0;   ///< New patterns captured into the cache
        size_t   fp16_ops            = 0;   ///< Tensor Core FP16 dot-product calls
        size_t   bf16_ops            = 0;   ///< Tensor Core BF16 dot-product calls
    };

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------
    struct Config {
        /// Row count below which the CPU path is used even when GPU is active.
        size_t gpu_threshold_rows = 10'000;
        /// Force CPU path unconditionally (useful for testing or CPU-only builds).
        bool force_cpu = false;
        /// Enable CUDA graph capture for recurring query execution patterns.
        bool enable_graph_cache = false;
        /// Precision mode for Tensor Core dot-product operations.
        PrecisionMode precision_mode = PrecisionMode::FP32;
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------
    GPUQueryAccelerator();
    explicit GPUQueryAccelerator(const Config& config);

    // -----------------------------------------------------------------------
    // Operations
    // -----------------------------------------------------------------------

    /**
     * @brief Parallel row scan with optional filter predicate.
     *
     * When @p filter is nullptr every row passes.  GPU path would use a
     * Thrust/cub parallel select; CPU path is a sequential scan.
     */
    ScanResult scan(const std::vector<Row>& rows,
                    FilterFn filter = nullptr);

    /**
     * @brief Sort @p rows by @p key_fn in @p order.
     *
     * GPU path would use Thrust sort; CPU path uses std::stable_sort.
     * Rows with equal keys retain their original relative order.
     */
    SortResult sort(std::vector<Row> rows,
                    KeyFn            key_fn,
                    SortOrder        order = SortOrder::ASC);

    /**
     * @brief Compute an aggregate over @p rows using @p value_fn.
     *
     * GPU path would use a reduction kernel; CPU path is a sequential pass.
     */
    AggResult aggregate(const std::vector<Row>& rows,
                        AggFunc                  func,
                        KeyFn                    value_fn);

    /**
     * @brief Hash join @p left and @p right on matching join keys.
     *
     * Builds a hash table on the smaller side then probes with the larger
     * side.  GPU path would use a parallel hash join; CPU path uses
     * std::unordered_multimap.
     */
    JoinResult hashJoin(const std::vector<Row>& left,
                        const std::vector<Row>& right,
                        JoinKeyFn               left_key,
                        JoinKeyFn               right_key);

    /**
     * @brief Compute the dot product of two float vectors using the configured
     *        precision mode (FP32, FP16, or BF16).
     *
     * In FP16/BF16 modes inputs are first quantised to the target precision
     * then de-quantised back to float before accumulation, simulating the
     * precision loss of Tensor Core operations on hardware that does not
     * accumulate FP32 internally.  On real hardware this call would be
     * replaced by a cuBLAS `cublasSgemv` (FP32), `cublasHgemm` (FP16), or
     * `cublasGemmEx` with `CUBLAS_COMPUTE_16F` / `CUDA_R_16BF` (BF16).
     *
     * @param a  First operand; must be the same length as @p b.
     * @param b  Second operand.
     * @return   DotProductResult with `value` and the precision actually used.
     *           Returns 0.0 on empty or size-mismatch input.
     */
    DotProductResult dotProduct(const std::vector<float>& a,
                                const std::vector<float>& b);

    /**
     * @brief Approximate k-nearest-neighbor vector similarity search.
     *
     * Searches @p database (a flat array of @p numVectors vectors each of
     * length @p dim) for the @p k nearest neighbors of each query in @p queries
     * (a flat array of @p numQueries vectors each of length @p dim).
     *
     * Distance metric
     * ---------------
     * When @p useL2 is true (default), squared Euclidean (L2) distance is used.
     * When false, negative inner product is used as a distance metric (lower
     * value = higher similarity).  For unit-normalized vectors this is equivalent
     * to cosine distance; for unnormalized vectors callers performing maximum
     * inner product search (MIPS) should normalize their vectors beforehand.
     *
     * GPU path (when THEMIS_ENABLE_CUDA is defined and vector count ≥
     * `Config::gpu_threshold_rows`) — stub for production cuVS/RAFT wiring:
     *   1. Allocate device memory and copy @p database + @p queries.
     *   2. Build an IVF-Flat index: `cuvs::neighbors::ivf_flat::build()`
     *   3. Search: `cuvs::neighbors::ivf_flat::search()`
     *   4. Copy results back to host and populate `AnnResult`.
     *
     * CPU fallback — brute-force exact k-NN using a max-heap per query.
     *
     * @param queries    Flat float array of @p numQueries × @p dim elements.
     * @param numQueries Number of query vectors.
     * @param dim        Vector dimensionality; must match for queries and database.
     * @param database   Flat float array of @p numVectors × @p dim elements.
     * @param numVectors Number of database vectors.
     * @param k          Number of nearest neighbors to return per query.
     * @param useL2      If true use L2 distance; if false use inner-product distance.
     * @return AnnResult where `results[i]` holds the @p k nearest neighbors for
     *         query @p i, sorted ascending by distance.
     *         Returns empty results if inputs are invalid (dim=0, k=0, etc.).
     */
    AnnResult annSearch(const std::vector<float>& queries,
                        size_t                    numQueries,
                        size_t                    dim,
                        const std::vector<float>& database,
                        size_t                    numVectors,
                        size_t                    k,
                        bool                      useL2 = true);

    /**
     * @brief GPU-accelerated partial sort — return the top @p k rows by
     *        @p key_fn in @p order without fully sorting the input.
     *
     * Uses `thrust::partial_sort_copy` on GPU (CUDA/HIP) and
     * `std::partial_sort` on the CPU fallback path.  When `k >= rows.size()`
     * the result is equivalent to a full sort.
     *
     * Fail-closed: any GPU allocation failure or SLA timeout causes an
     * immediate CPU fallback; `used_gpu` reflects the path actually taken.
     *
     * @param rows    Input row set.
     * @param key_fn  Numeric key extractor (host callable).
     * @param k       Maximum number of rows to return.
     * @param order   ASC returns the k rows with the smallest keys;
     *                DESC returns the k rows with the largest keys.
     * @return TopKResult with at most k rows sorted in @p order.
     */
    TopKResult topK(std::vector<Row> rows,
                    KeyFn            key_fn,
                    size_t           k,
                    SortOrder        order = SortOrder::ASC);

    // -----------------------------------------------------------------------
    // Stats
    // -----------------------------------------------------------------------
    Stats getStats() const;
    void  resetStats();

    // -----------------------------------------------------------------------
    // Graph cache control
    // -----------------------------------------------------------------------

    /**
     * @brief Enable CUDA graph capture for recurring query patterns.
     *
     * When enabled, each operation checks the graph cache before executing.
     * On a cache miss the shape is captured; on a hit the cached graph is
     * replayed and the `graph_cache_hits` stat is incremented.
     *
     * In a production CUDA build, replaying a cached graph eliminates
     * per-launch kernel-setup overhead via `cudaGraphLaunch`.
     */
    void enableGraphCache();

    /**
     * @brief Disable CUDA graph capture.  The existing cache is preserved
     * but will not be consulted until re-enabled.
     */
    void disableGraphCache();

    /**
     * @brief Return statistics from the underlying GPUGraphCache.
     */
    GPUGraphCache::Stats getGraphCacheStats() const;

private:
    Config               config_;
    mutable std::mutex   mutex_;
    Stats                stats_;
    GPUGraphCache        graph_cache_;
    std::atomic<bool>    graph_cache_enabled_{false};

    bool shouldUseGPU(size_t num_rows) const noexcept;
    void recordOp(size_t rows, uint64_t bytes, bool gpu_used);

    /// Build a QueryShape for a single-sided operation.
    static QueryShape makeShape(QueryShape::OpType op,
                                size_t             row_count,
                                uint64_t           param_hash = 0) noexcept;
};

} // namespace gpu
} // namespace themis
