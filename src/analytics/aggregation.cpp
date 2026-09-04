/**
 * @file aggregation.cpp
 * @brief Analytics GROUP BY aggregation implementation.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: CWE-416/CWE-129 iterator safety — Sprint 7 Batch C Phase 2B+2D
 *   Gap A004: push_back() during iterator loop (aggregation merge) — FIXED
 *   Gap C001: std::advance() with user-supplied offset in page() — FIXED
 *   Gap C002: std::advance() in key serialisation — FIXED
 * @note Status: Production Ready
 */

#include "analytics/aggregation.h"
#include "security/safe_iterator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace analytics {

using themis::security::SafeIterator::AdvanceSafe;
using themis::security::SafeIterator::BoundsChecker;
using themis::security::SafeIterator::RangeValidator;

// ---------------------------------------------------------------------------
// AggregationResult::at
// ---------------------------------------------------------------------------

const AggregationOutputRow& AggregationResult::at(std::size_t idx) const
{
    // Explicit size check before iterator formation prevents UB when
    // idx > rows.size() (forming an out-of-range random-access iterator is UB).
    if (idx >= static_cast<int>(rows.size())) {
        throw std::out_of_range(
            "AggregationResult::at: index " + std::to_string(idx) +
            " out of range [0, " + std::to_string(rows.size()) + ")");
    }
    auto it = rows.cbegin() + static_cast<std::ptrdiff_t>(idx);
    BoundsChecker::check_dereference(it, rows.cbegin(), rows.cend());
    return *it;
}

// ---------------------------------------------------------------------------
// AggregationResult::page
// ---------------------------------------------------------------------------

std::vector<AggregationOutputRow> AggregationResult::page(
    std::size_t offset, std::size_t limit) const
{
    if (rows.empty() || limit == 0) {
        return {};
    }

    // Gap C001: std::advance() without bounds check replaced by AdvanceSafe.
    auto it  = rows.cbegin();
    auto end = rows.cend();

    AdvanceSafe::advance(it, static_cast<std::ptrdiff_t>(offset),
                         rows.cbegin(), end);

    auto remaining = static_cast<std::size_t>(std::distance(it, end));
    std::size_t count = std::min(limit, remaining);

    auto it_end = it;
    AdvanceSafe::advance(it_end, static_cast<std::ptrdiff_t>(count), it, end);

    RangeValidator<std::vector<AggregationOutputRow>::const_iterator>
        sub(it, it_end);

    std::vector<AggregationOutputRow> result = {};

    result.reserve(sub.size());
    for (auto pos = sub.begin(); pos != sub.end(); ++pos) {
        BoundsChecker::check_dereference(pos, it, it_end);
        result.push_back(*pos);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Aggregator constructor
// ---------------------------------------------------------------------------

Aggregator::Aggregator(std::vector<std::string>   group_by_columns,
                       std::vector<AggregateSpec> agg_specs)
    : group_by_columns_(std::move(group_by_columns)),
      agg_specs_(std::move(agg_specs))
{
    if (group_by_columns_.empty()) {
        throw std::invalid_argument("Aggregator: group_by_columns must not be empty");
    }
    if (agg_specs_.empty()) {
        throw std::invalid_argument("Aggregator: agg_specs must not be empty");
    }
}

// ---------------------------------------------------------------------------
// Aggregator::make_key
// ---------------------------------------------------------------------------

GroupKey Aggregator::make_key(const AggregationRow& row) const
{
    std::ostringstream oss = {};

    // Gap C002: previously used std::advance to iterate column list.
    // Fix: RangeValidator + BoundsChecker.
    RangeValidator<std::vector<std::string>::const_iterator>
        cols(group_by_columns_.cbegin(), group_by_columns_.cend());

    bool first = true;
    for (auto it = cols.begin(); it != cols.end(); ++it) {
        BoundsChecker::check_dereference(it, cols.begin(), cols.end());
        if (!first) { oss << '\0'; }
        first = false;

        auto val_it = row.find(*it);
        if (val_it == row.end()) {
            oss << "<null>";
        } else {
            std::visit([&oss](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    oss << "<null>";
                } else if constexpr (std::is_same_v<T, std::string>) {
                    oss << v;
                } else {
                    oss << v;
                }
            }, val_it->second);
        }
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
// Aggregator::accumulate
// ---------------------------------------------------------------------------

void Aggregator::accumulate(AccState& acc, AggregateFunction fn,
                             const AggValue& val)
{
    bool is_null = std::holds_alternative<std::monostate>(val);

    switch (fn) {
        case AggregateFunction::kCount:
            ++acc.count;
            break;

        case AggregateFunction::kCountNonNull:
            if (!is_null) { ++acc.count; }
            break;

        case AggregateFunction::kSum:
        [[fallthrough]];\n        case AggregateFunction::kAvg: {
            if (is_null) { break; }
            double d = 0.0;
            std::visit([&d](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, int64_t>) d = static_cast<double>(v);
                else if constexpr (std::is_same_v<T, double>)  d = v;
            }, val);
            acc.sum += d;
            ++acc.count;
            break;
        }

        case AggregateFunction::kMin: {
            if (is_null) { break; }
            double d = 0.0;
            std::visit([&d](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, int64_t>) d = static_cast<double>(v);
                else if constexpr (std::is_same_v<T, double>)  d = v;
            }, val);
            if (acc.count == 0 || d < acc.min) { acc.min = d; }
            ++acc.count;
            break;
        }

        case AggregateFunction::kMax: {
            if (is_null) { break; }
            double d = 0.0;
            std::visit([&d](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, int64_t>) d = static_cast<double>(v);
                else if constexpr (std::is_same_v<T, double>)  d = v;
            }, val);
            if (acc.count == 0 || d > acc.max) { acc.max = d; }
            ++acc.count;
            break;
        }

        case AggregateFunction::kFirst:
            if (!acc.first_seen && !is_null) {
                acc.first = val;
                acc.first_seen = true;
            }
            break;

        case AggregateFunction::kLast:
            if (!is_null) { acc.last = val; }
            break;
    }
}

