/**
 * @file test_http_retention_api.cpp
 * @brief Unit tests for RetentionApiHandler
 *        (themis::server::RetentionApiHandler + vcc::RetentionManager)
 *
 * Tests exercise listPolicies, createOrUpdatePolicy, deletePolicy,
 * getHistory, and getPolicyStats directly through the handler's JSON API,
 * without starting an HTTP server.
 */

#include <gtest/gtest.h>
#include "server/retention_api_handler.h"
#include "utils/retention_manager.h"
#include <memory>
#include <nlohmann/json.hpp>

using namespace themis::server;
using nlohmann::json;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static json makePolicy(const std::string& name,
                       int retention_days = 30,
                       const std::string& classification = "offen",
                       bool auto_purge = false) {
    return json{
        {"name",                  name},
        {"retention_period_days", retention_days},
        {"archive_after_days",    retention_days / 2},
        {"auto_purge_enabled",    auto_purge},
        {"require_audit_trail",   true},
        {"classification_level",  classification}
    };
}

// ─── Fixture ─────────────────────────────────────────────────────────────────

struct RetentionApiFixture : ::testing::Test {
    std::shared_ptr<vcc::RetentionManager> mgr;
    std::unique_ptr<RetentionApiHandler> handler;

    void SetUp() override {
        mgr     = std::make_shared<vcc::RetentionManager>();
        handler = std::make_unique<RetentionApiHandler>(mgr);
    }
};

// ─── listPolicies ─────────────────────────────────────────────────────────────

TEST_F(RetentionApiFixture, ListPolicies_EmptyByDefault) {
    RetentionQueryFilter f;
    auto result = handler->listPolicies(f);
    ASSERT_TRUE(result.contains("items"));
    ASSERT_TRUE(result.contains("total"));
    EXPECT_EQ(result["total"].get<int>(), 0);
    EXPECT_TRUE(result["items"].is_array());
    EXPECT_TRUE(result["items"].empty());
}

TEST_F(RetentionApiFixture, ListPolicies_AfterCreate) {
    handler->createOrUpdatePolicy(makePolicy("gdpr"));
    handler->createOrUpdatePolicy(makePolicy("eidas"));

    RetentionQueryFilter f;
    auto result = handler->listPolicies(f);
    EXPECT_EQ(result["total"].get<int>(), 2);
    EXPECT_EQ(result["items"].size(), 2u);
}

TEST_F(RetentionApiFixture, ListPolicies_NameFilter) {
    handler->createOrUpdatePolicy(makePolicy("gdpr_personal"));
    handler->createOrUpdatePolicy(makePolicy("eidas_signing"));

    RetentionQueryFilter f;
    f.name_filter = "gdpr";
    auto result = handler->listPolicies(f);
    EXPECT_EQ(result["total"].get<int>(), 1);
    EXPECT_EQ(result["items"][0]["name"].get<std::string>(), "gdpr_personal");
}

TEST_F(RetentionApiFixture, ListPolicies_InvalidNameFilterReturnsError) {
    RetentionQueryFilter f;
    f.name_filter = "gdpr\r\nX-Injected: 1";
    auto result = handler->listPolicies(f);
    EXPECT_EQ(result["status"].get<std::string>(), "error");
}

TEST_F(RetentionApiFixture, ListPolicies_ClassificationFilter) {
    handler->createOrUpdatePolicy(makePolicy("p1", 30, "geheim"));
    handler->createOrUpdatePolicy(makePolicy("p2", 30, "offen"));

    RetentionQueryFilter f;
    f.classification_filter = "geheim";
    auto result = handler->listPolicies(f);
    EXPECT_EQ(result["total"].get<int>(), 1);
    EXPECT_EQ(result["items"][0]["name"].get<std::string>(), "p1");
}

TEST_F(RetentionApiFixture, ListPolicies_Pagination) {
    for (int i = 0; i < 5; ++i) {
        handler->createOrUpdatePolicy(makePolicy("policy_" + std::to_string(i)));
    }
    RetentionQueryFilter f;
    f.page      = 1;
    f.page_size = 2;
    auto result = handler->listPolicies(f);
    EXPECT_EQ(result["total"].get<int>(), 5);
    EXPECT_EQ(result["items"].size(), 2u);
    EXPECT_EQ(result["page"].get<int>(), 1);
    EXPECT_EQ(result["page_size"].get<int>(), 2);
}

TEST_F(RetentionApiFixture, ListPolicies_Page2) {
    for (int i = 0; i < 5; ++i) {
        handler->createOrUpdatePolicy(makePolicy("policy_" + std::to_string(i)));
    }
    RetentionQueryFilter f;
    f.page      = 2;
    f.page_size = 2;
    auto result = handler->listPolicies(f);
    EXPECT_EQ(result["items"].size(), 2u);
}

// ─── createOrUpdatePolicy ────────────────────────────────────────────────────

TEST_F(RetentionApiFixture, CreatePolicy_Success) {
    auto result = handler->createOrUpdatePolicy(makePolicy("new_policy"));
    ASSERT_TRUE(result.contains("status"));
    EXPECT_EQ(result["status"].get<std::string>(), "created");
    EXPECT_EQ(result["name"].get<std::string>(), "new_policy");
}

