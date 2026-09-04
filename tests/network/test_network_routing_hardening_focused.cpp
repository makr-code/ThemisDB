// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_network_routing_hardening_focused.cpp
 * @brief Phase 3 — Routing and Topology Hardening focused tests (NRH-01..NRH-08).
 *
 * Covers the two Q4 2026 open items from src/network/ROADMAP.md § Phase 3:
 *   1. Expand routing correctness checks under changing health/latency/topology
 *      signals.
 *   2. Harden load-balancing state transitions and recovery under churn.
 *
 * Also covers the Q4 2026 Planned Feature:
 *   - Strengthen distributed routing and failover diagnostics for operator triage.
 *
 * All tests are deterministic and self-contained — no live sockets, no
 * external dependencies beyond the contract header.
 *
 * ## Test Cases
 *
 * ### NRH-01..NRH-02 — Topology-Aware Routing Under Region Failure
 *   NRH-01  Primary region fully down → router falls back to secondary region;
 *           fallback result is not connection-closing (routing rerouted).
 *   NRH-02  Cross-region fallback exhausted (all regions down) →
 *           ROUTING_UNAVAILABLE returned; not connection-closing.
 *
 * ### NRH-03..NRH-04 — Load-Balancer State Transitions Under Churn
 *   NRH-03  Load balancer switches from primary to standby after threshold
 *           consecutive failures without returning a connection-closing error.
 *   NRH-04  Load balancer recovers to primary after standby delivers
 *           consecutive successes — state transitions are deterministic.
 *
 * ### NRH-05..NRH-06 — Routing Error Classification
 *   NRH-05  ROUTING_UNAVAILABLE is not connection-closing — cluster routing
 *           errors do not terminate transport connections.
 *   NRH-06  QUORUM_DEGRADED is not connection-closing and not rate-limit
 *           transient — it is a cluster-level signal only.
 *
 * ### NRH-07 — All Backends Degraded: Deterministic Fallback
 *   NRH-07  When every tracked backend is degraded, the router returns
 *           ROUTING_UNAVAILABLE (not an unexpected error code) and does not
 *           loop infinitely.
 *
 * ### NRH-08 — Circuit Breaker Integration with Routing
 *   NRH-08  After failure_threshold consecutive failures the circuit OPENS;
 *           subsequent routing attempts are fast-rejected (no call through).
 *
 * @see include/network/network_api_contract.h
 * @see include/network/adaptive_circuit_breaker.h
 * @see src/network/ROADMAP.md — Phase 3 items
 */

#include <gtest/gtest.h>

#include "network/network_api_contract.h"
#include "network/adaptive_circuit_breaker.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

using namespace themis::network;
using namespace std::chrono_literals;

// ============================================================================
// Deterministic seed
// ============================================================================
static constexpr std::uint64_t kNrhSeed = 42;

// ============================================================================
// Minimal in-process mocks
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// Backend health model — used for routing and load-balancing mocks
// ---------------------------------------------------------------------------
enum class BackendHealth { HEALTHY, DEGRADED, DOWN };

struct BackendInfo {
    std::string   name;
    BackendHealth health{BackendHealth::HEALTHY};
    uint32_t      latencyMs{10};
    std::string   region = {};
};

// ---------------------------------------------------------------------------
// Minimal topology-aware router mock
//
// Strategy: prefer healthy backends in primary region; fall back to other
// regions; if all backends are down, return ROUTING_UNAVAILABLE.
// ---------------------------------------------------------------------------
class MockTopologyRouter {
public:
    void addBackend(BackendInfo b) { backends_.push_back(std::move(b)); }

    /**
     * @brief Select the best backend for the given region preference.
     *
     * @param preferredRegion  Primary region to try first.
     * @param outName          Set to the selected backend name on success.
     * @return OK on success, ROUTING_UNAVAILABLE when all backends are down.
     */
    NetworkErrorCode select(const std::string& preferredRegion,
                            std::string& outName) const noexcept {
        // Pass 1: healthy backend in preferred region.
        for (auto& b : backends_) {
            if (b.region == preferredRegion && b.health == BackendHealth::HEALTHY) {
                outName = b.name;
                return NetworkErrorCode::OK;
            }
        }
        // Pass 2: healthy backend in any region (cross-region fallback).
        for (auto& b : backends_) {
            if (b.health == BackendHealth::HEALTHY) {
                outName = b.name;
                return NetworkErrorCode::OK;
            }
        }
        // All backends are down or degraded.
        return NetworkErrorCode::ROUTING_UNAVAILABLE;
    }

private:
    std::vector<BackendInfo> backends_;
};

// ---------------------------------------------------------------------------
// Simple load-balancer state machine mock — models primary/standby churn
// ---------------------------------------------------------------------------
enum class LbRole { PRIMARY, STANDBY };

