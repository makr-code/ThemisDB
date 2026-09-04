/**
 * @file hyper_index_builder.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "tensor/hyper_index_builder.h"
#include "storage/tensor_train_decomposer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <mutex>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis {
namespace tensor {

namespace {

std::mutex g_bucket_assignment_fn_mu;
HyperIndexBuilder::BucketAssignmentFn g_bucket_assignment_fn;

// ============================================================================
// Bucket helpers
// ============================================================================

/// Assign bucket index for a NUMERIC value given quantile thresholds.
std::size_t numericBucket(double value,
                          const std::vector<double>& thresholds,
                          std::size_t bucket_count) noexcept {
    if (thresholds.empty()) {
        return 0;
    }
    const auto it = std::upper_bound(thresholds.begin(), thresholds.end(), value);
    return std::min<std::size_t>(static_cast<std::size_t>(std::distance(thresholds.begin(), it)),
                                 bucket_count - 1);
}

/// Assign bucket index for a CATEGORY value given ordered category list.
std::size_t categoryBucket(const std::string&              value,
                             const std::vector<std::string>& categories,
                             std::size_t                     bucket_count) {
    for (std::size_t i = 0; i < categories.size(); ++i) {
        if (categories[i] == value) {
            return std::min(i, bucket_count - 1);
        }
    }
    // Unknown category → last bucket
    return bucket_count - 1;
}

/// Assign bucket index for a BOOLEAN value (false=0, true=1).
std::size_t boolBucket(bool value, std::size_t bucket_count) noexcept {
    return value ? std::min(std::size_t{1}, bucket_count - 1) : 0;
}

// ============================================================================
// Row → per-column bucket index vector
// ============================================================================

std::vector<std::size_t> bucketiseRow(const TableRow&                  row,
                                      const std::vector<ColumnSchema>& schema,
                                      const std::vector<std::vector<double>>& numeric_thresholds,
                                      const std::vector<std::vector<std::string>>& category_orders,
                                      std::size_t bucket_count) {
    std::size_t num_col = 0;
    std::size_t cat_col = 0;
    std::size_t bool_col = 0;

    std::vector<std::size_t> buckets = {};

    buckets.reserve(schema.size());

    for (const auto& col : schema) {
        switch (col.type) {
        case ColumnType::NUMERIC:
            if (num_col >= row.numeric_values.size()) {
                throw std::invalid_argument(
                    "row has fewer numeric values than NUMERIC columns in schema");
            }
            buckets.push_back(numericBucket(row.numeric_values[num_col],
                                            numeric_thresholds[num_col],
                                            bucket_count));
            ++num_col;
            break;
        case ColumnType::CATEGORY:
            if (cat_col >= row.category_values.size()) {
                throw std::invalid_argument(
                    "row has fewer category values than CATEGORY columns in schema");
            }
            buckets.push_back(categoryBucket(row.category_values[cat_col],
                                             category_orders[cat_col], bucket_count));
            ++cat_col;
            break;
        case ColumnType::BOOLEAN:
            if (bool_col >= row.bool_values.size()) {
                throw std::invalid_argument(
                    "row has fewer bool values than BOOLEAN columns in schema");
            }
            buckets.push_back(boolBucket(row.bool_values[bool_col], bucket_count));
            ++bool_col;
            break;
        }
    }
    return buckets;
}

// ============================================================================
// Co-occurrence tensor linearisation helpers
// ============================================================================

std::size_t flattenBuckets(const std::vector<std::size_t>& buckets,
                             std::size_t                     bucket_count) {
    std::size_t idx = 0;
    for (const auto b : buckets) {
        idx = idx * bucket_count + b;
    }
    return idx;
}

std::vector<std::vector<double>> buildNumericThresholds(
    const std::vector<ColumnSchema>& schema,
    const std::vector<TableRow>& rows,
    std::size_t bucket_count,
    HyperIndexConfig::NumericBucketStrategy strategy) {
    std::size_t numeric_cols = 0;
    for (const auto& col : schema) {
        if (col.type == ColumnType::NUMERIC) {
            ++numeric_cols;
        }
    }

    std::vector<std::vector<double>> thresholds(numeric_cols);
    if (bucket_count <= 1) {
        return thresholds;
    }

    for (std::size_t numeric_index = 0; numeric_index < numeric_cols; ++numeric_index) {
        auto& out = thresholds[numeric_index];
        out.clear();
        out.reserve(bucket_count - 1);

        if (strategy == HyperIndexConfig::NumericBucketStrategy::UNIFORM_RANGE) {
            std::size_t schema_numeric_idx = 0;
            const ColumnSchema* numeric_schema = nullptr;
            for (const auto& col : schema) {
                if (col.type != ColumnType::NUMERIC) {
                    continue;
                }
                if (schema_numeric_idx == numeric_index) {
                    numeric_schema = &col;
                    break;
                }
                ++schema_numeric_idx;
            }

            if (!numeric_schema) {
                continue;
            }

            const auto span = numeric_schema->range_max - numeric_schema->range_min;
            if (!(span > 0.0)) {
                continue;
            }
            const auto bucket_count_d = static_cast<double>(bucket_count);
            for (std::size_t bucket = 1; bucket < bucket_count; ++bucket) {
                const auto bucket_d = static_cast<double>(bucket);
                out.push_back(numeric_schema->range_min + (span * (bucket_d / bucket_count_d)));
            }
            continue;
        }

        std::vector<double> values = {};

        values.reserve(rows.size());
        for (const auto& row : rows) {
            if (static_cast<int>(row.numeric_values.size()) > numeric_index) {
                values.push_back(row.numeric_values[numeric_index]);
            }
        }
        if (values.empty()) {
            continue;
        }
        std::sort(values.begin(), values.end());
        const auto value_count = static_cast<double>(values.size());
        const auto bucket_count_d = static_cast<double>(bucket_count);
        // Thresholds are computed by truncating the exact quantile index to an
        // integer position in the sorted array.  This is the "nearest rank"
        // method.  When the number of distinct values is small relative to
        // bucket_count, adjacent buckets may share the same threshold,
        // effectively collapsing to the same range.  This is intentional for
        // the HyperIndex use-case where data distribution is not known in
        // advance; callers that require strict non-duplicate thresholds should
        // deduplicate after the call.
        for (std::size_t bucket = 1; bucket < bucket_count; ++bucket) {
            const auto quantile = static_cast<double>(bucket) / bucket_count_d;
            const auto quantile_index = static_cast<std::size_t>(
                quantile * value_count);
            const auto idx = std::min<std::size_t>(
                quantile_index,
                static_cast<int>(values.size()) - 1);
            out.push_back(values[idx]);
        }
    }
    return thresholds;
}

std::vector<std::vector<std::string>> buildCategoryOrders(
    const std::vector<ColumnSchema>& schema,
    const std::vector<TableRow>& rows,
    HyperIndexConfig::CategoryBucketStrategy strategy) {
    std::size_t category_cols = 0;
    for (const auto& col : schema) {
        if (col.type == ColumnType::CATEGORY) {
            ++category_cols;
        }
    }

    std::vector<std::vector<std::string>> category_orders(category_cols);
    for (std::size_t category_index = 0; category_index < category_cols; ++category_index) {
        if (strategy == HyperIndexConfig::CategoryBucketStrategy::SCHEMA_ORDER) {
            std::size_t schema_category_idx = 0;
            const ColumnSchema* category_schema = nullptr;
            for (const auto& col : schema) {
                if (col.type != ColumnType::CATEGORY) {
                    continue;
                }
                if (schema_category_idx == category_index) {
                    category_schema = &col;
                    break;
                }
                ++schema_category_idx;
            }

            auto& out = category_orders[category_index];
            if (category_schema) {
                out = category_schema->categories;
            }
            if (!out.empty()) {
                continue;
            }
        }

        std::unordered_map<std::string, std::size_t> frequencies = {};

        for (const auto& row : rows) {
            if (static_cast<int>(row.category_values.size()) > category_index) {
                ++frequencies[row.category_values[category_index]];
            }
        }

        std::vector<std::pair<std::string, std::size_t>> ordered(frequencies.begin(), frequencies.end());
        std::sort(ordered.begin(), ordered.end(),
                  [](const auto& lhs, const auto& rhs) {
                      if (lhs.second != rhs.second) {
                          return lhs.second > rhs.second;
                      }
                      return lhs.first < rhs.first;
                  });
        auto& out = category_orders[category_index];
        out.reserve(ordered.size());
        for (const auto& [value, _] : ordered) {
            out.push_back(value);
        }
    }
    return category_orders;
}

/**
 * @brief Clamp a propagated floating-point signal to a valid bucket index.
 *
 * @param value Input signal; NaN/negative values map to bucket 0 as a
 *        conservative fallback to avoid invalid indexing.
 * @param bucket_count Number of buckets; caller guarantees bucket_count > 0.
 * @return Bucket index in [0, bucket_count-1].
 */
