/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_optimization_example.cpp                    ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     274                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_optimization_example.cpp
 * @brief Example usage of the Prompt Engineering & Optimization Framework
 * 
 * Demonstrates:
 * - Iterative prompt optimization
 * - Prompt evaluation with multiple metrics
 * - Few-shot example selection
 * - Meta-prompt generation for improvements
 */

#include <iostream>
#include <vector>
#include <string>
#include "prompt_engineering/prompt_optimizer.h"
#include "prompt_engineering/prompt_evaluator.h"
#include "llm/fewshot_optimizer.h"
#include "prompt_engineering/meta_prompt_generator.h"

using namespace themis::prompt_engineering;

// Example evaluation function for demonstration purposes
// NOTE: In production, replace this with a function that:
//   1. Runs the prompt against test cases using a real LLM
//   2. Evaluates the LLM outputs against expected results
//   3. Returns a quality score (0.0-1.0)
// Example production integration:
//   - Call your LLM API (OpenAI, Anthropic, local llama.cpp, etc.)
//   - Use PromptEvaluator to compare outputs vs expected
//   - Return the aggregated score
double exampleEvaluator(const std::string& prompt, const std::vector<TestCase>& test_cases) {
    double score = 0.0;
    
    // Simple heuristic scoring based on prompt quality indicators
    // (Replace with actual LLM evaluation in production)
    if (prompt.find("Task") != std::string::npos || 
        prompt.find("Instructions") != std::string::npos) {
        score += 0.3;
    }
    
    if (prompt.find("Example") != std::string::npos) {
        score += 0.3;
    }
    
    if (prompt.find("Format") != std::string::npos || 
        prompt.find("Output") != std::string::npos) {
        score += 0.2;
    }
    
    if (prompt.length() > 50) {
        score += 0.2;
    }
    
    return std::min(1.0, score);
}

