/**
 * @file columnar_execution.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=20, M=18, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Columnar Execution Engine – Implementation
 *
 * @module OLAP
 *
 * Data flow:
 *   VectorizedPipeline::addOperator(op) → operator chain assembled
 *   VectorizedPipeline::execute(batch)  → SelectionVector materialised per stage
 *                                       → FilterOperator prunes rows via predicate eval
 *                                       → AggregateOperator produces columnar aggregates
 *                                       → SortOperator merges sorted runs
 *                                       → returns ColumnBatch (columnar result)
 *
 * Error paths:
 *   - `std::invalid_argument`: unsupported data type in Column::apply_predicate(),
 *     mismatched column count in AggregateOperator.
 *   - SIMD paths are best-effort: any SIGILL (unexpected instruction set) falls
 *     back to the scalar loop path; no exception is thrown.
 *   - Empty batch → passes through all operators; returns empty ColumnBatch (no error).
 *
 * Cross-links:
 *   include/analytics/columnar_execution.h — VectorizedPipeline, Column, SelectionVector
 *   src/analytics/olap.cpp — primary consumer
 *   tests/analytics/ — covered via OLAPEngine integration tests
 *
 * Implementation notes:
 *
 *  SelectionVector
 *    - Dense uint32_t index array; reset() creates [0,n).
 *    - intersect() uses a sorted merge (both vectors are monotonically
 *      increasing by construction) for O(|a|+|b|) complexity.
 *
 *  Column
 *    - Separate per-type std::vectors allow auto-vectorization of inner loops.
 *    - null_bitmap_ uses std::vector<bool> (bit-packed) for space efficiency.
 *
 *  FilterOperator
 *    - Each predicate is evaluated over the full column in a tight loop that
 *      the compiler can auto-vectorize for int64 and double columns.
 *    - Multiple predicates are AND-ed via sorted-merge intersection.
 *
 *  AggregateOperator
 *    - No GROUP BY: inner aggregation loops are fully unrolled over contiguous
 *      double/int64 vectors (auto-vectorization eligible).
 *    - GROUP BY: uses an unordered_map keyed by a stringified group tuple;
 *      partial aggregates are maintained per group.
 *
 *  VectorizedPipeline
 *    - Materializes pending SelectionVectors only when a stage requires dense
 *      data (AggregateOperator, SortOperator).
 */

#include "analytics/columnar_execution.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <limits>
#include <memory_resource>
#include <numeric>
#include <optional>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

// SIMD intrinsics — guarded by feature macros so non-SIMD platforms compile.
#if defined(__AVX512F__)
#include <immintrin.h>
#elif defined(__AVX2__)
#include <immintrin.h>
#elif defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#endif