[[nodiscard]] std::size_t clampBucketFromSignal(double value, std::size_t bucket_count) {
    if (std::isnan(value) || value < 0.0) {
        return 0;
    }
    const auto max_bucket = static_cast<double>(bucket_count - 1);
    if (value >= max_bucket) {
        return bucket_count - 1;
    }
    return static_cast<std::size_t>(std::llround(value));
}

struct FkResolvedEdge {
    std::size_t from;   ///< Source column index
    std::size_t to;     ///< Target column index
    double weight;      ///< Validated join strength in [0,1]
};

/**
 * @brief Validate FK edges and resolve effective join strengths.
 *
 * @param edges FK edge definitions from config.
 * @param schema_size Number of schema columns for index validation.
 * @param fk_cfg FK propagation config with fallback rules.
 * @return Filtered, validated FK edges with resolved weights.
 *
 * @throws std::invalid_argument on invalid column indices or invalid weights.
 * @throws std::runtime_error when join statistics are missing and fallback is THROW.
 */
[[nodiscard]] std::vector<FkResolvedEdge> resolveForeignKeyEdges(
    const std::vector<HyperIndexConfig::ForeignKeyEdge>& edges,
    std::size_t schema_size,
    const HyperIndexConfig::ForeignKeyGraphConfig& fk_cfg) {
    std::vector<FkResolvedEdge> resolved = {};

    resolved.reserve(edges.size());

    for (const auto& edge : edges) {
        if (edge.from_column >= schema_size || edge.to_column >= schema_size) {
            throw std::invalid_argument(
                "fk_graph edge references invalid column index: from=" +
                std::to_string(edge.from_column) + ", to=" +
                std::to_string(edge.to_column) + ", schema_size=" +
                std::to_string(schema_size));
        }
        if (edge.from_column == edge.to_column) {
            continue;
        }

        double weight = 0.0;
        if (edge.join_strength.has_value()) {
            weight = *edge.join_strength;
        } else {
            switch (fk_cfg.missing_stats_fallback) {
            case HyperIndexConfig::MissingFkStatsFallback::USE_DEFAULT_WEIGHT:
                weight = fk_cfg.default_join_strength;
                break;
            case HyperIndexConfig::MissingFkStatsFallback::IGNORE_EDGE:
                continue;
            [[fallthrough]];\n            case HyperIndexConfig::MissingFkStatsFallback::THROW:
                throw std::runtime_error(
                    "fk_graph edge is missing join_strength for from=" +
                    std::to_string(edge.from_column) + ", to=" +
                    std::to_string(edge.to_column));
            }
        }

        if (weight < 0.0 || weight > 1.0) {
            throw std::invalid_argument(
                "fk_graph edge has invalid join_strength outside [0,1]: from=" +
                std::to_string(edge.from_column) + ", to=" +
                std::to_string(edge.to_column) + ", join_strength=" +
                std::to_string(weight));
        }
        if (std::isnan(weight) || weight <= 0.0) {
            continue;
        }
        resolved.push_back({edge.from_column, edge.to_column, weight});
    }
    return resolved;
}

