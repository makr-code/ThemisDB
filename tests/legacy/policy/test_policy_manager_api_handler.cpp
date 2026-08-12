#include <gtest/gtest.h>
#include "server/policy_manager_api_handler.h"
#include "governance/policy_manager.h"
#include <memory>

using namespace themis::server;
using namespace themis::governance;
namespace http = boost::beast::http;

class PolicyManagerApiHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        policy_manager = std::make_shared<PolicyManager>();
        handler = std::make_unique<PolicyManagerApiHandler>(policy_manager, nullptr);
        
        // Add some test rules
        PolicyRule rule1;
        rule1.id = "test_rule_001";
        rule1.name = "Test Rule 1";
        rule1.resources = {"data/*"};
        rule1.actions = {"read"};
        rule1.enabled = true;
        policy_manager->addRule(rule1);
        
        PolicyRule rule2;
        rule2.id = "test_rule_002";
        rule2.name = "Test Rule 2";
        rule2.resources = {"keys/*"};
        rule2.actions = {"*"};
        rule2.enabled = false;
        policy_manager->addRule(rule2);
    }
    
    http::request<http::string_body> makeRequest(
        http::verb method,
        const std::string& target = "/",
        const std::string& body = ""
    ) {
        http::request<http::string_body> req{method, target, 11};
        req.set(http::field::host, "localhost");
        req.set(http::field::content_type, "application/json");
        req.set(http::field::authorization, "Bearer test_token");
        req.body() = body;
        req.prepare_payload();
        return req;
    }
    
    std::shared_ptr<PolicyManager> policy_manager;
    std::unique_ptr<PolicyManagerApiHandler> handler;
};

TEST_F(PolicyManagerApiHandlerTest, HandleListRules) {
    auto req = makeRequest(http::verb::get, "/policies/rules");
    auto res = handler->handleListRules(req);
    
    EXPECT_EQ(res.result(), http::status::ok);
    
    auto body = nlohmann::json::parse(res.body());
    EXPECT_TRUE(body.contains("rules"));
    EXPECT_TRUE(body.contains("count"));
    EXPECT_EQ(body["count"].get<int>(), 2);
    EXPECT_EQ(body["rules"].size(), 2);
}

TEST_F(PolicyManagerApiHandlerTest, HandleGetRuleFound) {
    auto req = makeRequest(http::verb::get, "/policies/rules/test_rule_001");
    auto res = handler->handleGetRule(req, "test_rule_001");
    
    EXPECT_EQ(res.result(), http::status::ok);
    
    auto body = nlohmann::json::parse(res.body());
    EXPECT_EQ(body["id"].get<std::string>(), "test_rule_001");
    EXPECT_EQ(body["name"].get<std::string>(), "Test Rule 1");
}

