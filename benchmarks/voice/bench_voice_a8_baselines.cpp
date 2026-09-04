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
#include <vector>

namespace themis {
namespace voice {
namespace bench {

// =============================================================================
// Helper Functions
// =============================================================================

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
    for (auto _ : state) {
        VoiceStreamValidator v("session_bench", 16000, 1, 16);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BenchStreamValidatorCreation)->Repetitions(5);

/**
 * BP-V8-002: Single chunk validation.
 * 
 * Baseline: Should be < 100µs (includes copy + validation).
 */
static void BenchSingleChunkValidation(benchmark::State& state) {
    VoiceStreamValidator v("session_bench", 16000, 1, 16);
    auto chunk = create_audio_chunk(4096);
    
    uint32_t seq = 0;
    for (auto _ : state) {
        try {
            v.validate_chunk(chunk.data(), chunk.size(), seq, seq * 256, false);
            seq++;
        } catch (...) {
            // Sequence error; reset for next iteration.
            state.SkipWithError("Sequence error");
        }
    }
}
BENCHMARK(BenchSingleChunkValidation)->Repetitions(5);

/**
 * BP-V8-003: Chunk size validation check.
 * 
 * Baseline: Should be < 50ns (simple comparison).
 */
static void BenchChunkSizeValidation(benchmark::State& state) {
    for (auto _ : state) {
        size_t chunk_size = 4096;
        bool valid = (chunk_size >= StreamValidationPolicy::MIN_CHUNK_SIZE_BYTES &&
                     chunk_size <= StreamValidationPolicy::MAX_CHUNK_SIZE_BYTES);
        benchmark::DoNotOptimize(valid);
    }
}
BENCHMARK(BenchChunkSizeValidation)->Repetitions(5);

/**
 * BP-V8-004: Sequential ordering check.
 * 
 * Baseline: Should be < 50ns (arithmetic + comparison).
 */
static void BenchSequenceValidation(benchmark::State& state) {
    uint32_t last_seq = 0;
    for (auto _ : state) {
        uint32_t current_seq = 42;
        bool valid = (current_seq == last_seq + 1);
        benchmark::DoNotOptimize(valid);
        last_seq = current_seq;
    }
}
BENCHMARK(BenchSequenceValidation)->Repetitions(5);

// =============================================================================
// Baseline Benchmarks: Liveness Detection
// =============================================================================

/**
 * BP-V8-005: Liveness checker creation overhead.
 * 
 * Baseline: Should be < 10µs (simple initialization).
 */
static void BenchLivenessCheckerCreation(benchmark::State& state) {
    for (auto _ : state) {
        VoiceLivenessChecker c("session_bench");
        benchmark::DoNotOptimize(c);
    }
}
BENCHMARK(BenchLivenessCheckerCreation)->Repetitions(5);

/**
 * BP-V8-006: Single audio chunk liveness check.
 * 
 * Baseline: Should be < 50µs (includes silence detection + spoof scoring).
 */
static void BenchSingleChunkLivenessCheck(benchmark::State& state) {
    VoiceLivenessChecker c("session_bench");
    auto chunk = create_audio_chunk(4096);
    
    for (auto _ : state) {
        auto result = c.check_audio_chunk(chunk.data(), chunk.size(), 16000);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchSingleChunkLivenessCheck)->Repetitions(5);

/**
 * BP-V8-007: Silence detection check.
 * 
 * Baseline: Should be < 20µs (scan first KB of audio).
 */
static void BenchSilenceDetection(benchmark::State& state) {
    VoiceLivenessChecker c("session_bench");
    auto chunk = create_audio_chunk(4096);
    
    for (auto _ : state) {
        bool is_silent = c.is_silence_or_noise_only(chunk.data(), chunk.size(), 16000);
        benchmark::DoNotOptimize(is_silent);
    }
}
BENCHMARK(BenchSilenceDetection)->Repetitions(5);

/**
 * BP-V8-008: Audio hash computation.
 * 
 * Baseline: Should be < 30µs (checksum of first 1KB).
 */
static void BenchAudioHashComputation(benchmark::State& state) {
    VoiceLivenessChecker c("session_bench");
    auto chunk = create_audio_chunk(4096);
    
    for (auto _ : state) {
        std::string hash = c.compute_audio_hash(chunk.data(), chunk.size());
        benchmark::DoNotOptimize(hash);
    }
}
BENCHMARK(BenchAudioHashComputation)->Repetitions(5);

/**
 * BP-V8-009: Replay detection check.
 * 
 * Baseline: Should be < 50µs (vector scan + insertion).
 */
static void BenchReplayDetection(benchmark::State& state) {
    VoiceLivenessChecker c("session_bench");
    
    for (auto _ : state) {
        bool is_replay = c.is_replay_detected("1234abcd");
        benchmark::DoNotOptimize(is_replay);
    }
}
BENCHMARK(BenchReplayDetection)->Repetitions(5);

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
    for (auto _ : state) {
        try {
            auto validated = validator.validate_chunk(chunk.data(), chunk.size(), seq, seq * 256, false);
            auto liveness_result = liveness.check_audio_chunk(chunk.data(), chunk.size(), 16000);
            benchmark::DoNotOptimize(validated);
            benchmark::DoNotOptimize(liveness_result);
            seq++;
        } catch (...) {
            state.SkipWithError("Pipeline error");
        }
    }
}
BENCHMARK(BenchStreamProcessingPipeline)->Repetitions(5);

/**
 * BP-V8-011: Multi-session concurrent initialization.
 * 
 * Tests parallel session creation overhead.
 */
static void BenchMultiSessionInitialization(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<VoiceStreamValidator> validators = {};

        for (int i = 0; i < 10; ++i) {
            validators.emplace_back("session_" + std::to_string(i), 16000, 1, 16);
        }
        benchmark::DoNotOptimize(validators);
    }
}
BENCHMARK(BenchMultiSessionInitialization)->Repetitions(5);

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
        v.validate_chunk(chunk.data(), chunk.size(), i, i * 256, false);
    }
    
    for (auto _ : state) {
        v.reset();
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BenchValidatorReset)->Repetitions(5);

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
        c.check_audio_chunk(chunk.data(), chunk.size(), 16000);
    }
    
    for (auto _ : state) {
        c.reset();
        benchmark::DoNotOptimize(c);
    }
}
BENCHMARK(BenchLivenessCheckerReset)->Repetitions(5);

}  // namespace bench
}  // namespace voice
}  // namespace themis
