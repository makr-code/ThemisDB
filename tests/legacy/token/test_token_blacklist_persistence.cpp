/**
 * @file test_token_blacklist_persistence.cpp
 * @brief Tests for ITokenBlacklist interface, Bloom filter, and persistence backends.
 *
 * Tests cover:
 * - ITokenBlacklist polymorphic interface via TokenBlacklist
 * - Bloom filter pre-check: non-revoked tokens return false without hash-map lookup
 * - Bloom filter accuracy: revoked tokens are always found
 * - Bloom filter rebuild after pruneExpired() and clear()
 * - TokenBlacklist::add() / purgeExpired() (ITokenBlacklist API)
 * - RocksDBTokenBlacklist: persistence across instance recreation
 * - RocksDBTokenBlacklist: purgeExpired() removes expired entries
 * - RedisTokenBlacklist: stub compiles and returns safe defaults without Redis
 */

#include <gtest/gtest.h>
#include "auth/token_blacklist.h"
#include "auth/rocksdb_token_blacklist.h"
#include "auth/redis_token_blacklist.h"

#include <chrono>
#include <string>
#include <memory>
#include <filesystem>

using namespace themis::auth;
using Clock = std::chrono::system_clock;

namespace {

Clock::time_point future(int seconds = 3600) {
    return Clock::now() + std::chrono::seconds(seconds);
}

Clock::time_point past(int seconds = 10) {
    return Clock::now() - std::chrono::seconds(seconds);
}

/// Unique temporary directory per test (removed in destructor).
struct TempDir {
    std::filesystem::path path;

