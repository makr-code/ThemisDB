/**
 * @file llm_bench.cpp
 * @brief Performance benchmarks for speculative decoding and the LLM inference
 *        engines (AsyncInferenceEngine, InferenceEngineEnhanced).
 *
 * Executed automatically on every PR that touches:
 *   - async_inference_engine.cpp
 *   - inference_engine_enhanced.cpp
 *
 * Acceptance criterion: tokens/sec regression < 5 % vs. the baseline stored in
 * benchmarks/baselines/.
 *
 * Benchmarks:
 *  BM_SpeculativeDecoder_Softmax        — softmax throughput at various vocab sizes
 *  BM_SpeculativeDecoder_AdjustedDist   — adjusted-distribution throughput
 *  BM_SpeculativeDecoder_Verify_AllAccepted  — verify() when all K tokens are accepted
 *  BM_SpeculativeDecoder_Verify_AllRejected  — verify() when first token is rejected
 *  BM_SpeculativeDecoder_Verify_VaryK   — verify() across K = 1..16
 *  BM_SpeculativeDecoder_Verify_VaryVocab — verify() across vocab sizes
 *  BM_SpeculativeDecoder_ThreadSafety   — concurrent verify() from N threads
 *  BM_InferenceEngine_Speculative_vs_Standard — end-to-end engine comparison
 */

#include <benchmark/benchmark.h>

#include "llm/speculative_decoder.h"
#include "llm/async_inference_engine.h"
#include "llm/inference_engine_enhanced.h"
#include "llm/llm_plugin_interface.h"
#include "llm/openai_compat_adapter.h"

#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <numeric>
#include <algorithm>

using namespace themis::llm;

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Build a logit vector with a strong peak at `peak_token`.
static std::vector<float> makePeakedLogits(size_t vocab_size, size_t peak_token,
                                            float peak = 10.0f,
                                            float baseline = -10.0f)
{
    std::vector<float> logits(vocab_size, baseline);
    if (peak_token < vocab_size) logits[peak_token] = peak;
    return logits;
}

/// Build a draft/target logit matrix where both models strongly agree on token 0.
/// Returns (draft_tokens, draft_logits, target_logits) ready for verify().
static void buildAllAcceptedInput(
    size_t K,
    size_t vocab_size,
    std::vector<int>&                   draft_tokens,
    std::vector<std::vector<float>>&    draft_logits,
    std::vector<std::vector<float>>&    target_logits)
{
    draft_tokens.assign(K, 0);
    auto row = makePeakedLogits(vocab_size, /*peak_token=*/0);
    draft_logits.assign(K, row);
    target_logits.assign(K + 1, row);
}

/// Build input where the draft always proposes token 0 but the target
/// strongly prefers token 1 — first token is always rejected.
static void buildAllRejectedInput(
    size_t K,
    size_t vocab_size,
    std::vector<int>&                   draft_tokens,
    std::vector<std::vector<float>>&    draft_logits,
    std::vector<std::vector<float>>&    target_logits)
{
    draft_tokens.assign(K, 0);
    auto draft_row  = makePeakedLogits(vocab_size, /*peak_token=*/0);
    auto target_row = makePeakedLogits(vocab_size, /*peak_token=*/1);
    draft_logits.assign(K, draft_row);
    target_logits.assign(K + 1, target_row);
}

// ─────────────────────────────────────────────────────────────────────────────
// Minimal mock plugin (no real model inference — returns immediately)
// ─────────────────────────────────────────────────────────────────────────────

