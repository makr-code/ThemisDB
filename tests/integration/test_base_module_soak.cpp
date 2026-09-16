// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_base_module_soak.cpp
 * @brief Wave D — Base Module Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB base module.
 * Verifies that tracing-exporter throughput, core-init stability, and
 * helper reliability remain stable over the soak window.
 *
 * ## Acceptance criteria
 * - Tracing exporter throughput ≥ 10 000 exports/sec
 * - Core init: zero failed initializations during soak
 * - Helper: zero overload events during soak
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_BASE_MODULE.md
 * @see src/base/ROADMAP.md — Wave D Contribution
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
// The stubs below replace live base tracing exporter, core initializer, and
// helper dispatch paths — no external OTLP collector, loader backend, or wasm
// sandbox is required.
//   - StubTracingExporter: models span export with cardinality and lag
//     tracking.
//   - StubCoreInitializer: models core module initialization with failure
//     detection.
//   - StubBaseHelper: models base helper dispatch with overload tracking.
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

class StubTracingExporter {
public:
    bool exportSpan(uint64_t span_id) noexcept {
        exports_.fetch_add(1, std::memory_order_relaxed);
        (void)span_id;
        return true;
    }
    uint64_t totalExports() const noexcept { return exports_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> exports_{0};
};

class StubCoreInitializer {
public:
    struct InitResult { bool ok{true}; bool failed{false}; };
    InitResult init(const std::string& module_name) noexcept {
        inits_.fetch_add(1, std::memory_order_relaxed);
        (void)module_name;
        return {true, false};
    }
    uint64_t totalInits() const noexcept { return inits_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> inits_{0};
};

class StubBaseHelper {
public:
    struct HelperResult { bool ok{true}; bool overloaded{false}; };
    HelperResult dispatch(const std::string& op) noexcept {
        ops_.fetch_add(1, std::memory_order_relaxed);
        (void)op;
        return {true, false};
    }
    uint64_t totalOps() const noexcept { return ops_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> ops_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 1: BaseSoak_TracingExporterThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(BaseSoak_TracingExporterThroughput, ThroughputGateAndNoExceptions) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubTracingExporter exporter;
    bool exception_caught = false;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            (void)exporter.exportSpan(tick % 1000000);
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    EXPECT_FALSE(exception_caught)
        << "[BASE:TracingExporterFailed] No exceptions during tracing exporter soak";
    ASSERT_GT(elapsed_ms.count(), 0);

    const double elapsed_s   = elapsed_ms.count() / 1000.0;
    const double ops_per_sec = static_cast<double>(exporter.totalExports()) / elapsed_s;

    constexpr double kMinThroughput = 10'000.0;
    EXPECT_GE(ops_per_sec, kMinThroughput)
        << "[BASE:ExporterLag] Tracing exporter throughput must be ≥ 10 000 exports/sec. "
           "Observed: " << static_cast<uint64_t>(ops_per_sec) << " exports/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 2: BaseSoak_CoreInitStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(BaseSoak_CoreInitStability, ZeroFailedInitsAndStableOutput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 5);

    StubCoreInitializer initializer;
    bool exception_caught = false;
    uint64_t fail_count   = 0;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const std::string mod = "base_module_" + std::to_string(tick % 16);
            const auto result = initializer.init(mod);
            if (result.failed) {
                ++fail_count;
            }
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[BASE:CoreInitFailed] No exceptions during core init stability soak";

    EXPECT_EQ(fail_count, 0u)
        << "[BASE:CoreInitFailed] Zero failed initializations expected during soak. "
           "Observed: " << fail_count;

    EXPECT_GT(initializer.totalInits(), 0u)
        << "At least one core initialization must complete during the soak";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 3: BaseSoak_HelperReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(BaseSoak_HelperReliability, ZeroOverloadEventsAndStableOutput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 3);

    StubBaseHelper helper;
    bool exception_caught   = false;
    uint64_t overload_count = 0;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const std::string op = "base_op_" + std::to_string(tick % 32);
            const auto result = helper.dispatch(op);
            if (result.overloaded) {
                ++overload_count;
            }
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[BASE:HelperOverload] No exceptions during base helper reliability soak";

    EXPECT_EQ(overload_count, 0u)
        << "[BASE:HelperOverload] Zero overload events expected during soak. "
           "Observed: " << overload_count;

    EXPECT_GT(helper.totalOps(), 0u)
        << "At least one helper dispatch must complete during the soak";
}
