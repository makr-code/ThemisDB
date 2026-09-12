/**
 * @file bench_voice_a8_baselines.cpp
 * @brief Performance baselines for Voice operations (Wave A-8).
 * @date 2026-08-16
 * 
 * Captures p95/p99 measurements for stream validation, liveness checking,
 * and session management overhead.
 * 
 * @see src/voice/ROADMAP.md § Wave A-8 Closure Evidence Block
 */

#include <benchmark/benchmark.h>
#include "voice/voice_stream_validator.h"
#include "voice/voice_liveness_checker.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <vector>

namespace themis {
namespace voice {
namespace bench {

// =============================================================================
// Helper Functions
// =============================================================================

namespace {

struct LatencySummary {
    double p50_ns = 0.0;
    double p95_ns = 0.0;
    double p99_ns = 0.0;
    double avg_ns = 0.0;
};

double percentile(std::vector<double> samples, double fraction) {
    if (samples.empty()) {
        return 0.0;
    }
    std::sort(samples.begin(), samples.end());
    const auto raw_index =
        static_cast<double>(samples.size() - 1U) * std::clamp(fraction, 0.0, 1.0);
    const auto lower_index = static_cast<std::size_t>(std::floor(raw_index));
    const auto upper_index = static_cast<std::size_t>(std::ceil(raw_index));
    if (lower_index == upper_index) {
        return samples[lower_index];
    }
    const auto weight = raw_index - static_cast<double>(lower_index);
    return samples[lower_index] +
           ((samples[upper_index] - samples[lower_index]) * weight);
}

LatencySummary summarizeLatencies(const std::vector<double>& samples_ns) {
    LatencySummary summary = {};
    if (samples_ns.empty()) {
        return summary;
    }
    summary.p50_ns = percentile(samples_ns, 0.50);
    summary.p95_ns = percentile(samples_ns, 0.95);
    summary.p99_ns = percentile(samples_ns, 0.99);
    summary.avg_ns = std::accumulate(samples_ns.begin(), samples_ns.end(), 0.0) /
                     static_cast<double>(samples_ns.size());
    return summary;
}

void publishLatencyCounters(benchmark::State& state,
                            const std::vector<double>& samples_ns,
                            double gate_target_ns = 0.0) {
    const auto summary = summarizeLatencies(samples_ns);
    state.counters["p50_ns"] = summary.p50_ns;
    state.counters["p95_ns"] = summary.p95_ns;
    state.counters["p99_ns"] = summary.p99_ns;
    state.counters["avg_ns"] = summary.avg_ns;
    if (gate_target_ns > 0.0) {
        state.counters["gate_target_ns"] = gate_target_ns;
        state.counters["gate_pass"] = summary.p99_ns <= gate_target_ns ? 1.0 : 0.0;
    }
}

template <typename Fn>
void runLatencyBenchmark(benchmark::State& state,
                         Fn&& fn,
                         double gate_target_ns = 0.0) {
    auto samples_ns = std::vector<double>{};
    samples_ns.reserve(state.max_iterations);
    for (auto _ : state) {
        const auto start = std::chrono::steady_clock::now();
        fn();
        const auto end = std::chrono::steady_clock::now();
        const auto elapsed_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        samples_ns.push_back(static_cast<double>(elapsed_ns));
        state.SetIterationTime(static_cast<double>(elapsed_ns) * 1e-9);
    }
    publishLatencyCounters(state, samples_ns, gate_target_ns);
}

}  // namespace

static std::vector<uint8_t> create_audio_chunk(size_t size) {
    std::vector<uint8_t> chunk(size);
    for (size_t i = 0; i < size; ++i) {
        chunk[i] = static_cast<uint8_t>((i * 17) % 256);
    }
    return chunk;
}

// =============================================================================
// Baseline Benchmarks: Stream Validation
// =============================================================================

/**
 * BP-V8-001: Stream validator creation overhead.
 * 
 * Baseline: Should be < 10µs (simple initialization).
 */
static void BenchStreamValidatorCreation(benchmark::State& state) {
    runLatencyBenchmark(state, []() {
        VoiceStreamValidator v("session_bench", 16000, 1, 16);
        benchmark::DoNotOptimize(v);
    }, 10'000.0);
}
BENCHMARK(BenchStreamValidatorCreation)->UseManualTime()->Repetitions(5);

/**
 * BP-V8-002: Single chunk validation.
 * 
 * Baseline: Should be < 100µs (includes copy + validation).
 */
static void BenchSingleChunkValidation(benchmark::State& state) {
    VoiceStreamValidator v("session_bench", 16000, 1, 16);
    auto chunk = create_audio_chunk(4096);
    
    uint32_t seq = 0;
    runLatencyBenchmark(state, [&]() {
        try {
            auto validated =
                v.validate_chunk(chunk.data(), chunk.size(), seq, seq * 256, false);
            benchmark::DoNotOptimize(validated);
            seq++;
        } catch (...) {
            // Sequence error; reset for next iteration.
            state.SkipWithError("Sequence error");
        }
    }, 100'000.0);
}
BENCHMARK(BenchSingleChunkValidation)->UseManualTime()->Repetitions(5);

/**
 * BP-V8-003: Chunk size validation check.
 * 
 * Baseline: Should be < 50ns (simple comparison).
 */
static void BenchChunkSizeValidation(benchmark::State& state) {
    runLatencyBenchmark(state, []() {
        size_t chunk_size = 4096;
        bool valid = (chunk_size >= StreamValidationPolicy::MIN_CHUNK_SIZE_BYTES &&
                     chunk_size <= StreamValidationPolicy::MAX_CHUNK_SIZE_BYTES);
        benchmark::DoNotOptimize(valid);
    }, 50.0);
}
BENCHMARK(BenchChunkSizeValidation)->UseManualTime()->Repetitions(5);

/**
 * BP-V8-004: Sequential ordering check.
 * 
 * Baseline: Should be < 50ns (arithmetic + comparison).
 */
static void BenchSequenceValidation(benchmark::State& state) {
    uint32_t last_seq = 0;
    runLatencyBenchmark(state, [&]() {
        uint32_t current_seq = 42;
        bool valid = (current_seq == last_seq + 1);
        benchmark::DoNotOptimize(valid);
        last_seq = current_seq;
    }, 50.0);
}
BENCHMARK(BenchSequenceValidation)->UseManualTime()->Repetitions(5);

// =============================================================================
// Baseline Benchmarks: Liveness Detection
// =============================================================================

/**
 * BP-V8-005: Liveness checker creation overhead.
 * 
 * Baseline: Should be < 10µs (simple initialization).
 */
static void BenchLivenessCheckerCreation(benchmark::State& state) {
    runLatencyBenchmark(state, []() {
        VoiceLivenessChecker c("session_bench");
        benchmark::DoNotOptimize(c);
    }, 10'000.0);
}
BENCHMARK(BenchLivenessCheckerCreation)->UseManualTime()->Repetitions(5);

/**
 * BP-V8-006: Single audio chunk liveness check.
 * 
 * Baseline: Should be < 50µs (includes silence detection + spoof scoring).
 */
static void BenchSingleChunkLivenessCheck(benchmark::State& state) {
    VoiceLivenessChecker c("session_bench");
    auto chunk = create_audio_chunk(4096);
    
    runLatencyBenchmark(state, [&]() {
        auto result = c.check_audio_chunk(chunk.data(), chunk.size(), 16000);
        benchmark::DoNotOptimize(result);
    }, 50'000.0);
}
BENCHMARK(BenchSingleChunkLivenessCheck)->UseManualTime()->Repetitions(5);

/**
 * BP-V8-007: Silence detection check.
 * 
 * Baseline: Should be < 20µs (scan first KB of audio).
 */
static void BenchSilenceDetection(benchmark::State& state) {
    VoiceLivenessChecker c("session_bench");
    auto chunk = create_audio_chunk(4096);
    
    runLatencyBenchmark(state, [&]() {
        bool is_silent = c.is_silence_or_noise_only(chunk.data(), chunk.size(), 16000);
        benchmark::DoNotOptimize(is_silent);
    }, 20'000.0);
}
BENCHMARK(BenchSilenceDetection)->UseManualTime()->Repetitions(5);

/**
 * BP-V8-008: Audio hash computation.
 * 
 * Baseline: Should be < 30µs (checksum of first 1KB).
 */
static void BenchAudioHashComputation(benchmark::State& state) {
    VoiceLivenessChecker c("session_bench");
    auto chunk = create_audio_chunk(4096);
    
    runLatencyBenchmark(state, [&]() {
        std::string hash = c.compute_audio_hash(chunk.data(), chunk.size());
        benchmark::DoNotOptimize(hash);
    }, 30'000.0);
}
BENCHMARK(BenchAudioHashComputation)->UseManualTime()->Repetitions(5);

/**
 * BP-V8-009: Replay detection check.
 * 
 * Baseline: Should be < 50µs (vector scan + insertion).
 */
static void BenchReplayDetection(benchmark::State& state) {
    VoiceLivenessChecker c("session_bench");
    
    runLatencyBenchmark(state, [&]() {
        bool is_replay = c.is_replay_detected("1234abcd");
        benchmark::DoNotOptimize(is_replay);
    }, 50'000.0);
}
BENCHMARK(BenchReplayDetection)->UseManualTime()->Repetitions(5);

// =============================================================================
// Latency Envelope Tests (verify contract bounds)
// =============================================================================

/**
 * BP-V8-010: Sequential chunk processing pipeline.
 * 
 * Simulates validating and checking liveness for a stream of chunks.
 */
static void BenchStreamProcessingPipeline(benchmark::State& state) {
    VoiceStreamValidator validator("session_bench", 16000, 1, 16);
    VoiceLivenessChecker liveness("session_bench");
    auto chunk = create_audio_chunk(4096);
    
    uint32_t seq = 0;
    runLatencyBenchmark(state, [&]() {
        try {
            auto validated = validator.validate_chunk(chunk.data(), chunk.size(), seq, seq * 256, false);
            auto liveness_result = liveness.check_audio_chunk(chunk.data(), chunk.size(), 16000);
            benchmark::DoNotOptimize(validated);
            benchmark::DoNotOptimize(liveness_result);
            seq++;
        } catch (...) {
            state.SkipWithError("Pipeline error");
        }
    });
}
BENCHMARK(BenchStreamProcessingPipeline)->UseManualTime()->Repetitions(5);

/**
 * BP-V8-011: Multi-session concurrent initialization.
 * 
 * Tests parallel session creation overhead.
 */
static void BenchMultiSessionInitialization(benchmark::State& state) {
    runLatencyBenchmark(state, []() {
        std::vector<VoiceStreamValidator> validators = {};

        for (int i = 0; i < 10; ++i) {
            validators.emplace_back("session_" + std::to_string(i), 16000, 1, 16);
        }
        benchmark::DoNotOptimize(validators);
    });
}
BENCHMARK(BenchMultiSessionInitialization)->UseManualTime()->Repetitions(5);

/**
 * BP-V8-012: Validator reset/cleanup cost.
 * 
 * Baseline: Should be < 10µs (just state reset).
 */
static void BenchValidatorReset(benchmark::State& state) {
    VoiceStreamValidator v("session_bench", 16000, 1, 16);
    auto chunk = create_audio_chunk(4096);
    
    // Pre-populate with some chunks.
    for (int i = 0; i < 10; ++i) {
        auto validated = v.validate_chunk(chunk.data(), chunk.size(), i, i * 256, false);
        benchmark::DoNotOptimize(validated);
    }
    
    runLatencyBenchmark(state, [&]() {
        v.reset();
        benchmark::DoNotOptimize(v);
    }, 10'000.0);
}
BENCHMARK(BenchValidatorReset)->UseManualTime()->Repetitions(5);

/**
 * BP-V8-013: Liveness checker reset/cleanup cost.
 * 
 * Baseline: Should be < 10µs (just state reset).
 */
static void BenchLivenessCheckerReset(benchmark::State& state) {
    VoiceLivenessChecker c("session_bench");
    auto chunk = create_audio_chunk(4096);
    
    // Pre-populate with some checks.
    for (int i = 0; i < 10; ++i) {
        auto result = c.check_audio_chunk(chunk.data(), chunk.size(), 16000);
        benchmark::DoNotOptimize(result);
    }
    
    runLatencyBenchmark(state, [&]() {
        c.reset();
        benchmark::DoNotOptimize(c);
    }, 10'000.0);
}
BENCHMARK(BenchLivenessCheckerReset)->UseManualTime()->Repetitions(5);

}  // namespace bench
}  // namespace voice
}  // namespace themis
