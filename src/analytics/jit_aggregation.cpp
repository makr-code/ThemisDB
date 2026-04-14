/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            jit_aggregation.cpp                                ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-14 11:31:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     590                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a0b664127e  2026-02-26  fix(analytics): remove unused includes from jit_aggregati... ║
    • ebb5b620e7  2026-02-26  feat(analytics): implement JIT aggregation compiler for h... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * JIT Aggregation Compiler – Implementation
 *
 * Hot-path architecture (no LLVM dependency required):
 *
 *  Call-site key
 *    Each unique combination of (AggregateSpec::Function, input_column,
 *    result_name, group_by columns) is encoded into a compact string key
 *    via makeSpecKey().  This key is the unit of hot-path tracking.
 *
 *  Cold path (call_count < hot_threshold)
 *    Aggregation is delegated to AggregateOperator – the generic columnar
 *    executor that dispatches on function enum per row.
 *
 *  Compilation (call_count == hot_threshold)
 *    compileSpecialisation() analyses the spec-set and builds a
 *    std::function<ColumnBatch(const ColumnBatch&)> that:
 *      • Hard-codes the aggregation functions (no per-row enum switch).
 *      • Hard-codes group-by presence and column names.
 *      • Performs the inner aggregation loop directly, enabling the
 *        compiler to inline and auto-vectorise the hot loop.
 *    This mirrors the specialisation that an LLVM MCJIT backend would
 *    generate as native machine code (see THEMIS_HAS_LLVM_JIT below).
 *
 *  Hot path (call_count > hot_threshold)
 *    The cached specialised function is invoked.  No dispatch overhead.
 *
 *  THEMIS_HAS_LLVM_JIT (future extension)
 *    When this compile-time flag is set the compilation step may instead
 *    emit LLVM IR, run the MCJIT pass pipeline at the requested
 *    optimisation level, and store a function pointer to the native code.
 *    The rest of the dispatch logic is identical.
 */

#include "analytics/jit_aggregation.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <spdlog/spdlog.h>

namespace themisdb {
namespace analytics {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

// -- Numeric value extraction (same logic as columnar_execution.cpp) ----------

static std::optional<double> numericAt(const Column& col, size_t row) {
    if (col.isNull(row)) return std::nullopt;
    switch (col.type()) {
        case ColumnType::Double: return col.doubleData()[row];
        case ColumnType::Int64:  return static_cast<double>(col.int64Data()[row]);
        case ColumnType::Bool:   return col.boolData()[row] ? 1.0 : 0.0;
        default:                 return std::nullopt;
    }
}

// -- Per-spec aggregate state -------------------------------------------------

struct AggState {
    double  sum           = 0.0;
    double  min_val       = std::numeric_limits<double>::max();
    double  max_val       = std::numeric_limits<double>::lowest();
    int64_t count         = 0;
    int64_t count_nonnull = 0;
    std::unordered_set<std::string> distinct_set;
};

static void updateState(AggState& st, const Column& col, size_t row) {
    ++st.count;
    auto v = numericAt(col, row);
    if (!v) return;
    ++st.count_nonnull;
    st.sum += *v;
    if (*v < st.min_val) st.min_val = *v;
    if (*v > st.max_val) st.max_val = *v;
}

static void updateDistinct(AggState& st, const Column& col, size_t row) {
    ++st.count;
    if (col.isNull(row)) return;
    ++st.count_nonnull;
    auto val = col.get(row);
    std::ostringstream oss;
    std::visit([&oss](auto&& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) oss << "null";
        else oss << v;
    }, val);
    st.distinct_set.insert(oss.str());
}

static double finalizeAgg(const AggState& st, AggregateSpec::Function fn) {
    switch (fn) {
        case AggregateSpec::Function::Count:
            return static_cast<double>(st.count);
        case AggregateSpec::Function::Sum:
            return st.sum;
        case AggregateSpec::Function::Avg:
            return st.count_nonnull > 0
                       ? st.sum / static_cast<double>(st.count_nonnull) : 0.0;
        case AggregateSpec::Function::Min:
            return st.count_nonnull > 0 ? st.min_val : 0.0;
        case AggregateSpec::Function::Max:
            return st.count_nonnull > 0 ? st.max_val : 0.0;
        case AggregateSpec::Function::CountDistinct:
            return static_cast<double>(st.distinct_set.size());
    }
    return 0.0;
}

static std::string makeGroupKey(const ColumnBatch&               batch,
                                const std::vector<std::string>&  group_cols,
                                size_t                           row) {
    std::ostringstream oss;
    for (const auto& gc : group_cols) {
        auto col = batch.getColumn(gc);
        if (!col) { oss << "null|"; continue; }
        auto val = col->get(row);
        std::visit([&oss](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::nullptr_t>) oss << "null";
            else oss << v;
        }, val);
        oss << '|';
    }
    return oss.str();
}

