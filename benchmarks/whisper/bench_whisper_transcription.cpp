/*
 * ThemisDB | File: bench_whisper_transcription.cpp | Version: 0.0.10
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=13; TODO=1, Stub=11, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file bench_whisper_transcription.cpp
 * @brief Whisper transcription benchmark — plugin overhead and CLI parity harness
 *
 * Validates:
 *   WHISPER-PHASE5: Define API limits for file size and sample-rate ranges
 *
 * Scenarios:
 *   - WhisperPlugin::transcribe() latency in stub mode (plugin overhead)
 *   - WhisperPlugin::detectLanguage() latency
 *   - transcribeFile() call overhead (no real file I/O on CI)
 *   - WhisperStubTranscriber direct — raw interface dispatch cost
 *   - Varying PCM buffer sizes (16 kHz, 8 kHz) — API boundary coverage
 *   - Statistics query cost
 *   - CLI comparison harness: logs expected real-model latency budget
 *     (< 2× real-time for 30-second audio on a 4-core CPU)
 *
 * When THEMIS_ENABLE_WHISPER is defined and THEMIS_BENCH_WHISPER_MODEL_PATH
 * points to a valid GGML model the benchmarks exercise the real inference path.
 * Without a model the stub path is used and results represent pure dispatch
 * overhead — a useful CI regression guard.
 */

#include <benchmark/benchmark.h>
#include "whisper/whisper_plugin.h"
#include "whisper/whisper_transcriber.h"
#include "whisper/whisper_config.h"

#include <cstdlib>
#include <string>
#include <vector>
#include <random>

using namespace themis::whisper;
using namespace themis::audio;

// ─── helpers ─────────────────────────────────────────────────────────────────

#define THEMIS_BENCH_STRINGIFY_INNER(x) #x
#define THEMIS_BENCH_STRINGIFY(x) THEMIS_BENCH_STRINGIFY_INNER(x)

static std::string resolveCompileTimeWhisperModelPath() {
#ifdef THEMIS_BENCH_WHISPER_MODEL_PATH
    std::string value = THEMIS_BENCH_STRINGIFY(THEMIS_BENCH_WHISPER_MODEL_PATH);
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

static std::string resolveWhisperModelPath() {
    const char* runtime = std::getenv("THEMIS_BENCH_WHISPER_MODEL_PATH");
    if (runtime != nullptr && *runtime != '\0') {
        return runtime;
    }
    return resolveCompileTimeWhisperModelPath();
}

/// Generate synthetic PCM float samples (silence + low-energy noise).
static std::vector<float> makePCM(int duration_ms, float sample_rate = 16000.0f) {
    int n = static_cast<int>(sample_rate * duration_ms / 1000);
    std::vector<float> pcm(static_cast<size_t>(n), 0.0f);

    std::mt19937 rng(0xDEADC0DE);
    std::uniform_real_distribution<float> dist(-0.001f, 0.001f);
    for (auto& s : pcm) s = dist(rng);

    return pcm;
}

// ─── WhisperPlugin fixture ────────────────────────────────────────────────────

class WhisperPluginFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        plugin = std::make_unique<WhisperPlugin>();

        nlohmann::json cfg;
        cfg["language"]       = "auto";
        cfg["n_threads"]      = 4;
        cfg["translate"]      = false;
        cfg["print_progress"] = false;
        plugin->initialize(resolveWhisperModelPath(), cfg);
        // initialize() may return false in stub mode; plugin remains usable.

        pcm_1s   = makePCM(1000);
        pcm_5s   = makePCM(5000);
        pcm_30s  = makePCM(30000);
    }

    void TearDown(const benchmark::State& /*s*/) override {
        plugin.reset();
    }

    std::unique_ptr<WhisperPlugin> plugin;
    std::vector<float>             pcm_1s;
    std::vector<float>             pcm_5s;
    std::vector<float>             pcm_30s;
};

// ─── 1. transcribe() — 1-second chunk ────────────────────────────────────────

BENCHMARK_F(WhisperPluginFixture, Transcribe_1s)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = plugin->transcribe(pcm_1s, 16000.0f);
        benchmark::DoNotOptimize(result.text);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("transcribe() 1 s @ 16 kHz — stub or real model");
}

// ─── 2. transcribe() — 5-second chunk ────────────────────────────────────────

BENCHMARK_F(WhisperPluginFixture, Transcribe_5s)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = plugin->transcribe(pcm_5s, 16000.0f);
        benchmark::DoNotOptimize(result.text);
    }

    state.SetLabel("transcribe() 5 s @ 16 kHz");
    state.SetItemsProcessed(state.iterations());
}

// ─── 3. transcribe() — 30-second chunk (CLI parity budget) ───────────────────
//
// Real-time target for whisper.cpp CLI on a 4-core CPU: < 60 s wall-clock
// to transcribe 30 s of audio (i.e. < 2× real-time).
// In stub mode this measures pure dispatch overhead (should be < 1 ms).

