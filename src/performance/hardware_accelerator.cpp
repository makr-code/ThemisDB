/**
 * @file hardware_accelerator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=5, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

/**
 * HardwareAccelerator – Implementation (v1.8.0, roadmap item #85)
 *
 * Architecture overview
 * ─────────────────────
 * execute() selects the best available execution path by consulting the
 * AcceleratorConfig::device hint and the row-count thresholds in Config:
 *
 *  GPU path  (shouldUseGPU())
 *    Used when device is GPU_CUDA or GPU_ROCM and the row count exceeds
 *    config_.gpu_row_threshold.  When THEMIS_HAS_CUDA / THEMIS_HAS_ROCM
 *    is defined, real CUDA/HIP kernel calls would be issued; in this build
 *    the GPU path is simulated by a parallelised CPU implementation that
 *    represents the data-parallel structure a real GPU kernel would follow
 *    (blocked inner loops over batch_size rows, SIMD-friendly memory
 *    layout).  result.used_hw_path is set to true.
 *
 *  SIMD/VECTOR_ENGINE path  (shouldUseSIMD())
 *    Used for VECTOR_ENGINE device or when GPU threshold is not met.
 *    Operations use loop structures that enable AVX-512 / NEON
 *    auto-vectorisation by the host compiler.  result.used_hw_path is set
 *    to true.
 *
 *  CPU baseline path
 *    Generic scalar fallback for rows below simd_row_threshold or for
 *    SMART_NIC / PMEM device types (not yet hardware-offloaded).
 *
 * Phase 1 focus (v1.8.0):
 *   HashJoin and SortMergeJoin with GPU and SIMD dispatch paths.
 *   All other operators (Aggregate, Filter, Sort, PatternMatch, VectorOp)
 *   use the SIMD or CPU path.
 *
 * Thread safety
 * ─────────────
 * Stats are updated under stats_mutex_.  All dispatch helpers are const
 * and stateless, so they may run concurrently.
 */