class MockPlugin : public ILLMPlugin {
public:
    explicit MockPlugin(const std::string& model_id, int latency_us = 100,
                        int tokens_per_response = 10)
        : model_id_(model_id), latency_us_(latency_us),
          tokens_per_response_(tokens_per_response) {}

    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    bool isModelLoaded() const override { return true; }
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo mi;
        mi.model_id   = model_id_;
        mi.is_loaded  = true;
        mi.vocab_size = 32000;
        return mi;
    }
    InferenceResponse generate(const InferenceRequest& req) override {
        if (latency_us_ > 0)
            std::this_thread::sleep_for(std::chrono::microseconds(latency_us_));
        InferenceResponse r;
        r.request_id        = req.request_id;
        r.text              = "ok";
        r.model_id          = model_id_;
        r.tokens_generated  = tokens_per_response_;
        r.inference_time_ms = latency_us_ / 1000.0f;
        r.latency_ms        = latency_us_ / 1000;
        r.tokens_per_second = (latency_us_ > 0)
            ? (tokens_per_response_ * 1e6f / latency_us_)
            : 0.0f;
        return r;
    }
    InferenceResponse generateRAG(const RAGContext&,
                                   const InferenceRequest& req) override {
        return generate(req);
    }
    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities getCapabilities() const override { return {}; }
    json getMemoryStats() const override { return json::object(); }
    json getPerformanceStats() const override { return json::object(); }
    bool loadLoRA(const std::string&, const std::string&, float) override {
        return true;
    }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&,
                    const std::vector<uint8_t>&) override { return true; }

private:
    std::string model_id_;
    int latency_us_;
    int tokens_per_response_;
};

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════
// BM_SpeculativeDecoder_Softmax
// Measure softmax throughput at different vocab sizes.
// ═══════════════════════════════════════════════════════════════════

static void BM_SpeculativeDecoder_Softmax(benchmark::State& state) {
    const size_t vocab_size = static_cast<size_t>(state.range(0));
    std::vector<float> logits(vocab_size, 0.0f);
    // Give the vector non-trivial values.
    std::iota(logits.begin(), logits.end(), -static_cast<float>(vocab_size) / 2.0f);

    for (auto _ : state) {
        auto probs = SpeculativeDecoder::softmax(logits);
        benchmark::DoNotOptimize(probs.data());
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(vocab_size));
    state.SetLabel("softmax_tokens");
}
BENCHMARK(BM_SpeculativeDecoder_Softmax)
    ->Arg(1024)
    ->Arg(4096)
    ->Arg(32000)
    ->Arg(128256);  // LLaMA-3 vocab size

// ═══════════════════════════════════════════════════════════════════
// BM_SpeculativeDecoder_AdjustedDist
// Measure adjustedDistribution() throughput (used on rejection).
// ═══════════════════════════════════════════════════════════════════

static void BM_SpeculativeDecoder_AdjustedDist(benchmark::State& state) {
    const size_t vocab_size = static_cast<size_t>(state.range(0));

    // Pre-compute realistic probability vectors once.
    std::vector<float> target_probs = SpeculativeDecoder::softmax(
        std::vector<float>(vocab_size, 0.0f));
    std::vector<float> draft_probs = SpeculativeDecoder::softmax(
        makePeakedLogits(vocab_size, 0));

    for (auto _ : state) {
        auto adj = SpeculativeDecoder::adjustedDistribution(target_probs, draft_probs);
        benchmark::DoNotOptimize(adj.data());
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(vocab_size));
    state.SetLabel("adj_dist_tokens");
}
BENCHMARK(BM_SpeculativeDecoder_AdjustedDist)
    ->Arg(1024)
    ->Arg(32000)
    ->Arg(128256);

// ═══════════════════════════════════════════════════════════════════
// BM_SpeculativeDecoder_Verify_AllAccepted
// Best-case: all K draft tokens are accepted, one bonus token sampled.
// ═══════════════════════════════════════════════════════════════════

static void BM_SpeculativeDecoder_Verify_AllAccepted(benchmark::State& state) {
    constexpr size_t K          = 4;
    constexpr size_t vocab_size = 32000;

    std::vector<int>                     draft_tokens;
    std::vector<std::vector<float>>      draft_logits;
    std::vector<std::vector<float>>      target_logits;
    buildAllAcceptedInput(K, vocab_size, draft_tokens, draft_logits, target_logits);

    SpeculativeDecoder::Config cfg;
    cfg.k        = K;
    cfg.rng_seed = 42;
    SpeculativeDecoder dec(cfg);

    for (auto _ : state) {
        auto result = dec.verify(draft_tokens, draft_logits, target_logits);
        benchmark::DoNotOptimize(result.num_accepted);
        benchmark::ClobberMemory();
    }

    // Report effective tokens decoded per second (accepted + bonus).
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(K + 1));
    state.SetLabel("tokens_decoded_all_accepted");
}
BENCHMARK(BM_SpeculativeDecoder_Verify_AllAccepted)->Threads(1)->Threads(4);

