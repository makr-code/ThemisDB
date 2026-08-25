/**
 * @file llm_process_analyzer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
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
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {

/**
 * @brief Sanitize (mask) an LLM API key for safe logging and display.
 *
 * Replaces the middle portion of the key with asterisks so that the raw
 * credential never appears in log output or error messages.  At most 4
 * characters at the start and 4 at the end are shown; keys shorter than
 * 9 characters are fully masked.
 *
 * Example: "sk-abcdefghij1234567890xyz" → "sk-a***...***0xyz"
 *
 * @param api_key  The raw API key string (may be empty).
 * @return         A masked representation safe for logging.
 */
std::string sanitizeApiKey(const std::string& api_key);

/**
 * @brief LLM Integration Layer for Process Mining
 * 
 * Provides unified interface for LLM-assisted process analysis:
 * - Process conformance checking
 * - Next activity prediction
 * - Compliance verification (5R Rule, Vier-Augen-Prinzip, etc.)
 * - Fraud detection
 * - Sentiment analysis
 * - Process optimization recommendations
 */

enum class TaskType {
    ANALYZE_PROCESS,      // General process analysis & conformance
    PROCESS_CONFORMANCE = ANALYZE_PROCESS, // Backward-compatible alias
    PREDICT_NEXT,         // Next activity prediction
    VERIFY_5R_RULE,       // Healthcare 5R Rule verification
    DETECT_FRAUD,         // Financial anomaly/fraud detection
    CLASSIFY_INCIDENT,    // IT incident categorization
    SENTIMENT_ANALYSIS,   // Customer service sentiment
    OPTIMIZE_PROCESS,     // Process optimization recommendations
    COMPLIANCE_CHECK      // Regulatory compliance verification
};

enum class LLMProvider {
    OPENAI,               // OpenAI GPT-4, GPT-3.5
    ANTHROPIC,            // Claude 3 Opus, Sonnet
    LOCAL,                // Local models (llama.cpp, ollama)
    AZURE_OPENAI          // Azure OpenAI Service
};

struct LLMConfig {
    LLMProvider provider = LLMProvider::OPENAI;
    std::string api_key;
    std::string model_name = "gpt-4";
    std::string base_url;  // For custom endpoints
    
    // Retry configuration
    int max_retries = 3;
    int retry_delay_ms = 1000;
    
    // Performance
    bool enable_caching = true;
    int cache_ttl_seconds = 3600;
    int max_cache_entries = 1000;  // Maximum LRU cache entries (0 = use default 1000)
    
    // Limits
    int max_tokens = 2000;
    double temperature = 0.3;  // Lower = more deterministic

    /// Optional path to an operator-configurable injection-prefix file.
    /// Each non-empty, non-comment line (lines beginning with '#' are skipped)
    /// is interpreted as a lower-case prompt-injection prefix that
    /// sanitizeUserContent() will redact from any user-supplied data before
    /// it is embedded in an LLM prompt.
    /// When empty (the default) or when the file cannot be opened, the
    /// built-in 13-pattern prefix list is used as the fallback.
    std::string injection_prefix_config_path;
};

struct LLMRequest {
    TaskType task_type;
    std::string domain;  // "administrative", "healthcare", "financial", etc.
    
    // Process data
    nlohmann::json process_trace;
    nlohmann::json ideal_model;
    nlohmann::json context;  // Additional context
    
    // Task-specific parameters
    std::map<std::string, std::string> parameters;

    // Backward-compatible field used by older tests.
    nlohmann::json process_data;
};

struct LLMResponse {
    bool success = false;
    std::string error_message;
    // Backward-compatible summary text used by older tests.
    std::string summary;
    
    // Core metrics
    double conformance_score = 0.0;  // 0.0 - 1.0
    
    // Deviations
    struct Deviation {
        std::string activity;
        std::string type;  // "missing", "extra", "wrong_order", "wrong_resource"
        std::string severity;  // "critical", "major", "minor"
        std::string description;
    };
    std::vector<Deviation> deviations;
    
