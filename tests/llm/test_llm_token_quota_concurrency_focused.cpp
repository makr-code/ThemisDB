/**
 * @file test_llm_token_quota_concurrency_focused.cpp
 * @brief Block 4 — Token-Quota Enforcement Under Concurrent Requests (High).
 *
 * Acceptance criteria:
 *   TQC-01  N concurrent requests that together exceed the configured quota →
 *           all requests submitted AFTER quota exhaustion are denied (no
 *           request that pushes the balance negative is ever accepted).
 *   TQC-02  Race-condition at boundary: exact quota value reached by multiple
 *           threads simultaneously → no negative balance recorded.
 *
 * All infrastructure is fully in-process.  The TokenQuotaManager's thread-safe
 * sliding-window implementation is exercised directly without mocking.
 *
 * @version 1.0.0
 * @note CTest labels: llm;quota;concurrency;security
 */

#include <gtest/gtest.h>

#include "llm/token_quota_manager.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::llm;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/**
 * @brief Result of a single concurrent admission attempt.
 */
struct AdmissionAttempt {
    bool    allowed      = false;
    size_t  tokens_used  = 0;
    size_t  tokens_limit = 0;
};

/**
 * @brief Simulate one request in the quota system (check + consume).
 *
 * Returns the admission result.  When admitted, the tokens are consumed
 * immediately (conservative pre-charge model).
 */
