/**
 * @file rag_llm_integration_example.cpp
 * @brief Example demonstrating RAG Judge with LLM integration
 * 
 * This example shows how to:
 * 1. Set up InferenceEngineEnhanced
 * 2. Connect it to RAG Judge components
 * 3. Evaluate RAG outputs with LLM-as-Judge
 * 4. Use NLI for faithfulness verification
 * 5. Run quality control pipeline
 */

#include "rag/quality_control_pipeline.h"
#include "rag/llm_judge_client.h"
#include "rag/nli_faithfulness_verifier.h"
#include "rag/rag_judge.h"
#include "llm/inference_engine_enhanced.h"
#include <iostream>
#include <memory>

using namespace themis::rag::judge;
using namespace themis::llm;

int main() {
    std::cout << "=== RAG Judge LLM Integration Example ===" << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // 1. Set up InferenceEngineEnhanced
    // ========================================================================
    std::cout << "1. Setting up InferenceEngineEnhanced..." << std::endl;
    
    InferenceEngineEnhanced::Config engine_config;
    engine_config.enable_context_caching = true;
    engine_config.enable_batch_processing = true;
    engine_config.max_cache_entries = 1000;
    
    auto engine = std::make_shared<InferenceEngineEnhanced>(engine_config);
    
    // In production, register actual LLM model plugin
    // engine->registerModel("llama2-7b", llm_plugin);
    // engine->start();
    
    std::cout << "   ✓ Engine configured" << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // 2. Set up LLM Judge Client
    // ========================================================================
    std::cout << "2. Setting up LLM Judge Client..." << std::endl;
    
    LLMJudgeClient::Config client_config;
    client_config.temperature = 0.3;
    client_config.max_tokens = 1024;
    client_config.enable_caching = true;
    
    auto llm_client = std::make_shared<LLMJudgeClient>(client_config);
    // Connect client to engine (when engine has models)
    // llm_client->setEngine(engine);
    
    std::cout << "   ✓ LLM client configured" << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // 3. Set up NLI Faithfulness Verifier
    // ========================================================================
    std::cout << "3. Setting up NLI Verifier..." << std::endl;
    
    NLIFaithfulnessVerifier::Config nli_config;
    nli_config.use_heuristic_fallback = true;  // Use heuristic until model loaded
    nli_config.entailment_threshold = 0.7;
    
    auto nli_verifier = std::make_shared<NLIFaithfulnessVerifier>(nli_config);
    
    std::cout << "   ✓ NLI verifier ready: " << nli_verifier->getModelInfo() << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // 4. Set up Quality Control Pipeline
    // ========================================================================
    std::cout << "4. Setting up Quality Control Pipeline..." << std::endl;
    
    QualityControlPipeline::Config pipeline_config;
    pipeline_config.max_evaluation_time_ms = 500.0;  // Target: <500ms
    pipeline_config.min_confidence = 0.6;
    pipeline_config.enable_adaptive_sampling = true;
    pipeline_config.enable_result_caching = true;
    
    QualityControlPipeline pipeline(pipeline_config);
    
    // Inject components
    pipeline.setLLMClient(llm_client);
    pipeline.setNLIVerifier(nli_verifier);
    
    std::cout << "   ✓ Pipeline configured with <500ms target" << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // 5. Prepare test data
    // ========================================================================
    std::cout << "5. Preparing test data..." << std::endl;
    
    std::string query = "What is the capital of France and what is it famous for?";
    
    std::vector<RetrievedDocument> documents = {
        {"doc1", "Paris is the capital and largest city of France. It has a population of over 2 million people.", 0.95},
        {"doc2", "The Eiffel Tower is a famous landmark in Paris, France. It was built in 1889 for the World's Fair.", 0.88},
        {"doc3", "Paris is known for its art, culture, and cuisine. The Louvre Museum is one of the world's largest museums.", 0.82}
    };
    
    std::string answer = "Paris is the capital of France. It is famous for the Eiffel Tower, "
                        "which was built in 1889, and the Louvre Museum. Paris is known for "
                        "its rich cultural heritage and cuisine.";
    
    std::cout << "   Query: " << query << std::endl;
    std::cout << "   Documents: " << documents.size() << " retrieved" << std::endl;
    std::cout << "   Answer length: " << answer.length() << " characters" << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // 6. Run evaluation
    // ========================================================================
    std::cout << "6. Running evaluation..." << std::endl;
    std::cout << "   (This will use heuristic NLI and stub LLM responses)" << std::endl;
    
    auto result = pipeline.evaluate(query, documents, answer);
    
    std::cout << std::endl;
    std::cout << "=== Evaluation Results ===" << std::endl;
    std::cout << std::endl;
    
    // Overall score
    std::cout << "Overall Score: " << result.evaluation.overall_score << std::endl;
    std::cout << "Confidence: " << result.evaluation.confidence << std::endl;
    std::cout << std::endl;
    
    // Dimension scores
    std::cout << "Dimension Scores:" << std::endl;
    for (const auto& [dimension, score] : result.evaluation.dimension_scores) {
        std::cout << "  - " << dimension << ": " << score << std::endl;
    }
    std::cout << std::endl;
    
    // Performance metrics
    std::cout << "Performance Metrics:" << std::endl;
    std::cout << "  - Total time: " << result.metrics.total_time_ms << "ms" << std::endl;
    std::cout << "  - Met time target (<500ms): " 
              << (result.metrics.met_time_target ? "YES" : "NO") << std::endl;
    std::cout << "  - LLM calls: " << result.metrics.llm_calls_count << std::endl;
    std::cout << "  - NLI calls: " << result.metrics.nli_calls_count << std::endl;
    std::cout << std::endl;
    
    // Quality checks
    std::cout << "Quality Checks:" << std::endl;
    for (const auto& check : result.quality_checks) {
        std::cout << "  - [" << (check.passed ? "PASS" : "FAIL") << "] "
                  << check.dimension << ": " << check.reason << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "Overall Quality: " 
              << (result.overall_quality_passed ? "PASSED" : "FAILED") << std::endl;
    std::cout << std::endl;
    
    // Quality summary
    std::cout << "Quality Summary:" << std::endl;
    std::cout << result.quality_summary << std::endl;
    
    // ========================================================================
    // 7. Demonstrate NLI verification directly
    // ========================================================================
    std::cout << std::endl;
    std::cout << "=== Direct NLI Verification Example ===" << std::endl;
    std::cout << std::endl;
    
    std::string premise = "Paris is the capital of France. It has over 2 million people.";
    std::string hypothesis1 = "Paris is the capital of France.";
    std::string hypothesis2 = "Paris has a large population.";
    std::string hypothesis3 = "Paris is in Germany.";
    
    auto nli_result1 = nli_verifier->verify(premise, hypothesis1);
    auto nli_result2 = nli_verifier->verify(premise, hypothesis2);
    auto nli_result3 = nli_verifier->verify(premise, hypothesis3);
    
    std::cout << "Premise: " << premise << std::endl;
    std::cout << std::endl;
    
    std::cout << "Hypothesis 1: " << hypothesis1 << std::endl;
    std::cout << "  Prediction: " << (nli_result1.prediction == NLIPrediction::ENTAILMENT ? "ENTAILMENT" :
                                      nli_result1.prediction == NLIPrediction::NEUTRAL ? "NEUTRAL" : "CONTRADICTION") << std::endl;
    std::cout << "  Confidence: " << nli_result1.confidence << std::endl;
    std::cout << std::endl;
    
    std::cout << "Hypothesis 2: " << hypothesis2 << std::endl;
    std::cout << "  Prediction: " << (nli_result2.prediction == NLIPrediction::ENTAILMENT ? "ENTAILMENT" :
                                      nli_result2.prediction == NLIPrediction::NEUTRAL ? "NEUTRAL" : "CONTRADICTION") << std::endl;
    std::cout << "  Confidence: " << nli_result2.confidence << std::endl;
    std::cout << std::endl;
    
    std::cout << "Hypothesis 3: " << hypothesis3 << std::endl;
    std::cout << "  Prediction: " << (nli_result3.prediction == NLIPrediction::ENTAILMENT ? "ENTAILMENT" :
                                      nli_result3.prediction == NLIPrediction::NEUTRAL ? "NEUTRAL" : "CONTRADICTION") << std::endl;
    std::cout << "  Confidence: " << nli_result3.confidence << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // 8. Summary
    // ========================================================================
    std::cout << "=== Summary ===" << std::endl;
    std::cout << std::endl;
    std::cout << "This example demonstrated:" << std::endl;
    std::cout << "  1. Setting up InferenceEngineEnhanced for LLM inference" << std::endl;
    std::cout << "  2. Creating LLMJudgeClient to manage LLM interactions" << std::endl;
    std::cout << "  3. Using NLI verifier for faithfulness checking" << std::endl;
    std::cout << "  4. Running quality control pipeline with <500ms target" << std::endl;
    std::cout << "  5. Direct NLI verification for claim entailment" << std::endl;
    std::cout << std::endl;
    std::cout << "In production, you would:" << std::endl;
    std::cout << "  - Register actual LLM model plugins with the engine" << std::endl;
    std::cout << "  - Load actual NLI models (RoBERTa-large-MNLI, etc.)" << std::endl;
    std::cout << "  - Configure model paths and GPU settings" << std::endl;
    std::cout << "  - Enable batch processing for better throughput" << std::endl;
    std::cout << std::endl;
    
    return 0;
}
