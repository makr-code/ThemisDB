/*
 * ThemisDB | File: bench_llama_cpp_inference.cpp | Version: 0.0.10
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=9; TODO=1, Stub=7, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file bench_llama_cpp_inference.cpp
 * @brief Real llama.cpp inference benchmark via LlamaCppPlugin
 *
 * Validates:
 *   LLAMACPP-PHASE5: Track compatibility for high-throughput concurrent inference calls
 *
 * Scenarios:
 *   - LlamaCppPlugin::generate() — stub path (no model file), measures plugin overhead
 *   - LlamaCppPlugin::generateBatch() — throughput across varying request counts
 *   - LlamaCppPlugin::embed() — embedding vector latency
 *   - LlamaCppPlugin::generateStream() — streaming token dispatch overhead
 *   - Concurrent inference from multiple threads (mutex contention baseline)
 *   - getPerformanceStats() / getMemoryStats() query cost
 *
 * When THEMIS_ENABLE_LLAMA_CPP is defined and a GGUF model is available at
 * THEMIS_BENCH_LLAMA_MODEL_PATH, the benchmarks exercise the real inference
 * path.  Without a model the plugin runs in stub mode and the benchmarks
 * measure pure plugin / dispatch overhead — useful as a regression baseline.
 */

#include <benchmark/benchmark.h>
#include "llama_cpp/llama_cpp_plugin.h"
#include "llm/gguf_loader.h"
#include "llm/llm_plugin_interface.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace themis::llamacpp;
using namespace themis::llm;

// ─── helpers ─────────────────────────────────────────────────────────────────

#define THEMIS_BENCH_STRINGIFY_INNER(x) #x
#define THEMIS_BENCH_STRINGIFY(x) THEMIS_BENCH_STRINGIFY_INNER(x)

static std::string getEnvOrEmpty(const char* key) {
    const char* value = std::getenv(key);
    return (value != nullptr) ? std::string(value) : std::string();
}

static std::string resolveCompileTimeModelPath() {
#ifdef THEMIS_BENCH_LLAMA_MODEL_PATH
    std::string value = THEMIS_BENCH_STRINGIFY(THEMIS_BENCH_LLAMA_MODEL_PATH);

    // Normalize optional quoting so both quoted and unquoted macro forms work.
    if (value.size() >= 2) {
        const char first = value.front();
        const char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            value = value.substr(1, value.size() - 2);
        }
    }
    return value;
#else
    return {};
#endif
}

static std::string resolveModelPath() {
    const auto runtimePath = getEnvOrEmpty("THEMIS_BENCH_LLAMA_MODEL_PATH");
    if (!runtimePath.empty()) {
        return runtimePath;
    }
    return resolveCompileTimeModelPath();
}

static int resolveGpuLayers() {
    const auto envValue = getEnvOrEmpty("THEMIS_BENCH_LLAMA_N_GPU_LAYERS");
    if (envValue.empty()) {
        return 32;
    }
    try {
        return std::max(0, std::stoi(envValue));
    } catch (...) {
        return 32;
    }
}

static bool fileExists(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    std::error_code ec;
    return std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec);
}

static std::string toLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

static bool isGemmaArtifact(const std::string& modelPath, const themis::llm::GGUFMetadata& metadata) {
    const std::string pathLc = toLowerCopy(modelPath);
    const std::string archLc = toLowerCopy(metadata.architecture);
    return pathLc.find("gemma") != std::string::npos || archLc.find("gemma") != std::string::npos;
}

static std::optional<size_t> parseArrayLength(const std::string& encodedValue) {
    constexpr std::string_view kPrefix = "[array:";
    constexpr std::string_view kSuffix = "]";

    if (encodedValue.size() <= kPrefix.size() + kSuffix.size()) {
        return std::nullopt;
    }
    if (encodedValue.compare(0, kPrefix.size(), kPrefix) != 0) {
        return std::nullopt;
    }
    if (encodedValue.back() != ']') {
        return std::nullopt;
    }

    const std::string numberPart = encodedValue.substr(
        kPrefix.size(),
        encodedValue.size() - kPrefix.size() - kSuffix.size());
    if (numberPart.empty()) {
        return std::nullopt;
    }

    try {
        return static_cast<size_t>(std::stoull(numberPart));
    } catch (...) {
        return std::nullopt;
    }
}

