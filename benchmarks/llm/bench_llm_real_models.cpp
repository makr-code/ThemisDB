#include <benchmark/benchmark.h>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>

#include "benchmark_artifact_preflight.h"

#ifdef THEMIS_ENABLE_LLM
#include "llm/llm_plugin_manager.h"
#endif

using namespace themis;

// Resolve model path via the standardised preflight utility (Maßnahme #6).
// Priority: THEMIS_LLM_MODEL_PATH > THEMIS_MODEL_DIR/{stub,real} > legacy paths.
std::string getModelPath() {
    std::string path = bench::resolveModelPath();
    if (!path.empty()) {
        return path;
    }

    // Legacy fallback for local developer setups that pre-date Maßnahme #6.
    // Deprecated: set THEMIS_MODEL_DIR or run scripts/download_models.sh instead.
    const std::vector<std::string> legacy_paths = {
        "./models/tinyllama_1.1b.gguf",
        "./models/llama3.2_1b.gguf",
        "./models/phi3_mini.gguf",
        "../models/tinyllama_1.1b.gguf",
    };
    for (const auto& p : legacy_paths) {
        if (std::filesystem::exists(p)) {
            return p;
        }
    }

    return "";
}

#ifdef THEMIS_ENABLE_LLM

class RealLLMBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        (void)state;
        model_path_ = getModelPath();

        if (model_path_.empty()) {
            ready_ = false;
            error_message_ =
                "LLM artefact preflight FAILED: no model file found. "
                "Run 'scripts/download_models.sh --stub-only' or set "
                "THEMIS_MODEL_DIR / THEMIS_LLM_MODEL_PATH. "
                "See docs/BENCHMARK_RUNBOOK.md §\"LLM/LoRA Model Setup\".";
            return;
        }

        model_id_ = "benchmark_model_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        auto& manager = llm::LLMPluginManager::instance();
        loaded_model_ = manager.loadModel(model_id_, model_path_);
        ready_ = loaded_model_;
        if (!ready_) {
            error_message_ = "Failed to load model via LLMPluginManager::loadModel";
        }
    }

    void TearDown(const ::benchmark::State& state) override {
        (void)state;
        if (loaded_model_) {
            auto& manager = llm::LLMPluginManager::instance();
            manager.unloadModel(model_id_);
        }
    }

protected:
    bool ensureReady(benchmark::State& state) const {
        if (!ready_) {
            state.SkipWithError(error_message_.c_str());
            return false;
        }
        return true;
    }

    bool ready_ = false;
    bool loaded_model_ = false;
    std::string error_message_;
    std::string model_id_;
    std::string model_path_;
};

BENCHMARK_F(RealLLMBench, RealModel_TextEmbedding_Generation)(benchmark::State& state) {
    if (!ensureReady(state)) {
        return;
    }

    auto& manager = llm::LLMPluginManager::instance();
    std::vector<std::string> test_texts = {
        "The quick brown fox jumps over the lazy dog",
        "Machine learning is transforming the world of technology",
        "Vector databases enable semantic search capabilities",
        "ThemisDB provides high-performance storage and retrieval"
    };
    
    size_t text_idx = 0;
    
    for (auto _ : state) {
        const auto& text = test_texts[text_idx % test_texts.size()];

        auto embedding = manager.embed(text);
        benchmark::DoNotOptimize(embedding);
        text_idx++;
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * 64);
}

BENCHMARK_F(RealLLMBench, RealModel_TextGeneration_50Tokens)(benchmark::State& state) {
    if (!ensureReady(state)) {
        return;
    }

    auto& manager = llm::LLMPluginManager::instance();
    std::vector<std::string> prompts = {
        "Explain database indexing in one sentence:",
        "What is a vector database?",
        "Describe semantic search:",
        "How does RAG work?"
    };
    
    size_t prompt_idx = 0;
    
    for (auto _ : state) {
        const auto& prompt = prompts[prompt_idx % prompts.size()];

        llm::InferenceRequest request;
        request.prompt = prompt;
        request.max_tokens = 50;
        request.temperature = 0.7f;
        request.top_p = 0.9f;

        auto response = manager.generate(request);
        benchmark::DoNotOptimize(response);
        prompt_idx++;
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(RealLLMBench, RealModel_BatchEmbedding_100Docs)(benchmark::State& state) {
    if (!ensureReady(state)) {
        return;
    }

    auto& manager = llm::LLMPluginManager::instance();
    // Generate test documents
    std::vector<std::string> documents;
    for (int i = 0; i < 100; ++i) {
        documents.push_back("Document " + std::to_string(i) + ": Sample content for embedding generation benchmark.");
    }
    
    for (auto _ : state) {
        std::vector<std::vector<float>> embeddings;
        embeddings.reserve(documents.size());

        for (const auto& doc : documents) {
            embeddings.push_back(manager.embed(doc));
        }

        benchmark::DoNotOptimize(embeddings);
    }

    state.SetItemsProcessed(state.iterations() * documents.size());
}

static void BM_RealModel_LoadingTime(benchmark::State& state) {
    std::string model_path = getModelPath();

    if (model_path.empty()) {
        state.SkipWithError(
            "LLM artefact preflight FAILED: no model file found. "
            "Run 'scripts/download_models.sh --stub-only' or set "
            "THEMIS_MODEL_DIR / THEMIS_LLM_MODEL_PATH. "
            "See docs/BENCHMARK_RUNBOOK.md §\"LLM/LoRA Model Setup\".");
        return;
    }

    auto& manager = llm::LLMPluginManager::instance();
    for (auto _ : state) {
        std::string model_id = "load_test_" + std::to_string(static_cast<unsigned long long>(state.iterations()));
        bool loaded = manager.loadModel(model_id, model_path);
        benchmark::DoNotOptimize(loaded);
        if (loaded) {
            manager.unloadModel(model_id);
        }
    }
}

BENCHMARK(BM_RealModel_LoadingTime)->Unit(benchmark::kSecond);

BENCHMARK_F(RealLLMBench, RealModel_ContextScaling)(benchmark::State& state) {
    if (!ensureReady(state)) {
        return;
    }

    auto& manager = llm::LLMPluginManager::instance();
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
        llm::InferenceRequest request;
        request.prompt = prompt;
        request.max_tokens = 50;
        request.temperature = 0.7f;
        request.top_p = 0.9f;

        auto response = manager.generate(request);
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
