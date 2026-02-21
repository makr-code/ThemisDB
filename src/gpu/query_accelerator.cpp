/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_accelerator.cpp                              ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   75.0/100                                       ║
    • Total Lines:     243                                            ║
    • Open Issues:     TODOs: 0, Stubs: 5                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "themis/gpu/query_accelerator.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <unordered_map>

namespace themis {
namespace gpu {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

GPUQueryAccelerator::GPUQueryAccelerator()
    : GPUQueryAccelerator(Config{}) {}

GPUQueryAccelerator::GPUQueryAccelerator(const Config& config)
    : config_(config) {}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

bool GPUQueryAccelerator::shouldUseGPU(size_t num_rows) const noexcept {
    if (config_.force_cpu) return false;
    return num_rows >= config_.gpu_threshold_rows;
}

void GPUQueryAccelerator::recordOp(size_t rows, uint64_t bytes, bool gpu_used) {
    stats_.rows_processed += rows;
    stats_.bytes_scanned  += bytes;
    if (gpu_used) ++stats_.gpu_ops;
    else          ++stats_.cpu_fallback_ops;
}

// ---------------------------------------------------------------------------
// scan
// ---------------------------------------------------------------------------

GPUQueryAccelerator::ScanResult
GPUQueryAccelerator::scan(const std::vector<Row>& rows, FilterFn filter) {
    ScanResult result;
    result.rows_scanned = rows.size();

    // Determine path ---------------------------------------------------------
    bool use_gpu = shouldUseGPU(rows.size());
    result.used_gpu = use_gpu;

    // GPU path stub: when THEMIS_ENABLE_CUDA / THEMIS_ENABLE_HIP is defined,
    // copy rows to device, run a Thrust::copy_if / cub::DeviceSelect kernel,
    // copy results back.  For now we fall through to the CPU implementation.
    (void)use_gpu;

    // CPU sequential scan ----------------------------------------------------
    for (const auto& row : rows) {
        if (!filter || filter(row)) {
            result.rows.push_back(row);
        }
    }
    result.rows_passed = result.rows.size();

    // Stats ------------------------------------------------------------------
    uint64_t bytes = 0;
    for (const auto& r : rows) bytes += r.data.size();
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_scans;
    recordOp(rows.size(), bytes, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// sort
// ---------------------------------------------------------------------------

GPUQueryAccelerator::SortResult
GPUQueryAccelerator::sort(std::vector<Row> rows, KeyFn key_fn, SortOrder order) {
    SortResult result;
    bool use_gpu = shouldUseGPU(rows.size());
    result.used_gpu = use_gpu;

    // GPU stub: would copy IDs + keys to device, run Thrust stable_sort_by_key,
    // gather rows back.  CPU path:
    std::stable_sort(rows.begin(), rows.end(),
        [&](const Row& a, const Row& b) {
            double ka = key_fn(a);
            double kb = key_fn(b);
            return (order == SortOrder::ASC) ? ka < kb : ka > kb;
        });
    result.rows = std::move(rows);

    uint64_t bytes = 0;
    for (const auto& r : result.rows) bytes += r.data.size();
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_sorts;
    recordOp(result.rows.size(), bytes, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// aggregate
// ---------------------------------------------------------------------------

GPUQueryAccelerator::AggResult
GPUQueryAccelerator::aggregate(const std::vector<Row>& rows,
                               AggFunc                  func,
                               KeyFn                    value_fn) {
    AggResult result;
    if (rows.empty()) return result;

    bool use_gpu = shouldUseGPU(rows.size());
    result.used_gpu = use_gpu;
    result.count    = rows.size();

    // GPU stub: would use cub::DeviceReduce.  CPU sequential path:
    double sum = 0.0;
    double mn  = std::numeric_limits<double>::max();
    double mx  = std::numeric_limits<double>::lowest();

    for (const auto& row : rows) {
        double v = value_fn(row);
        sum += v;
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }

    switch (func) {
        case AggFunc::SUM:   result.value = sum;                         break;
        case AggFunc::COUNT: result.value = static_cast<double>(rows.size()); break;
        case AggFunc::MIN:   result.value = mn;                          break;
        case AggFunc::MAX:   result.value = mx;                          break;
        case AggFunc::AVG:   result.value = sum / static_cast<double>(rows.size()); break;
    }

    uint64_t bytes = 0;
    for (const auto& r : rows) bytes += r.data.size();
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_aggregates;
    recordOp(rows.size(), bytes, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// hashJoin
// ---------------------------------------------------------------------------

GPUQueryAccelerator::JoinResult
GPUQueryAccelerator::hashJoin(const std::vector<Row>& left,
                               const std::vector<Row>& right,
                               JoinKeyFn               left_key,
                               JoinKeyFn               right_key) {
    JoinResult result;
    if (left.empty() || right.empty()) return result;

    bool use_gpu = shouldUseGPU(left.size() + right.size());
    result.used_gpu = use_gpu;

    // GPU stub: would use a parallel hash join kernel.  CPU path uses
    // an unordered_multimap on the smaller side:
    const std::vector<Row>* build_side  = &left;
    const std::vector<Row>* probe_side  = &right;
    JoinKeyFn                build_key  = left_key;
    JoinKeyFn                probe_key  = right_key;
    bool swapped = false;

    if (right.size() < left.size()) {
        std::swap(build_side, probe_side);
        std::swap(build_key,  probe_key);
        swapped = true;
    }

    std::unordered_multimap<uint64_t, const Row*> ht;
    ht.reserve(build_side->size());
    for (const auto& row : *build_side) {
        ht.emplace(build_key(row), &row);
    }

    for (const auto& row : *probe_side) {
        uint64_t k = probe_key(row);
        auto [beg, end] = ht.equal_range(k);
        for (auto it = beg; it != end; ++it) {
            if (!swapped)
                result.pairs.emplace_back(*it->second, row);
            else
                result.pairs.emplace_back(row, *it->second);
        }
    }

    uint64_t bytes = 0;
    for (const auto& r : left)  bytes += r.data.size();
    for (const auto& r : right) bytes += r.data.size();
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_joins;
    recordOp(left.size() + right.size(), bytes, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------

GPUQueryAccelerator::Stats GPUQueryAccelerator::getStats() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return stats_;
}

void GPUQueryAccelerator::resetStats() {
    std::lock_guard<std::mutex> lk(mutex_);
    stats_ = Stats{};
}

} // namespace gpu
} // namespace themis