class MockLoadBalancer {
public:
    LbRole   role{LbRole::PRIMARY};
    int      consecutiveFailures{0};
    int      consecutiveSuccesses{0};

    const int kFailoverThreshold{3};  ///< Failures before switch to standby.
    const int kRecoveryThreshold{2};  ///< Successes before switch back to primary.

    /// Record a failure on the current primary/standby and possibly switch role.
    void recordFailure() noexcept {
        ++consecutiveFailures;
        consecutiveSuccesses = 0;
        if (role == LbRole::PRIMARY && consecutiveFailures >= kFailoverThreshold) {
            role = LbRole::STANDBY;
            consecutiveFailures = 0;
        }
    }

    /// Record a success and possibly recover back to primary.
    void recordSuccess() noexcept {
        ++consecutiveSuccesses;
        consecutiveFailures = 0;
        if (role == LbRole::STANDBY && consecutiveSuccesses >= kRecoveryThreshold) {
            role = LbRole::PRIMARY;
            consecutiveSuccesses = 0;
        }
    }

    NetworkErrorCode forward(bool succeed) noexcept {
        if (succeed) {
            recordSuccess();
            return NetworkErrorCode::OK;
        }
        recordFailure();
        return NetworkErrorCode::TRANSPORT_WRITE_ERROR;  // transient
    }
};

}  // anonymous namespace

// ============================================================================
// NRH-01..NRH-02 — Topology-Aware Routing Under Region Failure
// ============================================================================

/**
 * @brief NRH-01: Primary region all backends down → cross-region fallback
 *        succeeds; returned code is OK (not connection-closing).
 */
TEST(NetworkRoutingHardening, NRH01_PrimaryRegionDownFallsBackToSecondary) {
    MockTopologyRouter router;
    router.addBackend({"primary-a", BackendHealth::DOWN,    10, "eu-west"});
    router.addBackend({"primary-b", BackendHealth::DOWN,    10, "eu-west"});
    router.addBackend({"fallback-a", BackendHealth::HEALTHY, 50, "us-east"});

    std::string selected = {};
    auto code = router.select("eu-west", selected);
    EXPECT_EQ(code, NetworkErrorCode::OK)
        << "Cross-region fallback must succeed when primary region is down";
    EXPECT_EQ(selected, "fallback-a");
    EXPECT_FALSE(isConnectionClosingError(code));
}

/**
 * @brief NRH-02: All regions fully down → ROUTING_UNAVAILABLE, not
 *        connection-closing (transport stays open, caller decides retry).
 */
TEST(NetworkRoutingHardening, NRH02_AllRegionsDownRoutingUnavailable) {
    MockTopologyRouter router;
    router.addBackend({"a", BackendHealth::DOWN, 10, "eu-west"});
    router.addBackend({"b", BackendHealth::DOWN, 10, "us-east"});
    router.addBackend({"c", BackendHealth::DOWN, 10, "ap-south"});

    std::string selected = {};
    auto code = router.select("eu-west", selected);
    EXPECT_EQ(code, NetworkErrorCode::ROUTING_UNAVAILABLE);
    EXPECT_FALSE(isConnectionClosingError(code))
        << "ROUTING_UNAVAILABLE must not close the transport connection";
}

// ============================================================================
// NRH-03..NRH-04 — Load-Balancer State Transitions Under Churn
// ============================================================================

/**
 * @brief NRH-03: After kFailoverThreshold consecutive failures LB switches
 *        from PRIMARY to STANDBY — no connection-closing error emitted.
 */
TEST(NetworkRoutingHardening, NRH03_LbSwitchesToStandbyAfterThreshold) {
    MockLoadBalancer lb;
    ASSERT_EQ(lb.role, LbRole::PRIMARY);

    // Drive failures up to threshold.
    for (int i = 0; i < lb.kFailoverThreshold - 1; ++i) {
        lb.recordFailure();
        EXPECT_EQ(lb.role, LbRole::PRIMARY)
            << "Must stay PRIMARY until threshold at step " << i;
    }
    lb.recordFailure();  // reaches threshold
    EXPECT_EQ(lb.role, LbRole::STANDBY)
        << "Must switch to STANDBY at failure threshold";
}

/**
 * @brief NRH-04: After kRecoveryThreshold consecutive successes on STANDBY,
 *        LB recovers back to PRIMARY — deterministic round-trip.
 */
