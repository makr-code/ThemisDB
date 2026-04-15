/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_error_codes.h                                  ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     330                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3a758b465a  2026-04-12  feat(aql): AQL module enhancements — Features 8, 10, 12, ... ║
    • dc13f3fdb3  2026-03-13  feat(aql): implement post-generation AQL validation with ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <stdexcept>

namespace themis {
namespace aql {

/**
 * @brief Error code taxonomy for LLM operations
 * 
 * Provides structured error codes for better error handling and monitoring.
 * Error codes follow the pattern: LLM_<CATEGORY>_<SPECIFIC_ERROR>
 */
enum class LLMErrorCode {
    // Input validation errors (1xxx)
    INVALID_PROMPT = 1001,
    PROMPT_TOO_LONG = 1002,
    INVALID_MODEL_ID = 1003,
    INVALID_LORA_ID = 1004,
    INVALID_COLLECTION = 1005,
    INVALID_OPTIONS = 1006,
    PROMPT_INJECTION = 1007,   // Input rejected due to detected prompt injection attempt
    
    // Model errors (2xxx)
    MODEL_NOT_FOUND = 2001,
    MODEL_LOAD_FAILED = 2002,
    MODEL_UNLOAD_FAILED = 2003,
    MODEL_NOT_LOADED = 2004,
    
    // LoRA errors (3xxx)
    LORA_NOT_FOUND = 3001,
    LORA_LOAD_FAILED = 3002,
    LORA_UNLOAD_FAILED = 3003,
    LORA_NOT_LOADED = 3004,
    
    // Execution errors (4xxx)
    INFERENCE_FAILED = 4001,
    RAG_FAILED = 4002,
    EMBEDDING_FAILED = 4003,
    TIMEOUT = 4004,
    INVALID_RESPONSE = 4005,   ///< LLM-generated output failed post-generation validation
    
    // Resource errors (5xxx)
    OUT_OF_MEMORY = 5001,
    CACHE_FULL = 5002,
    
    // Internal errors (9xxx)
    INTERNAL_ERROR = 9001,
    UNKNOWN_ERROR = 9999
};

/**
 * @brief Exception class for LLM operations
 * 
 * Provides structured error information with error codes and correlation IDs.
 */
class LLMException : public std::runtime_error {
public:
    LLMException(LLMErrorCode code, const std::string& message)
        : std::runtime_error(message)
        , error_code_(code)
        , correlation_id_("")
    {}
    
    LLMException(LLMErrorCode code, const std::string& message, const std::string& correlation_id)
        : std::runtime_error(message)
        , error_code_(code)
        , correlation_id_(correlation_id)
    {}
    
    LLMErrorCode getErrorCode() const { return error_code_; }
    const std::string& getCorrelationId() const { return correlation_id_; }
    
    /**
     * @brief Get user-friendly error message (masks internal details)
     */
    std::string getSafeMessage() const {
        return formatErrorMessage(error_code_, what());
    }
    
    /**
     * @brief Get error code as string
     */
    static std::string getErrorCodeString(LLMErrorCode code) {
        switch (code) {
            case LLMErrorCode::INVALID_PROMPT: return "LLM_INVALID_PROMPT";
            case LLMErrorCode::PROMPT_TOO_LONG: return "LLM_PROMPT_TOO_LONG";
            case LLMErrorCode::INVALID_MODEL_ID: return "LLM_INVALID_MODEL_ID";
            case LLMErrorCode::INVALID_LORA_ID: return "LLM_INVALID_LORA_ID";
            case LLMErrorCode::INVALID_COLLECTION: return "LLM_INVALID_COLLECTION";
            case LLMErrorCode::INVALID_OPTIONS: return "LLM_INVALID_OPTIONS";
            case LLMErrorCode::MODEL_NOT_FOUND: return "LLM_MODEL_NOT_FOUND";
            case LLMErrorCode::MODEL_LOAD_FAILED: return "LLM_MODEL_LOAD_FAILED";
            case LLMErrorCode::MODEL_UNLOAD_FAILED: return "LLM_MODEL_UNLOAD_FAILED";
            case LLMErrorCode::MODEL_NOT_LOADED: return "LLM_MODEL_NOT_LOADED";
            case LLMErrorCode::LORA_NOT_FOUND: return "LLM_LORA_NOT_FOUND";
            case LLMErrorCode::LORA_LOAD_FAILED: return "LLM_LORA_LOAD_FAILED";
            case LLMErrorCode::LORA_UNLOAD_FAILED: return "LLM_LORA_UNLOAD_FAILED";
            case LLMErrorCode::LORA_NOT_LOADED: return "LLM_LORA_NOT_LOADED";
            case LLMErrorCode::INFERENCE_FAILED: return "LLM_INFERENCE_FAILED";
            case LLMErrorCode::RAG_FAILED: return "LLM_RAG_FAILED";
            case LLMErrorCode::EMBEDDING_FAILED: return "LLM_EMBEDDING_FAILED";
            case LLMErrorCode::TIMEOUT: return "LLM_TIMEOUT";
            case LLMErrorCode::INVALID_RESPONSE: return "LLM_INVALID_RESPONSE";
            case LLMErrorCode::OUT_OF_MEMORY: return "LLM_OUT_OF_MEMORY";
            case LLMErrorCode::CACHE_FULL: return "LLM_CACHE_FULL";
            case LLMErrorCode::INTERNAL_ERROR: return "LLM_INTERNAL_ERROR";
            case LLMErrorCode::UNKNOWN_ERROR: return "LLM_UNKNOWN_ERROR";
            default: return "LLM_UNKNOWN_ERROR";
        }
    }

private:
    LLMErrorCode error_code_;
    std::string correlation_id_;
    