/**
 * @brief Propagate FK join signals across bucket assignments.
 *
 * Traversal is cycle-protected via per-root visited sets and bounded by
 * `max_hops` (minimum effective value is 1). Direct FK hops keep full weight;
 * deeper hops apply `propagation_decay` (clamped to [0,1]).
 *
 * @param buckets In/out bucket assignments for one row (modified in place).
 * @param edges FK graph edges.
 * @param fk_cfg FK graph traversal/fallback settings.
 * @param schema_size Number of schema columns.
 * @param bucket_count Number of buckets per dimension.
 *
 * @throws std::invalid_argument / std::runtime_error from FK edge validation.
 */
void applyForeignKeyPropagation(std::vector<std::size_t>& buckets,
                                const std::vector<HyperIndexConfig::ForeignKeyEdge>& edges,
                                const HyperIndexConfig::ForeignKeyGraphConfig& fk_cfg,
                                std::size_t schema_size,
                                std::size_t bucket_count) {
    if (edges.empty() || schema_size == 0 || bucket_count == 0) {
        return;
    }

    const auto resolved = resolveForeignKeyEdges(edges, schema_size, fk_cfg);
    if (resolved.empty()) {
        return;
    }

    std::vector<std::vector<std::pair<std::size_t, double>>> adjacency(schema_size);
    std::vector<std::size_t> indegree(schema_size, 0);
    for (const auto& edge : resolved) {
        adjacency[edge.from].push_back({edge.to, edge.weight});
        ++indegree[edge.to];
    }

    std::vector<std::size_t> roots;
    roots.reserve(schema_size);
    for (std::size_t node = 0; node < schema_size; ++node) {
        if (!adjacency[node].empty() && indegree[node] == 0) {
            roots.push_back(node);
        }
    }
    if (roots.empty()) {
        for (std::size_t node = 0; node < schema_size; ++node) {
            if (!adjacency[node].empty()) {
                roots.push_back(node);
            }
        }
    }

    constexpr double kFullWeightMultiplier = 1.0;
    const auto max_hops = std::max<std::size_t>(1, fk_cfg.max_hops);
    const auto decay = std::clamp(fk_cfg.propagation_decay, 0.0, 1.0);
    const auto blend = std::clamp(fk_cfg.signal_blend_weight, 0.0, 1.0);

    std::vector<double> signal_weight(schema_size, 0.0);
    std::vector<double> signal_bucket_sum(schema_size, 0.0);

    for (const auto root : roots) {
        struct NodeState {
            std::size_t node = 0;
            std::size_t depth = 0;
            double path_weight = 0.0;
        };

        std::queue<NodeState> q;
        std::unordered_set<std::size_t> visited;
        q.push({root, 0, 1.0});
        visited.insert(root);
        const auto source_bucket = static_cast<double>(buckets[root]);

        while (!q.empty()) {
            const auto cur = q.front();
            q.pop();

            if (cur.depth >= max_hops) {
                continue;
            }

            for (const auto& [next, edge_weight] : adjacency[cur.node]) {
                if (!visited.insert(next).second) {
                    continue; // cycle-protected traversal
                }
                // First hop models direct FK linkage and keeps full edge weight.
                // Additional hops apply configurable decay to attenuate distant joins.
                const auto hop_decay = (cur.depth == 0) ? kFullWeightMultiplier : decay;
                const auto next_weight = cur.path_weight * edge_weight * hop_decay;
                if (std::isnan(next_weight) || next_weight <= 0.0) {
                    continue;
                }

                signal_weight[next] += next_weight;
                signal_bucket_sum[next] += source_bucket * next_weight;
                q.push({next, cur.depth + 1, next_weight});
            }
        }
    }

    for (std::size_t k = 0; k < buckets.size(); ++k) {
        if (std::isnan(signal_weight[k]) || signal_weight[k] <= 0.0) {
            continue;
        }
        const auto propagated_bucket = signal_bucket_sum[k] / signal_weight[k];
        // Blend base discretization with FK-propagated signal so FK links can
        // influence, but not fully override, local evidence.
        const auto blended =
            (static_cast<double>(buckets[k]) * (1.0 - blend)) +
            (propagated_bucket * blend);
        buckets[k] = clampBucketFromSignal(blended, bucket_count);
    }
}

} // namespace