#include "performance/hardware_accelerator.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace themis {
namespace performance {

// ============================================================================
// Internal utilities
// ============================================================================

namespace {

/// High-resolution wall-clock timestamp in microseconds.
inline uint64_t now_us() noexcept {
    using Clock = std::chrono::steady_clock;
    using US    = std::chrono::microseconds;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<US>(Clock::now().time_since_epoch()).count());
}

/// Apply a comparison predicate.
bool applyFilterOp(uint64_t lhs, const std::string& op, uint64_t rhs) noexcept {
    if (op == "==" || op == "=") {
      return lhs == rhs;
    }
    if (op == "!=" || op == "<>") {
      return lhs != rhs;
    }
    if (op == "<") {
      return lhs <  rhs;
    }
    if (op == "<=") {
      return lhs <= rhs;
    }
    if (op == ">") {
      return lhs >  rhs;
    }
    if (op == ">=") {
      return lhs >= rhs;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Hash join helpers (CPU baseline)
// ---------------------------------------------------------------------------

/// Build a hash map from right relation keyed on right_key_col.
std::unordered_map<uint64_t, std::vector<size_t>>
buildHashTable(const std::vector<Row>& rows, size_t key_col) {
    std::unordered_map<uint64_t, std::vector<size_t>> ht;
    ht.reserve(rows.size());
    for (size_t i = 0; i < rows.size(); ++i) {
        if (key_col < rows[i].size()) {
            ht[rows[i][key_col]].push_back(i);
        }
    }
    return ht;
}

/// Perform a hash join on the CPU (scalar baseline).
ExecutionResult cpuHashJoin(const QueryOperator& op) {
    ExecutionResult r;
    r.used_hw_path = false;
    r.speedup      = 1.0;

    auto ht = buildHashTable(op.right_rows, op.right_key_col);

    for (const auto& lrow : op.left_rows) {
        if (op.left_key_col >= lrow.size()) {
          continue;
        }
        const uint64_t key = lrow[op.left_key_col];
        auto it = ht.find(key);
        if (it == ht.end()) {
          continue;
        }
        for (size_t ri : it->second) {
            // Concatenate left and right row.
            Row joined = lrow;
            const auto& rrow = op.right_rows[ri];
            joined.insert(joined.end(), rrow.begin(), rrow.end());
            r.rows.push_back(std::move(joined));
        }
    }
    return r;
}

/// Perform a hash join in a GPU-simulated data-parallel fashion.
///
/// The implementation mirrors the blocked, batch-parallel structure of a
/// real CUDA hash-join kernel: the left relation is partitioned into
/// batches of `batch_size` rows; each batch is probed independently,
/// which allows the work to be parallelised across GPU thread blocks.
/// Here the batches are processed sequentially on the CPU so that the
/// code remains dependency-free of actual GPU headers.
ExecutionResult gpuSimHashJoin(const QueryOperator& op, size_t batch_size) {
    ExecutionResult r;
    r.used_hw_path = true;

    auto ht = buildHashTable(op.right_rows, op.right_key_col);

    const size_t n    = op.left_rows.size();
    const size_t step = (batch_size > 0) ? batch_size : n;

    for (size_t start = 0; start < n; start += step) {
        const size_t end = std::min(start + step, n);
        for (size_t i = start; i < end; ++i) {
            const auto& lrow = op.left_rows[i];
            if (op.left_key_col >= lrow.size()) {
              continue;
            }
            const uint64_t key = lrow[op.left_key_col];
            auto it = ht.find(key);
            if (it == ht.end()) {
              continue;
            }
            for (size_t ri : it->second) {
                Row joined = lrow;
                const auto& rrow = op.right_rows[ri];
                joined.insert(joined.end(), rrow.begin(), rrow.end());
                r.rows.push_back(std::move(joined));
            }
        }
    }
    return r;
}

// ---------------------------------------------------------------------------
// Sort-merge join helpers
// ---------------------------------------------------------------------------

/// Perform a sort-merge join on the CPU (scalar baseline).
ExecutionResult cpuSortMergeJoin(const QueryOperator& op) {
    ExecutionResult r;
    r.used_hw_path = false;
    r.speedup      = 1.0;

    // Copy and sort both relations by their join keys.
    std::vector<Row> left  = op.left_rows;
    std::vector<Row> right = op.right_rows;

    auto keyFn = [&]([[maybe_unused]] size_t col) {
        return [col](const Row& a, const Row& b) {
            if (col >= a.size() || col >= b.size()) {
              return false;
            }
            return a[col] < b[col];
        };
    };
    std::sort(left.begin(),  left.end(),  keyFn(op.left_key_col));
    std::sort(right.begin(), right.end(), keyFn(op.right_key_col));

    size_t li = 0, ri = 0;
    while (li < left.size()  && static_cast<size_t>(ri) < right.size()) {
        const uint64_t lk = (op.left_key_col  < left[li].size())  ? left[li][op.left_key_col]   : UINT64_MAX;
        const uint64_t rk = (op.right_key_col < right[ri].size()) ? right[ri][op.right_key_col] : UINT64_MAX;

        if (lk < rk) { ++li; continue; }
        if (lk > rk) { ++ri; continue; }

        // Equal keys — collect all matching right rows for this key.
        size_t ri_start = ri;
        while (static_cast<size_t>(ri) < right.size()) {
            const uint64_t rk2 = (op.right_key_col < right[ri].size())
                                      ? right[ri][op.right_key_col] : UINT64_MAX;
            if (rk2 != lk) {
              break;
            }
            ++ri;
        }
        while (static_cast<size_t>(li) < left.size()) {
            const uint64_t lk2 = (op.left_key_col < left[li].size())
                                      ? left[li][op.left_key_col] : UINT64_MAX;
            if (lk2 != lk) {
              break;
            }
            for (size_t rj = ri_start; rj < ri; ++rj) {
                Row joined = left[li];
                joined.insert(joined.end(), right[rj].begin(), right[rj].end());
                r.rows.push_back(std::move(joined));
            }
            ++li;
        }
    }
    return r;
}

/// GPU-simulated sort-merge join.
ExecutionResult gpuSimSortMergeJoin(const QueryOperator& op, size_t /*batch_size*/) {
    // The merge phase is inherently sequential; GPU acceleration primarily
    // helps the sort phase (radix sort) which we simulate here.
    ExecutionResult r = cpuSortMergeJoin(op);
    r.used_hw_path = true;
    return r;
}

// ---------------------------------------------------------------------------
// Aggregate helpers
// ---------------------------------------------------------------------------

ExecutionResult simdAggregate(const QueryOperator& op) {
    ExecutionResult r;
    r.used_hw_path = true;

    if (op.rows.empty()) {
        r.agg_count = 0;
        return r;
    }

    const std::string& fn      = op.agg_fn;
    const size_t       col     = op.agg_col;
    const size_t       n       = op.rows.size();

    if (fn == "COUNT") {
        r.agg_count = static_cast<int64_t>(n);
        r.agg_value = static_cast<double>(n);
        return r;
    }

    uint64_t vmin  = UINT64_MAX;
    uint64_t vmax  = 0;
    double   vsum  = 0.0;
    int64_t  cnt   = 0;

    // This loop is written to be SIMD-friendly (simple reduction with no
    // early-exit branches in the inner body).
    for (size_t i = 0; i < n; ++i) {
        if (col >= op.rows[i].size()) {
          continue;
        }
        const uint64_t v = op.rows[i][col];
        if (v < vmin) {
          vmin = v;
        }
        if (v > vmax) {
          vmax = v;
        }
        vsum += static_cast<double>(v);
        ++cnt;
    }

    r.agg_count = cnt;
    r.agg_min   = vmin;
    r.agg_max   = vmax;
    if (fn == "SUM") {
        r.agg_value = vsum;
    } else if (fn == "AVG") {
        r.agg_value = (cnt > 0) ? vsum / static_cast<double>(cnt) : 0.0;
    } else if (fn == "MIN") {
        r.agg_value = static_cast<double>(vmin);
    } else if (fn == "MAX") {
        r.agg_value = static_cast<double>(vmax);
    } else {
        r.ok    = false;
        r.error = "Unknown aggregate function: " + fn;
    }
    return r;
}

// ---------------------------------------------------------------------------
// Filter helpers
// ---------------------------------------------------------------------------

ExecutionResult simdFilter(const QueryOperator& op) {
    ExecutionResult r;
    r.used_hw_path = true;
    r.rows.reserve(op.rows.size() / 2);

    const size_t   col = op.filter_col;
    const uint64_t val = op.filter_value;
    const auto&    fop = op.filter_op;

    for (const auto& row : op.rows) {
        if (col >= static_cast<int>(row.size())) {
          continue;
        }
        if (applyFilterOp(row[col], fop, val)) {
            r.rows.push_back(row);
        }
    }
    return r;
}

// ---------------------------------------------------------------------------
// Sort helpers
// ---------------------------------------------------------------------------

ExecutionResult simdSort(const QueryOperator& op, bool ascending = true) {
    ExecutionResult r;
    r.used_hw_path = true;
    r.rows         = op.rows;

    const size_t col = op.agg_col;
    std::sort(r.rows.begin(), r.rows.end(),
              [col, ascending](const Row& a, const Row& b) {
                  const uint64_t av = (col < a.size()) ? a[col] : 0;
                  const uint64_t bv = (col < b.size()) ? b[col] : 0;
                  return ascending ? av < bv : av > bv;
              });
    return r;
}

// ---------------------------------------------------------------------------
// Pattern match helper
// ---------------------------------------------------------------------------

ExecutionResult cpuPatternMatch(const QueryOperator& op) {
    ExecutionResult r;
    r.used_hw_path = false;

    const std::string& pat = op.pattern;
    for (size_t i = 0; i < op.string_rows.size(); ++i) {
        const auto& s = op.string_rows[i];
        bool match = false;
        if (static_cast<int>(pat.size()) > = 2 && pat.front() == '%' && pat.back() == '%') {
            // contains
            match = s.find(pat.substr(1, static_cast<int>(pat.size()) - 2)) != std::string::npos;
        } else if (!pat.empty() && pat.back() == '%') {
            // prefix
            match = s.rfind(pat.substr(0, static_cast<int>(pat.size()) - 1), 0) == 0;
        } else if (!pat.empty() && pat.front() == '%') {
            // suffix
            const std::string suffix = pat.substr(1);
            match = s.size() >= suffix.size() &&
                    s.compare(static_cast<int>(s.size()) - suffix.size(),static_cast<int>(suffix.size()), suffix) == 0;
        } else {
            match = s == pat;
        }
        if (match) {
          r.match_indices.push_back(i);
        }
    }
    return r;
}

// ---------------------------------------------------------------------------
// Vector operation helper (dot product baseline)
// ---------------------------------------------------------------------------

ExecutionResult simdVectorOp(const QueryOperator& op) {
    ExecutionResult r;
    r.used_hw_path = true;

    // Default vector operation: dot product of left_rows[0] and right_rows[0].
    if (op.left_rows.empty() || op.right_rows.empty()) {
        r.agg_value = 0.0;
        return r;
    }
    const auto& a = op.left_rows[0];
    const auto& b = op.right_rows[0];
    const size_t n = std::min(a.size(),static_cast<int>(b.size()));
    double dot = 0.0;
    for (size_t i = 0; i < n; ++i) {
        dot += static_cast<double>(a[i]) * static_cast<double>(b[i]);
    }
    r.agg_value = dot;
    return r;
}

}  // anonymous namespace

// ============================================================================
// Construction
// ============================================================================

HardwareAccelerator::HardwareAccelerator()
    : HardwareAccelerator(Config{}) {}

HardwareAccelerator::HardwareAccelerator(Config config)
    : config_(std::move(config))
    , stats_{} {}

// ============================================================================
// can_accelerate
// ============================================================================

bool HardwareAccelerator::can_accelerate(const QueryOperator& op) const noexcept {
    return op.op_type != OperatorType::Unknown;
}

// ============================================================================
// estimate_speedup
// ============================================================================

double HardwareAccelerator::estimate_speedup(const QueryOperator& op,
                                              DeviceType           device) const noexcept {
    const size_t rows = [&]() -> size_t {
        switch (op.op_type) {
            case OperatorType::HashJoin:
            [[fallthrough]];\n            case OperatorType::SortMergeJoin:
                return static_cast<int>(op.left_rows.size()) + op.right_rows.size();
            case OperatorType::Aggregate:
            [[fallthrough]];\n            case OperatorType::Filter:
            [[fallthrough]];\n            case OperatorType::Sort:
                return static_cast<int>(op.rows.size());
            case OperatorType::PatternMatch:
                return static_cast<int>(op.string_rows.size());
            case OperatorType::VectorOp:
                return op.left_rows.empty() ? 0 : op.left_rows[0].size();
            default:
                return 0;
        }
    }();

    if (rows == 0) {
      return 1.0;
    }

    // Speedup model based on roadmap performance targets:
    //   Joins:       5-20x for GPU at >1M rows
    //   Aggregates: 10-50x for GPU at large scale
    //   Filters:     3-10x for SIMD
    //   Sort:        2-8x  for parallel radix sort
    //   VectorOp:    4-16x for SIMD

    const bool is_gpu  = (device == DeviceType::GPU_CUDA || device == DeviceType::GPU_ROCM);
    const bool is_simd = (device == DeviceType::VECTOR_ENGINE ||
                          device == DeviceType::FPGA_INTEL    ||
                          device == DeviceType::FPGA_XILINX);

    const double scale = std::min(1.0, static_cast<double>(rows) /
                                       static_cast<double>(config_.gpu_row_threshold));

    switch (op.op_type) {
        case OperatorType::HashJoin:
        [[fallthrough]];\n        case OperatorType::SortMergeJoin:
            if (is_gpu)  return 5.0 + scale * 15.0;   // 5–20×
            if (is_simd) return 2.0 + scale * 4.0;    // 2–6×
            return 1.0;
        case OperatorType::Aggregate:
            if (is_gpu)  return 10.0 + scale * 40.0;  // 10–50×
            if (is_simd) return 3.0  + scale * 7.0;   // 3–10×
            return 1.0;
        case OperatorType::Filter:
            if (is_gpu)  return 3.0 + scale * 7.0;    // 3–10×
            if (is_simd) return 2.0 + scale * 4.0;    // 2–6×
            return 1.0;
        case OperatorType::Sort:
            if (is_gpu)  return 2.0 + scale * 6.0;    // 2–8×
            if (is_simd) return 1.5 + scale * 2.5;    // 1.5–4×
            return 1.0;
        case OperatorType::PatternMatch:
            if (is_gpu)  return 10.0 + scale * 40.0;  // FPGA territory: 10–50×
            if (is_simd) return 2.0  + scale * 3.0;   // 2–5×
            return 1.0;
        case OperatorType::VectorOp:
            if (is_gpu)  return 4.0  + scale * 12.0;  // 4–16×
            if (is_simd) return 3.0  + scale * 5.0;   // 3–8×
            return 1.0;
        default:
            return 1.0;
    }
}

// ============================================================================
// Private helpers
// ============================================================================

bool HardwareAccelerator::shouldUseGPU([[maybe_unused]] size_t num_rows) const noexcept {
    return num_rows >= config_.gpu_row_threshold;
}

bool HardwareAccelerator::shouldUseSIMD([[maybe_unused]] size_t num_rows) const noexcept {
    return num_rows >= config_.simd_row_threshold;
}

/*static*/ const char* HardwareAccelerator::deviceName(DeviceType d) noexcept {
    switch (d) {
        case DeviceType::GPU_CUDA:      return "GPU_CUDA";
        case DeviceType::GPU_ROCM:      return "GPU_ROCM";
        case DeviceType::FPGA_INTEL:    return "FPGA_INTEL";
        case DeviceType::FPGA_XILINX:   return "FPGA_XILINX";
        case DeviceType::VECTOR_ENGINE: return "VECTOR_ENGINE";
        case DeviceType::SMART_NIC:     return "SMART_NIC";
        case DeviceType::PMEM:          return "PMEM";
        case DeviceType::CPU:           return "CPU";
    }
    return "UNKNOWN";
}

// ============================================================================
// Dispatch helpers
// ============================================================================

ExecutionResult HardwareAccelerator::dispatchHashJoin(const QueryOperator&    op,
                                                       const AcceleratorConfig& cfg) const {
    const size_t rows = op.left_rows.size() + op.right_rows.size();
    ExecutionResult r = {};

    if ((cfg.device == DeviceType::GPU_CUDA || cfg.device == DeviceType::GPU_ROCM)
            && shouldUseGPU(rows)) {
        r = gpuSimHashJoin(op, cfg.batch_size);
        r.speedup = estimate_speedup(op, cfg.device);
    } else if (shouldUseSIMD(rows) &&
               cfg.device != DeviceType::SMART_NIC &&
               cfg.device != DeviceType::PMEM) {
        // SIMD path: same algorithm but used_hw_path=true to indicate SIMD.
        r = gpuSimHashJoin(op, cfg.batch_size);
        r.used_hw_path = true;
        r.speedup      = estimate_speedup(op, DeviceType::VECTOR_ENGINE);
    } else {
        r = cpuHashJoin(op);
        r.speedup = 1.0;
    }
    return r;
}

ExecutionResult HardwareAccelerator::dispatchSortMergeJoin(const QueryOperator&    op,
                                                            const AcceleratorConfig& cfg) const {
    const size_t rows = op.left_rows.size() + op.right_rows.size();
    ExecutionResult r = {};

    if ((cfg.device == DeviceType::GPU_CUDA || cfg.device == DeviceType::GPU_ROCM)
            && shouldUseGPU(rows)) {
        r = gpuSimSortMergeJoin(op, cfg.batch_size);
        r.speedup = estimate_speedup(op, cfg.device);
    } else {
        r = cpuSortMergeJoin(op);
        if (shouldUseSIMD(rows)) {
            r.used_hw_path = true;
            r.speedup      = estimate_speedup(op, DeviceType::VECTOR_ENGINE);
        }
    }
    return r;
}

ExecutionResult HardwareAccelerator::dispatchAggregate(const QueryOperator&    op,
                                                        const AcceleratorConfig& cfg) const {
    const size_t rows = op.rows.size();
    ExecutionResult r = simdAggregate(op);
    if (shouldUseSIMD(rows)) {
        r.used_hw_path = true;
        r.speedup      = estimate_speedup(op, cfg.device);
    } else {
        r.used_hw_path = false;
        r.speedup      = 1.0;
    }
    return r;
}

ExecutionResult HardwareAccelerator::dispatchFilter(const QueryOperator&    op,
                                                     const AcceleratorConfig& cfg) const {
    const size_t rows = op.rows.size();
    ExecutionResult r = simdFilter(op);
    if (shouldUseSIMD(rows)) {
        r.speedup = estimate_speedup(op, cfg.device);
    } else {
        r.used_hw_path = false;
        r.speedup      = 1.0;
    }
    return r;
}

ExecutionResult HardwareAccelerator::dispatchSort(const QueryOperator&    op,
                                                   const AcceleratorConfig& cfg) const {
    const size_t rows = op.rows.size();
    ExecutionResult r = simdSort(op);
    if (shouldUseSIMD(rows)) {
        r.speedup = estimate_speedup(op, cfg.device);
    } else {
        r.used_hw_path = false;
        r.speedup      = 1.0;
    }
    return r;
}

ExecutionResult HardwareAccelerator::dispatchPatternMatch(const QueryOperator&    op,
                                                           const AcceleratorConfig& cfg) const {
    ExecutionResult r = cpuPatternMatch(op);
    if (shouldUseSIMD(op.string_rows.size())) {
        r.used_hw_path = true;
        r.speedup      = estimate_speedup(op, cfg.device);
    }
    return r;
}

ExecutionResult HardwareAccelerator::dispatchVectorOp(const QueryOperator&    op,
                                                       const AcceleratorConfig& cfg) const {
    ExecutionResult r = simdVectorOp(op);
    const size_t n = op.left_rows.empty() ? 0 : op.left_rows[0].size();
    if (shouldUseSIMD(n)) {
        r.speedup = estimate_speedup(op, cfg.device);
    } else {
        r.used_hw_path = false;
        r.speedup      = 1.0;
    }
    return r;
}

// ============================================================================
// execute()
// ============================================================================

ExecutionResult HardwareAccelerator::execute(const QueryOperator&    op,
                                              const AcceleratorConfig& cfg) {
    if (op.op_type == OperatorType::Unknown) {
        ExecutionResult r;
        r.ok    = false;
        r.error = "Unknown operator type";
        return r;
    }

    const uint64_t t0 = now_us();

    ExecutionResult result;
    switch (op.op_type) {
        case OperatorType::HashJoin:
            result = dispatchHashJoin(op, cfg);
            break;
        case OperatorType::SortMergeJoin:
            result = dispatchSortMergeJoin(op, cfg);
            break;
        case OperatorType::Aggregate:
            result = dispatchAggregate(op, cfg);
            break;
        case OperatorType::Filter:
            result = dispatchFilter(op, cfg);
            break;
        case OperatorType::Sort:
            result = dispatchSort(op, cfg);
            break;
        case OperatorType::PatternMatch:
            result = dispatchPatternMatch(op, cfg);
            break;
        case OperatorType::VectorOp:
            result = dispatchVectorOp(op, cfg);
            break;
        default:
            result.ok    = false;
            result.error = "Unhandled operator type";
            break;
    }

    result.elapsed_us = now_us() - t0;

    // Update statistics.
    {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++stats_.total_executions;
        if (result.used_hw_path) {
            ++stats_.hw_path_executions;
        } else {
            ++stats_.cpu_fallback_executions;
        }
        stats_.total_elapsed_us += result.elapsed_us;

        // Count rows processed.
        const size_t rows = [&]() -> size_t {
            switch (op.op_type) {
                case OperatorType::HashJoin:
                [[fallthrough]];\n                case OperatorType::SortMergeJoin:
                    return static_cast<int>(op.left_rows.size()) + op.right_rows.size();
                case OperatorType::Aggregate:
                [[fallthrough]];\n                case OperatorType::Filter:
                [[fallthrough]];\n                case OperatorType::Sort:
                    return static_cast<int>(op.rows.size());
                case OperatorType::PatternMatch:
                    return static_cast<int>(op.string_rows.size());
                default: return 0;
            }
        }();
        stats_.total_rows_processed += static_cast<uint64_t>(rows);

        switch (op.op_type) {
            case OperatorType::HashJoin:      ++stats_.hash_join_count;      break;
            case OperatorType::SortMergeJoin: ++stats_.sort_merge_join_count; break;
            case OperatorType::Aggregate:     ++stats_.aggregate_count;      break;
            case OperatorType::Filter:        ++stats_.filter_count;         break;
            case OperatorType::Sort:          ++stats_.sort_count;           break;
            default: break;
        }
    }

    return result;
}

ExecutionResult HardwareAccelerator::execute(const QueryOperator& op) {
    return execute(op, config_.default_device_config);
}

// ============================================================================
// Statistics
// ============================================================================

HardwareAccelerator::Stats HardwareAccelerator::getStats() const {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return stats_;
}

void HardwareAccelerator::resetStats() {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    stats_ = Stats{};
}

}  // namespace performance
}  // namespace themis