static std::optional<size_t> extractTokenCount(const themis::llm::GGUFMetadata& metadata) {
    const auto findCount = [&](const char* key) -> std::optional<size_t> {
        const auto it = metadata.config.find(key);
        if (it == metadata.config.end()) {
            return std::nullopt;
        }
        return parseArrayLength(it->second);
    };

    if (auto tokens = findCount("tokenizer.ggml.tokens")) {
        return tokens;
    }
    if (auto tokens = findCount("tokenizer.tokens")) {
        return tokens;
    }
    return std::nullopt;
}

static const themis::llm::TensorMetadata* findTensorByName(const themis::llm::GGUFMetadata& metadata,
                                                           std::string_view tensorName) {
    for (const auto& tensor : metadata.tensors) {
        if (tensor.name == tensorName) {
            return &tensor;
        }
    }
    return nullptr;
}

static std::optional<std::string> runRealModelPreflight(const std::string& modelPath) {
    const bool gemmaByPath = toLowerCopy(modelPath).find("gemma") != std::string::npos;
    themis::llm::GGUFLoader loader;
    if (!loader.parseFile(modelPath)) {
        if (!gemmaByPath) {
            // Non-Gemma models keep runtime loader fallback behavior.
            return std::nullopt;
        }

        const std::string ggufError = loader.getLastError();
        if (!ggufError.empty()) {
            return "GGUF preflight failed: " + ggufError +
                   " Action: use a llama.cpp-compatible GGUF artifact for this benchmark profile.";
        }
        return "GGUF preflight failed for model file. Action: verify the artifact and use a llama.cpp-compatible GGUF.";
    }

    const auto& metadata = loader.getMetadata();
    if (!gemmaByPath && !isGemmaArtifact(modelPath, metadata)) {
        return std::nullopt;
    }

    const auto* tokenEmbd = findTensorByName(metadata, "token_embd.weight");
    if (tokenEmbd == nullptr) {
        return "Gemma preflight failed: missing tensor token_embd.weight. Action: use a complete GGUF export for this model.";
    }

    if (tokenEmbd->shape.size() < 2) {
        return "Gemma preflight failed: tensor token_embd.weight has invalid rank. Action: re-export GGUF with standard embedding tensor layout.";
    }

    const size_t tokenEmbdRows = static_cast<size_t>(tokenEmbd->shape[0]);
    const size_t tokenEmbdCols = static_cast<size_t>(tokenEmbd->shape[1]);
    const size_t tokenEmbdVocabDim = std::max(tokenEmbdRows, tokenEmbdCols);

    const auto tokenCountOpt = extractTokenCount(metadata);
    if (!tokenCountOpt.has_value()) {
        return std::nullopt;
    }

    const size_t tokenCount = *tokenCountOpt;
    if (tokenEmbdVocabDim != tokenCount) {
        return "Gemma artifact compatibility check failed: tokenizer token count=" + std::to_string(tokenCount) +
               " but token_embd.weight vocab dimension=" + std::to_string(tokenEmbdVocabDim) +
               ". Action: use a matching GGUF conversion/runtime pair (or update llama.cpp to a revision that supports this Gemma export).";
    }

    return std::nullopt;
}

