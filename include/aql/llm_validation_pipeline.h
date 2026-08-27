/**
 * @file llm_validation_pipeline.h
 * @brief LLM → Parser → Retry validation pipeline for AQL generation
 *
 * Encapsulates the complete NL-to-validated-AQL workflow with automatic retry logic.
 *
 * WORKFLOW:
 * 1. NL query arrives → LLM generates AQL
 * 2. AQL sent to parser for validation
 * 3. If parser succeeds: return validated AQL
 * 4. If parser fails: format error as feedback → retry LLM with corrective prompt
 * 5. Repeat until: success OR max_retries exhausted
 *
 * @author ThemisDB Team
 * @date 2026-06-18
 */

#pragma once

#include "query/aql_parser_service.h"

#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace themis::llm {
class LLMClient;
}

namespace themis::aql {

/**
 * @brief Status of LLM validation pipeline execution
 */
enum class LLMValidationStatus {
    /// AQL passed parser validation and is ready for execution
    SUCCESS = 0,
    
    /// Parser detected syntax error; will retry if retries remaining
    PARSE_ERROR = 1,
    
    /// Validation error detected and retry is possible
    RETRYABLE = 2,
    
    /// All retries exhausted; AQL failed all validation attempts
    EXHAUSTED_RETRIES = 3,
    
    /// Validation mode is REJECT_ON_ERROR; failed on first parse
    REJECTED = 4,
    
    /// LLM generation failed (timeout, error, etc.)
    LLM_GENERATION_FAILED = 5,
};

/**
 * @brief Result of LLM validation pipeline
 *
 * Contains validated AQL on success, or diagnostic/feedback info on failure.
 */
struct LLMValidationResult {
    /// Outcome of validation pipeline
    LLMValidationStatus status = LLMValidationStatus::SUCCESS;
    
    /// The validated AQL query (only populated if status == SUCCESS)
    std::string validated_aql;
    
    /// Diagnostic info from parser (populated if status == PARSE_ERROR or similar)
    query::ParserDiagnostics parser_diagnostics;
    
    /// Feedback message for LLM on retry (used internally)
    std::string retry_feedback;
    
    /// Number of retry attempts made
    size_t attempts_made = 0;
    
    /// Human-readable error message
    std::string error_message;
};

/**
 * @brief Configuration for validation pipeline behavior
 */
struct LLMValidationPipelineConfig {
    /// Maximum number of LLM retries on parse failure (0 = no retries)
    size_t max_retries = 1;
    
    /// Maximum total time for pipeline (all retries included)
    uint32_t timeout_ms = 5000;
    
    /// If true, validation failures are errors; if false, warn only
    bool reject_on_error = true;
    
    /// Log level for validation events (debug/info/warn/error)
    std::string log_level = "info";
};

/**
 * @brief Function signature for feedback generator
 *
 * Converts parser diagnostics into prompt feedback for LLM retry.
 *
 * @param diagnostics Parser error details
 * @return Feedback string to include in retry prompt
 */
using FeedbackGenerator = std::function<std::string(
    const query::ParserDiagnostics& diagnostics
)>;

/**
 * @brief Function signature for retryability check
 *
 * Determines if a particular parse error should trigger a retry.
 *
 * @param diagnostics Parser error details
 * @return true if error is retryable, false if should reject immediately
 */
using RetryabilityCheck = std::function<bool(
    const query::ParserDiagnostics& diagnostics
)>;

/**
 * @brief LLM AQL validation pipeline with automatic retry
 *
 * Orchestrates NL → LLM → Parser → Retry loop with intelligent feedback.
 *
 * SEPARATION OF CONCERNS:
 * - Parser (src/query/): Validates syntax
 * - LLM client: Generates AQL
 * - Pipeline: Coordinates + retry logic
 *
 * DEPENDENCY INJECTION:
 * - Parser service injected via constructor (can be mocked for testing)
 * - LLM client injected via constructor
 * - Strategies (feedback, retryability) customizable
 */
class LLMValidationPipeline {
public:
    /// @brief Create validation pipeline with dependencies
    ///
    /// @param parser_service Parser service for validation (from src/query/)
    /// @param llm_client LLM inference client
    /// @param config Configuration options
    /// @throws std::invalid_argument if parser_service or llm_client is null
    LLMValidationPipeline(
        std::shared_ptr<query::AQLParserService> parser_service,
        std::shared_ptr<llm::LLMClient> llm_client,
        const LLMValidationPipelineConfig& config = {}
    );
    
    ~LLMValidationPipeline();
    
    /**
     * @brief Execute full NL-to-validated-AQL pipeline
     *
     * Flow:
     * 1. Generate AQL via LLM
     * 2. Validate via parser
     * 3. On error: format feedback → retry LLM
     * 4. Return result (success or exhausted retries)
     *
     * @param nl_query Natural language query from user
     * @param schema_context Collection/field constraints (for LLM context)
     * @return LLMValidationResult with status + validated AQL or error details
     *
     * @note Thread-safe; can be called concurrently
     */
    LLMValidationResult execute(
        const std::string& nl_query,
        const std::string& schema_context
    );
    
    /// @brief Set custom feedback generator strategy
    /// Default: generates contextual error messages
    void setFeedbackGenerator(const FeedbackGenerator& gen);
    
    /// @brief Set custom retryability check strategy
    /// Default: retries on syntax errors, not on access violations
    void setRetryabilityCheck(const RetryabilityCheck& check);
    
    /// @brief Get pipeline configuration
    const LLMValidationPipelineConfig& config() const;
    
    /// @brief Update pipeline configuration
    void setConfig(const LLMValidationPipelineConfig& config);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    // Private methods for internal workflow
    std::string generateAQL(const std::string& nl_query,
                            const std::string& schema_context,
                            const std::string& retry_feedback = "");
    
    std::string formatRetryFeedback(const query::ParserDiagnostics& diagnostics) const;
    
    bool shouldRetry(const query::ParserDiagnostics& diagnostics) const;
};

/**
 * @brief Factory for creating validation pipeline instances
 *
 * Handles dependency injection and configuration.
 */
class LLMValidationPipelineFactory {
public:
    /// @brief Create pipeline with default configuration
    static std::shared_ptr<LLMValidationPipeline> create(
        std::shared_ptr<query::AQLParserService> parser_service,
        std::shared_ptr<llm::LLMClient> llm_client
    );
    
    /// @brief Create pipeline with custom configuration
    static std::shared_ptr<LLMValidationPipeline> createWithConfig(
        std::shared_ptr<query::AQLParserService> parser_service,
        std::shared_ptr<llm::LLMClient> llm_client,
        const LLMValidationPipelineConfig& config
    );
};

} // namespace themis::aql
