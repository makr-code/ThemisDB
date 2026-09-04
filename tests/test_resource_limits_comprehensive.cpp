/**
 * @file test_resource_limits_comprehensive.cpp
 * @brief Tests for resource limits in PolicyEngine, JWTKeyRotationManager, and SecretManager
 *
 * Validates issue §2 "Resource-Limits fehlend" requirements:
 *   - PolicyEngine::Config::max_policies cap
 *   - JWTKeyRotationManager::Config::max_keys cap
 *   - SecretRotationPolicy::max_secrets cap
 *   - SecretRotationPolicy::max_versions_per_secret cap
 */

#include <gtest/gtest.h>
#include "server/policy_engine.h"
#include "auth/jwt_key_rotation_manager.h"
#include "auth/jwt_validator.h"
#include "auth/token_blacklist.h"
#include "security/secret_manager.h"

#include <stdexcept>
#include <string>

using namespace themis;
using namespace themis::auth;
using namespace themis::security;

// ============================================================================
// PolicyEngine – max_policies
// ============================================================================

class PolicyEngineLimitTest : public ::testing::Test {
protected:
    static PolicyEngine::Policy makePolicy(const std::string& id) {
        PolicyEngine::Policy p;
        p.id       = id;
        p.name     = "Policy " + id;
        p.subjects = {"alice"};
        p.actions  = {"read"};
        p.resources = {"/data"};
        p.effect_allow = true;
        return p;
    }
};

TEST_F(PolicyEngineLimitTest, DefaultConfigHasNoLimit) {
    PolicyEngine engine;
    // Should accept many policies without throwing
    for (int i = 0; i < 200; ++i) {
        EXPECT_NO_THROW(engine.addPolicy(makePolicy("p" + std::to_string(i))));
    }
    EXPECT_EQ(engine.listPolicies().size(), 200u);
}

TEST_F(PolicyEngineLimitTest, MaxPoliciesZeroMeansUnlimited) {
    PolicyEngine::Config cfg;
    cfg.max_policies = 0;  // unlimited
    PolicyEngine engine(cfg);
    for (int i = 0; i < 50; ++i) {
        EXPECT_NO_THROW(engine.addPolicy(makePolicy("p" + std::to_string(i))));
    }
    EXPECT_EQ(engine.listPolicies().size(), 50u);
}

TEST_F(PolicyEngineLimitTest, MaxPoliciesEnforced) {
    PolicyEngine::Config cfg;
    cfg.max_policies = 3;
    PolicyEngine engine(cfg);

    engine.addPolicy(makePolicy("p1"));
    engine.addPolicy(makePolicy("p2"));
    engine.addPolicy(makePolicy("p3"));

    EXPECT_THROW(engine.addPolicy(makePolicy("p4")), std::length_error);
}

TEST_F(PolicyEngineLimitTest, MaxPoliciesOneIsEnforced) {
    PolicyEngine::Config cfg;
    cfg.max_policies = 1;
    PolicyEngine engine(cfg);

    engine.addPolicy(makePolicy("only"));
    EXPECT_THROW(engine.addPolicy(makePolicy("extra")), std::length_error);
}

TEST_F(PolicyEngineLimitTest, RemovingPolicyAllowsAddingAnother) {
    PolicyEngine::Config cfg;
    cfg.max_policies = 2;
    PolicyEngine engine(cfg);

    engine.addPolicy(makePolicy("p1"));
    engine.addPolicy(makePolicy("p2"));

    // At limit — adding fails
    EXPECT_THROW(engine.addPolicy(makePolicy("p3")), std::length_error);

    // Remove one → adding succeeds
    engine.removePolicy("p1");
    EXPECT_NO_THROW(engine.addPolicy(makePolicy("p3")));
    EXPECT_EQ(engine.listPolicies().size(), 2u);
}

