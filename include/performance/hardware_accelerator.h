/**
 * @file hardware_accelerator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace performance {

// ============================================================================
// Query operator types
// ============================================================================

/**
 * @brief The kind of relational operator being dispatched to the accelerator.
 */
enum class OperatorType {
    HashJoin,        ///< Hash join between two row sets
    SortMergeJoin,   ///< Sort-merge join
    Aggregate,       ///< SUM / COUNT / MIN / MAX / AVG
    Filter,          ///< Predicate-based row filter
    Sort,            ///< Sort by one or more keys
    PatternMatch,    ///< LIKE / regex string matching
    VectorOp,        ///< SIMD / vector arithmetic operation
    Unknown
};

/**
 * @brief A single row represented as a vector of uint64_t values.
 *
 * Numeric values are stored directly; string/opaque data is passed as
 * 64-bit hashes for join-key purposes.
 */
using Row = std::vector<uint64_t>;

/**
 * @brief Lightweight descriptor of a relational operator to be accelerated.
 *
 * Callers populate the relevant fields for their operator type and pass
 * this struct to HardwareAccelerator::execute().
 */
struct QueryOperator {
    OperatorType op_type = OperatorType::Unknown;

    // ── Join operands ─────────────────────────────────────────────────────
    std::vector<Row> left_rows;   ///< Left input relation
    std::vector<Row> right_rows;  ///< Right input relation (join only)
    size_t left_key_col  = 0;     ///< Column index of the join key in left
    size_t right_key_col = 0;     ///< Column index of the join key in right

    // ── Filter / aggregate operands ───────────────────────────────────────
    std::vector<Row> rows;        ///< Input rows for filter / aggregate / sort
    size_t           agg_col = 0; ///< Column index for aggregation / sort
    std::string      agg_fn;      ///< "SUM", "COUNT", "MIN", "MAX", "AVG"

    /// Predicate: keep rows where rows[i][filter_col] OP filter_value.
    size_t   filter_col   = 0;
    uint64_t filter_value = 0;
    /// Comparison operator encoded as a string: "<", "<=", ">", ">=", "==", "!="
    std::string filter_op = "==";

    // ── Pattern matching ─────────────────────────────────────────────────
    std::vector<std::string> string_rows;   ///< Input strings for pattern match
    std::string              pattern;        ///< Pattern (prefix / suffix / contains)

    // ── Metadata ─────────────────────────────────────────────────────────
    std::string label;   ///< Human-readable label for debugging / profiling
};

// ============================================================================
// Execution result
// ============================================================================

/**
 * @brief Result produced by HardwareAccelerator::execute().
 */
struct ExecutionResult {
    // ── Join / filter result ─────────────────────────────────────────────
    std::vector<Row>         rows;    ///< Output rows (join / filter / sort)

    // ── Aggregate result ─────────────────────────────────────────────────
    double    agg_value   = 0.0;  ///< Aggregate value (SUM / AVG)
    int64_t   agg_count   = 0;   ///< COUNT result
    uint64_t  agg_min     = 0;   ///< MIN result
    uint64_t  agg_max     = 0;   ///< MAX result

    // ── Pattern match result ──────────────────────────────────────────────
    std::vector<size_t> match_indices;  ///< Indices of matching strings

    // ── Status ────────────────────────────────────────────────────────────
    bool        ok        = true;
    std::string error;

    /// True when the result was produced by the hardware-accelerated path.
    bool        used_hw_path = false;

    /// Estimated speedup factor (≥ 1.0) relative to the CPU baseline.
    double      speedup   = 1.0;

    /// Wall-clock execution time in microseconds.
    uint64_t    elapsed_us = 0;
};

// ============================================================================
// HardwareAccelerator
// ============================================================================

