/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            continuous_learning_integration_example.cpp        ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     305                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file continuous_learning_integration_example.cpp
 * @brief Example demonstrating continuous learning integration with quality control
 */

#include "rag/quality_control_factory.h"
#include "rag/continuous_learning_client.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

using namespace themis::rag::judge;

/**
 * @brief Example 1: Basic Continuous Learning Integration
 */
void example1_basic_integration() {
    std::cout << "\n=== Example 1: Basic Continuous Learning Integration ===\n\n";
    
    // Create continuous learning client
    ContinuousLearningClient::Config cl_config;
    cl_config.endpoint = "http://localhost:8080/metrics";
    cl_config.enable_logging = true;
    cl_config.enable_triggers = true;
    
    auto cl_client = std::make_shared<ContinuousLearningClient>(cl_config);
    
    // Create quality control pipeline
    auto pipeline = QualityControlFactory::createBasic();
    
    // Simulate RAG evaluations
    std::vector<RetrievedDocument> documents = {
        {"doc1", "Paris is the capital of France.", 0.95, {}}
    };
    
    for (int i = 0; i < 5; i++) {
        std::string query = "What is the capital of France?";
        std::string answer = "The capital of France is Paris.";
        
        auto result = pipeline->runQualityControl(query, documents, answer);
        
        // Log to continuous learning
        cl_client->logQCResult(result);
        
        std::cout << "Evaluation " << (i + 1) << ": Score " << std::fixed 
                  << std::setprecision(3) << result.overall_score << "\n";
    }
    
    auto stats = cl_client->getStatistics();
    std::cout << "\nMetrics logged: " << stats.metrics_logged << "\n";
    std::cout << "Metrics sent: " << stats.metrics_sent << "\n";
}

/**
 * @brief Example 2: Automatic Trigger Detection
 */
