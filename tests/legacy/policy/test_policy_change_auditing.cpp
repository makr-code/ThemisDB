/**
 * @file test_policy_change_auditing.cpp
 * @brief Tests that PolicyEngine emits audit events when policies are changed
 *
 * Issue §1.3: "Policy- & Config-Change-Auditing"
 *
 * Tests cover:
 *  - addPolicy() emits a POLICY_UPDATED audit event with action="add"
 *  - removePolicy() emits a POLICY_UPDATED audit event with action="remove"
 *  - removePolicy() for non-existent id emits nothing
 *  - setPolicies() emits a POLICY_UPDATED event with action="set_all"
 *  - No audit event when audit_logger is nullptr (default)
 *  - setAuditLogger(nullptr) detaches the logger
 *  - JWTKeyRotationManager emits KEY_ROTATED on rotateActiveKey()
 *  - JWTKeyRotationManager emits KEY_DELETED on revokeKey()
 *  - No audit event from JWTKeyRotationManager when no logger attached
 */

#include <gtest/gtest.h>
#include "server/policy_engine.h"
#include "utils/audit_logger.h"
#include "auth/jwt_key_rotation_manager.h"
#include "auth/jwt_validator.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace themis;
using namespace themis::utils;
using namespace themis::auth;

// ============================================================================
// Helper: create a minimal AuditLogger that writes to a temp file
// ============================================================================

namespace {

AuditLoggerConfig makeAuditConfig(const std::string& log_path) {
    AuditLoggerConfig cfg;
    cfg.enabled             = true;
    cfg.encrypt_then_sign   = false;
    cfg.log_path            = log_path;
    cfg.key_id              = "test-key";
    cfg.enable_hash_chain   = false;
    cfg.enable_siem         = false;
    return cfg;
}

// Count log lines containing a specific JSON substring
size_t countMatching(const std::string& path, const std::string& substr) {
    std::ifstream f(path);
    size_t count = 0;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.find(substr) != std::string::npos) {
          ++count;
        }
    }
    return count;
}

PolicyEngine::Policy makePolicy(const std::string& id) {
    PolicyEngine::Policy p;
    p.id      = id;
    p.name    = "Policy " + id;
    p.effect_allow = true;
    p.subjects.insert("alice");
    p.actions.insert("read");
    p.resources.push_back("/data/" + id);
    return p;
}

} // anonymous namespace

// ============================================================================
// PolicyEngine audit tests
// ============================================================================

class PolicyChangeAuditTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_ = std::filesystem::temp_directory_path() / "policy_audit_test";
        std::filesystem::create_directories(tmp_);
        log_path_ = (tmp_ / "audit.jsonl").string();
        logger_ = std::make_unique<AuditLogger>(nullptr, nullptr,
                                                 makeAuditConfig(log_path_));
    }

    void TearDown() override {
        std::filesystem::remove_all(tmp_);
    }

    std::filesystem::path tmp_;
    std::string log_path_;
    std::unique_ptr<AuditLogger> logger_;
};

TEST_F(PolicyChangeAuditTest, AddPolicy_EmitsAuditEvent) {
    PolicyEngine engine;
    engine.setAuditLogger(logger_.get());

    engine.addPolicy(makePolicy("p1"));

    EXPECT_GE(countMatching(log_path_, "POLICY_UPDATED"), 1u)
        << "Expected a POLICY_UPDATED event after addPolicy()";
}

TEST_F(PolicyChangeAuditTest, AddPolicy_AuditContainsPolicyId) {
    PolicyEngine engine;
    engine.setAuditLogger(logger_.get());

    engine.addPolicy(makePolicy("my-policy-123"));

    EXPECT_GE(countMatching(log_path_, "my-policy-123"), 1u)
        << "Audit event should contain the policy id";
}

TEST_F(PolicyChangeAuditTest, AddPolicy_AuditContainsAddAction) {
    PolicyEngine engine;
    engine.setAuditLogger(logger_.get());

    engine.addPolicy(makePolicy("p2"));

    EXPECT_GE(countMatching(log_path_, "\"add\""), 1u)
        << "Audit event action should be 'add'";
}

TEST_F(PolicyChangeAuditTest, RemovePolicy_Existing_EmitsAuditEvent) {
    PolicyEngine engine;
    engine.setAuditLogger(logger_.get());
    engine.addPolicy(makePolicy("p3"));

    // Clear file to isolate remove event
    std::ofstream(log_path_, std::ios::trunc).close();

    engine.removePolicy("p3");

    EXPECT_GE(countMatching(log_path_, "POLICY_UPDATED"), 1u)
        << "Expected a POLICY_UPDATED event after removePolicy()";
    EXPECT_GE(countMatching(log_path_, "\"remove\""), 1u);
}

TEST_F(PolicyChangeAuditTest, RemovePolicy_NonExistent_NoAuditEvent) {
    PolicyEngine engine;
    engine.setAuditLogger(logger_.get());

    engine.removePolicy("does-not-exist");

    // No event should be emitted since nothing was removed
    EXPECT_EQ(countMatching(log_path_, "POLICY_UPDATED"), 0u)
        << "No audit event expected for removing a non-existent policy";
}