/**
 * @brief Hardware-accelerated query execution for the performance module
 *        (v1.8.0, roadmap item #85).
 *
 * Dispatches compute-intensive relational operators to the fastest available
 * execution path in the following priority order:
 *
 *   1. **GPU_CUDA / GPU_ROCM** — GPU parallel execution via simulated
 *      CUDA/HIP dispatch (real GPU back-end is gated on THEMIS_HAS_CUDA /
 *      THEMIS_HAS_ROCM compile flags).  Falls back to VECTOR_ENGINE when
 *      the GPU is not available or the row count is below the GPU threshold.
 *
 *   2. **VECTOR_ENGINE** — AVX-512 / NEON SIMD execution for batch
 *      arithmetic and hash operations.  The compiler's auto-vectoriser is
 *      guided by data-layout hints emitted by this class.
 *
 *   3. **FPGA_INTEL / FPGA_XILINX** — placeholder; routes to VECTOR_ENGINE
 *      in this release.
 *
 *   4. **SMART_NIC / PMEM** — placeholder; routes to CPU baseline.
 *
 * **CPU baseline fallback** is always used when no matching device path is
 * available, ensuring the API is fully functional without hardware.
 *
 * ## v1.8.0 scope (Phase 1 — GPU join acceleration)
 *
 * The v1.8.0 deliverable focuses on Phase 1: GPU-accelerated hash joins and
 * sort-merge joins for large row sets (> gpu_row_threshold rows).  All other
 * operator types are handled by the SIMD or CPU paths.
 *
 * ## Thread Safety
 *
 * All public methods are thread-safe.  Statistics are updated atomically.
 * The accelerator configuration is immutable after construction.
 *
 * ## Usage
 *
 * @code
 *   HardwareAccelerator accel;
 *
 *   QueryOperator op;
 *   op.op_type      = OperatorType::HashJoin;
 *   op.left_rows    = buildRelation(1'000'000);
 *   op.right_rows   = buildRelation(500'000);
 *   op.left_key_col  = 0;
 *   op.right_key_col = 0;
 *
 *   HardwareAccelerator::AcceleratorConfig cfg;
 *   cfg.device           = HardwareAccelerator::DeviceType::GPU_CUDA;
 *   cfg.device_memory_mb = 8192;
 *   cfg.batch_size       = 100'000;
 *
 *   if (accel.can_accelerate(op)) {
 *       auto result = accel.execute(op, cfg);
 *       // result.used_hw_path == true when GPU path was taken
 *       assert(result.speedup >= 5.0);
 *   }
 * @endcode
 */
class HardwareAccelerator {
public:
    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * @brief Enumeration of supported accelerator device types.
     */
    enum class DeviceType {
        GPU_CUDA,      ///< NVIDIA GPU (CUDA)
        GPU_ROCM,      ///< AMD GPU (ROCm / HIP)
        FPGA_INTEL,    ///< Intel FPGA (OpenCL / oneAPI)
        FPGA_XILINX,   ///< Xilinx / AMD FPGA (Vitis)
        VECTOR_ENGINE, ///< CPU SIMD (AVX-512 / NEON)
        SMART_NIC,     ///< SmartNIC offload (not yet implemented)
        PMEM,          ///< Persistent Memory direct access
        CPU            ///< Software baseline (always available)
    };

    /**
     * @brief Per-execution accelerator configuration.
     */
    struct AcceleratorConfig {
        /** Preferred execution device. */
        DeviceType device = DeviceType::GPU_CUDA;

        /** Device memory budget in MB. */
        size_t device_memory_mb = 8192;

        /** Enable multi-stage execution pipelining. */
        bool enable_pipelining = true;

        /** Use DMA for host-to-device data transfer (GPU paths). */
        bool enable_async_copy = true;

        /** Number of rows per batch when processing large inputs. */
        size_t batch_size = 10'000;
    };

    /**
     * @brief Global accelerator configuration set at construction time.
     */
    struct Config {
        /** Minimum number of rows to trigger the GPU path.
         *  Below this threshold, VECTOR_ENGINE or CPU is preferred. */
        size_t gpu_row_threshold = 100'000;

        /** Minimum number of rows to trigger the SIMD/VECTOR_ENGINE path. */
        size_t simd_row_threshold = 1'000;

        /** Default AcceleratorConfig used when the caller does not provide one. */
        AcceleratorConfig default_device_config;
    };

    // =========================================================================
    // Statistics
    // =========================================================================

    /** @brief Aggregate execution statistics. */
    struct Stats {
        /** Total number of execute() calls. */
        uint64_t total_executions = 0;

        /** Calls that used a hardware-accelerated path (GPU or SIMD). */
        uint64_t hw_path_executions = 0;

        /** Calls that fell back to the CPU baseline. */
        uint64_t cpu_fallback_executions = 0;

        /** Total number of rows processed across all executions. */
        uint64_t total_rows_processed = 0;

        /** Cumulative wall-clock time in execute() calls (µs). */
        uint64_t total_elapsed_us = 0;

        /** Number of hash-join operations dispatched. */
        uint64_t hash_join_count = 0;

