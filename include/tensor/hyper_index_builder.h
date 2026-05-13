/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/hyper_index_builder.h                       ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-07                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 7 (Q4 2028)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/hyper_index_builder.h
 * @brief HyperIndexBuilder — relational Hyper-Index construction from tabular data.
 *
 * ## Overview
 *
 * The Hyper-Index encodes relational data (one or more tables) as a TT-train
 * that exposes latent cross-column and cross-table relationships that are
 * invisible to the relational query planner.  Inspired by "Tensor Methods for
 * Data Science" (Anandkumar et al.) and the ThemisDB Phase-7 paper §Relational.
 *
 * ### Construction algorithm
 *
 * 1. For each column, discretise values into `bucket_count` buckets.
 * 2. Build a co-occurrence count tensor T of order `d = num_columns` and shape
 *    `[bucket_count] ^ d`, where T[b₀,…,b_{d-1}] counts rows that fall into
 *    bucket b_k along column k simultaneously.
 * 3. TT-decompose T (ε ≤ config.eps) to obtain the `HyperIndexTensor`.
 *
 * ### Latent join discovery
 *
 * Given a query predicate on a subset of columns the query engine contracts the
 * corresponding TT-cores and returns the residual train as a latent join result.
 *
 * ### Tenant isolation
 *
 * Each `HyperIndexTensor` carries the `tenant_id` from which it was built.
 * `HyperIndexBuilder` never mixes rows from different tenants.
 *
 * ## Extension bridge
 *
 * `HyperIndexBuilder::BucketAssignmentFn` allows callers to inject FK-aware or
 * domain-aware bucket assignment per row while retaining the built-in uniform
 * bucketisation path as fallback.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// ColumnSchema — descriptor for one tabular column
// ============================================================================

enum class ColumnType : uint8_t {
    NUMERIC  = 0,  ///< Float or integer values; bucketed by uniform range
    CATEGORY = 1,  ///< String category values; bucketed by frequency rank
    BOOLEAN  = 2   ///< true / false (2 buckets)
};

struct ColumnSchema {
    std::string name;
    ColumnType  type         = ColumnType::NUMERIC;
    double      range_min    = 0.0;   ///< Used only for NUMERIC columns
    double      range_max    = 1.0;   ///< Used only for NUMERIC columns
    std::vector<std::string> categories; ///< Ordered labels for CATEGORY columns
};

// ============================================================================
// TableRow — a single row of mixed-type values
// ============================================================================

struct TableRow {
    std::vector<double>      numeric_values;   ///< Parallel to schema NUMERIC cols
    std::vector<std::string> category_values;  ///< Parallel to schema CATEGORY cols
    std::vector<bool>        bool_values;      ///< Parallel to schema BOOLEAN cols
};

// ============================================================================
// HyperIndexTensor — the result of HyperIndexBuilder::fromSchema()
// ============================================================================

/**
 * @brief TT-encoded co-occurrence structure for one or more related tables.
 *
 * The tensor mode ordering follows the column ordering in the schema.
 * Each mode has `bucket_count` indices (configurable; default 8).
 */
struct HyperIndexTensor {
    storage::TTTrain         tt_train;      ///< TT-encoded co-occurrence tensor
    std::string              tenant_id;     ///< Owning tenant
    std::vector<ColumnSchema>schema;        ///< Column descriptors used at build time
    std::size_t              bucket_count;  ///< Buckets per dimension
    std::size_t              total_rows;    ///< Number of rows indexed

    /**
     * @brief Contract over a subset of modes to obtain a marginal distribution.
     *
     * `pinned_modes[k]` = b means mode k is pinned to bucket b.
     * Modes not in `pinned_modes` are summed out.
     * Returns the scalar count estimate.
     */
    [[nodiscard]] double contract(const std::vector<std::pair<std::size_t, std::size_t>>& pinned_modes) const;
};

// ============================================================================
// HyperIndexConfig
// ============================================================================

struct HyperIndexConfig {
    enum class NumericBucketStrategy : uint8_t {
        QUANTILE = 0,      ///< Data-driven quantiles from observed row values
        UNIFORM_RANGE = 1  ///< Uniform bins over [ColumnSchema::range_min, range_max]
    };