// -- Specialised aggregation implementations ---------------------------------
//
// These functions are produced by compileSpecialisation() and stored in the
// cache.  They differ from the generic AggregateOperator::execute() path in
// that the aggregation function set is fixed at "compile" time, so the inner
// loop has no per-row enum dispatch.  The compiler can therefore inline and
// auto-vectorise the hot loop bodies.

/** Specialised no-GROUP-BY path. */
static ColumnBatch specialisedAggregateAll(
    const ColumnBatch& input,
    const std::vector<AggregateSpec>& specs)
{
    const size_t n = input.rowCount();
    std::vector<AggState> states(specs.size());

    for (size_t s = 0; s < specs.size(); ++s) {
        const auto& spec = specs[s];
        auto& st = states[s];

        if (spec.function == AggregateSpec::Function::Count
            && spec.input_column.empty()) {
            st.count = static_cast<int64_t>(n);
            continue;
        }
        if (spec.function == AggregateSpec::Function::CountDistinct) {
            auto col = input.getColumn(spec.input_column);
            if (col) for (size_t i = 0; i < n; ++i) updateDistinct(st, *col, i);
            continue;
        }
        auto col = input.getColumn(spec.input_column);
        if (!col) continue;

        // Inner hot loop – no enum dispatch; compiler can vectorise this.
        switch (spec.function) {
            case AggregateSpec::Function::Sum: {
                const auto& data = col->doubleData();
                if (col->type() == ColumnType::Double && data.size() >= n) {
                    for (size_t i = 0; i < n; ++i) {
                        if (!col->isNull(i)) { st.sum += data[i]; ++st.count_nonnull; }
                        ++st.count;
                    }
                } else {
                    for (size_t i = 0; i < n; ++i) updateState(st, *col, i);
                }
                break;
            }
            case AggregateSpec::Function::Avg: {
                for (size_t i = 0; i < n; ++i) updateState(st, *col, i);
                break;
            }
            case AggregateSpec::Function::Min: {
                const auto& data = col->doubleData();
                if (col->type() == ColumnType::Double && data.size() >= n) {
                    for (size_t i = 0; i < n; ++i) {
                        if (!col->isNull(i)) {
                            if (data[i] < st.min_val) st.min_val = data[i];
                            ++st.count_nonnull;
                        }
                        ++st.count;
                    }
                } else {
                    for (size_t i = 0; i < n; ++i) updateState(st, *col, i);
                }
                break;
            }
            case AggregateSpec::Function::Max: {
                const auto& data = col->doubleData();
                if (col->type() == ColumnType::Double && data.size() >= n) {
                    for (size_t i = 0; i < n; ++i) {
                        if (!col->isNull(i)) {
                            if (data[i] > st.max_val) st.max_val = data[i];
                            ++st.count_nonnull;
                        }
                        ++st.count;
                    }
                } else {
                    for (size_t i = 0; i < n; ++i) updateState(st, *col, i);
                }
                break;
            }
            default:
                for (size_t i = 0; i < n; ++i) updateState(st, *col, i);
                break;
        }
    }

    ColumnBatch result(1);
    for (size_t s = 0; s < specs.size(); ++s) {
        double val = finalizeAgg(states[s], specs[s].function);
        auto out_col = std::make_shared<Column>(specs[s].result_name, ColumnType::Double);
        out_col->appendDouble(val);
        result.addColumn(out_col);
    }
    return result;
}

