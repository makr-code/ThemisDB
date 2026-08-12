/**
 * @file adaptive_join.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=0, H=5, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/adaptive_join.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>

namespace themis {

namespace {

[[nodiscard]] const std::string* findKeyValue(const RowValue* row,
                                              const std::string& key) noexcept {
    if (row == nullptr) {
        return nullptr;
    }
    const auto it = row->find(key);
    if (it == row->end()) {
        return nullptr;
    }
    return &it->second;
}

} // namespace

// ============================================================================
// joinAlgorithmName
// ============================================================================

const char* joinAlgorithmName(JoinAlgorithm algo) noexcept {
    switch (algo) {
        case JoinAlgorithm::HASH_JOIN:          return "HASH_JOIN";
        case JoinAlgorithm::MERGE_JOIN:         return "MERGE_JOIN";
        case JoinAlgorithm::NESTED_LOOP_JOIN:   return "NESTED_LOOP_JOIN";
        case JoinAlgorithm::INDEX_NESTED_LOOP:  return "INDEX_NESTED_LOOP";
        case JoinAlgorithm::BROADCAST_JOIN:     return "BROADCAST_JOIN";
        case JoinAlgorithm::SHUFFLE_JOIN:       return "SHUFFLE_JOIN";
        case JoinAlgorithm::GRACE_HASH_JOIN:    return "GRACE_HASH_JOIN";
        default:                                return "UNKNOWN";
    }
}

// ============================================================================
// estimateJoinCost
// ============================================================================

double estimateJoinCost(JoinAlgorithm algo,
                        size_t left_rows,
                        size_t right_rows,
                        bool left_sorted,
                        bool right_sorted) noexcept {
    const auto L = static_cast<double>(left_rows);
    const auto R = static_cast<double>(right_rows);

    switch (algo) {
        case JoinAlgorithm::HASH_JOIN:
            // Build phase (smaller side) + probe phase (larger side)
            return L + R;

        case JoinAlgorithm::MERGE_JOIN: {
            // Scan both sides.  Add sort cost for unsorted inputs.
            double sort_cost_left  = left_sorted  ? 0.0 : (L > 1.0 ? L * std::log2(L) : 0.0);
            double sort_cost_right = right_sorted ? 0.0 : (R > 1.0 ? R * std::log2(R) : 0.0);
            return L + R + sort_cost_left + sort_cost_right;
        }

        case JoinAlgorithm::NESTED_LOOP_JOIN:
            // O(left × right) comparisons
            return L * R;

        case JoinAlgorithm::INDEX_NESTED_LOOP:
            // For each left row, one index lookup on the right side (log R)
            return L * (R > 1.0 ? std::log2(R) : 1.0);

        case JoinAlgorithm::BROADCAST_JOIN:
            // Broadcast cost: proportional to smaller side + scan of larger side
            return std::min(L, R) + std::max(L, R);

        case JoinAlgorithm::SHUFFLE_JOIN:
            // Repartition both sides + hash join cost
            return L + R + (L + R);  // transfer + processing

        case JoinAlgorithm::GRACE_HASH_JOIN:
            // Partition + build + probe (roughly 3× hash join I/O)
            return 3.0 * (L + R);

        default:
            return std::numeric_limits<double>::max();
    }
}

// ============================================================================
// AdaptiveJoinExecutor — constructor
// ============================================================================

AdaptiveJoinExecutor::AdaptiveJoinExecutor(AdaptiveJoinConfig config) noexcept
    : config_(std::move(config)) {}

// ============================================================================
// selectAlgorithm
// ============================================================================

JoinAlgorithm AdaptiveJoinExecutor::selectAlgorithm(
        size_t left_rows,
        size_t right_rows,
        bool   left_sorted,
        bool   right_sorted,
        bool   has_index,
        const RuntimeStats& stats) const noexcept {

    // --- AC-3: Nested Loop — left side < nested_loop_threshold rows ----------
    if (left_rows < config_.nested_loop_threshold) {
        spdlog::debug("AdaptiveJoin: NESTED_LOOP_JOIN selected (left_rows={} < {})",
                      left_rows, config_.nested_loop_threshold);
        return JoinAlgorithm::NESTED_LOOP_JOIN;
    }

    // --- AC-4: Index Nested Loop — right has index AND left < 10,000 rows ----
    if (has_index && left_rows < config_.index_nested_loop_threshold) {
        spdlog::debug("AdaptiveJoin: INDEX_NESTED_LOOP selected (has_index, left_rows={})",
                      left_rows);
        return JoinAlgorithm::INDEX_NESTED_LOOP;
    }

    // --- AC-2: Merge Join — both inputs sorted on join key -------------------
    if (left_sorted && right_sorted) {
        spdlog::debug("AdaptiveJoin: MERGE_JOIN selected (both inputs sorted)");
        return JoinAlgorithm::MERGE_JOIN;
    }

    // --- AC-5: Grace Hash — build side would exceed memory budget ------------
    {
        // Estimate memory required to hold the smaller (build) side's hash table.
        const size_t build_rows = std::min(left_rows, right_rows);
        const bool would_overflow =
            (stats.bytes_per_row > 0) &&
            (build_rows > (std::numeric_limits<size_t>::max() / stats.bytes_per_row));
        const size_t estimated_memory =
            would_overflow ? std::numeric_limits<size_t>::max()
                           : (build_rows * stats.bytes_per_row);
        const double threshold = stats.grace_hash_threshold *
                                 static_cast<double>(stats.memory_budget_bytes);
        if (static_cast<double>(estimated_memory) > threshold) {
            spdlog::debug("AdaptiveJoin: GRACE_HASH_JOIN selected "
                          "(est_mem={} bytes > threshold={:.0f})",
                          estimated_memory, threshold);
            return JoinAlgorithm::GRACE_HASH_JOIN;
        }
    }

    // --- Distributed join strategies -----------------------------------------
    if (stats.is_distributed) {
        const size_t smaller_rows = std::min(left_rows, right_rows);
        if (smaller_rows <= config_.broadcast_threshold) {
            spdlog::debug("AdaptiveJoin: BROADCAST_JOIN selected "
                          "(distributed, smaller_rows={} <= {})",
                          smaller_rows, config_.broadcast_threshold);
            return JoinAlgorithm::BROADCAST_JOIN;
        }
        spdlog::debug("AdaptiveJoin: SHUFFLE_JOIN selected (distributed, large inputs)");
        return JoinAlgorithm::SHUFFLE_JOIN;
    }

    // --- AC-1: Hash Join — default for large equi-joins ---------------------
    spdlog::debug("AdaptiveJoin: HASH_JOIN selected (default, left_rows={}, right_rows={})",
                  left_rows, right_rows);
    return JoinAlgorithm::HASH_JOIN;
}

// ============================================================================
// executeJoin — public entry point
// ============================================================================

JoinResult AdaptiveJoinExecutor::executeJoin(const JoinSpec& spec,
                                              const Table&    left,
                                              const Table&    right,
                                              const RuntimeStats& stats) const {
    if (spec.left_key.empty() || spec.right_key.empty()) {
        throw std::invalid_argument("AdaptiveJoinExecutor: join keys must not be empty");
    }

    const JoinAlgorithm algo = selectAlgorithm(
        left.rowCount(), right.rowCount(),
        left.is_sorted, right.is_sorted,
        right.has_index,
        stats);

    const double cost = estimateJoinCost(algo, left.rowCount(), right.rowCount(),
                                         left.is_sorted, right.is_sorted);

    spdlog::info("AdaptiveJoin: executing {} (left={} rows, right={} rows, cost={:.1f})",
                 joinAlgorithmName(algo), left.rowCount(), right.rowCount(), cost);

    JoinResult result;
    result.algorithm_used  = algo;
    result.estimated_cost  = cost;

    switch (algo) {
        case JoinAlgorithm::HASH_JOIN:
            result = executeHashJoin(spec, left, right);
            break;
        case JoinAlgorithm::MERGE_JOIN:
            result = executeMergeJoin(spec, left, right);
            break;
        case JoinAlgorithm::NESTED_LOOP_JOIN:
            result = executeNestedLoopJoin(spec, left, right);
            break;
        case JoinAlgorithm::INDEX_NESTED_LOOP:
            result = executeIndexNestedLoopJoin(spec, left, right);
            break;
        case JoinAlgorithm::GRACE_HASH_JOIN:
            result = executeGraceHashJoin(spec, left, right);
            break;
        case JoinAlgorithm::BROADCAST_JOIN:
            result = executeBroadcastJoin(spec, left, right);
            break;
        case JoinAlgorithm::SHUFFLE_JOIN:
            result = executeShuffleJoin(spec, left, right);
            break;
        default:
            throw std::logic_error("AdaptiveJoinExecutor: unhandled JoinAlgorithm");
    }

    result.algorithm_used = algo;
    result.estimated_cost = cost;
    return result;
}

// ============================================================================
// mergeRows — helper
// ============================================================================

RowValue AdaptiveJoinExecutor::mergeRows(const RowValue& left_row,
                                          const RowValue& right_row) {
    RowValue merged = left_row;
    for (const auto& [k, v] : right_row) {
        merged[k] = v;  // right side wins on collision
    }
    return merged;
}

// ============================================================================
// executeHashJoin
// ============================================================================

JoinResult AdaptiveJoinExecutor::executeHashJoin(const JoinSpec& spec,
                                                  const Table&    left,
                                                  const Table&    right) const {
    JoinResult result;
    result.algorithm_used = JoinAlgorithm::HASH_JOIN;

    // Build phase: hash table on the smaller side for memory efficiency.
    const bool build_on_right = (right.rowCount() <= left.rowCount());
    const Table&       build_side = build_on_right ? right : left;
    const Table&       probe_side = build_on_right ? left  : right;
    const std::string& build_key  = build_on_right ? spec.right_key : spec.left_key;
    const std::string& probe_key  = build_on_right ? spec.left_key  : spec.right_key;

    std::unordered_map<std::string, std::vector<const RowValue*>> hash_table;
    hash_table.reserve(build_side.rowCount());

    for (const auto& row : build_side.rows) {
        auto it = row.find(build_key);
        if (it != row.end()) {
            hash_table[it->second].push_back(&row);
        }
    }

    // Probe phase: for each probe row, look up matching build rows.
    result.rows.reserve(probe_side.rowCount());
    for (const auto& probe_row : probe_side.rows) {
        auto key_it = probe_row.find(probe_key);
        if (key_it == probe_row.end()) {
            continue;
        }
        auto bucket_it = hash_table.find(key_it->second);
        if (bucket_it == hash_table.end()) {
            continue;
        }
        for (const RowValue* build_row : bucket_it->second) {
            const RowValue& left_row  = build_on_right ? probe_row : *build_row;
            const RowValue& right_row = build_on_right ? *build_row : probe_row;
            if (!spec.filter || spec.filter(left_row, right_row)) {
                result.rows.push_back(mergeRows(left_row, right_row));
            }
        }
    }

    return result;
}

// ============================================================================
// executeMergeJoin
// ============================================================================

JoinResult AdaptiveJoinExecutor::executeMergeJoin(const JoinSpec& spec,
                                                   const Table&    left,
                                                   const Table&    right) const {
    JoinResult result;
    result.algorithm_used = JoinAlgorithm::MERGE_JOIN;

    // Copy + sort both sides (handles callers that set is_sorted=true but
    // whose rows are not actually in order).
    std::vector<const RowValue*> left_ptrs, right_ptrs;
    left_ptrs.reserve(left.rowCount());
    right_ptrs.reserve(right.rowCount());
    for (const auto& r : left.rows)  left_ptrs.push_back(&r);
    for (const auto& r : right.rows) right_ptrs.push_back(&r);

    auto cmp_left  = [&](const RowValue* a, const RowValue* b) {
        auto ia = a->find(spec.left_key),  ib = b->find(spec.left_key);
        if (ia == a->end() && ib == b->end()) return false;
        if (ia == a->end()) return true;
        if (ib == b->end()) return false;
        return ia->second < ib->second;
    };
    auto cmp_right = [&](const RowValue* a, const RowValue* b) {
        auto ia = a->find(spec.right_key), ib = b->find(spec.right_key);
        if (ia == a->end() && ib == b->end()) return false;
        if (ia == a->end()) return true;
        if (ib == b->end()) return false;
        return ia->second < ib->second;
    };

    std::stable_sort(left_ptrs.begin(),  left_ptrs.end(),  cmp_left);
    std::stable_sort(right_ptrs.begin(), right_ptrs.end(), cmp_right);

    // Classic merge join with equal-range matching.
    size_t li = 0, ri = 0;
    const size_t ln = left_ptrs.size(), rn = right_ptrs.size();
    result.rows.reserve(std::min(ln, rn));

    while (li < ln && ri < rn) {
        const std::string* left_value = findKeyValue(left_ptrs[li], spec.left_key);
        const std::string* right_value = findKeyValue(right_ptrs[ri], spec.right_key);

        // Rows without join keys cannot match and must not stall the merge cursor.
        if (left_value == nullptr) {
            ++li;
            continue;
        }
        if (right_value == nullptr) {
            ++ri;
            continue;
        }

        const std::string& lk = *left_value;
        const std::string& rk = *right_value;

        if (lk < rk) {
            ++li;
        } else if (lk > rk) {
            ++ri;
        } else {
            // Equal: find the extents of the equal range on each side.
            size_t li_end = li, ri_end = ri;
            while (li_end < ln) {
                const std::string* value = findKeyValue(left_ptrs[li_end], spec.left_key);
                if (!value || *value != lk) break;
                ++li_end;
            }
            while (ri_end < rn) {
                const std::string* value = findKeyValue(right_ptrs[ri_end], spec.right_key);
                if (!value || *value != rk) break;
                ++ri_end;
            }
            // Cross-product of the equal range.
            for (size_t a = li; a < li_end; ++a) {
                for (size_t b = ri; b < ri_end; ++b) {
                    const RowValue* left_row_ptr = left_ptrs[a];
                    const RowValue* right_row_ptr = right_ptrs[b];
                    if (left_row_ptr == nullptr || right_row_ptr == nullptr) {
                        continue;
                    }
                    if (!spec.filter || spec.filter(*left_row_ptr, *right_row_ptr)) {
                        result.rows.push_back(mergeRows(*left_row_ptr, *right_row_ptr));
                    }
                }
            }
            li = li_end;
            ri = ri_end;
        }
    }

    return result;
}

// ============================================================================
// executeNestedLoopJoin
// ============================================================================

JoinResult AdaptiveJoinExecutor::executeNestedLoopJoin(const JoinSpec& spec,
                                                        const Table&    left,
                                                        const Table&    right) const {
    JoinResult result;
    result.algorithm_used = JoinAlgorithm::NESTED_LOOP_JOIN;
    result.rows.reserve(left.rowCount());

    for (const auto& left_row : left.rows) {
        auto lk_it = left_row.find(spec.left_key);
        if (lk_it == left_row.end()) continue;

        for (const auto& right_row : right.rows) {
            auto rk_it = right_row.find(spec.right_key);
            if (rk_it == right_row.end()) continue;

            if (lk_it->second == rk_it->second) {
                if (!spec.filter || spec.filter(left_row, right_row)) {
                    result.rows.push_back(mergeRows(left_row, right_row));
                }
            }
        }
    }

    return result;
}

// ============================================================================
// executeIndexNestedLoopJoin
// ============================================================================

JoinResult AdaptiveJoinExecutor::executeIndexNestedLoopJoin(
        const JoinSpec& spec,
        const Table&    left,
        const Table&    right) const {
    JoinResult result;
    result.algorithm_used = JoinAlgorithm::INDEX_NESTED_LOOP;

    // Simulate index: build a hash map on the right side's join key once.
    std::unordered_map<std::string, std::vector<const RowValue*>> index;
    index.reserve(right.rowCount());
    for (const auto& row : right.rows) {
        auto it = row.find(spec.right_key);
        if (it != row.end()) {
            index[it->second].push_back(&row);
        }
    }

    // For each left row, perform an O(1) index lookup.
    result.rows.reserve(left.rowCount());
    for (const auto& left_row : left.rows) {
        auto lk_it = left_row.find(spec.left_key);
        if (lk_it == left_row.end()) continue;

        auto bucket = index.find(lk_it->second);
        if (bucket == index.end()) continue;

        for (const RowValue* right_row : bucket->second) {
            if (right_row == nullptr) {
                continue;
            }
            if (!spec.filter || spec.filter(left_row, *right_row)) {
                result.rows.push_back(mergeRows(left_row, *right_row));
            }
        }
    }

    return result;
}

// ============================================================================
// executeGraceHashJoin
// ============================================================================

JoinResult AdaptiveJoinExecutor::executeGraceHashJoin(const JoinSpec& spec,
                                                       const Table&    left,
                                                       const Table&    right) const {
    JoinResult result;
    result.algorithm_used = JoinAlgorithm::GRACE_HASH_JOIN;
    result.rows.reserve(std::min(left.rowCount(), right.rowCount()));

    // Grace hash join: partition both sides on join key, then hash-join each
    // partition pair.  We use a fixed number of partitions proportional to
    // the data size.
    constexpr size_t NUM_PARTITIONS = 16;

    // Q3: Pre-allocate partition vectors to avoid incremental reallocation.
    // Each partition receives approximately (total / NUM_PARTITIONS) rows.
    const size_t left_per_partition  = (left.rowCount()  + NUM_PARTITIONS - 1) / NUM_PARTITIONS;
    const size_t right_per_partition = (right.rowCount() + NUM_PARTITIONS - 1) / NUM_PARTITIONS;

    // Partition left side.
    std::vector<std::vector<const RowValue*>> left_parts(NUM_PARTITIONS);
    for (auto& part : left_parts) part.reserve(left_per_partition);
    for (const auto& row : left.rows) {
        auto it = row.find(spec.left_key);
        if (it == row.end()) continue;
        const size_t p = std::hash<std::string>{}(it->second) % NUM_PARTITIONS;
        left_parts[p].push_back(&row);
    }

    // Partition right side.
    std::vector<std::vector<const RowValue*>> right_parts(NUM_PARTITIONS);
    for (auto& part : right_parts) part.reserve(right_per_partition);
    for (const auto& row : right.rows) {
        auto it = row.find(spec.right_key);
        if (it == row.end()) continue;
        const size_t p = std::hash<std::string>{}(it->second) % NUM_PARTITIONS;
        right_parts[p].push_back(&row);
    }

    // For each partition pair, run an in-memory hash join.
    for (size_t p = 0; p < NUM_PARTITIONS; ++p) {
        if (left_parts[p].empty() || right_parts[p].empty()) continue;

        // Build hash table on the right partition.
        std::unordered_map<std::string, std::vector<const RowValue*>> ht;
        ht.reserve(right_parts[p].size());
        for (const RowValue* row : right_parts[p]) {
            auto it = row->find(spec.right_key);
            if (it != row->end()) {
                ht[it->second].push_back(row);
            }
        }

        // Probe with left partition.
        for (const RowValue* left_row : left_parts[p]) {
            auto lk_it = left_row->find(spec.left_key);
            if (lk_it == left_row->end()) continue;

            auto bucket = ht.find(lk_it->second);
            if (bucket == ht.end()) continue;

            for (const RowValue* right_row : bucket->second) {
                if (right_row == nullptr) {
                    continue;
                }
                if (!spec.filter || spec.filter(*left_row, *right_row)) {
                    result.rows.push_back(mergeRows(*left_row, *right_row));
                }
            }
        }
    }

    return result;
}

// ============================================================================
// executeBroadcastJoin
// ============================================================================

JoinResult AdaptiveJoinExecutor::executeBroadcastJoin(const JoinSpec& spec,
                                                       const Table&    left,
                                                       const Table&    right) const {
    // Broadcast the smaller table and hash-join locally (simulated).
    // In a real distributed engine the coordinator would send the broadcast
    // table to each worker.  The caller (executeJoin) sets algorithm_used.
    return executeHashJoin(spec, left, right);
}

// ============================================================================
// executeShuffleJoin
// ============================================================================

JoinResult AdaptiveJoinExecutor::executeShuffleJoin(const JoinSpec& spec,
                                                     const Table&    left,
                                                     const Table&    right) const {
    // Simulate shuffle by repartitioning then hash-joining locally.
    // The caller (executeJoin) sets algorithm_used.
    return executeHashJoin(spec, left, right);
}

} // namespace themis
