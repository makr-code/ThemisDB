/**
 * @file aql_error_types.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.48
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 92/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note Phase 4 Enhancement (Error Taxonomy Consolidation) - 2026-07-19
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <cstdint>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>

namespace themis {
namespace aql {

/**
 * @brief Structured error context for comprehensive error diagnostics.
 *
 * Provides rich error information including operation type, component, timestamp,
 * and actionable diagnostic hints for production triage and debugging.
 *
 * Usage example:
 * @code
 *   AQLErrorContext ctx(AQLErrorType::ValidationError,
 *                       AQLErrorCategory::MalformedAQL,
 *                       "validateAQLWithParser",
 *                       "Schema field 'age' has undefined type annotation");
 *   ctx.addDiagnosticHint("Check schema metadata for collection 'users'");
 *   ctx.setOperationType("translate_nl_to_aql");
 *   ctx.setLineNumber(5);
 *   ctx.setTokenPosition(42);
 * @endcode
 */
class AQLErrorContext {
public:
    /**
     * @brief Create error context with comprehensive metadata.
     *
     * @param error_type Main error type (validation, translation, bridge, provider)
     * @param category Specific error category (malformed AQL, injection, timeout, etc.)
     * @param component Component that generated the error (e.g., "aql_validator", "llm_handler")
     * @param message Human-readable error message
     */
    AQLErrorContext(const std::string& error_type,
                    const std::string& category,
                    const std::string& component,
                    const std::string& message)
        : error_type_(error_type)
        , category_(category)
        , component_(component)
        , message_(message)
        , timestamp_(std::time(nullptr))
        , line_number_(0)
        , token_position_(0)
        , retry_count_(0)
        , is_recoverable_(false) {}

    /// @brief Get the main error type (validation, translation, bridge, provider)
    const std::string& getErrorType() const { return error_type_; }

    /// @brief Get the specific error category (MalformedAQL, InjectionAttempt, etc.)
    const std::string& getCategory() const { return category_; }

    /// @brief Get the component that generated the error
    const std::string& getComponent() const { return component_; }

    /// @brief Get the error message
    const std::string& getMessage() const { return message_; }

    /// @brief Get the timestamp when the error occurred
    std::time_t getTimestamp() const { return timestamp_; }

    /// @brief Set the operation type (e.g., "translate_nl_to_aql", "validate_schema")
    void setOperationType(const std::string& op_type) { operation_type_ = op_type; }

    /// @brief Get the operation type
    const std::string& getOperationType() const { return operation_type_; }

    /// @brief Set the line number where the error occurred (for AST/parse errors)
    void setLineNumber(uint32_t line) { line_number_ = line; }

    /// @brief Get the line number
    uint32_t getLineNumber() const { return line_number_; }

    /// @brief Set the token/character position in the query string
    void setTokenPosition(uint32_t pos) { token_position_ = pos; }

    /// @brief Get the token position
    uint32_t getTokenPosition() const { return token_position_; }

    /// @brief Add a diagnostic hint for production triage (actionable advice for operators)
    void addDiagnosticHint(const std::string& hint) {
        diagnostic_hints_.push_back(hint);
    }

    /// @brief Get all diagnostic hints
    const std::vector<std::string>& getDiagnosticHints() const {
        return diagnostic_hints_;
    }

    /// @brief Set the retry count (how many times recovery was attempted)
    void setRetryCount(uint32_t count) { retry_count_ = count; }

    /// @brief Get the retry count
    uint32_t getRetryCount() const { return retry_count_; }

    /// @brief Mark whether this error is recoverable
    void setRecoverable(bool recoverable) { is_recoverable_ = recoverable; }

    /// @brief Check if error is recoverable
    bool isRecoverable() const { return is_recoverable_; }

    /// @brief Set schema-related context (field name, collection name, type mismatch info)
    void setSchemaContext(const std::string& field_name, const std::string& collection_name,
                          const std::string& type_info = "") {
        schema_field_ = field_name;
        schema_collection_ = collection_name;
        schema_type_info_ = type_info;
    }

    /// @brief Get schema field name
    const std::string& getSchemaField() const { return schema_field_; }

    /// @brief Get collection name
    const std::string& getSchemaCollection() const { return schema_collection_; }

    /// @brief Get type information
    const std::string& getSchemaTypeInfo() const { return schema_type_info_; }

