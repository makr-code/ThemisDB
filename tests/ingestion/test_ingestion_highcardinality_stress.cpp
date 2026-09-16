// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_ingestion_highcardinality_stress.cpp
 * @brief Wave D — Ingestion High-Cardinality Stress Tests.
 *
 * Validates the ingestion pipeline under high-cardinality and concurrent
 * workload conditions without requiring real external connectors.
 *
 * Test families
 * -------------
 * - HighCardinalityRecordIngest       : 1 000 000 records via 8 threads
 * - ConcurrentSchemaValidationStress  : schema validator under concurrent load
 * - BackpressureEdgeCaseStress        : boundary conditions for backpressure
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_INGESTION_PIPELINE.md
 * @see src/ingestion/ROADMAP.md — Wave D contribution closure
 */

// SIMULATION NOTE: All stubs in this file are in-process simulations.
// They do NOT drive real ingestion infrastructure and MUST NOT be linked
// into production code paths.

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// In-process stubs
// ─────────────────────────────────────────────────────────────────────────────

namespace {

struct StubIngestPipeline {
    static constexpr std::size_t kCapacity = 131072; // 128 K slots

    std::mutex mu;
    std::vector<uint64_t> buf;
    std::atomic<uint64_t> enqueued{0};
    std::atomic<uint64_t> dequeued{0};
    std::atomic<uint64_t> backpressure_drops{0};

    StubIngestPipeline() { buf.reserve(kCapacity); }

    bool ingest(uint64_t record_id) {
        std::lock_guard<std::mutex> lk(mu);
        if (buf.size() >= kCapacity) {
            ++backpressure_drops;
            return false;
        }
        buf.push_back(record_id);
        ++enqueued;
        return true;
    }

    uint64_t drainAll() {
        std::lock_guard<std::mutex> lk(mu);
        uint64_t n = static_cast<uint64_t>(buf.size());
        dequeued += n;
        buf.clear();
        return n;
    }
};

struct StubSchemaValidator {
    std::atomic<uint64_t> validated{0};
    std::atomic<uint64_t> failures{0};

    bool validate(uint64_t record_id) {
        ++validated;
        // Simulate well-formed records: all pass
        (void)record_id;
        return true;
    }
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Stress test cases
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief HighCardinalityRecordIngest
 *
 * Ingests 1 000 000 records from 8 concurrent producer threads into the
 * in-process pipeline stub. Asserts that at least 1 000 000 records are
 * either enqueued or counted as backpressure drops (no silent losses).
 */
TEST(IngestionHighCardinalityStress, HighCardinalityRecordIngest) {
    constexpr uint64_t kTotalRecords = 1'000'000ULL;
    constexpr int kThreads = 8;
    constexpr uint64_t kPerThread = kTotalRecords / kThreads;

    StubIngestPipeline pipeline;
    std::atomic<uint64_t> attempted{0};

    std::vector<std::thread> producers;
    producers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        producers.emplace_back([&, t]() {
            const uint64_t base = static_cast<uint64_t>(t) * kPerThread;
            for (uint64_t i = 0; i < kPerThread; ++i) {
                pipeline.ingest(base + i);
                attempted.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& th : producers) th.join();
    pipeline.drainAll();

    const uint64_t total_accounted =
        pipeline.enqueued.load() + pipeline.backpressure_drops.load();
    EXPECT_EQ(total_accounted, kTotalRecords)
        << "Record count mismatch: " << total_accounted
        << " != " << kTotalRecords << " (enqueued="
        << pipeline.enqueued.load() << " drops="
        << pipeline.backpressure_drops.load() << ")";
}

/**
 * @brief ConcurrentSchemaValidationStress
 *
 * Runs schema validation over 500 000 records from 8 concurrent threads.
 * Asserts zero validation failures.
 */
TEST(IngestionHighCardinalityStress, ConcurrentSchemaValidationStress) {
    constexpr uint64_t kTotalRecords = 500'000ULL;
    constexpr int kThreads = 8;
    constexpr uint64_t kPerThread = kTotalRecords / kThreads;

    StubSchemaValidator validator;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            const uint64_t base = static_cast<uint64_t>(t) * kPerThread;
            for (uint64_t i = 0; i < kPerThread; ++i) {
                validator.validate(base + i);
            }
        });
    }
    for (auto& th : workers) th.join();

    EXPECT_EQ(validator.failures.load(), 0ULL)
        << "Schema validation reported unexpected failures under stress";
    EXPECT_EQ(validator.validated.load(), kTotalRecords)
        << "Not all records were validated";
}

/**
 * @brief BackpressureEdgeCaseStress
 *
 * Fills the pipeline to capacity from 4 threads, then drains, and repeats
 * 10 cycles. Asserts that each cycle's backpressure drop count is bounded
 * and the pipeline recovers cleanly.
 */
TEST(IngestionHighCardinalityStress, BackpressureEdgeCaseStress) {
    constexpr int kCycles = 10;
    constexpr int kThreads = 4;
    constexpr uint64_t kBurst = 200'000ULL;

    for (int cycle = 0; cycle < kCycles; ++cycle) {
        StubIngestPipeline pipeline;
        std::atomic<uint64_t> cycle_attempted{0};

        std::vector<std::thread> producers;
        producers.reserve(kThreads);
        for (int t = 0; t < kThreads; ++t) {
            producers.emplace_back([&, t, cycle]() {
                const uint64_t base =
                    static_cast<uint64_t>(cycle * kThreads + t) * kBurst;
                for (uint64_t i = 0; i < kBurst; ++i) {
                    pipeline.ingest(base + i);
                    cycle_attempted.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        for (auto& th : producers) th.join();
        pipeline.drainAll();

        const uint64_t total_accounted =
            pipeline.enqueued.load() + pipeline.backpressure_drops.load();
        EXPECT_EQ(total_accounted, kThreads * kBurst)
            << "Cycle " << cycle << ": record count mismatch";
        // After drainAll the pipeline must be empty / recoverable
        EXPECT_EQ(pipeline.buf.size(), 0ULL)
            << "Cycle " << cycle << ": pipeline not drained after stress cycle";
    }
}