    /**
     * @brief Format error message for user consumption (masks internal details)
     */
    static std::string formatErrorMessage(LLMErrorCode code, const std::string& internal_msg) {
        // For user-facing errors, provide generic safe messages
        switch (code) {
            case LLMErrorCode::INVALID_PROMPT:
                return "Invalid prompt provided";
            case LLMErrorCode::PROMPT_TOO_LONG:
                return "Prompt exceeds maximum allowed length";
            case LLMErrorCode::INVALID_MODEL_ID:
                return "Invalid model identifier";
            case LLMErrorCode::INVALID_LORA_ID:
                return "Invalid LoRA adapter identifier";
            case LLMErrorCode::INVALID_COLLECTION:
                return "Invalid collection name";
            case LLMErrorCode::INVALID_OPTIONS:
                return "Invalid options provided";
            case LLMErrorCode::PROMPT_INJECTION:
                return "Input rejected: potentially unsafe content detected";
            case LLMErrorCode::MODEL_NOT_FOUND:
                return "Requested model not found";
            case LLMErrorCode::MODEL_LOAD_FAILED:
                return "Failed to load model";
            case LLMErrorCode::MODEL_UNLOAD_FAILED:
                return "Failed to unload model";
            case LLMErrorCode::MODEL_NOT_LOADED:
                return "Model is not loaded";
            case LLMErrorCode::LORA_NOT_FOUND:
                return "Requested LoRA adapter not found";
            case LLMErrorCode::LORA_LOAD_FAILED:
                return "Failed to load LoRA adapter";
            case LLMErrorCode::LORA_UNLOAD_FAILED:
                return "Failed to unload LoRA adapter";
            case LLMErrorCode::LORA_NOT_LOADED:
                return "LoRA adapter is not loaded";
            case LLMErrorCode::INFERENCE_FAILED:
                return "LLM inference failed";
            case LLMErrorCode::RAG_FAILED:
                return "RAG query failed";
            case LLMErrorCode::EMBEDDING_FAILED:
                return "Embedding generation failed";
            case LLMErrorCode::TIMEOUT:
                return "Operation timed out";
            case LLMErrorCode::INVALID_RESPONSE:
                return "LLM generated an invalid or structurally broken response";
            case LLMErrorCode::OUT_OF_MEMORY:
                return "Insufficient memory for operation";
            case LLMErrorCode::CACHE_FULL:
                return "Cache is full";
            case LLMErrorCode::INTERNAL_ERROR:
            case LLMErrorCode::UNKNOWN_ERROR:
            default:
                // Mask internal error details
                return "An internal error occurred. Please contact support.";
        }
    }
};

/**
 * @brief Input validation constants
 */
namespace ValidationLimits {
    // Maximum prompt length (approximately 32K tokens at 4 chars/token)
    constexpr size_t MAX_PROMPT_LENGTH = 128000;
    
    // Maximum collection name length
    constexpr size_t MAX_COLLECTION_NAME_LENGTH = 256;
    
    // Maximum model/LoRA ID length
    constexpr size_t MAX_ID_LENGTH = 128;
    
    // Maximum number of documents for RAG (top_k)
    constexpr int MAX_RAG_TOP_K = 100;
    
    // Minimum number of documents for RAG (top_k)
    constexpr int MIN_RAG_TOP_K = 1;
    
    // Default execution timeout in seconds
    constexpr int DEFAULT_TIMEOUT_SECONDS = 300; // 5 minutes
    
    // Maximum length for a natural-language query passed to translateNLToAQL()
    // (4 096 chars ≈ 1 024 tokens – more than enough for any realistic NL query)
    constexpr size_t MAX_NL_QUERY_LENGTH = 4096;
    