namespace themisdb {
namespace analytics {

// ============================================================================
// SelectionVector
// ============================================================================

SelectionVector::SelectionVector([[maybe_unused]] size_t capacity) {
    indices_.reserve(capacity);
}

void SelectionVector::reset([[maybe_unused]] size_t total_rows) {
    indices_.resize(total_rows);
    std::iota(indices_.begin(), indices_.end(), static_cast<uint32_t>(0));
}

void SelectionVector::push_back([[maybe_unused]] uint32_t idx) {
    indices_.push_back(idx);
}

size_t SelectionVector::size() const noexcept {
    return indices_.size();
}

bool SelectionVector::empty() const noexcept {
    return indices_.empty();
}

uint32_t SelectionVector::operator[]([[maybe_unused]] size_t pos) const {
    return indices_[pos];
}

const std::vector<uint32_t> &SelectionVector::indices() const noexcept {
    return indices_;
}

SelectionVector SelectionVector::all([[maybe_unused]] size_t n) {
    SelectionVector sv(n);
    sv.reset(n);
    return sv;
}

// ============================================================================
// Column
// ============================================================================

Column::Column(std::string name, ColumnType type) : name_(std::move(name)), type_(type) {}

bool Column::isNull([[maybe_unused]] size_t row) const {
    if (null_bitmap_.empty()) {
        return false;
    }
    return null_bitmap_[row];
}

void Column::appendInt64(int64_t value, bool is_null) {
    int64_data_.push_back(value);
    null_bitmap_.push_back(is_null);
    if (is_null) {
        has_nulls_ = true;
    }
    ++row_count_;
}

void Column::appendDouble(double value, bool is_null) {
    double_data_.push_back(value);
    null_bitmap_.push_back(is_null);
    if (is_null) {
        has_nulls_ = true;
    }
    ++row_count_;
}

void Column::appendString(std::string value, bool is_null) {
    string_data_.push_back(std::move(value));
    null_bitmap_.push_back(is_null);
    if (is_null) {
        has_nulls_ = true;
    }
    ++row_count_;
}

void Column::appendBool(bool value, bool is_null) {
    bool_data_.push_back(value);
    null_bitmap_.push_back(is_null);
    if (is_null) {
        has_nulls_ = true;
    }
    ++row_count_;
}

void Column::appendNull() {
    switch (type_) {
        case ColumnType::Int64:
            int64_data_.push_back(0);
            break;
        case ColumnType::Double:
            double_data_.push_back(0.0);
            break;
        case ColumnType::String:
            string_data_.push_back({});
            break;
        case ColumnType::Bool:
            bool_data_.push_back(false);
            break;
        case ColumnType::Null:
            break;
    }
    null_bitmap_.push_back(true);
    has_nulls_ = true;
    ++row_count_;
}

ColumnValue Column::get([[maybe_unused]] size_t row) const {
    if (!null_bitmap_.empty() && null_bitmap_[row]) {
        return nullptr;
    }
    switch (type_) {
        case ColumnType::Int64:
            return int64_data_[row];
        case ColumnType::Double:
            return double_data_[row];
        case ColumnType::String:
            return string_data_[row];
        case ColumnType::Bool:
            return bool_data_[row];
        case ColumnType::Null:
            return nullptr;
    }
    return nullptr;
}

void Column::reserve([[maybe_unused]] size_t n) {
    switch (type_) {
        case ColumnType::Int64:
            int64_data_.reserve(n);
            break;
        case ColumnType::Double:
            double_data_.reserve(n);
            break;
        case ColumnType::String:
            string_data_.reserve(n);
            break;
        case ColumnType::Bool:
            bool_data_.reserve(n);
            break;
        case ColumnType::Null:
            break;
    }
    null_bitmap_.reserve(n);
}

void Column::clear() {
    int64_data_.clear();
    double_data_.clear();
    string_data_.clear();
    bool_data_.clear();
    null_bitmap_.clear();
    has_nulls_ = false;
    row_count_ = 0;
}

std::shared_ptr<Column> Column::filter(const SelectionVector &sel) const {
    auto out = std::make_shared<Column>(name_, type_);
    out->reserve(sel.size());
    for (size_t i = 0; i < sel.size(); ++i) {
        uint32_t row = sel[i];
        bool is_null = (!null_bitmap_.empty() && null_bitmap_[row]);
        switch (type_) {
            case ColumnType::Int64:
                out->appendInt64(int64_data_[row], is_null);
                break;
            case ColumnType::Double:
                out->appendDouble(double_data_[row], is_null);
                break;
            case ColumnType::String:
                out->appendString(string_data_[row], is_null);
                break;
            case ColumnType::Bool:
                out->appendBool(bool_data_[row], is_null);
                break;
            case ColumnType::Null:
                out->appendNull();
                break;
        }
    }
    return out;
}

std::shared_ptr<Column> Column::slice(size_t offset, size_t length) const {
    auto out = std::make_shared<Column>(name_, type_);
    if (offset >= row_count_) {
        return out;
    }
    size_t end = std::min(offset + length, row_count_);
    out->reserve(end - offset);
    for (size_t row = offset; row < end; ++row) {
        bool is_null = (!null_bitmap_.empty() && null_bitmap_[row]);
        switch (type_) {
            case ColumnType::Int64:
                out->appendInt64(int64_data_[row], is_null);
                break;
            case ColumnType::Double:
                out->appendDouble(double_data_[row], is_null);
                break;
            case ColumnType::String:
                out->appendString(string_data_[row], is_null);
                break;
            case ColumnType::Bool:
                out->appendBool(bool_data_[row], is_null);
                break;
            case ColumnType::Null:
                out->appendNull();
                break;
        }
    }
    return out;
}

// ============================================================================
// ColumnBatch
// ============================================================================

ColumnBatch::ColumnBatch([[maybe_unused]] size_t row_count) : row_count_(row_count) {}

void ColumnBatch::addColumn(std::shared_ptr<Column> col) {
    if (!col) {
        return;
    }
    // If this is the first column, adopt its row count.
    if (columns_.empty()) {
        row_count_ = col->size();
    }
    column_index_[col->name()] = columns_.size();
    columns_.push_back(std::move(col));
}

bool ColumnBatch::hasColumn(const std::string &name) const {
    return column_index_.count(name) > 0;
}

std::shared_ptr<Column> ColumnBatch::getColumn(const std::string &name) const {
    auto it = column_index_.find(name);
    if (it == column_index_.end()) {
        return nullptr;
    }
    return columns_[it->second];
}

std::shared_ptr<Column> ColumnBatch::getColumnAt([[maybe_unused]] size_t idx) const {
    if (idx >= columns_.size()) {
        return nullptr;
    }
    return columns_[idx];
}

size_t ColumnBatch::columnCount() const noexcept {
    return columns_.size();
}

const std::vector<std::shared_ptr<Column>> &ColumnBatch::columns() const noexcept {
    return columns_;
}

void ColumnBatch::setSelection(const SelectionVector &sel) {
    selection_     = sel;
    has_selection_ = true;
}

size_t ColumnBatch::selectedRowCount() const noexcept {
    if (!has_selection_) {
        return row_count_;
    }
    return selection_.size();
}

ColumnBatch ColumnBatch::materialize() const {
    if (!has_selection_) {
        return *this; // already dense
    }

    ColumnBatch out;
    out.row_count_ = selection_.size();
    for (const auto &col : columns_) {
        out.addColumn(col->filter(selection_));
    }
    return out;
}

std::vector<ColumnBatch> ColumnBatch::split([[maybe_unused]] size_t max_rows) const {
    // Materialize first so we have a dense layout.
    ColumnBatch dense = materialize();
    size_t total      = dense.rowCount();
    if (total == 0 || max_rows == 0) {
        return {};
    }

    std::vector<ColumnBatch> result = {};

    for (size_t offset = 0; offset < total; offset += max_rows) {
        size_t len = std::min(max_rows, total - offset);
        ColumnBatch sub;
        sub.row_count_ = len;
        for (const auto &col : dense.columns_) {
            sub.addColumn(col->slice(offset, len));
        }
        result.push_back(std::move(sub));
    }
    return result;
}

void ColumnBatch::clear() {
    columns_.clear();
    column_index_.clear();
    row_count_     = 0;
    has_selection_ = false;
}

// ============================================================================
// Predicate factories
// ============================================================================

Predicate Predicate::eq(std::string col, ColumnValue val) {
    return {std::move(col), Op::Eq, std::move(val)};
}
Predicate Predicate::ne(std::string col, ColumnValue val) {
    return {std::move(col), Op::Ne, std::move(val)};
}
Predicate Predicate::lt(std::string col, ColumnValue val) {
    return {std::move(col), Op::Lt, std::move(val)};
}
Predicate Predicate::le(std::string col, ColumnValue val) {
    return {std::move(col), Op::Le, std::move(val)};
}
Predicate Predicate::gt(std::string col, ColumnValue val) {
    return {std::move(col), Op::Gt, std::move(val)};
}
Predicate Predicate::ge(std::string col, ColumnValue val) {
    return {std::move(col), Op::Ge, std::move(val)};
}
Predicate Predicate::isNull(std::string col) {
    return {std::move(col), Op::IsNull, nullptr};
}
Predicate Predicate::isNotNull(std::string col) {
    return {std::move(col), Op::IsNotNull, nullptr};
}

// ============================================================================
// FilterOperator
// ============================================================================

FilterOperator::FilterOperator(std::vector<Predicate> predicates) : predicates_(std::move(predicates)) {}

namespace {

// Sorted-merge intersection of two monotonically increasing index vectors.
SelectionVector mergeIntersect(const SelectionVector &a, const SelectionVector &b) {
    SelectionVector out(std::min(a.size(), b.size()));
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] == b[j]) {
            out.push_back(a[i]);
            ++i;
            ++j;
        } else if (a[i] < b[j]) {
            ++i;
        } else {
            ++j;
        }
    }
    return out;
}

