/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_to_plan.cpp                                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:48:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     245                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "prompt_to_plan.h"
#include <stdexcept>
#include <sstream>
#include <regex>

namespace themis {
namespace llm_translator {

PromptToPlanTranslator::PromptToPlanTranslator(
    std::shared_ptr<VLLMClient> vllm_client,
    const PromptToPlanConfig& config
)
    : vllm_client_(vllm_client)
    , config_(config)
{
    if (!vllm_client_) {
        throw std::invalid_argument("VLLMClient cannot be null");
    }
}

std::string PromptToPlanTranslator::getFewShotExamples() {
    return R"(
# Example 1:
User: Find all users who registered in the last 7 days
Plan:
{
  "operation": 0,
  "datasource": "users",
  "filters": [
    {"field": "registration_date", "op": ">=", "value": "-7d"}
  ],
  "fields": ["*"]
}

# Example 2:
User: Show average temperature per sensor for readings above 50°C in last 24 hours
Plan:
{
  "operation": 1,
  "datasource": "sensor_readings",
  "filters": [
    {"field": "timestamp", "op": ">=", "value": "-24h"},
    {"field": "temperature", "op": ">", "value": 50}
  ],
  "group_by": ["sensor_id"],
  "aggregations": [
    {"function": "AVG", "field": "temperature", "alias": "avg_temperature"}
  ]
}

# Example 3:
User: Count total orders grouped by customer city
Plan:
{
  "operation": 1,
  "datasource": "orders",
  "group_by": ["customer_city"],
  "aggregations": [
    {"function": "COUNT", "field": "*", "alias": "total_orders"}
  ]
}
)";
}

std::string PromptToPlanTranslator::buildPrompt(
    const std::string& user_prompt,
    const std::string& schema_context
) {
    std::ostringstream prompt = {};
    
    prompt << "You are an AI that translates natural language queries into execution plans.\n";
    prompt << "Generate a JSON execution plan for the given query.\n\n";
    
    prompt << "Operation types (use numeric value):\n";
    prompt << "0 = QUERY (data retrieval)\n";
    prompt << "1 = AGGREGATE (aggregations with GROUP BY)\n";
    prompt << "2 = TRANSFORM (data transformation)\n";
    prompt << "3 = JOIN (multi-source joining)\n";
    prompt << "4 = GRAPH_TRAVERSE (graph operations)\n";
    prompt << "5 = VECTOR_SEARCH (similarity search)\n";
    prompt << "6 = TIME_SERIES (time-series analysis)\n";
    prompt << "7 = MUTATION (insert/update/delete)\n\n";
    
    if (config_.use_schema_context && !schema_context.empty()) {
        prompt << "Database Schema:\n";
        prompt << schema_context << "\n\n";
    }
    
    if (config_.use_few_shot_examples) {
        prompt << "Examples:\n";
        prompt << getFewShotExamples() << "\n";
    }
    
    prompt << "Now generate the plan:\n";
    prompt << "User: " << user_prompt << "\n";
    prompt << "Plan:\n";
    
    return prompt.str();
}

ExecutionPlan PromptToPlanTranslator::parseLLMOutput(
    const std::string& llm_output,
    const std::string& original_prompt
) {
    // Extract JSON from output (may contain explanation text)
    std::regex json_regex(R"(\{[\s\S]*\})");
    std::smatch match = {};
    
    std::string json_str = llm_output;
    if (std::regex_search(llm_output, match, json_regex)) {
        json_str = match.str();
    }
    
    try {
        auto json = nlohmann::json::parse(json_str);
        auto plan = ExecutionPlan::fromJson(json);
        plan.original_prompt = original_prompt;
        return plan;
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse LLM output as execution plan: " + 
                                std::string(e.what()) + "\nOutput: " + llm_output);
    }
}

ExecutionPlan PromptToPlanTranslator::translate(
    const std::string& prompt,
    const std::string& schema_context
) {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::string full_prompt = buildPrompt(prompt, schema_context);
    
    int retries = 0;
    while (retries < config_.max_plan_generation_retries) {
        try {
            // Generate completion from vLLM
            auto response = vllm_client_->generate(full_prompt);
            
            if (!response.success) {
                throw std::runtime_error("vLLM generation failed: " + response.error_message);
            }
            
            // Parse to execution plan
            auto plan = parseLLMOutput(response.text, prompt);
            
            // Validate if enabled
            if (config_.validate_plans) {
                std::string error_msg = {};
                if (!plan.validate(&error_msg)) {
                    throw std::runtime_error("Plan validation failed: " + error_msg);
                }
            }
            
            // Add metadata
            auto end = std::chrono::high_resolution_clock::now();
            plan.generation_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                end - start
            ).count();
            plan.llm_model_used = vllm_client_->getConfig().model_name;
            plan.confidence_score = 1.0;  // Could be extracted from LLM if available
            
            return plan;
            
        } catch (const std::exception& e) {
            retries++;
            if (retries >= config_.max_plan_generation_retries) {
                throw std::runtime_error("Failed to generate execution plan after " + 
                                       std::to_string(retries) + " retries: " + e.what());
            }
        }
    }
    
    throw std::runtime_error("Failed to generate execution plan");
}

std::vector<ExecutionPlan> PromptToPlanTranslator::translateMultiple(
    const std::string& prompt,
    const std::string& schema_context
) {
    std::string full_prompt = buildPrompt(prompt, schema_context);
    
    // Generate multiple completions
    auto responses = vllm_client_->generateMultiple(
        full_prompt,
        config_.num_samples,
        config_.sample_temperature
    );
    
    std::vector<ExecutionPlan> plans;
    
    for (const auto& response : responses) {
        if (!response.success) {
            continue;  // Skip failed responses
        }
        
        try {
            auto plan = parseLLMOutput(response.text, prompt);
            
            // Validate if enabled
            if (config_.validate_plans) {
                std::string error_msg = {};
                if (!plan.validate(&error_msg)) {
                    continue;  // Skip invalid plans
                }
            }
            
            plan.llm_model_used = vllm_client_->getConfig().model_name;
            plan.generation_time_ms = static_cast<int64_t>(response.latency_ms);
            
            plans.push_back(plan);
            
        } catch (const std::exception&) {
            // Skip unparseable plans
            continue;
        }
    }
    
    return plans;
}

} // namespace llm_translator
} // namespace themis
