/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_to_plan.h                                   ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:48:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     115                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "vllm_client.h"
#include "execution_plan.h"
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace llm_translator {

/**
 * @brief Configuration for prompt-to-plan translation
 */
struct PromptToPlanConfig {
    bool use_few_shot_examples = true;      // Include examples in prompt
    bool use_schema_context = true;         // Include database schema
    int max_plan_generation_retries = 3;    // Retry on parse errors
    bool validate_plans = true;             // Validate generated plans
    
    // Multi-sample configuration (for neural approaches)
    bool use_multi_sample = false;          // Generate multiple candidates
    int num_samples = 10;                   // Number of candidates
    float sample_temperature = 0.8;         // Temperature for sampling
};

/**
 * @brief Translates natural language prompts to execution plans using LLM
 * 
 * This is the core component that takes user prompts and generates
 * platform-independent execution plans via vLLM.
 */
class PromptToPlanTranslator {
public:
    explicit PromptToPlanTranslator(
        std::shared_ptr<VLLMClient> vllm_client,
        const PromptToPlanConfig& config = PromptToPlanConfig()
    );
    
    /**
     * @brief Translate a natural language prompt to an execution plan
     * @param prompt Natural language description of desired operation
     * @param schema_context Optional database schema context
     * @return Execution plan
     */
    ExecutionPlan translate(
        const std::string& prompt,
        const std::string& schema_context = ""
    );
    
    /**
     * @brief Generate multiple execution plan candidates (for neural approaches)
     * @param prompt Natural language description
     * @param schema_context Optional database schema context
     * @return Vector of execution plan candidates
     */
    std::vector<ExecutionPlan> translateMultiple(
        const std::string& prompt,
        const std::string& schema_context = ""
    );
    
    /**
     * @brief Set configuration
     */
    void setConfig(const PromptToPlanConfig& config) { config_ = config; }
    
    /**
     * @brief Get configuration
     */
    const PromptToPlanConfig& getConfig() const { return config_; }
    
private:
    std::shared_ptr<VLLMClient> vllm_client_;
    PromptToPlanConfig config_;
    
    // Build the full prompt with examples and context
    std::string buildPrompt(
        const std::string& user_prompt,
        const std::string& schema_context
    );
    
    // Parse LLM output to execution plan
    ExecutionPlan parseLLMOutput(
        const std::string& llm_output,
        const std::string& original_prompt
    );
    
    // Get few-shot examples for prompt
    std::string getFewShotExamples();
};

} // namespace llm_translator
} // namespace themis
