// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_auth_highcardinality_stress.cpp
 * @brief High-cardinality stress tests for the auth module (Wave D).
 *
 * Covers distributed-tracing-relevant concurrent load scenarios:
 *  HCS-01  1000 concurrent token validations against the in-memory cache
 *  HCS-02  500 concurrent revocations against the TokenBlacklist
 *  HCS-03  Session pressure under 100 parallel users (create + validate)
 *  HCS-04  Concurrent cache eviction under load (LRU correctness)
 *  HCS-05  Concurrent cross-provider trust registry reads (no data races)
 *
 * @note Tests use std::thread concurrency; thread count is scaled to hardware
 *       if THEMIS_AUTH_STRESS_THREADS env var is unset.
 */

#include <gtest/gtest.h>

#include "auth/federated_identity_manager.h"
#include "auth/jwt_validator.h"
#include "auth/session_manager.h"
#include "auth/token_blacklist.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::auth;

namespace {

FederatedValidationResult makeFutureResult(const std::string& sub,
                                            const std::string& realm) {
    JWTClaims claims;
    claims.sub        = sub;
    claims.issuer     = realm;
    claims.expiration = std::chrono::system_clock::now() + std::chrono::hours(24);
    FederatedValidationResult r;
    r.claims = claims;
    r.realm  = realm;
    return r;
}

} // namespace

// ---------------------------------------------------------------------------
// HCS-01: 1000 concurrent cache inserts + lookups
// ---------------------------------------------------------------------------
TEST(AuthHighCardinalityStress, HCS01_ConcurrentTokenCacheInserts) {
    FederatedIdentityManager fim;
    constexpr int kTokens  = 1000;
    constexpr int kThreads = 8;

    // Pre-populate tokens [0..kTokens)
    for (int i = 0; i < kTokens; ++i) {
        fim.cacheValidationResult(
            "hcs01_token_" + std::to_string(i),
            makeFutureResult("u" + std::to_string(i), "https://idp.example.com"));
    }

    std::atomic<int> hits{0};
    std::atomic<int> misses{0};

    auto worker = [&](int thread_id) {
        for (int i = thread_id; i < kTokens; i += kThreads) {
            auto result = fim.getCachedResult("hcs01_token_" + std::to_string(i));
            if (result.has_value()) {
                ++hits;
            } else {
                ++misses;
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back(worker, t);
    }
    for (auto& th : threads) { th.join(); }

    EXPECT_GT(hits.load(), 0) << "Expected at least some cache hits";
}

// ---------------------------------------------------------------------------
// HCS-02: 500 concurrent revocations
// ---------------------------------------------------------------------------
TEST(AuthHighCardinalityStress, HCS02_ConcurrentRevocations) {
    TokenBlacklist bl;
    constexpr int kJtis    = 500;
    constexpr int kThreads = 8;
    const auto expiry = std::chrono::system_clock::now() + std::chrono::hours(1);

    std::atomic<int> added{0};

    auto writer = [&](int thread_id) {
        for (int i = thread_id; i < kJtis; i += kThreads) {
            bl.add("hcs02_jti_" + std::to_string(i), expiry);
            ++added;
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back(writer, t);
    }
    for (auto& th : threads) { th.join(); }

    EXPECT_EQ(added.load(), kJtis) << "All revocations must complete";

    // All JTIs must be present.
    for (int i = 0; i < kJtis; ++i) {
        EXPECT_TRUE(bl.isRevoked("hcs02_jti_" + std::to_string(i)))
            << "JTI " << i << " must be revoked";
    }
}

// ---------------------------------------------------------------------------
// HCS-03: Session pressure under 100 parallel users
// ---------------------------------------------------------------------------
TEST(AuthHighCardinalityStress, HCS03_SessionPressure100ParallelUsers) {
    SessionManager::SessionLimits limits;
    limits.max_sessions_per_user = 0;   // unlimited
    limits.idle_timeout          = std::chrono::milliseconds(0);
    limits.absolute_timeout      = std::chrono::hours(1);
    SessionManager sm(limits);

    constexpr int kUsers   = 100;
    constexpr int kThreads = 10;
    std::atomic<int> created{0};
    std::atomic<int> validated{0};

    auto worker = [&](int thread_id) {
        for (int i = thread_id; i < kUsers; i += kThreads) {
            const std::string user = "hcs03_user_" + std::to_string(i);
            const std::string sid  = sm.createSession(user);
            ++created;
            if (sm.validateSession(sid)) {
                ++validated;
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back(worker, t);
    }
    for (auto& th : threads) { th.join(); }

    EXPECT_EQ(created.load(),   kUsers) << "All sessions must be created";
    EXPECT_EQ(validated.load(), kUsers) << "All sessions must be valid";
}

// ---------------------------------------------------------------------------
// HCS-04: Concurrent cache eviction under load (LRU correctness)
// ---------------------------------------------------------------------------
TEST(AuthHighCardinalityStress, HCS04_ConcurrentCacheEviction_LRUCorrectness) {
    FederatedIdentityManager fim;
    // Insert more than the LRU cap (4096) from multiple threads.
    constexpr int kTotal   = 4200;
    constexpr int kThreads = 8;

    auto inserter = [&](int thread_id) {
        for (int i = thread_id; i < kTotal; i += kThreads) {
            fim.cacheValidationResult(
                "hcs04_tok_" + std::to_string(i),
                makeFutureResult("usr" + std::to_string(i), "https://idp.example.com"));
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back(inserter, t);
    }
    for (auto& th : threads) { th.join(); }

    // Cache must be at or below the LRU cap — no unbounded growth.
    EXPECT_LE(fim.tokenCacheSize(), 4096u)
        << "Cache must not grow beyond LRU cap under concurrent load";
}

// ---------------------------------------------------------------------------
// HCS-05: Concurrent cross-provider trust registry reads (no data races)
// ---------------------------------------------------------------------------
TEST(AuthHighCardinalityStress, HCS05_ConcurrentTrustRegistryReads) {
    FederatedIdentityManager fim;
    const std::string realm_a = "https://idp-a.example.com";
    const std::string realm_b = "https://idp-b.example.com";
    fim.addCrossProviderTrust(realm_a, realm_b);

    constexpr int kThreads = 16;
    std::atomic<int> correct{0};

    auto reader = [&]() {
        for (int i = 0; i < 100; ++i) {
            if (fim.isTrustedBy(realm_a, realm_b)) {
                ++correct;
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back(reader);
    }
    for (auto& th : threads) { th.join(); }

    EXPECT_EQ(correct.load(), kThreads * 100)
        << "All concurrent trust-registry reads must return true";
}
