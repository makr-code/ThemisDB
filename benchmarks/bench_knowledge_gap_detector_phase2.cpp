/**
 * @file bench_knowledge_gap_detector_phase2.cpp
 * @brief Google Benchmark suite for Knowledge Gap Detector Phase 2
 * 
 * Benchmarks for LLM-based confidence metrics:
 * - Token probability tracking
 * - Perplexity calculation
 * - Sliding window analysis
 * - Outlier detection
 * - Self-consistency checking
 * - FLARE active retrieval
 * 
 * Performance Targets:
 * - Token probability: < 10ms overhead
 * - Perplexity calculation: < 5ms overhead
 * - Self-consistency: < 2s for 5 samples
 * - FLARE re-retrieval: < 500ms per round
 */

#include <benchmark/benchmark.h>
#include "rag/knowledge_gap_detector.h"
#include <vector>
#include <random>
#include <cmath>

using namespace themis::rag::knowledge_gap;

// ============================================================================
// Helper Functions for Benchmark Data Generation
// ============================================================================

std::vector<RetrievedDocument> createTestDocuments(size_t count, double avg_similarity = 0.85) {
    std::vector<RetrievedDocument> docs;
    docs.reserve(count);
    
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::normal_distribution<double> dist(avg_similarity, 0.05);
    
    for (size_t i = 0; i < count; ++i) {
        RetrievedDocument doc;
        doc.id = "doc_" + std::to_string(i);
        doc.content = "This is a test document with relevant information about the topic. "
                     "It contains multiple sentences with varying levels of detail. "
                     "The content is designed to test the knowledge gap detector's ability "
                     "to assess document quality and relevance.";
        doc.similarity_score = std::clamp(dist(rng), 0.0, 1.0);
        docs.push_back(doc);
    }
    
    return docs;
}

std::vector<double> createTokenProbabilities(size_t count, double mean = 0.8, double stddev = 0.1) {
    std::vector<double> probs;
    probs.reserve(count);
    
    std::mt19937 rng(42);
    std::normal_distribution<double> dist(mean, stddev);
    
    for (size_t i = 0; i < count; ++i) {
        probs.push_back(std::clamp(dist(rng), 0.01, 0.99));
    }
    
    return probs;
}

// ============================================================================
// Phase 2.1: Token Probability Tracking Benchmarks
// ============================================================================

