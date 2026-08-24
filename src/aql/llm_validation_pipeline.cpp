/**
 * @file llm_validation_pipeline.cpp
 * @brief Implementation of LLM validation pipeline with retry logic
 *
 * Orchestrates: NL → LLM → Parser → Retry → Validated AQL
 *
 * @author ThemisDB Team
 * @date 2026-06-18
 */

#include "aql/llm_validation_pipeline.h"
#include "aql/llm_metrics_collector.h"
#include "query/aql_parser_service.h"
#include "llm/llm_client.h"

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <chrono>
#include <utility>

namespace themis::aql {

/** @brief Implementation detail. */
class LLMValidationPipeline::Impl {
public:
    std::shared_ptr<query::AQLParserService> parser_service;
    std::shared_ptr<llm::LLMClient> llm_client;
    LLMValidationPipelineConfig config;
    
    FeedbackGenerator feedback_generator;
    RetryabilityCheck retryability_check;
    
    explicit Impl(
        std::shared_ptr<query::AQLParserService> parser,
        std::shared_ptr<llm::LLMClient> llm,
        const LLMValidationPipelineConfig& cfg)
        : parser_service(std::move(parser)),
          llm_client(std::move(llm)),
          config(cfg),
          feedback_generator(defaultFeedbackGenerator()),
          retryability_check(defaultRetryabilityCheck()) {}
    
    /// Default feedback generator: convert parser diagnostics to LLM prompt
    static FeedbackGenerator defaultFeedbackGenerator() {
        return [](const query::ParserDiagnostics& diag) -> std::string {
            std::string feedback = "Fix the following error in the AQL query:\n";
            feedback += "Line " + std::to_string(diag.line_number) + ": " + diag.error_message;
            
            if (!diag.suggestions.empty()) {
                feedback += "\n\nSuggestion: " + diag.suggestions[0];
            }
            
            if (!diag.error_context.empty()) {
                feedback += "\n\nContext:\n" + diag.error_context;
            }
            
            return feedback;
        };
    }
    
    /// Default retryability check: retry on syntax errors, not access violations
    static RetryabilityCheck defaultRetryabilityCheck() {
        return [](const query::ParserDiagnostics& diag) -> bool {
            // Retryable error categories
            const std::string& category = diag.error_category;
            
            // Do NOT retry these:
            if (category == "ACCESS_DENIED" || 
                category == "PERMISSION_ERROR" ||
                category == "COLLECTION_NOT_FOUND") {
                return false;
            }
            
            // Retry these:
            if (category == "SYNTAX_ERROR" ||
                category == "UNKNOWN_KEYWORD" ||
                category == "MISSING_CLAUSE" ||
                category == "MALFORMED_FUNCTION") {
                return true;
            }
            
            // Default: don't retry unknown error types
            return false;
        };
    }
};

// ============================================================================
// LLMValidationPipeline Implementation
// ============================================================================

LLMValidationPipeline::LLMValidationPipeline(
    std::shared_ptr<query::AQLParserService> parser_service,
    std::shared_ptr<llm::LLMClient> llm_client,
    const LLMValidationPipelineConfig& config)
{
    if (!parser_service) {
        throw std::invalid_argument("parser_service cannot be null");
    }
    if (!llm_client) {
        throw std::invalid_argument("llm_client cannot be null");
    }
    
    impl_ = std::make_unique<Impl>(parser_service, llm_client, config);
    
    spdlog::info("LLMValidationPipeline initialized: max_retries={}, timeout={}ms",
                 config.max_retries, config.timeout_ms);
}

LLMValidationPipeline::~LLMValidationPipeline() = default;

LLMValidationResult LLMValidationPipeline::execute(
    const std::string& nl_query,
    const std::string& schema_context)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    LLMValidationResult result;

    if (!impl_->llm_client || !impl_->llm_client->isReady()) {
        result.status = LLMValidationStatus::LLM_GENERATION_FAILED;
        result.error_message = "LLM client unavailable or not ready";
        result.attempts_made = 0;
        LLMMetricsCollector::instance().recordAQLGenerationAttempt(
            false, 0, std::chrono::milliseconds(0), "client_unavailable");
        spdlog::error("LLMValidationPipeline: client unavailable or not ready");
        return result;
    }
    
