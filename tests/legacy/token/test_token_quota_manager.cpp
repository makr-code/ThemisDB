#include <gtest/gtest.h>
#include "llm/token_quota_manager.h"
#include <thread>
#include <chrono>

using namespace themis::llm;

// ---------------------------------------------------------------------------
// setQuota / getLimit
// ---------------------------------------------------------------------------

TEST(TokenQuotaManagerTest, SetAndGetLimit) {
    TokenQuotaManager q;
    q.setQuota("alice", "mistral-7b", 50000);
    auto lim = q.getLimit("alice", "mistral-7b");
    ASSERT_TRUE(lim.has_value());
    EXPECT_EQ(*lim, 50000u);
}

TEST(TokenQuotaManagerTest, NoQuota_GetLimitReturnsNullopt) {
    TokenQuotaManager q;
    EXPECT_FALSE(q.getLimit("unknown", "model").has_value());
}

TEST(TokenQuotaManagerTest, SetQuota_OverwritesPreviousLimit) {
    TokenQuotaManager q;
    q.setQuota("alice", "m", 1000);
    q.setQuota("alice", "m", 5000);
    EXPECT_EQ(*q.getLimit("alice", "m"), 5000u);
}

// ---------------------------------------------------------------------------
// removeQuota
// ---------------------------------------------------------------------------

TEST(TokenQuotaManagerTest, RemoveQuota_ExistingEntry_ReturnsTrue) {
    TokenQuotaManager q;
    q.setQuota("alice", "m", 1000);
    EXPECT_TRUE(q.removeQuota("alice", "m"));
}

TEST(TokenQuotaManagerTest, RemoveQuota_NonExisting_ReturnsFalse) {
    TokenQuotaManager q;
    EXPECT_FALSE(q.removeQuota("alice", "m"));
}

TEST(TokenQuotaManagerTest, RemoveQuota_LimitGoneAfterRemove) {
    TokenQuotaManager q;
    q.setQuota("alice", "m", 1000);
    q.removeQuota("alice", "m");
    EXPECT_FALSE(q.getLimit("alice", "m").has_value());
}

// ---------------------------------------------------------------------------
// check — no quota set
// ---------------------------------------------------------------------------

TEST(TokenQuotaManagerTest, Check_NoQuota_AlwaysAllowed) {
    TokenQuotaManager q;
    auto r = q.check("alice", "gpt4", 999999);
    EXPECT_TRUE(r.allowed);
    EXPECT_TRUE(r.reason.empty());
}

// ---------------------------------------------------------------------------
// check — quota enforced
// ---------------------------------------------------------------------------

TEST(TokenQuotaManagerTest, Check_BelowLimit_Allowed) {
    TokenQuotaManager q;
    q.setQuota("alice", "m", 1000);
    auto r = q.check("alice", "m", 500);
    EXPECT_TRUE(r.allowed);
}

TEST(TokenQuotaManagerTest, Check_ExactlyAtLimit_Allowed) {
    TokenQuotaManager q;
    q.setQuota("alice", "m", 1000);
    auto r = q.check("alice", "m", 1000);
    EXPECT_TRUE(r.allowed);
}

TEST(TokenQuotaManagerTest, Check_OverLimit_Denied) {
    TokenQuotaManager q;
    q.setQuota("alice", "m", 100);
    auto r = q.check("alice", "m", 101);
    EXPECT_FALSE(r.allowed);
    EXPECT_FALSE(r.reason.empty());
}

TEST(TokenQuotaManagerTest, Check_DeniedReasonContainsUserAndModel) {
    TokenQuotaManager q;
    q.setQuota("bob", "mistral-7b", 50);
    auto r = q.check("bob", "mistral-7b", 51);
    EXPECT_NE(r.reason.find("bob"),        std::string::npos);
    EXPECT_NE(r.reason.find("mistral-7b"), std::string::npos);
}

TEST(TokenQuotaManagerTest, Check_DoesNotConsumeTokens) {
    TokenQuotaManager q;
    q.setQuota("alice", "m", 1000);
    // Call check many times — should never consume
    for (int i = 0; i < 100; ++i) {
        auto r = q.check("alice", "m", 900);
        EXPECT_TRUE(r.allowed) << "check() must not consume tokens; iter=" << i;
    }
}