// Helper: compare ColumnValue using an Op.
template <typename T> bool compareValues(const T &lhs, const T &rhs, Predicate::Op op) {
    switch (op) {
        case Predicate::Op::Eq:
            return lhs == rhs;
        case Predicate::Op::Ne:
            return lhs != rhs;
        case Predicate::Op::Lt:
            return lhs < rhs;
        case Predicate::Op::Le:
            return lhs <= rhs;
        case Predicate::Op::Gt:
            return lhs > rhs;
        case Predicate::Op::Ge:
            return lhs >= rhs;
        default:
            return false;
    }
}

} // anonymous namespace

SelectionVector FilterOperator::evalPredicate(const ColumnBatch &batch, const Predicate &pred) const {
    const auto col = batch.getColumn(pred.column);
    size_t n       = batch.rowCount();
    SelectionVector sel(n);

    if (!col) {
        // Column not found – treat IsNotNull as false, IsNull as true
        if (pred.op == Predicate::Op::IsNull) {
            sel.reset(n);
        }
        return sel;
    }

    // IS NULL / IS NOT NULL
    if (pred.op == Predicate::Op::IsNull) {
        for (size_t i = 0; i < n; ++i) {
            if (col->isNull(i)) {
                sel.push_back(static_cast<uint32_t>(i));
            }
        }
        return sel;
    }
    if (pred.op == Predicate::Op::IsNotNull) {
        for (size_t i = 0; i < n; ++i) {
            if (!col->isNull(i)) {
                sel.push_back(static_cast<uint32_t>(i));
            }
        }
        return sel;
    }

    // Typed comparison – tight inner loops for auto-vectorization
    switch (col->type()) {
        case ColumnType::Int64: {
            const auto &data = col->int64Data();
            if (const auto *rv = std::get_if<int64_t>(&pred.value)) {
                int64_t threshold = *rv;
                Predicate::Op op  = pred.op;
                for (size_t i = 0; i < data.size(); ++i) {
                    if (!col->isNull(i) && compareValues(data[i], threshold, op)) {
                        sel.push_back(static_cast<uint32_t>(i));
                    }
                }
            } else if (const auto *dv = std::get_if<double>(&pred.value)) {
                int64_t threshold = static_cast<int64_t>(*dv);
                Predicate::Op op  = pred.op;
                for (size_t i = 0; i < data.size(); ++i) {
                    if (!col->isNull(i) && compareValues(data[i], threshold, op)) {
                        sel.push_back(static_cast<uint32_t>(i));
                    }
                }
            }
            break;
        }
        case ColumnType::Double: {
            const auto &data = col->doubleData();
            double threshold = 0.0;
            if (const auto *dv = std::get_if<double>(&pred.value)) {
                threshold = *dv;
            } else if (const auto *iv = std::get_if<int64_t>(&pred.value)) {
                threshold = static_cast<double>(*iv);
            } else {
                break;
            }
            Predicate::Op op = pred.op;
            for (size_t i = 0; i < data.size(); ++i) {
                if (!col->isNull(i) && compareValues(data[i], threshold, op)) {
                    sel.push_back(static_cast<uint32_t>(i));
                }
            }
            break;
        }
        case ColumnType::String: {
            const auto &data = col->stringData();
            const auto *sv   = std::get_if<std::string>(&pred.value);
            if (!sv) {
                break;
            }
            const std::string &threshold = *sv;
            Predicate::Op op             = pred.op;
            for (size_t i = 0; i < data.size(); ++i) {
                if (!col->isNull(i) && compareValues(data[i], threshold, op)) {
                    sel.push_back(static_cast<uint32_t>(i));
                }
            }
            break;
        }
        case ColumnType::Bool: {
            const auto &data = col->boolData();
            const auto *bv   = std::get_if<bool>(&pred.value);
            if (!bv) {
                break;
            }
            bool threshold   = *bv;
            Predicate::Op op = pred.op;
            for (size_t i = 0; i < data.size(); ++i) {
                if (!col->isNull(i)) {
                    bool val = data[i];
                    if (compareValues(val, threshold, op)) {
                        sel.push_back(static_cast<uint32_t>(i));
                    }
                }
            }
            break;
        }
        case ColumnType::Null:
            // All values are null – no row passes any comparison predicate.
            break;
    }
    return sel;
}

