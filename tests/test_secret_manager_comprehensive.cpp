/**
 * @file test_secret_manager_comprehensive.cpp
 * @brief Comprehensive tests for the SecretManager class.
 *
 * Tests cover: store, get, rotate, revoke, delete, listVersions,
 * listSecrets, isRotationDue, checkAndRevoke, statistics, thread-safety,
 * and edge cases.
 */

#include <gtest/gtest.h>
#include "security/secret_manager.h"

#include <thread>
#include <vector>
#include <chrono>

using namespace themis::security;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: short grace period for housekeeping tests
// ─────────────────────────────────────────────────────────────────────────────
static SecretManager::RotationPolicy shortGracePolicy() {
    SecretManager::RotationPolicy p;
    p.max_age                      = std::chrono::seconds(0);
    p.retiring_grace_period        = std::chrono::seconds(0);
    p.auto_revoke_expired_retiring = true;
    return p;
}

// ─────────────────────────────────────────────────────────────────────────────
// storeSecret
// ─────────────────────────────────────────────────────────────────────────────

TEST(SecretManager, StoreReturnsVersionOne) {
    SecretManager sm;
    auto ver = sm.storeSecret("api-key", "secret-value");
    EXPECT_EQ(ver, 1u);
}

TEST(SecretManager, StoredSecretIsActiveByDefault) {
    SecretManager sm;
    sm.storeSecret("db-password", "hunter2");
    auto secret = sm.getSecret("db-password");
    ASSERT_TRUE(secret.has_value());
    EXPECT_EQ(secret->status, SecretManager::SecretStatus::ACTIVE);
}

TEST(SecretManager, StoredSecretValueMatches) {
    SecretManager sm;
    sm.storeSecret("token", "my-token-value");
    auto secret = sm.getSecret("token");
    ASSERT_TRUE(secret.has_value());
    EXPECT_EQ(secret->value, "my-token-value");
}

TEST(SecretManager, StoreWithMetadata) {
    SecretManager sm;
    sm.storeSecret("key1", "val1", "alice", "initial key");
    auto v = sm.getSecretVersion("key1", 1);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->created_by, "alice");
    EXPECT_EQ(v->description, "initial key");
}