BENCHMARK_F(WhisperPluginFixture, Transcribe_30s_CLIParity)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = plugin->transcribe(pcm_30s, 16000.0f);
        benchmark::DoNotOptimize(result.text);
    }

    state.SetLabel("transcribe() 30 s — real-time budget < 60 s wall-clock");
    state.SetItemsProcessed(state.iterations());
}

// ─── 4. transcribe() — 8 kHz sample rate (telephony) ────────────────────────

BENCHMARK_F(WhisperPluginFixture, Transcribe_8kHz_1s)(benchmark::State& state) {
    auto pcm_8k = makePCM(1000, 8000.0f);

    for (auto _ : state) {
        auto result = plugin->transcribe(pcm_8k, 8000.0f);
        benchmark::DoNotOptimize(result.text);
    }

    state.SetLabel("transcribe() 1 s @ 8 kHz (telephony)");
}

// ─── 5. detectLanguage() latency ─────────────────────────────────────────────

BENCHMARK_F(WhisperPluginFixture, DetectLanguage)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = plugin->detectLanguage(pcm_1s, 16000.0f);
        benchmark::DoNotOptimize(result.language);
    }

    state.SetLabel("detectLanguage() 1 s @ 16 kHz");
}

// ─── 6. transcribeFile() overhead ────────────────────────────────────────────
// Uses a non-existent path — measures the plugin's fast-fail / error path.

BENCHMARK_F(WhisperPluginFixture, TranscribeFile_NonExistent)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = plugin->transcribeFile("/tmp/bench_nonexistent_audio.wav");
        benchmark::DoNotOptimize(result.text);
    }

    state.SetLabel("transcribeFile() fast-fail (missing file) path");
}

// ─── 7. getStatistics() cost ──────────────────────────────────────────────────

BENCHMARK_F(WhisperPluginFixture, StatsQuery)(benchmark::State& state) {
    // Warm up the call counter
    for (int i = 0; i < 10; ++i) {
        plugin->transcribe(pcm_1s, 16000.0f);
    }

    for (auto _ : state) {
        auto stats = plugin->getStatistics();
        benchmark::DoNotOptimize(stats);
    }

    state.SetLabel("getStatistics() after 10 transcriptions");
}

// ─── 8. Direct stub transcriber — pure interface dispatch cost ────────────────

static void BM_WhisperStub_Direct(benchmark::State& state) {
    WhisperStubTranscriber stub;
    WhisperConfig cfg;
    cfg.model_path = "stub";
    stub.initialize(cfg);

    auto pcm = makePCM(1000);

    for (auto _ : state) {
        auto result = stub.transcribe(pcm, 16000.0f);
        benchmark::DoNotOptimize(result.text);
    }

    state.SetLabel("WhisperStubTranscriber::transcribe() — pure dispatch overhead");
}
BENCHMARK(BM_WhisperStub_Direct);

// ─── 9. Buffer-size sweep ─────────────────────────────────────────────────────

static void BM_Transcribe_BufferSize(benchmark::State& state) {
    WhisperPlugin plugin;
    nlohmann::json cfg;
    plugin.initialize(resolveWhisperModelPath(), cfg);

    const int duration_ms = static_cast<int>(state.range(0));
    auto pcm = makePCM(duration_ms);

    for (auto _ : state) {
        auto result = plugin.transcribe(pcm, 16000.0f);
        benchmark::DoNotOptimize(result.text);
    }

    state.SetLabel(std::to_string(duration_ms) + " ms PCM buffer");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Transcribe_BufferSize)
    ->Arg(100)->Arg(500)->Arg(1000)->Arg(5000)->Arg(30000)
    ->Unit(benchmark::kMillisecond);

#if defined(THEMIS_ENABLE_WHISPER) && defined(THEMIS_BENCH_WHISPER_MODEL_PATH)
static void BM_WhisperRealModel_1min(benchmark::State& state) {
    WhisperPlugin plugin;
    nlohmann::json cfg;
    cfg["language"] = "auto";
    cfg["n_threads"] = 4;
    const bool init_ok = plugin.initialize(resolveWhisperModelPath(), cfg);
    if (!init_ok) {
        state.SkipWithError("Whisper real-model benchmark skipped: initialization failed");
        return;
    }

    auto pcm = makePCM(60000);
    for (auto _ : state) {
        auto result = plugin.transcribe(pcm, 16000.0f);
        benchmark::DoNotOptimize(result.text);
    }
    state.SetLabel("real model validation gate: transcribe() 60 s @ 16 kHz");
}
BENCHMARK(BM_WhisperRealModel_1min)->Unit(benchmark::kMillisecond);
#endif
