/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            simple_qc_integration_example.cpp                  ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     290                                            ║
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
 * @file simple_qc_integration_example.cpp
 * @brief Simple example showing how to integrate quality control into RAG pipeline
 */

#include "rag/quality_control_factory.h"
#include "rag/rag_judge.h"
#include <iostream>
#include <iomanip>

using namespace themis::rag::judge;

/**
 * @brief Simulate a simple RAG pipeline with quality control
 */
class SimpleRAGPipeline {
private:
    std::unique_ptr<QualityControlPipeline> qc_pipeline_;
    
public:
    SimpleRAGPipeline() {
        // Create a basic quality control pipeline
        // Uses heuristic fallbacks - no model files needed
        qc_pipeline_ = QualityControlFactory::createBasic(QCMode::BALANCED);
        
        std::cout << "✓ RAG Pipeline initialized with quality control\n";
    }
    
    /**
     * @brief Generate answer with automatic quality control
     */
    std::string generateWithQC(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents
    ) {
        // Step 1: Generate answer (simulated)
        std::string answer = generateAnswer(query, documents);
        
        std::cout << "\n=== Quality Control ===\n";
        std::cout << "Query: " << query << "\n";
        std::cout << "Generated Answer: " << answer << "\n\n";
        
        // Step 2: Run quality control
        auto qc_result = qc_pipeline_->runQualityControl(query, documents, answer);
        
        // Step 3: Display results
        std::cout << "Quality Scores:\n";
        std::cout << "  Overall:      " << std::fixed << std::setprecision(3) 
                  << qc_result.overall_score << "\n";
        std::cout << "  Faithfulness: " << qc_result.faithfulness_score << "\n";
        std::cout << "  Relevance:    " << qc_result.relevance_score << "\n";
        std::cout << "  Completeness: " << qc_result.completeness_score << "\n";
        std::cout << "  Coherence:    " << qc_result.coherence_score << "\n";
        std::cout << "\nDecision: ";
        
        // Step 4: Handle decision
        switch (qc_result.decision) {
            case QCDecision::ACCEPT:
                std::cout << "✓ ACCEPT (quality passed)\n";
                return answer;
                
            case QCDecision::REJECT:
                std::cout << "✗ REJECT (quality too low)\n";
                std::cout << "Action: Regenerating with better retrieval...\n";
                return regenerateAnswer(query, documents);
                
            case QCDecision::RETRY:
                std::cout << "⟳ RETRY (quality marginal)\n";
                std::cout << "Action: Retrying with different parameters...\n";
                return retryGeneration(query, documents);
                
            case QCDecision::WARN:
                std::cout << "⚠ WARN (quality acceptable with issues)\n";
                std::cout << "Action: Using answer but logging warning\n";
                logWarning(qc_result);
                return answer;
        }
        
        return answer;
    }
    
private:
    std::string generateAnswer(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents
    ) {
        // Simulated answer generation
        if (!documents.empty()) {
            return "Based on the retrieved information: " + documents[0].content;
        }
        return "I don't have enough information to answer that question.";
    }
    
    std::string regenerateAnswer(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents
    ) {
        // Simulated regeneration with better retrieval
        return "Regenerated answer with improved retrieval and context.";
    }
    
    std::string retryGeneration(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents
    ) {
        // Simulated retry with different parameters
        return "Retried generation with adjusted parameters.";
    }
    
    void logWarning(const QCResult& result) {
        std::cout << "Warning logged: Score " << result.overall_score 
                  << " below optimal threshold\n";
    }
};

/**
 * @brief Example 1: Basic integration
 */
void example1_basic_integration() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Example 1: Basic RAG Pipeline with Quality Control\n";
    std::cout << std::string(60, '=') << "\n";
    
    SimpleRAGPipeline pipeline;
    
    // Prepare test data
    std::string query = "What is the capital of France?";
    std::vector<RetrievedDocument> documents = {
        {"doc1", "Paris is the capital of France.", 0.95, {}},
        {"doc2", "France is a country in Western Europe.", 0.85, {}}
    };
    
    // Generate answer with automatic quality control
    std::string final_answer = pipeline.generateWithQC(query, documents);
    
    std::cout << "\nFinal Answer: " << final_answer << "\n";
}

/**
 * @brief Example 2: Using factory methods for different scenarios
 */