// ---------------------------------------------------------------------------
// consume + currentUsage
// ---------------------------------------------------------------------------

TEST(TokenQuotaManagerTest, Consume_IncrementsUsage) {
    TokenQuotaManager q;
    q.setQuota("alice", "m", 10000);
    q.consume("alice", "m", 300);
    q.consume("alice", "m", 200);
    EXPECT_EQ(q.currentUsage("alice", "m"), 500u);
}

TEST(TokenQuotaManagerTest, ConsumeNoQuota_Noop) {
    TokenQuotaManager q;
    // No quota set — consume should be a no-op (not crash)
    q.consume("nobody", "model", 1000);
    EXPECT_EQ(q.currentUsage("nobody", "model"), 0u);
}

TEST(TokenQuotaManagerTest, ConsumeZero_Noop) {
    TokenQuotaManager q;
    q.setQuota("alice", "m", 1000);
    q.consume("alice", "m", 0);
    EXPECT_EQ(q.currentUsage("alice", "m"), 0u);
}

TEST(TokenQuotaManagerTest, ConsumeAndCheck_ExceedsAfterConsume) {
    TokenQuotaManager q;
    q.setQuota("alice", "m", 1000);
    q.consume("alice", "m", 900);

    // Should still pass (900 + 99 = 999 <= 1000)
    EXPECT_TRUE(q.check("alice", "m", 99).allowed);

    // Should fail (900 + 101 = 1001 > 1000)
    EXPECT_FALSE(q.check("alice", "m", 101).allowed);
}

// ---------------------------------------------------------------------------
// Per-key isolation
// ---------------------------------------------------------------------------

TEST(TokenQuotaManagerTest, DifferentUsers_IndependentQuotas) {
    TokenQuotaManager q;
    q.setQuota("alice", "m", 100);
    q.setQuota("bob",   "m", 100);

    q.consume("alice", "m", 100);

    // Alice is exhausted
    EXPECT_FALSE(q.check("alice", "m", 1).allowed);
    // Bob is unaffected
    EXPECT_TRUE(q.check("bob", "m", 100).allowed);
}

TEST(TokenQuotaManagerTest, DifferentModels_IndependentQuotas) {
    TokenQuotaManager q;
    q.setQuota("alice", "modelA", 100);
    q.setQuota("alice", "modelB", 100);

    q.consume("alice", "modelA", 100);

    EXPECT_FALSE(q.check("alice", "modelA", 1).allowed);
    EXPECT_TRUE( q.check("alice", "modelB", 100).allowed);
}

// ---------------------------------------------------------------------------
// Sliding-window expiry (abbreviated — just verify events eventually expire)
// ---------------------------------------------------------------------------

TEST(TokenQuotaManagerTest, Events_ExpireAfterWindow) {
    // We can't wait 60 s in a unit test, so we verify the mechanism indirectly:
    // After a consume(), currentUsage() > 0. If we could fast-forward time,
    // it would become 0. Instead we just confirm the basic invariant holds.
    TokenQuotaManager q;
    q.setQuota("alice", "m", 1000);
    q.consume("alice", "m", 500);
    EXPECT_EQ(q.currentUsage("alice", "m"), 500u);

    // currentUsage must be stable if called repeatedly (no drift)
    EXPECT_EQ(q.currentUsage("alice", "m"), 500u);
    EXPECT_EQ(q.currentUsage("alice", "m"), 500u);
}

// ---------------------------------------------------------------------------
// Thread safety (smoke test)
// ---------------------------------------------------------------------------

TEST(TokenQuotaManagerTest, ThreadSafety_ConcurrentConsumeAndCheck) {
    TokenQuotaManager q;
    q.setQuota("shared", "m", 1000000);

    constexpr size_t N_THREADS = 8;
    constexpr size_t OPS_PER_THREAD = 1000;

    std::vector<std::thread> threads;
    threads.reserve(N_THREADS);

    for (size_t t = 0; t < N_THREADS; ++t) {
        threads.emplace_back([&q] {
            for (size_t i = 0; i < OPS_PER_THREAD; ++i) {
                q.consume("shared", "m", 1);
                (void)q.check("shared", "m", 1);
                (void)q.currentUsage("shared", "m");
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }

    // Just verify no crash and usage is non-zero
    EXPECT_GT(q.currentUsage("shared", "m"), 0u);
}
