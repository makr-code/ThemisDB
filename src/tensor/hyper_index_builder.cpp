/*
 * @file src/tensor/hyper_index_builder.cpp
 * @brief HyperIndexBuilder and HyperIndexTensor implementation.
 *
 * See include/tensor/hyper_index_builder.h for design details.
 */

#include "tensor/hyper_index_builder.h"
#include "storage/tensor_train_decomposer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace themis {
namespace tensor {

namespace {

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
                                 bucket_count - 1U);
}

/// Assign bucket index for a CATEGORY value given ordered category list.
std::size_t categoryBucket(const std::string&              value,
                             const std::vector<std::string>& categories,
                             std::size_t                     bucket_count) {
    for (std::size_t i = 0; i < categories.size(); ++i) {
        if (categories[i] == value) {
            return std::min(i, bucket_count - 1U);
        }
    }
    // Unknown category → last bucket
    return bucket_count - 1U;
}

/// Assign bucket index for a BOOLEAN value (false=0, true=1).
std::size_t boolBucket(bool value, std::size_t bucket_count) noexcept {
    return value ? std::min(std::size_t{1}, bucket_count - 1U) : 0;
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

    std::vector<std::size_t> buckets;
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
    std::size_t bucket_count) {
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
        std::vector<double> values;
        values.reserve(rows.size());
        for (const auto& row : rows) {
            if (numeric_index < row.numeric_values.size()) {
                values.push_back(row.numeric_values[numeric_index]);
            }
        }
        if (values.empty()) {
            continue;
        }
        std::sort(values.begin(), values.end());
        auto& out = thresholds[numeric_index];
        out.reserve(bucket_count - 1U);
        const auto value_count = static_cast<double>(values.size());
        const auto bucket_count_d = static_cast<double>(bucket_count);
        for (std::size_t bucket = 1; bucket < bucket_count; ++bucket) {
            const auto quantile = static_cast<double>(bucket) / bucket_count_d;
            const auto quantile_index = static_cast<std::size_t>(
                quantile * value_count);
            const auto idx = std::min<std::size_t>(
                quantile_index,
                values.size() - 1U);
            out.push_back(values[idx]);
        }
    }
    return thresholds;
}

std::vector<std::vector<std::string>> buildCategoryOrders(
    const std::vector<ColumnSchema>& schema,
    const std::vector<TableRow>& rows) {
    std::size_t category_cols = 0;
    for (const auto& col : schema) {
        if (col.type == ColumnType::CATEGORY) {
            ++category_cols;
        }
    }

    std::vector<std::vector<std::string>> category_orders(category_cols);
    for (std::size_t category_index = 0; category_index < category_cols; ++category_index) {
        std::unordered_map<std::string, std::size_t> frequencies;
        for (const auto& row : rows) {
            if (category_index < row.category_values.size()) {
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
    if (schema.size() < 2) {
        throw std::invalid_argument("schema must have at least 2 columns, got: " +
                                    std::to_string(schema.size()));
    }
    if (rows.empty()) {
        throw std::invalid_argument("rows must be non-empty");
    }

    const auto d            = schema.size();
    const auto bucket_count = cfg.bucket_count;

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

    // Adaptive bucketing:
    // - NUMERIC columns use empirical quantiles derived from the observed rows
    // - CATEGORY columns are ranked by observed frequency
    // - FK-graph-aware cross-table propagation is still future work

    std::vector<float> count_tensor(total_elements, 0.0f);

    const auto numeric_thresholds = buildNumericThresholds(schema, rows, bucket_count);
    const auto category_orders = buildCategoryOrders(schema, rows);

    for (const auto& row : rows) {
        const auto buckets = bucketiseRow(row, schema, numeric_thresholds, category_orders, bucket_count);
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

} // namespace tensor
} // namespace themis
