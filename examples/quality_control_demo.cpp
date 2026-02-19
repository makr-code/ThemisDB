/**
 * @file quality_control_demo.cpp
 * @brief Demonstration of ThemisDB Quality Control System
 * 
 * Shows how to use the complete post-generation quality control pipeline
 * for RAG outputs with Fast, Balanced, and Thorough modes.
 */

#include "rag/quality_control_pipeline.h"
#include "rag/llm_judge_client.h"
#include "rag/geval_evaluator.h"
#include "rag/nli_faithfulness_verifier.h"
#include <iostream>
#include <iomanip>

using namespace themis::rag::judge;

// Helper to print decision
std::string decisionToString(QCDecision decision) {
    switch (decision) {
        case QCDecision::ACCEPT: return "ACCEPT";
        case QCDecision::REJECT: return "REJECT";
        case QCDecision::RETRY: return "RETRY";
        case QCDecision::WARN: return "WARN";
        default: return "UNKNOWN";
    }
}

// Helper to print mode
std::string modeToString(QCMode mode) {
    switch (mode) {
        case QCMode::FAST: return "FAST";
        case QCMode::BALANCED: return "BALANCED";
        case QCMode::THOROUGH: return "THOROUGH";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Example 1: Basic Quality Control
 */
void example1_basic_qc() {
    std::cout << "\n=== Example 1: Basic Quality Control ===\n\n";
    
    // Create pipeline with default configuration
    QualityControlPipeline pipeline;
    
    // Sample RAG output
    std::string query = "What is the capital of France?";
    
    std::vector<RetrievedDocument> documents = {
        {"doc1", "Paris is the capital of France, located in Western Europe.", 0.95, {}},
        {"doc2", "France has a population of about 67 million people.", 0.85, {}},
        {"doc3", "The Eiffel Tower is a famous landmark in Paris.", 0.80, {}}
    };
    
    std::string answer = "The capital of France is Paris, a beautiful city in Western Europe.";
    
    // Run quality control
    auto result = pipeline.runQualityControl(query, documents, answer);
    
    // Display results
    std::cout << "Query: " << query << "\n";
    std::cout << "Answer: " << answer << "\n\n";
    std::cout << "Quality Control Results:\n";
    std::cout << "  Decision: " << decisionToString(result.decision) << "\n";
    std::cout << "  Overall Score: " << std::fixed << std::setprecision(3) 
              << result.overall_score << "\n";
    std::cout << "  Faithfulness: " << result.faithfulness_score << "\n";
    std::cout << "  Relevance: " << result.relevance_score << "\n";
    std::cout << "  Completeness: " << result.completeness_score << "\n";
    std::cout << "  Coherence: " << result.coherence_score << "\n";
    std::cout << "  Latency: " << result.latency.count() << " ms\n";
    std::cout << "  Passed Threshold: " << (result.passed_threshold ? "Yes" : "No") << "\n";
}

/**
 * @brief Example 2: Different QC Modes
 */
void example2_different_modes() {
    std::cout << "\n=== Example 2: Different QC Modes ===\n\n";
    
    QualityControlPipeline pipeline;
    
    std::string query = "Explain quantum entanglement";
    std::vector<RetrievedDocument> documents = {
        {"doc1", "Quantum entanglement is a physical phenomenon where particles remain connected.", 0.9, {}}
    };
    std::string answer = "Quantum entanglement is when particles are mysteriously connected.";
    
    // Test each mode
    std::vector<QCMode> modes = {QCMode::FAST, QCMode::BALANCED, QCMode::THOROUGH};
    
    for (auto mode : modes) {
        auto result = pipeline.runQualityControl(query, documents, answer, mode);
        
        std::cout << modeToString(mode) << " Mode:\n";
        std::cout << "  Score: " << std::fixed << std::setprecision(3) 
                  << result.overall_score << "\n";
        std::cout << "  Decision: " << decisionToString(result.decision) << "\n";
        std::cout << "  Latency: " << result.latency.count() << " ms\n\n";
    }
}

/**
 * @brief Example 3: Adaptive QC with Time Budget
 */
void example3_adaptive_qc() {
    std::cout << "\n=== Example 3: Adaptive QC ===\n\n";
    
    QualityControlPipeline pipeline;
    
    std::string query = "What is machine learning?";
    std::vector<RetrievedDocument> documents = {
        {"doc1", "Machine learning is a subset of AI that enables systems to learn from data.", 0.9, {}}
    };
    std::string answer = "Machine learning allows computers to learn from data without explicit programming.";
    
    // Different time budgets
    std::vector<int> budgets = {50, 500, 2000};
    
    for (int budget : budgets) {
        auto result = pipeline.runAdaptiveQC(query, documents, answer, budget);
        
        std::cout << "Time Budget: " << budget << " ms\n";
        std::cout << "  Selected Mode: " << modeToString(result.mode) << "\n";
        std::cout << "  Score: " << std::fixed << std::setprecision(3) 
                  << result.overall_score << "\n";
        std::cout << "  Actual Latency: " << result.latency.count() << " ms\n\n";
    }
}

/**
 * @brief Example 4: Batch Quality Control
 */
void example4_batch_qc() {
    std::cout << "\n=== Example 4: Batch Quality Control ===\n\n";
    
    QualityControlPipeline pipeline;
    
    // Multiple RAG outputs to evaluate
    std::vector<EvaluationInput> inputs;
    
    for (int i = 1; i <= 3; i++) {
        EvaluationInput input;
        input.query = "Question " + std::to_string(i);
        input.documents = {
            {"doc" + std::to_string(i), "Document content for question " + std::to_string(i), 0.9, {}}
        };
        input.generated_answer = "Answer " + std::to_string(i) + " with some content.";
        inputs.push_back(input);
    }
    
    // Batch process
    auto results = pipeline.batchQualityControl(inputs, QCMode::FAST);
    
    std::cout << "Batch processed " << results.size() << " outputs:\n\n";
    
    for (size_t i = 0; i < results.size(); i++) {
        std::cout << "Output " << (i + 1) << ":\n";
        std::cout << "  Score: " << std::fixed << std::setprecision(3) 
                  << results[i].overall_score << "\n";
        std::cout << "  Decision: " << decisionToString(results[i].decision) << "\n";
    }
}

/**
 * @brief Example 5: Custom Configuration
 */
void example5_custom_config() {
    std::cout << "\n=== Example 5: Custom Configuration ===\n\n";
    
    // Create custom configuration
    QualityControlPipeline::Config config;
    config.default_mode = QCMode::THOROUGH;
    config.accept_threshold = 0.85;  // Stricter threshold
    config.reject_threshold = 0.60;
    config.enable_retry = true;
    config.max_retries = 3;
    config.enable_nli_verification = true;
    config.enable_geval_scoring = true;
    
    QualityControlPipeline pipeline(config);
    
    std::string query = "What are the benefits of renewable energy?";
    std::vector<RetrievedDocument> documents = {
        {"doc1", "Renewable energy reduces carbon emissions and helps combat climate change.", 0.9, {}}
    };
    std::string answer = "Renewable energy is good for the environment.";
    
    auto result = pipeline.runQualityControl(query, documents, answer);
    
    std::cout << "Custom Configuration Results:\n";
    std::cout << "  Accept Threshold: " << config.accept_threshold << "\n";
    std::cout << "  Score: " << std::fixed << std::setprecision(3) 
              << result.overall_score << "\n";
    std::cout << "  Decision: " << decisionToString(result.decision) << "\n";
    std::cout << "  Retry Count: " << result.retry_count << "\n";
}

/**
 * @brief Example 6: Using Callback for Monitoring
 */
void example6_with_callback() {
    std::cout << "\n=== Example 6: Quality Control with Callback ===\n\n";
    
    QualityControlPipeline pipeline;
    
    // Set callback to log quality metrics
    pipeline.setQCCallback([](const QCResult& result) {
        std::cout << "  [Callback] Evaluation complete:\n";
        std::cout << "    Score: " << std::fixed << std::setprecision(3) 
                  << result.overall_score << "\n";
        std::cout << "    Decision: " << decisionToString(result.decision) << "\n";
    });
    
    std::string query = "What is photosynthesis?";
    std::vector<RetrievedDocument> documents = {
        {"doc1", "Photosynthesis is the process by which plants convert light into energy.", 0.9, {}}
    };
    std::string answer = "Photosynthesis is how plants make food using sunlight.";
    
    std::cout << "Running QC with callback...\n";
    auto result = pipeline.runQualityControl(query, documents, answer);
}

/**
 * @brief Example 7: Statistics and Monitoring
 */
void example7_statistics() {
    std::cout << "\n=== Example 7: Statistics and Monitoring ===\n\n";
    
    QualityControlPipeline pipeline;
    
    // Run multiple evaluations
    for (int i = 0; i < 5; i++) {
        pipeline.runQualityControl(
            "Question " + std::to_string(i),
            {{"doc", "Document content", 0.9, {}}},
            "Answer " + std::to_string(i),
            i % 2 == 0 ? QCMode::FAST : QCMode::BALANCED
        );
    }
    
    // Get statistics
    auto stats = pipeline.getStatistics();
    
    std::cout << "Quality Control Statistics:\n";
    std::cout << "  Total Evaluations: " << stats.total_evaluations << "\n";
    std::cout << "  Accepted: " << stats.accepted << "\n";
    std::cout << "  Rejected: " << stats.rejected << "\n";
    std::cout << "  Warned: " << stats.warned << "\n";
    std::cout << "  Retried: " << stats.retried << "\n";
    std::cout << "  Avg Score: " << std::fixed << std::setprecision(3) 
              << stats.avg_score << "\n";
    std::cout << "  Avg Latency: " << stats.avg_latency_ms << " ms\n";
    std::cout << "  Mode Usage:\n";
    std::cout << "    FAST: " << stats.mode_usage.count(QCMode::FAST) 
              << " (" << (stats.mode_usage.count(QCMode::FAST) ? 
                        stats.mode_usage.at(QCMode::FAST) : 0) << ")\n";
    std::cout << "    BALANCED: " << stats.mode_usage.count(QCMode::BALANCED)
              << " (" << (stats.mode_usage.count(QCMode::BALANCED) ? 
                        stats.mode_usage.at(QCMode::BALANCED) : 0) << ")\n";
}

/**
 * @brief Example 8: Factory Methods
 */
void example8_factory_methods() {
    std::cout << "\n=== Example 8: Using Factory Methods ===\n\n";
    
    std::string query = "What is DNA?";
    std::vector<RetrievedDocument> documents = {
        {"doc1", "DNA is the molecule that carries genetic information.", 0.9, {}}
    };
    std::string answer = "DNA stores genetic information in living organisms.";
    
    // Create pipelines using factory
    auto fast_pipeline = QualityControlPipelineFactory::createFast();
    auto balanced_pipeline = QualityControlPipelineFactory::createBalanced();
    auto thorough_pipeline = QualityControlPipelineFactory::createThorough();
    
    std::cout << "Fast Pipeline:\n";
    auto result1 = fast_pipeline->runQualityControl(query, documents, answer);
    std::cout << "  Score: " << std::fixed << std::setprecision(3) 
              << result1.overall_score << "\n\n";
    
    std::cout << "Balanced Pipeline:\n";
    auto result2 = balanced_pipeline->runQualityControl(query, documents, answer);
    std::cout << "  Score: " << std::fixed << std::setprecision(3) 
              << result2.overall_score << "\n\n";
    
    std::cout << "Thorough Pipeline:\n";
    auto result3 = thorough_pipeline->runQualityControl(query, documents, answer);
    std::cout << "  Score: " << std::fixed << std::setprecision(3) 
              << result3.overall_score << "\n";
}

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ThemisDB Quality Control System - Demonstration         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    
    try {
        example1_basic_qc();
        example2_different_modes();
        example3_adaptive_qc();
        example4_batch_qc();
        example5_custom_config();
        example6_with_callback();
        example7_statistics();
        example8_factory_methods();
        
        std::cout << "\n=== All examples completed successfully! ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
