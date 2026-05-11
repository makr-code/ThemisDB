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
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace themis {
namespace tensor {

namespace {

std::mutex g_bucket_assignment_fn_mu;
HyperIndexBuilder::BucketAssignmentFn g_bucket_assignment_fn;

// ============================================================================
// Bucket helpers
// ============================================================================

/// Assign bucket index for a NUMERIC value given [range_min, range_max).
std::size_t numericBucket(double value,
                           double range_min, double range_max,
                           std::size_t bucket_count) noexcept {
    if (range_max <= range_min) return 0;
    const double frac = (value - range_min) / (range_max - range_min);
    const auto   raw  = static_cast<std::size_t>(frac * static_cast<double>(bucket_count));
    return std::min(raw, bucket_count - 1U);
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
                                       std::size_t                      bucket_count) {
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
                                             col.range_min, col.range_max,
                                             bucket_count));
            ++num_col;
            break;
        case ColumnType::CATEGORY:
            if (cat_col >= row.category_values.size()) {
                throw std::invalid_argument(
                    "row has fewer category values than CATEGORY columns in schema");
            }
            buckets.push_back(categoryBucket(row.category_values[cat_col],
                                              col.categories, bucket_count));
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
    std::vector&lt;int&gt; pin(d, -1);
    for (const auto& [mode, bucket] : pinned_modes) pin[mode] = static_cast&lt;int&gt;(bucket);

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

    // Default path uses built-in uniform/category/bool bucketisation.
    // Optional BucketAssignmentFn bridge can override per-row buckets (e.g.,
    // FK-graph-aware assignment) before co-occurrence counting.

    std::vector<float> count_tensor(total_elements, 0.0f);
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
        auto buckets = bucketiseRow(row, schema, bucket_count);

        if (bucket_assignment_fn) {
            auto assigned = bucket_assignment_fn(
                tenant_id, schema, row, row_idx, buckets);
            if (assigned.size() != schema.size()) {
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
