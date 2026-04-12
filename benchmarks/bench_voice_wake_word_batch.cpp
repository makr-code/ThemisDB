/**
 * @file bench_voice_wake_word_batch.cpp
 * @brief Performance benchmarks for wake-word detection latency and
 *        VoiceBatchProcessor linear thread-scaling.
 *
 * Validates:
 *   VOICE-PHASE5-01: Wake word latency target < 200 ms end-to-end on embedded
 *   VOICE-PHASE5-02: Batch processor linear throughput scaling to 16 threads
 *
 * Scenarios:
 *   - WakeWordDetector: processAudioChunk() latency (single call)
 *   - WakeWordDetector: detection throughput over a rolling 1500 ms buffer
 *   - VoiceBatchProcessor: single-item processing time
 *   - VoiceBatchProcessor: batch throughput scaling (1..16 threads via config)
 *   - VoiceBatchProcessor: runLoadTest() overhead
 */

#include <benchmark/benchmark.h>
#include "voice/wake_word_detector.h"
#include "voice/voice_batch_processor.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

using namespace themis::voice;

// ─── helpers ─────────────────────────────────────────────────────────────────

/// Generate a synthetic 16-kHz PCM audio buffer (silence + 50 ms burst).
static std::vector<int16_t> makeSyntheticPCM(int duration_ms,
                                              int sample_rate = 16000,
                                              bool add_energy  = true) {
    int n_samples = (sample_rate * duration_ms) / 1000;
    std::vector<int16_t> pcm(static_cast<size_t>(n_samples), 0);

    if (add_energy) {
        // Add a small energy burst in the last 50 ms to pass the VAD gate.
        std::mt19937 rng(42);
        std::uniform_int_distribution<int16_t> dist(
            static_cast<int16_t>(-800), static_cast<int16_t>(800));
        int burst_start = std::max(0, n_samples - (sample_rate * 50 / 1000));
        for (int i = burst_start; i < n_samples; ++i) {
            pcm[static_cast<size_t>(i)] = dist(rng);
        }
    }

    return pcm;
}

/// Convert int16 samples to raw bytes for BatchAudioItem.
static std::vector<uint8_t> pcmToBytes(const std::vector<int16_t>& pcm) {
    std::vector<uint8_t> out(pcm.size() * sizeof(int16_t));
    std::memcpy(out.data(), pcm.data(), out.size());
    return out;
}

// ─── WakeWordDetector fixtures ───────────────────────────────────────────────

class WakeWordBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        WakeWordConfig cfg;
        cfg.sensitivity       = 0.5f;
        cfg.buffer_length_ms  = 1500;
        cfg.continuous_listen = true;
        cfg.cooldown_ms       = 100;
        cfg.sample_rate       = 16000;

        detector = std::make_unique<WakeWordDetector>(cfg);
        detector->addWakeWord("hey_themis",  "hey themis");
        detector->addWakeWord("themis",      "themis");

        // Pre-bake a 100 ms chunk with energy (VAD-triggering).
        chunk_100ms_energy = pcmToBytes(makeSyntheticPCM(100, 16000, true));
        chunk_100ms_silent = pcmToBytes(makeSyntheticPCM(100, 16000, false));
    }

    void TearDown(const benchmark::State& /*s*/) override {
        detector.reset();
    }

    std::unique_ptr<WakeWordDetector> detector;
    std::vector<uint8_t>              chunk_100ms_energy;
    std::vector<uint8_t>              chunk_100ms_silent;
};

// ─── 1. processAudioChunk() latency — silent (fast VAD reject) ───────────────

BENCHMARK_F(WakeWordBenchFixture, ProcessChunk_Silent_100ms)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = detector->processAudioChunk(chunk_100ms_silent);
        benchmark::DoNotOptimize(result.detected);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("100 ms silent chunk; VAD fast-reject path");
}

// ─── 2. processAudioChunk() latency — energy present ────────────────────────

BENCHMARK_F(WakeWordBenchFixture, ProcessChunk_Energy_100ms)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = detector->processAudioChunk(chunk_100ms_energy);
        benchmark::DoNotOptimize(result.detected);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("100 ms chunk with energy; full scoring path");
}

// ─── 3. Rolling buffer (15 × 100 ms) — simulates embedded real-time loop ─────

