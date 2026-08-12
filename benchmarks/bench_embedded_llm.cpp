/**
 * @file bench_embedded_llm.cpp
 * @brief Google Benchmark suite for EmbeddedLLM performance
 * 
 * Measures throughput, latency, and resource usage of LLM operations.
 * 
 * @author ThemisDB Team / GitHub Copilot
 * @date January 2026
 */

#include <benchmark/benchmark.h>
#include "llm/embedded_llm.h"
#include <string>
#include <vector>

using namespace themis::llm;

// ═══════════════════════════════════════════════════════════
// Setup & Helpers
// ═══════════════════════════════════════════════════════════

static void InitLLM(const benchmark::State& state) {
    static bool initialized = false;
    if (!initialized) {
        EmbeddedLLM::Config config;
        config.model_path = "models/tinyllama.gguf";
        config.n_gpu_layers = 0;
        config.n_ctx = 2048;
        EmbeddedLLMManager::instance().initialize(config);
        initialized = true;
    }
}

// ═══════════════════════════════════════════════════════════
// Text Generation Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_LLM_Generation_Latency(benchmark::State& state) {
    InitLLM(state);
    int max_tokens = state.range(0);
    
    for (auto _ : state) {
        auto result = THEMIS_LLM().generate("Count slowly", max_tokens);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * max_tokens);
}

BENCHMARK(BM_LLM_Generation_Latency)
    ->Arg(10)->Arg(50)->Arg(100)->Arg(500)
    ->Unit(benchmark::kMillisecond);

static void BM_LLM_Generation_Throughput(benchmark::State& state) {
    InitLLM(state);
    int num_requests = state.range(0);
    int tokens_per = state.range(1);
    
    for (auto _ : state) {
        for (int i = 0; i < num_requests; ++i) {
            auto result = THEMIS_LLM().generate("test prompt", tokens_per);
            benchmark::DoNotOptimize(result);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_requests * tokens_per);
}

BENCHMARK(BM_LLM_Generation_Throughput)
    ->Args({10, 50})->Args({50, 100})->Args({100, 200})
    ->Unit(benchmark::kMillisecond);

// ═══════════════════════════════════════════════════════════
// Embeddings Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_LLM_Embeddings(benchmark::State& state) {
    InitLLM(state);
    int num_words = state.range(0);
    
    std::string text;
    for (int i = 0; i < num_words; ++i) {
        text += "word" + std::to_string(i) + " ";
    }
    
    for (auto _ : state) {
        auto embedding = THEMIS_LLM_EMBED(text);
        benchmark::DoNotOptimize(embedding);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_LLM_Embeddings)
    ->Arg(10)->Arg(100)->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

static void BM_LLM_Embeddings_Batch(benchmark::State& state) {
    InitLLM(state);
    int batch_size = state.range(0);
    
    std::vector<std::string> texts;
    for (int i = 0; i < batch_size; ++i) {
        texts.push_back("Sample text number " + std::to_string(i));
    }
    
    for (auto _ : state) {
        for (const auto& text : texts) {
            auto embedding = THEMIS_LLM_EMBED(text);
            benchmark::DoNotOptimize(embedding);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}

BENCHMARK(BM_LLM_Embeddings_Batch)
    ->Arg(10)->Arg(100)->Arg(1000)
    ->Unit(benchmark::kMillisecond);

// ═══════════════════════════════════════════════════════════
// Chat Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_LLM_Chat_MultiTurn(benchmark::State& state) {
    InitLLM(state);
    int num_turns = state.range(0);
    
    std::vector<ChatMessage> messages;
    messages.push_back({ChatRole::System, "You are helpful"});
    
    for (auto _ : state) {
        messages.clear();
        messages.push_back({ChatRole::System, "You are helpful"});
        
        for (int i = 0; i < num_turns; ++i) {
            messages.push_back({ChatRole::User, "Question " + std::to_string(i)});
            auto response = THEMIS_LLM_CHAT(messages);
            messages.push_back({ChatRole::Assistant, response});
            benchmark::DoNotOptimize(response);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_turns);
}

BENCHMARK(BM_LLM_Chat_MultiTurn)
    ->Arg(1)->Arg(5)->Arg(10)
    ->Unit(benchmark::kMillisecond);

// ═══════════════════════════════════════════════════════════
// Streaming Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_LLM_Streaming(benchmark::State& state) {
    InitLLM(state);
    int max_tokens = state.range(0);
    
    for (auto _ : state) {
        std::vector<std::string> tokens;
        auto callback = [&tokens](const std::string& token) {
            tokens.push_back(token);
        };
        
        THEMIS_LLM().generateStreaming("Test prompt", callback, max_tokens);
        benchmark::DoNotOptimize(tokens);
    }
    
    state.SetItemsProcessed(state.iterations() * max_tokens);
}

BENCHMARK(BM_LLM_Streaming)
    ->Arg(50)->Arg(100)->Arg(500)
    ->Unit(benchmark::kMillisecond);

// ═══════════════════════════════════════════════════════════
// Concurrent Access Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_LLM_Concurrent_Requests(benchmark::State& state) {
    InitLLM(state);
    
    for (auto _ : state) {
        auto result = THEMIS_LLM_GENERATE("concurrent test");
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_LLM_Concurrent_Requests)
    ->ThreadRange(1, 16)
    ->Unit(benchmark::kMillisecond);

// ═══════════════════════════════════════════════════════════
// Memory Usage Benchmark
// ═══════════════════════════════════════════════════════════

static void BM_LLM_Memory_Usage(benchmark::State& state) {
    InitLLM(state);
    
    for (auto _ : state) {
        state.PauseTiming();
        std::string prompt = "Memory test prompt " + std::to_string(state.iterations());
        state.ResumeTiming();
        
        auto result = THEMIS_LLM_GENERATE(prompt);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_LLM_Memory_Usage)
    ->Iterations(100)
    ->Unit(benchmark::kMillisecond);

// Main function
BENCHMARK_MAIN();