static void setLlamaGpuEvidenceCounters(
    benchmark::State& state,
    const std::string& modelPath,
    int requestedGpuLayers,
    bool warmupSucceeded,
    const nlohmann::json& memoryStats) {

    state.counters["llama_model_path_present"] = modelPath.empty() ? 0.0 : 1.0;
    state.counters["llama_model_file_exists"] = fileExists(modelPath) ? 1.0 : 0.0;
    state.counters["llama_requested_gpu_layers"] = static_cast<double>(requestedGpuLayers);
    state.counters["llama_warmup_generate_success"] = warmupSucceeded ? 1.0 : 0.0;

    const bool modelLoaded =
        memoryStats.contains("model_loaded") && memoryStats["model_loaded"].is_boolean() &&
        memoryStats["model_loaded"].get<bool>();
    state.counters["llama_memory_model_loaded"] = modelLoaded ? 1.0 : 0.0;

    const bool runtimeOffloadRequested =
        memoryStats.contains("runtime_gpu_offload_requested") &&
        memoryStats["runtime_gpu_offload_requested"].is_boolean() &&
        memoryStats["runtime_gpu_offload_requested"].get<bool>();
    const bool runtimeOffloadEffective =
        memoryStats.contains("runtime_gpu_offload_effective") &&
        memoryStats["runtime_gpu_offload_effective"].is_boolean() &&
        memoryStats["runtime_gpu_offload_effective"].get<bool>();
    const bool runtimeAssignedCpu =
        memoryStats.contains("runtime_llama_assigned_cpu_tensors") &&
        memoryStats["runtime_llama_assigned_cpu_tensors"].is_boolean() &&
        memoryStats["runtime_llama_assigned_cpu_tensors"].get<bool>();
    const bool runtimeAssignedNonCpu =
        memoryStats.contains("runtime_llama_assigned_non_cpu_tensors") &&
        memoryStats["runtime_llama_assigned_non_cpu_tensors"].is_boolean() &&
        memoryStats["runtime_llama_assigned_non_cpu_tensors"].get<bool>();

    state.counters["llama_runtime_gpu_offload_requested"] = runtimeOffloadRequested ? 1.0 : 0.0;
    state.counters["llama_runtime_gpu_offload_effective"] = runtimeOffloadEffective ? 1.0 : 0.0;
    state.counters["llama_runtime_assigned_cpu_tensors"] = runtimeAssignedCpu ? 1.0 : 0.0;
    state.counters["llama_runtime_assigned_non_cpu_tensors"] = runtimeAssignedNonCpu ? 1.0 : 0.0;

#ifdef THEMIS_ENABLE_VULKAN
    state.counters["llama_build_has_vulkan"] = 1.0;
#else
    state.counters["llama_build_has_vulkan"] = 0.0;
#endif

#ifdef THEMIS_ENABLE_CUDA
    state.counters["llama_build_has_cuda"] = 1.0;
#else
    state.counters["llama_build_has_cuda"] = 0.0;
#endif

#ifdef THEMIS_ENABLE_HIP
    state.counters["llama_build_has_hip"] = 1.0;
#else
    state.counters["llama_build_has_hip"] = 0.0;
#endif
}

/// Build an InferenceRequest with a given prompt and max_tokens budget.
static InferenceRequest makeRequest(const std::string& prompt,
                                    int max_tokens   = 64,
                                    float temperature = 0.0f) {
    InferenceRequest req;
    req.prompt      = prompt;
    req.max_tokens  = max_tokens;
    req.temperature = temperature;
    req.top_p       = 1.0f;
    req.top_k       = 1;
    return req;
}

// ─── Fixture ─────────────────────────────────────────────────────────────────

class LlamaCppBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        plugin = std::make_unique<LlamaCppPlugin>();
        modelPath = resolveModelPath();
        requestedGpuLayers = resolveGpuLayers();

        if (modelPath.empty()) {
            setupError = "THEMIS_BENCH_LLAMA_MODEL_PATH is required for real-model benchmark profile";
            return;
        }
        if (!fileExists(modelPath)) {
            setupError = "THEMIS_BENCH_LLAMA_MODEL_PATH does not point to an existing file: " + modelPath;
            return;
        }

        if (const auto preflightError = runRealModelPreflight(modelPath); preflightError.has_value()) {
            setupError = *preflightError;
            return;
        }

        nlohmann::json cfg;
        cfg["model_path"] = modelPath;
        cfg["n_threads"] = 4;
        cfg["n_ctx"] = 2048;
        cfg["n_gpu_layers"] = requestedGpuLayers;

        if (!plugin->loadModel(modelPath, cfg)) {
            setupError = "loadModel() failed for model path: " + modelPath;
            return;
        }

        auto warmup = plugin->generate(makeRequest("healthcheck", 8, 0.0f));
        warmupSucceeded = warmup.success;
        if (!warmupSucceeded) {
            setupError = "warmup generate() failed, benchmark would measure stub/error path";
            return;
        }

        memoryStats = plugin->getMemoryStats();
        ready = true;
    }

    void TearDown(const benchmark::State& /*s*/) override {
        plugin->unloadModel();
        plugin.reset();
    }

    bool ensureReady(benchmark::State& state) {
        if (ready) {
            setLlamaGpuEvidenceCounters(
                state,
                modelPath,
                requestedGpuLayers,
                warmupSucceeded,
                memoryStats);
            return true;
        }

        state.SkipWithError(setupError.c_str());
        return false;
    }

    std::unique_ptr<LlamaCppPlugin> plugin;
    std::string modelPath;
    int requestedGpuLayers = 0;
    bool warmupSucceeded = false;
    bool ready = false;
    std::string setupError;
    nlohmann::json memoryStats;
};

// ─── 1. generate() single request latency ────────────────────────────────────

