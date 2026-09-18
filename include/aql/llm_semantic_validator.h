/**
 * @file llm_semantic_validator.h
 * @brief Semantic validation layer for LLM-generated AQL queries
 * @version 0.1.0
 *
 * Provides advanced semantic validation beyond syntax checking, including:
 * - Type checking (attribute types vs. filter/projection operations)
 * - Cardinality estimation (join selectivity, aggregation impacts)
 * - Join order validation (circular dependencies, impossible joins)
 * - Function signature validation (parameter type compatibility)
 *
 * @note This validator operates on validated AQL ASTs (post-parser).
 * @see src/aql/AQL_ARCHITECTURE_MASTER.md (Section 8.2: Semantic Validation)
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>
#include <chrono>

#include "query/aql_parser.h"  // AST definitions
#include "aql/llm_error_codes.h"

namespace themis {
namespace aql {

/**
 * @brief Result type for semantic validation
 *
 * Contains validation status, error details, warnings, and confidence score.
 */
struct SemanticValidationResult {
    enum class Status {
        VALID,                    ///< Query passes all semantic checks
        TYPE_MISMATCH,            ///< Attribute type incompatible with operation
        CARDINALITY_WARNING,      ///< Query may be inefficient (estimated large result set)
        JOIN_IMPOSSIBLE,          ///< Join references non-existent collection/attribute
        FUNCTION_SIGNATURE_ERROR, ///< Function parameters don't match signature
        TIMEOUT,                  ///< Semantic validation exceeded time limit
        UNKNOWN_ERROR             ///< Internal validation error
    };

    Status status = Status::VALID;
    std::string error_message;
    std::vector<std::string> warnings;
    
    /// Confidence score (0.0 to 1.0) for the query quality
    double confidence_score = 1.0;
    
    /// Estimated output cardinality (rows)
    std::optional<size_t> estimated_output_rows;
    
    /// Time spent in semantic validation (ms)
    std::chrono::milliseconds validation_latency_ms{0};

    bool isValid() const { return status == Status::VALID; }
};

/**
 * @brief Semantic type information for a collection attribute
 */
struct AttributeTypeInfo {
    std::string collection_name;
    std::string attribute_name;
    std::string data_type;           // e.g., "string", "number", "boolean"
    bool is_nullable = true;
    std::optional<size_t> cardinality; // Number of distinct values
};

/**
 * @brief Schema context provider for semantic validation
 *
 * Abstracts how semantic validator accesses collection schemas,
 * function signatures, and cardinality statistics.
 */
class SemanticSchemaContext {
public:
    virtual ~SemanticSchemaContext() = default;

    /// Retrieve type information for an attribute
    virtual std::optional<AttributeTypeInfo> getAttributeType(
        const std::string& collection_name,
        const std::string& attribute_name) const = 0;

    /// List all collections available in current context
    virtual std::vector<std::string> listCollections() const = 0;

    /// Check if a function exists and retrieve signature
    virtual bool isFunctionDefined(const std::string& function_name) const = 0;

    /// Get estimated cardinality (row count) for a collection
    virtual std::optional<size_t> getCollectionCardinality(
        const std::string& collection_name) const = 0;
};

/**
 * @brief Semantic validator for LLM-generated AQL queries
 *
 * Performs type checking, cardinality estimation, and semantic constraint
 * validation on ASTs that have already passed syntax validation.
 *
 * @see Configuration: configure() method
 */
class LLMSemanticValidator {
public:
    /// Configuration for semantic validation behavior
    struct Config {
        /// Maximum time allowed for semantic validation (ms)
        std::chrono::milliseconds validation_timeout_ms{5000};

        /// Warn if estimated output exceeds this threshold (rows)
        size_t cardinality_warning_threshold = 1000000;

        /// Enable type checking
        bool enable_type_checking = true;

        /// Enable cardinality estimation
        bool enable_cardinality_estimation = true;

        /// Enable join validation
        bool enable_join_validation = true;

        /// Enable function signature validation
        bool enable_function_validation = true;

        /// Minimum confidence score for query acceptance (0.0 to 1.0)
        double min_confidence_score = 0.6;
    };

    explicit LLMSemanticValidator(
        std::shared_ptr<SemanticSchemaContext> schema_context,
        const Config& config = Config());

    virtual ~LLMSemanticValidator() = default;

    /**
     * @brief Validate semantic constraints on a parsed AQL AST
     *
     * @param ast The abstract syntax tree (post-parser validation)
     * @return Validation result with status, confidence score, and diagnostics
     *
     * @throws LLMException on internal validation errors (not schema issues)
     */
    SemanticValidationResult validate(const query::ASTNode* ast);

    /**
     * @brief Reconfigure validation behavior at runtime
     *
     * @param config New configuration parameters
     */
    void configure(const Config& config);

    /**
     * @brief Get current configuration
     */
    const Config& getConfig() const;

private:
    std::shared_ptr<SemanticSchemaContext> schema_context_;
    Config config_;
    std::shared_ptr<spdlog::logger> logger_;

    /// Perform type checking on expressions
    void checkAttributeTypes(const query::ASTNode* ast, SemanticValidationResult& result);

    /// Estimate output cardinality based on AST operations
    void estimateCardinality(const query::ASTNode* ast, SemanticValidationResult& result);

    /// Validate join feasibility and detect impossible joins
    void validateJoins(const query::ASTNode* ast, SemanticValidationResult& result);

    /// Validate function calls against known signatures
    void validateFunctionSignatures(const query::ASTNode* ast, SemanticValidationResult& result);

    /// Calculate confidence score based on validation findings
    void computeConfidenceScore(SemanticValidationResult& result);
};

} // namespace aql
} // namespace themis