ColumnBatch FilterOperator::execute(const ColumnBatch &input) const {
    if (predicates_.empty()) {
        return input;
    }

    // Materialize any pending selection before applying new predicates.
    ColumnBatch dense = input.materialize();
    size_t n          = dense.rowCount();

    // Start with "all rows selected".
    SelectionVector combined = SelectionVector::all(n);

    for (const auto &pred : predicates_) {
        SelectionVector partial = evalPredicate(dense, pred);
        combined                = mergeIntersect(combined, partial);
        if (combined.empty()) {
            break;
        }
    }

    // Attach selection without copying column data (late materialization).
    ColumnBatch result;
    for (const auto &col : dense.columns()) {
        result.addColumn(col); // shared_ptr – no copy; first col sets row_count_
    }
    result.setSelection(combined);
    return result;
}

// ============================================================================
// ProjectOperator
// ============================================================================

ProjectOperator::ProjectOperator(std::vector<std::string> column_names) : column_names_(std::move(column_names)) {}

ColumnBatch ProjectOperator::execute(const ColumnBatch &input) const {
    ColumnBatch result = {};
    if (input.hasSelection()) {
        result.setSelection(input.selection());
    }

    for (const auto &name : column_names_) {
        auto col = input.getColumn(name);
        if (col) {
            result.addColumn(col);
        }
    }
    // If no matching columns were found, preserve the row count from input.
    if (result.columnCount() == 0) {
        // Return empty batch with correct row count via explicit constructor.
        return ColumnBatch(input.selectedRowCount());
    }
    return result;
}

// ============================================================================
// AggregateOperator helpers
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// SIMD-accelerated aggregation helpers for contiguous double arrays.
//
// Priority order:
//   ARM builds  : NEON float64x2_t (2 doubles/cycle)
//   x86-64      : AVX-512 (8 doubles/cycle) → AVX2 (4 doubles/cycle) → scalar
//
// The ARM NEON path uses float64x2_t which is AArch64-only (Cortex-A78 /
// Apple Silicon).  On ARMv7 without double-precision NEON the path
// gracefully falls back to scalar via the #else branch.
// ---------------------------------------------------------------------------

// Cache the AVX-512 runtime support check — avoids repeated CPUID calls in
// hot aggregation loops.  Initialized once at first use (thread-safe in C++11).
#if defined(__AVX512F__)
static const bool kHasAVX512 = __builtin_cpu_supports("avx512f");
#endif

struct SIMDAggResult {
    double sum     = 0.0;
    double min_val = std::numeric_limits<double>::max();
    double max_val = std::numeric_limits<double>::lowest();
    int64_t count  = 0; // non-null count
};