TEST_F(PolicyEngineLimitTest, SetPoliciesIgnoresLimit) {
    // setPolicies() replaces everything; it does not enforce the limit
    // (bulk loads are intentional operations). This is intentional design.
    PolicyEngine::Config cfg;
    cfg.max_policies = 2;
    PolicyEngine engine(cfg);

    std::vector<PolicyEngine::Policy> many;
    for (int i = 0; i < 10; ++i) {
      many.push_back(makePolicy("p" + std::to_string(i)));
    }

    EXPECT_NO_THROW(engine.setPolicies(std::move(many)));
    EXPECT_EQ(engine.listPolicies().size(), 10u);
}

TEST_F(PolicyEngineLimitTest, LimitErrorMessageContainsCount) {
    PolicyEngine::Config cfg;
    cfg.max_policies = 2;
    PolicyEngine engine(cfg);
    engine.addPolicy(makePolicy("p1"));
    engine.addPolicy(makePolicy("p2"));

    try {
        engine.addPolicy(makePolicy("p3"));
        FAIL() << "Expected std::length_error";
    } catch (const std::length_error& e) {
        EXPECT_NE(std::string(e.what()).find("2"), std::string::npos)
            << "Error message should mention the limit: " << e.what();
    }
}

// ============================================================================
// JWTKeyRotationManager – max_keys
// ============================================================================

class JWTKeyRotationLimitTest : public ::testing::Test {
protected:
    JWTValidatorConfig jwt_cfg_ = []() {
        JWTValidatorConfig c;
        c.require_issuer_validation = false;
        c.require_audience_validation = false;
        return c;
    }();
    JWTValidator validator_{jwt_cfg_};
    TokenBlacklist blacklist_;
};

TEST_F(JWTKeyRotationLimitTest, DefaultConfigHasNoKeyLimit) {
    JWTKeyRotationManager::Config cfg;  // max_keys defaults to 0 (unlimited)
    JWTKeyRotationManager mgr(validator_, &blacklist_, cfg);

    for (int i = 0; i < 20; ++i) {
        EXPECT_NO_THROW(mgr.rotateActiveKey("kid-" + std::to_string(i)));
    }
}

TEST_F(JWTKeyRotationLimitTest, MaxKeysEnforced) {
    JWTKeyRotationManager::Config cfg;
    cfg.max_keys = 3;
    JWTKeyRotationManager mgr(validator_, &blacklist_, cfg);

    mgr.rotateActiveKey("kid-1");
    mgr.rotateActiveKey("kid-2");
    mgr.rotateActiveKey("kid-3");

    // Fourth distinct kid exceeds limit
    EXPECT_THROW(mgr.rotateActiveKey("kid-4"), std::length_error);
}

TEST_F(JWTKeyRotationLimitTest, ReactivatingExistingKidDoesNotExceedLimit) {
    JWTKeyRotationManager::Config cfg;
    cfg.max_keys = 2;
    JWTKeyRotationManager mgr(validator_, &blacklist_, cfg);

    mgr.rotateActiveKey("kid-A");
    mgr.rotateActiveKey("kid-B");  // kid-A is now PASSIVE, kid-B is ACTIVE — 2 keys total

    // Rotating back to kid-A (already tracked): should NOT throw even though size == 2
    EXPECT_NO_THROW(mgr.rotateActiveKey("kid-A"));
}

TEST_F(JWTKeyRotationLimitTest, LimitErrorMessageContainsCount) {
    JWTKeyRotationManager::Config cfg;
    cfg.max_keys = 1;
    JWTKeyRotationManager mgr(validator_, &blacklist_, cfg);

    mgr.rotateActiveKey("kid-1");

    try {
        mgr.rotateActiveKey("kid-2");
        FAIL() << "Expected std::length_error";
    } catch (const std::length_error& e) {
        EXPECT_NE(std::string(e.what()).find("1"), std::string::npos)
            << "Error message should mention the limit: " << e.what();
    }
}

// ============================================================================
// SecretManager – max_secrets and max_versions_per_secret
// ============================================================================

TEST(SecretManagerLimitTest, DefaultPolicyHasNoLimits) {
    SecretManager mgr;
    for (int i = 0; i < 50; ++i) {
        EXPECT_NO_THROW(mgr.storeSecret("secret-" + std::to_string(i), "val", "test"));
    }
    EXPECT_EQ(mgr.getStatistics().total_secrets, 50u);
}

