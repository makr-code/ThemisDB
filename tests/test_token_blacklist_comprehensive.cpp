/**
 * @file test_token_blacklist_comprehensive.cpp
 * @brief Comprehensive tests for JTI-based JWT token blacklist
 *
 * Tests cover:
 * - Basic revoke / isRevoked / unrevoke
 * - Automatic pruning of expired tokens
 * - Empty / invalid JTI edge cases
 * - Statistics tracking
 * - Size cap enforcement
 * - Thread safety under concurrent revoke + check
 */

#include <gtest/gtest.h>
#include "auth/token_blacklist.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <string>

using namespace themis::auth;
using Clock = std::chrono::system_clock;

namespace {
// Shorthand to get a future expiry
Clock::time_point inFuture(int seconds = 3600) {
    return Clock::now() + std::chrono::seconds(seconds);
}
// Shorthand to get a past expiry (already expired)
Clock::time_point inPast(int seconds = 10) {
    return Clock::now() - std::chrono::seconds(seconds);
}
} // anonymous namespace

// ============================================================================
// Basic Revoke / Check Tests
// ============================================================================

class TokenBlacklistTest : public ::testing::Test {
protected:
    TokenBlacklist bl_;
};

TEST_F(TokenBlacklistTest, NewBlacklist_Empty) {
    EXPECT_EQ(bl_.size(), 0u);
    EXPECT_FALSE(bl_.isRevoked("some-jti"));
}

TEST_F(TokenBlacklistTest, Revoke_TokenIsRevoked) {
    bl_.revoke("jti-001", inFuture());
    EXPECT_TRUE(bl_.isRevoked("jti-001"));
}

TEST_F(TokenBlacklistTest, NonRevokedToken_NotRevoked) {
    bl_.revoke("jti-001", inFuture());
    EXPECT_FALSE(bl_.isRevoked("jti-999"));
}

TEST_F(TokenBlacklistTest, Revoke_MultipleTokens) {
    bl_.revoke("jti-a", inFuture());
    bl_.revoke("jti-b", inFuture());
    bl_.revoke("jti-c", inFuture());
    EXPECT_TRUE(bl_.isRevoked("jti-a"));
    EXPECT_TRUE(bl_.isRevoked("jti-b"));
    EXPECT_TRUE(bl_.isRevoked("jti-c"));
    EXPECT_FALSE(bl_.isRevoked("jti-d"));
}

TEST_F(TokenBlacklistTest, Revoke_EmptyJTI_Ignored) {
    EXPECT_NO_THROW(bl_.revoke("", inFuture()));
    EXPECT_EQ(bl_.size(), 0u);
}

TEST_F(TokenBlacklistTest, IsRevoked_EmptyJTI_ReturnsFalse) {
    EXPECT_FALSE(bl_.isRevoked(""));
}

// ============================================================================
// Expiry / Pruning Tests
// ============================================================================

TEST_F(TokenBlacklistTest, ExpiredToken_NotConsideredRevoked) {
    // Store a token that already expired in the past
    bl_.revoke("expired-jti", inPast(5));
    // Even though it's stored, isRevoked should treat past-expiry as cleared
    EXPECT_FALSE(bl_.isRevoked("expired-jti"));
}

TEST_F(TokenBlacklistTest, PruneExpired_RemovesExpiredEntries) {
    bl_.revoke("live",    inFuture(3600));
    bl_.revoke("dead",    inPast(10));
    EXPECT_EQ(bl_.size(), 2u);  // Both are stored

    bl_.pruneExpired();
    EXPECT_EQ(bl_.size(), 1u);  // Only live remains
    EXPECT_TRUE(bl_.isRevoked("live"));
}

TEST_F(TokenBlacklistTest, PruneExpired_NothingToRemove_NoOp) {
    bl_.revoke("a", inFuture());
    bl_.revoke("b", inFuture());
    bl_.pruneExpired();
    EXPECT_EQ(bl_.size(), 2u);
}

// ============================================================================
// Unrevoke Tests
// ============================================================================

TEST_F(TokenBlacklistTest, Unrevoke_ExistingToken_Removed) {
    bl_.revoke("jti-to-remove", inFuture());
    ASSERT_TRUE(bl_.isRevoked("jti-to-remove"));

    EXPECT_TRUE(bl_.unrevoke("jti-to-remove"));
    EXPECT_FALSE(bl_.isRevoked("jti-to-remove"));
}