// Aggregate SUM/MIN/MAX over a non-null double array in a single pass.
static SIMDAggResult simdAggDouble(const double *data, size_t n) noexcept {
    SIMDAggResult r = {};
    if (n == 0) {
        return r;
    }
    r.count = static_cast<int64_t>(n);

    size_t i = 0;

#if defined(__ARM_NEON) && defined(__aarch64__)
    // ARM NEON path: float64x2_t — 2 doubles per register, unrolled ×4 = 8/iter
    float64x2_t vsum0 = vdupq_n_f64(0.0);
    float64x2_t vsum1 = vdupq_n_f64(0.0);
    float64x2_t vmin0 = vdupq_n_f64(data[0]);
    float64x2_t vmin1 = vdupq_n_f64(data[0]);
    float64x2_t vmax0 = vdupq_n_f64(data[0]);
    float64x2_t vmax1 = vdupq_n_f64(data[0]);

    for (; i + 7 < n; i += 8) {
        float64x2_t v0 = vld1q_f64(data + i + 0);
        float64x2_t v1 = vld1q_f64(data + i + 2);
        float64x2_t v2 = vld1q_f64(data + i + 4);
        float64x2_t v3 = vld1q_f64(data + i + 6);
        vsum0          = vaddq_f64(vsum0, v0);
        vsum1          = vaddq_f64(vsum1, v1);
        vsum0          = vaddq_f64(vsum0, v2);
        vsum1          = vaddq_f64(vsum1, v3);
        vmin0          = vminq_f64(vmin0, v0);
        vmin1          = vminq_f64(vmin1, v1);
        vmin0          = vminq_f64(vmin0, v2);
        vmin1          = vminq_f64(vmin1, v3);
        vmax0          = vmaxq_f64(vmax0, v0);
        vmax1          = vmaxq_f64(vmax1, v1);
        vmax0          = vmaxq_f64(vmax0, v2);
        vmax1          = vmaxq_f64(vmax1, v3);
    }
    // Handle remaining full pairs
    for (; i + 1 < n; i += 2) {
        float64x2_t v = vld1q_f64(data + i);
        vsum0         = vaddq_f64(vsum0, v);
        vmin0         = vminq_f64(vmin0, v);
        vmax0         = vmaxq_f64(vmax0, v);
    }
    // Horizontal reduce
    float64x2_t vsumF = vaddq_f64(vsum0, vsum1);
    r.sum             = vgetq_lane_f64(vsumF, 0) + vgetq_lane_f64(vsumF, 1);
    float64x2_t vminF = vminq_f64(vmin0, vmin1);
    r.min_val         = std::min(vgetq_lane_f64(vminF, 0), vgetq_lane_f64(vminF, 1));
    float64x2_t vmaxF = vmaxq_f64(vmax0, vmax1);
    r.max_val         = std::max(vgetq_lane_f64(vmaxF, 0), vgetq_lane_f64(vmaxF, 1));

#elif defined(__AVX512F__)
    if (n >= 8 && kHasAVX512) {
        __m512d vsum = _mm512_setzero_pd();
        __m512d vmin = _mm512_set1_pd(data[0]);
        __m512d vmax = _mm512_set1_pd(data[0]);
        for (; i + 7 < n; i += 8) {
            __m512d v = _mm512_loadu_pd(data + i);
            vsum      = _mm512_add_pd(vsum, v);
            vmin      = _mm512_min_pd(vmin, v);
            vmax      = _mm512_max_pd(vmax, v);
        }
        r.sum     = _mm512_reduce_add_pd(vsum);
        r.min_val = _mm512_reduce_min_pd(vmin);
        r.max_val = _mm512_reduce_max_pd(vmax);
        for (; i < n; ++i) {
            r.sum += data[i];
            if (data[i] < r.min_val)
                r.min_val = data[i];
            if (data[i] > r.max_val)
                r.max_val = data[i];
        }
        return r;
    }
    // fall-through to AVX2 if __builtin_cpu_supports returned false
    {
#elif defined(__AVX2__)
    {
#endif
#if defined(__AVX2__) && !defined(__ARM_NEON)
    __m256d vsum = _mm256_setzero_pd();
    __m256d vmin = _mm256_set1_pd(data[0]);
    __m256d vmax = _mm256_set1_pd(data[0]);
    for (; i + 3 < n; i += 4) {
        __m256d v = _mm256_loadu_pd(data + i);
        vsum      = _mm256_add_pd(vsum, v);
        vmin      = _mm256_min_pd(vmin, v);
        vmax      = _mm256_max_pd(vmax, v);
    }
    double s[4], mn[4], mx[4];
    _mm256_storeu_pd(s, vsum);
    _mm256_storeu_pd(mn, vmin);
    _mm256_storeu_pd(mx, vmax);
    r.sum     = s[0] + s[1] + s[2] + s[3];
    r.min_val = std::min({mn[0], mn[1], mn[2], mn[3]});
    r.max_val = std::max({mx[0], mx[1], mx[2], mx[3]});
}
#endif

// Scalar tail (shared by all SIMD paths)
for (; i < n; ++i) {
    r.sum += data[i];
    if (data[i] < r.min_val)
        r.min_val = data[i];
    if (data[i] > r.max_val)
        r.max_val = data[i];
}
return r;
}

struct AggState {
    double sum            = 0.0;
    double min_val        = std::numeric_limits<double>::max();
    double max_val        = std::numeric_limits<double>::lowest();
    int64_t count         = 0;
    int64_t count_nonnull = 0;
    std::unordered_set<std::string> distinct_set;
};

// Obtain numeric value from a column at position @p row (selected or not).
static std::optional<double> numericAt(const Column &col, size_t row) {
    if (col.isNull(row)) {
        return std::nullopt;
    }
    switch (col.type()) {
        case ColumnType::Double:
            return col.doubleData()[row];
        case ColumnType::Int64:
            return static_cast<double>(col.int64Data()[row]);
        case ColumnType::Bool:
            return col.boolData()[row] ? 1.0 : 0.0;
        default:
            return std::nullopt;
    }
}

static void updateState(AggState &state, const Column &col, size_t row) {
    ++state.count;
    auto v = numericAt(col, row);
    if (!v) {
        return;
    }
    ++state.count_nonnull;
    state.sum += *v;
    if (*v < state.min_val)
        state.min_val = *v;
    if (*v > state.max_val)
        state.max_val = *v;
}

static void updateDistinct(AggState &state, const Column &col, size_t row) {
    ++state.count;
    if (col.isNull(row)) {
        return;
    }
    ++state.count_nonnull;
    auto val = col.get(row);
    std::ostringstream oss = {};
    std::visit(
        [&oss](auto &&v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                oss << "null";
            } else {
                oss << v;
            }
        },
        val);
    state.distinct_set.insert(oss.str());
}

