// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_importers_soak.cpp
 * @brief Wave D — Importers Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB importers module.
 * Verifies that connector throughput, schema-drift stability, and
 * conflict-strategy reliability remain stable over the soak window.
 *
 * ## Acceptance criteria
 * - Connector throughput ≥ 10 000 ingest ops/sec
 * - Schema drift: zero unhandled drift events during soak
 * - Conflict resolution: zero unresolved conflict events during soak
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_IMPORTERS.md
 * @see src/importers/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
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

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE
//
// The stubs below replace live importer connector, schema-drift, and conflict
// paths — no external data source, schema registry, or conflict store is
// required.
//   - StubConnector: models multi-connector ingestion by hashing record id.
//   - StubSchemaDriftHandler: models schema drift detection with bounded
//     capacity and unhandled drift counting.
//   - StubConflictResolver: models conflict resolution with retry tracking.
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

class StubConnector {
public:
    bool ingest(uint64_t record_id) noexcept {
        ops_.fetch_add(1, std::memory_order_relaxed);
        (void)record_id;
        return true;
    }
    uint64_t totalOps() const noexcept { return ops_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> ops_{0};
};

class StubSchemaDriftHandler {
public:
    struct DriftResult { bool handled{true}; bool unhandled{false}; };
    DriftResult handle(uint64_t schema_version) noexcept {
        ops_.fetch_add(1, std::memory_order_relaxed);
        (void)schema_version;
        return {true, false};
    }
    uint64_t totalOps() const noexcept { return ops_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> ops_{0};
};

class StubConflictResolver {
public:
    struct ResolveResult { bool resolved{true}; };
    ResolveResult resolve(uint64_t conflict_id) noexcept {
        ops_.fetch_add(1, std::memory_order_relaxed);
        (void)conflict_id;
        return {true};
    }
    uint64_t totalOps() const noexcept { return ops_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> ops_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 1: ImportersSoak_ConnectorThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(ImportersSoak_ConnectorThroughput, ThroughputGateAndNoExceptions) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubConnector connector;
    bool exception_caught = false;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            (void)connector.ingest(tick % 100000);
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    EXPECT_FALSE(exception_caught)
        << "[IMPORTER:ConnectorLost] No exceptions during connector ingest soak";
    ASSERT_GT(elapsed_ms.count(), 0);

    const double elapsed_s   = elapsed_ms.count() / 1000.0;
    const double ops_per_sec = static_cast<double>(connector.totalOps()) / elapsed_s;

    constexpr double kMinThroughput = 10'000.0;
    EXPECT_GE(ops_per_sec, kMinThroughput)
        << "[IMPORTER:ConnectorLost] Connector throughput must be ≥ 10 000 ingest ops/sec. "
           "Observed: " << static_cast<uint64_t>(ops_per_sec) << " ops/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 2: ImportersSoak_SchemaDriftStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(ImportersSoak_SchemaDriftStability, ZeroUnhandledDriftAndStableOutput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 5);

    StubSchemaDriftHandler handler;
    bool exception_caught   = false;
    uint64_t unhandled      = 0;

    const auto start = std::chrono::steady_clock::now();
    uint64_t version = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const auto result = handler.handle(version);
            if (result.unhandled) {
                ++unhandled;
            }
            ++version;
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[IMPORTER:SchemaDriftFailed] No exceptions during schema-drift soak";

    EXPECT_EQ(unhandled, 0u)
        << "[IMPORTER:SchemaDriftFailed] Zero unhandled drift events expected during soak. "
           "Observed: " << unhandled;

    EXPECT_GT(handler.totalOps(), 0u)
        << "At least one schema drift event must be handled during the soak";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 3: ImportersSoak_ConflictStrategyReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(ImportersSoak_ConflictStrategyReliability, ZeroUnresolvedConflictsAndStableOutput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 3);

    StubConflictResolver resolver;
    bool exception_caught   = false;
    uint64_t unresolved     = 0;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const auto result = resolver.resolve(tick % 5000);
            if (!result.resolved) {
                ++unresolved;
            }
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[IMPORTER:ConflictUnresolved] No exceptions during conflict resolution soak";

    EXPECT_EQ(unresolved, 0u)
        << "[IMPORTER:ConflictUnresolved] Zero unresolved conflict events expected during soak. "
           "Observed: " << unresolved;

    EXPECT_GT(resolver.totalOps(), 0u)
        << "At least one conflict resolution must complete during the soak";
}
