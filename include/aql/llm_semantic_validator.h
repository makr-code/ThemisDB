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
    
    double confidence_score = 1.0;
    
    std::optional<size_t> estimated_output_rows;
    
    std::chrono::milliseconds validation_latency_ms{0};

    bool isValid() const { return status == Status::VALID; }
};

struct AttributeTypeInfo {
    std::string collection_name;
    std::string attribute_name;
    std::string data_type;           // e.g., "string", "number", "boolean"
    bool is_nullable = true;
    std::optional<size_t> cardinality; // Number of distinct values
};

class SemanticSchemaContext {
public:
    /**
     * @brief Semantic Schema Context.
     * @return Return value.
     */
    virtual ~SemanticSchemaContext() = default;

    /**
     * @brief Get Attribute Type.
     * @param[in] collection_name Name of the collection.
     * @param[in] attribute_name Name of the attribute.
     * @return Return value.
     */
    virtual std::optional<AttributeTypeInfo> getAttributeType(
        const std::string& collection_name,
        const std::string& attribute_name) const = 0;

    /**
     * @brief List Collections.
     * @return Return value.
     */
    virtual std::vector<std::string> listCollections() const = 0;

    /**
     * @brief Is Function Defined.
     * @param[in] function_name Name of the function.
     * @return True when the operation succeeds.
     */
    virtual bool isFunctionDefined(const std::string& function_name) const = 0;

    /**
     * @brief Get Collection Cardinality.
     * @param[in] collection_name Name of the collection.
     * @return Return value.
     */
    virtual std::optional<size_t> getCollectionCardinality(
        const std::string& collection_name) const = 0;
};

class LLMSemanticValidator {
public:
    struct Config {
        std::chrono::milliseconds validation_timeout_ms{5000};

        size_t cardinality_warning_threshold = 1000000;

        bool enable_type_checking = true;

        bool enable_cardinality_estimation = true;

        bool enable_join_validation = true;

        bool enable_function_validation = true;

        double min_confidence_score = 0.6;
    };

    explicit LLMSemanticValidator(
        std::shared_ptr<SemanticSchemaContext> schema_context,
        const Config& config = Config());

    /**
     * @brief LLMSemantic Validator.
     * @return Return value.
     */
    virtual ~LLMSemanticValidator() = default;

    /**
     * @brief Validate.
     * @param[in] ast Input parameter.
     * @return Return value.
     */
    SemanticValidationResult validate(const query::ASTNode* ast);

    /**
     * @brief Configure.
     * @param[in] config Input parameter.
     */
    void configure(const Config& config);

    /**
     * @brief Get Config.
     * @return Return value.
     */
    const Config& getConfig() const;

private:
    std::shared_ptr<SemanticSchemaContext> schema_context_;
    Config config_;
    std::shared_ptr<spdlog::logger> logger_;

    /**
     * @brief Check Attribute Types.
     * @param[in] ast Input parameter.
     * @param[in,out] result Input/output parameter.
     */
    void checkAttributeTypes(const query::ASTNode* ast, SemanticValidationResult& result);

    /**
     * @brief Estimate Cardinality.
     * @param[in] ast Input parameter.
     * @param[in,out] result Input/output parameter.
     */
    void estimateCardinality(const query::ASTNode* ast, SemanticValidationResult& result);

    /**
     * @brief Validate Joins.
     * @param[in] ast Input parameter.
     * @param[in,out] result Input/output parameter.
     */
    void validateJoins(const query::ASTNode* ast, SemanticValidationResult& result);

    /**
     * @brief Validate Function Signatures.
     * @param[in] ast Input parameter.
     * @param[in,out] result Input/output parameter.
     */
    void validateFunctionSignatures(const query::ASTNode* ast, SemanticValidationResult& result);

    /**
     * @brief Compute Confidence Score.
     * @param[in,out] result Input/output parameter.
     */
    void computeConfidenceScore(SemanticValidationResult& result);
};

} // namespace aql
} // namespace themis