int main() {
    std::cout << "=== Prompt Engineering & Optimization Framework Example ===\n\n";
    
    // ========================================================================
    // Example 1: Basic Prompt Optimization
    // ========================================================================
    std::cout << "Example 1: Iterative Prompt Optimization\n";
    std::cout << "----------------------------------------\n";
    
    OptimizationConfig opt_config;
    opt_config.max_iterations = 3;
    opt_config.target_score = 0.8;
    opt_config.min_improvement = 0.05;
    
    PromptOptimizer optimizer(opt_config);
    
    std::string initial_prompt = "Summarize the text.";
    
    std::vector<TestCase> test_cases = {
        {"Long article text...", "Brief summary", {}},
        {"Another article...", "Another summary", {}},
        {"Technical document...", "Technical summary", {}}
    };
    
    auto result = optimizer.optimize(
        initial_prompt,
        test_cases,
        exampleEvaluator
    );
    
    std::cout << "Initial prompt: " << initial_prompt << "\n";
    std::cout << "Initial score: " << result.score_history[0] << "\n";
    std::cout << "Final prompt: " << result.optimized_prompt << "\n";
    std::cout << "Final score: " << result.final_score << "\n";
    std::cout << "Iterations: " << result.iterations << "\n";
    std::cout << "Converged: " << (result.converged ? "Yes" : "No") << "\n\n";
    
    // ========================================================================
    // Example 2: Prompt Evaluation
    // ========================================================================
    std::cout << "Example 2: Prompt Evaluation with Multiple Metrics\n";
    std::cout << "------------------------------------------------\n";
    
    EvaluatorConfig eval_config;
    eval_config.similarity_weight = 0.5;
    eval_config.exact_match_weight = 0.3;
    eval_config.relevance_weight = 0.2;
    eval_config.pass_threshold = 0.7;
    
    PromptEvaluator evaluator(eval_config);
    
    std::vector<std::string> outputs = {
        "This is a comprehensive summary of the document.",
        "Brief summary of key points.",
        "Detailed analysis with examples."
    };
    
    std::vector<std::string> expected = {
        "This is a comprehensive summary of the document.",
        "Short summary of main ideas.",
        "Detailed analysis with examples."
    };
    
    auto metrics = evaluator.evaluateBatch(outputs, expected);
    
    std::cout << "Overall Score: " << metrics.overall_score << "\n";
    std::cout << "Mean Similarity: " << metrics.mean_similarity << "\n";
    std::cout << "Exact Matches: " << metrics.num_exact_matches << "/" << outputs.size() << "\n";
    std::cout << "Pass Rate: " << (metrics.pass_rate * 100.0) << "%\n\n";
    
    // ========================================================================
    // Example 3: Few-Shot Example Selection
    // ========================================================================
    std::cout << "Example 3: Automatic Few-Shot Example Selection\n";
    std::cout << "----------------------------------------------\n";
    
    FewShotConfig fs_config;
    fs_config.max_examples = 3;
    fs_config.diversity_weight = 0.4;
    fs_config.relevance_weight = 0.6;
    
    FewShotOptimizer fs_optimizer(fs_config);
    
    std::vector<FewShotExample> candidate_examples = {
        {"What is 2+2?", "4", {}, 0.0, 0.0, {}},
        {"What is 3+3?", "6", {}, 0.0, 0.0, {}},
        {"What is the capital of France?", "Paris", {}, 0.0, 0.0, {}},
        {"What is the capital of Germany?", "Berlin", {}, 0.0, 0.0, {}},
        {"Who wrote Hamlet?", "Shakespeare", {}, 0.0, 0.0, {}},
        {"Calculate 5+7", "12", {}, 0.0, 0.0, {}}
    };
    
    std::string query = "What is 10+15?";
    auto selection = fs_optimizer.selectExamples(query, candidate_examples, 3);
    
    std::cout << "Query: " << query << "\n";
    std::cout << "Selected " << selection.selected_examples.size() << " examples:\n";
    for (size_t i = 0; i < selection.selected_examples.size(); ++i) {
        const auto& ex = selection.selected_examples[i];
        std::cout << "  " << (i+1) << ". Input: " << ex.input 
                  << " -> Output: " << ex.output << "\n";
    }
    std::cout << "Average Relevance: " << selection.avg_relevance << "\n";
    std::cout << "Average Diversity: " << selection.avg_diversity << "\n";
    std::cout << "Selection Score: " << selection.selection_score << "\n\n";
    
    // ========================================================================
    // Example 4: Meta-Prompt Generation
    // ========================================================================
    std::cout << "Example 4: Meta-Prompt Generation for Improvement\n";
    std::cout << "------------------------------------------------\n";
    
    MetaPromptConfig meta_config;
    meta_config.improvement_strategy = "iterative";
    meta_config.include_examples = true;
    meta_config.include_constraints = true;
    
    MetaPromptGenerator meta_gen(meta_config);
    
    std::string current_prompt = "Classify the sentiment of the text.";
    std::string feedback = "The prompt needs to be more specific about output format and provide examples.";
    double current_score = 0.6;
    
    auto meta_result = meta_gen.generateImprovementPrompt(
        current_prompt,
        feedback,
        current_score,
        "Sentiment classification (positive, negative, neutral)"
    );
    
    std::cout << "Original Prompt: " << current_prompt << "\n";
    std::cout << "Current Score: " << current_score << "\n";
    std::cout << "Feedback: " << feedback << "\n\n";
    
    std::cout << "Generated Meta-Prompt:\n";
    std::cout << "------------------------\n";
    std::cout << meta_result.meta_prompt.substr(0, 500) << "...\n\n";
    
    std::cout << "Key Insights:\n";
    for (const auto& insight : meta_result.key_insights) {
        std::cout << "  - " << insight << "\n";
    }
    std::cout << "\n";
    
    // ========================================================================
    // Example 5: Complete Workflow
    // ========================================================================
    std::cout << "Example 5: Complete Optimization Workflow\n";
    std::cout << "----------------------------------------\n";
    
    // 1. Start with a basic prompt
    std::string workflow_prompt = "Answer questions about databases.";
    std::cout << "Step 1: Initial prompt: " << workflow_prompt << "\n";
    
    // 2. Select relevant few-shot examples
    std::vector<FewShotExample> db_examples = {
        {"What is SQL?", "SQL is a language for managing databases.", {}, 0.0, 0.0, {}},
        {"What is a primary key?", "A primary key uniquely identifies each record.", {}, 0.0, 0.0, {}},
        {"What is normalization?", "Normalization organizes data to reduce redundancy.", {}, 0.0, 0.0, {}}
    };
    
    auto db_selection = fs_optimizer.selectExamples(
        "What is ACID?", 
        db_examples, 
        2
    );
    std::cout << "Step 2: Selected " << db_selection.selected_examples.size() 
              << " few-shot examples\n";
    
    // 3. Format examples for prompt
    std::string formatted_examples = FewShotOptimizer::formatExamples(
        db_selection.selected_examples
    );
    
    // 4. Create enhanced prompt
    std::string enhanced_prompt = workflow_prompt + "\n\nExamples:\n" + formatted_examples;
    std::cout << "Step 3: Enhanced prompt with examples (length: " 
              << enhanced_prompt.length() << " chars)\n";
    
    // 5. Evaluate the enhanced prompt
    std::vector<std::string> workflow_outputs = {
        "ACID stands for Atomicity, Consistency, Isolation, Durability.",
        "ACID is a set of properties for database transactions."
    };
    std::vector<std::string> workflow_expected = {
        "ACID stands for Atomicity, Consistency, Isolation, Durability.",
        "ACID is a set of database transaction properties."
    };
    
    auto workflow_metrics = evaluator.evaluateBatch(workflow_outputs, workflow_expected);
    std::cout << "Step 4: Evaluation score: " << workflow_metrics.overall_score << "\n";
    
    std::cout << "\n=== Example Complete ===\n";
    
    return 0;
}