    enum class CategoryBucketStrategy : uint8_t {
        FREQUENCY_ORDER = 0, ///< Sort by observed frequency (descending)
        SCHEMA_ORDER    = 1  ///< Prefer ColumnSchema::categories declaration order
    };

    enum class MissingFkStatsFallback : uint8_t {
        USE_DEFAULT_WEIGHT = 0, ///< Use fk_graph.default_join_strength
        IGNORE_EDGE        = 1, ///< Skip FK edge when stats are missing
        THROW              = 2  ///< Fail fast with runtime_error
    };

    struct ForeignKeyEdge {
        std::size_t from_column = 0;                 ///< Source mode index
        std::size_t to_column   = 0;                 ///< Target mode index
        /**
         * Optional [0,1] signal weight.
         * - `std::nullopt` triggers `missing_stats_fallback` handling.
         * - Self-loops (`from_column == to_column`) are ignored during resolution.
         */
        std::optional<double> join_strength;
    };

    struct ForeignKeyGraphConfig {
        std::vector<ForeignKeyEdge> edges;             ///< FK relationships
        std::size_t max_hops            = 2;           ///< Min-clamped to 1 during traversal
        double propagation_decay        = 0.8;         ///< Clamped to [0,1]; starts at hop 2
        double default_join_strength    = 0.5;         ///< Used only for USE_DEFAULT_WEIGHT
        MissingFkStatsFallback missing_stats_fallback =
            MissingFkStatsFallback::USE_DEFAULT_WEIGHT;
    };

    std::size_t bucket_count  = 8;    ///< Buckets per column dimension
    double      eps           = 0.05; ///< TT reconstruction error tolerance
    std::size_t max_rank      = 16;   ///< Hard cap on TT-rank
    NumericBucketStrategy numeric_bucket_strategy =
        NumericBucketStrategy::QUANTILE;
    CategoryBucketStrategy category_bucket_strategy =
        CategoryBucketStrategy::FREQUENCY_ORDER;
    ForeignKeyGraphConfig fk_graph;    ///< FK-aware join-signal propagation config
};

// ============================================================================
// HyperIndexBuilder
// ============================================================================

/**
 * @brief Builds a HyperIndexTensor from tabular rows + column schema.
 *
 * Thread-safe: stateless; all context is passed via parameters.
 */
class HyperIndexBuilder {
public:
    /**
     * @brief Optional bridge for FK-aware/custom bucket assignment.
     *
     * Inputs include tenant/schema/row index and the default per-column buckets
     * produced by built-in uniform/category/bool bucketisation.
     *
     * Return a per-column bucket vector with the same size as `schema`.
     */
    using BucketAssignmentFn = std::function<std::vector<std::size_t>(
        const std::string&,
        const std::vector<ColumnSchema>&,
        const TableRow&,
        std::size_t,
        const std::vector<std::size_t>&)>;

    /**
     * @brief Construct a HyperIndexTensor from rows and schema.
     *
     * @param tenant_id  Owning tenant (isolation guarantee).
     * @param schema     Column descriptors; length d ≥ 2.
     * @param rows       Data rows (parallel to schema).
     * @param cfg        Build configuration.
     *
     * @throws std::invalid_argument if schema has < 2 columns, rows is empty,
     *         or FK graph references invalid column indices.
     * @throws std::runtime_error if FK graph is configured to fail when
     *         join-strength statistics are missing.
     *
      * @note
      * Default path uses built-in numeric/category/bool bucketisation.
      * If cfg.fk_graph.edges is non-empty, FK join-signal propagation is
      * applied with cycle-protected graph traversal before counting.
      * For FK-aware/custom assignment, install `BucketAssignmentFn`.
      */
    [[nodiscard]] static HyperIndexTensor fromSchema(
        const std::string&                tenant_id,
        const std::vector<ColumnSchema>&  schema,
        const std::vector<TableRow>&      rows,
        const HyperIndexConfig&           cfg = {});

    /**
     * @brief Thread-safe bridge management for custom bucket assignment.
     */
    static void setBucketAssignmentFn(BucketAssignmentFn fn);
    static void clearBucketAssignmentFn();
    [[nodiscard]] static BucketAssignmentFn getBucketAssignmentFn();
};

} // namespace tensor
} // namespace themis