static void BM_PerplexityCalculation_Small(benchmark::State& state) {
    KnowledgeGapConfig config;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(3);
    auto token_probs = createTokenProbabilities(50); // 50 tokens
    
    GenerationContext context;
    context.token_probs = token_probs;
    context.generation_started = true;
    
    for (auto _ : state) {
        auto result = detector->detectDuringGeneration("test query", docs, context);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("50 tokens");
}
BENCHMARK(BM_PerplexityCalculation_Small);

static void BM_PerplexityCalculation_Medium(benchmark::State& state) {
    KnowledgeGapConfig config;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(3);
    auto token_probs = createTokenProbabilities(200); // 200 tokens
    
    GenerationContext context;
    context.token_probs = token_probs;
    context.generation_started = true;
    
    for (auto _ : state) {
        auto result = detector->detectDuringGeneration("test query", docs, context);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("200 tokens");
}
BENCHMARK(BM_PerplexityCalculation_Medium);

static void BM_PerplexityCalculation_Large(benchmark::State& state) {
    KnowledgeGapConfig config;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(3);
    auto token_probs = createTokenProbabilities(512); // 512 tokens
    
    GenerationContext context;
    context.token_probs = token_probs;
    context.generation_started = true;
    
    for (auto _ : state) {
        auto result = detector->detectDuringGeneration("test query", docs, context);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("512 tokens");
}
BENCHMARK(BM_PerplexityCalculation_Large);

static void BM_SlidingWindowPerplexity_WindowSize10(benchmark::State& state) {
    KnowledgeGapConfig config;
    config.perplexity_window_size = 10;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(3);
    auto token_probs = createTokenProbabilities(100);
    
    GenerationContext context;
    context.token_probs = token_probs;
    context.generation_started = true;
    
    for (auto _ : state) {
        auto result = detector->detectDuringGeneration("test query", docs, context);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("window=10");
}
BENCHMARK(BM_SlidingWindowPerplexity_WindowSize10);

static void BM_SlidingWindowPerplexity_WindowSize20(benchmark::State& state) {
    KnowledgeGapConfig config;
    config.perplexity_window_size = 20;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(3);
    auto token_probs = createTokenProbabilities(100);
    
    GenerationContext context;
    context.token_probs = token_probs;
    context.generation_started = true;
    
    for (auto _ : state) {
        auto result = detector->detectDuringGeneration("test query", docs, context);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("window=20");
}
BENCHMARK(BM_SlidingWindowPerplexity_WindowSize20);

static void BM_OutlierDetection_NoOutliers(benchmark::State& state) {
    KnowledgeGapConfig config;
    config.outlier_zscore_threshold = 3.0;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(3);
    auto token_probs = createTokenProbabilities(100, 0.85, 0.05); // Low variance
    
    GenerationContext context;
    context.token_probs = token_probs;
    context.generation_started = true;
    
    for (auto _ : state) {
        auto result = detector->detectDuringGeneration("test query", docs, context);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("no outliers");
}
BENCHMARK(BM_OutlierDetection_NoOutliers);

static void BM_OutlierDetection_WithOutliers(benchmark::State& state) {
    KnowledgeGapConfig config;
    config.outlier_zscore_threshold = 3.0;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(3);
    auto token_probs = createTokenProbabilities(100, 0.85, 0.2); // High variance
    
    // Add some outliers
    token_probs[10] = 0.05;
    token_probs[30] = 0.02;
    token_probs[70] = 0.98;
    
    GenerationContext context;
    context.token_probs = token_probs;
    context.generation_started = true;
    
    for (auto _ : state) {
        auto result = detector->detectDuringGeneration("test query", docs, context);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("with outliers");
}
BENCHMARK(BM_OutlierDetection_WithOutliers);

static void BM_ConfidenceAggregation(benchmark::State& state) {
    KnowledgeGapConfig config;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(5);
    auto token_probs = createTokenProbabilities(200);
    
    GenerationContext context;
    context.token_probs = token_probs;
    context.generation_started = true;
    
    for (auto _ : state) {
        auto result = detector->detectDuringGeneration("test query", docs, context);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ConfidenceAggregation);

// ============================================================================
// Phase 2.2: Self-Consistency Check Benchmarks
// ============================================================================

static void BM_SelfConsistency_3Samples(benchmark::State& state) {
    KnowledgeGapConfig config;
    config.enable_self_consistency_check = true;
    config.self_consistency_samples = 3;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(5);
    std::string answer = "This is a generated answer with consistent information.";
    
    for (auto _ : state) {
        auto result = detector->detectPostGeneration("test query", docs, answer);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("3 samples");
}
BENCHMARK(BM_SelfConsistency_3Samples);

static void BM_SelfConsistency_5Samples(benchmark::State& state) {
    KnowledgeGapConfig config;
    config.enable_self_consistency_check = true;
    config.self_consistency_samples = 5;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(5);
    std::string answer = "This is a generated answer with consistent information.";
    
    for (auto _ : state) {
        auto result = detector->detectPostGeneration("test query", docs, answer);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("5 samples");
}
BENCHMARK(BM_SelfConsistency_5Samples);

static void BM_SemanticSimilarity_ShortTexts(benchmark::State& state) {
    KnowledgeGapConfig config;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(3);
    std::string answer1 = "Machine learning is a subset of AI.";
    std::string answer2 = "ML is part of artificial intelligence.";
    
    // Trigger semantic similarity indirectly through consistency check
    for (auto _ : state) {
        auto result = detector->detectPostGeneration("What is ML?", docs, answer1);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("short texts");
}
BENCHMARK(BM_SemanticSimilarity_ShortTexts);

static void BM_SemanticSimilarity_LongTexts(benchmark::State& state) {
    KnowledgeGapConfig config;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(3);
    std::string answer = "Machine learning is a method of data analysis that automates "
                        "analytical model building. It is a branch of artificial intelligence "
                        "based on the idea that systems can learn from data, identify patterns "
                        "and make decisions with minimal human intervention. The process uses "
                        "algorithms to iteratively learn from data.";
    
    for (auto _ : state) {
        auto result = detector->detectPostGeneration("What is ML?", docs, answer);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("long texts");
}
BENCHMARK(BM_SemanticSimilarity_LongTexts);

static void BM_ContradictionDetection(benchmark::State& state) {
    KnowledgeGapConfig config;
    config.enable_self_consistency_check = true;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(3);
    std::string answer = "The system is operational but not working correctly.";
    
    for (auto _ : state) {
        auto result = detector->detectPostGeneration("test query", docs, answer);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ContradictionDetection);

// ============================================================================
// Phase 2.3: FLARE Active Retrieval Benchmarks
// ============================================================================

static void BM_FLARE_Disabled(benchmark::State& state) {
    KnowledgeGapConfig config;
    config.enable_flare = false;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(2); // Insufficient documents
    
    for (auto _ : state) {
        auto result = detector->detectWithActiveRetrieval("test query", docs);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("FLARE disabled");
}
// Disabled: FLARE retrieval engine requires external service | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_FLARE_Disabled);

static void BM_FLARE_SingleRound(benchmark::State& state) {
    KnowledgeGapConfig config;
    config.enable_flare = true;
    config.max_retrieval_rounds = 1;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(2);
    
    for (auto _ : state) {
        auto result = detector->detectWithActiveRetrieval("complex query", docs);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("1 round");
}
BENCHMARK(BM_FLARE_SingleRound);

static void BM_FLARE_ThreeRounds(benchmark::State& state) {
    KnowledgeGapConfig config;
    config.enable_flare = true;
    config.max_retrieval_rounds = 3;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(2);
    
    for (auto _ : state) {
        auto result = detector->detectWithActiveRetrieval("complex query", docs);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("3 rounds");
}
BENCHMARK(BM_FLARE_ThreeRounds);

static void BM_SentenceSplitting_ShortText(benchmark::State& state) {
    KnowledgeGapConfig config;
    config.enable_flare = true;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(3);
    
    for (auto _ : state) {
        auto result = detector->detectWithActiveRetrieval("short query", docs);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("short text");
}
BENCHMARK(BM_SentenceSplitting_ShortText);

static void BM_SentenceSplitting_LongText(benchmark::State& state) {
    KnowledgeGapConfig config;
    config.enable_flare = true;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(3);
    std::string long_query = "This is a complex query with multiple sentences. "
                            "It requires detailed analysis. Each sentence adds context. "
                            "The system needs to process all information.";
    
    for (auto _ : state) {
        auto result = detector->detectWithActiveRetrieval(long_query, docs);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("long text");
}
BENCHMARK(BM_SentenceSplitting_LongText);

static void BM_QueryReformulation(benchmark::State& state) {
    KnowledgeGapConfig config;
    config.enable_flare = true;
    auto detector = std::make_unique<KnowledgeGapDetector>(config);
    
    auto docs = createTestDocuments(1); // Very insufficient
    
    for (auto _ : state) {
        auto result = detector->detectWithActiveRetrieval("query needing reformulation", docs);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_QueryReformulation);

// ============================================================================
// Comprehensive Detection Benchmarks
// ============================================================================

static void BM_ComprehensiveDetection_Fast(benchmark::State& state) {
    auto detector = KnowledgeGapDetectorFactory::createFast();
    
    auto docs = createTestDocuments(5);
    std::string answer = "Generated answer.";
    GenerationContext context;
    
    for (auto _ : state) {
        auto result = detector->detectGap("test query", docs, answer, context);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("Fast mode");
}
BENCHMARK(BM_ComprehensiveDetection_Fast);

static void BM_ComprehensiveDetection_Balanced(benchmark::State& state) {
    auto detector = KnowledgeGapDetectorFactory::createBalanced();
    
    auto docs = createTestDocuments(5);
    auto token_probs = createTokenProbabilities(100);
    
    std::string answer = "Generated answer.";
    GenerationContext context;
    context.token_probs = token_probs;
    context.generation_started = true;
    
    for (auto _ : state) {
        auto result = detector->detectGap("test query", docs, answer, context);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("Balanced mode");
}
BENCHMARK(BM_ComprehensiveDetection_Balanced);

static void BM_ComprehensiveDetection_Thorough(benchmark::State& state) {
    auto detector = KnowledgeGapDetectorFactory::createThorough();
    
    auto docs = createTestDocuments(5);
    auto token_probs = createTokenProbabilities(100);
    
    std::string answer = "Generated answer with multiple claims.";
    GenerationContext context;
    context.token_probs = token_probs;
    context.generation_started = true;
    
    for (auto _ : state) {
        auto result = detector->detectGap("test query", docs, answer, context);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("Thorough mode");
}
BENCHMARK(BM_ComprehensiveDetection_Thorough);

// ============================================================================
// Configuration & Setup Benchmarks
// ============================================================================

static void BM_DetectorCreation_Default(benchmark::State& state) {
    for (auto _ : state) {
        KnowledgeGapConfig config;
        auto detector = std::make_unique<KnowledgeGapDetector>(config);
        benchmark::DoNotOptimize(detector);
    }
}
BENCHMARK(BM_DetectorCreation_Default);

static void BM_DetectorCreation_FastFactory(benchmark::State& state) {
    for (auto _ : state) {
        auto detector = KnowledgeGapDetectorFactory::createFast();
        benchmark::DoNotOptimize(detector);
    }
}
BENCHMARK(BM_DetectorCreation_FastFactory);

static void BM_DetectorCreation_BalancedFactory(benchmark::State& state) {
    for (auto _ : state) {
        auto detector = KnowledgeGapDetectorFactory::createBalanced();
        benchmark::DoNotOptimize(detector);
    }
}
BENCHMARK(BM_DetectorCreation_BalancedFactory);

static void BM_DetectorCreation_ThoroughFactory(benchmark::State& state) {
    for (auto _ : state) {
        auto detector = KnowledgeGapDetectorFactory::createThorough();
        benchmark::DoNotOptimize(detector);
    }
}
BENCHMARK(BM_DetectorCreation_ThoroughFactory);

static void BM_ConfigurationUpdate(benchmark::State& state) {
    auto detector = KnowledgeGapDetectorFactory::createBalanced();
    KnowledgeGapConfig new_config;
    new_config.perplexity_threshold = 150.0;
    new_config.self_consistency_samples = 7;
    
    for (auto _ : state) {
        detector->setConfig(new_config);
        benchmark::DoNotOptimize(detector);
    }
}
BENCHMARK(BM_ConfigurationUpdate);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
