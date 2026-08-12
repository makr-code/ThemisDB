#include <gtest/gtest.h>
#include "security/query_masking_policy.h"
#include "utils/pii_detection_engine.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace themis::security;
using namespace themis::utils;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Fixture: shared policy with default config
// ---------------------------------------------------------------------------

class QueryMaskingPolicyTest : public ::testing::Test {
protected:
    void SetUp() override {
        policy_ = QueryMaskingPolicy::create();
    }

    std::shared_ptr<QueryMaskingPolicy> policy_;
};

// ---------------------------------------------------------------------------
// Feature: policy enable/disable
// ---------------------------------------------------------------------------

TEST_F(QueryMaskingPolicyTest, EnabledByDefault) {
    EXPECT_TRUE(policy_->isEnabled());
}

TEST_F(QueryMaskingPolicyTest, DisabledPolicyPassesThroughUnchanged) {
    policy_->setEnabled(false);

    json entity = {{"email", "alice@example.com"}, {"name", "Alice"}};
    auto result = policy_->maskResult(entity);

    EXPECT_EQ(result["email"].get<std::string>(), "alice@example.com")
        << "Disabled policy must not mask values";
}

// ---------------------------------------------------------------------------
// Feature: privileged roles bypass masking
// ---------------------------------------------------------------------------

TEST_F(QueryMaskingPolicyTest, AdminRoleBypassesMasking) {
    json entity = {{"email", "bob@corp.org"}, {"status", "active"}};
    auto result = policy_->maskResult(entity, {"admin"});

    EXPECT_EQ(result["email"].get<std::string>(), "bob@corp.org")
        << "Admin role must receive unmasked results";
    EXPECT_EQ(result["status"].get<std::string>(), "active");
}

TEST_F(QueryMaskingPolicyTest, NonPrivilegedRoleGetsMaskedResults) {
    json entity = {{"email", "bob@corp.org"}};
    auto result = policy_->maskResult(entity, {"readonly"});

    EXPECT_NE(result["email"].get<std::string>(), "bob@corp.org")
        << "Non-privileged role must receive masked results";
}

TEST_F(QueryMaskingPolicyTest, IsPrivilegedReturnsTrueForAdminRole) {
    EXPECT_TRUE(policy_->isPrivileged({"admin"}));
    EXPECT_FALSE(policy_->isPrivileged({"user", "readonly"}));
}

// ---------------------------------------------------------------------------
// Feature: field-name hint masking
// ---------------------------------------------------------------------------

TEST_F(QueryMaskingPolicyTest, EmailFieldMaskedByName) {
    // The PIIDetector recognises "email" as a PII field-name hint.
    json entity = {{"email", "alice@example.com"}};
    auto result = policy_->maskResult(entity);

    EXPECT_EQ(result["email"].get<std::string>().find("alice@example.com"),
              std::string::npos)
        << "Email field must be masked by field-name hint";
}

TEST_F(QueryMaskingPolicyTest, NonPIIFieldPassesThrough) {
    json entity = {
        {"id", "12345"},
        {"status", "active"},
        {"count", 42}
    };
    auto result = policy_->maskResult(entity);

    EXPECT_EQ(result["id"].get<std::string>(), "12345");
    EXPECT_EQ(result["status"].get<std::string>(), "active");
    EXPECT_EQ(result["count"].get<int>(), 42);
}

// ---------------------------------------------------------------------------
// Feature: auto-detect PII in string values
// ---------------------------------------------------------------------------

TEST_F(QueryMaskingPolicyTest, AutoDetectEmailInValue) {
    // Value contains an email but key name is neutral.
    json entity = {{"note", "Contact alice@example.com for info"}};
    auto result = policy_->maskResult(entity);

    EXPECT_EQ(result["note"].get<std::string>().find("alice@example.com"),
              std::string::npos)
        << "Auto-detect should mask email in value";
    // Non-PII parts should be preserved.
    EXPECT_NE(result["note"].get<std::string>().find("Contact"),
              std::string::npos);
}

TEST_F(QueryMaskingPolicyTest, AutoDetectSSNInValue) {
    json entity = {{"description", "SSN: 123-45-6789 on file"}};
    auto result = policy_->maskResult(entity);

    EXPECT_EQ(result["description"].get<std::string>().find("123-45-6789"),
              std::string::npos)
        << "Auto-detect should mask SSN in value";
}