TEST_F(RetentionApiFixture, UpdatePolicy_ExistingReturnsUpdated) {
    handler->createOrUpdatePolicy(makePolicy("my_policy"));
    auto result = handler->createOrUpdatePolicy(makePolicy("my_policy", 60));
    EXPECT_EQ(result["status"].get<std::string>(), "updated");
}

TEST_F(RetentionApiFixture, CreatePolicy_MissingNameReturnsError) {
    json bad_policy = {{"retention_period_days", 30}};  // no "name"
    auto result = handler->createOrUpdatePolicy(bad_policy);
    ASSERT_TRUE(result.contains("status"));
    EXPECT_EQ(result["status"].get<std::string>(), "error");
    EXPECT_TRUE(result.contains("error"));
}

TEST_F(RetentionApiFixture, CreatePolicy_InvalidJsonReturnsError) {
    // retention_period_days is missing – parsing will throw
    json policy = {{"name", "p"}, {"classification_level", "offen"}};
    // Missing required "retention_period_days" → jsonToPolicy throws
    auto result = handler->createOrUpdatePolicy(policy);
    EXPECT_EQ(result["status"].get<std::string>(), "error");
}

TEST_F(RetentionApiFixture, CreatePolicy_InvalidNameReturnsError) {
    auto result = handler->createOrUpdatePolicy(makePolicy("../bad_policy"));
    EXPECT_EQ(result["status"].get<std::string>(), "error");
}

TEST_F(RetentionApiFixture, CreatePolicy_NegativeRetentionReturnsError) {
    auto result = handler->createOrUpdatePolicy(makePolicy("bad_days", -3));
    EXPECT_EQ(result["status"].get<std::string>(), "error");
}

TEST_F(RetentionApiFixture, CreatePolicy_RetentionPeriodInJson) {
    handler->createOrUpdatePolicy(makePolicy("check_policy", 90));

    RetentionQueryFilter f;
    auto list = handler->listPolicies(f);
    ASSERT_FALSE(list["items"].empty());
    bool found = false;
    for (const auto& item : list["items"]) {
        if (item["name"] == "check_policy") {
            EXPECT_EQ(item["retention_period_days"].get<int>(), 90);
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// ─── deletePolicy ─────────────────────────────────────────────────────────────

TEST_F(RetentionApiFixture, DeletePolicy_Success) {
    handler->createOrUpdatePolicy(makePolicy("to_delete"));
    auto result = handler->deletePolicy("to_delete");
    EXPECT_EQ(result["status"].get<std::string>(), "deleted");
    EXPECT_EQ(result["name"].get<std::string>(), "to_delete");

    RetentionQueryFilter f;
    EXPECT_EQ(handler->listPolicies(f)["total"].get<int>(), 0);
}

TEST_F(RetentionApiFixture, DeletePolicy_NonExistentReturnsError) {
    auto result = handler->deletePolicy("does_not_exist");
    EXPECT_EQ(result["status"].get<std::string>(), "error");
    EXPECT_TRUE(result.contains("error"));
}

TEST_F(RetentionApiFixture, DeletePolicy_InvalidNameReturnsError) {
    auto result = handler->deletePolicy("../bad_policy");
    EXPECT_EQ(result["status"].get<std::string>(), "error");
}

// ─── getHistory ───────────────────────────────────────────────────────────────

TEST_F(RetentionApiFixture, GetHistory_EmptyByDefault) {
    auto result = handler->getHistory(100);
    ASSERT_TRUE(result.contains("items"));
    EXPECT_TRUE(result["items"].is_array());
    EXPECT_EQ(result["total"].get<size_t>(), 0u);
}

TEST_F(RetentionApiFixture, GetHistory_LimitFieldPresent) {
    auto result = handler->getHistory(50);
    EXPECT_EQ(result["limit"].get<size_t>(), 50u);
}

// ─── getPolicyStats ───────────────────────────────────────────────────────────

TEST_F(RetentionApiFixture, GetPolicyStats_HasExpectedFields) {
    handler->createOrUpdatePolicy(makePolicy("stats_policy"));
    auto result = handler->getPolicyStats("stats_policy");
    EXPECT_TRUE(result.contains("policy_name"));
    EXPECT_TRUE(result.contains("total_scanned"));
    EXPECT_TRUE(result.contains("archived"));
    EXPECT_TRUE(result.contains("purged"));
    EXPECT_TRUE(result.contains("retained"));
    EXPECT_TRUE(result.contains("errors"));
    EXPECT_TRUE(result.contains("duration_ms"));
    EXPECT_EQ(result["policy_name"].get<std::string>(), "stats_policy");
}

TEST_F(RetentionApiFixture, GetPolicyStats_InvalidNameReturnsError) {
    auto result = handler->getPolicyStats("../bad_policy");
    EXPECT_EQ(result["status"].get<std::string>(), "error");
}

// ─── Default constructor (no manager provided) ────────────────────────────────

TEST(RetentionApiHandlerTest, ConstructsWithDefaultManager) {
    // Should not throw; creates a default RetentionManager internally
    EXPECT_NO_THROW({ RetentionApiHandler handler; });
}

TEST(RetentionApiHandlerTest, DefaultManagerListsEmpty) {
    RetentionApiHandler handler;
    RetentionQueryFilter f;
    auto result = handler.listPolicies(f);
    EXPECT_EQ(result["total"].get<int>(), 0);
}

