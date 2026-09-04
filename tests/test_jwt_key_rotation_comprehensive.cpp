/**
 * @file test_jwt_key_rotation_comprehensive.cpp
 * @brief Comprehensive tests for JWTKeyRotationManager
 *
 * Tests cover:
 * - rotateActiveKey: old key demoted to PASSIVE, new key becomes ACTIVE
 * - revokeKey: key added to JWTValidator denylist
 * - reactivateKey: PASSIVE key re-promoted to ACTIVE
 * - isRotationDue: returns true when no active key or key expired
 * - checkAndRotate: auto-revokes expired PASSIVE keys
 * - Statistics tracking
 * - Thread safety
 */

#include <gtest/gtest.h>
#include "auth/jwt_key_rotation_manager.h"
#include "auth/jwt_validator.h"
#include "auth/token_blacklist.h"
#include <chrono>
#include <thread>
#include <atomic>

using namespace themis::auth;

// ============================================================================
// Fixture
// ============================================================================

class KeyRotationTest : public ::testing::Test {
protected:
    void SetUp() override {
        JWTValidatorConfig cfg;
        cfg.jwks_url = "http://localhost:8080/jwks"; // Not called in tests
        cfg.require_issuer_validation = false;
        cfg.require_audience_validation = false;
        validator_ = std::make_unique<JWTValidator>(cfg);
        mgr_ = std::make_unique<JWTKeyRotationManager>(*validator_);
    }

    std::unique_ptr<JWTValidator>          validator_;
    std::unique_ptr<JWTKeyRotationManager> mgr_;
};

// ============================================================================
// Initial state
// ============================================================================

TEST_F(KeyRotationTest, InitialState_NoActiveKey) {
    EXPECT_TRUE(mgr_->activeKeyId().empty());
    EXPECT_TRUE(mgr_->passiveKeyIds().empty());
    EXPECT_TRUE(mgr_->revokedKeyIds().empty());
}

TEST_F(KeyRotationTest, InitialState_RotationIsDue) {
    // No active key registered → rotation is always "due"
    EXPECT_TRUE(mgr_->isRotationDue());
}

// ============================================================================
// rotateActiveKey
// ============================================================================

TEST_F(KeyRotationTest, RotateActiveKey_BecomesActive) {
    mgr_->rotateActiveKey("key-v1");
    EXPECT_EQ(mgr_->activeKeyId(), "key-v1");
}

TEST_F(KeyRotationTest, RotateActiveKey_OldKeyBecomesPassive) {
    mgr_->rotateActiveKey("key-v1");
    ASSERT_EQ(mgr_->activeKeyId(), "key-v1");

    mgr_->rotateActiveKey("key-v2");
    EXPECT_EQ(mgr_->activeKeyId(), "key-v2");

    auto passive = mgr_->passiveKeyIds();
    ASSERT_EQ(passive.size(), 1u);
    EXPECT_EQ(passive[0], "key-v1");
}

TEST_F(KeyRotationTest, RotateActiveKey_MultipleRotations) {
    mgr_->rotateActiveKey("key-v1");
    mgr_->rotateActiveKey("key-v2");
    mgr_->rotateActiveKey("key-v3");

    EXPECT_EQ(mgr_->activeKeyId(), "key-v3");
    EXPECT_EQ(mgr_->passiveKeyIds().size(), 2u);
}

TEST_F(KeyRotationTest, RotateActiveKey_RotationCountIncremented) {
    mgr_->rotateActiveKey("key-v1");
    mgr_->rotateActiveKey("key-v2");
    auto stats = mgr_->getStatistics();
    EXPECT_EQ(stats.total_rotations, 2u);
}

TEST_F(KeyRotationTest, RotateActiveKey_CustomMaxAge) {
    mgr_->rotateActiveKey("key-v1", std::chrono::seconds(3600));
    auto info = mgr_->getKeyInfo("key-v1");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->max_age, std::chrono::seconds(3600));
}