// ---------------------------------------------------------------------------
// Feature: explicit field declarations
// ---------------------------------------------------------------------------

TEST_F(QueryMaskingPolicyTest, DeclaredFieldAlwaysMasked) {
    policy_->declareField("national_id", "strict");

    json entity = {{"national_id", "DE12345678"}};
    auto result = policy_->maskResult(entity);

    EXPECT_EQ(result["national_id"].get<std::string>().find("DE12345678"),
              std::string::npos)
        << "Explicitly declared field must be masked";
}

TEST_F(QueryMaskingPolicyTest, UndeclaredFieldRestoredToAutoDetect) {
    policy_->declareField("safe_field", "strict");
    policy_->undeclareField("safe_field");

    // After undeclaration, value with no PII should pass through.
    json entity = {{"safe_field", "hello_world"}};
    auto result = policy_->maskResult(entity);

    EXPECT_EQ(result["safe_field"].get<std::string>(), "hello_world");
}

// ---------------------------------------------------------------------------
// Feature: maskResultSet (array of entities)
// ---------------------------------------------------------------------------

TEST_F(QueryMaskingPolicyTest, MaskResultSetMasksAllEntities) {
    json entities = json::array({
        {{"email", "alice@example.com"}, {"id", "1"}},
        {{"email", "bob@example.com"},   {"id", "2"}}
    });

    auto result = policy_->maskResultSet(entities);

    ASSERT_TRUE(result.is_array());
    ASSERT_EQ(result.size(), 2u);

    for (const auto& entity : result) {
        auto email = entity["email"].get<std::string>();
        EXPECT_EQ(email.find("@example.com"), std::string::npos)
            << "All entities in result set should have email masked";
    }
}

TEST_F(QueryMaskingPolicyTest, MaskResultSetAdminBypassesAll) {
    json entities = json::array({
        {{"email", "alice@example.com"}},
        {{"email", "bob@example.com"}}
    });

    auto result = policy_->maskResultSet(entities, {"admin"});

    ASSERT_TRUE(result.is_array());
    EXPECT_EQ(result[0]["email"].get<std::string>(), "alice@example.com");
    EXPECT_EQ(result[1]["email"].get<std::string>(), "bob@example.com");
}

// ---------------------------------------------------------------------------
// Feature: nested JSON objects are recursively masked
// ---------------------------------------------------------------------------

TEST_F(QueryMaskingPolicyTest, NestedObjectFieldsMasked) {
    json entity = {
        {"contact", {
            {"email", "nested@example.com"},
            {"phone", "+1-555-123-4567"}
        }},
        {"id", "user-001"}
    };

    auto result = policy_->maskResult(entity);

    EXPECT_EQ(result["id"].get<std::string>(), "user-001");
    auto contact = result["contact"];
    EXPECT_EQ(contact["email"].get<std::string>().find("nested@example.com"),
              std::string::npos)
        << "Nested email must be masked";
}

// ---------------------------------------------------------------------------
// Feature: non-string scalars pass through unchanged
// ---------------------------------------------------------------------------

TEST_F(QueryMaskingPolicyTest, NumericAndBoolFieldsPassThrough) {
    json entity = {
        {"score", 98.6},
        {"active", true},
        {"count", 42},
        {"nothing", nullptr}
    };

    auto result = policy_->maskResult(entity);

    EXPECT_DOUBLE_EQ(result["score"].get<double>(), 98.6);
    EXPECT_EQ(result["active"].get<bool>(), true);
    EXPECT_EQ(result["count"].get<int>(), 42);
    EXPECT_TRUE(result["nothing"].is_null());
}

// ---------------------------------------------------------------------------
// Feature: maskResultSet on non-array input (graceful fallback)
// ---------------------------------------------------------------------------

TEST_F(QueryMaskingPolicyTest, MaskResultSetOnObjectFallsBackToMaskResult) {
    json single = {{"email", "fallback@example.com"}};
    auto result = policy_->maskResultSet(single);

    // Should still mask correctly even though input is not an array.
    EXPECT_EQ(result["email"].get<std::string>().find("fallback@example.com"),
              std::string::npos);
}

