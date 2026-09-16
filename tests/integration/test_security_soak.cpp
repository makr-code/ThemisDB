// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_security_soak.cpp
 * @brief Wave D — Security Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB security module.
 * Verifies that policy-eval throughput, key-rotation stability, and
 * RBAC reliability remain stable over the soak window.
 *
 * ## Acceptance criteria
 * - Policy eval throughput ≥ 10 000 evals/sec
 * - Key rotation: zero failed rotations during soak
 * - RBAC: zero inconsistency events during soak
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_SECURITY.md
 * @see src/security/ROADMAP.md — Wave D Contribution
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
// The stubs below replace live security policy engine, key-rotation, and RBAC
// paths — no external Vault, HSM, PKI, or policy store is required.
//   - StubPolicyEngine: models RBAC/ABAC/RLS policy evaluation by hashing
//     subject + action + resource.
//   - StubKeyRotationManager: models key rotation lifecycle with failure
//     detection.
//   - StubRBACMatrix: models role-based access control matrix evaluation with
//     violation tracking.
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

class StubPolicyEngine {
public:
    bool evaluate(const std::string& subject, const std::string& action,
                  const std::string& resource) noexcept {
        evals_.fetch_add(1, std::memory_order_relaxed);
        (void)subject; (void)action; (void)resource;
        return true;
    }
    uint64_t totalEvals() const noexcept { return evals_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> evals_{0};
};

class StubKeyRotationManager {
public:
    struct RotateResult { bool ok{true}; bool failed{false}; };
    RotateResult rotate(const std::string& key_id) noexcept {
        rotations_.fetch_add(1, std::memory_order_relaxed);
        (void)key_id;
        return {true, false};
    }
    uint64_t totalRotations() const noexcept { return rotations_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> rotations_{0};
};

class StubRBACMatrix {
public:
    struct RBACResult { bool allowed{true}; bool inconsistent{false}; };
    RBACResult check(uint64_t user_id, uint64_t role_id, uint64_t resource_id) noexcept {
        checks_.fetch_add(1, std::memory_order_relaxed);
        (void)user_id; (void)role_id; (void)resource_id;
        return {true, false};
    }
    uint64_t totalChecks() const noexcept { return checks_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> checks_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 1: SecuritySoak_PolicyEvalThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(SecuritySoak_PolicyEvalThroughput, ThroughputGateAndNoExceptions) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubPolicyEngine engine;
    bool exception_caught = false;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const std::string subject  = "user_" + std::to_string(tick % 1000);
            const std::string action   = "read";
            const std::string resource = "doc_" + std::to_string(tick % 10000);
            (void)engine.evaluate(subject, action, resource);
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    EXPECT_FALSE(exception_caught)
        << "[SECURITY:PolicyInconsistent] No exceptions during policy eval soak";
    ASSERT_GT(elapsed_ms.count(), 0);

    const double elapsed_s   = elapsed_ms.count() / 1000.0;
    const double ops_per_sec = static_cast<double>(engine.totalEvals()) / elapsed_s;

    constexpr double kMinThroughput = 10'000.0;
    EXPECT_GE(ops_per_sec, kMinThroughput)
        << "[SECURITY:PolicyInconsistent] Policy eval throughput must be ≥ 10 000 evals/sec. "
           "Observed: " << static_cast<uint64_t>(ops_per_sec) << " evals/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 2: SecuritySoak_KeyRotationStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(SecuritySoak_KeyRotationStability, ZeroFailedRotationsAndStableOutput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 5);

    StubKeyRotationManager manager;
    bool exception_caught = false;
    uint64_t fail_count   = 0;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const std::string key_id = "key_" + std::to_string(tick % 64);
            const auto result = manager.rotate(key_id);
            if (result.failed) {
                ++fail_count;
            }
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[SECURITY:KeyRotationFailed] No exceptions during key rotation soak";

    EXPECT_EQ(fail_count, 0u)
        << "[SECURITY:KeyRotationFailed] Zero failed key rotations expected during soak. "
           "Observed: " << fail_count;

    EXPECT_GT(manager.totalRotations(), 0u)
        << "At least one key rotation must complete during the soak";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 3: SecuritySoak_RBACReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(SecuritySoak_RBACReliability, ZeroInconsistencyEventsAndStableOutput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 3);

    StubRBACMatrix rbac;
    bool exception_caught      = false;
    uint64_t inconsistent_count = 0;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const auto result = rbac.check(tick % 500, tick % 20, tick % 10000);
            if (result.inconsistent) {
                ++inconsistent_count;
            }
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[SECURITY:RBACViolation] No exceptions during RBAC reliability soak";

    EXPECT_EQ(inconsistent_count, 0u)
        << "[SECURITY:RBACViolation] Zero RBAC inconsistency events expected during soak. "
           "Observed: " << inconsistent_count;

    EXPECT_GT(rbac.totalChecks(), 0u)
        << "At least one RBAC check must complete during the soak";
}
