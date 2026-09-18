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

enum class LLMValidationStatus {
    SUCCESS = 0,
    
    PARSE_ERROR = 1,
    
    RETRYABLE = 2,
    
    EXHAUSTED_RETRIES = 3,
    
    REJECTED = 4,
    
    LLM_GENERATION_FAILED = 5,
};

struct LLMValidationResult {
    LLMValidationStatus status = LLMValidationStatus::SUCCESS;
    
    std::string validated_aql;
    
    query::ParserDiagnostics parser_diagnostics;
    
    std::string retry_feedback;
    
    size_t attempts_made = 0;
    
    std::string error_message;
};

struct LLMValidationPipelineConfig {
    size_t max_retries = 1;
    
    uint32_t timeout_ms = 5000;
    
    bool reject_on_error = true;
    
    std::string log_level = "info";
};

using FeedbackGenerator = std::function<std::string(
    const query::ParserDiagnostics& diagnostics
)>;

using RetryabilityCheck = std::function<bool(
    const query::ParserDiagnostics& diagnostics
)>;

class LLMValidationPipeline {
public:
    LLMValidationPipeline(
        std::shared_ptr<query::AQLParserService> parser_service,
        std::shared_ptr<llm::LLMClient> llm_client,
        const LLMValidationPipelineConfig& config = {}
    );
    
    ~LLMValidationPipeline();
    
    /**
     * @brief Execute.
     * @param[in] nl_query Input parameter.
     * @param[in] schema_context Input parameter.
     * @return Return value.
     */
    LLMValidationResult execute(
        const std::string& nl_query,
        const std::string& schema_context
    );
    
    /**
     * @brief Set Feedback Generator.
     * @param[in] gen Input parameter.
     */
    void setFeedbackGenerator(const FeedbackGenerator& gen);
    
    /**
     * @brief Set Retryability Check.
     * @param[in] check Input parameter.
     */
    void setRetryabilityCheck(const RetryabilityCheck& check);
    
    /**
     * @brief Config.
     * @return Return value.
     */
    const LLMValidationPipelineConfig& config() const;
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     */
    void setConfig(const LLMValidationPipelineConfig& config);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    // Private methods for internal workflow
    std::string generateAQL(const std::string& nl_query,
                            const std::string& schema_context,
                            const std::string& retry_feedback = "");
    
    /**
     * @brief Format Retry Feedback.
     * @param[in] diagnostics Input parameter.
     * @return Return value.
     */
    std::string formatRetryFeedback(const query::ParserDiagnostics& diagnostics) const;
    
    /**
     * @brief Should Retry.
     * @param[in] diagnostics Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldRetry(const query::ParserDiagnostics& diagnostics) const;
};

class LLMValidationPipelineFactory {
public:
    /**
     * @brief Create.
     * @param[in] parser_service Input parameter.
     * @param[in] llm_client Input parameter.
     * @return Return value.
     */
    static std::shared_ptr<LLMValidationPipeline> create(
        std::shared_ptr<query::AQLParserService> parser_service,
        std::shared_ptr<llm::LLMClient> llm_client
    );
    
    /**
     * @brief Create With Config.
     * @param[in] parser_service Input parameter.
     * @param[in] llm_client Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    static std::shared_ptr<LLMValidationPipeline> createWithConfig(
        std::shared_ptr<query::AQLParserService> parser_service,
        std::shared_ptr<llm::LLMClient> llm_client,
        const LLMValidationPipelineConfig& config
    );
};

} // namespace themis::aql