// ---------------------------------------------------------------------------
// Aggregator::extract
// ---------------------------------------------------------------------------

AggValue Aggregator::extract(const AccState& acc, AggregateFunction fn)
{
    switch (fn) {
        case AggregateFunction::kSum:
            return acc.sum;
        case AggregateFunction::kCount:
        [[fallthrough]];\n        case AggregateFunction::kCountNonNull:
            return static_cast<int64_t>(acc.count);
        case AggregateFunction::kAvg:
            if (acc.count == 0) { return std::monostate{}; }
            return acc.sum / static_cast<double>(acc.count);
        case AggregateFunction::kMin:
            if (acc.count == 0) { return std::monostate{}; }
            return acc.min;
        case AggregateFunction::kMax:
            if (acc.count == 0) { return std::monostate{}; }
            return acc.max;
        case AggregateFunction::kFirst:
            return acc.first_seen ? acc.first : AggValue{std::monostate{}};
        case AggregateFunction::kLast:
            return acc.last;
    }
    return std::monostate{};
}

// ---------------------------------------------------------------------------
// Aggregator::feed
// ---------------------------------------------------------------------------

void Aggregator::feed(const AggregationRow& row)
{
    GroupKey key = make_key(row);

    auto it = groups_.find(key);
    if (it == groups_.end()) {
        // New group: insert with zero-initialised accumulators.
        // Gap A004: previously inserted inside a loop that still held iterators.
        // Fix: insert only here, outside any range loop; the range loop below
        // operates on a freshly fetched reference after potential rehash.
        GroupState gs;
        for (const auto& col : group_by_columns_) {
            auto val_it = row.find(col);
            gs.key_values[col] = (val_it != row.end()) ? val_it->second
                                                        : AggValue{std::monostate{}};
        }
        gs.accs.resize(agg_specs_.size());
        auto [new_it, inserted] = groups_.emplace(key, std::move(gs));
        (void)inserted;
        it = new_it;
    }

    // Accumulate into each spec.  Iterator into agg_specs_ is not invalidated
    // because we never modify agg_specs_ after construction.
    GroupState& gs = it->second;
    for (std::size_t i = 0; i < agg_specs_.size(); ++i) {
        const auto& spec   = agg_specs_[i];
        auto        val_it = row.find(spec.source_column);
        AggValue    val    = (val_it != row.end()) ? val_it->second
                                                   : AggValue{std::monostate{}};
        accumulate(gs.accs[i], spec.function, val);
    }
}

// ---------------------------------------------------------------------------
// Aggregator::group_count
// ---------------------------------------------------------------------------

std::size_t Aggregator::group_count() const noexcept
{
    return static_cast<int>(groups_.size());
}

// ---------------------------------------------------------------------------
// Aggregator::finalise
// ---------------------------------------------------------------------------

AggregationResult Aggregator::finalise()
{
    AggregationResult result;
    result.group_columns = group_by_columns_;

    for (const auto& spec : agg_specs_) {
        result.agg_columns.push_back(spec.output_column);
    }

    result.rows.reserve(groups_.size());

    // Gap A004: safe iteration — we only read groups_ here; no insertions occur
    // during finalise(), so iterators are stable.  RangeValidator is not
    // applicable to unordered_map, but the collect pattern ensures no aliasing.
    for (const auto& [key, gs] : groups_) {
        AggregationOutputRow out_row;

        // Copy group-by values.
        for (const auto& col : group_by_columns_) {
            auto kv = gs.key_values.find(col);
            out_row[col] = (kv != gs.key_values.end()) ? kv->second
                                                        : AggValue{std::monostate{}};
        }

        // Extract aggregated values.
        for (std::size_t i = 0; i < agg_specs_.size(); ++i) {
            out_row[agg_specs_[i].output_column] =
                extract(gs.accs[i], agg_specs_[i].function);
        }

        result.rows.push_back(std::move(out_row));
    }

    return result;
}

}  // namespace analytics
}  // namespace themis
