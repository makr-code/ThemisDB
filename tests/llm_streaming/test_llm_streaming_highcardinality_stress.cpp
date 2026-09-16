// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_llm_streaming_highcardinality_stress.cpp
 * @brief Wave D — LLM Streaming High-Cardinality Stress Tests.
 *
 * High-cardinality stress tests for the llm_streaming module:
 * - 1 000 concurrent streaming sessions across 8 threads
 * - Concurrent backpressure stress
 * - Chunk assembly edge case stress
 *
 * Labels: wave_d;stress;not_release_critical
 *
 * SIMULATION NOTE: All session, backpressure, and chunk assembly operations
 * use in-process stubs that model the production hot paths without requiring
 * gRPC, HTTP, or LLM inference backends.
 * These stubs MUST NOT be used in production code paths.
 *
 * @see docs/operability/RUNBOOK_LLM_STREAMING.md
 * @see src/llm_streaming/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// SIMULATION NOTE — in-process LLM streaming stubs
// ---------------------------------------------------------------------------

class StubStreamSession {
public:
    explicit StubStreamSession(uint64_t id) : id_(id) {}

    void sendToken(const std::string& token) {
        token_count_.fetch_add(1, std::memory_order_relaxed);
        (void)token;
    }
    void close() { closed_.store(true, std::memory_order_relaxed); }
    bool isClosed() const noexcept { return closed_.load(std::memory_order_relaxed); }
    uint64_t tokenCount() const noexcept { return token_count_.load(std::memory_order_relaxed); }
    uint64_t id() const noexcept { return id_; }

private:
    uint64_t id_;
    std::atomic<uint64_t> token_count_{0};
    std::atomic<bool> closed_{false};
};

class StubBackpressureStress {
public:
    explicit StubBackpressureStress(std::size_t capacity) : capacity_(capacity) {}

    bool push(const std::string& chunk) {
        std::lock_guard<std::mutex> lk(mu_);
        if (buf_.size() >= capacity_) {
            overflow_.fetch_add(1, std::memory_order_relaxed);
            return false;
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

    uint64_t overflow() const noexcept { return overflow_.load(std::memory_order_relaxed); }

private:
    std::mutex mu_;
    std::deque<std::string> buf_;
    std::size_t capacity_;
    std::atomic<uint64_t> overflow_{0};
};

class StubChunkAssemblerStress {
public:
    void receiveChunk(uint64_t seq, const std::string& chunk) {
        std::lock_guard<std::mutex> lk(mu_);
        (void)chunk;
        if (seq != next_seq_) {
            order_errors_++;
        }
        ++next_seq_;
        ++received_;
    }

    uint64_t received()    const noexcept { return received_; }
    uint64_t orderErrors() const noexcept { return order_errors_; }

private:
    std::mutex mu_;
    uint64_t next_seq_{0};
    uint64_t received_{0};
    uint64_t order_errors_{0};
};

// ============================================================================
// Test cases
// ============================================================================

/**
 * @test HighCardinalityStreamSession
 * Creates 1 000 streaming sessions distributed across 8 threads, sends tokens
 * into each session, and verifies all sessions close cleanly.
 */
TEST(HighCardinalityStreamSession, ConcurrentSessions) {
    static constexpr int kSessions = 1'000;
    static constexpr int kThreads = 8;
    static constexpr uint64_t kTokensPerSession = 100;

    std::vector<std::unique_ptr<StubStreamSession>> sessions;
    sessions.reserve(kSessions);
    for (int i = 0; i < kSessions; ++i) {
        sessions.push_back(std::make_unique<StubStreamSession>(static_cast<uint64_t>(i)));
    }

    std::atomic<uint64_t> next_session{0};
    std::vector<std::thread> workers;

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&]() {
            while (true) {
                const uint64_t idx = next_session.fetch_add(1, std::memory_order_relaxed);
                if (idx >= static_cast<uint64_t>(kSessions)) break;
                auto& sess = *sessions[idx];
                for (uint64_t tok = 0; tok < kTokensPerSession; ++tok) {
                    sess.sendToken("tok_" + std::to_string(tok));
                }
                sess.close();
            }
        });
    }

    for (auto& th : workers) th.join();

    uint64_t total_tokens = 0;
    uint64_t closed_count = 0;
    for (const auto& s : sessions) {
        total_tokens += s->tokenCount();
        if (s->isClosed()) ++closed_count;
    }

    EXPECT_EQ(static_cast<uint64_t>(kSessions) * kTokensPerSession, total_tokens)
        << "[LLM_STREAM:GenerationStall] Token count mismatch in HighCardinalityStreamSession";
    EXPECT_EQ(static_cast<uint64_t>(kSessions), closed_count);
}

/**
 * @test ConcurrentBackpressureStress
 * Runs 8 concurrent producers against a single backpressure buffer and verifies
 * no uncaught exceptions occur.
 */
TEST(ConcurrentBackpressureStress, MultiThreadedPushPop) {
    static constexpr int kThreads = 8;
    static constexpr uint64_t kOpsPerThread = 50'000;
    static constexpr std::size_t kBufCap = 256;

    StubBackpressureStress bp(kBufCap);
    std::atomic<uint64_t> exceptions{0};
    std::vector<std::thread> workers;

    std::atomic<bool> consuming{true};
    std::thread consumer([&]() {
        while (consuming.load(std::memory_order_relaxed)) {
            std::string out;
            bp.pop(out);
        }
        std::string out;
        while (bp.pop(out)) {}
    });

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (uint64_t i = 0; i < kOpsPerThread; ++i) {
                try { bp.push("bp_" + std::to_string(t) + "_" + std::to_string(i)); }
                catch (...) { exceptions.fetch_add(1, std::memory_order_relaxed); }
            }
        });
    }

    for (auto& th : workers) th.join();
    consuming.store(false, std::memory_order_relaxed);
    consumer.join();

    EXPECT_EQ(0u, exceptions.load())
        << "[LLM_STREAM:BackpressureCascade] Exceptions in ConcurrentBackpressureStress";
}

/**
 * @test ChunkAssemblyEdgeCaseStress
 * Sends 100 000 in-order chunks to the assembler from 8 threads (serialised
 * via mutex) and verifies zero order violations.
 */
TEST(ChunkAssemblyEdgeCaseStress, InOrderDelivery) {
    static constexpr uint64_t kTotalChunks = 100'000;
    static constexpr int kThreads = 8;

    StubChunkAssemblerStress assembler;
    std::atomic<uint64_t> exceptions{0};
    std::atomic<uint64_t> next_seq{0};
    std::mutex send_mu;

    std::vector<std::thread> workers;
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&]() {
            for (;;) {
                std::lock_guard<std::mutex> lk(send_mu);
                const uint64_t s = next_seq.load(std::memory_order_relaxed);
                if (s >= kTotalChunks) break;
                next_seq.fetch_add(1, std::memory_order_relaxed);
                try { assembler.receiveChunk(s, "chunk_" + std::to_string(s)); }
                catch (...) { exceptions.fetch_add(1, std::memory_order_relaxed); }
            }
        });
    }

    for (auto& th : workers) th.join();

    EXPECT_EQ(0u, exceptions.load())
        << "[LLM_STREAM:TokenOrderViolation] Exceptions in ChunkAssemblyEdgeCaseStress";
    EXPECT_EQ(0u, assembler.orderErrors())
        << "[LLM_STREAM:TokenOrderViolation] Order violations in ChunkAssemblyEdgeCaseStress";
    EXPECT_EQ(kTotalChunks, assembler.received());
}
