#include <benchmark/benchmark.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <cmath>
#include <utility>

#include "benchmark_artifact_preflight.h"

#ifdef THEMIS_ENABLE_LLM
#include "llm/llm_plugin_manager.h"
#endif

using namespace themis;

// Resolve only from the documented model artifact configuration.
std::string getModelPath() {
    return bench::resolveModelPath();
}

#ifdef THEMIS_ENABLE_LLM

namespace {

class ScopedDefaultLlamaPlugin {
public:
    ScopedDefaultLlamaPlugin() = default;
    ScopedDefaultLlamaPlugin(const ScopedDefaultLlamaPlugin&) = delete;
    ScopedDefaultLlamaPlugin& operator=(const ScopedDefaultLlamaPlugin&) = delete;

    ~ScopedDefaultLlamaPlugin() {
        if (registered_here_) {
            llm::LLMPluginManager::instance().unregisterPlugin("llamacpp");
        }
    }

    bool ensureRegistered(const std::string& model_path, std::string& error_message) {
        auto& manager = llm::LLMPluginManager::instance();
        if (manager.getDefaultPlugin() != nullptr) {
            return true;
        }

        registered_here_ = llm::createLlamaWrapper(
            "llamacpp", model_path, llm::json{{"enable_embeddings", true}});
        if (!registered_here_ || manager.getDefaultPlugin() == nullptr) {
            error_message = "Failed to register llama.cpp benchmark backend";
            return false;
        }
        return true;
    }

    bool registeredHere() const noexcept {
        return registered_here_;
    }

private:
    bool registered_here_ = false;
};

} // namespace

class RealLLMBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        (void)state;
        model_path_ = getModelPath();

        if (model_path_.empty()) {
            ready_ = false;
            optional_artifact_missing_ = true;
            error_message_ =
                "Optional model-backed LLM benchmark skipped: no GGUF model artifact "
                "was found at the configured model path. Set THEMIS_LLM_MODEL_PATH "
                "or THEMIS_MODEL_DIR to a real GGUF model to collect performance evidence.";
            return;
        }

        model_id_ = "benchmark_model_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        auto& manager = llm::LLMPluginManager::instance();
        if (!backend_.ensureRegistered(model_path_, error_message_)) {
            ready_ = false;
            return;
        }

        if (backend_.registeredHere()) {
            const auto* plugin = manager.getDefaultPlugin();
            loaded_model_ = plugin != nullptr && plugin->isModelLoaded();
        } else {
            const auto* plugin = manager.getDefaultPlugin();
            loaded_model_ = plugin != nullptr && plugin->isModelLoaded();
            if (!loaded_model_) {
                loaded_model_ = manager.loadModel(model_id_, model_path_);
            }
        }
        ready_ = loaded_model_;
        if (!ready_) {
            error_message_ = "Failed to load model via LLMPluginManager::loadModel";
        }
    }

    void TearDown(const ::benchmark::State& state) override {
        (void)state;
        if (loaded_model_ && !backend_.registeredHere()) {
            auto& manager = llm::LLMPluginManager::instance();
            manager.unloadModel(model_id_);
        }
    }

protected:
    bool ensureReady(benchmark::State& state) const {
        if (!ready_) {
            if (optional_artifact_missing_) {
                state.SkipWithMessage(error_message_.c_str());
            } else {
                state.SkipWithError(error_message_.c_str());
            }
            return false;
        }
        return true;
    }

    bool ready_ = false;
    bool loaded_model_ = false;
    bool optional_artifact_missing_ = false;
    std::string error_message_ = {};
    std::string model_id_ = {};
    std::string model_path_ = {};
    ScopedDefaultLlamaPlugin backend_;
};

BENCHMARK_F(RealLLMBench, RealModel_TextEmbedding_Generation)(benchmark::State& state) {
    state.SetLabel("component");
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
        if (embedding.empty()) {
            state.SkipWithError("LLMPluginManager::embed returned an empty embedding");
            break;
        }
        for (const auto value : embedding) {
            if (!std::isfinite(value)) {
                state.SkipWithError("LLMPluginManager::embed returned a non-finite value");
                return;
            }
        }
        benchmark::DoNotOptimize(embedding);
        text_idx++;
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * 64);
}