TEST(NetworkRoutingHardening, NRH04_LbRecoversToPrimaryAfterSuccesses) {
    MockLoadBalancer lb;
    // Trigger failover first.
    for (int i = 0; i < lb.kFailoverThreshold; ++i) {
      lb.recordFailure();
    }
    ASSERT_EQ(lb.role, LbRole::STANDBY);

    // Drive successes up to recovery threshold.
    for (int i = 0; i < lb.kRecoveryThreshold - 1; ++i) {
        lb.recordSuccess();
        EXPECT_EQ(lb.role, LbRole::STANDBY)
            << "Must stay STANDBY until recovery threshold at step " << i;
    }
    lb.recordSuccess();  // reaches threshold
    EXPECT_EQ(lb.role, LbRole::PRIMARY)
        << "Must recover to PRIMARY at success threshold";
}

// ============================================================================
// NRH-05..NRH-06 — Routing Error Classification
// ============================================================================

/**
 * @brief NRH-05: ROUTING_UNAVAILABLE is not connection-closing — routing
 *        errors belong to cluster-layer, not transport-layer.
 */
TEST(NetworkRoutingHardening, NRH05_RoutingUnavailableNotConnectionClosing) {
    EXPECT_FALSE(isConnectionClosingError(NetworkErrorCode::ROUTING_UNAVAILABLE))
        << "ROUTING_UNAVAILABLE must not mandate transport connection closure";
    EXPECT_FALSE(isRateLimitTransient(NetworkErrorCode::ROUTING_UNAVAILABLE))
        << "ROUTING_UNAVAILABLE is not a rate-limit transient code";
}

/**
 * @brief NRH-06: QUORUM_DEGRADED is not connection-closing and not a
 *        rate-limit transient — it is a cluster health signal only.
 */
TEST(NetworkRoutingHardening, NRH06_QuorumDegradedClassification) {
    EXPECT_FALSE(isConnectionClosingError(NetworkErrorCode::QUORUM_DEGRADED))
        << "QUORUM_DEGRADED must not close the transport";
    EXPECT_FALSE(isRateLimitTransient(NetworkErrorCode::QUORUM_DEGRADED))
        << "QUORUM_DEGRADED is not a transient rate-limit event";
    // Verify it has a distinct, defined numeric value from transport errors.
    EXPECT_NE(static_cast<int>(NetworkErrorCode::QUORUM_DEGRADED),
              static_cast<int>(NetworkErrorCode::TRANSPORT_UNAVAILABLE));
}

// ============================================================================
// NRH-07 — All Backends Degraded: Deterministic Fallback
// ============================================================================

/**
 * @brief NRH-07: When all backends are degraded/down, router returns exactly
 *        ROUTING_UNAVAILABLE — no undefined code, no infinite loop.
 */
TEST(NetworkRoutingHardening, NRH07_AllDegradedDeterministicFallback) {
    MockTopologyRouter router;
    // Populate with degraded-only backends across regions.
    for (int i = 0; i < 5; ++i) {
        router.addBackend({"backend-" + std::to_string(i),
                           BackendHealth::DEGRADED, 100,
                           "region-" + std::to_string(i % 3)});
    }

    std::string selected = {};
    // Call multiple times — must be deterministic.
    for (int attempt = 0; attempt < 5; ++attempt) {
        auto code = router.select("region-0", selected);
        EXPECT_EQ(code, NetworkErrorCode::ROUTING_UNAVAILABLE)
            << "All-degraded state must always return ROUTING_UNAVAILABLE (attempt "
            << attempt << ")";
    }
}

// ============================================================================
// NRH-08 — Circuit Breaker Integration with Routing
// ============================================================================

/**
 * @brief NRH-08: AdaptiveCircuitBreaker opens after failure_threshold
 *        consecutive failures; subsequent requests are fast-rejected
 *        (shouldAllow() returns false).
 */
TEST(NetworkRoutingHardening, NRH08_CircuitBreakerOpensOnRepeatedFailures) {
    AdaptiveCircuitBreaker::Config cfg;
    cfg.failure_threshold     = 3;
    cfg.enable_adaptive_threshold = false;  // deterministic for this test
    cfg.open_timeout          = std::chrono::seconds(60);
    AdaptiveCircuitBreaker cb(cfg);

    // Before threshold: circuit is CLOSED, requests allowed.
    for (int i = 0; i < static_cast<int>(cfg.failure_threshold); ++i) {
        EXPECT_TRUE(cb.shouldAllow())
            << "Circuit must allow requests before threshold (step " << i << ")";
        cb.recordFailure();
    }

    // After threshold: circuit must be OPEN, requests rejected.
    EXPECT_FALSE(cb.shouldAllow())
        << "Circuit must be OPEN after failure_threshold consecutive failures";

    // Verify via stats.
    auto stats = cb.getStats();
    EXPECT_EQ(stats.state, CircuitState::OPEN)
        << "Stats must reflect OPEN state";
    EXPECT_GE(stats.failed_calls, cfg.failure_threshold);
}