// ═══════════════════════════════════════════════════════════════════
// BM_SpeculativeDecoder_Verify_AllRejected
// Worst-case: first token is always rejected (correction token sampled).
// ═══════════════════════════════════════════════════════════════════

static void BM_SpeculativeDecoder_Verify_AllRejected(benchmark::State& state) {
    constexpr size_t K          = 4;
    constexpr size_t vocab_size = 32000;

    std::vector<int>                     draft_tokens;
    std::vector<std::vector<float>>      draft_logits;
    std::vector<std::vector<float>>      target_logits;
    buildAllRejectedInput(K, vocab_size, draft_tokens, draft_logits, target_logits);

    SpeculativeDecoder::Config cfg;
    cfg.k        = K;
    cfg.rng_seed = 42;
    SpeculativeDecoder dec(cfg);

    for (auto _ : state) {
        auto result = dec.verify(draft_tokens, draft_logits, target_logits);
        benchmark::DoNotOptimize(result.bonus_token);
        benchmark::ClobberMemory();
    }

    // Report 1 token per step (the correction token on first rejection).
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("tokens_decoded_all_rejected");
}
BENCHMARK(BM_SpeculativeDecoder_Verify_AllRejected)->Threads(1)->Threads(4);

// ═══════════════════════════════════════════════════════════════════
// BM_SpeculativeDecoder_Verify_VaryK
// Vary speculative window K from 1 to 16 (vocab fixed at 32000).
// ═══════════════════════════════════════════════════════════════════

static void BM_SpeculativeDecoder_Verify_VaryK(benchmark::State& state) {
    const size_t K          = static_cast<size_t>(state.range(0));
    constexpr size_t vocab_size = 32000;

    std::vector<int>                     draft_tokens;
    std::vector<std::vector<float>>      draft_logits;
    std::vector<std::vector<float>>      target_logits;
    buildAllAcceptedInput(K, vocab_size, draft_tokens, draft_logits, target_logits);

    SpeculativeDecoder::Config cfg;
    cfg.k        = K;
    cfg.rng_seed = 1;
    SpeculativeDecoder dec(cfg);

    for (auto _ : state) {
        auto result = dec.verify(draft_tokens, draft_logits, target_logits);
        benchmark::DoNotOptimize(result.num_accepted);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(K));
    state.SetLabel("verify_vary_k");
}
BENCHMARK(BM_SpeculativeDecoder_Verify_VaryK)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16);

// ═══════════════════════════════════════════════════════════════════
// BM_SpeculativeDecoder_Verify_VaryVocab
// Vary vocab size with K fixed at 4.
// ═══════════════════════════════════════════════════════════════════

static void BM_SpeculativeDecoder_Verify_VaryVocab(benchmark::State& state) {
    constexpr size_t K    = 4;
    const size_t vocab_size = static_cast<size_t>(state.range(0));

    std::vector<int>                     draft_tokens;
    std::vector<std::vector<float>>      draft_logits;
    std::vector<std::vector<float>>      target_logits;
    buildAllAcceptedInput(K, vocab_size, draft_tokens, draft_logits, target_logits);

    SpeculativeDecoder::Config cfg;
    cfg.k        = K;
    cfg.rng_seed = 1;
    SpeculativeDecoder dec(cfg);

    for (auto _ : state) {
        auto result = dec.verify(draft_tokens, draft_logits, target_logits);
        benchmark::DoNotOptimize(result.num_accepted);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(vocab_size));
    state.SetLabel("verify_vary_vocab");
}
BENCHMARK(BM_SpeculativeDecoder_Verify_VaryVocab)
    ->Arg(1024)
    ->Arg(4096)
    ->Arg(32000)
    ->Arg(128256);

// ═══════════════════════════════════════════════════════════════════
// BM_SpeculativeDecoder_ThreadSafety
// Concurrent verify() calls share a single SpeculativeDecoder instance.
// Measures throughput under N-thread contention.
// ═══════════════════════════════════════════════════════════════════