    // Maximum length for schema context passed to translateNLToAQL()
    // (32 768 chars ≈ 8 192 tokens)
    constexpr size_t MAX_SCHEMA_CONTEXT_LENGTH = 32768;
}

/**
 * @brief Runtime-configurable counterpart of the @c ValidationLimits constants.
 *
 * Inject an instance into @c LLMAQLHandler via
 * @c LLMAQLHandler::setValidationLimits() to tune all input-length and
 * query-count caps without recompilation. Fields default to the same values
 * as the corresponding @c ValidationLimits constexpr constants so that
 * existing deployments are unaffected until an explicit override is applied.
 */
struct ValidationLimitsConfig {
    /// Maximum prompt length (chars).  Default: ValidationLimits::MAX_PROMPT_LENGTH
    std::size_t max_prompt_length           = ValidationLimits::MAX_PROMPT_LENGTH;
    /// Maximum NL query length (chars).  Default: ValidationLimits::MAX_NL_QUERY_LENGTH
    std::size_t max_nl_query_length         = ValidationLimits::MAX_NL_QUERY_LENGTH;
    /// Maximum schema context length (chars).  Default: ValidationLimits::MAX_SCHEMA_CONTEXT_LENGTH
    std::size_t max_schema_context_length   = ValidationLimits::MAX_SCHEMA_CONTEXT_LENGTH;
    /// Maximum RAG top_k.  Default: ValidationLimits::MAX_RAG_TOP_K
    int         max_rag_top_k               = ValidationLimits::MAX_RAG_TOP_K;
    /// Minimum RAG top_k.  Default: ValidationLimits::MIN_RAG_TOP_K
    int         min_rag_top_k               = ValidationLimits::MIN_RAG_TOP_K;
    /// Default execution timeout (seconds).  Default: ValidationLimits::DEFAULT_TIMEOUT_SECONDS
    int         default_timeout_seconds     = ValidationLimits::DEFAULT_TIMEOUT_SECONDS;
};

/**
 * @brief Validation helper functions
 */
class LLMValidator {
public:
    /**
     * @brief Validate prompt length
     * @throws LLMException if validation fails
     */
    static void validatePrompt(const std::string& prompt) {
        if (prompt.empty()) {
            throw LLMException(LLMErrorCode::INVALID_PROMPT, "Prompt cannot be empty");
        }
        if (prompt.length() > ValidationLimits::MAX_PROMPT_LENGTH) {
            throw LLMException(LLMErrorCode::PROMPT_TOO_LONG,
                "Prompt length " + std::to_string(prompt.length()) +
                " exceeds maximum " + std::to_string(ValidationLimits::MAX_PROMPT_LENGTH));
        }
    }
    
    /**
     * @brief Validate model or LoRA ID format (alphanumeric + dash/underscore)
     * @throws LLMException if validation fails
     */
    static void validateId(const std::string& id, bool is_lora = false) {
        if (id.empty()) {
            return; // Empty IDs are allowed (use default)
        }
        
        if (id.length() > ValidationLimits::MAX_ID_LENGTH) {
            throw LLMException(
                is_lora ? LLMErrorCode::INVALID_LORA_ID : LLMErrorCode::INVALID_MODEL_ID,
                "ID length exceeds maximum " + std::to_string(ValidationLimits::MAX_ID_LENGTH)
            );
        }
        
        // Check for valid characters: alphanumeric, dash, underscore, dot
        for (char c : id) {
            if (!std::isalnum(c) && c != '-' && c != '_' && c != '.') {
                throw LLMException(
                    is_lora ? LLMErrorCode::INVALID_LORA_ID : LLMErrorCode::INVALID_MODEL_ID,
                    "ID contains invalid characters. Only alphanumeric, dash, underscore, and dot are allowed"
                );
            }
        }
    }
    
    /**
     * @brief Validate collection name
     * @throws LLMException if validation fails
     */
    static void validateCollection(const std::string& collection) {
        if (collection.empty()) {
            throw LLMException(LLMErrorCode::INVALID_COLLECTION, "Collection name cannot be empty");
        }
        if (collection.length() > ValidationLimits::MAX_COLLECTION_NAME_LENGTH) {
            throw LLMException(LLMErrorCode::INVALID_COLLECTION,
                "Collection name length exceeds maximum " + std::to_string(ValidationLimits::MAX_COLLECTION_NAME_LENGTH));
        }
    }
    
    /**
     * @brief Validate RAG top_k parameter
     * @throws LLMException if validation fails
     */
    static void validateTopK(int top_k) {
        if (top_k < ValidationLimits::MIN_RAG_TOP_K || top_k > ValidationLimits::MAX_RAG_TOP_K) {
            throw LLMException(LLMErrorCode::INVALID_OPTIONS,
                "top_k must be between " + std::to_string(ValidationLimits::MIN_RAG_TOP_K) +
                " and " + std::to_string(ValidationLimits::MAX_RAG_TOP_K));
        }
    }
};

} // namespace aql
} // namespace themis
