// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_voice_highcardinality_stress.cpp
 * @brief Wave D — Voice Pipeline High-Cardinality Stress Tests.
 *
 * High-cardinality stress tests for the voice module:
 * - Large volume audio chunk processing under 8-thread concurrency
 * - Concurrent STT stress
 * - TTS backlog stress
 *
 * Labels: wave_d;stress;not_release_critical
 *
 * SIMULATION NOTE: All STT, TTS, and audio processing operations use
 * in-process stubs that model the production hot paths without requiring
 * real audio models or hardware backends.
 * These stubs MUST NOT be used in production code paths.
 *
 * @see docs/operability/RUNBOOK_VOICE_PIPELINE.md
 * @see src/voice/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// SIMULATION NOTE — in-process voice stubs
// ---------------------------------------------------------------------------

class StubSTTEngineStress {
public:
    std::string transcribe(const std::vector<uint8_t>& audio) {
        ops_.fetch_add(1, std::memory_order_relaxed);
        return "text_" + std::to_string(audio.size());
    }
    uint64_t ops() const noexcept { return ops_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> ops_{0};
};

class StubTTSEngineStress {
public:
    std::vector<uint8_t> synthesize(const std::string& text) {
        ops_.fetch_add(1, std::memory_order_relaxed);
        return std::vector<uint8_t>(text.size(), 0xBB);
    }
    uint64_t ops() const noexcept { return ops_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> ops_{0};
};

class StubAudioChunkQueue {
public:
    explicit StubAudioChunkQueue(std::size_t capacity) : capacity_(capacity) {}

    bool push(const std::vector<uint8_t>& chunk) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.size() >= capacity_) {
            overflow_count_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        queue_.push_back(chunk);
        return true;
    }

    bool pop(std::vector<uint8_t>& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.empty()) return false;
        out = queue_.front();
        queue_.pop_front();
        return true;
    }

    uint64_t overflowCount() const noexcept { return overflow_count_.load(std::memory_order_relaxed); }

private:
    std::mutex mu_;
    std::deque<std::vector<uint8_t>> queue_;
    std::size_t capacity_;
    std::atomic<uint64_t> overflow_count_{0};
};

// ============================================================================
// Test cases
// ============================================================================

/**
 * @test HighCardinalityAudioChunk
 * Pushes 500 000 audio chunks across 8 concurrent threads into a bounded queue
 * and verifies that the queue handles load without uncaught exceptions.
 */
TEST(HighCardinalityAudioChunk, ConcurrentPush) {
    static constexpr int kThreads = 8;
    static constexpr uint64_t kChunksPerThread = 62'500; // 500 000 total
    static constexpr std::size_t kQueueCap = 2048;

    StubAudioChunkQueue queue(kQueueCap);
    std::atomic<uint64_t> exceptions{0};
    std::vector<std::thread> producers;
    std::atomic<bool> consuming{true};

    // Consumer drains in background.
    std::thread consumer([&]() {
        while (consuming.load(std::memory_order_relaxed)) {
            std::vector<uint8_t> out;
            queue.pop(out);
        }
        // Drain remainder
        std::vector<uint8_t> out;
        while (queue.pop(out)) {}
    });

    for (int t = 0; t < kThreads; ++t) {
        producers.emplace_back([&]() {
            const std::vector<uint8_t> chunk(256, static_cast<uint8_t>(t));
            for (uint64_t i = 0; i < kChunksPerThread; ++i) {
                try { queue.push(chunk); }
                catch (...) { exceptions.fetch_add(1, std::memory_order_relaxed); }
            }
        });
    }

    for (auto& th : producers) th.join();
    consuming.store(false, std::memory_order_relaxed);
    consumer.join();

    EXPECT_EQ(0u, exceptions.load())
        << "[VOICE:BufferOverflow] Exceptions during HighCardinalityAudioChunk stress";
}

/**
 * @test ConcurrentSTTStress
 * Runs 8 concurrent STT threads each processing 10 000 audio chunks and verifies
 * total op count and no errors.
 */
TEST(ConcurrentSTTStress, MultiThreadedTranscription) {
    static constexpr int kThreads = 8;
    static constexpr uint64_t kOpsPerThread = 10'000;

    StubSTTEngineStress stt;
    std::atomic<uint64_t> errors{0};
    std::vector<std::thread> workers;
    const std::vector<uint8_t> dummy(512, 0x00);

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&]() {
            for (uint64_t i = 0; i < kOpsPerThread; ++i) {
                try { stt.transcribe(dummy); }
                catch (...) { errors.fetch_add(1, std::memory_order_relaxed); }
            }
        });
    }

    for (auto& th : workers) th.join();

    EXPECT_EQ(0u, errors.load())
        << "[VOICE:STTUnavailable] Errors during ConcurrentSTTStress";
    EXPECT_EQ(static_cast<uint64_t>(kThreads) * kOpsPerThread, stt.ops());
}

/**
 * @test TTSBacklogStress
 * Runs 8 concurrent TTS threads each synthesizing 10 000 text items to verify
 * that the TTS path handles sustained backlog without errors.
 */
TEST(TTSBacklogStress, MultiThreadedSynthesis) {
    static constexpr int kThreads = 8;
    static constexpr uint64_t kOpsPerThread = 10'000;

    StubTTSEngineStress tts;
    std::atomic<uint64_t> errors{0};
    std::vector<std::thread> workers;

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (uint64_t i = 0; i < kOpsPerThread; ++i) {
                try {
                    const auto audio = tts.synthesize(
                        "tts_stress_" + std::to_string(t) + "_" + std::to_string(i));
                    if (audio.empty()) {
                        errors.fetch_add(1, std::memory_order_relaxed);
                    }
                } catch (...) {
                    errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& th : workers) th.join();

    EXPECT_EQ(0u, errors.load())
        << "[VOICE:TTSCrash] Errors during TTSBacklogStress";
    EXPECT_EQ(static_cast<uint64_t>(kThreads) * kOpsPerThread, tts.ops());
}