BENCHMARK_F(RealLLMBench, RealModel_TextGeneration_50Tokens)(benchmark::State& state) {
    state.SetLabel("end_to_end");
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
        if (!response.success || response.text.empty()) {
            state.SkipWithError(response.error_message.empty()
                                    ? "LLMPluginManager::generate returned no valid output"
                                    : response.error_message.c_str());
            break;
        }
        benchmark::DoNotOptimize(response);
        prompt_idx++;
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(RealLLMBench, RealModel_BatchEmbedding_100Docs)(benchmark::State& state) {
    state.SetLabel("component");
    if (!ensureReady(state)) {
        return;
    }

    auto& manager = llm::LLMPluginManager::instance();
    // Generate test documents
    std::vector<std::string> documents = {};

    for (int i = 0; i < 100; ++i) {
        documents.push_back("Document " + std::to_string(i) + ": Sample content for embedding generation benchmark.");
    }
    
    for (auto _ : state) {
        std::vector<std::vector<float>> embeddings;
        embeddings.reserve(documents.size());

        for (const auto& doc : documents) {
            auto embedding = manager.embed(doc);
            if (embedding.empty()) {
                state.SkipWithError("Batch embedding returned an empty embedding");
                return;
            }
            embeddings.push_back(std::move(embedding));
        }

        benchmark::DoNotOptimize(embeddings);
    }

    state.SetItemsProcessed(state.iterations() * documents.size());
}

static void BM_RealModel_LoadingTime(benchmark::State& state) {
    state.SetLabel("component");
    std::string model_path = getModelPath();

    if (model_path.empty()) {
        state.SkipWithMessage(
            "Optional model-backed LLM benchmark skipped: no GGUF model artifact "
            "was found at the configured model path. Set THEMIS_LLM_MODEL_PATH "
            "or THEMIS_MODEL_DIR to a real GGUF model to collect performance evidence.");
        return;
    }

    auto& manager = llm::LLMPluginManager::instance();
    std::string registration_error;
    ScopedDefaultLlamaPlugin backend;
    if (!backend.ensureRegistered(model_path, registration_error)) {
        state.SkipWithError(registration_error.c_str());
        return;
    }

    if (backend.registeredHere()) {
        if (auto* plugin = manager.getDefaultPlugin()) {
            plugin->unloadModel();
        }
    } else if (const auto* plugin = manager.getDefaultPlugin();
               plugin != nullptr && plugin->isModelLoaded()) {
        state.SkipWithError(
            "Cannot benchmark model loading while another default model is already loaded");
        return;
    }

    for (auto _ : state) {
        std::string model_id = "load_test_" + std::to_string(static_cast<unsigned long long>(state.iterations()));
        bool loaded = manager.loadModel(model_id, model_path);
        if (!loaded) {
            state.SkipWithError("LLMPluginManager::loadModel failed");
            return;
        }
        benchmark::DoNotOptimize(loaded);
        manager.unloadModel(model_id);
    }
}

BENCHMARK(BM_RealModel_LoadingTime)->Unit(benchmark::kSecond);

BENCHMARK_F(RealLLMBench, RealModel_ContextScaling)(benchmark::State& state) {
    state.SetLabel("end_to_end");
    if (!ensureReady(state)) {
        return;
    }

    auto& manager = llm::LLMPluginManager::instance();
    size_t context_tokens = state.range(0);

    // Generate prompt with approximate token count
    std::string base_prompt = "Summarize the following text:\n";
    std::string long_text = {};

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
        if (!response.success || response.text.empty()) {
            state.SkipWithError(response.error_message.empty()
                                    ? "Context-scaling inference returned no valid output"
                                    : response.error_message.c_str());
            break;
        }
        benchmark::DoNotOptimize(response);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("end_to_end;ctx_tokens=" + std::to_string(context_tokens));
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