// ============================================================================
// revokeKey
// ============================================================================

TEST_F(KeyRotationTest, RevokeKey_ActiveKey_RemovedFromActive) {
    mgr_->rotateActiveKey("key-v1");
    ASSERT_EQ(mgr_->activeKeyId(), "key-v1");

    mgr_->revokeKey("key-v1");

    EXPECT_TRUE(mgr_->activeKeyId().empty());
    EXPECT_EQ(mgr_->revokedKeyIds().size(), 1u);
    EXPECT_EQ(mgr_->revokedKeyIds()[0], "key-v1");
}

TEST_F(KeyRotationTest, RevokeKey_PassiveKey_Revoked) {
    mgr_->rotateActiveKey("key-v1");
    mgr_->rotateActiveKey("key-v2");  // key-v1 is now passive
    mgr_->revokeKey("key-v1");

    EXPECT_TRUE(mgr_->passiveKeyIds().empty());
    EXPECT_EQ(mgr_->revokedKeyIds().size(), 1u);
}

TEST_F(KeyRotationTest, RevokeKey_AddsToValidatorDenylist) {
    mgr_->rotateActiveKey("revoke-me");
    ASSERT_FALSE(validator_->isKidRevoked("revoke-me"));

    mgr_->revokeKey("revoke-me");
    EXPECT_TRUE(validator_->isKidRevoked("revoke-me"));
}

TEST_F(KeyRotationTest, RevokeKey_UnknownKey_ReturnsFalse) {
    EXPECT_FALSE(mgr_->revokeKey("nonexistent-key"));
}

TEST_F(KeyRotationTest, RevokeKey_AlreadyRevoked_ReturnsTrueNoOp) {
    mgr_->rotateActiveKey("key-v1");
    mgr_->revokeKey("key-v1");
    EXPECT_TRUE(mgr_->revokeKey("key-v1"));  // Should not crash
}

TEST_F(KeyRotationTest, RevokeKey_RevocationCountIncremented) {
    mgr_->rotateActiveKey("key-v1");
    mgr_->revokeKey("key-v1");
    auto stats = mgr_->getStatistics();
    EXPECT_EQ(stats.total_revocations, 1u);
}

// ============================================================================
// reactivateKey
// ============================================================================

TEST_F(KeyRotationTest, ReactivateKey_PassiveKey_BecomesActive) {
    mgr_->rotateActiveKey("key-v1");
    mgr_->rotateActiveKey("key-v2");  // key-v1 is now passive
    ASSERT_EQ(mgr_->activeKeyId(), "key-v2");

    EXPECT_TRUE(mgr_->reactivateKey("key-v1"));
    EXPECT_EQ(mgr_->activeKeyId(), "key-v1");
}

TEST_F(KeyRotationTest, ReactivateKey_PreviousActiveBecomesPassive) {
    mgr_->rotateActiveKey("key-v1");
    mgr_->rotateActiveKey("key-v2");
    mgr_->reactivateKey("key-v1");

    auto passive = mgr_->passiveKeyIds();
    EXPECT_EQ(passive.size(), 1u);
    EXPECT_EQ(passive[0], "key-v2");
}

TEST_F(KeyRotationTest, ReactivateKey_RevokedKey_ReturnsFalse) {
    mgr_->rotateActiveKey("key-v1");
    mgr_->revokeKey("key-v1");
    EXPECT_FALSE(mgr_->reactivateKey("key-v1"));
}

TEST_F(KeyRotationTest, ReactivateKey_UnknownKey_ReturnsFalse) {
    EXPECT_FALSE(mgr_->reactivateKey("ghost-key"));
}

// ============================================================================
// isRotationDue
// ============================================================================

TEST_F(KeyRotationTest, IsRotationDue_FreshKey_NotDue) {
    mgr_->rotateActiveKey("fresh-key");
    EXPECT_FALSE(mgr_->isRotationDue());
}