TEST(SecretManagerLimitTest, MaxSecretsEnforced) {
    SecretRotationPolicy pol;
    pol.max_secrets = 3;
    SecretManager mgr(pol);

    mgr.storeSecret("s1", "v1", "test");
    mgr.storeSecret("s2", "v2", "test");
    mgr.storeSecret("s3", "v3", "test");

    EXPECT_THROW(mgr.storeSecret("s4", "v4", "test"), std::length_error);
}

TEST(SecretManagerLimitTest, MaxSecretsZeroMeansUnlimited) {
    SecretRotationPolicy pol;
    pol.max_secrets = 0;  // unlimited
    SecretManager mgr(pol);

    for (int i = 0; i < 30; ++i) {
        EXPECT_NO_THROW(mgr.storeSecret("s" + std::to_string(i), "v", "test"));
    }
}

TEST(SecretManagerLimitTest, MaxSecretsErrorMessageContainsCount) {
    SecretRotationPolicy pol;
    pol.max_secrets = 2;
    SecretManager mgr(pol);

    mgr.storeSecret("s1", "v1", "test");
    mgr.storeSecret("s2", "v2", "test");

    try {
        mgr.storeSecret("s3", "v3", "test");
        FAIL() << "Expected std::length_error";
    } catch (const std::length_error& e) {
        EXPECT_NE(std::string(e.what()).find("2"), std::string::npos)
            << "Error message should mention the limit: " << e.what();
    }
}

TEST(SecretManagerLimitTest, MaxVersionsPerSecretEnforced) {
    SecretRotationPolicy pol;
    pol.max_versions_per_secret = 2;  // 1 initial + 1 rotation
    SecretManager mgr(pol);

    mgr.storeSecret("key", "v1", "test");    // version 1 (ACTIVE) — 1 total
    mgr.rotateSecret("key", "v2", "test");   // version 2 (ACTIVE), version 1 → RETIRING — 2 total

    EXPECT_THROW(mgr.rotateSecret("key", "v3", "test"), std::length_error);
}

TEST(SecretManagerLimitTest, MaxVersionsZeroMeansUnlimited) {
    SecretRotationPolicy pol;
    pol.max_versions_per_secret = 0;  // unlimited
    SecretManager mgr(pol);

    mgr.storeSecret("key", "v1", "test");
    for (int i = 2; i <= 20; ++i) {
        EXPECT_NO_THROW(mgr.rotateSecret("key", "v" + std::to_string(i), "test"));
    }
}

TEST(SecretManagerLimitTest, MaxVersionsErrorMessageContainsName) {
    SecretRotationPolicy pol;
    pol.max_versions_per_secret = 1;
    SecretManager mgr(pol);

    mgr.storeSecret("my-api-key", "v1", "test");

    try {
        mgr.rotateSecret("my-api-key", "v2", "test");
        FAIL() << "Expected std::length_error";
    } catch (const std::length_error& e) {
        std::string msg(e.what());
        EXPECT_NE(msg.find("my-api-key"), std::string::npos)
            << "Error message should mention the secret name: " << msg;
    }
}

TEST(SecretManagerLimitTest, LimitsAreIndependent) {
    // Different secrets can each have up to max_versions independently
    SecretRotationPolicy pol;
    pol.max_secrets = 3;
    pol.max_versions_per_secret = 2;
    SecretManager mgr(pol);

    for (int s = 1; s <= 3; ++s) {
        std::string name = "secret-" + std::to_string(s);
        mgr.storeSecret(name, "v1", "test");
        EXPECT_NO_THROW(mgr.rotateSecret(name, "v2", "test"));
        EXPECT_THROW(mgr.rotateSecret(name, "v3", "test"), std::length_error);
    }

    // max_secrets limit still applies
    EXPECT_THROW(mgr.storeSecret("secret-4", "v1", "test"), std::length_error);
}
