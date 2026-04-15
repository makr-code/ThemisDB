/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_whisper_transcription.cpp                    ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 07:05:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟠 BETA                                         ║
    • Quality Score:   48.0/100                                       ║
    • Total Lines:     240                                            ║
    • Open Issues:     TODOs: 0, Stubs: 11                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 4c802da514  2026-04-12  feat(benchmarks): add 9 missing benchmark suites for util... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🔧 In Progress                                               ║
╚═════════════════════════════════════════════════════════════════════╝
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

#include <string>
#include <vector>
#include <random>

using namespace themis::whisper;
using namespace themis::audio;

// ─── helpers ─────────────────────────────────────────────────────────────────

static const char* kWhisperModel =
#ifdef THEMIS_BENCH_WHISPER_MODEL_PATH
    THEMIS_BENCH_WHISPER_MODEL_PATH;
#else
    "";  // empty → stub/no-model path
#endif

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
        plugin->initialize(kWhisperModel, cfg);
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
    plugin.initialize(kWhisperModel, cfg);

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
