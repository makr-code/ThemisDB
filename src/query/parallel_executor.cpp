/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            parallel_executor.cpp                              ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-14                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Parallel Query Execution (Intra-Query) – v1.7.0
//
// Morsel-driven parallelism for three query operators:
//   1. Parallel table scan with predicate filtering
//   2. Partitioned parallel hash join
//   3. Two-phase parallel aggregation
//
// All parallel paths use Intel TBB task_group so that ThemisDB's
// existing TBB thread pool is reused; no extra OS threads are created.

#include "query/parallel_executor.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <tbb/task_group.h>

#include "utils/error_registry.h"

namespace themis {

// ============================================================================
// Construction
// ============================================================================

ParallelExecutor::ParallelExecutor(ParallelConfig config)
    : config_(std::move(config)) {
    if (config_.max_threads == 0) {
        config_.max_threads = 1;
    }
    if (config_.morsel_size == 0) {
        config_.morsel_size = 1;
    }
}

// ============================================================================
// Helpers
// ============================================================================

size_t ParallelExecutor::resolveThreads(size_t requested) const noexcept {
    const size_t t = (requested == 0) ? config_.max_threads : requested;
    return std::max<size_t>(1, std::min(t, config_.max_threads));
}

// static
std::string ParallelExecutor::groupKey(
    const BaseEntity&             e,
    const std::vector<std::string>& group_by) {
    if (group_by.empty()) return {};
    std::string key;
    for (const auto& field : group_by) {
        if (!key.empty()) key += '|';
        auto v = e.getFieldAsString(field);
        key += v.value_or("");
    }
    return key;
}

// static
void ParallelExecutor::mergePartial(PartialMap& dst, const PartialMap& src) {
    for (const auto& [k, s] : src) {
        auto& d  = dst[k];
        d.sum   += s.sum;
        d.count += s.count;
        d.min    = std::min(d.min, s.min);
        d.max    = std::max(d.max, s.max);
    }
}

// static
double ParallelExecutor::finalise(const PartialAgg& p, AggregateFunction fn) {
    switch (fn) {
        case AggregateFunction::Count: return p.count;
        case AggregateFunction::Sum:   return p.sum;
        case AggregateFunction::Avg:
            return (p.count > 0.0) ? (p.sum / p.count) : 0.0;
        case AggregateFunction::Min:
            return (p.count > 0.0) ? p.min : 0.0;
        case AggregateFunction::Max:
            return (p.count > 0.0) ? p.max : 0.0;
    }
    return 0.0; // unreachable
}

// ============================================================================
// Sequential helpers
// ============================================================================

// static
ParallelExecutor::Table ParallelExecutor::sequentialScan(
    const Table& input, const FilterFn& filter) {
    Table out;
    out.reserve(input.size());
    for (const auto& e : input) {
        if (filter(e)) out.push_back(e);
    }
    return out;
}

// static
std::vector<ParallelExecutor::JoinTuple> ParallelExecutor::sequentialHashJoin(
    const Table& left, const Table& right, const JoinSpec& spec) {
    // Build phase: index right side by join key.
    std::unordered_multimap<std::string, const BaseEntity*> ht;
    ht.reserve(right.size());
    for (const auto& r : right) {
        auto key = r.getFieldAsString(spec.right_key);
        if (key) ht.emplace(std::move(*key), &r);
    }
    // Probe phase.
    std::vector<JoinTuple> out;
    for (const auto& l : left) {
        auto lkey = l.getFieldAsString(spec.left_key);
        if (!lkey) continue;
        auto [beg, end] = ht.equal_range(*lkey);
        for (auto it = beg; it != end; ++it) {
            out.push_back({l, *it->second});
        }
    }
    return out;
}

// static
ParallelExecutor::AggregateResult ParallelExecutor::sequentialAggregate(
    const Table& input, const AggregateSpec& spec) {
    PartialMap partial;
    for (const auto& e : input) {
        const std::string gk = groupKey(e, spec.group_by);
        auto& p = partial[gk];
        if (spec.function == AggregateFunction::Count) {
            p.count += 1.0;
        } else {
            auto v = e.getFieldAsDouble(spec.field);
            if (!v) continue;
            p.sum   += *v;
            p.count += 1.0;
            p.min    = std::min(p.min, *v);
            p.max    = std::max(p.max, *v);
        }
    }
    AggregateResult result;
    result.reserve(partial.size());
    for (const auto& [k, p] : partial) {
        result[k] = finalise(p, spec.function);
    }
    return result;
}

// ============================================================================
// parallelScan
// ============================================================================

Result<ParallelExecutor::Table> ParallelExecutor::parallelScan(
    const Table&    input,
    const FilterFn& filter,
    size_t          num_threads) const {
    if (!filter) {
        return Err<Table>(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                          "parallelScan: filter must be callable");
    }

    const size_t n = input.size();

    // Fall back to sequential path when parallelism is disabled, thread count
    // is 1, or the input fits inside a single morsel.
    const size_t threads = resolveThreads(num_threads);
    if (!config_.enable_parallel_scan || threads <= 1 || n <= config_.morsel_size) {
        return Ok(sequentialScan(input, filter));
    }

    const size_t morsel  = config_.morsel_size;
    const size_t nmors   = (n + morsel - 1) / morsel;
    std::vector<Table> buckets(nmors);

    tbb::task_group tg;
    for (size_t m = 0; m < nmors; ++m) {
        tg.run([&, m]() {
            const size_t start = m * morsel;
            const size_t end   = std::min(start + morsel, n);
            Table local;
            local.reserve(end - start);
            for (size_t i = start; i < end; ++i) {
                if (filter(input[i])) local.push_back(input[i]);
            }
            buckets[m] = std::move(local);
        });
    }
    tg.wait();

    // Merge morsel buckets.
    Table out;
    for (auto& b : buckets) {
        out.insert(out.end(),
                   std::make_move_iterator(b.begin()),
                   std::make_move_iterator(b.end()));
    }
    return Ok(std::move(out));
}

// ============================================================================
// parallelHashJoin
// ============================================================================

Result<std::vector<ParallelExecutor::JoinTuple>> ParallelExecutor::parallelHashJoin(
    const Table&    left,
    const Table&    right,
    const JoinSpec& spec,
    size_t          num_threads) const {
    if (spec.left_key.empty() || spec.right_key.empty()) {
        return Err<std::vector<JoinTuple>>(
            errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
            "parallelHashJoin: join keys must not be empty");
    }

    const size_t threads = resolveThreads(num_threads);
    if (!config_.enable_parallel_join || threads <= 1) {
        return Ok(sequentialHashJoin(left, right, spec));
    }

    // Partition both sides by hash(join_key) % threads.
    const size_t P = threads;
    std::vector<Table> left_parts(P);
    std::vector<Table> right_parts(P);

    // Pre-size partitions to avoid repeated allocations.
    if (!left.empty()) {
        const size_t hint = std::max<size_t>(1, left.size() / P);
        for (auto& p : left_parts)  p.reserve(hint);
    }
    if (!right.empty()) {
        const size_t hint = std::max<size_t>(1, right.size() / P);
        for (auto& p : right_parts) p.reserve(hint);
    }

    for (const auto& l : left) {
        auto k = l.getFieldAsString(spec.left_key);
        if (!k) continue;
        const size_t slot = std::hash<std::string>{}(*k) % P;
        left_parts[slot].push_back(l);
    }
    for (const auto& r : right) {
        auto k = r.getFieldAsString(spec.right_key);
        if (!k) continue;
        const size_t slot = std::hash<std::string>{}(*k) % P;
        right_parts[slot].push_back(r);
    }

    // Each worker joins its partition independently.
    std::vector<std::vector<JoinTuple>> part_results(P);

    tbb::task_group tg;
    for (size_t p = 0; p < P; ++p) {
        tg.run([&, p]() {
            part_results[p] = sequentialHashJoin(
                left_parts[p], right_parts[p], spec);
        });
    }
    tg.wait();

    // Merge partition results.
    std::vector<JoinTuple> out;
    for (auto& pr : part_results) {
        out.insert(out.end(),
                   std::make_move_iterator(pr.begin()),
                   std::make_move_iterator(pr.end()));
    }
    return Ok(std::move(out));
}

// ============================================================================
// parallelAggregate
// ============================================================================

Result<ParallelExecutor::AggregateResult> ParallelExecutor::parallelAggregate(
    const Table&         input,
    const AggregateSpec& spec,
    size_t               num_threads) const {
    const size_t n = input.size();

    const size_t threads = resolveThreads(num_threads);
    if (!config_.enable_parallel_aggregate || threads <= 1 ||
        n <= config_.morsel_size) {
        return Ok(sequentialAggregate(input, spec));
    }

    // Phase 1: compute per-morsel partial aggregates.
    const size_t morsel = config_.morsel_size;
    const size_t nmors  = (n + morsel - 1) / morsel;
    std::vector<PartialMap> partials(nmors);

    tbb::task_group tg;
    for (size_t m = 0; m < nmors; ++m) {
        tg.run([&, m]() {
            const size_t start = m * morsel;
            const size_t end   = std::min(start + morsel, n);
            PartialMap& pm = partials[m];
            for (size_t i = start; i < end; ++i) {
                const BaseEntity& e = input[i];
                const std::string gk = groupKey(e, spec.group_by);
                auto& p = pm[gk];
                if (spec.function == AggregateFunction::Count) {
                    p.count += 1.0;
                } else {
                    auto v = e.getFieldAsDouble(spec.field);
                    if (!v) continue;
                    p.sum   += *v;
                    p.count += 1.0;
                    p.min    = std::min(p.min, *v);
                    p.max    = std::max(p.max, *v);
                }
            }
        });
    }
    tg.wait();

    // Phase 2: merge all partial maps into a single map.
    PartialMap merged;
    for (auto& pm : partials) {
        mergePartial(merged, pm);
    }

    AggregateResult result;
    result.reserve(merged.size());
    for (const auto& [k, p] : merged) {
        result[k] = finalise(p, spec.function);
    }
    return Ok(std::move(result));
}

} // namespace themis
