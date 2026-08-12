/**
 * @file bench_llm_judge_integration.cpp
 * @brief Performance benchmarks for LLMJudgeIntegration
 * 
 * Benchmarks:
 * - Mock mode evaluation performance
 * - Custom inference function overhead
 * - Configuration change overhead
 * - Retry mechanism performance
 * - Different evaluation dimensions
 * 
 * Performance targets:
 * - Mock mode evaluation: < 10ms
 * - Custom inference call: < 5ms overhead
 * - Configuration update: < 1ms
 * - Retry logic: < 50ms for 3 retries
 */

#include <benchmark/benchmark.h>
#include "rag/llm_judge_integration.h"
#include "rag/prompt_templates.h"
#include "rag/rag_judge.h"
#include <string>
#include <chrono>
#include <thread>

using namespace themis::rag::judge;

// ============================================================================
// Test Data Fixtures
// ============================================================================

static EvaluationInput createTestInput() {
    EvaluationInput input;
    input.query = "What is the capital of France?";
    input.generated_answer = "Paris is the capital of France, known for the Eiffel Tower.";
    
    RetrievedDocument doc1;
    doc1.id = "doc1";
    doc1.content = "Paris is the capital and most populous city of France.";
    doc1.similarity_score = 0.95;
    input.documents.push_back(doc1);
    
    RetrievedDocument doc2;
    doc2.id = "doc2";
    doc2.content = "The Eiffel Tower is located in Paris, France.";
    doc2.similarity_score = 0.90;
    input.documents.push_back(doc2);
    
    return input;
}

static std::string mockInferenceFunction(const std::string& prompt) {
    return R"({
        "score": 4.5,
        "confidence": 0.92,
        "reasoning": "The answer accurately states Paris as the capital and provides relevant context.",
        "supporting_claims": ["Paris is the capital of France", "Eiffel Tower is in Paris"],
        "unsupported_claims": []
    })";
}

static std::string fastInferenceFunction(const std::string& prompt) {
    // Simulate fast LLM response
    return R"({"score": 4.0, "confidence": 0.85, "reasoning": "Good", "supporting_claims": [], "unsupported_claims": []})";
}

static std::string slowInferenceFunction(const std::string& prompt) {
    // Simulate slower LLM response (e.g., cloud API)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return R"({"score": 4.2, "confidence": 0.88, "reasoning": "Acceptable", "supporting_claims": [], "unsupported_claims": []})";
}

// ============================================================================
// Mock Mode Benchmarks
// ============================================================================

static void BM_MockModeEvaluation(benchmark::State& state) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    config.warn_on_mock_mode = false;  // Disable warnings for benchmarking
    
    LLMJudgeIntegration integration(config);
    PromptTemplateManager template_manager = PromptTemplateManager::createDefault();
    EvaluationInput input = createTestInput();
    
    for (auto _ : state) {
        auto result = integration.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS,
            input,
            template_manager
        );
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("Mock mode evaluation");
}
BENCHMARK(BM_MockModeEvaluation)->Unit(benchmark::kMillisecond);

static void BM_MockModeMultipleDimensions(benchmark::State& state) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    config.warn_on_mock_mode = false;
    
    LLMJudgeIntegration integration(config);
    PromptTemplateManager template_manager = PromptTemplateManager::createDefault();
    EvaluationInput input = createTestInput();
    
    const std::vector<EvaluationDimension> dimensions = {
        EvaluationDimension::FAITHFULNESS,
        EvaluationDimension::RELEVANCE,
        EvaluationDimension::COMPLETENESS,
        EvaluationDimension::COHERENCE
    };
    
    for (auto _ : state) {
        for (const auto& dim : dimensions) {
            auto result = integration.evaluateWithLLM(dim, input, template_manager);
            benchmark::DoNotOptimize(result);
        }
    }
    
    state.SetLabel("4 dimensions in mock mode");
}
BENCHMARK(BM_MockModeMultipleDimensions)->Unit(benchmark::kMillisecond);

// ============================================================================
// Custom Inference Function Benchmarks
// ============================================================================

