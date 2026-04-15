/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            basic_usage.cpp                                    ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:39:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     325                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file basic_usage.cpp
 * @brief Basic usage examples for LLM Code Translator
 * 
 * This file demonstrates how to use the LLM Code Translator to generate
 * and execute code from natural language descriptions.
 */

#include "../src/llm_code_translator.h"
#include <iostream>
#include <memory>

using namespace themis;

/**
 * Example 1: Simple AQL Query Generation
 */
void example1_simple_aql_generation() {
    std::cout << "=== Example 1: Simple AQL Query Generation ===\n\n";

    // Initialize translator
    // Note: In real usage, you would pass your actual DB instance
    rocksdb::TransactionDB* db = nullptr;  // Placeholder - use actual DB in production
    
    LLMCodeTranslator::Config config;
    config.llm_endpoint = "http://localhost:8000";
    config.enable_security_review = true;
    
    LLMCodeTranslator translator(db, config);

    // User describes what they want
    std::string user_request = R"(
        Find all users who were active in the last 7 days
        and group them by city. Return the city name and count.
    )";

    // Generate AQL code
    auto result = translator.generateCode(user_request, "aql");

    if (result.success) {
        std::cout << "Generated Code:\n";
        std::cout << "```aql\n" << result.code << "\n```\n\n";
        
        std::cout << "Quality Score: " << result.quality_score << "\n";
        std::cout << "Security Approved: " << (result.security_approved ? "Yes" : "No") << "\n\n";

        // Execute if approved
        if (result.security_approved) {
            std::cout << "Executing code...\n";
            auto exec_result = translator.executeCode(result.code, "aql");
            
            if (exec_result.success) {
                std::cout << "Result: " << exec_result.output << "\n";
                std::cout << "Execution time: " << exec_result.duration_ms << "ms\n";
            } else {
                std::cout << "Execution failed: " << exec_result.error << "\n";
            }
        }
    } else {
        std::cout << "Generation failed: " << result.error << "\n";
    }
}

/**
 * Example 2: Python Data Analysis Script
 */
void example2_python_data_analysis() {
    std::cout << "\n=== Example 2: Python Data Analysis Script ===\n\n";

    auto db = /* your ThemisDB instance */;
    LLMCodeTranslator translator(db);

    std::string user_request = R"(
        Create a Python script that:
        1. Fetches sales data from ThemisDB (table: sales)
        2. Calculates top 10 products by revenue
        3. Creates a bar chart visualization
        4. Saves the chart as 'top_products.png'
    )";

    // Provide context about available data
    std::map<std::string, std::string> context;
    context["available_tables"] = "sales (columns: product_id, amount, timestamp, customer_id)";
    context["themisdb_api"] = "http://localhost:8765";

    auto result = translator.generateCode(user_request, "python", context);

    if (result.success) {
        std::cout << "Generated Python Script:\n";
        std::cout << "```python\n" << result.code << "\n```\n\n";

        // Show warnings if any
        if (!result.warnings.empty()) {
            std::cout << "Warnings:\n";
            for (const auto& warning : result.warnings) {
                std::cout << "  - " << warning << "\n";
            }
            std::cout << "\n";
        }

        // Show suggestions
        if (!result.suggestions.empty()) {
            std::cout << "Suggestions:\n";
            for (const auto& suggestion : result.suggestions) {
                std::cout << "  - " << suggestion << "\n";
            }
        }
    }
}

/**
 * Example 3: C++ API Handler Generation
 */
void example3_cpp_api_handler() {
    std::cout << "\n=== Example 3: C++ API Handler Generation ===\n\n";

    rocksdb::TransactionDB* db = nullptr;  // Placeholder - use actual DB in production
    LLMCodeTranslator translator(db);

    std::string user_request = R"(
        Create a C++ HTTP POST handler that:
        1. Accepts JSON with fields: id, name, email
        2. Validates all required fields are present
        3. Validates email format
        4. Stores the user in ThemisDB table 'users'
        5. Returns 201 Created with the user ID on success
        6. Returns appropriate error codes on failure (400, 500)
    )";

    std::map<std::string, std::string> context;
    context["db_api"] = "db->Put(WriteOptions(), key, value)";
    context["http_lib"] = "httplib (Beast/Boost.Asio)";

    auto result = translator.generateCode(user_request, "cpp", context);

    if (result.success) {
        std::cout << "Generated C++ Handler:\n";
        std::cout << "```cpp\n" << result.code << "\n```\n\n";
        
        // Review before use
        auto review = translator.reviewCode(result.code, "cpp");
        
        std::cout << "Code Review:\n";
        std::cout << "Approved: " << (review.approved ? "Yes" : "No") << "\n";
        std::cout << "Quality Score: " << review.quality_score << "\n";
        
        if (!review.issues.empty()) {
            std::cout << "\nIssues:\n";
            for (const auto& issue : review.issues) {
                std::cout << "  - " << issue << "\n";
            }
        }
    }
}