    /**
     * @brief Generate a formatted error report for logging/diagnostics.
     *
     * @return Structured error report with all context information
     */
    std::string formatForLogging() const {
        std::ostringstream oss;
        oss << "[AQLError] Type=" << error_type_
            << " Category=" << category_
            << " Component=" << component_;
        
        if (!operation_type_.empty()) {
            oss << " Operation=" << operation_type_;
        }
        
        oss << " Message=\"" << message_ << "\"";
        
        if (line_number_ > 0) {
            oss << " Line=" << line_number_;
        }
        
        if (token_position_ > 0) {
            oss << " Position=" << token_position_;
        }
        
        if (!schema_field_.empty()) {
            oss << " SchemaField=" << schema_field_;
        }
        
        if (!schema_collection_.empty()) {
            oss << " Collection=" << schema_collection_;
        }
        
        if (!diagnostic_hints_.empty()) {
            oss << " Hints=[";
            for (size_t i = 0; i < diagnostic_hints_.size(); ++i) {
                if (i > 0) oss << "; ";
                oss << diagnostic_hints_[i];
            }
            oss << "]";
        }
        
        oss << " Recoverable=" << (is_recoverable_ ? "yes" : "no");
        
        if (retry_count_ > 0) {
            oss << " Retries=" << retry_count_;
        }
        
        return oss.str();
    }

private:
    std::string error_type_;           ///< Main error type (validation, translation, bridge, provider)
    std::string category_;              ///< Specific error category
    std::string component_;             ///< Component that generated error
    std::string message_;               ///< Human-readable error message
    std::time_t timestamp_;             ///< When error occurred
    std::string operation_type_;        ///< Operation being performed (e.g., "translate_nl_to_aql")
    uint32_t line_number_;              ///< Line number in query (for parse errors)
    uint32_t token_position_;           ///< Character position in query
    std::vector<std::string> diagnostic_hints_;  ///< Actionable hints for triage
    uint32_t retry_count_;              ///< Number of recovery attempts
    bool is_recoverable_;               ///< Whether error can be recovered
    std::string schema_field_;          ///< Schema field name (if applicable)
    std::string schema_collection_;     ///< Collection name (if applicable)
    std::string schema_type_info_;      ///< Type information (if applicable)
};

// ============================================================================
// Error Category Definitions
// ============================================================================

/**
 * @brief Validation Error Categories
 *
 * Errors that occur during AQL query validation and schema checking.
 */
namespace ValidationError {
    constexpr const char* MalformedAQL = "MalformedAQL";           ///< Invalid AQL syntax
    constexpr const char* InjectionAttempt = "InjectionAttempt";   ///< Detected prompt/SQL injection
    constexpr const char* SchemaMismatch = "SchemaMismatch";       ///< Query references non-existent fields/collections
    constexpr const char* UnsupportedOperator = "UnsupportedOperator";  ///< Query uses unsupported AQL operator
    constexpr const char* TypeMismatch = "TypeMismatch";          ///< Field type doesn't match filter operation
    constexpr const char* NullSchemaContext = "NullSchemaContext"; ///< Schema metadata is missing/null
    constexpr const char* MissingFieldMetadata = "MissingFieldMetadata";  ///< Field metadata incomplete
}

/**
 * @brief Translation Error Categories
 *
 * Errors that occur during NL-to-AQL translation process.
 */
namespace TranslationError {
    constexpr const char* GenerationFailed = "GenerationFailed";       ///< LLM generation failed
    constexpr const char* RetryExhausted = "RetryExhausted";           ///< Retry attempts exhausted
    constexpr const char* ContextOverflow = "ContextOverflow";         ///< Context window exhausted
    constexpr const char* ProviderUnavailable = "ProviderUnavailable"; ///< LLM provider not available
    constexpr const char* TimeoutExceeded = "TimeoutExceeded";         ///< Generation timeout
    constexpr const char* InvalidResponse = "InvalidResponse";         ///< LLM response failed post-generation validation
}

/**
 * @brief Bridge/Helper Error Categories
 *
 * Errors from embedding bridge, highlighter, scorer, and helper components.
 */
namespace BridgeError {
    constexpr const char* ExecutionFailed = "ExecutionFailed";         ///< Bridge execution failed
    constexpr const char* EmbeddingGenerationFailed = "EmbeddingGenerationFailed";  ///< Embedding generation failed
    constexpr const char* InvalidSchema = "InvalidSchema";             ///< Schema context invalid for bridge
    constexpr const char* ResourceExhausted = "ResourceExhausted";     ///< Memory/CPU resources exhausted
    constexpr const char* TimeoutExceeded = "TimeoutExceeded";         ///< Bridge operation timeout
    constexpr const char* ContextBoundExceeded = "ContextBoundExceeded";  ///< Conversation context limit exceeded
}