static void BM_CustomInferenceFast(benchmark::State& state) {
    LLMJudgeIntegration integration;
    integration.setInferenceFunction(fastInferenceFunction);
    
    PromptTemplateManager template_manager = PromptTemplateManager::createDefault();
    EvaluationInput input = createTestInput();
    
    for (auto _ : state) {
        auto result = integration.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS,
            input,
            template_manager
        );
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("Fast custom inference");
}
BENCHMARK(BM_CustomInferenceFast)->Unit(benchmark::kMillisecond);

static void BM_CustomInferenceSlow(benchmark::State& state) {
    LLMJudgeIntegration integration;
    integration.setInferenceFunction(slowInferenceFunction);
    
    PromptTemplateManager template_manager = PromptTemplateManager::createDefault();
    EvaluationInput input = createTestInput();
    
    for (auto _ : state) {
        auto result = integration.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS,
            input,
            template_manager
        );
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("Slow custom inference (10ms)");
}
BENCHMARK(BM_CustomInferenceSlow)->Unit(benchmark::kMillisecond);

static void BM_InferenceFunctionOverhead(benchmark::State& state) {
    LLMJudgeIntegration integration;
    
    // Minimal inference function to measure overhead
    integration.setInferenceFunction([](const std::string& prompt) {
        return R"({"score": 4.0, "confidence": 0.85, "reasoning": "", "supporting_claims": [], "unsupported_claims": []})";
    });
    
    PromptTemplateManager template_manager = PromptTemplateManager::createDefault();
    EvaluationInput input = createTestInput();
    
    for (auto _ : state) {
        auto result = integration.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS,
            input,
            template_manager
        );
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("Minimal inference overhead");
}
BENCHMARK(BM_InferenceFunctionOverhead)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Configuration Benchmarks
// ============================================================================

static void BM_ConfigurationUpdate(benchmark::State& state) {
    LLMJudgeIntegration integration;
    
    for (auto _ : state) {
        LLMJudgeIntegration::Config config;
        config.model_name = "test-model";
        config.temperature = 0.5;
        config.max_tokens = 2048;
        config.max_retries = 5;
        
        integration.setConfig(config);
        benchmark::DoNotOptimize(config);
    }
    
    state.SetLabel("Configuration update");
}
BENCHMARK(BM_ConfigurationUpdate)->Unit(benchmark::kNanosecond);

static void BM_ConfigurationRetrieval(benchmark::State& state) {
    LLMJudgeIntegration integration;
    
    for (auto _ : state) {
        auto config = integration.getConfig();
        benchmark::DoNotOptimize(config);
    }
    
    state.SetLabel("Configuration retrieval");
}
BENCHMARK(BM_ConfigurationRetrieval)->Unit(benchmark::kNanosecond);

static void BM_MockModeCheck(benchmark::State& state) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    LLMJudgeIntegration integration(config);
    
    for (auto _ : state) {
        bool is_mock = integration.isMockMode();
        benchmark::DoNotOptimize(is_mock);
    }
    
    state.SetLabel("Mock mode check");
}
BENCHMARK(BM_MockModeCheck)->Unit(benchmark::kNanosecond);

// ============================================================================
// Retry Mechanism Benchmarks
// ============================================================================

static void BM_RetryMechanism_Success(benchmark::State& state) {
    LLMJudgeIntegration::Config config;
    config.max_retries = 3;
    
    LLMJudgeIntegration integration(config);
    integration.setInferenceFunction(fastInferenceFunction);
    
    PromptTemplateManager template_manager = PromptTemplateManager::createDefault();
    EvaluationInput input = createTestInput();
    
    for (auto _ : state) {
        auto result = integration.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS,
            input,
            template_manager
        );
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("Retry mechanism (immediate success)");
}
BENCHMARK(BM_RetryMechanism_Success)->Unit(benchmark::kMillisecond);

static void BM_RetryMechanism_SingleRetry(benchmark::State& state) {
    LLMJudgeIntegration::Config config;
    config.max_retries = 3;
    
    LLMJudgeIntegration integration(config);
    
    int call_count = 0;
    integration.setInferenceFunction([&call_count](const std::string& prompt) {
        call_count++;
        if (call_count % 2 == 1) {  // Fail on odd calls
            throw std::runtime_error("Simulated failure");
        }
        return fastInferenceFunction(prompt);
    });
    
    PromptTemplateManager template_manager = PromptTemplateManager::createDefault();
    EvaluationInput input = createTestInput();
    
    for (auto _ : state) {
        auto result = integration.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS,
            input,
            template_manager
        );
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("Retry mechanism (1 retry)");
}
BENCHMARK(BM_RetryMechanism_SingleRetry)->Unit(benchmark::kMillisecond);