// ---------------------------------------------------------------------------
// Feature: empty inputs produce safe defaults
// ---------------------------------------------------------------------------

TEST_F(QueryMaskingPolicyTest, EmptyObjectReturnedAsEmptyObject) {
    json empty = json::object();
    auto result = policy_->maskResult(empty);
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.empty());
}

TEST_F(QueryMaskingPolicyTest, EmptyArrayReturnedAsEmptyArray) {
    json empty = json::array();
    auto result = policy_->maskResultSet(empty);
    EXPECT_TRUE(result.is_array());
    EXPECT_TRUE(result.empty());
}

// ---------------------------------------------------------------------------
// Feature: thread-safety – concurrent declareField and maskResult
// ---------------------------------------------------------------------------

#include <thread>
#include <atomic>

TEST_F(QueryMaskingPolicyTest, ConcurrentMaskAndDeclareFieldIsSafe) {
    // Repeatedly mask entities from one thread while another thread adds and
    // removes declared fields.  The test verifies there is no crash or
    // sanitizer error (the actual masked output may legitimately vary by
    // ordering, but no undefined behaviour must occur).
    const int iterations = 200;
    std::atomic<bool> stop{false};

    std::thread writer([&]() {
        for (int i = 0; !stop.load() && i < iterations * 2; ++i) {
            policy_->declareField("concurrent_field", "strict");
            policy_->undeclareField("concurrent_field");
        }
    });

    for (int i = 0; i < iterations; ++i) {
        json entity = {
            {"concurrent_field", "test_value_12345"},
            {"email", "race@example.com"}
        };
        // Must not crash or abort – outcome may be masked or not depending on timing.
        auto result = policy_->maskResult(entity);
        EXPECT_TRUE(result.is_object());
        EXPECT_TRUE(result.contains("concurrent_field"));
        EXPECT_TRUE(result.contains("email"));
    }

    stop.store(true);
    writer.join();
}

// ---------------------------------------------------------------------------
// Feature: mixed roles – admin in list with other roles bypasses masking
// ---------------------------------------------------------------------------

TEST_F(QueryMaskingPolicyTest, MixedRolesWithAdminBypassesMasking) {
    json entity = {{"email", "mixed@example.com"}, {"status", "active"}};
    // Admin is listed alongside other non-privileged roles.
    auto result = policy_->maskResult(entity, {"user", "admin", "readonly"});

    EXPECT_EQ(result["email"].get<std::string>(), "mixed@example.com")
        << "Admin role among other roles must still bypass masking";
    EXPECT_EQ(result["status"].get<std::string>(), "active");
}

// ---------------------------------------------------------------------------
// Feature: both auto-detect and field-name masking disabled → pass-through
// ---------------------------------------------------------------------------

TEST_F(QueryMaskingPolicyTest, BothDetectionModesDisabledPassesThrough) {
    QueryMaskingPolicy::Config cfg;
    cfg.mask_by_field_name = false;
    cfg.auto_detect_pii    = false;
    auto policy = QueryMaskingPolicy::create(cfg);

    // Even a well-known PII field name should pass through when both modes are off.
    json entity = {{"email", "alice@example.com"}, {"id", "123"}};
    auto result = policy->maskResult(entity);

    EXPECT_EQ(result["email"].get<std::string>(), "alice@example.com")
        << "With both detection modes disabled, PII field must not be masked";
    EXPECT_EQ(result["id"].get<std::string>(), "123");
}

// ---------------------------------------------------------------------------
// Feature: declareField with explicit PIIType hint
// ---------------------------------------------------------------------------

TEST_F(QueryMaskingPolicyTest, DeclaredFieldWithExplicitPIITypeHint) {
    // Declare a field with an explicit SSN type hint so the masker uses the
    // SSN masking format rather than falling back to the generic form.
    policy_->declareField("national_id", "strict", PIIType::SSN);

    json entity = {{"national_id", "123-45-6789"}};
    auto result = policy_->maskResult(entity);

    EXPECT_EQ(result["national_id"].get<std::string>().find("123-45-6789"),
              std::string::npos)
        << "Declared field with explicit PIIType must be masked";
    // The returned value must not be empty (a masking token or redaction
    // marker is expected).
    EXPECT_FALSE(result["national_id"].get<std::string>().empty());
}
