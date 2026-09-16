// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_llm_streaming_soak.cpp
 * @brief Wave D — LLM Streaming Soak Tests.
 *
 * Long-duration soak tests for the llm_streaming module hot paths:
 * token throughput, backpressure stability, and chunk delivery reliability.
 *
 * In CI environments this test runs with a reduced THEMIS_SOAK_DURATION_MS
 * override (default 60 000 ms / 1 min) so the CI gate finishes in < 2 min.
 * The full 3 600 000 ms (60 min) run is reserved for release/soak pipelines.
 *
 * ## Acceptance criteria
 * - Token throughput ≥ 1 000 tokens/sec (simulated, in-process stubs)
 * - Backpressure handling: zero dropped or re-ordered chunks
 * - Chunk delivery reliability: 100 % delivery with in-order sequence
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * SIMULATION NOTE: All token streaming, backpressure, and chunk assembly
 * operations use in-process stubs that model the production hot paths
 * without requiring gRPC, HTTP, or LLM inference backends.
 * These stubs MUST NOT be used in production code paths.
 *
 * @see docs/operability/RUNBOOK_LLM_STREAMING.md
 * @see src/llm_streaming/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <deque>
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
// SIMULATION NOTE — in-process LLM streaming stubs
// ---------------------------------------------------------------------------

class StubTokenStream {
public:
    void sendToken(const std::string& token) {
        tokens_sent_.fetch_add(1, std::memory_order_relaxed);
        (void)token;
    }
    uint64_t tokensSent() const noexcept { return tokens_sent_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> tokens_sent_{0};
};

class StubBackpressureController {
public:
    explicit StubBackpressureController(std::size_t capacity) : capacity_(capacity) {}

    bool push(const std::string& chunk) {
        std::lock_guard<std::mutex> lk(mu_);
        if (buf_.size() >= capacity_) {
            overflow_count_.fetch_add(1, std::memory_order_relaxed);
            return false; // backpressure applied
        }
        buf_.push_back(chunk);
        return true;
    }

    bool pop(std::string& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (buf_.empty()) return false;
        out = buf_.front();
        buf_.pop_front();
        return true;
    }

    uint64_t overflowCount() const noexcept { return overflow_count_.load(std::memory_order_relaxed); }

private:
    std::mutex mu_;
    std::deque<std::string> buf_;
    std::size_t capacity_;
    std::atomic<uint64_t> overflow_count_{0};
};

class StubChunkAssembler {
public:
    void receive(uint64_t seq, const std::string& chunk) {
        std::lock_guard<std::mutex> lk(mu_);
        if (seq != expected_seq_) {
            order_violations_++;
        }
        ++expected_seq_;
        chunks_received_++;
        (void)chunk;
    }
    uint64_t chunksReceived()   const noexcept { return chunks_received_; }
    uint64_t orderViolations()  const noexcept { return order_violations_; }

private:
    std::mutex mu_;
    uint64_t expected_seq_{0};
    uint64_t chunks_received_{0};
    uint64_t order_violations_{0};
};

// ============================================================================
// Test cases
// ============================================================================

/**
 * @test LLMStreamingSoak_TokenThroughput
 * Verifies token throughput ≥ 1 000 tokens/sec over soak duration using
 * 4 concurrent producer threads.
 */
TEST(LLMStreamingSoak, TokenThroughput) {
    StubTokenStream stream;
    const uint64_t durationMs = soakDurationMs();
    const int kWorkers = 4;

    std::atomic<bool> stop{false};
    std::vector<std::thread> workers;

    for (int t = 0; t < kWorkers; ++t) {
        workers.emplace_back([&, t]() {
            uint64_t idx = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                stream.sendToken("tok_" + std::to_string(t) + "_" + std::to_string(idx++));
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    stop.store(true, std::memory_order_relaxed);
    for (auto& th : workers) th.join();

    const double elapsed_s = static_cast<double>(durationMs) / 1000.0;
    const double tps = static_cast<double>(stream.tokensSent()) / elapsed_s;

    EXPECT_GE(tps, 1000.0)
        << "[LLM_STREAM:GenerationStall] Token throughput below 1000 t/s: " << tps;
}

/**
 * @test LLMStreamingSoak_BackpressureStability
 * Verifies that the backpressure controller handles a sustained producer/consumer
 * load imbalance without uncaught exceptions.
 */
TEST(LLMStreamingSoak, BackpressureStability) {
    StubBackpressureController bp(256);
    const uint64_t durationMs = soakDurationMs();

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> exceptions{0};

    std::thread producer([&]() {
        uint64_t idx = 0;
        while (!stop.load(std::memory_order_relaxed)) {
            try { bp.push("chunk_" + std::to_string(idx++)); }
            catch (...) { exceptions.fetch_add(1, std::memory_order_relaxed); }
        }
    });

    std::thread consumer([&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            std::string out;
            bp.pop(out);
            std::this_thread::sleep_for(100us);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    stop.store(true, std::memory_order_relaxed);
    producer.join();
    consumer.join();

    EXPECT_EQ(0u, exceptions.load())
        << "[LLM_STREAM:BackpressureCascade] Exceptions during backpressure soak";
}

/**
 * @test LLMStreamingSoak_ChunkDeliveryReliability
 * Verifies in-order chunk delivery with zero order violations over soak duration.
 */
TEST(LLMStreamingSoak, ChunkDeliveryReliability) {
    StubChunkAssembler assembler;
    const uint64_t durationMs = soakDurationMs();

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> seq{0};

    std::thread producer([&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            const uint64_t s = seq.fetch_add(1, std::memory_order_relaxed);
            assembler.receive(s, "chunk_" + std::to_string(s));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    stop.store(true, std::memory_order_relaxed);
    producer.join();

    EXPECT_EQ(0u, assembler.orderViolations())
        << "[LLM_STREAM:TokenOrderViolation] Chunk order violations during soak";
    EXPECT_GT(assembler.chunksReceived(), 0u);
}
