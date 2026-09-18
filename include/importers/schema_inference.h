/**
 * @file schema_inference.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct SampleData {
    std::string table_name;
    std::string column_name;
    std::vector<std::string> values;
};

struct InferenceTableSchema {
    std::string name;
    std::string schema_ns;
    std::vector<std::string> columns;
    std::map<std::string, std::string> column_types;
    std::vector<std::string> primary_keys;
    std::vector<std::pair<std::string, std::string>> foreign_keys; ///< (local_col, ref_table.ref_col)
};

struct SchemaInferenceConfig {
    double relationship_confidence_threshold{0.75};
    double semantic_type_confidence_threshold{0.70};  ///< Min agreement % for semantic type
    size_t max_sample_values{1000};
    bool enable_semantic_detection{true};
    bool enable_cycle_detection{true};  ///< Detect circular FK references
};

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

    std::map<std::string, SemanticType> detectSemanticTypes(
        const std::vector<InferenceTableSchema>& schemas,
        const std::vector<SampleData>& samples
    );

    /**
     * @brief Semantic Type To String.
     * @param[in] t Input parameter.
     * @return Return value.
     */
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

    static constexpr size_t kMaxIdentifierLength = 128;

    static constexpr size_t kMaxTableCount = 5000;

    static constexpr size_t kMaxColumnCount = 1600;

    static constexpr size_t kMaxTablePairsComparison = 10000;

    static constexpr size_t kMaxColumnPairsPerTable = 2500;

    /**
     * @brief Is Valid Identifier.
     * @param[in] identifier Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isValidIdentifier(const std::string& identifier);

    /**
     * @brief Validate Schema Structure.
     * @param[in] schemas Input parameter.
     * @return Return value.
     */
    static std::vector<SchemaStructureError> validateSchemaStructure(
        const std::vector<InferenceTableSchema>& schemas
    );

    static std::map<std::string, std::vector<std::string>> detectRelationshipCycles(
        const std::vector<InferredSchema>& inferred_schemas
    );

private:
    Config config_;

    /**
     * @brief Column Name Similar.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return True when the operation succeeds.
     */
    bool columnNameSimilar(const std::string& a, const std::string& b) const;
    /**
     * @brief Jaccard Similarity.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    double jaccardSimilarity(const std::vector<std::string>& a,
                             const std::vector<std::string>& b) const;
    /**
     * @brief Detect Single Column.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    SemanticType detectSingleColumn(const std::vector<std::string>& values) const;
};

} // namespace importers
} // namespace themis