TEST_F(KeyRotationTest, IsRotationDue_ExpiredAge_IsDue) {
    // Register key with very short max_age (1 second)
    mgr_->rotateActiveKey("expiring-key", std::chrono::seconds(0));
    // max_age=0 means key has already exceeded its lifetime
    EXPECT_TRUE(mgr_->isRotationDue());
}

// ============================================================================
// getKeyInfo
// ============================================================================

TEST_F(KeyRotationTest, GetKeyInfo_ExistingKey_Returns) {
    mgr_->rotateActiveKey("key-info-test");
    auto info = mgr_->getKeyInfo("key-info-test");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->kid, "key-info-test");
    EXPECT_EQ(info->status, JWKKeyInfo::Status::ACTIVE);
}

TEST_F(KeyRotationTest, GetKeyInfo_NonExistentKey_Nullopt) {
    EXPECT_FALSE(mgr_->getKeyInfo("ghost").has_value());
}

// ============================================================================
// Statistics
// ============================================================================

TEST_F(KeyRotationTest, Statistics_Counts) {
    mgr_->rotateActiveKey("k1");
    mgr_->rotateActiveKey("k2");
    mgr_->revokeKey("k1");

    auto s = mgr_->getStatistics();
    EXPECT_EQ(s.total_keys,        2u);
    EXPECT_EQ(s.active_keys,       1u);
    EXPECT_EQ(s.passive_keys,      0u);
    EXPECT_EQ(s.revoked_keys,      1u);
    EXPECT_EQ(s.total_rotations,   2u);
    EXPECT_EQ(s.total_revocations, 1u);
}

// ============================================================================
// Auto-revoke via checkAndRotate
// ============================================================================

TEST(KeyRotationAutoRevokeTest, CheckAndRotate_ExpiredPassive_AutoRevoked) {
    JWTValidatorConfig cfg;
    cfg.jwks_url = "http://localhost:8080/jwks";
    cfg.require_issuer_validation = false;
    cfg.require_audience_validation = false;
    JWTValidator validator(cfg);

    JWTKeyRotationManager::Config mgr_cfg;
    mgr_cfg.passive_grace_period    = std::chrono::seconds(0); // Immediate expiry
    mgr_cfg.auto_revoke_expired_passive = true;
    JWTKeyRotationManager mgr(validator, nullptr, mgr_cfg);

    mgr.rotateActiveKey("old-key");
    mgr.rotateActiveKey("new-key");  // old-key is now PASSIVE

    ASSERT_EQ(mgr.passiveKeyIds().size(), 1u);

    // Wait a moment so the passive key's age > 0 seconds
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    mgr.checkAndRotate();

    // Old key should now be REVOKED
    EXPECT_TRUE(mgr.passiveKeyIds().empty());
    EXPECT_EQ(mgr.revokedKeyIds().size(), 1u);
    EXPECT_TRUE(validator.isKidRevoked("old-key"));
}

// ============================================================================
// Thread safety
// ============================================================================

TEST(KeyRotationThreadSafetyTest, ConcurrentRotations_NoDataRace) {
    JWTValidatorConfig cfg;
    cfg.jwks_url = "http://localhost:8080/jwks";
    cfg.require_issuer_validation = false;
    cfg.require_audience_validation = false;
    JWTValidator validator(cfg);
    JWTKeyRotationManager mgr(validator);

    std::atomic<int> errors{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&mgr, &errors, i]() {
            try {
                for (int j = 0; j < 10; ++j) {
                    mgr.rotateActiveKey("key-" + std::to_string(i * 100 + j));
                    (void)mgr.activeKeyId();
                    (void)mgr.isRotationDue();
                }
            } catch (...) {
                errors++;
            }
        });
    }

    for (auto& t : threads) {
      t.join();
    }
    EXPECT_EQ(errors.load(), 0);
}
