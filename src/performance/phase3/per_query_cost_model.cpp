/**
 * @file per_query_cost_model.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Per-Query Cost Model – implementation
// Integrates hardware cycle measurement (HardwareCycleCounter) with
// OptimizerCostModel to calibrate cost constants from real execution data.

#include "performance/phase3/per_query_cost_model.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <stdexcept>

namespace themis {
namespace performance {
namespace phase3 {

// =============================================================
// PerQueryCostModel
// =============================================================

PerQueryCostModel::PerQueryCostModel() {
    records_.reserve(MAX_RECORDS);
}

PerQueryCostModel::~PerQueryCostModel() = default;

// -----------------------------------------------------------------
// beginQuery – returns a QueryGuard that auto-records on destruction
// -----------------------------------------------------------------

PerQueryCostModel::QueryGuard
PerQueryCostModel::beginQuery(const std::string& query_type,
                               double             estimated_cost) noexcept {
    // Validate estimated_cost
    if (estimated_cost < 0.0) {
        // Log warning: negative cost estimates are invalid
        return QueryGuard(*this, query_type, 0.0);
    }
    return QueryGuard(*this, query_type, estimated_cost);
}

// -----------------------------------------------------------------
// pushRecord – called from QueryGuard::end()
// -----------------------------------------------------------------

void PerQueryCostModel::pushRecord(QueryCostRecord record) noexcept {
    // Validate record: execution time must be non-negative
    // (cycles_elapsed is uint64_t and is always non-negative by construction)
    if (record.execution_time_ms < 0.0) {
        return;  // Ignore invalid records
    }
    
    std::lock_guard<std::mutex> lock(mutex_);

    if (records_.size() < MAX_RECORDS) {
        records_.push_back(std::move(record));
    } else {
        // Rolling overwrite: wrap-around ring
        size_t pos = write_pos_.fetch_add(1, std::memory_order_relaxed) % MAX_RECORDS;
        records_[pos] = std::move(record);
    }

    total_queries_.fetch_add(1, std::memory_order_relaxed);
}

void PerQueryCostModel::reset() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
    total_queries_.store(0, std::memory_order_relaxed);
    write_pos_.store(0, std::memory_order_relaxed);
}

// -----------------------------------------------------------------
// getRecentRecords
// -----------------------------------------------------------------

std::vector<QueryCostRecord>
PerQueryCostModel::getRecentRecords(size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (records_.empty()) {
        return {};
    }

    size_t count = std::min(limit, records_.size());

    // Has the ring buffer rolled over?
    bool has_rolled = total_queries_.load(std::memory_order_relaxed) > MAX_RECORDS;

    if (!has_rolled) {
        // Not yet wrapped: vector is in insertion order; return the tail.
        size_t start = records_.size() > count ? records_.size() - count : 0;
        return std::vector<QueryCostRecord>(
            records_.begin() + static_cast<std::ptrdiff_t>(start),
            records_.end());
    }

    // Wrapped: write_pos_ % MAX_RECORDS is the index of the NEXT write slot.
    // The most recently written slot is at (write_pos_ - 1) % MAX_RECORDS,
    // the slot before that at (write_pos_ - 2) % MAX_RECORDS, etc.
    // Return the most recent 'count' records in chronological order.
    size_t next_write = write_pos_.load(std::memory_order_relaxed) % MAX_RECORDS;
    std::vector<QueryCostRecord> result;
    result.reserve(count);
    for (size_t i = count; i > 0; --i) {
        size_t pos = (next_write + MAX_RECORDS - i) % MAX_RECORDS;
        result.push_back(records_[pos]);
    }
    return result;
}

// -----------------------------------------------------------------
// getCalibrationFactors
//
// Derives updated OptimizerCostModel constant values from recorded
// execution history.  Uses averaged measurements across all recorded
// queries to compute:
//   cpuCostPerRow  = avg_cycles / (rows_processed * cpu_freq_hz * 1e3)
//   pageReadCost   = avg_time_ms / pages_read  (when pages_read > 0)
//
// All computations include bounds checking to ensure fail-closed behavior
// (reject invalid parameter combinations, do not degrade silently).
// -----------------------------------------------------------------

std::unordered_map<std::string, double>
PerQueryCostModel::getCalibrationFactors(
    const OptimizerCostModel::CostConstants* current) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::unordered_map<std::string, double> factors;

    if (records_.empty()) {
        return factors;
    }

    // Derive cpuCostPerRow from records that have row data
    double total_time_per_row = 0.0;
    size_t row_samples = 0;

    double total_time_per_page = 0.0;
    size_t page_samples = 0;

    const uint64_t cpu_hz = HardwareCycleCounter::cpu_frequency_hz();
    const double   ms_per_cycle = cpu_hz > 0
                                    ? 1000.0 / static_cast<double>(cpu_hz)
                                    : 0.0;

    for (const auto& r : records_) {
        // Validate record: execution_time_ms must be positive
        if (r.execution_time_ms <= 0.0) continue;
        
        if (r.rows_processed > 0) {
            double time_ms = ms_per_cycle > 0.0
                               ? static_cast<double>(r.cycles_elapsed) * ms_per_cycle
                               : r.execution_time_ms;
            total_time_per_row += time_ms / static_cast<double>(r.rows_processed);
            ++row_samples;
        }

        if (r.pages_read > 0) {
            total_time_per_page += r.execution_time_ms / static_cast<double>(r.pages_read);
            ++page_samples;
        }
    }

    if (row_samples > 0) {
        double avg_time_per_row = total_time_per_row / static_cast<double>(row_samples);
        // Map ms/row to a dimensionless cost unit consistent with OptimizerCostModel
        // (OptimizerCostModel defaults: cpuCostPerRow = 0.01)
        // Scale: 1 cost unit ≈ 0.01 ms; clamp to [1e-6, 1.0]
        double calibrated = std::max(1e-6, std::min(1.0, avg_time_per_row / 0.01));
        factors["cpuCostPerRow"] = calibrated;
    }

    if (page_samples > 0) {
        double avg_time_per_page = total_time_per_page / static_cast<double>(page_samples);
        // Default pageReadCost = 1.0 ≈ 1 ms sequential page read
        // Clamp to [0.001, 100.0] to avoid extreme outliers
        double calibrated = std::max(0.001, std::min(100.0, avg_time_per_page));
        factors["pageReadCost"] = calibrated;
    }

    // ---- GPU threshold calibration from serialization feedback ----
    //
    // When records using the GPU_VRAM path show that serialization overhead is
    // disproportionately high (> 50 % of total execution time), the GPU breakeven
    // threshold should be raised so the advisor graduates to GPU only for larger
    // batches.  Conversely, when CPU_SINGLE records show negligible overhead, the
    // msgpack threshold may be safely lowered.

    using ExecutionPath = OptimizerCostModel::SerializationAdvice::ExecutionPath;

    double gpu_serial_ratio_sum    = 0.0;
    size_t gpu_serial_samples      = 0;
    double cpu_single_serial_ratio = 0.0;
    size_t cpu_single_samples      = 0;

    for (const auto& r : records_) {
        if (r.execution_time_ms <= 0.0) { continue; }
        
        // Validate serialization_time_ms is reasonable
        if (r.serialization_time_ms < 0.0 || r.serialization_time_ms > r.execution_time_ms * 10) {
            continue;  // Skip records with invalid serialization time
        }
        
        const double ratio = r.serialization_time_ms / r.execution_time_ms;
        if (r.exec_path_used == ExecutionPath::GPU_VRAM) {
            gpu_serial_ratio_sum += ratio;
            ++gpu_serial_samples;
        } else if (r.exec_path_used == ExecutionPath::CPU_SINGLE) {
            cpu_single_serial_ratio += ratio;
            ++cpu_single_samples;
        }
    }

    if (gpu_serial_samples >= 5) {
        const double avg_gpu_ratio =
            gpu_serial_ratio_sum / static_cast<double>(gpu_serial_samples);
        if (avg_gpu_ratio > 0.5) {
            // GPU serialisation dominates: raise the low threshold by 25 %
            // (capped between 10k and 2M rows so the advisor stays reasonable).
            // Use the currently configured threshold (if provided) as the base so
            // repeated calibration calls converge instead of oscillating.
            const double base_low = current
                ? static_cast<double>(current->gpu_row_threshold_low)
                : 50'000.0;
            const double raised = std::min(2'000'000.0, base_low * 1.25);
            factors["gpu_row_threshold_low"] = raised;
        }
    }

    if (cpu_single_samples >= 5) {
        const double avg_cpu_ratio =
            cpu_single_serial_ratio / static_cast<double>(cpu_single_samples);
        // If CPU_SINGLE serialisation is very cheap, the binary threshold can be
        // pushed down slightly so binary format kicks in sooner.
        if (avg_cpu_ratio < 0.05) {
            const double base_msgpack = current
                ? static_cast<double>(current->msgpack_row_threshold)
                : 1'000.0;
            const double lowered = std::max(100.0, base_msgpack * 0.8);
            factors["msgpack_row_threshold"] = lowered;
        }
    }

    return factors;
}

void PerQueryCostModel::calibrate(OptimizerCostModel& model) const {
    // Pass the model's current constants so threshold adjustments compound
    // correctly across successive calibration calls.
    auto factors = getCalibrationFactors(&model.getConstants());
    if (!factors.empty()) {
        // calibrateCosts expects std::map; convert from unordered_map
        std::map<std::string, double> ordered_factors(factors.begin(), factors.end());
        model.calibrateCosts(ordered_factors);
    }
}

// -----------------------------------------------------------------
// getStats
// -----------------------------------------------------------------

PerQueryCostModel::Stats PerQueryCostModel::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    Stats s{};
    s.total_queries = total_queries_.load(std::memory_order_relaxed);

    if (records_.empty()) {
        return s;
    }

    std::vector<double> times_ms;
    times_ms.reserve(records_.size());

    std::unordered_map<std::string, double> type_time_sum;
    std::unordered_map<std::string, size_t> type_count;

    double cost_ratio_sum = 0.0;
    size_t cost_ratio_samples = 0;

    for (const auto& r : records_) {
        // Validate execution time
        if (r.execution_time_ms < 0.0) continue;
        
        times_ms.push_back(r.execution_time_ms);
        type_time_sum[r.query_type] += r.execution_time_ms;
        type_count[r.query_type]++;

        if (r.estimated_cost > 0.0 && r.cost_ratio > 0.0) {
            cost_ratio_sum += r.cost_ratio;
            ++cost_ratio_samples;
        }
    }

    if (times_ms.empty()) {
        return s;
    }

    // Average
    double sum = std::accumulate(times_ms.begin(), times_ms.end(), 0.0);
    s.avg_execution_time_ms = sum / static_cast<double>(times_ms.size());

    s.avg_cost_ratio = cost_ratio_samples > 0
                         ? cost_ratio_sum / static_cast<double>(cost_ratio_samples)
                         : 1.0;

    // Percentiles
    std::sort(times_ms.begin(), times_ms.end());
    auto percentile = [&]([[maybe_unused]] double pct) -> double {
        if (times_ms.empty()) return 0.0;
        size_t idx = static_cast<size_t>(pct * static_cast<double>(times_ms.size() - 1));
        return times_ms[idx];
    };
    s.p50_execution_time_ms = percentile(0.50);
    s.p95_execution_time_ms = percentile(0.95);

    for (const auto& [type, total] : type_time_sum) {
        s.per_type_avg_time_ms[type] = total / static_cast<double>(type_count.at(type));
        s.per_type_count[type]       = type_count.at(type);
    }

    return s;
}

// =============================================================
// QueryGuard
// =============================================================

PerQueryCostModel::QueryGuard::QueryGuard(PerQueryCostModel& model,
                                           std::string        query_type,
                                           double             estimated_cost) noexcept
    : model_(&model)
    , query_type_(std::move(query_type))
    , estimated_cost_(estimated_cost)
    , start_cycles_(HardwareCycleCounter::cpu_cycles())
    , start_wall_(std::chrono::steady_clock::now())
    , ended_(false)
{}

PerQueryCostModel::QueryGuard::QueryGuard(QueryGuard&& other) noexcept
    : model_(other.model_)
    , query_type_(std::move(other.query_type_))
    , estimated_cost_(other.estimated_cost_)
    , start_cycles_(other.start_cycles_)
    , start_wall_(other.start_wall_)
    , ended_(other.ended_)
{
    other.ended_ = true; // prevent double-record in moved-from guard
}

PerQueryCostModel::QueryGuard::~QueryGuard() noexcept {
    if (!ended_) {
        end();
    }
}

void PerQueryCostModel::QueryGuard::end(size_t rows_processed,
                                         size_t pages_read) noexcept {
    if (ended_ || model_ == nullptr) {
        return;
    }
    ended_ = true;

    uint64_t end_cycles = HardwareCycleCounter::rdtscp();
    auto     now        = std::chrono::steady_clock::now();

    uint64_t cycles_elapsed = (end_cycles >= start_cycles_)
                                ? (end_cycles - start_cycles_)
                                : 0;  // guard against counter wrap on very short paths

    double execution_time_ms =
        std::chrono::duration<double, std::milli>(now - start_wall_).count();

    // Compute actual cost (simple model: ms * scaling factor)
    double actual_cost = execution_time_ms;
    double cost_ratio  = (actual_cost > 0.0 && estimated_cost_ > 0.0)
                           ? estimated_cost_ / actual_cost
                           : 1.0;

    QueryCostRecord rec;
    rec.query_type       = query_type_;
    rec.cycles_elapsed   = cycles_elapsed;
    rec.execution_time_ms = execution_time_ms;
    rec.rows_processed   = rows_processed;
    rec.pages_read       = pages_read;
    rec.estimated_cost   = estimated_cost_;
    rec.cost_ratio       = cost_ratio;

    model_->pushRecord(std::move(rec));
}


} // namespace phase3
} // namespace performance
} // namespace themis
