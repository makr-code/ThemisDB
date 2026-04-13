/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            quality_control_demo.cpp                           ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:19:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     308                                            ║
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
 * @file quality_control_demo.cpp
 * @brief Demonstration of the quality control pipeline for RAG
 * 
 * Shows how to:
 * - Set up quality control components
 * - Run multi-stage quality checks
 * - Handle quality gate failures
 * - Integrate with continuous learning
 */

#include <iostream>
#include <memory>
#include <iomanip>
#include "rag/quality_control_pipeline.h"
#include "rag/llm_judge_client.h"
#include "rag/geval_evaluator.h"
#include "rag/nli_faithfulness_verifier.h"
#include "rag/rag_judge.h"

using namespace themis::rag::judge;

void printHeader(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

void printDimensionScores(const QualityCheckResult& result) {
    std::cout << "Dimension Scores:\n";
    std::cout << std::string(50, '-') << "\n";
    
    for (const auto& score : result.dimension_scores) {
        std::cout << std::left << std::setw(20) << score.dimension << ": "
                  << std::fixed << std::setprecision(3) << score.score
                  << " (confidence: " << score.confidence << ")"
                  << " [" << score.method << "]\n";
    }
    std::cout << std::string(50, '-') << "\n";
    std::cout << "Overall Score: " << std::fixed << std::setprecision(3) 
              << result.overall_score << "\n\n";
}

void printTiming(const QualityCheckResult& result) {
    std::cout << "Stage Timing:\n";
    std::cout << std::string(50, '-') << "\n";
    std::cout << "Fast Screening:      " << result.fast_stage_time.count() << "ms\n";
    std::cout << "Balanced Evaluation: " << result.balanced_stage_time.count() << "ms\n";
    std::cout << "Thorough Verification: " << result.thorough_stage_time.count() << "ms\n";
    std::cout << "Total:               " << result.total_time.count() << "ms\n";
    std::cout << std::string(50, '-') << "\n\n";
}

void printStatus(const QualityCheckResult& result) {
    std::cout << "Quality Gate Status: ";
    
    switch (result.status) {
        case QualityGateStatus::PASSED:
            std::cout << "✓ PASSED\n";
            break;
        case QualityGateStatus::FAILED:
            std::cout << "✗ FAILED\n";
            break;
        case QualityGateStatus::RETRY_NEEDED:
            std::cout << "⚠ RETRY NEEDED\n";
            break;
        case QualityGateStatus::ESCALATE:
            std::cout << "⚠ ESCALATE (Human Review)\n";
            break;
    }
    
    if (!result.failure_reasons.empty()) {
        std::cout << "\nFailure Reasons:\n";
        for (const auto& reason : result.failure_reasons) {
            std::cout << "  - " << reason << "\n";
        }
    }
    
    if (!result.improvement_suggestions.empty()) {
        std::cout << "\nImprovement Suggestions:\n";
        for (const auto& suggestion : result.improvement_suggestions) {
            std::cout << "  - " << suggestion << "\n";
        }
    }
    
    std::cout << "\n";
}

int main() {
    std::cout << "=== Quality Control Pipeline Demo ===\n\n";
    
    // ========================================================================
    // Example 1: Fast Screening Mode
    // ========================================================================
    printHeader("Example 1: Fast Screening Mode (<50ms)");
    
    std::cout << "Creating fast pipeline (screening only)...\n";
    auto fast_pipeline = QualityPipelineFactory::createFast();
    
    // Prepare test data
    std::string query = "What is the capital of France?";
    std::string answer = "The capital of France is Paris, a beautiful city on the Seine River.";
    
    std::vector<RetrievedDocument> documents;
    RetrievedDocument doc1;
    doc1.id = "doc1";
    doc1.content = "Paris is the capital and most populous city of France. "
                   "It is situated on the Seine River in northern France.";
    doc1.similarity_score = 0.95;
    documents.push_back(doc1);
    
    std::cout << "Running fast screening...\n";
    auto result1 = fast_pipeline->runQualityControl(query, answer, documents);
    
    printDimensionScores(result1);
    printTiming(result1);
    printStatus(result1);
    
    // ========================================================================
    // Example 2: Balanced Mode
    // ========================================================================
    printHeader("Example 2: Balanced Mode (<500ms)");
    
    std::cout << "Creating balanced pipeline (fast + balanced stages)...\n";
    auto balanced_pipeline = QualityPipelineFactory::createBalanced();
    
    std::cout << "Running balanced evaluation...\n";
    auto result2 = balanced_pipeline->runQualityControl(query, answer, documents);
    
    printDimensionScores(result2);
    printTiming(result2);
    printStatus(result2);
    
    // ========================================================================
    // Example 3: Thorough Verification Mode
    // ========================================================================
    printHeader("Example 3: Thorough Verification Mode (<2s)");
    
    std::cout << "Creating thorough pipeline (all stages + NLI)...\n";
    auto thorough_pipeline = QualityPipelineFactory::createThorough();
    
    std::cout << "Running thorough verification...\n";
    auto result3 = thorough_pipeline->runQualityControl(query, answer, documents);
    
    printDimensionScores(result3);
    printTiming(result3);
    printStatus(result3);
    
    // ========================================================================
    // Example 4: Production Mode with Learning Feedback
    // ========================================================================
    printHeader("Example 4: Production Mode with Learning Feedback");
    
    std::cout << "Creating production pipeline (all stages + learning)...\n";
    auto prod_pipeline = QualityPipelineFactory::createProduction();
    
    // Set up callbacks
    prod_pipeline->setFailureCallback([](const QualityCheckResult& result) {
        std::cout << "[CALLBACK] Quality gate failed, triggering retry...\n";
    });
    
    prod_pipeline->setLearningCallback([](const std::string& query, 
                                          const QualityCheckResult& result) {
        std::cout << "[CALLBACK] Sending feedback to learning system...\n";
        std::cout << "  Query: " << query.substr(0, 30) << "...\n";
        std::cout << "  Score: " << std::fixed << std::setprecision(3) 
                  << result.overall_score << "\n";
    });
    
    std::cout << "Running production quality control...\n";
    auto result4 = prod_pipeline->runQualityControl(query, answer, documents);
    
    printDimensionScores(result4);
    printTiming(result4);
    printStatus(result4);
    
    // ========================================================================
    // Example 5: Testing with Low-Quality Answer
    // ========================================================================
    printHeader("Example 5: Low-Quality Answer Detection");
    
    std::string bad_query = "What is quantum computing?";
    std::string bad_answer = "Quantum computing is about computers that use quantum mechanics. "
                            "It was invented in 1985 by Steve Jobs at Apple. "
                            "Quantum computers can solve any problem instantly.";
    
    std::vector<RetrievedDocument> bad_documents;
    RetrievedDocument bad_doc;
    bad_doc.id = "doc2";
    bad_doc.content = "Quantum computing is a type of computation that harnesses "
                      "the collective properties of quantum states, such as superposition "
                      "and entanglement, to perform calculations. The concept was pioneered "
                      "by physicists like Richard Feynman and David Deutsch in the 1980s.";
    bad_doc.similarity_score = 0.85;
    bad_documents.push_back(bad_doc);
    
    std::cout << "Testing with factually incorrect answer...\n";
    auto result5 = prod_pipeline->runQualityControl(bad_query, bad_answer, bad_documents);
    
    printDimensionScores(result5);
    printTiming(result5);
    printStatus(result5);
    
    // ========================================================================
    // Example 6: NLI Faithfulness Verification
    // ========================================================================
    printHeader("Example 6: Direct NLI Faithfulness Verification");
    
    std::cout << "Creating standalone NLI verifier...\n";
    NLIFaithfulnessVerifier::Config nli_config;
    nli_config.entailment_threshold = 0.7;
    nli_config.max_claims = 10;
    
    auto nli_verifier = std::make_shared<NLIFaithfulnessVerifier>(nli_config);
    
    std::vector<std::pair<std::string, std::string>> doc_pairs;
    doc_pairs.emplace_back("doc1", doc1.content);
    
    std::cout << "Verifying answer faithfulness with NLI...\n";
    auto nli_result = nli_verifier->verify(answer, doc_pairs);
    
    std::cout << "NLI Verification Results:\n";
    std::cout << std::string(50, '-') << "\n";
    std::cout << "Faithfulness Score: " << std::fixed << std::setprecision(3) 
              << nli_result.faithfulness_score << "\n";
    std::cout << "Is Faithful: " << (nli_result.is_faithful ? "Yes" : "No") << "\n";
    std::cout << "Total Claims: " << nli_result.total_claims << "\n";
    std::cout << "Supported: " << nli_result.supported_claims << "\n";
    std::cout << "Partially Supported: " << nli_result.partially_supported_claims << "\n";
    std::cout << "Unsupported: " << nli_result.unsupported_claims << "\n";
    std::cout << "Contradicted: " << nli_result.contradicted_claims << "\n";
    std::cout << "Verification Time: " << nli_result.verification_time.count() << "ms\n";
    std::cout << std::string(50, '-') << "\n\n";
    
    // ========================================================================
    // Example 7: G-Eval Probabilistic Scoring
    // ========================================================================
    printHeader("Example 7: G-Eval Probabilistic Scoring");
    
    std::cout << "Creating G-Eval evaluator...\n";
    GEvalEvaluator::Config geval_config;
    geval_config.num_samples = 3;
    geval_config.aggregation = AggregationMethod::MEAN;
    
    auto geval = std::make_shared<GEvalEvaluator>(geval_config);
    
    std::cout << "Running G-Eval for faithfulness...\n";
    auto geval_result = geval->evaluate(query, answer, doc_pairs, "faithfulness");
    
    std::cout << "G-Eval Results:\n";
    std::cout << std::string(50, '-') << "\n";
    std::cout << "Dimension: " << geval_result.dimension << "\n";
    std::cout << "G-Eval Score: " << std::fixed << std::setprecision(3) 
              << geval_result.geval_score << "\n";
    std::cout << "Confidence: " << geval_result.confidence << "\n";
    std::cout << "Variance: " << geval_result.variance << "\n";
    std::cout << "\nToken Probabilities (Levels 1-5):\n";
    for (size_t i = 0; i < geval_result.token_probabilities.size(); i++) {
        std::cout << "  Level " << (i+1) << ": " 
                  << std::fixed << std::setprecision(3)
                  << geval_result.token_probabilities[i] << "\n";
    }
    std::cout << std::string(50, '-') << "\n\n";
    
    // ========================================================================
    // Summary
    // ========================================================================
    printHeader("Summary");
    
    std::cout << "Quality Control Pipeline Features Demonstrated:\n";
    std::cout << "✓ Fast screening mode (<50ms)\n";
    std::cout << "✓ Balanced evaluation mode (<500ms)\n";
    std::cout << "✓ Thorough verification mode (<2s)\n";
    std::cout << "✓ Production mode with learning feedback\n";
    std::cout << "✓ Low-quality answer detection\n";
    std::cout << "✓ NLI faithfulness verification\n";
    std::cout << "✓ G-Eval probabilistic scoring\n";
    std::cout << "✓ Multi-dimensional quality assessment\n";
    std::cout << "✓ Quality gate with automatic retry\n";
    std::cout << "✓ Integration with continuous learning\n";
    
    std::cout << "\n=== Demo Complete ===\n";
    
    return 0;
}