/**
 * @brief Provider Error Categories
 *
 * Errors from LLM, RAG, embedding, and fine-tuning providers.
 */
namespace ProviderError {
    constexpr const char* InferFailed = "InferFailed";                 ///< Inference provider error
    constexpr const char* RAGFailed = "RAGFailed";                     ///< RAG provider error
    constexpr const char* EmbedFailed = "EmbedFailed";                 ///< Embedding provider error
    constexpr const char* FinetuneFailed = "FinetuneFailed";           ///< Fine-tuning provider error
    constexpr const char* CircuitBreakerOpen = "CircuitBreakerOpen";   ///< Circuit breaker is open
    constexpr const char* ProviderTimeout = "ProviderTimeout";         ///< Provider request timeout
    constexpr const char* ProviderUnavailable = "ProviderUnavailable"; ///< Provider service unavailable
}

// ============================================================================
// Error Recovery Strategies
// ============================================================================

/**
 * @brief Recovery action for different error types.
 *
 * Maps errors to appropriate recovery strategies and fail-closed/fail-open decisions.
 */
enum class RecoveryStrategy {
    FAIL_CLOSED,           ///< Reject operation explicitly with diagnostic
    RETRY_WITH_BACKOFF,    ///< Retry with exponential backoff
    DEGRADE_GRACEFULLY,    ///< Fall back to simpler operation
    RESET_STATE,           ///< Reset component state and retry
    PROPAGATE_ERROR,       ///< Propagate error to caller
    CIRCUIT_BREAK          ///< Open circuit breaker to prevent cascading failures
};

/**
 * @brief Determine recovery strategy for a given error.
 *
 * @param error_type Error type (validation, translation, bridge, provider)
 * @param category Error category (MalformedAQL, timeout, etc.)
 * @return Recovery strategy to apply
 */
inline RecoveryStrategy getRecoveryStrategy(const std::string& error_type,
                                           const std::string& category) {
    // Validation errors: always fail-closed
    if (error_type == "validation") {
        if (category == ValidationError::InjectionAttempt ||
            category == ValidationError::MalformedAQL ||
            category == ValidationError::SchemaMismatch ||
            category == ValidationError::TypeMismatch ||
            category == ValidationError::UnsupportedOperator ||
            category == ValidationError::NullSchemaContext ||
            category == ValidationError::MissingFieldMetadata) {
            return RecoveryStrategy::FAIL_CLOSED;
        }
    }

    // Translation errors: context overflow degrades, retry exhaustion propagates,
    // remaining translation failures retry with backoff.
    if (error_type == "translation") {
        if (category == TranslationError::ContextOverflow) {
            return RecoveryStrategy::DEGRADE_GRACEFULLY;
        }
        if (category == TranslationError::RetryExhausted) {
            return RecoveryStrategy::PROPAGATE_ERROR;
        }
        if (category == TranslationError::ProviderUnavailable ||
            category == TranslationError::TimeoutExceeded ||
            category == TranslationError::InvalidResponse ||
            category == TranslationError::GenerationFailed) {
            return RecoveryStrategy::RETRY_WITH_BACKOFF;
        }
    }

    // Bridge errors: degrade or retry
    if (error_type == "bridge") {
        if (category == BridgeError::ContextBoundExceeded) {
            return RecoveryStrategy::DEGRADE_GRACEFULLY;
        }
        if (category == BridgeError::TimeoutExceeded) {
            return RecoveryStrategy::RETRY_WITH_BACKOFF;
        }
        if (category == BridgeError::ResourceExhausted) {
            return RecoveryStrategy::DEGRADE_GRACEFULLY;
        }
    }

    // Provider errors: circuit break or degrade
    if (error_type == "provider") {
        if (category == ProviderError::CircuitBreakerOpen ||
            category == ProviderError::ProviderUnavailable) {
            return RecoveryStrategy::DEGRADE_GRACEFULLY;
        }
        if (category == ProviderError::ProviderTimeout) {
            return RecoveryStrategy::RETRY_WITH_BACKOFF;
        }
    }

    // Default: propagate error
    return RecoveryStrategy::PROPAGATE_ERROR;
}

}  // namespace aql
}  // namespace themis