TEST_F(PolicyChangeAuditTest, SetPolicies_EmitsAuditEvent) {
    PolicyEngine engine;
    engine.setAuditLogger(logger_.get());

    std::vector<PolicyEngine::Policy> bulk = {
        makePolicy("bulk-1"), makePolicy("bulk-2"), makePolicy("bulk-3")
    };
    engine.setPolicies(bulk);

    EXPECT_GE(countMatching(log_path_, "POLICY_UPDATED"), 1u);
    EXPECT_GE(countMatching(log_path_, "set_all"), 1u);
}

TEST_F(PolicyChangeAuditTest, NoLogger_AddPolicy_NoFileCreated) {
    PolicyEngine engine;
    // Intentionally do NOT call setAuditLogger()

    EXPECT_NO_THROW(engine.addPolicy(makePolicy("silent")));
    EXPECT_NO_THROW(engine.removePolicy("silent"));
    EXPECT_NO_THROW(engine.setPolicies({}));
    // No audit file created (logger is null)
    EXPECT_FALSE(std::filesystem::exists(log_path_));
}

TEST_F(PolicyChangeAuditTest, DetachLogger_AfterDetach_NoNewEvents) {
    PolicyEngine engine;
    engine.setAuditLogger(logger_.get());
    engine.addPolicy(makePolicy("before"));

    size_t events_before = countMatching(log_path_, "POLICY_UPDATED");
    ASSERT_GE(events_before, 1u);

    // Detach
    engine.setAuditLogger(nullptr);

    // More changes should NOT produce new audit events
    engine.addPolicy(makePolicy("after"));
    engine.removePolicy("before");

    size_t events_after = countMatching(log_path_, "POLICY_UPDATED");
    EXPECT_EQ(events_before, events_after)
        << "No new audit events expected after detaching logger";
}

TEST_F(PolicyChangeAuditTest, MultipleAdds_EachEmitsEvent) {
    PolicyEngine engine;
    engine.setAuditLogger(logger_.get());

    for (int i = 0; i < 5; ++i) {
        engine.addPolicy(makePolicy("p-" + std::to_string(i)));
    }

    EXPECT_GE(countMatching(log_path_, "POLICY_UPDATED"), 5u);
}

// ============================================================================
// JWTKeyRotationManager audit tests
// ============================================================================

class KeyRotationAuditTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_ = std::filesystem::temp_directory_path() / "rotation_audit_test";
        std::filesystem::create_directories(tmp_);
        log_path_ = (tmp_ / "rotation_audit.jsonl").string();

        JWTValidatorConfig cfg;
        cfg.jwks_url = "http://localhost:8080/jwks";
        cfg.require_issuer_validation = false;
        cfg.require_audience_validation = false;
        validator_ = std::make_unique<JWTValidator>(cfg);
        logger_    = std::make_unique<AuditLogger>(nullptr, nullptr,
                                                    makeAuditConfig(log_path_));
        mgr_ = std::make_unique<JWTKeyRotationManager>(*validator_);
    }

    void TearDown() override {
        std::filesystem::remove_all(tmp_);
    }

    std::filesystem::path tmp_;
    std::string log_path_;
    std::unique_ptr<JWTValidator>          validator_;
    std::unique_ptr<AuditLogger>           logger_;
    std::unique_ptr<JWTKeyRotationManager> mgr_;
};

TEST_F(KeyRotationAuditTest, RotateActiveKey_EmitsKeyRotatedEvent) {
    mgr_->setAuditLogger(logger_.get());
    mgr_->rotateActiveKey("key-v1");

    EXPECT_GE(countMatching(log_path_, "KEY_ROTATED"), 1u)
        << "Expected KEY_ROTATED audit event after rotateActiveKey()";
    EXPECT_GE(countMatching(log_path_, "key-v1"), 1u)
        << "Audit event should contain the new kid";
}

TEST_F(KeyRotationAuditTest, RevokeKey_EmitsKeyDeletedEvent) {
    mgr_->setAuditLogger(logger_.get());
    mgr_->rotateActiveKey("key-v1");

    // Clear log to isolate the revoke event
    std::ofstream(log_path_, std::ios::trunc).close();

    mgr_->revokeKey("key-v1");

    EXPECT_GE(countMatching(log_path_, "KEY_DELETED"), 1u)
        << "Expected KEY_DELETED audit event after revokeKey()";
    EXPECT_GE(countMatching(log_path_, "key-v1"), 1u);
}

TEST_F(KeyRotationAuditTest, RevokeUnknownKey_NoAuditEvent) {
    mgr_->setAuditLogger(logger_.get());

    bool result = mgr_->revokeKey("nonexistent-key");
    EXPECT_FALSE(result);

    EXPECT_EQ(countMatching(log_path_, "KEY_DELETED"), 0u)
        << "No event expected for revoking unknown key";
}

TEST_F(KeyRotationAuditTest, NoLogger_RotateAndRevoke_NoCrash) {
    // Default: no audit logger attached
    EXPECT_NO_THROW({
        mgr_->rotateActiveKey("key-v1");
        mgr_->revokeKey("key-v1");
    });
    EXPECT_FALSE(std::filesystem::exists(log_path_));
}

TEST_F(KeyRotationAuditTest, MultipleRotations_EachEmitsEvent) {
    mgr_->setAuditLogger(logger_.get());

    mgr_->rotateActiveKey("key-v1");
    mgr_->rotateActiveKey("key-v2");
    mgr_->rotateActiveKey("key-v3");

    EXPECT_GE(countMatching(log_path_, "KEY_ROTATED"), 3u);
}
