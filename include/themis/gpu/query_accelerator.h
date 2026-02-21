/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_accelerator.h                                ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:35:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     196                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f6d2a1ab9  2026-02-20  GPU module: production-ready implementation — memory mana... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace gpu {

/**
 * @brief GPU-accelerated database query operations.
 *
 * Provides the infrastructure layer for GPU query acceleration described in
 * `src/gpu/FUTURE_ENHANCEMENTS.md` v1.2.0.  Operations are dispatched to the
 * GPU path when the row count exceeds `Config::gpu_threshold_rows`; otherwise
 * they fall back to a CPU implementation so the interface is always usable
 * without real GPU hardware.
 *
 * Supported operations
 * --------------------
 * - **scan**      — parallel row scan with optional filter predicate
 * - **sort**      — sort rows by a numeric key extractor (ASC / DESC)
 * - **aggregate** — SUM / COUNT / MIN / MAX / AVG over a numeric column
 * - **hashJoin**  — hash join two row sets on matching uint64_t keys
 *
 * When THEMIS_ENABLE_CUDA / THEMIS_ENABLE_HIP are defined the stub body can
 * be replaced with the matching cuBLAS / hipBLAS / Thrust call.
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
    // Statistics
    // -----------------------------------------------------------------------
    struct Stats {
        size_t   total_scans       = 0;
        size_t   total_sorts       = 0;
        size_t   total_aggregates  = 0;
        size_t   total_joins       = 0;
        uint64_t rows_processed    = 0;
        uint64_t bytes_scanned     = 0;
        size_t   gpu_ops           = 0;
        size_t   cpu_fallback_ops  = 0;
    };

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------
    struct Config {
        /// Row count below which the CPU path is used even when GPU is active.
        size_t gpu_threshold_rows = 10'000;
        /// Force CPU path unconditionally (useful for testing or CPU-only builds).
        bool force_cpu = false;
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

    // -----------------------------------------------------------------------
    // Stats
    // -----------------------------------------------------------------------
    Stats getStats() const;
    void  resetStats();

private:
    Config             config_;
    mutable std::mutex mutex_;
    Stats              stats_;

    bool shouldUseGPU(size_t num_rows) const noexcept;
    void recordOp(size_t rows, uint64_t bytes, bool gpu_used);
};

} // namespace gpu
} // namespace themis