    spdlog::debug("LLMValidationPipeline::execute: nl_query='{}'", nl_query);
    std::string retry_feedback;
    
    // Loop: LLM generation + validation + retry
    for (size_t attempt = 0; attempt <= impl_->config.max_retries; ++attempt) {
        // Check timeout
        auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        
        if (elapsed_ms > static_cast<int64_t>(impl_->config.timeout_ms)) {
            result.status = LLMValidationStatus::EXHAUSTED_RETRIES;
            result.error_message = "Validation pipeline timeout";
            result.attempts_made = attempt;
            
            spdlog::warn("LLMValidationPipeline timeout after {} ms", elapsed_ms);
            return result;
        }
        
        // Generate AQL via LLM
        std::string generated_aql;
        try {
            generated_aql = generateAQL(nl_query, schema_context, retry_feedback);
            
            if (generated_aql.empty()) {
                result.status = LLMValidationStatus::LLM_GENERATION_FAILED;
                result.error_message = "LLM generated empty response";
                result.attempts_made = attempt + 1;
                LLMMetricsCollector::instance().recordAQLGenerationAttempt(
                    false, static_cast<int>(attempt + 1), std::chrono::milliseconds(0), "empty_response");
                
                spdlog::error("LLM generated empty response");
                return result;
            }
            
            spdlog::debug("LLM generated AQL (attempt {}): '{}'", attempt + 1, generated_aql);
            
        } catch (const std::exception& e) {
            result.status = LLMValidationStatus::LLM_GENERATION_FAILED;
            result.error_message = std::string("LLM generation failed: ") + e.what();
            result.attempts_made = attempt + 1;
            LLMMetricsCollector::instance().recordAQLGenerationAttempt(
                false, static_cast<int>(attempt + 1), std::chrono::milliseconds(0), "generation_exception");
            
            spdlog::error("LLM generation exception: {}", e.what());
            return result;
        }
        
        // Validate AQL via parser
        auto parse_result = impl_->parser_service->parse(generated_aql);
        
        if (parse_result.success) {
            // SUCCESS: Parser accepted the AQL
            result.status = LLMValidationStatus::SUCCESS;
            result.validated_aql = generated_aql;
            result.attempts_made = attempt + 1;
            LLMMetricsCollector::instance().recordAQLGenerationAttempt(
                true, static_cast<int>(attempt + 1), std::chrono::milliseconds(0), "success");
            
            spdlog::info("AQL validation succeeded (attempt {}/{})",
                        attempt + 1, impl_->config.max_retries + 1);
            LLMMetricsCollector::instance().recordAQLValidation(
                true, std::chrono::milliseconds(0), "");
            
            return result;
        }
        
        // Parse failed
        result.parser_diagnostics = parse_result.diagnostics;
        result.attempts_made = attempt + 1;
        
        spdlog::warn("AQL parse failed (attempt {}/{}): {}",
                    attempt + 1, impl_->config.max_retries + 1,
                    parse_result.diagnostics.error_message);

        // Reject mode is a hard fail-fast contract: do not enter retry logic
        // even when retries are configured and the error is retryable.
        if (impl_->config.reject_on_error) {
            result.status = LLMValidationStatus::REJECTED;
            result.error_message = parse_result.diagnostics.error_message;
            LLMMetricsCollector::instance().recordAQLGenerationAttempt(
                false, static_cast<int>(attempt + 1), std::chrono::milliseconds(0), "reject_on_error");
            LLMMetricsCollector::instance().recordAQLValidation(
                false, std::chrono::milliseconds(0), "parse_error");

            spdlog::error("AQL validation rejected: {} (reject_on_error=true)",
                         parse_result.diagnostics.error_message);
            return result;
        }
        
        // Decision: retry or reject?
        bool can_retry = (attempt < impl_->config.max_retries);
        bool should_retry = can_retry && shouldRetry(parse_result.diagnostics);
        
        if (!can_retry || !should_retry) {
            // No more retries or error is not retryable
            result.status = LLMValidationStatus::EXHAUSTED_RETRIES;
            
            result.error_message = parse_result.diagnostics.error_message;
            LLMMetricsCollector::instance().recordAQLGenerationAttempt(
                false, static_cast<int>(attempt + 1), std::chrono::milliseconds(0), "exhausted_retries");
            LLMMetricsCollector::instance().recordAQLValidation(
                false, std::chrono::milliseconds(0), "parse_error");
            
            spdlog::error("AQL validation rejected: {} (retries_available={}, should_retry={})",
                         parse_result.diagnostics.error_message, can_retry, should_retry);
            return result;
        }
        
        // Format feedback for LLM retry
        retry_feedback = formatRetryFeedback(parse_result.diagnostics);
        result.retry_feedback = retry_feedback;
        
        spdlog::info("Retrying LLM with feedback: {}", result.retry_feedback);
        LLMMetricsCollector::instance().recordValidationRetry(false, static_cast<int>(attempt + 1));
    }
    
