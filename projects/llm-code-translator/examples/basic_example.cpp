/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            basic_example.cpp                                  ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:45:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     123                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "vllm_client.h"
#include "execution_plan.h"
#include "prompt_to_plan.h"
#include <iostream>
#include <iomanip>

using namespace themis::llm_translator;

int main() {
    std::cout << "=== LLM Code Translator - Basic Example ===\n\n";
    
    // 1. Configure vLLM client
    VLLMConfig vllm_config;
    vllm_config.base_url = "http://localhost:8000";  // vLLM server
    vllm_config.model_name = "codegen-16B";
    vllm_config.temperature = 0.7;
    vllm_config.max_tokens = 1024;
    
    auto vllm_client = std::make_shared<VLLMClient>(vllm_config);
    
    // 2. Health check
    std::cout << "Checking vLLM server health... ";
    if (vllm_client->healthCheck()) {
        std::cout << "✓ Server is healthy\n\n";
    } else {
        std::cout << "✗ Server is not reachable\n";
        std::cout << "Please ensure vLLM is running on " << vllm_config.base_url << "\n";
        return 1;
    }
    
    // 3. Create translator
    PromptToPlanConfig translator_config;
    translator_config.use_few_shot_examples = true;
    translator_config.validate_plans = true;
    
    PromptToPlanTranslator translator(vllm_client, translator_config);
    
    // 4. Example queries
    std::vector<std::string> queries = {
        "Find all users who registered in the last 7 days",
        "Show average temperature per sensor for readings above 50°C in last 24 hours",
        "Count total orders grouped by customer city"
    };
    
    for (const auto& query : queries) {
        std::cout << "Query: " << query << "\n";
        std::cout << std::string(60, '-') << "\n";
        
        try {
            // Translate prompt to execution plan
            auto plan = translator.translate(query);
            
            // Display plan
            auto plan_json = plan.toJson();
            std::cout << "Execution Plan:\n";
            std::cout << std::setw(2) << plan_json << "\n\n";
            
            // Display metadata
            std::cout << "Metadata:\n";
            std::cout << "  - Model: " << plan.llm_model_used << "\n";
            std::cout << "  - Generation Time: " << plan.generation_time_ms << " ms\n";
            std::cout << "  - Confidence: " << (plan.confidence_score * 100) << "%\n";
            std::cout << "  - Valid: " << (plan.validate() ? "Yes" : "No") << "\n";
            
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
        
        std::cout << "\n";
    }
    
    // 5. Multi-sample generation example (AlphaCode-inspired)
    std::cout << "\n=== Multi-Sample Generation ===\n\n";
    
    translator_config.use_multi_sample = true;
    translator_config.num_samples = 5;
    translator.setConfig(translator_config);
    
    std::string complex_query = "Find top 10 customers by total order value in the last month";
    
    std::cout << "Query: " << complex_query << "\n";
    std::cout << std::string(60, '-') << "\n";
    
    try {
        auto plans = translator.translateMultiple(complex_query);
        
        std::cout << "Generated " << plans.size() << " plan candidates:\n\n";
        
        for (size_t i = 0; i < plans.size(); i++) {
            std::cout << "Candidate " << (i + 1) << ":\n";
            std::cout << std::setw(2) << plans[i].toJson() << "\n\n";
        }
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    
    std::cout << "\n=== Example Complete ===\n";
    
    return 0;
}
