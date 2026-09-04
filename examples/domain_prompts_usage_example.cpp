/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            domain_prompts_usage_example.cpp                   ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     242                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//
// Example: Using Domain-Specific Prompts
//
// This example demonstrates how to load and use domain-specific prompt
// templates from the config/prompts/ directory.

#include "prompt_engineering/prompt_manager.h"
#include "llm/llama_wrapper.h"
#include <iostream>
#include <memory>

using namespace themis::prompt_engineering;

int main() {
    std::cout << "=== Domain-Specific Prompts Usage Example ===" << std::endl;
    
    // 1. Create PromptManager
    auto manager = std::make_shared<PromptManager>();
    
    // 2. Load all domain-specific prompts
    std::cout << "\nLoading domain-specific prompt templates..." << std::endl;
    
    const std::vector<std::string> prompt_files = {
        "config/prompts/standard_prompts.yaml",
        "config/prompts/scientific_prompts.yaml",
        "config/prompts/legal_prompts.yaml",
        "config/prompts/technical_prompts.yaml",
        "config/prompts/economic_prompts.yaml",
        "config/prompts/mathematical_prompts.yaml",
        "config/prompts/geographic_prompts.yaml"
    };
    
    size_t total_prompts = 0;
    for (const auto& file : prompt_files) {
        size_t loaded = manager->loadFromYAML(file);
        std::cout << "  Loaded " << loaded << " prompts from " << file << std::endl;
        total_prompts += loaded;
    }
    
    std::cout << "\nTotal prompts loaded: " << total_prompts << std::endl;
    
    // 3. Example 1: Standard Query
    std::cout << "\n=== Example 1: Standard Data Summary ===" << std::endl;
    {
        std::unordered_map<std::string, std::string> context = {
            {"query", "Summarize customer purchase data for Q4 2025"},
            {"version", "1.5.0"},
            {"edition", "Enterprise"},
            {"table_count", "15"}
        };
        
        auto prompt = manager->getPromptWithContext("data_summary", context);
        if (prompt) {
            std::cout << "Generated prompt:" << std::endl;
            std::cout << prompt.value().substr(0, 200) << "..." << std::endl;
        }
    }
    
    // 4. Example 2: Scientific Analysis
    std::cout << "\n=== Example 2: Scientific Hypothesis Testing ===" << std::endl;
    {
        std::unordered_map<std::string, std::string> context = {
            {"query", "Test hypothesis: Customer satisfaction correlates with response time"},
            {"context", "Response time data: [120ms, 250ms, 180ms, ...]\n"
                       "Satisfaction scores: [4.5, 3.8, 4.2, ...]"}
        };
        
        auto prompt = manager->getPromptWithContext("scientific_hypothesis", context);
        if (prompt) {
            std::cout << "Generated prompt:" << std::endl;
            std::cout << prompt.value().substr(0, 200) << "..." << std::endl;
        }
    }
    
    // 5. Example 3: Legal Compliance Check
    std::cout << "\n=== Example 3: Legal Compliance ===" << std::endl;
    {
        std::unordered_map<std::string, std::string> context = {
            {"query", "Check GDPR compliance for customer data storage"},
            {"context", "Current practices:\n"
                       "- Data encrypted at rest\n"
                       "- Access logs maintained\n"
                       "- 30-day retention policy"}
        };
        
        auto prompt = manager->getPromptWithContext("compliance_check", context);
        if (prompt) {
            std::cout << "Generated prompt:" << std::endl;
            std::cout << prompt.value().substr(0, 200) << "..." << std::endl;
        }
    }
    
    // 6. Example 4: Technical Debugging
    std::cout << "\n=== Example 4: Technical Debugging ===" << std::endl;
    {
        std::unordered_map<std::string, std::string> context = {
            {"query", "Database queries timing out after 5 seconds"},
            {"context", "Symptoms:\n"
                       "- Slow queries on customer table\n"
                       "- CPU at 80%\n"
                       "- Missing indexes on frequently queried columns"}
        };
        
        auto prompt = manager->getPromptWithContext("debugging_analysis", context);
        if (prompt) {
            std::cout << "Generated prompt:" << std::endl;
            std::cout << prompt.value().substr(0, 200) << "..." << std::endl;
        }
    }
    
    // 7. Example 5: Economic Analysis
    std::cout << "\n=== Example 5: ROI Analysis ===" << std::endl;
    {
        std::unordered_map<std::string, std::string> context = {
            {"query", "Calculate ROI for new feature development"},
            {"context", "Investment: $50,000\n"
                       "Expected additional revenue: $120,000/year\n"
                       "Ongoing costs: $20,000/year"}
        };
        
        auto prompt = manager->getPromptWithContext("roi_analysis", context);
        if (prompt) {
            std::cout << "Generated prompt:" << std::endl;
            std::cout << prompt.value().substr(0, 200) << "..." << std::endl;
        }
    }
    
    // 8. Example 6: Mathematical Statistics
    std::cout << "\n=== Example 6: Statistical Analysis ===" << std::endl;
    {
        std::unordered_map<std::string, std::string> context = {
            {"query", "Analyze distribution of response times"},
            {"context", "Response times (ms): [120, 135, 142, 156, 189, 201, 215, 234, 298, 312]\n"
                       "Sample size: 10,000 requests"}
        };
        
        auto prompt = manager->getPromptWithContext("statistical_analysis", context);
        if (prompt) {
            std::cout << "Generated prompt:" << std::endl;
            std::cout << prompt.value().substr(0, 200) << "..." << std::endl;
        }
    }
    
    // 9. Example 7: Geographic Query
    std::cout << "\n=== Example 7: Distance Calculation ===" << std::endl;
    {
        std::unordered_map<std::string, std::string> context = {
            {"query", "Calculate distance between New York and London"},
            {"context", "NYC: 40.7128°N, 74.0060°W\n"
                       "London: 51.5074°N, 0.1278°W"}
        };
        
        auto prompt = manager->getPromptWithContext("distance_calculation", context);
        if (prompt) {
            std::cout << "Generated prompt:" << std::endl;
            std::cout << prompt.value().substr(0, 200) << "..." << std::endl;
        }
    }
    
    // 10. List all available prompts
    std::cout << "\n=== All Available Prompts ===" << std::endl;
    auto all_prompts = manager->listTemplates();
    std::cout << "Total prompts available: " << all_prompts.size() << std::endl;
    
    // Group by domain (based on prefix or metadata)
    std::unordered_map<std::string, std::vector<std::string>> by_domain;
    for (const auto& pt : all_prompts) {
        std::string domain = "standard";
        if (pt.id.find("scientific") != std::string::npos) {
          domain = "scientific";
        }
        else if (pt.id.find("legal") != std::string::npos) domain = "legal";
        else if (pt.id.find("technical") != std::string::npos) domain = "technical";
        else if (pt.id.find("business") != std::string::npos || 
                 pt.id.find("financial") != std::string::npos ||
                 pt.id.find("economic") != std::string::npos ||
                 pt.id.find("roi") != std::string::npos ||
                 pt.id.find("market") != std::string::npos ||
                 pt.id.find("pricing") != std::string::npos ||
                 pt.id.find("competitive") != std::string::npos ||
                 pt.id.find("cost_benefit") != std::string::npos) domain = "economic";
        else if (pt.id.find("mathematical") != std::string::npos ||
                 pt.id.find("statistical") != std::string::npos ||
                 pt.id.find("optimization") != std::string::npos ||
                 pt.id.find("probability") != std::string::npos ||
                 pt.id.find("numerical") != std::string::npos ||
                 pt.id.find("linear_algebra") != std::string::npos ||
                 pt.id.find("calculus") != std::string::npos ||
                 pt.id.find("combinatorics") != std::string::npos) domain = "mathematical";
        else if (pt.id.find("location") != std::string::npos ||
                 pt.id.find("spatial") != std::string::npos ||
                 pt.id.find("distance") != std::string::npos ||
                 pt.id.find("region") != std::string::npos ||
                 pt.id.find("map") != std::string::npos ||
                 pt.id.find("route") != std::string::npos ||
                 pt.id.find("geospatial") != std::string::npos ||
                 pt.id.find("geocoding") != std::string::npos) domain = "geographic";
        
        by_domain[domain].push_back(pt.name);
    }
    
    for (const auto& [domain, prompts] : by_domain) {
        std::cout << "\n" << domain << " (" << prompts.size() << " prompts):" << std::endl;
        for (const auto& name : prompts) {
            std::cout << "  - " << name << std::endl;
        }
    }
    
    std::cout << "\n=== Usage with LLM ===" << std::endl;
    std::cout << "To use these prompts with an LLM:" << std::endl;
    std::cout << "1. Select appropriate domain prompt for your query" << std::endl;
    std::cout << "2. Build context map with relevant data" << std::endl;
    std::cout << "3. Get prompt with context injection" << std::endl;
    std::cout << "4. Send to LLM for generation" << std::endl;
    std::cout << "5. Process and return response" << std::endl;
    
    std::cout << "\n=== Complete! ===" << std::endl;
    
    return 0;
}