void example2_trigger_detection() {
    std::cout << "\n=== Example 2: Automatic Trigger Detection ===\n\n";
    
    // Configure with thresholds
    ContinuousLearningClient::Config cl_config;
    cl_config.faithfulness_threshold = 0.75;
    cl_config.relevance_threshold = 0.70;
    cl_config.metric_window_size = 10;
    cl_config.enable_batching = false;
    
    auto cl_client = std::make_shared<ContinuousLearningClient>(cl_config);
    
    // Set up trigger callback
    cl_client->setTriggerCallback([](const OptimizationTrigger& trigger) {
        std::cout << "\n⚠️  OPTIMIZATION TRIGGER FIRED\n";
        std::cout << "  Type: " << trigger.trigger_type << "\n";
        std::cout << "  Threshold: " << trigger.threshold << "\n";
        std::cout << "  Current Value: " << std::fixed << std::setprecision(3) 
                  << trigger.current_value << "\n";
        std::cout << "  Recommendation: " << trigger.recommendation << "\n\n";
    });
    
    std::cout << "Simulating quality degradation...\n";
    
    // Simulate decreasing quality
    for (int i = 0; i < 12; i++) {
        QualityMetric metric;
        metric.type = MetricType::FAITHFULNESS;
        metric.value = 0.85 - (i * 0.02);  // Gradually decrease
        metric.timestamp = std::chrono::system_clock::now();
        
        cl_client->logMetric(metric);
        
        std::cout << "Iteration " << (i + 1) << ": Faithfulness " 
                  << std::fixed << std::setprecision(3) << metric.value << "\n";
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

/**
 * @brief Example 3: Monitoring Multiple Dimensions
 */
void example3_multi_dimension_monitoring() {
    std::cout << "\n=== Example 3: Multi-Dimension Monitoring ===\n\n";
    
    auto cl_client = std::make_shared<ContinuousLearningClient>();
    auto pipeline = QualityControlFactory::createBasic();
    
    // Simulate various quality scenarios
    struct Scenario {
        std::string name;
        double faithfulness;
        double relevance;
        double completeness;
    };
    
    std::vector<Scenario> scenarios = {
        {"Good Quality", 0.90, 0.85, 0.88},
        {"Low Faithfulness", 0.65, 0.85, 0.80},
        {"Low Relevance", 0.85, 0.65, 0.80},
        {"Overall Low", 0.68, 0.70, 0.65}
    };
    
    for (const auto& scenario : scenarios) {
        std::cout << "\nScenario: " << scenario.name << "\n";
        
        // Create result with specific scores
        QCResult result;
        result.faithfulness_score = scenario.faithfulness;
        result.relevance_score = scenario.relevance;
        result.completeness_score = scenario.completeness;
        result.coherence_score = 0.80;
        result.overall_score = (scenario.faithfulness + scenario.relevance + 
                               scenario.completeness + 0.80) / 4.0;
        result.mode = QCMode::BALANCED;
        result.latency = std::chrono::milliseconds(100);
        result.decision = result.overall_score >= 0.75 ? QCDecision::ACCEPT : QCDecision::WARN;
        
        cl_client->logQCResult(result);
        
        std::cout << "  Faithfulness: " << std::fixed << std::setprecision(3) 
                  << scenario.faithfulness << "\n";
        std::cout << "  Relevance: " << scenario.relevance << "\n";
        std::cout << "  Completeness: " << scenario.completeness << "\n";
        std::cout << "  Overall: " << result.overall_score << "\n";
        
        // Get recommendation
        auto recommendation = cl_utils::generateRecommendation(result);
        std::cout << "  Recommendation: " << recommendation << "\n";
    }
    
    auto stats = cl_client->getStatistics();
    std::cout << "\n📊 Statistics:\n";
    std::cout << "  Total metrics: " << stats.metrics_logged << "\n";
    std::cout << "  Triggers fired: " << stats.triggers_fired << "\n";
}

/**
 * @brief Example 4: Integration with Quality Control Pipeline
 */
void example4_pipeline_integration() {
    std::cout << "\n=== Example 4: Pipeline Integration ===\n\n";
    
    // Create pipeline with continuous learning enabled
    QualityControlPipeline::Config pipeline_config;
    pipeline_config.log_to_continuous_learning = true;
    pipeline_config.cl_endpoint = "http://localhost:8080/metrics";
    
    QualityControlPipeline pipeline(pipeline_config);
    
    // The pipeline will automatically log metrics
    std::vector<RetrievedDocument> documents = {
        {"doc1", "Sample document content.", 0.9, {}}
    };
    
    std::cout << "Running quality control with automatic CL logging...\n\n";
    
    for (int i = 0; i < 3; i++) {
        std::string query = "Question " + std::to_string(i + 1);
        std::string answer = "Answer " + std::to_string(i + 1) + " with content.";
        
        auto result = pipeline.runQualityControl(query, documents, answer);
        
        std::cout << "Evaluation " << (i + 1) << ":\n";
        std::cout << "  Score: " << std::fixed << std::setprecision(3) 
                  << result.overall_score << "\n";
        std::cout << "  Decision: ";
        switch (result.decision) {
            case QCDecision::ACCEPT: std::cout << "ACCEPT\n"; break;
            case QCDecision::REJECT: std::cout << "REJECT\n"; break;
            case QCDecision::RETRY: std::cout << "RETRY\n"; break;
            case QCDecision::WARN: std::cout << "WARN\n"; break;
        }
        std::cout << "  ✓ Logged to continuous learning\n\n";
    }
}

/**
 * @brief Example 5: Custom Trigger Actions
 */
void example5_custom_actions() {
    std::cout << "\n=== Example 5: Custom Trigger Actions ===\n\n";
    
    ContinuousLearningClient::Config cl_config;
    cl_config.faithfulness_threshold = 0.75;
    cl_config.metric_window_size = 5;
    
    auto cl_client = std::make_shared<ContinuousLearningClient>(cl_config);
    
    // Set up custom actions for different trigger types
    cl_client->setTriggerCallback([](const OptimizationTrigger& trigger) {
        std::cout << "\n🔧 Taking Action for: " << trigger.trigger_type << "\n";
        
        if (trigger.trigger_type == "low_faithfulness") {
            std::cout << "  Action: Optimizing retrieval system\n";
            std::cout << "  - Reindex documents with better embeddings\n";
            std::cout << "  - Adjust ranking parameters\n";
            std::cout << "  - Increase top-k results\n";
        } else if (trigger.trigger_type == "low_relevance") {
            std::cout << "  Action: Optimizing prompts\n";
            std::cout << "  - Run prompt optimizer\n";
            std::cout << "  - Generate alternative prompts\n";
            std::cout << "  - A/B test new versions\n";
        } else if (trigger.trigger_type == "low_overall_quality") {
            std::cout << "  Action: Triggering LoRA fine-tuning\n";
            std::cout << "  - Collect recent examples\n";
            std::cout << "  - Start fine-tuning job\n";
            std::cout << "  - Schedule evaluation after training\n";
        }
        std::cout << "\n";
    });
    
    // Simulate different quality issues
    std::cout << "Simulating low faithfulness scenario...\n";
    for (int i = 0; i < 6; i++) {
        QualityMetric metric;
        metric.type = MetricType::FAITHFULNESS;
        metric.value = 0.68;  // Below threshold
        metric.timestamp = std::chrono::system_clock::now();
        cl_client->logMetric(metric);
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "Simulating low relevance scenario...\n";
    for (int i = 0; i < 6; i++) {
        QualityMetric metric;
        metric.type = MetricType::RELEVANCE;
        metric.value = 0.65;  // Below threshold
        metric.timestamp = std::chrono::system_clock::now();
        cl_client->logMetric(metric);
    }
}

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Continuous Learning Integration - Examples               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    
    try {
        example1_basic_integration();
        example2_trigger_detection();
        example3_multi_dimension_monitoring();
        example4_pipeline_integration();
        example5_custom_actions();
        
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "✓ All examples completed successfully!\n";
        std::cout << std::string(60, '=') << "\n\n";
        
        std::cout << "Key Features Demonstrated:\n";
        std::cout << "1. Automatic metric logging from quality control\n";
        std::cout << "2. Trigger detection for optimization needs\n";
        std::cout << "3. Multi-dimension quality monitoring\n";
        std::cout << "4. Seamless pipeline integration\n";
        std::cout << "5. Custom actions based on trigger types\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