/** Specialised GROUP-BY path. */
static ColumnBatch specialisedAggregateGroupBy(
    const ColumnBatch& input,
    const std::vector<AggregateSpec>& specs,
    const std::vector<std::string>& group_cols)
{
    const size_t n = input.rowCount();

    std::unordered_map<std::string, std::vector<AggState>> groups;
    std::vector<std::string> key_order;

    for (size_t row = 0; row < n; ++row) {
        std::string key = makeGroupKey(input, group_cols, row);

        auto it = groups.find(key);
        if (it == groups.end()) {
            groups.emplace(key, std::vector<AggState>(specs.size()));
            key_order.push_back(key);
            it = groups.find(key);
        }

        auto& states = it->second;
        for (size_t s = 0; s < specs.size(); ++s) {
            const auto& spec = specs[s];
            auto& st = states[s];

            if (spec.function == AggregateSpec::Function::Count
                && spec.input_column.empty()) {
                ++st.count; continue;
            }
            if (spec.function == AggregateSpec::Function::CountDistinct) {
                auto col = input.getColumn(spec.input_column);
                if (col) updateDistinct(st, *col, row);
                continue;
            }
            auto col = input.getColumn(spec.input_column);
            if (col) updateState(st, *col, row);
        }
    }

    const size_t num_rows = key_order.size();
    ColumnBatch result(num_rows);

    // Group-key columns (representative first-row value per group).
    std::unordered_map<std::string, size_t> first_row;
    for (size_t row = 0; row < n; ++row) {
        std::string k = makeGroupKey(input, group_cols, row);
        if (!first_row.count(k)) first_row[k] = row;
    }

    for (const auto& gc : group_cols) {
        auto src = input.getColumn(gc);
        if (!src) continue;
        auto out_col = std::make_shared<Column>(gc, src->type());
        out_col->reserve(num_rows);
        for (const auto& k : key_order) {
            size_t fr  = first_row.at(k);
            auto   val = src->get(fr);
            bool   is_null = src->isNull(fr);
            switch (src->type()) {
                case ColumnType::Int64:
                    out_col->appendInt64(
                        is_null ? 0 : std::get<int64_t>(val), is_null); break;
                case ColumnType::Double:
                    out_col->appendDouble(
                        is_null ? 0.0 : std::get<double>(val), is_null); break;
                case ColumnType::String:
                    out_col->appendString(
                        is_null ? "" : std::get<std::string>(val), is_null); break;
                case ColumnType::Bool:
                    out_col->appendBool(
                        is_null ? false : std::get<bool>(val), is_null); break;
                case ColumnType::Null:
                    out_col->appendNull(); break;
            }
        }
        result.addColumn(out_col);
    }

    // Aggregate measure columns.
    for (size_t s = 0; s < specs.size(); ++s) {
        auto out_col = std::make_shared<Column>(specs[s].result_name, ColumnType::Double);
        out_col->reserve(num_rows);
        for (const auto& k : key_order) {
            out_col->appendDouble(finalizeAgg(groups.at(k)[s], specs[s].function));
        }
        result.addColumn(out_col);
    }
    return result;
}

}  // anonymous namespace

// ============================================================================
// JITAggregationCompiler::Impl
// ============================================================================

class JITAggregationCompiler::Impl {
public:
    explicit Impl(const Config& cfg) : config_(cfg) {}

    // -------------------------------------------------------------------------
    // aggregate() – main dispatch
    // -------------------------------------------------------------------------

    ColumnBatch aggregate(const ColumnBatch& input,
                          const std::vector<AggregateSpec>& specs)
    {
        ++stats_.total_calls;

        // Dense materialisation before aggregation.
        ColumnBatch dense = input.materialize();

        if (!config_.enable_jit || specs.empty()) {
            return genericAggregate(dense, specs);
        }

        const std::string key = JITAggregationCompiler::makeSpecKey(specs);

        // --- hot-path lookup ---
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            ++stats_.jit_hits;
            return it->second(dense);
        }

        // --- call count update ---
        auto& cnt = call_counts_[key];
        ++cnt;

        if (cnt < config_.hot_threshold) {
            // Still warming up.
            return genericAggregate(dense, specs);
        }

        // --- compile (first time hitting threshold) ---
        compileSpecialisation(key, specs);
        ++stats_.jit_hits;                     // count this call as a hit
        return cache_.at(key)(dense);
    }

    // -------------------------------------------------------------------------
    // Introspection
    // -------------------------------------------------------------------------

    bool isCompiled(const std::string& key) const {
        return cache_.count(key) > 0;
    }

    size_t callCount(const std::string& key) const {
        auto it = call_counts_.find(key);
        return it != call_counts_.end() ? it->second : 0;
    }

    void invalidate(const std::string& key) {
        cache_.erase(key);
        call_counts_.erase(key);
        stats_.cache_size = cache_.size();
    }

    void invalidateAll() {
        cache_.clear();
        call_counts_.clear();
        stats_.cache_size = 0;
    }

    const Stats& stats() const noexcept { return stats_; }

    void resetStats() noexcept {
        stats_.total_calls      = 0;
        stats_.jit_hits         = 0;
        stats_.jit_compilations = 0;
        stats_.cache_size       = cache_.size();
    }

    const Config& config() const noexcept { return config_; }