static void BM_SpeculativeDecoder_ThreadSafety(benchmark::State& state) {
    constexpr size_t K          = 4;
    constexpr size_t vocab_size = 32000;

    std::vector<int>                     draft_tokens;
    std::vector<std::vector<float>>      draft_logits;
    std::vector<std::vector<float>>      target_logits;
    buildAllAcceptedInput(K, vocab_size, draft_tokens, draft_logits, target_logits);

    // Shared decoder — thread-safety exercised by the benchmark framework.
    static SpeculativeDecoder shared_dec([]() {
        SpeculativeDecoder::Config cfg;
        cfg.k        = K;
        cfg.rng_seed = 0;  // 0 triggers random_device seeding (appropriate for concurrent tests)
        return cfg;
    }());

    for (auto _ : state) {
        auto result = shared_dec.verify(draft_tokens, draft_logits, target_logits);
        benchmark::DoNotOptimize(result.num_accepted);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(K));
    state.SetLabel("thread_safe_verify");
}
BENCHMARK(BM_SpeculativeDecoder_ThreadSafety)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8);

// ═══════════════════════════════════════════════════════════════════
// BM_InferenceEngine_Speculative_vs_Standard
// End-to-end comparison: speculative decoding enabled vs. disabled.
// Uses a mock plugin (no real model; measures engine overhead only).
// ═══════════════════════════════════════════════════════════════════

