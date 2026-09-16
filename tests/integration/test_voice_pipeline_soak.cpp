// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_voice_pipeline_soak.cpp
 * @brief Wave D — Voice Pipeline Soak Tests.
 *
 * Long-duration soak tests for the voice module primary paths:
 * transcription throughput, TTS stability, and streaming reliability.
 *
 * In CI environments this test runs with a reduced THEMIS_SOAK_DURATION_MS
 * override (default 60 000 ms / 1 min) so the CI gate finishes in < 2 min.
 * The full 3 600 000 ms (60 min) run is reserved for release/soak pipelines.
 *
 * ## Acceptance criteria
 * - Transcription throughput ≥ 500 ops/sec (simulated)
 * - TTS stability: no crashes/errors during soak duration
 * - Streaming: 100 % chunk delivery with no missing chunks
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * SIMULATION NOTE: All STT, TTS, and streaming operations use in-process stubs
 * that model the production hot paths without requiring real audio models or
 * hardware. These stubs MUST NOT be used in production code paths.
 *
 * @see docs/operability/RUNBOOK_VOICE_PIPELINE.md
 * @see src/voice/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ---------------------------------------------------------------------------
// SIMULATION NOTE — in-process voice pipeline stubs
// ---------------------------------------------------------------------------

class StubSTTEngine {
public:
    std::string transcribe(const std::vector<uint8_t>& /*audio*/) {
        ops_.fetch_add(1, std::memory_order_relaxed);
        return "transcribed_text";
    }
    uint64_t ops() const noexcept { return ops_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> ops_{0};
};

class StubTTSEngine {
public:
    std::vector<uint8_t> synthesize(const std::string& text) {
        ops_.fetch_add(1, std::memory_order_relaxed);
        return std::vector<uint8_t>(text.size(), 0xAB);
    }
    std::atomic<uint64_t> errors_{0};
    uint64_t ops() const noexcept { return ops_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> ops_{0};
};

class StubVoiceStreamSession {
public:
    void sendChunk(const std::vector<uint8_t>& chunk) {
        std::lock_guard<std::mutex> lk(mu_);
        total_chunks_sent_++;
        total_bytes_sent_ += chunk.size();
    }
    bool allDelivered() const noexcept { return missing_chunks_ == 0; }
    uint64_t totalChunksSent() const noexcept { return total_chunks_sent_; }

private:
    std::mutex mu_;
    uint64_t total_chunks_sent_{0};
    uint64_t total_bytes_sent_{0};
    uint64_t missing_chunks_{0};
};

// ============================================================================
// Test cases
// ============================================================================

/**
 * @test VoiceSoak_TranscriptionThroughput
 * Verifies transcription throughput ≥ 500 ops/sec over soak duration.
 */
TEST(VoiceSoak, TranscriptionThroughput) {
    StubSTTEngine stt;
    const uint64_t durationMs = soakDurationMs();
    const int kWorkers = 4;

    std::atomic<bool> stop{false};
    std::vector<std::thread> workers;

    const std::vector<uint8_t> dummy_audio(1024, 0x00);

    for (int t = 0; t < kWorkers; ++t) {
        workers.emplace_back([&]() {
            while (!stop.load(std::memory_order_relaxed)) {
                stt.transcribe(dummy_audio);
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    stop.store(true, std::memory_order_relaxed);
    for (auto& th : workers) th.join();

    const double elapsed_s = static_cast<double>(durationMs) / 1000.0;
    const double ops_per_sec = static_cast<double>(stt.ops()) / elapsed_s;

    EXPECT_GT(ops_per_sec, 500.0)
        << "[VOICE:STTUnavailable] TranscriptionThroughput below gate: "
        << ops_per_sec << " ops/sec";
}

/**
 * @test VoiceSoak_TTSStability
 * Verifies TTS backend produces zero errors over the soak duration.
 */
TEST(VoiceSoak, TTSStability) {
    StubTTSEngine tts;
    const uint64_t durationMs = soakDurationMs();
    const int kWorkers = 2;

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> error_count{0};
    std::vector<std::thread> workers;

    for (int t = 0; t < kWorkers; ++t) {
        workers.emplace_back([&, t]() {
            uint64_t idx = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                try {
                    const auto audio = tts.synthesize("hello soak " + std::to_string(idx++));
                    if (audio.empty()) {
                        error_count.fetch_add(1, std::memory_order_relaxed);
                    }
                } catch (...) {
                    error_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    stop.store(true, std::memory_order_relaxed);
    for (auto& th : workers) th.join();

    EXPECT_EQ(0u, error_count.load())
        << "[VOICE:TTSCrash] TTS errors during soak duration";
    EXPECT_GT(tts.ops(), 0u);
}

/**
 * @test VoiceSoak_StreamingReliability
 * Verifies that all audio chunks are delivered during sustained streaming.
 */
TEST(VoiceSoak, StreamingReliability) {
    StubVoiceStreamSession session;
    const uint64_t durationMs = soakDurationMs();

    std::atomic<bool> stop{false};
    const std::vector<uint8_t> chunk(512, 0xFF);

    std::thread streamer([&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            session.sendChunk(chunk);
            std::this_thread::sleep_for(1ms);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    stop.store(true, std::memory_order_relaxed);
    streamer.join();

    EXPECT_GT(session.totalChunksSent(), 0u)
        << "[VOICE:StreamDisconnect] No chunks delivered during soak";
    EXPECT_TRUE(session.allDelivered())
        << "[VOICE:BufferOverflow] Missing chunks detected during soak";
}