// ============================================================================
// Dimension Evaluation Benchmarks
// ============================================================================

static void BM_EvaluateFaithfulness(benchmark::State& state) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    config.warn_on_mock_mode = false;
    
    LLMJudgeIntegration integration(config);
    PromptTemplateManager template_manager = PromptTemplateManager::createDefault();
    EvaluationInput input = createTestInput();
    
    for (auto _ : state) {
        auto result = integration.evaluateWithLLM(
            EvaluationDimension::FAITHFULNESS,
            input,
            template_manager
        );
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_EvaluateFaithfulness)->Unit(benchmark::kMillisecond);

static void BM_EvaluateRelevance(benchmark::State& state) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    config.warn_on_mock_mode = false;
    
    LLMJudgeIntegration integration(config);
    PromptTemplateManager template_manager = PromptTemplateManager::createDefault();
    EvaluationInput input = createTestInput();
    
    for (auto _ : state) {
        auto result = integration.evaluateWithLLM(
            EvaluationDimension::RELEVANCE,
            input,
            template_manager
        );
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_EvaluateRelevance)->Unit(benchmark::kMillisecond);

static void BM_EvaluateCompleteness(benchmark::State& state) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    config.warn_on_mock_mode = false;
    
    LLMJudgeIntegration integration(config);
    PromptTemplateManager template_manager = PromptTemplateManager::createDefault();
    EvaluationInput input = createTestInput();
    
    for (auto _ : state) {
        auto result = integration.evaluateWithLLM(
            EvaluationDimension::COMPLETENESS,
            input,
            template_manager
        );
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_EvaluateCompleteness)->Unit(benchmark::kMillisecond);

static void BM_EvaluateCoherence(benchmark::State& state) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    config.warn_on_mock_mode = false;
    
    LLMJudgeIntegration integration(config);
    PromptTemplateManager template_manager = PromptTemplateManager::createDefault();
    EvaluationInput input = createTestInput();
    
    for (auto _ : state) {
        auto result = integration.evaluateWithLLM(
            EvaluationDimension::COHERENCE,
            input,
            template_manager
        );
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_EvaluateCoherence)->Unit(benchmark::kMillisecond);

// ============================================================================
// Batch Evaluation Benchmarks
// ============================================================================

static void BM_BatchEvaluation_10Inputs(benchmark::State& state) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    config.warn_on_mock_mode = false;
    
    LLMJudgeIntegration integration(config);
    PromptTemplateManager template_manager = PromptTemplateManager::createDefault();
    
    std::vector<EvaluationInput> inputs;
    for (int i = 0; i < 10; i++) {
        inputs.push_back(createTestInput());
    }
    
    for (auto _ : state) {
        for (const auto& input : inputs) {
            auto result = integration.evaluateWithLLM(
                EvaluationDimension::FAITHFULNESS,
                input,
                template_manager
            );
            benchmark::DoNotOptimize(result);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 10);
    state.SetLabel("Batch evaluation (10 inputs)");
}
BENCHMARK(BM_BatchEvaluation_10Inputs)->Unit(benchmark::kMillisecond);

static void BM_BatchEvaluation_100Inputs(benchmark::State& state) {
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    config.warn_on_mock_mode = false;
    
    LLMJudgeIntegration integration(config);
    PromptTemplateManager template_manager = PromptTemplateManager::createDefault();
    
    std::vector<EvaluationInput> inputs;
    for (int i = 0; i < 100; i++) {
        inputs.push_back(createTestInput());
    }
    
    for (auto _ : state) {
        for (const auto& input : inputs) {
            auto result = integration.evaluateWithLLM(
                EvaluationDimension::FAITHFULNESS,
                input,
                template_manager
            );
            benchmark::DoNotOptimize(result);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
    state.SetLabel("Batch evaluation (100 inputs)");
}
BENCHMARK(BM_BatchEvaluation_100Inputs)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