static void BM_InferenceEngine_Standard(benchmark::State& state) {
    InferenceEngineEnhanced::Config cfg;
    cfg.enable_speculative_decoding = false;
    cfg.num_worker_threads          = 1;
    cfg.enable_context_caching      = false;
    cfg.batch_timeout_ms            = 20;

    InferenceEngineEnhanced engine(cfg);
    engine.registerModel("target",
                         std::make_shared<MockPlugin>("target", /*latency_us=*/200));
    engine.start();

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.base_request.prompt    = "benchmark prompt";
    req.base_request.max_tokens = 50;
    req.allow_caching          = false;
    req.preferred_model_id     = "target";

    size_t req_counter = 0;

    for (auto _ : state) {
        req.request_id = "std_" + std::to_string(req_counter++);
        auto handle   = engine.submit(req);
        auto response = handle.get();
        benchmark::DoNotOptimize(response.text.size());
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("standard_inference");

    engine.shutdown();
}
BENCHMARK(BM_InferenceEngine_Standard)->Iterations(10);

static void BM_InferenceEngine_Speculative(benchmark::State& state) {
    InferenceEngineEnhanced::Config cfg;
    cfg.enable_speculative_decoding = true;
    cfg.speculative_draft_tokens    = static_cast<size_t>(state.range(0));
    cfg.speculative_draft_model_id  = "draft";
    cfg.num_worker_threads          = 1;
    cfg.enable_context_caching      = false;
    cfg.batch_timeout_ms            = 20;

    InferenceEngineEnhanced engine(cfg);
    engine.registerModel("target",
                         std::make_shared<MockPlugin>("target", /*latency_us=*/200));
    engine.registerModel("draft",
                         std::make_shared<MockPlugin>("draft",  /*latency_us=*/50));
    engine.start();

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.base_request.prompt    = "benchmark prompt";
    req.base_request.max_tokens = 50;
    req.allow_caching          = false;
    req.preferred_model_id     = "target";

    size_t req_counter = 0;

    for (auto _ : state) {
        req.request_id = "spec_" + std::to_string(req_counter++);
        auto handle   = engine.submit(req);
        auto response = handle.get();
        benchmark::DoNotOptimize(response.text.size());
        benchmark::ClobberMemory();
    }

    auto stats = engine.getStatistics();
    state.counters["spec_steps"]     = benchmark::Counter(
        static_cast<double>(stats.speculative_steps));
    state.counters["accepted_tokens"] = benchmark::Counter(
        static_cast<double>(stats.speculative_accepted_tokens));
    state.counters["avg_accept_rate"] = benchmark::Counter(
        stats.speculative_avg_acceptance_rate);

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("speculative_inference");

    engine.shutdown();
}
BENCHMARK(BM_InferenceEngine_Speculative)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Iterations(10);

// ═══════════════════════════════════════════════════════════════════
// BM_AsyncEngine_TokensPerSec
// Measures tokens/sec throughput through AsyncInferenceEngine.
// Reports both Google Benchmark's items/sec (tokens) and the engine's
// own tokens_per_second counter from getWorkerStats().
// Acceptance criterion: regression < 5% vs. stored baseline.
// ═══════════════════════════════════════════════════════════════════

static void BM_AsyncEngine_TokensPerSec(benchmark::State& state) {
    constexpr int kLatencyUs       = 200;
    constexpr int kTokensPerResp   = 50;

    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = static_cast<size_t>(state.range(0));
    cfg.max_queue_size     = 1000;

    AsyncInferenceEngine engine(
        std::make_shared<MockPlugin>("model", kLatencyUs, kTokensPerResp), cfg);

    InferenceRequest req;
    req.prompt     = "benchmark prompt";
    req.max_tokens = kTokensPerResp;

    size_t counter = 0;
    int64_t total_tokens = 0;

    for (auto _ : state) {
        req.request_id = "tps_" + std::to_string(counter++);
        auto handle   = engine.submit(req);
        auto response = handle.get();
        total_tokens += response.tokens_generated;
        benchmark::DoNotOptimize(response.tokens_generated);
        benchmark::ClobberMemory();
    }

    // Report actual tokens generated as items processed → benchmark computes tokens/sec.
    state.SetItemsProcessed(total_tokens);
    state.SetLabel("async_tokens_per_sec");

    // Expose engine-computed tokens/sec for CI regression tracking.
    auto ws = engine.getWorkerStats();
    if (ws.contains("tokens_per_second")) {
        state.counters["engine_tokens_per_sec"] =
            benchmark::Counter(static_cast<double>(ws["tokens_per_second"]));
    }

    engine.shutdown();
}
BENCHMARK(BM_AsyncEngine_TokensPerSec)
    ->Arg(1)
    ->Arg(2)
    ->Iterations(20);

// ═══════════════════════════════════════════════════════════════════
// BM_AsyncEngine_LatencyP99
// Submits a burst of requests to AsyncInferenceEngine and reports
// the p99 end-to-end inference latency recorded by the engine.
// ═══════════════════════════════════════════════════════════════════

static void BM_AsyncEngine_LatencyP99(benchmark::State& state) {
    constexpr int kLatencyUs     = 500;   // 0.5 ms simulated inference
    constexpr int kBurstRequests = 200;   // enough samples for stable p99

    AsyncInferenceEngine::Config cfg;
    cfg.num_worker_threads = 2;
    cfg.max_queue_size     = 1000;

    AsyncInferenceEngine engine(
        std::make_shared<MockPlugin>("model", kLatencyUs), cfg);

    InferenceRequest req;
    req.prompt     = "latency benchmark";
    req.max_tokens = 10;

    size_t counter = 0;

    for (auto _ : state) {
        // Submit a burst; collect per-request latency for in-benchmark p99.
        std::vector<double> latencies;
        latencies.reserve(kBurstRequests);

        for (int i = 0; i < kBurstRequests; ++i) {
            req.request_id        = "p99_" + std::to_string(counter++);
            auto t0               = std::chrono::steady_clock::now();
            auto handle           = engine.submit(req);
            auto response         = handle.get();
            auto t1               = std::chrono::steady_clock::now();
            double latency_ms     = std::chrono::duration<double, std::milli>(t1 - t0).count();
            latencies.push_back(latency_ms);
            benchmark::DoNotOptimize(response.text.size());
        }

        // Compute in-benchmark p99 for this iteration.
        std::sort(latencies.begin(), latencies.end());
        size_t p99_idx      = static_cast<size_t>(latencies.size() * 0.99);
        double p99_bench_ms = latencies[std::min(p99_idx, latencies.size() - 1)];
        benchmark::DoNotOptimize(p99_bench_ms);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * kBurstRequests);
    state.SetLabel("async_latency_p99");

    // Report engine-computed p99 latency.
    auto ws = engine.getWorkerStats();
    if (ws.contains("p99_latency_ms")) {
        state.counters["latency_p99_ms"] =
            benchmark::Counter(static_cast<double>(ws["p99_latency_ms"]));
    }

    engine.shutdown();
}
BENCHMARK(BM_AsyncEngine_LatencyP99)->Iterations(3);

// ═══════════════════════════════════════════════════════════════════
// BM_EnhancedEngine_TokensPerSec
// Measures tokens/sec throughput through InferenceEngineEnhanced.
// Targets the `tokens_per_second` field in Statistics (previously stub).
// ═══════════════════════════════════════════════════════════════════

static void BM_EnhancedEngine_TokensPerSec(benchmark::State& state) {
    constexpr int kLatencyUs     = 200;
    constexpr int kTokensPerResp = 50;

    InferenceEngineEnhanced::Config cfg;
    cfg.num_worker_threads     = static_cast<size_t>(state.range(0));
    cfg.enable_context_caching = false;
    cfg.batch_timeout_ms       = 20;

    InferenceEngineEnhanced engine(cfg);
    engine.registerModel("model",
        std::make_shared<MockPlugin>("model", kLatencyUs, kTokensPerResp));
    engine.start();

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.base_request.prompt    = "benchmark prompt";
    req.base_request.max_tokens = kTokensPerResp;
    req.allow_caching          = false;
    req.preferred_model_id     = "model";

    size_t counter = 0;
    int64_t total_tokens = 0;

    for (auto _ : state) {
        req.request_id = "etps_" + std::to_string(counter++);
        auto handle   = engine.submit(req);
        auto response = handle.get();
        total_tokens += response.tokens_generated;
        benchmark::DoNotOptimize(response.tokens_generated);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(total_tokens);
    state.SetLabel("enhanced_tokens_per_sec");

    // Expose engine-computed tokens_per_second for CI regression tracking.
    auto stats = engine.getStatistics();
    state.counters["engine_tokens_per_sec"] =
        benchmark::Counter(stats.tokens_per_second);

    engine.shutdown();
}
BENCHMARK(BM_EnhancedEngine_TokensPerSec)
    ->Arg(1)
    ->Arg(2)
    ->Iterations(20);

// ═══════════════════════════════════════════════════════════════════
// BM_EnhancedEngine_LatencyP99
// Submits a burst of requests to InferenceEngineEnhanced and reports
// the p99 latency from Statistics.
// ═══════════════════════════════════════════════════════════════════

static void BM_EnhancedEngine_LatencyP99(benchmark::State& state) {
    constexpr int kLatencyUs     = 500;
    constexpr int kBurstRequests = 200;

    InferenceEngineEnhanced::Config cfg;
    cfg.num_worker_threads     = 2;
    cfg.enable_context_caching = false;
    cfg.batch_timeout_ms       = 20;

    InferenceEngineEnhanced engine(cfg);
    engine.registerModel("model",
        std::make_shared<MockPlugin>("model", kLatencyUs));
    engine.start();

    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.base_request.prompt    = "latency benchmark";
    req.base_request.max_tokens = 10;
    req.allow_caching          = false;
    req.preferred_model_id     = "model";

    size_t counter = 0;

    for (auto _ : state) {
        for (int i = 0; i < kBurstRequests; ++i) {
            req.request_id = "ep99_" + std::to_string(counter++);
            auto handle   = engine.submit(req);
            auto response = handle.get();
            benchmark::DoNotOptimize(response.text.size());
        }
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * kBurstRequests);
    state.SetLabel("enhanced_latency_p99");

    // Report engine-computed p99 latency from Statistics.
    auto stats = engine.getStatistics();
    state.counters["latency_p99_ms"] =
        benchmark::Counter(stats.p99_latency_ms);
    state.counters["latency_p95_ms"] =
        benchmark::Counter(stats.p95_latency_ms);

    engine.shutdown();
}
BENCHMARK(BM_EnhancedEngine_LatencyP99)->Iterations(3);

// ═══════════════════════════════════════════════════════════════════
// BM_OpenAICompatAdapter_ParseRequest
// Measures the overhead of OpenAICompatAdapter::parseRequest() on a
// typical multi-message chat body.
//
// Acceptance criterion (AC-6): adapter serialisation/deserialisation
// overhead ≤ 2 ms vs direct submitRequest() call.
// ═══════════════════════════════════════════════════════════════════

static void BM_OpenAICompatAdapter_ParseRequest(benchmark::State& state) {
    using json = nlohmann::json;
    // Representative chat body: 3 messages including a system prompt.
    const json body = {
        {"model",    "llama3"},
        {"messages", json::array({
            {{"role","system"},    {"content","You are a helpful assistant."}},
            {{"role","user"},      {"content","What is the capital of France?"}},
            {{"role","assistant"}, {"content","Paris."}},
            {{"role","user"},      {"content","And Germany?"}}
        })},
        {"temperature", 0.7},
        {"max_tokens",  512},
        {"stop",        json::array({"END", "STOP"})},
        {"stream",      false}
    };

    for (auto _ : state) {
        auto result = OpenAICompatAdapter::parseRequest(body);
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }

    state.SetLabel("parse_request");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OpenAICompatAdapter_ParseRequest)->Iterations(1000);

// ─────────────────────────────────────────────────────────────────────────────

static void BM_OpenAICompatAdapter_BuildResponse(benchmark::State& state) {
    InferenceResponse resp;
    resp.text              = "Paris is the capital of France.";
    resp.model_id          = "llama3";
    resp.tokens_prompt     = 25;
    resp.tokens_generated  = 8;

    const std::string cid = OpenAICompatAdapter::generateCompletionId();

    for (auto _ : state) {
        auto j = OpenAICompatAdapter::buildResponse(resp, "llama3", cid);
        benchmark::DoNotOptimize(j);
        benchmark::ClobberMemory();
    }

    state.SetLabel("build_response");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OpenAICompatAdapter_BuildResponse)->Iterations(1000);

// ─────────────────────────────────────────────────────────────────────────────
// Round-trip: parse + build_response; the combined figure must remain ≤ 2 ms.
// ─────────────────────────────────────────────────────────────────────────────

static void BM_OpenAICompatAdapter_RoundTrip(benchmark::State& state) {
    using json = nlohmann::json;
    const json body = {
        {"model",       "llama3"},
        {"messages",    json::array({
            {{"role","system"}, {"content","Be concise."}},
            {{"role","user"},   {"content","What is 2+2?"}}
        })},
        {"temperature", 0.0},
        {"max_tokens",  16}
    };

    const std::string cid = OpenAICompatAdapter::generateCompletionId();

    InferenceResponse resp;
    resp.text             = "4";
    resp.model_id         = "llama3";
    resp.tokens_prompt    = 12;
    resp.tokens_generated = 1;

    std::vector<double> round_trip_ns;
    round_trip_ns.reserve(1000);

    for (auto _ : state) {
        auto t0 = std::chrono::steady_clock::now();

        auto parse = OpenAICompatAdapter::parseRequest(body);
        auto j     = OpenAICompatAdapter::buildResponse(resp, "llama3", cid);

        auto t1 = std::chrono::steady_clock::now();
        round_trip_ns.push_back(
            static_cast<double>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));

        benchmark::DoNotOptimize(parse);
        benchmark::DoNotOptimize(j);
        benchmark::ClobberMemory();
    }

    // Compute mean and p99 and expose as benchmark counters
    if (!round_trip_ns.empty()) {
        std::sort(round_trip_ns.begin(), round_trip_ns.end());
        const double mean_us = std::accumulate(
            round_trip_ns.begin(), round_trip_ns.end(), 0.0) /
            (static_cast<double>(round_trip_ns.size()) * 1000.0);

        const size_t p99_idx =
            static_cast<size_t>(round_trip_ns.size() * 0.99);
        const double p99_us  = round_trip_ns[p99_idx] / 1000.0;

        state.counters["mean_us"]           = benchmark::Counter(mean_us);
        state.counters["p99_us"]            = benchmark::Counter(p99_us);
        // AC-6 assertion: p99 must remain well below 2000 µs (2 ms)
        state.counters["p99_within_2ms"]    =
            benchmark::Counter(p99_us < 2000.0 ? 1.0 : 0.0);
    }

    state.SetLabel("round_trip");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OpenAICompatAdapter_RoundTrip)->Iterations(1000);

// ═══════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════

BENCHMARK_MAIN();