    // Compliance issues
    struct ComplianceIssue {
        std::string rule;  // e.g., "Vier-Augen-Prinzip", "5R Rule"
        std::string violation;
        std::string severity;
        std::string remediation;
    };
    std::vector<ComplianceIssue> compliance_issues;
    
    // Recommendations
    struct Recommendation {
        std::string type;  // "optimization", "compliance", "performance"
        std::string priority;  // "critical", "high", "medium", "low"
        std::string description;
        double potential_improvement = 0.0;  // Estimated impact
    };
    std::vector<Recommendation> recommendations;
    
    // Predictions (for PREDICT_NEXT task)
    struct Prediction {
        std::string activity;
        double probability = 0.0;
        std::string reasoning;
    };
    std::vector<Prediction> predictions;
    
    // Healthcare 5R Rule (for VERIFY_5R_RULE task)
    struct FiveRCheck {
        bool right_patient = false;
        bool right_medication = false;
        bool right_dose = false;
        bool right_time = false;
        bool right_route = false;
        bool overall_compliance = false;
        std::string risk_level;  // "low", "medium", "high", "critical"
        std::vector<std::string> corrective_actions;
    };
    std::optional<FiveRCheck> five_rights_check;
    
    // Fraud detection (for DETECT_FRAUD task)
    struct FraudAnalysis {
        double risk_score = 0.0;  // 0.0 - 1.0
        std::vector<std::string> detected_anomalies;
        struct Flags {
            bool duplicate = false;
            bool unusual_amount = false;
            bool vendor_not_verified = false;
            bool missing_documentation = false;
        } flags;
        std::string recommended_action;
    };
    std::optional<FraudAnalysis> fraud_analysis;
    
    // Full response JSON (for advanced parsing)
    nlohmann::json raw_response;
    
    // Metadata
    int64_t response_time_ms = 0;
    int tokens_used = 0;
    bool from_cache = false;
};

/** @brief Llm process analyzer component. */
class LLMProcessAnalyzer {
public:
    explicit LLMProcessAnalyzer(const LLMConfig& config);
    ~LLMProcessAnalyzer();
    
    /**
     * @brief Analyze process with LLM
     * 
     * Main entry point for LLM-assisted analysis.
     * 
     * @param request The analysis request
     * @return Status and LLMResponse with analysis results
     */
    std::pair<bool, LLMResponse> analyze(const LLMRequest& request);
    
    /**
     * @brief Generate prompt for specific task
     * 
     * Creates task-specific prompts with proper formatting.
     * 
     * @param task_type Type of analysis task
     * @param data Input data (trace, model, etc.)
     * @param domain Domain context
     * @return Formatted prompt string
     */
    std::string generatePrompt(
        TaskType task_type,
        const nlohmann::json& data,
        const std::string& domain
    ) const;
    
    /**
     * @brief Validate LLM response against schema
     * 
     * Ensures response conforms to expected structure.
     * 
     * @param response Raw LLM response
     * @param task_type Expected task type
     * @return true if valid, false otherwise
     */
    bool validateResponse(
        const nlohmann::json& response,
        TaskType task_type
    ) const;
    
    /**
     * @brief Get cache statistics
     */
    struct CacheStats {
        size_t hits = 0;
        size_t misses = 0;
        size_t evictions = 0;
        double hit_rate() const {
            return (hits + misses) > 0 ? static_cast<double>(hits) / (hits + misses) : 0.0;
        }
    };
    CacheStats getCacheStats() const;
    
    /**
     * @brief Clear response cache
     */
    void clearCache();
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    
    // Internal helpers
    std::string callLLM(const std::string& prompt, const std::map<std::string, std::string>& params);
    nlohmann::json parseResponse(const std::string& raw_response, [[maybe_unused]] TaskType task_type);
    std::string getCacheKey(const LLMRequest& request) const;
};

} // namespace themis