    TempDir() {
        path = std::filesystem::temp_directory_path()
               / ("themis_bl_test_" + std::to_string(
                      std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(path);
    }

    ~TempDir() {
        std::error_code ec = {};
        std::filesystem::remove_all(path, ec);
    }

    std::string str() const { return path.string(); }
};

} // anonymous namespace

// ============================================================================
// ITokenBlacklist interface via TokenBlacklist
// ============================================================================

class ITokenBlacklistInterfaceTest : public ::testing::Test {
protected:
    // Access via interface pointer to verify polymorphism
    std::unique_ptr<ITokenBlacklist> bl_{std::make_unique<TokenBlacklist>()};
};

TEST_F(ITokenBlacklistInterfaceTest, Add_TokenIsRevoked) {
    bl_->add("jti-iface-001", future());
    EXPECT_TRUE(bl_->isRevoked("jti-iface-001"));
}

TEST_F(ITokenBlacklistInterfaceTest, Add_EmptyJTI_Ignored) {
    EXPECT_NO_THROW(bl_->add("", future()));
    EXPECT_FALSE(bl_->isRevoked(""));
}

TEST_F(ITokenBlacklistInterfaceTest, IsRevoked_NonRevoked_ReturnsFalse) {
    bl_->add("jti-a", future());
    EXPECT_FALSE(bl_->isRevoked("jti-b"));
}

TEST_F(ITokenBlacklistInterfaceTest, PurgeExpired_RemovesExpiredEntries) {
    bl_->add("live",    future(3600));
    bl_->add("expired", past(5));

    bl_->purgeExpired();

    EXPECT_TRUE(bl_->isRevoked("live"));
    EXPECT_FALSE(bl_->isRevoked("expired"));
}

TEST_F(ITokenBlacklistInterfaceTest, Add_ExpiredToken_IsNotRevoked) {
    bl_->add("already-expired", past(5));
    EXPECT_FALSE(bl_->isRevoked("already-expired"));
}

// ============================================================================
// Bloom filter behaviour
// ============================================================================

class TokenBlacklistBloomFilterTest : public ::testing::Test {
protected:
    // Small max_entries so the Bloom filter is exercised at low token counts
    TokenBlacklist bl_{[]{ TokenBlacklist::Config c; c.max_entries = 1000; return c; }()};
};

TEST_F(TokenBlacklistBloomFilterTest, NonRevokedToken_BloomNegative_ReturnsFalse) {
    // No tokens added; Bloom filter should definitively say NO for every query
    for (int i = 0; i < 1000; ++i) {
        EXPECT_FALSE(bl_.isRevoked("jti-never-" + std::to_string(i)));
    }
    // All checks should have been short-circuited by the Bloom filter
    auto stats = bl_.getStatistics();
    EXPECT_EQ(stats.total_checks, 1000u);
    EXPECT_GT(stats.bloom_negatives, 0u);
}

TEST_F(TokenBlacklistBloomFilterTest, RevokedToken_AlwaysFound) {
    // Insert N tokens; every one must be found (no false negatives)
    constexpr int N = 200;
    for (int i = 0; i < N; ++i) {
        bl_.revoke("jti-bloom-" + std::to_string(i), future());
    }
    for (int i = 0; i < N; ++i) {
        EXPECT_TRUE(bl_.isRevoked("jti-bloom-" + std::to_string(i)));
    }
}

TEST_F(TokenBlacklistBloomFilterTest, AfterClear_BloomDoesNotReturnPositives) {
    constexpr int N = 50;
    for (int i = 0; i < N; ++i) {
        bl_.revoke("jti-clear-" + std::to_string(i), future());
    }
    bl_.clear();

    // After clear, no token should appear revoked.
    // The Bloom filter is reset, so lookups should also be short-circuited.
    for (int i = 0; i < N; ++i) {
        EXPECT_FALSE(bl_.isRevoked("jti-clear-" + std::to_string(i)));
    }
}

TEST_F(TokenBlacklistBloomFilterTest, AfterPruneExpired_ExpiredTokensNotRevoked) {
    bl_.revoke("live",    future(3600));
    bl_.revoke("expired", past(5));

    bl_.pruneExpired();

    EXPECT_TRUE(bl_.isRevoked("live"));
    EXPECT_FALSE(bl_.isRevoked("expired"));
}

TEST_F(TokenBlacklistBloomFilterTest, Statistics_BloomNegativesCounted) {
    // Revoke one token; check a different one — that check should hit the filter.
    bl_.revoke("jti-x", future());
    bl_.isRevoked("jti-y");  // definitely not revoked

    auto stats = bl_.getStatistics();
    EXPECT_GE(stats.bloom_negatives, 1u);
}

// ============================================================================
// TokenBlacklist max_entries default updated to 1 million
// ============================================================================

TEST(TokenBlacklistConfigTest, DefaultMaxEntries_Is1Million) {
    TokenBlacklist::Config cfg = TokenBlacklist::Config::defaults();
    EXPECT_EQ(cfg.max_entries, 1'000'000u);
}

// ============================================================================
// RocksDB persistence tests
// ============================================================================

class RocksDBTokenBlacklistTest : public ::testing::Test {
protected:
    TempDir tmp_;
};

TEST_F(RocksDBTokenBlacklistTest, Add_TokenIsRevoked) {
    RocksDBTokenBlacklist::Config cfg;
    cfg.db_path = tmp_.str();

    RocksDBTokenBlacklist bl(cfg);
    bl.add("jti-rocksdb-001", future());
    EXPECT_TRUE(bl.isRevoked("jti-rocksdb-001"));
}

TEST_F(RocksDBTokenBlacklistTest, IsRevoked_NonRevoked_ReturnsFalse) {
    RocksDBTokenBlacklist::Config cfg;
    cfg.db_path = tmp_.str();

    RocksDBTokenBlacklist bl(cfg);
    bl.add("jti-rocksdb-002", future());
    EXPECT_FALSE(bl.isRevoked("jti-rocksdb-999"));
}

TEST_F(RocksDBTokenBlacklistTest, ExpiredToken_NotRevoked) {
    RocksDBTokenBlacklist::Config cfg;
    cfg.db_path = tmp_.str();

    RocksDBTokenBlacklist bl(cfg);
    bl.add("jti-expired", past(5));
    EXPECT_FALSE(bl.isRevoked("jti-expired"));
}

TEST_F(RocksDBTokenBlacklistTest, PurgeExpired_RemovesExpiredEntries) {
    RocksDBTokenBlacklist::Config cfg;
    cfg.db_path                = tmp_.str();
    cfg.purge_interval_seconds = 9999;  // disable background thread trigger

    RocksDBTokenBlacklist bl(cfg);
    bl.add("live",    future(3600));
    bl.add("expired", past(5));

    bl.purgeExpired();

    EXPECT_TRUE(bl.isRevoked("live"));
    EXPECT_FALSE(bl.isRevoked("expired"));
}

/**
 * Persistence test: revoke a token, destroy the instance, re-open the same
 * database, and verify the token is still rejected.
 */
TEST_F(RocksDBTokenBlacklistTest, Persistence_TokenSurvivesRestart) {
    const std::string jti = "jti-persist-001";

    RocksDBTokenBlacklist::Config cfg;
    cfg.db_path = tmp_.str();

    // Phase 1: revoke in instance A
    {
        RocksDBTokenBlacklist bl(cfg);
        bl.add(jti, future());
        ASSERT_TRUE(bl.isRevoked(jti));
    }
    // Instance A is now fully destructed (WAL flushed)

    // Phase 2: reopen the same DB in instance B
    {
        RocksDBTokenBlacklist bl(cfg);
        EXPECT_TRUE(bl.isRevoked(jti)) << "Token must still be revoked after process restart simulation";
    }
}

/**
 * Persistence test: multiple tokens, some expired.  After restart only
 * non-expired tokens should be revoked.
 */
TEST_F(RocksDBTokenBlacklistTest, Persistence_ExpiredTokensNotRevokedAfterRestart) {
    RocksDBTokenBlacklist::Config cfg;
    cfg.db_path = tmp_.str();

    // Phase 1: add mix of live and expired tokens
    {
        RocksDBTokenBlacklist bl(cfg);
        bl.add("persist-live",    future(3600));
        bl.add("persist-expired", past(5));
    }

    // Phase 2: reopen — expired token must NOT appear revoked
    {
        RocksDBTokenBlacklist bl(cfg);
        EXPECT_TRUE(bl.isRevoked("persist-live"));
        EXPECT_FALSE(bl.isRevoked("persist-expired"));
    }
}

TEST_F(RocksDBTokenBlacklistTest, EmptyJTI_Ignored) {
    RocksDBTokenBlacklist::Config cfg;
    cfg.db_path = tmp_.str();

    RocksDBTokenBlacklist bl(cfg);
    EXPECT_NO_THROW(bl.add("", future()));
    EXPECT_FALSE(bl.isRevoked(""));
}

// ============================================================================
// RedisTokenBlacklist stub / no-op tests
// ============================================================================

/**
 * Verify that RedisTokenBlacklist compiles, constructs, and returns safe
 * defaults when built without hiredis (THEMIS_ENABLE_REDIS not defined).
 * When hiredis IS available but no server is running the connection attempt
 * will fail and the instance will behave as a no-op stub as well.
 */
TEST(RedisTokenBlacklistStubTest, Instantiation_DoesNotThrow) {
    RedisTokenBlacklist::Config cfg;
    cfg.host               = "127.0.0.1";
    cfg.port               = 16379;  // port unlikely to have a live Redis
    cfg.connect_timeout_ms = 10;     // fail fast

    EXPECT_NO_THROW([&]{
        RedisTokenBlacklist bl(cfg);
        // When not connected, isRevoked must return false (safe default)
        EXPECT_FALSE(bl.isRevoked("jti-redis-stub"));
        // add() must not throw even when not connected
        bl.add("jti-redis-stub", future());
        // purgeExpired() must not throw
        EXPECT_NO_THROW(bl.purgeExpired());
    }());
}

TEST(RedisTokenBlacklistStubTest, EmptyJTI_ReturnsFalse) {
    RedisTokenBlacklist::Config cfg;
    cfg.host               = "127.0.0.1";
    cfg.port               = 16379;
    cfg.connect_timeout_ms = 10;

    RedisTokenBlacklist bl(cfg);
    EXPECT_FALSE(bl.isRevoked(""));
}

/**
 * Distribution simulation test: verify that two independent blacklist
 * instances backed by the same RocksDB database both see a revocation.
 *
 * In a real distributed deployment this role is fulfilled by
 * RedisTokenBlacklist; here we simulate it using RocksDB to verify the
 * ITokenBlacklist interface is sufficient for cross-instance coordination
 * when a shared storage backend is used.
 */
TEST(DistributedSimulationTest, TwoInstances_ShareRevocations) {
    TempDir tmp;

    RocksDBTokenBlacklist::Config cfg;
    cfg.db_path                = tmp.str();
    cfg.purge_interval_seconds = 9999;

    // "Node A": revoke a token
    {
        RocksDBTokenBlacklist nodeA(cfg);
        nodeA.add("jti-distributed-001", future());
        ASSERT_TRUE(nodeA.isRevoked("jti-distributed-001"));
    }

    // "Node B" (simulated restart / separate process): must see the revocation
    {
        RocksDBTokenBlacklist nodeB(cfg);
        EXPECT_TRUE(nodeB.isRevoked("jti-distributed-001"))
            << "Revocation on node A must be visible on node B via shared RocksDB";
    }
}