TEST_F(PolicyManagerApiHandlerTest, HandleGetRuleNotFound) {
    auto req = makeRequest(http::verb::get, "/policies/rules/nonexistent");
    auto res = handler->handleGetRule(req, "nonexistent");
    
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(PolicyManagerApiHandlerTest, HandleGetRule_InvalidRuleId_Returns400) {
    auto req = makeRequest(http::verb::get, "/policies/rules/../bad");
    auto res = handler->handleGetRule(req, "../bad");

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(PolicyManagerApiHandlerTest, HandleCreateRule) {
    nlohmann::json new_rule = {
        {"id", "test_rule_003"},
        {"name", "Test Rule 3"},
        {"description", "Created via API"},
        {"resources", {"api/*"}},
        {"actions", {"call"}},
        {"enabled", true},
        {"require_encryption", true}
    };
    
    auto req = makeRequest(http::verb::post, "/policies/rules", new_rule.dump());
    auto res = handler->handleCreateRule(req);
    
    EXPECT_EQ(res.result(), http::status::created);
    
    auto body = nlohmann::json::parse(res.body());
    EXPECT_TRUE(body.contains("rule"));
    EXPECT_EQ(body["rule"]["id"].get<std::string>(), "test_rule_003");
    
    // Verify rule was added
    auto rule = policy_manager->getRule("test_rule_003");
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->name, "Test Rule 3");
}

TEST_F(PolicyManagerApiHandlerTest, HandleCreateRuleDuplicate) {
    nlohmann::json duplicate_rule = {
        {"id", "test_rule_001"},  // Already exists
        {"name", "Duplicate"},
        {"resources", {"test/*"}},
        {"actions", {"read"}}
    };
    
    auto req = makeRequest(http::verb::post, "/policies/rules", duplicate_rule.dump());
    auto res = handler->handleCreateRule(req);
    
    EXPECT_EQ(res.result(), http::status::conflict);
}

TEST_F(PolicyManagerApiHandlerTest, HandleCreateRuleInvalidJSON) {
    auto req = makeRequest(http::verb::post, "/policies/rules", "{invalid json");
    auto res = handler->handleCreateRule(req);
    
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(PolicyManagerApiHandlerTest, HandleCreateRule_InvalidRuleId_Returns400) {
    nlohmann::json new_rule = {
        {"id", "../bad_rule"},
        {"name", "Bad Rule"},
        {"resources", {"api/*"}},
        {"actions", {"call"}}
    };

    auto req = makeRequest(http::verb::post, "/policies/rules", new_rule.dump());
    auto res = handler->handleCreateRule(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(PolicyManagerApiHandlerTest, HandleUpdateRule) {
    nlohmann::json updated_rule = {
        {"name", "Updated Rule 1"},
        {"description", "Updated via API"},
        {"resources", {"data/updated/*"}},
        {"actions", {"read", "write"}},
        {"enabled", false}
    };
    
    auto req = makeRequest(http::verb::put, "/policies/rules/test_rule_001", updated_rule.dump());
    auto res = handler->handleUpdateRule(req, "test_rule_001");
    
    EXPECT_EQ(res.result(), http::status::ok);
    
    auto body = nlohmann::json::parse(res.body());
    EXPECT_TRUE(body.contains("rule"));
    EXPECT_EQ(body["rule"]["name"].get<std::string>(), "Updated Rule 1");
    
    // Verify rule was updated
    auto rule = policy_manager->getRule("test_rule_001");
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->name, "Updated Rule 1");
    EXPECT_FALSE(rule->enabled);
}

TEST_F(PolicyManagerApiHandlerTest, HandleUpdateRuleNotFound) {
    nlohmann::json updated_rule = {
        {"name", "Should Fail"},
        {"resources", {"test/*"}},
        {"actions", {"read"}}
    };
    
    auto req = makeRequest(http::verb::put, "/policies/rules/nonexistent", updated_rule.dump());
    auto res = handler->handleUpdateRule(req, "nonexistent");
    
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(PolicyManagerApiHandlerTest, HandleDeleteRule) {
    auto req = makeRequest(http::verb::delete_, "/policies/rules/test_rule_002");
    auto res = handler->handleDeleteRule(req, "test_rule_002");
    
    EXPECT_EQ(res.result(), http::status::ok);
    
    // Verify rule was deleted
    auto rule = policy_manager->getRule("test_rule_002");
    EXPECT_FALSE(rule.has_value());
}

TEST_F(PolicyManagerApiHandlerTest, HandleDeleteRuleNotFound) {
    auto req = makeRequest(http::verb::delete_, "/policies/rules/nonexistent");
    auto res = handler->handleDeleteRule(req, "nonexistent");
    
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(PolicyManagerApiHandlerTest, HandleEvaluatePolicy) {
    nlohmann::json eval_request = {
        {"resource", "data/users"},
        {"action", "read"},
        {"user_roles", {"operator", "analyst"}}
    };
    
    auto req = makeRequest(http::verb::post, "/policies/evaluate", eval_request.dump());
    auto res = handler->handleEvaluatePolicy(req);
    
    EXPECT_EQ(res.result(), http::status::ok);
    
    auto body = nlohmann::json::parse(res.body());
    EXPECT_TRUE(body.contains("decision"));
    EXPECT_TRUE(body["decision"].contains("allowed"));
    EXPECT_TRUE(body["decision"]["allowed"].get<bool>());  // Should match test_rule_001
}

TEST_F(PolicyManagerApiHandlerTest, HandleEvaluatePolicyMissingFields) {
    nlohmann::json eval_request = {
        {"resource", "data/users"}
        // Missing "action" field
    };
    
    auto req = makeRequest(http::verb::post, "/policies/evaluate", eval_request.dump());
    auto res = handler->handleEvaluatePolicy(req);
    
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(PolicyManagerApiHandlerTest, HandleEvaluatePolicy_HeaderInjectionInResource_Returns400) {
    nlohmann::json eval_request = {
        {"resource", "data/users\r\nX-Injected: 1"},
        {"action", "read"},
        {"user_roles", {"operator"}}
    };

    auto req = makeRequest(http::verb::post, "/policies/evaluate", eval_request.dump());
    auto res = handler->handleEvaluatePolicy(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(PolicyManagerApiHandlerTest, HandleEvaluatePolicyNoMatchingRules) {
    nlohmann::json eval_request = {
        {"resource", "nonexistent/resource"},
        {"action", "delete"},
        {"user_roles", {"guest"}}
    };
    
    auto req = makeRequest(http::verb::post, "/policies/evaluate", eval_request.dump());
    auto res = handler->handleEvaluatePolicy(req);
    
    EXPECT_EQ(res.result(), http::status::ok);
    
    auto body = nlohmann::json::parse(res.body());
    EXPECT_TRUE(body["decision"]["allowed"].get<bool>());  // Default allow when no rules match
}

TEST_F(PolicyManagerApiHandlerTest, HandleGetStats) {
    auto req = makeRequest(http::verb::get, "/policies/stats");
    auto res = handler->handleGetStats(req);
    
    EXPECT_EQ(res.result(), http::status::ok);
    
    auto body = nlohmann::json::parse(res.body());
    EXPECT_TRUE(body.contains("total_rules"));
    EXPECT_TRUE(body.contains("enabled_rules"));
    EXPECT_TRUE(body.contains("disabled_rules"));
    EXPECT_EQ(body["total_rules"].get<int>(), 2);
    EXPECT_EQ(body["enabled_rules"].get<int>(), 1);
    EXPECT_EQ(body["disabled_rules"].get<int>(), 1);
}

TEST_F(PolicyManagerApiHandlerTest, HandleWithNullPolicyManager) {
    auto null_handler = std::make_unique<PolicyManagerApiHandler>(nullptr, nullptr);
    
    auto req = makeRequest(http::verb::get, "/policies/rules");
    auto res = null_handler->handleListRules(req);
    
    EXPECT_EQ(res.result(), http::status::service_unavailable);
}

TEST_F(PolicyManagerApiHandlerTest, ResponseHeaders) {
    auto req = makeRequest(http::verb::get, "/policies/rules");
    auto res = handler->handleListRules(req);
    
    EXPECT_EQ(res[http::field::content_type], "application/json");
    EXPECT_EQ(res[http::field::server], "ThemisDB");
}
