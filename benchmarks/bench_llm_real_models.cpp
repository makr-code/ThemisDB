/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_llm_real_models.cpp                          ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     379                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Example: Integration of Ollama models into ThemisDB benchmarks
// File: benchmarks/bench_llm_real_models.cpp
//
// This benchmark uses real GGUF models downloaded via download-ollama-models.ps1

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>

#ifdef THEMIS_ENABLE_LLM
#include "llm/llm_plugin_manager.h"
#include "llm/llama_wrapper.h"
#endif

#include "storage/rocksdb_wrapper.h"
#include "index/vector_index.h"

using namespace themis;

// Get model path from environment or use default
std::string getModelPath() {
    const char* env_path = std::getenv("THEMIS_LLM_MODEL_PATH");
    if (env_path && std::filesystem::exists(env_path)) {
        return env_path;
    }
    
    // Check default locations
    std::vector<std::string> default_paths = {
        "./models/tinyllama_1.1b.gguf",
        "./models/llama3.2_1b.gguf",
        "./models/phi3_mini.gguf",
        "../models/tinyllama_1.1b.gguf",
        "C:/VCC/themis/models/tinyllama_1.1b.gguf"
    };
    
    for (const auto& path : default_paths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    
    return "";
}

#ifdef THEMIS_ENABLE_LLM

// ============================================================================
// REAL LLM MODEL BENCHMARKS
// ============================================================================

class RealLLMBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        model_path_ = getModelPath();
        
        if (model_path_.empty()) {
            state.SkipWithError("No LLM model found. Run: .\\scripts\\download-ollama-models.ps1");
            return;
        }
        
        // Initialize DB
        db_path_ = "C:\\tmp\\bench_real_llm_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        // Initialize LLM Plugin Manager
        auto& manager = llm::LLMPluginManager::getInstance();
        
        // Load model
        try {
            llm::LlamaWrapperConfig plugin_config;
            plugin_config.n_threads = 4;
            plugin_config.n_ctx = 2048;
            plugin_config.use_mmap = true;
            
            manager.createLlamaWrapper("benchmark_model", model_path_, plugin_config);
            
        } catch (const std::exception& e) {
            state.SkipWithError(std::string("Failed to load model: ") + e.what());
            return;
        }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        auto& manager = llm::LLMPluginManager::getInstance();
        manager.unregisterPlugin("benchmark_model");
        
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string model_path_;
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

// Benchmark: Text Embedding Generation (real model)
BENCHMARK_F(RealLLMBench, RealModel_TextEmbedding_Generation)(benchmark::State& state) {
    auto& manager = llm::LLMPluginManager::getInstance();
    auto* plugin = manager.getPlugin("benchmark_model");
    
    if (!plugin) {
        state.SkipWithError("Model not loaded");
        return;
    }
    
    std::vector<std::string> test_texts = {
        "The quick brown fox jumps over the lazy dog",
        "Machine learning is transforming the world of technology",
        "Vector databases enable semantic search capabilities",
        "ThemisDB provides high-performance storage and retrieval"
    };
    
    size_t text_idx = 0;
    
    for (auto _ : state) {
        const auto& text = test_texts[text_idx % test_texts.size()];
        
        // Generate embedding using real model
        auto embedding = plugin->generateEmbedding(text);
        
        benchmark::DoNotOptimize(embedding);
        text_idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * 64); // Avg text size estimate
}

// Benchmark: Text Generation (real model)
BENCHMARK_F(RealLLMBench, RealModel_TextGeneration_50Tokens)(benchmark::State& state) {
    auto& manager = llm::LLMPluginManager::getInstance();
    auto* plugin = manager.getPlugin("benchmark_model");
    
    if (!plugin) {
        state.SkipWithError("Model not loaded");
        return;
    }
    
    std::vector<std::string> prompts = {
        "Explain database indexing in one sentence:",
        "What is a vector database?",
        "Describe semantic search:",
        "How does RAG work?"
    };
    
    size_t prompt_idx = 0;
    
    for (auto _ : state) {
        const auto& prompt = prompts[prompt_idx % prompts.size()];
        
        llm::GenerationParams params;
        params.max_tokens = 50;
        params.temperature = 0.7f;
        params.top_p = 0.9f;
        
        auto response = plugin->generate(prompt, params);
        
        benchmark::DoNotOptimize(response);
        prompt_idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: RAG Pipeline with Real Model
BENCHMARK_F(RealLLMBench, RealModel_RAG_Pipeline_EndToEnd)(benchmark::State& state) {
    auto& manager = llm::LLMPluginManager::getInstance();
    auto* plugin = manager.getPlugin("benchmark_model");
    
    if (!plugin) {
        state.SkipWithError("Model not loaded");
        return;
    }
    
    VectorIndexManager vim(*db_);
    
    // Populate knowledge base with embeddings
    std::vector<std::string> documents = {
        "ThemisDB is a high-performance vector database.",
        "It supports semantic search and RAG workflows.",
        "The database uses RocksDB as storage backend.",
        "Vector indexing enables fast similarity search.",
        "FAISS integration provides scalable vector operations."
    };
    
    // Pre-generate embeddings for documents
    std::vector<std::vector<float>> doc_embeddings;
    for (const auto& doc : documents) {
        doc_embeddings.push_back(plugin->generateEmbedding(doc));
    }
    
    // Store in DB
    for (size_t i = 0; i < documents.size(); ++i) {
        BaseEntity entity("doc_" + std::to_string(i), BaseEntity::FieldMap{
            {"embedding", doc_embeddings[i]},
            {"text", documents[i]}
        });
        vim.addEntity(entity);
    }
    
    std::string query = "What database features are available?";
    
    for (auto _ : state) {
        // 1. Generate query embedding
        auto query_embedding = plugin->generateEmbedding(query);
        
        // 2. Search for relevant documents
        auto results = vim.searchKnn(query_embedding, 3);
        
        // 3. Build context from results
        std::string context;
        for (const auto& result : results) {
            // In real scenario, retrieve text field
            context += "Context: " + std::to_string(result.score) + "\n";
        }
        
        // 4. Generate response with context
        std::string rag_prompt = "Based on:\n" + context + "\nAnswer: " + query;
        
        llm::GenerationParams params;
        params.max_tokens = 100;
        auto response = plugin->generate(rag_prompt, params);
        
        benchmark::DoNotOptimize(response);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Batch Embedding Generation
BENCHMARK_F(RealLLMBench, RealModel_BatchEmbedding_100Docs)(benchmark::State& state) {
    auto& manager = llm::LLMPluginManager::getInstance();
    auto* plugin = manager.getPlugin("benchmark_model");
    
    if (!plugin) {
        state.SkipWithError("Model not loaded");
        return;
    }
    
    // Generate test documents
    std::vector<std::string> documents;
    for (int i = 0; i < 100; ++i) {
        documents.push_back("Document " + std::to_string(i) + ": Sample content for embedding generation benchmark.");
    }
    
    for (auto _ : state) {
        std::vector<std::vector<float>> embeddings;
        embeddings.reserve(documents.size());
        
        for (const auto& doc : documents) {
            embeddings.push_back(plugin->generateEmbedding(doc));
        }
        
        benchmark::DoNotOptimize(embeddings);
    }
    
    state.SetItemsProcessed(state.iterations() * documents.size());
}

// Benchmark: Model Loading Performance
static void BM_RealModel_LoadingTime(benchmark::State& state) {
    std::string model_path = getModelPath();
    
    if (model_path.empty()) {
        state.SkipWithError("No LLM model found");
        return;
    }
    
    for (auto _ : state) {
        auto& manager = llm::LLMPluginManager::getInstance();
        
        llm::LlamaWrapperConfig config;
        config.n_threads = 4;
        config.n_ctx = 2048;
        
        state.PauseTiming();
        std::string plugin_name = "load_test_" + std::to_string(state.iterations());
        state.ResumeTiming();
        
        manager.createLlamaWrapper(plugin_name, model_path, config);
        
        state.PauseTiming();
        manager.unregisterPlugin(plugin_name);
        state.ResumeTiming();
    }
}

BENCHMARK(BM_RealModel_LoadingTime)->Unit(benchmark::kSecond);

// Benchmark: Context Size Scaling
BENCHMARK_F(RealLLMBench, RealModel_ContextScaling)(benchmark::State& state) {
    auto& manager = llm::LLMPluginManager::getInstance();
    auto* plugin = manager.getPlugin("benchmark_model");
    
    if (!plugin) {
        state.SkipWithError("Model not loaded");
        return;
    }
    
    size_t context_tokens = state.range(0);
    
    // Generate prompt with approximate token count
    std::string base_prompt = "Summarize the following text:\n";
    std::string long_text;
    
    // Approximate: ~4 chars per token
    size_t target_chars = context_tokens * 4;
    while (long_text.size() < target_chars) {
        long_text += "This is sample text for context scaling benchmarks. ";
    }
    
    std::string prompt = base_prompt + long_text;
    
    for (auto _ : state) {
        llm::GenerationParams params;
        params.max_tokens = 50;
        
        auto response = plugin->generate(prompt, params);
        benchmark::DoNotOptimize(response);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("ctx_tokens=" + std::to_string(context_tokens));
}

BENCHMARK_REGISTER_F(RealLLMBench, RealModel_ContextScaling)
    ->Args({512})
    ->Args({1024})
    ->Args({2048})
    ->Unit(benchmark::kMillisecond);

#else // THEMIS_ENABLE_LLM

// Stub benchmarks when LLM is not enabled
static void BM_LLM_NotEnabled(benchmark::State& state) {
    state.SkipWithError("LLM support not enabled. Build with -DTHEMIS_ENABLE_LLM=ON");
}

BENCHMARK(BM_LLM_NotEnabled);

#endif // THEMIS_ENABLE_LLM

// Run benchmarks
BENCHMARK_MAIN();