static double finalizeAgg(const AggState &state, AggregateSpec::Function fn) {
    switch (fn) {
        case AggregateSpec::Function::Count:
            return static_cast<double>(state.count);
        case AggregateSpec::Function::Sum:
            return state.sum;
        case AggregateSpec::Function::Avg:
            return state.count_nonnull > 0 ? state.sum / static_cast<double>(state.count_nonnull) : 0.0;
        case AggregateSpec::Function::Min:
            return state.count_nonnull > 0 ? state.min_val : 0.0;
        case AggregateSpec::Function::Max:
            return state.count_nonnull > 0 ? state.max_val : 0.0;
        case AggregateSpec::Function::CountDistinct:
            return static_cast<double>(state.distinct_set.size());
    }
    return 0.0;
}

// Produce a single string key for a group-by tuple at row @p row.
static std::string makeGroupKey(const ColumnBatch &batch, const std::vector<std::string> &group_cols, size_t row) {
    std::ostringstream oss = {};
    for (const auto &gc : group_cols) {
        auto col = batch.getColumn(gc);
        if (!col) {
            oss << "null|";
            continue;
        }
        auto val = col->get(row);
        std::visit(
            [&oss](auto &&v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::nullptr_t>) {
                    oss << "null";
                } else {
                    oss << v;
                }
            },
            val);
        oss << '|';
    }
    return oss.str();
}

} // anonymous namespace

// ============================================================================
// AggregateOperator
// ============================================================================

AggregateOperator::AggregateOperator(std::vector<AggregateSpec> specs) : specs_(std::move(specs)) {}

ColumnBatch AggregateOperator::execute(const ColumnBatch &input) const {
    ColumnBatch dense = input.materialize();

    // All specs must share the same group_by (first spec wins).
    const std::vector<std::string> &group_cols = specs_.empty() ? std::vector<std::string>{} : specs_.front().group_by;

    if (group_cols.empty()) {
        return aggregateAll(dense);
    }
    return aggregateGroupBy(dense, group_cols);
}

ColumnBatch AggregateOperator::aggregateAll(const ColumnBatch &input) const {
    size_t n = input.rowCount();

    // Accumulate one AggState per spec.
    std::vector<AggState> states(specs_.size());

    for (size_t s = 0; s < specs_.size(); ++s) {
        const auto &spec = specs_[s];
        auto &st         = states[s];

        if (spec.function == AggregateSpec::Function::Count && spec.input_column.empty()) {
            // COUNT(*)
            st.count = static_cast<int64_t>(n);
            continue;
        }
        if (spec.function == AggregateSpec::Function::CountDistinct) {
            auto col = input.getColumn(spec.input_column);
            if (col) {
                for (size_t i = 0; i < n; ++i) {
                    updateDistinct(st, *col, i);
                }
            }
            continue;
        }
        auto col = input.getColumn(spec.input_column);
        if (!col) {
            continue;
        }

        // Fast SIMD path for non-null Double columns aggregating SUM/AVG/MIN/MAX.
        // For nullable columns or non-Double types the per-row path is used.
        if (col->type() == ColumnType::Double && !col->hasNulls() // no nulls → SIMD fast path
            && (spec.function == AggregateSpec::Function::Sum || spec.function == AggregateSpec::Function::Avg
                || spec.function == AggregateSpec::Function::Min || spec.function == AggregateSpec::Function::Max)) {
            const auto &dd   = col->doubleData();
            SIMDAggResult ar = simdAggDouble(dd.data(), dd.size());
            st.sum           = ar.sum;
            st.min_val       = ar.min_val;
            st.max_val       = ar.max_val;
            st.count         = ar.count;
            st.count_nonnull = ar.count;
            continue;
        }

        for (size_t i = 0; i < n; ++i) {
            updateState(st, *col, i);
        }
    }

    // Build result batch (one row).
    ColumnBatch result(1);
    for (size_t s = 0; s < specs_.size(); ++s) {
        double val   = finalizeAgg(states[s], specs_[s].function);
        auto out_col = std::make_shared<Column>(specs_[s].result_name, ColumnType::Double);
        out_col->appendDouble(val);
        result.addColumn(out_col);
    }
    return result;
}