TEST(SecretManager, DuplicateStoreThrows) {
    SecretManager sm;
    sm.storeSecret("key", "value1");
    EXPECT_THROW(sm.storeSecret("key", "value2"), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// getSecret / getSecretVersion
// ─────────────────────────────────────────────────────────────────────────────

TEST(SecretManager, GetNonExistentReturnsNullopt) {
    SecretManager sm;
    EXPECT_FALSE(sm.getSecret("does-not-exist").has_value());
}

TEST(SecretManager, GetVersionNonExistentReturnsNullopt) {
    SecretManager sm;
    EXPECT_FALSE(sm.getSecretVersion("does-not-exist", 1).has_value());
}

TEST(SecretManager, GetVersionUnknownVersionReturnsNullopt) {
    SecretManager sm;
    sm.storeSecret("key", "val");
    EXPECT_FALSE(sm.getSecretVersion("key", 99).has_value());
}

TEST(SecretManager, GetSecretAfterRotateReturnsLatest) {
    SecretManager sm;
    sm.storeSecret("api", "v1");
    sm.rotateSecret("api", "v2");
    auto s = sm.getSecret("api");
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(s->value, "v2");
    EXPECT_EQ(s->version, 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// rotateSecret
// ─────────────────────────────────────────────────────────────────────────────

TEST(SecretManager, RotateIncreasesVersionNumber) {
    SecretManager sm;
    sm.storeSecret("k", "v1");
    auto v2 = sm.rotateSecret("k", "v2");
    auto v3 = sm.rotateSecret("k", "v3");
    EXPECT_EQ(v2, 2u);
    EXPECT_EQ(v3, 3u);
}

TEST(SecretManager, RotateDemotesPreviousToRetiring) {
    SecretManager sm;
    sm.storeSecret("k", "v1");
    sm.rotateSecret("k", "v2");

    auto v1 = sm.getSecretVersion("k", 1);
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->status, SecretManager::SecretStatus::RETIRING);
}

TEST(SecretManager, OldRetiringVersionIsStillReadable) {
    SecretManager sm;
    sm.storeSecret("k", "old-value");
    sm.rotateSecret("k", "new-value");

    auto old = sm.getSecretVersion("k", 1);
    ASSERT_TRUE(old.has_value());
    EXPECT_EQ(old->value, "old-value");
    EXPECT_EQ(old->status, SecretManager::SecretStatus::RETIRING);
}

TEST(SecretManager, RotateNonExistentThrows) {
    SecretManager sm;
    EXPECT_THROW(sm.rotateSecret("missing", "val"), std::invalid_argument);
}

TEST(SecretManager, MultipleRotationsKeepHistory) {
    SecretManager sm;
    sm.storeSecret("k", "v1");
    sm.rotateSecret("k", "v2");
    sm.rotateSecret("k", "v3");

    auto versions = sm.listVersions("k");
    ASSERT_EQ(versions.size(), 3u);
    EXPECT_EQ(versions[0].version, 1u);
    EXPECT_EQ(versions[1].version, 2u);
    EXPECT_EQ(versions[2].version, 3u);
    EXPECT_EQ(versions[2].status, SecretManager::SecretStatus::ACTIVE);
}

// ─────────────────────────────────────────────────────────────────────────────
// revokeVersion
// ─────────────────────────────────────────────────────────────────────────────

TEST(SecretManager, RevokeKnownVersion) {
    SecretManager sm;
    sm.storeSecret("k", "v1");
    sm.rotateSecret("k", "v2");
    EXPECT_TRUE(sm.revokeVersion("k", 1));

    auto v1 = sm.getSecretVersion("k", 1);
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->status, SecretManager::SecretStatus::REVOKED);
}

TEST(SecretManager, RevokeUnknownSecretReturnsFalse) {
    SecretManager sm;
    EXPECT_FALSE(sm.revokeVersion("missing", 1));
}

TEST(SecretManager, RevokeUnknownVersionReturnsFalse) {
    SecretManager sm;
    sm.storeSecret("k", "v1");
    EXPECT_FALSE(sm.revokeVersion("k", 99));
}

TEST(SecretManager, RevokeAlreadyRevokedReturnsFalse) {
    SecretManager sm;
    sm.storeSecret("k", "v1");
    sm.rotateSecret("k", "v2");
    sm.revokeVersion("k", 1);
    EXPECT_FALSE(sm.revokeVersion("k", 1));  // already revoked
}

TEST(SecretManager, GetSecretSkipsRevokedVersion) {
    SecretManager sm;
    sm.storeSecret("k", "v1");
    sm.revokeVersion("k", 1);
    // No active version remains
    EXPECT_FALSE(sm.getSecret("k").has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// deleteSecret
// ─────────────────────────────────────────────────────────────────────────────

TEST(SecretManager, DeleteExistingSecret) {
    SecretManager sm;
    sm.storeSecret("k", "v");
    EXPECT_TRUE(sm.deleteSecret("k"));
    EXPECT_FALSE(sm.getSecret("k").has_value());
}

TEST(SecretManager, DeleteNonExistentReturnsFalse) {
    SecretManager sm;
    EXPECT_FALSE(sm.deleteSecret("missing"));
}

TEST(SecretManager, DeletedSecretCanBeReCreated) {
    SecretManager sm;
    sm.storeSecret("k", "v1");
    sm.deleteSecret("k");
    EXPECT_NO_THROW(sm.storeSecret("k", "v2"));
    EXPECT_EQ(sm.getSecret("k")->version, 1u);  // fresh start
}

// ─────────────────────────────────────────────────────────────────────────────
// listVersions / listSecrets
// ─────────────────────────────────────────────────────────────────────────────

TEST(SecretManager, ListVersionsEmpty) {
    SecretManager sm;
    EXPECT_TRUE(sm.listVersions("missing").empty());
}

TEST(SecretManager, ListVersionsAfterRotation) {
    SecretManager sm;
    sm.storeSecret("k", "v1");
    sm.rotateSecret("k", "v2");
    auto v = sm.listVersions("k");
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0].status, SecretManager::SecretStatus::RETIRING);
    EXPECT_EQ(v[1].status, SecretManager::SecretStatus::ACTIVE);
}

TEST(SecretManager, ListSecretsEmpty) {
    SecretManager sm;
    EXPECT_TRUE(sm.listSecrets().empty());
}

TEST(SecretManager, ListSecretsReturnsAllNames) {
    SecretManager sm;
    sm.storeSecret("a", "1");
    sm.storeSecret("b", "2");
    sm.storeSecret("c", "3");
    auto names = sm.listSecrets();
    ASSERT_EQ(names.size(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// isRotationDue
// ─────────────────────────────────────────────────────────────────────────────

TEST(SecretManager, IsRotationDueFalseForNewSecret) {
    SecretManager sm;  // default 90-day max_age
    sm.storeSecret("k", "v");
    EXPECT_FALSE(sm.isRotationDue("k"));
}

TEST(SecretManager, IsRotationDueTrueAfterMaxAge) {
    SecretManager sm(shortGracePolicy());
    sm.storeSecret("k", "v");
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    EXPECT_TRUE(sm.isRotationDue("k"));
}

TEST(SecretManager, IsRotationDueFalseForMissingSecret) {
    SecretManager sm;
    EXPECT_FALSE(sm.isRotationDue("missing"));
}

// ─────────────────────────────────────────────────────────────────────────────
// checkAndRevoke
// ─────────────────────────────────────────────────────────────────────────────

TEST(SecretManager, CheckAndRevokeAutoRevokesExpiredRetiring) {
    SecretManager sm(shortGracePolicy());
    sm.storeSecret("k", "v1");
    sm.rotateSecret("k", "v2");  // v1 becomes RETIRING

    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    sm.checkAndRevoke();

    auto v1 = sm.getSecretVersion("k", 1);
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->status, SecretManager::SecretStatus::REVOKED);
}

TEST(SecretManager, CheckAndRevokeNoOpWhenPolicyDisabled) {
    SecretManager::RotationPolicy policy;
    policy.auto_revoke_expired_retiring = false;
    policy.retiring_grace_period        = std::chrono::seconds(0);
    SecretManager sm(policy);
    sm.storeSecret("k", "v1");
    sm.rotateSecret("k", "v2");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sm.checkAndRevoke();  // should not revoke

    auto v1 = sm.getSecretVersion("k", 1);
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->status, SecretManager::SecretStatus::RETIRING);  // unchanged
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics
// ─────────────────────────────────────────────────────────────────────────────

TEST(SecretManager, StatisticsInitiallyZero) {
    SecretManager sm;
    auto s = sm.getStatistics();
    EXPECT_EQ(s.total_secrets, 0u);
    EXPECT_EQ(s.active_versions, 0u);
    EXPECT_EQ(s.total_rotations, 0u);
}

TEST(SecretManager, StatisticsAfterStoreAndRotate) {
    SecretManager sm;
    sm.storeSecret("k1", "v1");
    sm.storeSecret("k2", "v2");
    sm.rotateSecret("k1", "v1-new");  // 1 rotation

    auto s = sm.getStatistics();
    EXPECT_EQ(s.total_secrets, 2u);
    EXPECT_EQ(s.active_versions, 2u);
    EXPECT_EQ(s.retiring_versions, 1u);
    EXPECT_EQ(s.total_rotations, 1u);
}

TEST(SecretManager, StatisticsAfterRevoke) {
    SecretManager sm;
    sm.storeSecret("k", "v1");
    sm.rotateSecret("k", "v2");
    sm.revokeVersion("k", 1);

    auto s = sm.getStatistics();
    EXPECT_EQ(s.active_versions, 1u);
    EXPECT_EQ(s.retiring_versions, 0u);
    EXPECT_EQ(s.revoked_versions, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Thread-safety
// ─────────────────────────────────────────────────────────────────────────────

TEST(SecretManager, ConcurrentStoreAndRotate) {
    SecretManager sm;
    sm.storeSecret("shared", "v0");

    constexpr int THREAD_COUNT = 8;
    std::vector<std::thread> threads;
    threads.reserve(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&sm, i] {
            std::string val = "v" + std::to_string(i + 1);
            try {
                sm.rotateSecret("shared", val, "thread-" + std::to_string(i));
            } catch (...) { /* might fail if racing */ }
        });
    }
    for (auto& t : threads) t.join();

    // After all rotations there must be exactly one ACTIVE version
    auto secret = sm.getSecret("shared");
    EXPECT_TRUE(secret.has_value());
    EXPECT_EQ(secret->status, SecretManager::SecretStatus::ACTIVE);
}

TEST(SecretManager, ConcurrentReadsAreSafe) {
    SecretManager sm;
    sm.storeSecret("k", "value");

    constexpr int READER_COUNT = 16;
    std::vector<std::thread> threads;
    threads.reserve(READER_COUNT);
    for (int i = 0; i < READER_COUNT; ++i) {
        threads.emplace_back([&sm] {
            for (int j = 0; j < 100; ++j) {
                auto s = sm.getSecret("k");
                ASSERT_TRUE(s.has_value());
            }
        });
    }
    for (auto& t : threads) t.join();
}