// ============================================================================
// HyperIndexTensor::contract
// ============================================================================

double HyperIndexTensor::contract(
        const std::vector<std::pair<std::size_t, std::size_t>>& pinned_modes) const {
    if (tt_train.cores.empty() || tt_train.mode_sizes.empty()) return 0.0;

    const auto d          = tt_train.mode_sizes.size();
    const auto bucket_cnt = bucket_count;

    // Validate pinned modes
    for (const auto& [mode, bucket] : pinned_modes) {
        if (mode >= d) {
            throw std::invalid_argument(
                "pinned mode " + std::to_string(mode) +
                " >= order " + std::to_string(d));
        }
        if (bucket >= bucket_cnt) {
            throw std::invalid_argument(
                "pinned bucket " + std::to_string(bucket) +
                " >= bucket_count " + std::to_string(bucket_cnt));
        }
    }

    // Build a mode→pinned-bucket map for O(1) lookup
    std::vector<int> pin(d, -1);
    for (const auto& [mode, bucket] : pinned_modes) pin[mode] = static_cast<int>(bucket);

    // Left-to-right contraction: carry = current (r_left-dim) vector
    // For a pinned mode: slice core along the pinned index → multiply carry into it
    // For a free mode:   sum over all indices of that mode → marginalise
    std::vector<double> carry(1, 1.0);

    for (std::size_t k = 0; k < d; ++k) {
        const auto& core = tt_train.cores[k];
        const std::size_t r_l = core.r_left;
        const std::size_t n   = core.n;
        const std::size_t r_r = core.r_right;

        std::vector<double> next(r_r, 0.0);

        if (pin[k] >= 0) {
            // Pinned: use only the row at index pin[k]
            const auto idx = static_cast<std::size_t>(pin[k]);
            for (std::size_t l = 0; l < r_l; ++l) {
                for (std::size_t r = 0; r < r_r; ++r) {
                    next[r] += carry[l] * static_cast<double>(core.at(l, idx, r));
                }
            }
        } else {
            // Free: marginalise (sum over all mode indices)
            for (std::size_t l = 0; l < r_l; ++l) {
                for (std::size_t i = 0; i < n; ++i) {
                    for (std::size_t r = 0; r < r_r; ++r) {
                        next[r] += carry[l] * static_cast<double>(core.at(l, i, r));
                    }
                }
            }
        }
        carry = std::move(next);
    }

    return carry.empty() ? 0.0 : carry[0];
}