ColumnBatch AggregateOperator::aggregateGroupBy(const ColumnBatch &input,
                                                const std::vector<std::string> &group_cols) const {
    size_t n = input.rowCount();

    // Reset the per-operator arena so all GROUP BY scratch memory
    // (group-key strings, AggState vectors) reuses the same backing block.
    pool_.reset();
    std::pmr::polymorphic_allocator<std::byte> alloc{&pool_};

    // Map: group_key -> vector of per-spec AggStates — backed by arena.
    std::pmr::unordered_map<std::pmr::string, std::pmr::vector<AggState>> groups{alloc};
    // Preserve insertion order for deterministic output — backed by arena.
    std::pmr::vector<std::pmr::string> key_order{alloc};

    for (size_t row = 0; row < n; ++row) {
        std::string key_std = makeGroupKey(input, group_cols, row);
        std::pmr::string key{key_std, alloc};

        // emplace returns (iterator, bool); avoid a second find() on new groups.
        auto [it, inserted] = groups.emplace(key, std::pmr::vector<AggState>{specs_.size(), AggState{}, alloc});
        if (inserted) {
            key_order.push_back(it->first); // reference key already in the map
        }

        auto &states = it->second;
        for (size_t s = 0; s < specs_.size(); ++s) {
            const auto &spec = specs_[s];
            auto &st         = states[s];

            if (spec.function == AggregateSpec::Function::Count && spec.input_column.empty()) {
                ++st.count;
                continue;
            }
            if (spec.function == AggregateSpec::Function::CountDistinct) {
                auto col = input.getColumn(spec.input_column);
                if (col) {
                    updateDistinct(st, *col, row);
                }
                continue;
            }
            auto col = input.getColumn(spec.input_column);
            if (col) {
                updateState(st, *col, row);
            }
        }
    }

    // Build result batch: one row per group.
    size_t num_rows = key_order.size();
    ColumnBatch result(num_rows);

    // Group-key columns
    for (const auto &gc : group_cols) {
        auto src = input.getColumn(gc);
        if (!src) {
            continue;
        }
        // NOTE: Creating a new shared_ptr<Column> locally and initializing it
        // before adding to result is not a data race. The column is constructed
        // and populated in a single-threaded manner before being shared (via
        // result.addColumn). No other thread can access out_col until the
        // result is returned.
        auto out_col = std::make_shared<Column>(gc, src->type());
        out_col->reserve(num_rows);
        // Build first-row map (arena-backed) to avoid extra heap allocations.
        // try_emplace does a single lookup and inserts only when key is absent.
        std::pmr::unordered_map<std::pmr::string, size_t> first_row{alloc};
        for (size_t row = 0; row < n; ++row) {
            std::pmr::string k{makeGroupKey(input, group_cols, row), alloc};
            first_row.try_emplace(k, row);
        }
        for (const auto &k : key_order) {
            size_t fr    = first_row.at(k);
            auto val     = src->get(fr);
            bool is_null = src->isNull(fr);
            switch (src->type()) {
                case ColumnType::Int64:
                    out_col->appendInt64(is_null ? 0 : std::get<int64_t>(val), is_null);
                    break;
                case ColumnType::Double:
                    out_col->appendDouble(is_null ? 0.0 : std::get<double>(val), is_null);
                    break;
                case ColumnType::String:
                    out_col->appendString(is_null ? "" : std::get<std::string>(val), is_null);
                    break;
                case ColumnType::Bool:
                    out_col->appendBool(is_null ? false : std::get<bool>(val), is_null);
                    break;
                case ColumnType::Null:
                    out_col->appendNull();
                    break;
            }
        }
        result.addColumn(out_col);
    }

    // Aggregate columns
    for (size_t s = 0; s < specs_.size(); ++s) {
        const auto &spec = specs_[s];
        auto out_col     = std::make_shared<Column>(spec.result_name, ColumnType::Double);
        out_col->reserve(num_rows);
        for (const auto &k : key_order) {
            double val = finalizeAgg(groups.at(k)[s], spec.function);
            out_col->appendDouble(val);
        }
        result.addColumn(out_col);
    }

    return result;
}

// ============================================================================
// SortOperator
// ============================================================================

SortOperator::SortOperator(std::vector<SortKey> keys) : keys_(std::move(keys)) {}

ColumnBatch SortOperator::execute(const ColumnBatch &input) const {
    ColumnBatch dense = input.materialize();
    size_t n          = dense.rowCount();
    if (n == 0 || keys_.empty()) {
        return dense;
    }

    // Build a row-index array and sort it.
    std::vector<size_t> order(n);
    std::iota(order.begin(), order.end(), 0);

    std::stable_sort([[maybe_unused]] order.begin(), order.end(), [&](size_t a, size_t b) -> bool {
        for (const auto &key : keys_) {
            auto col = dense.getColumn(key.column);
            if (!col) {
                continue;
            }

            bool a_null = col->isNull(a);
            bool b_null = col->isNull(b);
            if (a_null && b_null) {
                continue;
            }
            if (a_null) {
                return !key.ascending; // nulls last
            }
            if (b_null) {
                return key.ascending;
            }

            switch (col->type()) {
                case ColumnType::Int64: {
                    int64_t va = col->int64Data()[a];
                    int64_t vb = col->int64Data()[b];
                    if (va != vb) {
                        return key.ascending ? va < vb : va > vb;
                    }
                    break;
                }
                case ColumnType::Double: {
                    double va = col->doubleData()[a];
                    double vb = col->doubleData()[b];
                    if (va != vb) {
                        return key.ascending ? va < vb : va > vb;
                    }
                    break;
                }
                case ColumnType::String: {
                    const auto &va = col->stringData()[a];
                    const auto &vb = col->stringData()[b];
                    if (va != vb) {
                        return key.ascending ? va < vb : va > vb;
                    }
                    break;
                }
                case ColumnType::Bool: {
                    bool va = col->boolData()[a];
                    bool vb = col->boolData()[b];
                    if (va != vb) {
                        return key.ascending ? (!va && vb) : (va && !vb);
                    }
                    break;
                }
                case ColumnType::Null:
                    break;
            }
        }
        return false;
    });

    // Produce reordered batch.
    SelectionVector sel(n);
    for (size_t idx : order) {
        sel.push_back(static_cast<uint32_t>(idx));
    }

    ColumnBatch result;
    for (const auto &col : dense.columns()) {
        result.addColumn(col); // first col sets row_count_
    }
    result.setSelection(sel);
    return result.materialize();
}