        /** Number of sort-merge-join operations dispatched. */
        uint64_t sort_merge_join_count = 0;

        /** Number of aggregate operations dispatched. */
        uint64_t aggregate_count = 0;

        /** Number of filter operations dispatched. */
        uint64_t filter_count = 0;

        /** Number of sort operations dispatched. */
        uint64_t sort_count = 0;
    };

    // =========================================================================
    // Construction
    // =========================================================================

    HardwareAccelerator();
    explicit HardwareAccelerator(Config config);
    ~HardwareAccelerator() = default;

    HardwareAccelerator(const HardwareAccelerator&)            = delete;
    HardwareAccelerator& operator=(const HardwareAccelerator&) = delete;
    HardwareAccelerator(HardwareAccelerator&&)                 = default;
    HardwareAccelerator& operator=(HardwareAccelerator&&)      = default;

    // =========================================================================
    // Core API
    // =========================================================================

    /**
     * @brief Execute a query operator, dispatching to the fastest available
     *        hardware path.
     *
     * When the preferred device is unavailable the accelerator automatically
     * falls back in the order: GPU → VECTOR_ENGINE → CPU.
     *
     * @param op     Operator descriptor populated by the caller.
     * @param config Per-call device and memory configuration.
     * @return       ExecutionResult; result.ok is false on error.
     */
    ExecutionResult execute(const QueryOperator&    op,
                            const AcceleratorConfig& config);

    /**
     * @brief Execute using the default AcceleratorConfig.
     */
    ExecutionResult execute(const QueryOperator& op);

    /**
     * @brief Return true when the given operator can benefit from hardware
     *        acceleration.
     *
     * Always returns false for OperatorType::Unknown.  Returns true for
     * HashJoin, SortMergeJoin, Aggregate, Filter, Sort, PatternMatch, and
     * VectorOp regardless of row count — the size-based dispatch decision is
     * made inside execute().
     */
    bool can_accelerate(const QueryOperator& op) const noexcept;

    /**
     * @brief Estimate the expected speedup for the operator on the given
     *        device, relative to the CPU baseline.
     *
     * Returns a value ≥ 1.0.  The estimate is based on the row count and
     * operator type; no real hardware query is issued.
     *
     * @param op     Operator descriptor (row count must be populated).
     * @param device Target device; defaults to GPU_CUDA.
     */
    double estimate_speedup(const QueryOperator& op,
                            DeviceType           device = DeviceType::GPU_CUDA) const noexcept;

    // =========================================================================
    // Statistics
    // =========================================================================

    /** @brief Return a snapshot of current execution statistics. */
    Stats getStats() const;

    /** @brief Reset all execution statistics to zero. */
    void resetStats();

    // =========================================================================
    // Configuration accessors
    // =========================================================================

    /** @brief Return the global configuration (immutable after construction). */
    const Config& config() const noexcept { return config_; }

private:
    // ── Internal dispatch helpers ─────────────────────────────────────────

    ExecutionResult dispatchHashJoin(const QueryOperator&    op,
                                     const AcceleratorConfig& cfg) const;
    ExecutionResult dispatchSortMergeJoin(const QueryOperator&    op,
                                          const AcceleratorConfig& cfg) const;
    ExecutionResult dispatchAggregate(const QueryOperator&    op,
                                      const AcceleratorConfig& cfg) const;
    ExecutionResult dispatchFilter(const QueryOperator&    op,
                                   const AcceleratorConfig& cfg) const;
    ExecutionResult dispatchSort(const QueryOperator&    op,
                                 const AcceleratorConfig& cfg) const;
    ExecutionResult dispatchPatternMatch(const QueryOperator&    op,
                                         const AcceleratorConfig& cfg) const;
    ExecutionResult dispatchVectorOp(const QueryOperator&    op,
                                     const AcceleratorConfig& cfg) const;

    /// True when the row count justifies the GPU dispatch overhead.
    bool shouldUseGPU(size_t num_rows) const noexcept;
    /// True when the row count justifies the SIMD dispatch overhead.
    bool shouldUseSIMD(size_t num_rows) const noexcept;

    /// Convert DeviceType to a human-readable string for error messages.
    static const char* deviceName(DeviceType d) noexcept;

    // ── State ─────────────────────────────────────────────────────────────
    Config config_;

    mutable std::mutex stats_mutex_;
    Stats              stats_;
};

}  // namespace performance
}  // namespace themis
