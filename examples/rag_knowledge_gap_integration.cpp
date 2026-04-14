/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rag_knowledge_gap_integration.cpp                  ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   93.0/100                                       ║
    • Total Lines:     191                                            ║
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
 * @file rag_knowledge_gap_integration.cpp
 * @brief Example: Integration of Knowledge Gap Detector with LLM Inference
 * 
 * This example demonstrates how to use Phase 2 token probability tracking
 * from llama_wrapper with the Knowledge Gap Detector for real-time confidence
 * monitoring during RAG generation.
 */

#include "rag/knowledge_gap_detector.h"
#include "llm/llama_wrapper.h"
#include "llm/llm_plugin_interface.h"
#include <iostream>

using namespace themis::rag::knowledge_gap;
using namespace themis::llm;

int main() {
    std::cout << "=== RAG Knowledge Gap Detector Integration Example ===" << std::endl;
    std::cout << "Phase 2: Token Probability Tracking" << std::endl << std::endl;
    
    // 1. Initialize Knowledge Gap Detector with Phase 2 features
    KnowledgeGapConfig config;
    config.mode = DetectionMode::BALANCED;
    config.enable_token_probability = true;
    config.perplexity_threshold = 100.0;
    config.perplexity_window_size = 10;
    
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    // 2. Initialize LLM (llama_wrapper)
    LlamaWrapper::Config llm_config;
    llm_config.n_gpu_layers = 32;
    llm_config.n_ctx = 4096;
    
    auto llm = std::make_unique<LlamaWrapper>(llm_config);
    
    // Note: In a real scenario, you would load a model:
    // llm->loadModel("path/to/model.gguf", "model-id");
    
    // 3. Simulate retrieved documents (in real scenario, from VectorIndexManager)
    std::vector<RetrievedDocument> documents;
    
    RetrievedDocument doc1;
    doc1.id = "doc1";
    doc1.content = "Machine learning is a subset of artificial intelligence...";
    doc1.similarity_score = 0.85;
    documents.push_back(doc1);
    
    RetrievedDocument doc2;
    doc2.id = "doc2";
    doc2.content = "Deep learning uses neural networks with multiple layers...";
    doc2.similarity_score = 0.80;
    documents.push_back(doc2);
    
    RetrievedDocument doc3;
    doc3.id = "doc3";
    doc3.content = "Neural networks are inspired by biological neurons...";
    doc3.similarity_score = 0.78;
    documents.push_back(doc3);
    
    std::string query = "What is machine learning and how does it work?";
    
    // 4. Pre-generation gap detection
    std::cout << "Step 1: Pre-generation gap detection..." << std::endl;
    auto pre_result = detector->detectPreGeneration(query, documents);
    
    if (pre_result.gap_detected) {
        std::cout << "❌ Gap detected before generation!" << std::endl;
        std::cout << "   Type: " << static_cast<int>(pre_result.gap_type) << std::endl;
        std::cout << "   Explanation: " << pre_result.explanation << std::endl;
        std::cout << "   Recommendation: " << static_cast<int>(pre_result.recommendation) << std::endl;
        return 1;
    }
    
    std::cout << "✓ Pre-generation check passed" << std::endl;
    std::cout << "  Avg similarity: " << pre_result.avg_similarity_score << std::endl;
    std::cout << "  Coverage: " << pre_result.coverage_score << std::endl << std::endl;
    
    // 5. Generate answer with LLM (with token probability tracking)
    std::cout << "Step 2: Generating answer with LLM..." << std::endl;
    
    // Build prompt with context
    std::string prompt = "Based on the following information:\n\n";
    for (const auto& doc : documents) {
        prompt += "- " + doc.content + "\n";
    }
    prompt += "\nQuestion: " + query + "\n\nAnswer:";
    
    InferenceRequest request;
    request.prompt = prompt;
    request.max_tokens = 256;
    request.temperature = 0.7f;
    
    // Note: In real scenario, this would call the actual LLM
    // InferenceResponse response = llm->generate(request);
    
    // For this example, simulate a response with token probabilities
    InferenceResponse response;
    response.text = "Machine learning is a method of data analysis that automates analytical model building...";
    response.tokens_generated = 50;
    
    // Phase 2: Token probabilities are now automatically collected in llama_wrapper
    // Simulate token probabilities (in real case, these come from llama_wrapper)
    response.logprobs = {
        0.92f, 0.88f, 0.90f, 0.85f, 0.87f, 0.91f, 0.89f, 0.86f, 0.88f, 0.90f,
        0.84f, 0.86f, 0.88f, 0.87f, 0.85f, 0.89f, 0.90f, 0.88f, 0.87f, 0.86f,
        0.85f, 0.84f, 0.83f, 0.82f, 0.81f, 0.80f, 0.79f, 0.78f, 0.77f, 0.76f,
        0.75f, 0.74f, 0.73f, 0.72f, 0.71f, 0.70f, 0.69f, 0.68f, 0.67f, 0.66f,
        0.82f, 0.83f, 0.84f, 0.85f, 0.86f, 0.87f, 0.88f, 0.89f, 0.90f, 0.91f
    };
    
    std::cout << "✓ Generation complete" << std::endl;
    std::cout << "  Tokens generated: " << response.tokens_generated << std::endl;
    std::cout << "  Token probabilities collected: " << response.logprobs.size() << std::endl << std::endl;
    
    // 6. During-generation gap detection (using token probabilities)
    std::cout << "Step 3: During-generation gap detection..." << std::endl;
    
    // Build GenerationContext from LLM response
    GenerationContext gen_context;
    gen_context.token_probs.reserve(response.logprobs.size());
    for (float logprob : response.logprobs) {
        // Convert log probabilities to probabilities (if needed)
        // In this example, we already have probabilities
        gen_context.token_probs.push_back(logprob);
    }
    gen_context.generation_started = true;
    
    auto during_result = detector->detectDuringGeneration(query, documents, gen_context);
    
    if (during_result.gap_detected) {
        std::cout << "❌ Gap detected during generation!" << std::endl;
        std::cout << "   Type: " << static_cast<int>(during_result.gap_type) << std::endl;
        std::cout << "   Explanation: " << during_result.explanation << std::endl;
        std::cout << "   This indicates the LLM had low confidence in its generation." << std::endl;
        return 1;
    }
    
    std::cout << "✓ During-generation check passed" << std::endl;
    std::cout << "  Confidence score: " << during_result.confidence_score << std::endl << std::endl;
    
    // 7. Post-generation gap detection
    std::cout << "Step 4: Post-generation gap detection..." << std::endl;
    auto post_result = detector->detectPostGeneration(query, documents, response.text);
    
    if (post_result.gap_detected) {
        std::cout << "❌ Gap detected after generation!" << std::endl;
        std::cout << "   Type: " << static_cast<int>(post_result.gap_type) << std::endl;
        std::cout << "   Explanation: " << post_result.explanation << std::endl;
        return 1;
    }
    
    std::cout << "✓ Post-generation check passed" << std::endl << std::endl;
    
    // 8. Final result
    std::cout << "=== FINAL RESULT ===" << std::endl;
    std::cout << "All gap detection checks passed!" << std::endl;
    std::cout << "Answer: " << response.text << std::endl << std::endl;
    
    std::cout << "Phase 2 Integration Complete:" << std::endl;
    std::cout << "  ✓ Token probabilities automatically collected by llama_wrapper" << std::endl;
    std::cout << "  ✓ Perplexity calculated from token probabilities" << std::endl;
    std::cout << "  ✓ Real-time confidence monitoring during generation" << std::endl;
    std::cout << "  ✓ Anomaly detection for low-confidence regions" << std::endl;
    
    return 0;
}