void example2_factory_methods() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Example 2: Factory Methods for Different Scenarios\n";
    std::cout << std::string(60, '=') << "\n";
    
    // Scenario 1: Real-time application (fast mode)
    std::cout << "\n1. Lightweight pipeline for real-time:\n";
    auto lightweight = QualityControlFactory::createLightweight();
    std::cout << "   ✓ Created with Fast mode (<50ms target)\n";
    
    // Scenario 2: Standard production (balanced mode)
    std::cout << "\n2. Basic pipeline for standard production:\n";
    auto basic = QualityControlFactory::createBasic(QCMode::BALANCED);
    std::cout << "   ✓ Created with Balanced mode (<500ms target)\n";
    
    // Scenario 3: Batch processing (thorough mode)
    std::cout << "\n3. Comprehensive pipeline for batch processing:\n";
    QualityControlFactory::SetupConfig config;
    config.default_mode = QCMode::THOROUGH;
    config.enable_nli = true;
    config.enable_geval = true;
    auto comprehensive = QualityControlFactory::createComprehensive(config);
    std::cout << "   ✓ Created with Thorough mode (<2s target)\n";
}

/**
 * @brief Example 3: Configuring RAG Judge with quality control
 */
void example3_rag_judge_configuration() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Example 3: RAG Judge with Quality Control Features\n";
    std::cout << std::string(60, '=') << "\n";
    
    // Development configuration
    std::cout << "\n1. Development configuration:\n";
    auto dev_config = RAGJudgeQCConfigurator::getDevelopmentConfig();
    std::cout << "   Mode: Balanced\n";
    std::cout << "   NLI Verification: " << (dev_config.use_nli_verifier ? "✓" : "✗") << "\n";
    std::cout << "   G-Eval Scoring: " << (dev_config.use_geval_scoring ? "✓" : "✗") << "\n";
    std::cout << "   Quality Threshold: " << dev_config.quality_threshold << "\n";
    
    // Production configuration
    std::cout << "\n2. Production configuration:\n";
    auto prod_config = RAGJudgeQCConfigurator::getProductionConfig();
    std::cout << "   Mode: Thorough\n";
    std::cout << "   NLI Verification: " << (prod_config.use_nli_verifier ? "✓" : "✗") << "\n";
    std::cout << "   G-Eval Scoring: " << (prod_config.use_geval_scoring ? "✓" : "✗") << "\n";
    std::cout << "   Quality Threshold: " << prod_config.quality_threshold << "\n";
    std::cout << "   Ethical Evaluation: " << (prod_config.enable_ethical_evaluation ? "✓" : "✗") << "\n";
    
    // Custom configuration
    std::cout << "\n3. Custom configuration:\n";
    auto custom_config = RAGJudgeQCConfigurator::configure(
        true,   // enable_nli
        false,  // enable_geval (skip for speed)
        false   // enable_full_pipeline
    );
    std::cout << "   ✓ Custom configuration created\n";
}

/**
 * @brief Example 4: Monitoring and statistics
 */
void example4_monitoring() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Example 4: Quality Control Monitoring\n";
    std::cout << std::string(60, '=') << "\n";
    
    auto pipeline = QualityControlFactory::createBasic();
    
    // Simulate several evaluations
    std::cout << "\nRunning 5 quality checks...\n";
    
    std::vector<RetrievedDocument> docs = {
        {"doc1", "Sample document content.", 0.9, {}}
    };
    
    for (int i = 1; i <= 5; i++) {
        std::string query = "Question " + std::to_string(i);
        std::string answer = "Answer " + std::to_string(i);
        
        auto result = pipeline->runQualityControl(query, docs, answer);
        std::cout << "  Check " << i << ": Score " << std::fixed 
                  << std::setprecision(3) << result.overall_score << "\n";
    }
    
    // Get statistics
    auto stats = pipeline->getStatistics();
    
    std::cout << "\nQuality Control Statistics:\n";
    std::cout << "  Total Evaluations: " << stats.total_evaluations << "\n";
    std::cout << "  Average Score: " << std::fixed << std::setprecision(3) 
              << stats.avg_score << "\n";
    std::cout << "  Average Latency: " << stats.avg_latency_ms << " ms\n";
    std::cout << "  Accepted: " << stats.accepted << "\n";
    std::cout << "  Rejected: " << stats.rejected << "\n";
}

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Quality Control Integration - Simple Examples            ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    
    try {
        example1_basic_integration();
        example2_factory_methods();
        example3_rag_judge_configuration();
        example4_monitoring();
        
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "✓ All examples completed successfully!\n";
        std::cout << std::string(60, '=') << "\n\n";
        
        std::cout << "Next Steps:\n";
        std::cout << "1. Try quality_control_demo.cpp for more comprehensive examples\n";
        std::cout << "2. Review docs/quality-control-usage-guide.md for detailed usage\n";
        std::cout << "3. Integrate quality control into your RAG pipeline\n";
        std::cout << "4. Configure continuous learning for automatic optimization\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