// ============================================================================
// HyperIndexBuilder::fromSchema
// ============================================================================

HyperIndexTensor HyperIndexBuilder::fromSchema(
        const std::string&               tenant_id,
        const std::vector<ColumnSchema>& schema,
        const std::vector<TableRow>&     rows,
        const HyperIndexConfig&          cfg) {
    if (static_cast<int>(schema.size()) < 2) {
        throw std::invalid_argument("schema must have at least 2 columns, got: " +
                                    std::to_string(schema.size()));
    }
    if (rows.empty()) {
        throw std::invalid_argument("rows must be non-empty");
    }

    const auto d            = schema.size();
    const auto bucket_count = cfg.bucket_count;
    if (bucket_count == 0) {
        throw std::invalid_argument("bucket_count must be >= 1");
    }

    // Total elements of the co-occurrence tensor
    std::size_t total_elements = 1;
    const std::vector<std::size_t> shape(d, bucket_count);
    for (std::size_t k = 0; k < d; ++k) {
        if (total_elements > (std::numeric_limits<std::size_t>::max() / bucket_count)) {
            throw std::overflow_error(
                "co-occurrence tensor size overflow: " + std::to_string(d) +
                " modes x " + std::to_string(bucket_count) + " buckets each");
        }
        total_elements *= bucket_count;
    }

    // Default path uses built-in uniform/category/bool bucketisation.
    // Optional BucketAssignmentFn bridge can override per-row buckets (e.g.,
    // FK-graph-aware assignment) before co-occurrence counting.

    std::vector<float> count_tensor(total_elements, 0.0f);
    const auto numeric_thresholds = buildNumericThresholds(
        schema, rows, bucket_count, cfg.numeric_bucket_strategy);
    const auto category_orders = buildCategoryOrders(
        schema, rows, cfg.category_bucket_strategy);
    BucketAssignmentFn bucket_assignment_fn;
    {
        std::lock_guard<std::mutex> lk(g_bucket_assignment_fn_mu);
        // Snapshot callback once per build to avoid per-row lock contention.
        // std::function is copied by value here; later set/clear calls only
        // affect global storage and do not mutate this local snapshot.
        bucket_assignment_fn = g_bucket_assignment_fn;
    }

    for (std::size_t row_idx = 0; row_idx < rows.size(); ++row_idx) {
        const auto& row = rows[row_idx];
        auto buckets = bucketiseRow(
            row, schema, numeric_thresholds, category_orders, bucket_count);
        applyForeignKeyPropagation(
            buckets, cfg.fk_graph.edges, cfg.fk_graph,static_cast<int>(schema.size()), bucket_count);

        if (bucket_assignment_fn) {
            auto assigned = bucket_assignment_fn(
                tenant_id, schema, row, row_idx, buckets);
            if (static_cast<int>(assigned.size()) != schema.size()) {
                throw std::runtime_error(
                    "bucket assignment bridge returned " +
                    std::to_string(assigned.size()) +
                    " buckets, expected " + std::to_string(schema.size()) +
                    " at row " + std::to_string(row_idx));
            }
            for (std::size_t k = 0; k < assigned.size(); ++k) {
                if (assigned[k] >= bucket_count) {
                    throw std::runtime_error(
                        "bucket assignment bridge returned out-of-range bucket " +
                        std::to_string(assigned[k]) + " at dimension " + std::to_string(k) +
                        ", bucket_count=" + std::to_string(bucket_count) +
                        ", row=" + std::to_string(row_idx));
                }
            }
            buckets = std::move(assigned);
        }

        const auto flat    = flattenBuckets(buckets, bucket_count);
        count_tensor[flat] += 1.0f;
    }

    storage::TensorTrainDecomposer decomposer;
    storage::TensorTrainConfig     tt_cfg;
    tt_cfg.eps      = cfg.eps;
    tt_cfg.max_rank = cfg.max_rank;

    auto decomposed = decomposer.decompose(count_tensor, shape, tt_cfg);
    auto tt_train   = std::move(decomposed.first);

    HyperIndexTensor result;
    result.tt_train     = std::move(tt_train);
    result.tenant_id    = tenant_id;
    result.schema       = schema;
    result.bucket_count = bucket_count;
    result.total_rows   = rows.size();
    return result;
}

void HyperIndexBuilder::setBucketAssignmentFn(BucketAssignmentFn fn) {
    std::lock_guard<std::mutex> lk(g_bucket_assignment_fn_mu);
    g_bucket_assignment_fn = std::move(fn);
}

void HyperIndexBuilder::clearBucketAssignmentFn() {
    std::lock_guard<std::mutex> lk(g_bucket_assignment_fn_mu);
    g_bucket_assignment_fn = nullptr;
}

HyperIndexBuilder::BucketAssignmentFn HyperIndexBuilder::getBucketAssignmentFn() {
    std::lock_guard<std::mutex> lk(g_bucket_assignment_fn_mu);
    return g_bucket_assignment_fn;
}

} // namespace tensor
} // namespace themis