/**
 * Example 4: Iterative Improvement with Feedback
 */
void example4_iterative_improvement() {
    std::cout << "\n=== Example 4: Iterative Improvement ===\n\n";

    rocksdb::TransactionDB* db = nullptr;  // Placeholder - use actual DB in production
    LLMCodeTranslator translator(db);

    std::string initial_request = R"(
        Calculate the average temperature from sensor data
        for the last 24 hours.
    )";

    // First generation
    auto v1 = translator.generateCode(initial_request, "aql");
    std::cout << "Version 1:\n```aql\n" << v1.code << "\n```\n\n";

    // User provides feedback
    std::string feedback = R"(
        The code works but is slow. Please optimize by:
        1. Using an index on the timestamp field
        2. Limiting the scan with a filter on timestamp first
        3. Adding a comment explaining the optimization
    )";

    // Regenerate with feedback
    auto v2 = translator.regenerateWithFeedback(v1, feedback);
    
    if (v2.success) {
        std::cout << "Version 2 (Optimized):\n```aql\n" << v2.code << "\n```\n\n";
        std::cout << "Quality improved: " << v1.quality_score << " → " << v2.quality_score << "\n";
    }
}

/**
 * Example 5: Security Review and Sandboxed Execution
 */
void example5_security_and_sandboxing() {
    std::cout << "\n=== Example 5: Security and Sandboxing ===\n\n";

    rocksdb::TransactionDB* db = nullptr;  // Placeholder - use actual DB in production
    
    LLMCodeTranslator::Config config;
    config.enable_security_review = true;  // Enable automatic security review
    config.enable_auto_execution = false;  // Don't auto-execute (manual approval)
    
    LLMCodeTranslator translator(db, config);

    std::string user_request = "Process user input and store in database";

    auto result = translator.generateAndReview(user_request, "python");

    std::cout << "Security Review:\n";
    std::cout << "Approved: " << (result.security_approved ? "Yes" : "No") << "\n";
    
    if (!result.warnings.empty()) {
        std::cout << "Security Warnings:\n";
        for (const auto& warning : result.warnings) {
            std::cout << "  ⚠️  " << warning << "\n";
        }
    }

    if (result.security_approved) {
        // Execute with strict limits
        LLMCodeTranslator::ExecutionLimits limits;
        limits.max_execution_time_ms = 5000;   // 5 seconds max
        limits.max_memory_mb = 256;            // 256 MB max
        limits.allow_network = false;           // No network access
        limits.allow_file_writes = false;       // No file writes

        auto exec_result = translator.executeCode(result.code, "python", limits);
        
        if (exec_result.success) {
            std::cout << "\nExecution successful\n";
            std::cout << "Duration: " << exec_result.duration_ms << "ms\n";
            std::cout << "Memory used: " << exec_result.memory_used_kb << "KB\n";
        } else {
            std::cout << "\nExecution failed: " << exec_result.error << "\n";
        }
    } else {
        std::cout << "\nCode did not pass security review. Manual inspection required.\n";
    }
}

/**
 * Example 6: Custom Prompt Templates
 */
void example6_custom_templates() {
    std::cout << "\n=== Example 6: Custom Prompt Templates ===\n\n";

    rocksdb::TransactionDB* db = nullptr;  // Placeholder - use actual DB in production
    LLMCodeTranslator translator(db);

    // Define a custom template for specialized domain
    std::string custom_template = R"(
You are an expert in IoT sensor data processing with ThemisDB.

User Request:
{user_description}

Generate AQL code that:
1. Uses IoT-specific best practices
2. Handles sensor data edge cases (missing values, outliers)
3. Optimizes for time-series data patterns
4. Includes data quality checks

Available sensor tables:
{available_tables}

Generate only valid AQL code:
)";

    translator.setPromptTemplate("iot_sensor_processing", custom_template);

    std::string request = "Find sensors with anomalous temperature readings";
    
    std::map<std::string, std::string> context;
    context["available_tables"] = "sensors (sensor_id, temperature, humidity, timestamp)";

    auto result = translator.generateCode(request, "aql", context);
    
    if (result.success) {
        std::cout << "Generated IoT-optimized code:\n";
        std::cout << "```aql\n" << result.code << "\n```\n";
    }
}

int main() {
    std::cout << "LLM Code Translator - Usage Examples\n";
    std::cout << "====================================\n\n";

    try {
        example1_simple_aql_generation();
        example2_python_data_analysis();
        example3_cpp_api_handler();
        example4_iterative_improvement();
        example5_security_and_sandboxing();
        example6_custom_templates();

        std::cout << "\nAll examples completed successfully!\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