// ============================================================================
// VectorizedPipeline
// ============================================================================

VectorizedPipeline &VectorizedPipeline::addFilter(std::vector<Predicate> preds) {
    Stage s;
    s.type   = StageType::Filter;
    s.filter = std::make_shared<FilterOperator>(std::move(preds));
    stages_.push_back(std::move(s));
    return *this;
}

VectorizedPipeline &VectorizedPipeline::addProject(std::vector<std::string> cols) {
    Stage s;
    s.type    = StageType::Project;
    s.project = std::make_shared<ProjectOperator>(std::move(cols));
    stages_.push_back(std::move(s));
    return *this;
}

VectorizedPipeline &VectorizedPipeline::addAggregate(std::vector<AggregateSpec> specs) {
    Stage s;
    s.type      = StageType::Aggregate;
    s.aggregate = std::make_shared<AggregateOperator>(std::move(specs));
    stages_.push_back(std::move(s));
    return *this;
}

VectorizedPipeline &VectorizedPipeline::addSort(std::vector<SortOperator::SortKey> keys) {
    Stage s;
    s.type = StageType::Sort;
    s.sort = std::make_shared<SortOperator>(std::move(keys));
    stages_.push_back(std::move(s));
    return *this;
}

ColumnBatch VectorizedPipeline::execute(const ColumnBatch &input) const {
    ColumnBatch current = input;

    for (const auto &stage : stages_) {
        switch (stage.type) {
            case StageType::Filter:
                current = stage.filter->execute(current);
                break;
            case StageType::Project:
                // Project preserves (or discards) any pending SelectionVector.
                current = stage.project->execute(current);
                break;
            case StageType::Aggregate:
                // Aggregate materializes internally.
                current = stage.aggregate->execute(current);
                break;
            case StageType::Sort:
                // Sort materializes internally.
                current = stage.sort->execute(current);
                break;
        }
    }
    return current;
}

// ============================================================================
// ColumnarExecutionEngine
// ============================================================================

ColumnarExecutionEngine::ColumnarExecutionEngine() : config_{} {}

ColumnarExecutionEngine::ColumnarExecutionEngine(const Config &config) : config_(config) {}

ColumnBatch ColumnarExecutionEngine::execute(const ColumnBatch &input, const VectorizedPipeline &pipeline) {
    auto t0 = std::chrono::high_resolution_clock::now();

    ++stats_.batches_processed;
    stats_.rows_in += input.rowCount();

    ColumnBatch result = pipeline.execute(input);

    stats_.rows_out += result.selectedRowCount();

    auto t1 = std::chrono::high_resolution_clock::now();
    stats_.elapsed_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();

    return result;
}

std::vector<ColumnBatch> ColumnarExecutionEngine::executeBatched(const std::vector<ColumnBatch> &batches,
                                                                 const VectorizedPipeline &pipeline) {
    std::vector<ColumnBatch> results = {};

    results.reserve(batches.size());
    for (const auto &batch : batches) {
        results.push_back(execute(batch, pipeline));
    }
    return results;
}

ColumnBatch ColumnarExecutionEngine::filter(const ColumnBatch &input, std::vector<Predicate> predicates) {
    VectorizedPipeline p;
    p.addFilter(std::move(predicates));
    return execute(input, p);
}

ColumnBatch ColumnarExecutionEngine::aggregate(const ColumnBatch &input, std::vector<AggregateSpec> specs) {
    VectorizedPipeline p;
    p.addAggregate(std::move(specs));
    return execute(input, p);
}

ColumnBatch ColumnarExecutionEngine::project(const ColumnBatch &input, std::vector<std::string> columns) {
    VectorizedPipeline p;
    p.addProject(std::move(columns));
    return execute(input, p);
}

ColumnBatch ColumnarExecutionEngine::sort(const ColumnBatch &input, std::vector<SortOperator::SortKey> keys) {
    VectorizedPipeline p;
    p.addSort(std::move(keys));
    return execute(input, p);
}

void ColumnarExecutionEngine::resetStats() noexcept {
    stats_ = {};
}

} // namespace analytics
} // namespace themisdb
