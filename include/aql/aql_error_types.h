/**
 * @file aql_error_types.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.48
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 92/100
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

class AQLErrorContext {
public:
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

    const std::string& getErrorType() const { return error_type_; }

    const std::string& getCategory() const { return category_; }

    const std::string& getComponent() const { return component_; }

    const std::string& getMessage() const { return message_; }

    std::time_t getTimestamp() const { return timestamp_; }

    /**
     * @brief Set Operation Type.
     * @param[in] op_type Input parameter.
     * @details Implements setOperationType without additional internal calls.
     */
    void setOperationType(const std::string& op_type) { operation_type_ = op_type; }

    const std::string& getOperationType() const { return operation_type_; }

    /**
     * @brief Set Line Number.
     * @param[in] line Input parameter.
     * @details Implements setLineNumber without additional internal calls.
     */
    void setLineNumber(uint32_t line) { line_number_ = line; }

    uint32_t getLineNumber() const { return line_number_; }

    /**
     * @brief Set Token Position.
     * @param[in] pos Input parameter.
     * @details Implements setTokenPosition without additional internal calls.
     */
    void setTokenPosition(uint32_t pos) { token_position_ = pos; }

    uint32_t getTokenPosition() const { return token_position_; }

    /**
     * @brief Add Diagnostic Hint.
     * @param[in] hint Input parameter.
     * @details Calls: push_back().
     */
    void addDiagnosticHint(const std::string& hint) {
        diagnostic_hints_.push_back(hint);
    }

    const std::vector<std::string>& getDiagnosticHints() const {
        return diagnostic_hints_;
    }

    /**
     * @brief Set Retry Count.
     * @param[in] count Input parameter.
     * @details Implements setRetryCount without additional internal calls.
     */
    void setRetryCount(uint32_t count) { retry_count_ = count; }

    uint32_t getRetryCount() const { return retry_count_; }

    /**
     * @brief Set Recoverable.
     * @param[in] recoverable Input parameter.
     * @details Implements setRecoverable without additional internal calls.
     */
    void setRecoverable(bool recoverable) { is_recoverable_ = recoverable; }

    bool isRecoverable() const { return is_recoverable_; }

    void setSchemaContext(const std::string& field_name, const std::string& collection_name,
                          const std::string& type_info = "") {
        schema_field_ = field_name;
        schema_collection_ = collection_name;
        schema_type_info_ = type_info;
    }

    const std::string& getSchemaField() const { return schema_field_; }

    const std::string& getSchemaCollection() const { return schema_collection_; }

    const std::string& getSchemaTypeInfo() const { return schema_type_info_; }

    std::string formatForLogging() const {
        std::ostringstream oss = {};
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
                if (i > 0) {
                  oss << "; ";
                }
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

namespace ValidationError {
    constexpr const char* MalformedAQL = "MalformedAQL";           ///< Invalid AQL syntax
    constexpr const char* InjectionAttempt = "InjectionAttempt";   ///< Detected prompt/SQL injection
    constexpr const char* SchemaMismatch = "SchemaMismatch";       ///< Query references non-existent fields/collections
    constexpr const char* UnsupportedOperator = "UnsupportedOperator";  ///< Query uses unsupported AQL operator
    constexpr const char* TypeMismatch = "TypeMismatch";          ///< Field type doesn't match filter operation
    constexpr const char* NullSchemaContext = "NullSchemaContext"; ///< Schema metadata is missing/null
    constexpr const char* MissingFieldMetadata = "MissingFieldMetadata";  ///< Field metadata incomplete
}

namespace TranslationError {
    constexpr const char* GenerationFailed = "GenerationFailed";       ///< LLM generation failed
    constexpr const char* RetryExhausted = "RetryExhausted";           ///< Retry attempts exhausted
    constexpr const char* ContextOverflow = "ContextOverflow";         ///< Context window exhausted
    constexpr const char* ProviderUnavailable = "ProviderUnavailable"; ///< LLM provider not available
    constexpr const char* TimeoutExceeded = "TimeoutExceeded";         ///< Generation timeout
    constexpr const char* InvalidResponse = "InvalidResponse";         ///< LLM response failed post-generation validation
}

namespace BridgeError {
    constexpr const char* ExecutionFailed = "ExecutionFailed";         ///< Bridge execution failed
    constexpr const char* EmbeddingGenerationFailed = "EmbeddingGenerationFailed";  ///< Embedding generation failed
    constexpr const char* InvalidSchema = "InvalidSchema";             ///< Schema context invalid for bridge
    constexpr const char* ResourceExhausted = "ResourceExhausted";     ///< Memory/CPU resources exhausted
    constexpr const char* TimeoutExceeded = "TimeoutExceeded";         ///< Bridge operation timeout
    constexpr const char* ContextBoundExceeded = "ContextBoundExceeded";  ///< Conversation context limit exceeded
}

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

enum class RecoveryStrategy {
    FAIL_CLOSED,           ///< Reject operation explicitly with diagnostic
    RETRY_WITH_BACKOFF,    ///< Retry with exponential backoff
    DEGRADE_GRACEFULLY,    ///< Fall back to simpler operation
    RESET_STATE,           ///< Reset component state and retry
    PROPAGATE_ERROR,       ///< Propagate error to caller
    CIRCUIT_BREAK          ///< Open circuit breaker to prevent cascading failures
};

/**
 * @brief Get Recovery Strategy.
 * @param[in] error_type Input parameter.
 * @param[in] category Input parameter.
 * @return Return value.
 * @details Implements getRecoveryStrategy without additional internal calls.
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
