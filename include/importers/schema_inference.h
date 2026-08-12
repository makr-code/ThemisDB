/**
 * @file schema_inference.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    double semantic_type_confidence_threshold{0.70};  ///< Min agreement % for semantic type
    size_t max_sample_values{1000};
    bool enable_semantic_detection{true};
    bool enable_cycle_detection{true};  ///< Detect circular FK references
};

/**
 * @brief Schema structural validation error details.
 */
struct SchemaStructureError {
    enum class ViolationType {
        NULL_TABLE_NAME,           ///< Table has empty/null name
        NULL_COLUMN_NAME,          ///< Column has empty/null name
        DUPLICATE_COLUMN,          ///< Column name appears multiple times
        INVALID_TYPE_STRING,       ///< Unknown/invalid type string
        OVERSIZED_IDENTIFIER,      ///< Identifier exceeds max length
        NONE                        ///< No violation
    };
    
    ViolationType violation_type{ViolationType::NONE};
    std::string table_name;
    std::string column_name;
    std::string error_message;
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

    // -----------------------------------------------------------------
    // I2: Input validation helpers (Phase 4 hardening)
    // -----------------------------------------------------------------

    /// Maximum allowed length for a table or column identifier.
    static constexpr size_t kMaxIdentifierLength = 128;

    /// Maximum number of tables accepted by inferImplicitRelationships()
    /// and estimateCardinalities().  Inputs exceeding this are rejected
    /// to prevent quadratic O(n²) worst-case CPU/memory blow-up.
    static constexpr size_t kMaxTableCount = 5000;

    /// Maximum number of columns per table accepted by validation.
    static constexpr size_t kMaxColumnCount = 1600;

    /// Maximum number of table pairs to compare for relationship inference.
    /// Used to bound O(n²) complexity in relationship discovery.
    /// PHASE-2-HARDENING
    static constexpr size_t kMaxTablePairsComparison = 10000;

    /// Maximum number of column pairs per table to compare.
    /// Used to bound O(n²) complexity in cardinality estimation.
    /// PHASE-2-HARDENING
    static constexpr size_t kMaxColumnPairsPerTable = 2500;

    /**
     * @brief Validate a SQL identifier (table or column name) for safe use
     *        in dynamically-constructed query strings.
     *
     * Accepts identifiers consisting solely of ASCII letters, digits, and
     * underscores, between 1 and kMaxIdentifierLength characters. All SQL
     * metacharacters (quotes, semicolons, dashes, dots, spaces, etc.) cause
     * the function to return false.
     *
     * @param identifier  The string to validate.
     * @return true if the identifier is safe for SQL use; false otherwise.
     */
    static bool isValidIdentifier(const std::string& identifier);

    /**
     * @brief Validate the structural integrity of a schema set.
     *
     * Checks for:
     *   - Null or empty table names
     *   - Null or empty column names
     *   - Duplicate column names within a table
     *   - Invalid type strings (non-alphanumeric or oversized)
     *   - Oversized identifiers
     *
     * @param schemas  Vector of schemas to validate.
     * @return List of structural violations (empty if all valid).
     * PHASE-2-HARDENING
     */
    static std::vector<SchemaStructureError> validateSchemaStructure(
        const std::vector<InferenceTableSchema>& schemas
    );

    /**
     * @brief Detect circular foreign key references (cycles) in implicit relationships.
     *
     * A cycle occurs when relationship A → B and B → A exist, which would cause
     * infinite recursion in schema analysis. This method identifies such cycles
     * and returns a map of cycles detected.
     *
     * @param inferred_schemas  Inferred schemas from inferImplicitRelationships().
     * @return Map of relationship_id → list of cycle-forming relationship IDs.
     * PHASE-2-HARDENING
     */
    static std::map<std::string, std::vector<std::string>> detectRelationshipCycles(
        const std::vector<InferredSchema>& inferred_schemas
    );

private:
    Config config_;

    bool columnNameSimilar(const std::string& a, const std::string& b) const;
    double jaccardSimilarity(const std::vector<std::string>& a,
                             const std::vector<std::string>& b) const;
    SemanticType detectSingleColumn(const std::vector<std::string>& values) const;
};

} // namespace importers
} // namespace themis
