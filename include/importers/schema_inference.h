/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            schema_inference.h                                 ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 06:52:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     185                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9efa3acd76  2026-03-11  feat(importers): add PostgreSQL Importer v2.1+ with 12 ne... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

/**
 * @brief Column statistics collected from a data source.
 */
struct ColumnStatistics {
    std::string column_name;
    std::string table_name;
    size_t total_rows{0};
    size_t null_count{0};
    size_t distinct_count{0};
    double avg_length{0.0};
    double min_value{0.0};
    double max_value{0.0};
};

/**
 * @brief Sample data row used for semantic type detection.
 */
struct SampleData {
    std::string table_name;
    std::string column_name;
    std::vector<std::string> values;
};

/**
 * @brief Simple table schema description used by inference engine.
 */
struct InferenceTableSchema {
    std::string name;
    std::string schema_ns;
    std::vector<std::string> columns;
    std::map<std::string, std::string> column_types;
    std::vector<std::string> primary_keys;
    std::vector<std::pair<std::string, std::string>> foreign_keys; ///< (local_col, ref_table.ref_col)
};

/**
 * @brief Configuration for SchemaInferenceEngine.
 */
struct SchemaInferenceConfig {
    double relationship_confidence_threshold{0.75};
    size_t max_sample_values{1000};
    bool enable_semantic_detection{true};
};

/**
 * @brief Engine for ML-assisted schema inference.
 *
 * Implements three algorithms:
 *   1. Column Correlation Analysis – discovers implicit FK relationships
 *      (Reference: Quercini et al., 2018)
 *   2. Semantic Type Detection – recognises domain-specific column types
 *   3. Cardinality Estimation – Harmonic Mean estimator for relationship cardinality
 *      (Reference: Li et al., 2016)
 */
class SchemaInferenceEngine {
public:
    using Config = SchemaInferenceConfig;
    // -----------------------------------------------------------------
    // Algorithm 1 output: implicit relationship discovery
    // -----------------------------------------------------------------
    struct InferredSchema {
        std::string table_name;
        std::vector<std::string> likely_relationships; ///< "table_a.col -> table_b.col [conf=0.87]"
        std::vector<std::string> denormalization_candidates;
        std::map<std::string, double> cardinality_distribution;
        json recommendations;
    };

    /**
     * @brief Discover implicit FK relationships using column name and
     *        value-set overlap heuristics.
     *
     * For each pair of columns sharing the same name suffix (e.g. "user_id")
     * across different tables, the algorithm computes a Jaccard similarity
     * on sampled value sets and emits a relationship when similarity exceeds
     * the configured threshold (default 0.75).
     */
    std::vector<InferredSchema> inferImplicitRelationships(
        const std::vector<InferenceTableSchema>& schemas,
        const std::map<std::string, ColumnStatistics>& stats
    );

    // -----------------------------------------------------------------
    // Algorithm 2: semantic type detector
    // -----------------------------------------------------------------
    enum class SemanticType {
        EMAIL,
        PHONE,
        CURRENCY,
        LOCATION_COORD,
        ISO8601_DATETIME,
        UUID,
        HASH_SHA256,
        IP_ADDRESS,
        URL,
        UNKNOWN
    };

    /**
     * @brief Detect domain-specific semantic types by pattern matching on
     *        sampled column values.
     *
     * Returns a map of "table.column" → SemanticType.
     */
    std::map<std::string, SemanticType> detectSemanticTypes(
        const std::vector<InferenceTableSchema>& schemas,
        const std::vector<SampleData>& samples
    );

    /** @brief Convert SemanticType enum to a human-readable string. */
    static std::string semanticTypeToString(SemanticType t);

    // -----------------------------------------------------------------
    // Algorithm 3: cardinality estimation
    // -----------------------------------------------------------------
    struct CardinalityEstimate {
        std::string relationship_id;   ///< "parent_table.col -> child_table.col"
        double one_to_many_ratio{1.0}; ///< avg children per parent
        double selectivity{1.0};       ///< fraction of child rows with a matching parent
        std::vector<double> confidence_interval; ///< 95 % CI [lower, upper]
    };

    /**
     * @brief Estimate relationship cardinalities using the Harmonic Mean
     *        Estimator on distinct-count statistics.
     *
     * Reference: "Distinct Count Estimation for Streams" (Li et al., 2016).
     */
    std::vector<CardinalityEstimate> estimateCardinalities(
        const std::vector<InferenceTableSchema>& schemas,
        const std::map<std::string, ColumnStatistics>& stats
    );

    // -----------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------

    explicit SchemaInferenceEngine(Config cfg = Config{});

private:
    Config config_;

    bool columnNameSimilar(const std::string& a, const std::string& b) const;
    double jaccardSimilarity(const std::vector<std::string>& a,
                             const std::vector<std::string>& b) const;
    SemanticType detectSingleColumn(const std::vector<std::string>& values) const;
};

} // namespace importers
} // namespace themis