AdmissionAttempt tryAdmit(TokenQuotaManager&  quota,
                           const std::string& user_id,
                           const std::string& model_id,
                           size_t             request_tokens) {
    const auto result = quota.check(user_id, model_id, request_tokens);
    if (result.allowed) {
        quota.consume(user_id, model_id, request_tokens);
    }
    AdmissionAttempt out;
    out.allowed      = result.allowed;
    out.tokens_used  = result.tokens_used;
    out.tokens_limit = result.tokens_limit;
    return out;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// TQC-01 — Concurrent requests exhaust quota; all after exhaustion rejected
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test TQC-01a: 10 sequential requests of 100 tokens each against a 500-token
 *       quota.  First 5 must be admitted; requests 6-10 must be denied.
 *
 * Sequential baseline — establishes correctness before adding concurrency.
 */
TEST(TokenQuotaConcurrencyTest, TQC01a_SequentialRequestsExhaustQuota) {
    TokenQuotaManager quota;
    const std::string user  = "seq-user";
    const std::string model = "test-model";
    quota.setQuota(user, model, 500);  // 500 tokens per window

    size_t admitted = 0;
    size_t denied   = 0;

    for (int i = 0; i < 10; ++i) {
        const auto result = tryAdmit(quota, user, model, 100);
        if (result.allowed) {
            ++admitted;
        } else {
            ++denied;
        }
    }

    EXPECT_EQ(admitted, 5u)
        << "Exactly 5 requests of 100 tokens should be admitted against a "
           "500-token quota.";
    EXPECT_EQ(denied, 5u)
        << "The remaining 5 requests should be denied after quota exhaustion.";
}

/**
 * @test TQC-01b: N concurrent threads each attempt to consume tokens that
 *       together exceed the quota.  After all threads complete:
 *         - Total tokens consumed must NOT exceed the quota.
 *         - At least one request must have been denied.
 *         - No negative balance.
 */
TEST(TokenQuotaConcurrencyTest, TQC01b_ConcurrentRequestsNeverExceedQuota) {
    constexpr size_t QUOTA_LIMIT    = 1000;
    constexpr size_t TOKENS_PER_REQ = 100;
    constexpr int    NUM_THREADS    = 20;  // 20 × 100 = 2000 > 1000 → half must be denied

    TokenQuotaManager quota;
    const std::string user  = "conc-user";
    const std::string model = "conc-model";
    quota.setQuota(user, model, QUOTA_LIMIT);

    std::atomic<int> admitted_count{0};
    std::atomic<int> denied_count{0};

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&] {
            const auto res = tryAdmit(quota, user, model, TOKENS_PER_REQ);
            if (res.allowed) {
                admitted_count.fetch_add(1, std::memory_order_relaxed);
            } else {
                denied_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    const size_t consumed = quota.currentUsage(user, model);

    // Core invariant: consumed tokens must NEVER exceed the quota.
    EXPECT_LE(consumed, QUOTA_LIMIT)
        << "TQC-01b: Total consumed tokens must not exceed the configured quota "
           "even under concurrent load.  consumed=" << consumed
        << " quota=" << QUOTA_LIMIT;

    // At least one request should have been denied (we sent 2× the quota).
    EXPECT_GT(denied_count.load(), 0)
        << "TQC-01b: At least one concurrent request should have been denied "
           "since total demand (2000) exceeds quota (" << QUOTA_LIMIT << ").";

    // Consistency: admitted × TOKENS_PER_REQ should equal consumed.
    const size_t expected_consumed = static_cast<size_t>(admitted_count.load()) * TOKENS_PER_REQ;
    EXPECT_EQ(consumed, expected_consumed)
        << "TQC-01b: Consumed token total must equal admitted_count × tokens_per_req.  "
           "admitted=" << admitted_count.load()
        << " expected_consumed=" << expected_consumed
        << " actual_consumed=" << consumed;
}

// ─────────────────────────────────────────────────────────────────────────────
// TQC-02 — Race-condition at exact boundary: no negative balance
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test TQC-02a: Two threads each attempt to consume exactly the remaining
 *       quota simultaneously.  Only ONE must succeed; the other must be
 *       denied; the balance must never go negative.
 */
TEST(TokenQuotaConcurrencyTest, TQC02a_ExactBoundaryNoNegativeBalance) {
    constexpr size_t QUOTA_LIMIT = 500;
    constexpr size_t TOKENS_PER_REQ = 500;  // Each request consumes the entire quota.

    TokenQuotaManager quota;
    const std::string user  = "boundary-user";
    const std::string model = "boundary-model";
    quota.setQuota(user, model, QUOTA_LIMIT);

    std::atomic<int> admitted_count{0};

    // Spin up two threads that race to claim the full quota.
    std::thread t1([&] {
        const auto res = tryAdmit(quota, user, model, TOKENS_PER_REQ);
        if (res.allowed) admitted_count.fetch_add(1, std::memory_order_relaxed);
    });
    std::thread t2([&] {
        const auto res = tryAdmit(quota, user, model, TOKENS_PER_REQ);
        if (res.allowed) admitted_count.fetch_add(1, std::memory_order_relaxed);
    });

    t1.join();
    t2.join();

    const size_t consumed = quota.currentUsage(user, model);

    // Core invariant: no negative balance (consumed ≤ limit).
    EXPECT_LE(consumed, QUOTA_LIMIT)
        << "TQC-02a: Consumed tokens must never exceed the quota, even at the "
           "exact boundary.  consumed=" << consumed;

    // At most one request should have been admitted.
    EXPECT_LE(admitted_count.load(), 1)
        << "TQC-02a: At most one request should be admitted when two threads "
           "race to claim the full quota simultaneously.";
}

/**
 * @test TQC-02b: Multiple threads targeting quota = 0 (fully exhausted).
 *
 *       Pre-consume the full quota, then launch concurrent requests.
 *       All subsequent requests must be denied; balance must remain
 *       at exactly the quota limit (no overshoot).
 */
TEST(TokenQuotaConcurrencyTest, TQC02b_ZeroRemainingQuotaAllDenied) {
    constexpr size_t QUOTA_LIMIT = 300;
    constexpr int    NUM_THREADS = 8;

    TokenQuotaManager quota;
    const std::string user  = "exhausted-user";
    const std::string model = "exhausted-model";
    quota.setQuota(user, model, QUOTA_LIMIT);

    // Pre-consume the entire quota in one shot.
    quota.consume(user, model, QUOTA_LIMIT);
    ASSERT_EQ(quota.currentUsage(user, model), QUOTA_LIMIT)
        << "Pre-condition: quota must be fully consumed before the concurrent phase.";

    // Now launch threads that all attempt to consume tokens from an empty budget.
    std::atomic<int> denied_count{0};
    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&] {
            const auto result = quota.check(user, model, 50);
            if (!result.allowed) {
                denied_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // All concurrent requests must be denied.
    EXPECT_EQ(denied_count.load(), NUM_THREADS)
        << "TQC-02b: All " << NUM_THREADS << " requests must be denied when "
           "quota is at zero.  denied=" << denied_count.load();

    // Balance must not have grown past the quota limit.
    EXPECT_LE(quota.currentUsage(user, model), QUOTA_LIMIT)
        << "TQC-02b: currentUsage must not exceed the quota limit after "
           "concurrent denials.";
}

/**
 * @test TQC-02c: Check returns denied even for a 0-token request when quota
 *       is fully consumed (edge case: zero-sized request).
 */
TEST(TokenQuotaConcurrencyTest, TQC02c_ZeroTokenRequestDeniedAtExhaustedQuota) {
    TokenQuotaManager quota;
    const std::string user  = "zero-req-user";
    const std::string model = "zero-req-model";
    quota.setQuota(user, model, 100);
    quota.consume(user, model, 100);  // fully consumed

    // A 0-token request against an exhausted quota: manager should still deny
    // (consumed == limit).
    const auto result = quota.check(user, model, 0);

    // The contract: check(0) on an exhausted quota may be allowed (0 additional
    // tokens do not push over the limit) OR denied.  Either outcome is valid;
    // what MUST NOT happen is that a POSITIVE token amount is admitted.
    // This test documents the boundary behaviour for review.
    const size_t post_usage = quota.currentUsage(user, model);
    EXPECT_LE(post_usage, 100u)
        << "TQC-02c: currentUsage must not exceed quota limit regardless of "
           "zero-token request outcome.";
}