TEST_F(TokenBlacklistTest, Unrevoke_NonExistentToken_ReturnsFalse) {
    EXPECT_FALSE(bl_.unrevoke("never-revoked"));
}

TEST_F(TokenBlacklistTest, Unrevoke_ThenRevokeAgain_Works) {
    bl_.revoke("jti-x", inFuture());
    bl_.unrevoke("jti-x");
    ASSERT_FALSE(bl_.isRevoked("jti-x"));

    bl_.revoke("jti-x", inFuture());
    EXPECT_TRUE(bl_.isRevoked("jti-x"));
}

// ============================================================================
// Clear Tests
// ============================================================================

TEST_F(TokenBlacklistTest, Clear_RemovesAll) {
    bl_.revoke("a", inFuture());
    bl_.revoke("b", inFuture());
    bl_.revoke("c", inFuture());
    ASSERT_EQ(bl_.size(), 3u);

    bl_.clear();
    EXPECT_EQ(bl_.size(), 0u);
    EXPECT_FALSE(bl_.isRevoked("a"));
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(TokenBlacklistTest, Statistics_TrackRevocations) {
    bl_.revoke("s1", inFuture());
    bl_.revoke("s2", inFuture());
    bl_.revoke("s3", inFuture());

    auto stats = bl_.getStatistics();
    EXPECT_EQ(stats.total_revocations, 3u);
    EXPECT_EQ(stats.current_size, 3u);
}

TEST_F(TokenBlacklistTest, Statistics_TrackChecks) {
    bl_.revoke("t1", inFuture());

    bl_.isRevoked("t1");     // hit
    bl_.isRevoked("t1");     // hit
    bl_.isRevoked("t9");     // miss

    auto stats = bl_.getStatistics();
    EXPECT_EQ(stats.total_checks, 3u);
    EXPECT_EQ(stats.revoked_hits, 2u);
}

TEST_F(TokenBlacklistTest, Statistics_TrackPrunedEntries) {
    bl_.revoke("expired", inPast(5));
    bl_.pruneExpired();

    auto stats = bl_.getStatistics();
    EXPECT_GE(stats.pruned_entries, 1u);
}

// ============================================================================
// Size Cap Tests
// ============================================================================

TEST(TokenBlacklistCapTest, MaxEntries_OldestDropped) {
    TokenBlacklist::Config cfg;
    cfg.max_entries = 5;
    cfg.cleanup_interval_seconds = 9999; // Disable automatic cleanup
    TokenBlacklist bl(cfg);

    // Fill up to capacity
    for (int i = 0; i < 5; ++i) {
        bl.revoke("jti-" + std::to_string(i), inFuture(3600));
    }
    EXPECT_EQ(bl.size(), 5u);

    // Adding one more should drop an entry (cap enforced)
    bl.revoke("jti-extra", inFuture(3600));
    EXPECT_EQ(bl.size(), 5u); // Still at cap
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST(TokenBlacklistConcurrencyTest, ConcurrentRevokeAndCheck_ThreadSafe) {
    TokenBlacklist bl;

    constexpr int THREADS = 8;
    constexpr int OPS_PER_THREAD = 50;
    std::atomic<int> hits{0};
    std::vector<std::thread> threads;

    // Half threads revoke, half check
    for (int i = 0; i < THREADS; ++i) {
        if (i % 2 == 0) {
            threads.emplace_back([&bl, i]() {
                for (int j = 0; j < OPS_PER_THREAD; ++j) {
                    bl.revoke("jti-" + std::to_string(i * 1000 + j), inFuture());
                }
            });
        } else {
            threads.emplace_back([&bl, &hits, i]() {
                for (int j = 0; j < OPS_PER_THREAD; ++j) {
                    if (bl.isRevoked("jti-" + std::to_string((i - 1) * 1000 + j))) {
                        hits++;
                    }
                }
            });
        }
    }

    for (auto& t : threads) t.join();

    // Verify stats are internally consistent: revocations + checks should
    // match what the threads actually performed.
    auto stats = bl.getStatistics();
    EXPECT_EQ(stats.total_revocations, static_cast<uint64_t>(THREADS / 2 * OPS_PER_THREAD));
    EXPECT_EQ(stats.total_checks, static_cast<uint64_t>(THREADS / 2 * OPS_PER_THREAD));
    EXPECT_LE(bl.size(), static_cast<size_t>(THREADS / 2 * OPS_PER_THREAD));
}

TEST(TokenBlacklistConcurrencyTest, ConcurrentUnrevoke_ThreadSafe) {
    TokenBlacklist bl;

    // Pre-populate
    for (int i = 0; i < 100; ++i) {
        bl.revoke("jti-" + std::to_string(i), inFuture());
    }

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&bl, t]() {
            for (int i = t * 25; i < (t + 1) * 25; ++i) {
                bl.unrevoke("jti-" + std::to_string(i));
            }
        });
    }
    for (auto& t : threads) t.join();

    // All entries should have been unrevoked (none should be found)
    for (int i = 0; i < 100; ++i) {
        EXPECT_FALSE(bl.isRevoked("jti-" + std::to_string(i)));
    }
}