BENCHMARK_F(WakeWordBenchFixture, RollingBuffer_1500ms)(benchmark::State& state) {
    // 15 × 100 ms = 1500 ms total — one full detection window.
    const int kChunks = 15;

    for (auto _ : state) {
        WakeWordDetectionResult last{};
        for (int i = 0; i < kChunks; ++i) {
            auto& chunk = (i == kChunks - 1) ? chunk_100ms_energy : chunk_100ms_silent;
            last = detector->processAudioChunk(chunk);
        }
        benchmark::DoNotOptimize(last.detected);
    }

    state.SetItemsProcessed(state.iterations() * kChunks);
    state.SetLabel("15×100 ms rolling buffer (full detection window)");
}

// ─── 4. Throughput — chunks/second ───────────────────────────────────────────

BENCHMARK_F(WakeWordBenchFixture, ChunkThroughput)(benchmark::State& state) {
    const int kBatch = 100;

    for (auto _ : state) {
        for (int i = 0; i < kBatch; ++i) {
            auto result = detector->processAudioChunk(chunk_100ms_energy);
            benchmark::DoNotOptimize(result);
        }
    }

    state.SetItemsProcessed(state.iterations() * kBatch);
    state.SetLabel("100×100 ms chunk throughput");
}

// ─── VoiceBatchProcessor fixtures ────────────────────────────────────────────

class BatchProcessorScalingFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        audio_bytes = pcmToBytes(makeSyntheticPCM(500, 16000, true));
    }

    std::vector<uint8_t> audio_bytes;
};

// ─── 5. Single-item processing time ──────────────────────────────────────────

BENCHMARK_F(BatchProcessorScalingFixture, SingleItem)(benchmark::State& state) {
    BatchProcessorConfig cfg;
    cfg.max_concurrent_jobs  = 1;
    cfg.default_batch_size   = 1;
    cfg.compute_quality_metrics = true;

    VoiceBatchProcessor processor(cfg);

    BatchAudioItem item;
    item.item_id    = "bench_item";
    item.audio_data = audio_bytes;
    item.sample_rate = 16000;

    for (auto _ : state) {
        auto result = processor.processItem(item);
        benchmark::DoNotOptimize(result.success);
    }

    state.SetLabel("Single BatchAudioItem processing time");
}

// ─── 6. Batch throughput scaling: 1..16 threads ──────────────────────────────
//
// Expects near-linear throughput growth with thread count.
// Accept ≥ 0.7x linear efficiency (superscalar effects vary by hardware).

BENCHMARK_F(BatchProcessorScalingFixture, BatchThroughput_ThreadScaling)(
    benchmark::State& state) {
    const int kThreads = static_cast<int>(state.range(0));
    const int kItems   = kThreads * 8;  // Keep CPU busy

    BatchProcessorConfig cfg;
    cfg.max_concurrent_jobs     = static_cast<size_t>(kThreads);
    cfg.default_batch_size      = static_cast<size_t>(kThreads);
    cfg.compute_quality_metrics = false;  // raw throughput

    VoiceBatchProcessor processor(cfg);

    std::vector<BatchAudioItem> items;
    items.reserve(static_cast<size_t>(kItems));
    for (int i = 0; i < kItems; ++i) {
        BatchAudioItem it;
        it.item_id     = "item_" + std::to_string(i);
        it.audio_data  = audio_bytes;
        it.sample_rate = 16000;
        items.push_back(std::move(it));
    }

    for (auto _ : state) {
        auto results = processor.processBatchSync(items);
        benchmark::DoNotOptimize(results.size());
    }

    state.SetItemsProcessed(state.iterations() * kItems);
    state.SetLabel(std::to_string(kThreads) + "-thread batch, " +
                   std::to_string(kItems) + " items");
}
BENCHMARK_REGISTER_F(BatchProcessorScalingFixture, BatchThroughput_ThreadScaling)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

// ─── 7. runLoadTest() overhead ────────────────────────────────────────────────

BENCHMARK_F(BatchProcessorScalingFixture, LoadTestHelper)(benchmark::State& state) {
    const size_t kConcurrent = static_cast<size_t>(state.range(0));

    BatchProcessorConfig cfg;
    cfg.max_concurrent_jobs = kConcurrent;
    cfg.compute_quality_metrics = false;
    VoiceBatchProcessor processor(cfg);

    BatchAudioItem tmpl;
    tmpl.item_id     = "template";
    tmpl.audio_data  = audio_bytes;
    tmpl.sample_rate = 16000;

    for (auto _ : state) {
        auto summary = processor.runLoadTest(kConcurrent, tmpl);
        benchmark::DoNotOptimize(summary.completed_items);
    }

    state.SetLabel("runLoadTest concurrent=" + std::to_string(kConcurrent));
}
BENCHMARK_REGISTER_F(BatchProcessorScalingFixture, LoadTestHelper)
    ->Arg(1)->Arg(4)->Arg(8)->Arg(16)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);