BENCHMARK_F(LlamaCppBenchFixture, Generate_SingleRequest)(benchmark::State& state) {
    if (!ensureReady(state)) {
        return;
    }

    auto req = makeRequest("Summarise the concept of data gravity in one sentence.",
                           /*max_tokens=*/32);

    for (auto _ : state) {
        auto resp = plugin->generate(req);
        benchmark::DoNotOptimize(resp.text);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("generate() real model; max_tokens=32");
}

// ─── 2. generate() prompt-size sweep ─────────────────────────────────────────

static void BM_Generate_PromptSize(benchmark::State& state) {
    LlamaCppPlugin plugin;
    const auto modelPath = resolveModelPath();
    const int requestedGpuLayers = resolveGpuLayers();

    if (modelPath.empty()) {
        state.SkipWithError("THEMIS_BENCH_LLAMA_MODEL_PATH is required for real-model benchmark profile");
        return;
    }
    if (!fileExists(modelPath)) {
        state.SkipWithError(("THEMIS_BENCH_LLAMA_MODEL_PATH does not point to an existing file: " + modelPath).c_str());
        return;
    }

    if (const auto preflightError = runRealModelPreflight(modelPath); preflightError.has_value()) {
        state.SkipWithError(preflightError->c_str());
        return;
    }

    nlohmann::json cfg;
    cfg["model_path"] = modelPath;
    cfg["n_gpu_layers"] = requestedGpuLayers;
    if (!plugin.loadModel(modelPath, cfg)) {
        state.SkipWithError("loadModel() failed for configured THEMIS_BENCH_LLAMA_MODEL_PATH");
        return;
    }

    auto warmup = plugin.generate(makeRequest("healthcheck", 8, 0.0f));
    if (!warmup.success) {
        state.SkipWithError("warmup generate() failed, benchmark would measure stub/error path");
        plugin.unloadModel();
        return;
    }

    const int prompt_tokens = static_cast<int>(state.range(0));
    // Approximate token count: 1 word ≈ 1.3 tokens
    std::string prompt(static_cast<size_t>(prompt_tokens) * 5, 'A');
    auto req = makeRequest(prompt, /*max_tokens=*/16);

    for (auto _ : state) {
        auto resp = plugin.generate(req);
        benchmark::DoNotOptimize(resp.text);
    }

    state.SetItemsProcessed(state.iterations());
    setLlamaGpuEvidenceCounters(
        state,
        modelPath,
        requestedGpuLayers,
        true,
        plugin.getMemoryStats());
    state.SetLabel("prompt ~" + std::to_string(prompt_tokens) + " tokens");
    plugin.unloadModel();
}
BENCHMARK(BM_Generate_PromptSize)->Arg(16)->Arg(64)->Arg(256)->Arg(512);

// ─── 3. generateBatch() throughput ───────────────────────────────────────────

BENCHMARK_F(LlamaCppBenchFixture, GenerateBatch_Throughput)(benchmark::State& state) {
    if (!ensureReady(state)) {
        return;
    }

    const int kBatchSize = static_cast<int>(state.range(0));
    std::vector<InferenceRequest> requests;
    requests.reserve(static_cast<size_t>(kBatchSize));
    for (int i = 0; i < kBatchSize; ++i) {
        requests.push_back(makeRequest("What is " + std::to_string(i) + "?",
                                       /*max_tokens=*/8));
    }

    for (auto _ : state) {
        auto responses = plugin->generateBatch(requests);
        benchmark::DoNotOptimize(responses.size());
    }

    state.SetItemsProcessed(state.iterations() * kBatchSize);
    state.SetLabel("generateBatch size=" + std::to_string(kBatchSize));
}
BENCHMARK_REGISTER_F(LlamaCppBenchFixture, GenerateBatch_Throughput)
    ->Arg(1)->Arg(4)->Arg(8)->Arg(16)->Arg(32)
    ->Unit(benchmark::kMillisecond);

// ─── 4. embed() latency ──────────────────────────────────────────────────────

BENCHMARK_F(LlamaCppBenchFixture, Embed_Latency)(benchmark::State& state) {
    if (!ensureReady(state)) {
        return;
    }

    const std::string text = "ThemisDB is a hybrid database system for enterprise workloads.";

    for (auto _ : state) {
        auto emb = plugin->embed(text);
        benchmark::DoNotOptimize(emb.size());
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("embed() one sentence");
}

// ─── 5. generateStream() overhead ────────────────────────────────────────────

BENCHMARK_F(LlamaCppBenchFixture, GenerateStream_Overhead)(benchmark::State& state) {
    if (!ensureReady(state)) {
        return;
    }

    auto req = makeRequest("List three benefits of column stores.", /*max_tokens=*/32);

    for (auto _ : state) {
        std::atomic<int> token_count{0};
        auto resp = plugin->generateStream(req, [&token_count](const std::string& /*tok*/) {
            token_count.fetch_add(1, std::memory_order_relaxed);
        });
        benchmark::DoNotOptimize(resp.text);
        benchmark::DoNotOptimize(token_count.load());
    }

    state.SetLabel("generateStream() real model callback path");
}

// ─── 6. Concurrent inference — mutex contention baseline ─────────────────────

static void BM_ConcurrentInference(benchmark::State& state) {
    const int kThreads = static_cast<int>(state.range(0));

    LlamaCppPlugin plugin;
    const auto modelPath = resolveModelPath();
    const int requestedGpuLayers = resolveGpuLayers();

    if (modelPath.empty()) {
        state.SkipWithError("THEMIS_BENCH_LLAMA_MODEL_PATH is required for real-model benchmark profile");
        return;
    }
    if (!fileExists(modelPath)) {
        state.SkipWithError(("THEMIS_BENCH_LLAMA_MODEL_PATH does not point to an existing file: " + modelPath).c_str());
        return;
    }

    nlohmann::json cfg;
    cfg["model_path"] = modelPath;
    cfg["n_gpu_layers"] = requestedGpuLayers;
    if (!plugin.loadModel(modelPath, cfg)) {
        state.SkipWithError("loadModel() failed for configured THEMIS_BENCH_LLAMA_MODEL_PATH");
        return;
    }

    auto warmup = plugin.generate(makeRequest("healthcheck", 8, 0.0f));
    if (!warmup.success) {
        state.SkipWithError("warmup generate() failed, benchmark would measure stub/error path");
        plugin.unloadModel();
        return;
    }

    auto req = makeRequest("ping", /*max_tokens=*/4);

    for (auto _ : state) {
        std::atomic<uint64_t> completed{0};
        std::vector<std::thread> threads;
        threads.reserve(static_cast<size_t>(kThreads));

        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([&plugin, &req, &completed]() {
                auto resp = plugin.generate(req);
                benchmark::DoNotOptimize(resp.text);
                completed.fetch_add(1, std::memory_order_relaxed);
            });
        }
        for (auto& th : threads) th.join();
        benchmark::DoNotOptimize(completed.load());
    }

    state.SetItemsProcessed(state.iterations() * kThreads);
    setLlamaGpuEvidenceCounters(
        state,
        modelPath,
        requestedGpuLayers,
        true,
        plugin.getMemoryStats());
    state.SetLabel("concurrent inference threads=" + std::to_string(kThreads));
    plugin.unloadModel();
}
BENCHMARK(BM_ConcurrentInference)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

// ─── 7. getPerformanceStats() / getMemoryStats() query cost ──────────────────

BENCHMARK_F(LlamaCppBenchFixture, StatsQuery)(benchmark::State& state) {
    if (!ensureReady(state)) {
        return;
    }

    for (auto _ : state) {
        auto perf = plugin->getPerformanceStats();
        auto mem  = plugin->getMemoryStats();
        benchmark::DoNotOptimize(perf);
        benchmark::DoNotOptimize(mem);
    }
    state.SetLabel("getPerformanceStats() + getMemoryStats()");
}

// ─── 8. getCapabilities() overhead ───────────────────────────────────────────

BENCHMARK_F(LlamaCppBenchFixture, GetCapabilities)(benchmark::State& state) {
    if (!ensureReady(state)) {
        return;
    }

    for (auto _ : state) {
        auto caps = plugin->getCapabilities();
        benchmark::DoNotOptimize(caps.supports_streaming);
    }
    state.SetLabel("getCapabilities()");
}

// Explicit real-model + GPU evidence path for release audits.
static void BM_LlamaCpp_RealModel_GPUEvidence(benchmark::State& state) {
    LlamaCppPlugin plugin;
    const auto modelPath = resolveModelPath();
    const int requestedGpuLayers = resolveGpuLayers();

    if (modelPath.empty()) {
        state.SkipWithError("THEMIS_BENCH_LLAMA_MODEL_PATH is required for real-model benchmark profile");
        return;
    }
    if (!fileExists(modelPath)) {
        state.SkipWithError(("THEMIS_BENCH_LLAMA_MODEL_PATH does not point to an existing file: " + modelPath).c_str());
        return;
    }
    if (const auto preflightError = runRealModelPreflight(modelPath); preflightError.has_value()) {
        state.SkipWithError(preflightError->c_str());
        return;
    }

    nlohmann::json cfg;
    cfg["model_path"] = modelPath;
    cfg["n_threads"] = 4;
    cfg["n_ctx"] = 2048;
    cfg["n_gpu_layers"] = requestedGpuLayers;
    if (!plugin.loadModel(modelPath, cfg)) {
        state.SkipWithError("loadModel() failed for configured THEMIS_BENCH_LLAMA_MODEL_PATH");
        return;
    }

    auto warmup = plugin.generate(makeRequest("healthcheck", 8, 0.0f));
    if (!warmup.success) {
        state.SkipWithError("warmup generate() failed, benchmark would measure stub/error path");
        plugin.unloadModel();
        return;
    }

    const auto memoryStats = plugin.getMemoryStats();
    const bool runtimeOffloadEffective =
        memoryStats.contains("runtime_gpu_offload_effective") &&
        memoryStats["runtime_gpu_offload_effective"].is_boolean() &&
        memoryStats["runtime_gpu_offload_effective"].get<bool>();
    if (requestedGpuLayers > 0 && !runtimeOffloadEffective) {
        state.SkipWithError("GPU offload requested but no non-CPU tensor assignment detected at runtime");
        plugin.unloadModel();
        return;
    }

    auto req = makeRequest("Provide one short GPU offload status sentence.", 16, 0.0f);
    for (auto _ : state) {
        const auto resp = plugin.generate(req);
        benchmark::DoNotOptimize(resp.text);
    }

    setLlamaGpuEvidenceCounters(
        state,
        modelPath,
        requestedGpuLayers,
        true,
        memoryStats);
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("real-model mandatory profile + gpu evidence");

    plugin.unloadModel();
}
BENCHMARK(BM_LlamaCpp_RealModel_GPUEvidence)->Unit(benchmark::kMillisecond);

// ═══════════════════════════════════════════════════════════════════════════
// LLCPG Gate Benchmarks — Release validation gates (v2.3.0)
// ═══════════════════════════════════════════════════════════════════════════

// LLCPG-1: Time to First Token (stub path baseline)
// Stub gate: ≤ 5 ms per call (plugin overhead only, no model)
// Production gate: ≤ 1100 ms P95 on A10G with real GGUF model
static void LLCPG1_TTFT_Stub_Baseline(benchmark::State& state) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", nlohmann::json::object());

    InferenceRequest req;
    req.prompt = "What is the capital of France? Please provide a detailed answer.";
    req.max_tokens = 50;

    for (auto _ : state) {
        auto result = plugin.generate(req);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["GATE_stub_max_ms"] = 5.0;
    state.counters["GATE_prod_p95_ms"] = 1100.0;
}
BENCHMARK(LLCPG1_TTFT_Stub_Baseline)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Iterations(20);

// LLCPG-2: Batch Embedding Throughput (100-document stub baseline)
// Production gate: ≥ 8500 tok/s
static void LLCPG2_BatchEmbedding_Stub(benchmark::State& state) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", nlohmann::json::object());

    constexpr int kDocsPerBatch = 100;
    // ~50 tokens per doc; typical embedding input size
    const std::string doc_text =
        "This is a sample document for embedding throughput measurement. "
        "It contains approximately fifty tokens of representative text.";
    const std::vector<std::string> docs(kDocsPerBatch, doc_text);

    for (auto _ : state) {
        for (const auto& doc : docs) {
            auto vec = plugin.embed(doc);
            benchmark::DoNotOptimize(vec);
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(kDocsPerBatch) * state.iterations());
    state.counters["docs_per_batch"] = kDocsPerBatch;
    state.counters["GATE_prod_min_tok_s"] = 8500.0;
}
BENCHMARK(LLCPG2_BatchEmbedding_Stub)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Iterations(10);

// LLCPG-3: LoRA Registry Load/Unload P99 Latency
// Gate: ≤ 75 ms P99 (metadata load, no real adapter file needed)
static void LLCPG3_LoRALoad_P99(benchmark::State& state) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", nlohmann::json::object());

    int64_t iteration = 0;
    for (auto _ : state) {
        const std::string id = "bench_lora_" + std::to_string(++iteration);
        // loadLoRA signature: (lora_id, lora_path, scale)
        bool ok = plugin.loadLoRA(id, "/tmp/nonexistent_lora.bin", 1.0f);
        benchmark::DoNotOptimize(ok);
        bool unloaded = plugin.unloadLoRA(id);
        benchmark::DoNotOptimize(unloaded);
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["GATE_p99_max_ms"] = 75.0;
}
BENCHMARK(LLCPG3_LoRALoad_P99)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(100);

// LLCPG-4: Regression Baseline Snapshot
// Records throughput baseline for regression gating.
// Gate: ≤ 8 % throughput regression vs. this baseline run.
// Compare using: benchmark compare tool or manual ops/s check.
static void LLCPG4_RegressionBaseline(benchmark::State& state) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", nlohmann::json::object());

    InferenceRequest req;
    req.prompt = "regression baseline";
    req.max_tokens = 10;

    for (auto _ : state) {
        auto result = plugin.generate(req);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["GATE_regression_pct_max"] = 8.0;
    state.counters["baseline_mode_stub"] = 1.0;
}
BENCHMARK(LLCPG4_RegressionBaseline)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->MinTime(1.0);