    // Should not reach here (loop exits early on success/failure)
    result.status = LLMValidationStatus::EXHAUSTED_RETRIES;
    result.error_message = "Exhausted all retry attempts";
    result.attempts_made = impl_->config.max_retries + 1;
    
    return result;
}

std::string LLMValidationPipeline::generateAQL(
    const std::string& nl_query,
    const std::string& schema_context,
    const std::string& retry_feedback)
{
    // Phase 0.4: Invoke real LLM client via generateAQL()
    llm::GenerationOptions options;
    options.max_tokens = 512;
    options.temperature = 0.5f;  // Deterministic for AQL generation
    options.timeout_ms = 8000;
    
    std::string effective_query = nl_query;
    if (!retry_feedback.empty()) {
        effective_query += "\n\nPrevious parser validation error:\n";
        effective_query += retry_feedback;
        effective_query += "\n\nRegenerate a corrected AQL query that fixes this error.";
    }

    auto result = impl_->llm_client->generateAQL(effective_query, schema_context, options);
    
    if (!result.success) {
        throw std::runtime_error("LLM generation failed: " + result.error_message);
    }
    
    return result.text;
}

std::string LLMValidationPipeline::formatRetryFeedback(
    const query::ParserDiagnostics& diagnostics) const
{
    return impl_->feedback_generator(diagnostics);
}

bool LLMValidationPipeline::shouldRetry(
    const query::ParserDiagnostics& diagnostics) const
{
    return impl_->retryability_check(diagnostics);
}

void LLMValidationPipeline::setFeedbackGenerator(const FeedbackGenerator& gen) {
    if (gen) {
        impl_->feedback_generator = gen;
        spdlog::debug("Custom feedback generator installed");
    }
}

void LLMValidationPipeline::setRetryabilityCheck(const RetryabilityCheck& check) {
    if (check) {
        impl_->retryability_check = check;
        spdlog::debug("Custom retryability check installed");
    }
}

const LLMValidationPipelineConfig& LLMValidationPipeline::config() const {
    return impl_->config;
}

void LLMValidationPipeline::setConfig(const LLMValidationPipelineConfig& config) {
    impl_->config = config;
    spdlog::info("Pipeline config updated: max_retries={}, timeout={}ms",
                 config.max_retries, config.timeout_ms);
}

// ============================================================================
// LLMValidationPipelineFactory Implementation
// ============================================================================

std::shared_ptr<LLMValidationPipeline> LLMValidationPipelineFactory::create(
    std::shared_ptr<query::AQLParserService> parser_service,
    std::shared_ptr<llm::LLMClient> llm_client)
{
    return std::make_shared<LLMValidationPipeline>(
        parser_service,
        llm_client,
        LLMValidationPipelineConfig{}
    );
}

std::shared_ptr<LLMValidationPipeline> LLMValidationPipelineFactory::createWithConfig(
    std::shared_ptr<query::AQLParserService> parser_service,
    std::shared_ptr<llm::LLMClient> llm_client,
    const LLMValidationPipelineConfig& config)
{
    return std::make_shared<LLMValidationPipeline>(
        parser_service,
        llm_client,
        config
    );
}

} // namespace themis::aql