private:
    // -------------------------------------------------------------------------
    // Generic (cold) path
    // -------------------------------------------------------------------------

    static ColumnBatch genericAggregate(const ColumnBatch& input,
                                        const std::vector<AggregateSpec>& specs)
    {
        AggregateOperator op(specs);
        return op.execute(input);
    }

    // -------------------------------------------------------------------------
    // Compilation: build a specialised closure for this spec-set
    // -------------------------------------------------------------------------

    void compileSpecialisation(const std::string& key,
                               const std::vector<AggregateSpec>& specs)
    {
        // Enforce cache capacity limit (evict LRU – here simplest: drop oldest).
        if (cache_.size() >= config_.max_cache_entries) {
            auto oldest = cache_.begin();
            call_counts_.erase(oldest->first);
            cache_.erase(oldest);
        }

        const std::vector<std::string>& group_cols =
            specs.empty() ? std::vector<std::string>{} : specs.front().group_by;

        // Capture specs by value so the closure is self-contained.
        std::vector<AggregateSpec> captured_specs = specs;
        std::vector<std::string>   captured_groups = group_cols;

        if (group_cols.empty()) {
            cache_[key] = [captured_specs](const ColumnBatch& batch) -> ColumnBatch {
                return specialisedAggregateAll(batch, captured_specs);
            };
        } else {
            cache_[key] = [captured_specs, captured_groups](
                              const ColumnBatch& batch) -> ColumnBatch {
                return specialisedAggregateGroupBy(batch, captured_specs,
                                                   captured_groups);
            };
        }

        ++stats_.jit_compilations;
        stats_.cache_size = cache_.size();

        spdlog::debug("JITAggregationCompiler: compiled specialisation for key '{}'",
                      key);

#ifdef THEMIS_HAS_LLVM_JIT
        // Future LLVM MCJIT extension point:
        //   1. Build LLVM IR for the aggregation loop.
        //   2. Run the pass pipeline at optimization_level.
        //   3. Extract a function pointer.
        //   4. Replace the std::function above with the native pointer.
        // The interface is already in place; the LLVM backend is a drop-in
        // replacement for the std::function stored in cache_[key].
        spdlog::debug("JITAggregationCompiler: LLVM JIT backend enabled "
                      "(optimisation level {})", config_.optimization_level);
#endif
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    Config config_;
    Stats  stats_;

    // call_counts_: tracks how many times a spec-set has been invoked.
    std::unordered_map<std::string, size_t> call_counts_;

    // cache_: compiled specialisations (key -> callable).
    std::unordered_map<std::string, std::function<ColumnBatch(const ColumnBatch&)>> cache_;
};

// ============================================================================
// JITAggregationCompiler – public API (pimpl forwarding)
// ============================================================================

JITAggregationCompiler::JITAggregationCompiler()
    : impl_(std::make_unique<Impl>(Config{})) {}

JITAggregationCompiler::JITAggregationCompiler(const Config& cfg)
    : impl_(std::make_unique<Impl>(cfg)) {}

JITAggregationCompiler::~JITAggregationCompiler() = default;

ColumnBatch JITAggregationCompiler::aggregate(
    const ColumnBatch& input,
    const std::vector<AggregateSpec>& specs)
{
    return impl_->aggregate(input, specs);
}

bool JITAggregationCompiler::isCompiled(const std::string& spec_key) const {
    return impl_->isCompiled(spec_key);
}

size_t JITAggregationCompiler::callCount(const std::string& spec_key) const {
    return impl_->callCount(spec_key);
}

// static
std::string JITAggregationCompiler::makeSpecKey(
    const std::vector<AggregateSpec>& specs)
{
    // Encode: function_name|input_col|result_name;... followed by group_by cols.
    static const char* kFnName[] = {
        "cnt", "sum", "avg", "min", "max", "cntd"
    };
    std::ostringstream oss;
    for (const auto& s : specs) {
        int fi = static_cast<int>(s.function);
        oss << (fi >= 0 && fi < 6 ? kFnName[fi] : "?")
            << '|' << s.input_column
            << '|' << s.result_name
            << ';';
    }
    oss << '[';
    if (!specs.empty()) {
        for (const auto& g : specs.front().group_by) oss << g << ',';
    }
    oss << ']';
    return oss.str();
}

void JITAggregationCompiler::invalidate(const std::string& spec_key) {
    impl_->invalidate(spec_key);
}

void JITAggregationCompiler::invalidateAll() {
    impl_->invalidateAll();
}

const JITAggregationCompiler::Stats& JITAggregationCompiler::stats() const noexcept {
    return impl_->stats();
}

void JITAggregationCompiler::resetStats() noexcept {
    impl_->resetStats();
}

const JITAggregationCompiler::Config& JITAggregationCompiler::config() const noexcept {
    return impl_->config();
}

}  // namespace analytics
}  // namespace themisdb