// ============================================================================
// Real-Time Revocation Callback Tests
// ============================================================================

TEST(TokenBlacklistCallbackTest, CallbackInvokedOnRevoke) {
    TokenBlacklist bl;
    std::string captured_jti;

    bl.setOnRevokeCallback([&captured_jti](const std::string& jti) {
        captured_jti = jti;
    });

    bl.revoke("jti-callback-test", inFuture());

    EXPECT_EQ(captured_jti, "jti-callback-test");
}

TEST(TokenBlacklistCallbackTest, CallbackNotInvokedForEmptyJti) {
    TokenBlacklist bl;
    int call_count = 0;

    bl.setOnRevokeCallback([&call_count](const std::string&) {
        ++call_count;
    });

    bl.revoke("", inFuture()); // empty JTI – should be ignored

    EXPECT_EQ(call_count, 0);
}

TEST(TokenBlacklistCallbackTest, CallbackInvokedForEachRevocation) {
    TokenBlacklist bl;
    std::vector<std::string> notified;

    bl.setOnRevokeCallback([&notified](const std::string& jti) {
        notified.push_back(jti);
    });

    bl.revoke("jti-a", inFuture());
    bl.revoke("jti-b", inFuture());
    bl.revoke("jti-c", inFuture());

    ASSERT_EQ(notified.size(), 3u);
    EXPECT_EQ(notified[0], "jti-a");
    EXPECT_EQ(notified[1], "jti-b");
    EXPECT_EQ(notified[2], "jti-c");
}

TEST(TokenBlacklistCallbackTest, ClearCallbackStopsNotifications) {
    TokenBlacklist bl;
    int call_count = 0;

    bl.setOnRevokeCallback([&call_count](const std::string&) {
        ++call_count;
    });

    bl.revoke("jti-before-clear", inFuture());
    EXPECT_EQ(call_count, 1);

    bl.clearOnRevokeCallback();
    bl.revoke("jti-after-clear", inFuture());
    EXPECT_EQ(call_count, 1); // No new call after clearing
}

TEST(TokenBlacklistCallbackTest, ReplaceCallbackUsesNewOne) {
    TokenBlacklist bl;
    std::string last_jti;

    bl.setOnRevokeCallback([&last_jti](const std::string& jti) {
        last_jti = "first:" + jti;
    });
    bl.setOnRevokeCallback([&last_jti](const std::string& jti) {
        last_jti = "second:" + jti;
    });

    bl.revoke("jti-new", inFuture());

    EXPECT_EQ(last_jti, "second:jti-new");
}

TEST(TokenBlacklistCallbackTest, CallbackCanCallIsRevokedWithoutDeadlock) {
    // The callback is invoked with the mutex released; calling back into
    // the blacklist from within the callback must not deadlock.
    TokenBlacklist bl;
    bool was_revoked = false;

    bl.setOnRevokeCallback([&bl, &was_revoked](const std::string& jti) {
        was_revoked = bl.isRevoked(jti);
    });

    bl.revoke("jti-reentrant", inFuture());

    EXPECT_TRUE(was_revoked);
}
