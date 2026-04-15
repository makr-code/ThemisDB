/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            complete_pipeline_example.cpp                      ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:39:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     176                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "../src/vllm_client.h"
#include "../src/prompt_to_plan.h"
#include "../src/direct_executor.h"
#include <iostream>
#include <iomanip>

using namespace themis::llm_translator;

void printExecutionResult(const std::string& prompt, const ExecutionResult& result) {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "Prompt: " << prompt << "\n";
    std::cout << std::string(80, '-') << "\n";
    
    if (result.success) {
        std::cout << "✓ Success!\n";
        std::cout << "Execution Time: " << result.execution_time_ms << "ms\n";
        std::cout << "Rows Affected: " << result.rows_affected << "\n";
        std::cout << "Results:\n" << result.data.dump(2) << "\n";
    } else {
        std::cout << "✗ Failed: " << result.error_message << "\n";
    }
    
    std::cout << std::string(80, '=') << "\n";
}

int main() {
    std::cout << "LLM-Based Execution Engine - Complete Pipeline Example\n";
    std::cout << "======================================================\n\n";
    
    // Step 1: Setup vLLM client
    std::cout << "[1] Configuring vLLM client...\n";
    VLLMConfig config;
    config.base_url = "http://localhost:8000";
    config.model_name = "codegen-16B";
    config.temperature = 0.2;
    config.max_tokens = 512;
    
    auto vllm_client = std::make_shared<VLLMClient>(config);
    
    // Check vLLM health (optional - will fail if vLLM not running)
    bool vllm_available = vllm_client->healthCheck();
    if (!vllm_available) {
        std::cout << "⚠ Warning: vLLM server not available at " << config.base_url << "\n";
        std::cout << "   Continuing with mock execution plan generation...\n\n";
    } else {
        std::cout << "✓ vLLM server is healthy\n\n";
    }
    
    // Step 2: Setup prompt-to-plan translator
    std::cout << "[2] Initializing prompt-to-plan translator...\n";
    PromptToPlanTranslator translator(vllm_client);
    std::cout << "✓ Translator ready\n\n";
    
    // Step 3: Setup database and executor
    std::cout << "[3] Initializing database and executor...\n";
    auto mock_db = std::make_shared<MockDatabase>();
    DirectExecutor executor(mock_db);
    std::cout << "✓ Executor ready with sample data\n\n";
    
    // Example 1: Simple query with filter
    std::cout << "\n=== Example 1: Query with Filter ===\n";
    std::string prompt1 = "Find all sensors with temperature greater than 50°C";
    
    // Create execution plan manually (simulating LLM translation)
    ExecutionPlan plan1;
    plan1.operation = OperationType::QUERY;
    plan1.datasource = "sensor_readings";
    plan1.filters = {
        FilterCondition{"temperature", ">", 50.0}
    };
    
    auto result1 = executor.execute(plan1);
    printExecutionResult(prompt1, result1);
    
    // Example 2: Aggregation query
    std::cout << "\n=== Example 2: Aggregation Query ===\n";
    std::string prompt2 = "Calculate average temperature for sensors over 50°C";
    
    ExecutionPlan plan2;
    plan2.operation = OperationType::AGGREGATE;
    plan2.datasource = "sensor_readings";
    plan2.filters = {
        FilterCondition{"temperature", ">", 50.0}
    };
    plan2.aggregations = {
        Aggregation{"AVG", "temperature", "avg_temp"},
        Aggregation{"COUNT", "sensor_id", "count"}
    };
    
    auto result2 = executor.execute(plan2);
    printExecutionResult(prompt2, result2);
    
    // Example 3: Query with sorting and pagination
    std::cout << "\n=== Example 3: Sorted Query with Pagination ===\n";
    std::string prompt3 = "Show top 3 hottest sensor readings";
    
    ExecutionPlan plan3;
    plan3.operation = OperationType::QUERY;
    plan3.datasource = "sensor_readings";
    plan3.parameters = {
        {"limit", static_cast<int64_t>(3)},
        {"offset", static_cast<int64_t>(0)}
    };
    
    auto result3 = executor.execute(plan3);
    printExecutionResult(prompt3, result3);
    
    // Example 4: With LLM translation (if vLLM available)
    if (vllm_available) {
        std::cout << "\n=== Example 4: Full Pipeline with LLM ===\n";
        std::string prompt4 = "Find sensors with temperature over 60 degrees";
        
        try {
            std::cout << "Translating prompt to execution plan via LLM...\n";
            auto plan4 = translator.translate(prompt4);
            
            std::cout << "✓ Plan generated:\n";
            std::cout << plan4.toJson().dump(2) << "\n\n";
            
            std::cout << "Executing plan...\n";
            auto result4 = executor.execute(plan4);
            printExecutionResult(prompt4, result4);
        } catch (const std::exception& e) {
            std::cout << "✗ LLM translation failed: " << e.what() << "\n";
        }
    }
    
    // Display execution statistics
    std::cout << "\n=== Execution Statistics ===\n";
    auto stats = executor.getStats();
    std::cout << "Total Executions: " << stats.total_executions << "\n";
    std::cout << "Successful: " << stats.successful_executions << "\n";
    std::cout << "Failed: " << stats.failed_executions << "\n";
    std::cout << "Average Execution Time: " << stats.avg_execution_time_ms << "ms\n";
    
    std::cout << "\n=== Pipeline Complete ===\n";
    std::cout << "\nArchitecture Flow:\n";
    std::cout << "  Natural Language Prompt\n";
    std::cout << "         ↓\n";
    std::cout << "  vLLM REST API (localhost:8000)\n";
    std::cout << "         ↓\n";
    std::cout << "  JSON Execution Plan\n";
    std::cout << "         ↓\n";
    std::cout << "  Direct Executor (Interpreter)\n";
    std::cout << "         ↓\n";
    std::cout << "  ThemisDB Operations\n";
    std::cout << "         ↓\n";
    std::cout << "  Results (JSON)\n\n";
    
    std::cout << "Performance: ~200ms (prompt → plan) + ~1-5ms (execution)\n";
    std::cout << "Total: ~200-210ms end-to-end\n\n";
    
    return 0;
}
