/**
 * @file test_api_key_mgmt_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include <gtest/gtest.h>
#include "server/api_key_mgmt_handler.h"
#include "server/auth_middleware.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <string>

using namespace themis;
using namespace themis::server;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class ApiKeyMgmtHandlerTest : public ::testing::Test {
protected:
    std::shared_ptr<AuthMiddleware> auth_;
    std::unique_ptr<ApiKeyMgmtHandler> handler_;

    void SetUp() override {
        auth_    = std::make_shared<AuthMiddleware>();
        handler_ = std::make_unique<ApiKeyMgmtHandler>(auth_);
    }
};

// ---------------------------------------------------------------------------
// Token format
// ---------------------------------------------------------------------------

TEST_F(ApiKeyMgmtHandlerTest, CreateKey_TokenFormat) {
    json body = {{"name", "test-key"}, {"permissions", json::array({"api:read"})}};
    json result = handler_->createKey(body);

    ASSERT_FALSE(result.contains("status_code")) << "Expected success, got: " << result.dump();
    ASSERT_TRUE(result.contains("secret"));
    std::string secret = result["secret"].get<std::string>();

    // Prefix check
    EXPECT_EQ(secret.substr(0, 7), "themis_") << "Token should start with 'themis_'";
    // Length check: "themis_" (7) + 64 hex chars = 71
    EXPECT_EQ(secret.size(), 71u) << "Token length should be 71";
}

// ---------------------------------------------------------------------------
// Create
// ---------------------------------------------------------------------------

TEST_F(ApiKeyMgmtHandlerTest, CreateKey_Success) {
    json body = {
        {"name", "my-service-key"},
        {"permissions", json::array({"api:read", "api:write"})},
        {"expires_in_days", 30}
    };
    json result = handler_->createKey(body);

    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_TRUE(result.contains("id"));
    EXPECT_EQ(result["name"].get<std::string>(), "my-service-key");
    EXPECT_TRUE(result.contains("secret"));
    EXPECT_TRUE(result.contains("created_at"));
    EXPECT_TRUE(result.contains("expires_at"));
    EXPECT_FALSE(result["id"].get<std::string>().empty());

    // Key id should start with "key_"
    std::string kid = result["id"].get<std::string>();
    EXPECT_EQ(kid.substr(0, 4), "key_");
}

TEST_F(ApiKeyMgmtHandlerTest, CreateKey_NoExpiry) {
    json body = {{"name", "no-expiry-key"}};
    json result = handler_->createKey(body);

    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_FALSE(result.contains("expires_at"));
}

TEST_F(ApiKeyMgmtHandlerTest, CreateKey_MissingName_ReturnsError) {
    json body = {{"permissions", json::array({"api:read"})}};
    json result = handler_->createKey(body);

    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

TEST_F(ApiKeyMgmtHandlerTest, CreateKey_EmptyName_ReturnsError) {
    json body = {{"name", ""}};
    json result = handler_->createKey(body);

    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

TEST_F(ApiKeyMgmtHandlerTest, CreateKey_ActivatesInAuthMiddleware) {
    json body = {{"name", "auth-test-key"}, {"permissions", json::array({"api:read"})}};
    json result = handler_->createKey(body);
    ASSERT_FALSE(result.contains("status_code"));

    std::string secret = result["secret"].get<std::string>();
    // The newly created key should be recognised by AuthMiddleware
    auto auth_result = auth_->validateToken(secret);
    EXPECT_TRUE(auth_result.authorized);
}

// ---------------------------------------------------------------------------
// List
// ---------------------------------------------------------------------------

TEST_F(ApiKeyMgmtHandlerTest, ListKeys_EmptyInitially) {
    json result = handler_->listKeys();
    ASSERT_TRUE(result.contains("total"));
    EXPECT_EQ(result["total"].get<int>(), 0);
    EXPECT_TRUE(result["items"].is_array());
    EXPECT_EQ(result["items"].size(), 0u);
}

TEST_F(ApiKeyMgmtHandlerTest, ListKeys_ReflectsCreatedKeys) {
    handler_->createKey({{"name", "key-a"}});
    handler_->createKey({{"name", "key-b"}});

    json result = handler_->listKeys();
    EXPECT_EQ(result["total"].get<int>(), 2);
    EXPECT_EQ(result["items"].size(), 2u);

    // Secrets must NOT appear in the list
    for (const auto& item : result["items"]) {
        EXPECT_FALSE(item.contains("secret"));
    }
}

// ---------------------------------------------------------------------------
// Get
// ---------------------------------------------------------------------------

TEST_F(ApiKeyMgmtHandlerTest, GetKey_Success) {
    json create_result = handler_->createKey({{"name", "fetch-me"}});
    ASSERT_FALSE(create_result.contains("status_code"));
    std::string kid = create_result["id"].get<std::string>();

    json get_result = handler_->getKey(kid);
    ASSERT_FALSE(get_result.contains("status_code")) << get_result.dump();
    EXPECT_EQ(get_result["id"].get<std::string>(), kid);
    EXPECT_EQ(get_result["name"].get<std::string>(), "fetch-me");
    EXPECT_FALSE(get_result.contains("secret"));
}

TEST_F(ApiKeyMgmtHandlerTest, GetKey_NotFound) {
    json result = handler_->getKey("key_doesnotexist");
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 404);
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

TEST_F(ApiKeyMgmtHandlerTest, UpdateKey_ChangeName) {
    json create_result = handler_->createKey({{"name", "old-name"}});
    std::string kid = create_result["id"].get<std::string>();

    json update_result = handler_->updateKey(kid, {{"name", "new-name"}});
    ASSERT_FALSE(update_result.contains("status_code")) << update_result.dump();
    EXPECT_EQ(update_result["name"].get<std::string>(), "new-name");
}

TEST_F(ApiKeyMgmtHandlerTest, UpdateKey_ChangePermissions) {
    json create_result = handler_->createKey({
        {"name", "perm-key"},
        {"permissions", json::array({"api:read"})}
    });
    std::string kid    = create_result["id"].get<std::string>();
    std::string secret = create_result["secret"].get<std::string>();

    // Verify old scope is active
    EXPECT_TRUE(auth_->authorize(secret, "api:read").authorized);

    // Update permissions
    json update_body = {{"permissions", json::array({"api:write"})}};
    json update_result = handler_->updateKey(kid, update_body);
    ASSERT_FALSE(update_result.contains("status_code")) << update_result.dump();

    // Old scope should no longer be active
    EXPECT_FALSE(auth_->authorize(secret, "api:read").authorized);
    // New scope should be active
    EXPECT_TRUE(auth_->authorize(secret, "api:write").authorized);
}

TEST_F(ApiKeyMgmtHandlerTest, UpdateKey_NotFound) {
    json result = handler_->updateKey("key_missing", {{"name", "x"}});
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 404);
}

// ---------------------------------------------------------------------------
// Delete
// ---------------------------------------------------------------------------

TEST_F(ApiKeyMgmtHandlerTest, DeleteKey_Success) {
    json create_result = handler_->createKey({{"name", "delete-me"}});
    std::string kid    = create_result["id"].get<std::string>();
    std::string secret = create_result["secret"].get<std::string>();

    // Key should be active before delete
    EXPECT_TRUE(auth_->validateToken(secret).authorized);

    json del_result = handler_->deleteKey(kid);
    ASSERT_FALSE(del_result.contains("status_code")) << del_result.dump();
    EXPECT_TRUE(del_result["success"].get<bool>());

    // Key should no longer appear in list
    json list_result = handler_->listKeys();
    EXPECT_EQ(list_result["total"].get<int>(), 0);

    // Key should no longer be active in AuthMiddleware
    EXPECT_FALSE(auth_->validateToken(secret).authorized);
}

TEST_F(ApiKeyMgmtHandlerTest, DeleteKey_NotFound) {
    json result = handler_->deleteKey("key_ghost");
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 404);
}

// ---------------------------------------------------------------------------
// Uniqueness
// ---------------------------------------------------------------------------

TEST_F(ApiKeyMgmtHandlerTest, MultipleKeys_HaveUniqueIds) {
    json r1 = handler_->createKey({{"name", "k1"}});
    json r2 = handler_->createKey({{"name", "k2"}});
    ASSERT_FALSE(r1.contains("status_code"));
    ASSERT_FALSE(r2.contains("status_code"));

    EXPECT_NE(r1["id"].get<std::string>(), r2["id"].get<std::string>());
    EXPECT_NE(r1["secret"].get<std::string>(), r2["secret"].get<std::string>());
}

// ---------------------------------------------------------------------------
// Null auth middleware
// ---------------------------------------------------------------------------

TEST_F(ApiKeyMgmtHandlerTest, NullAuthMiddleware_CreateKeyStillWorks) {
    // Handler with no auth middleware should not crash – just skips registration
    ApiKeyMgmtHandler no_auth_handler(nullptr);
    json result = no_auth_handler.createKey({{"name", "standalone-key"}});
    EXPECT_FALSE(result.contains("status_code"));
    EXPECT_TRUE(result.contains("secret"));
}

// ---------------------------------------------------------------------------
// Consistency: key must appear in list immediately after creation
// ---------------------------------------------------------------------------

TEST_F(ApiKeyMgmtHandlerTest, CreateKey_ImmediatelyVisibleInList) {
    json create_result = handler_->createKey({{"name", "visible-key"}});
    ASSERT_FALSE(create_result.contains("status_code"));
    std::string kid = create_result["id"].get<std::string>();

    // Must be visible via listKeys
    json list_result = handler_->listKeys();
    bool found = false;
    for (const auto& item : list_result["items"]) {
        if (item["id"].get<std::string>() == kid) { found = true; break; }
    }
    EXPECT_TRUE(found) << "Newly created key must be immediately visible in list";

    // Must also be retrievable by ID
    json get_result = handler_->getKey(kid);
    EXPECT_FALSE(get_result.contains("status_code"));
}

// ---------------------------------------------------------------------------
// After delete: key is gone from both list and auth
// ---------------------------------------------------------------------------

TEST_F(ApiKeyMgmtHandlerTest, DeleteKey_RemovedFromListAndAuth) {
    json r1 = handler_->createKey({{"name", "stay"}});
    json r2 = handler_->createKey({{"name", "go"}});
    std::string kid_go = r2["id"].get<std::string>();
    std::string secret_go = r2["secret"].get<std::string>();

    handler_->deleteKey(kid_go);

    // List should have exactly 1 item now
    json list_result = handler_->listKeys();
    EXPECT_EQ(list_result["total"].get<int>(), 1);

    // getKey must return 404
    json get_result = handler_->getKey(kid_go);
    EXPECT_EQ(get_result.value("status_code", 0), 404);

    // Auth should reject the deleted key
    EXPECT_FALSE(auth_->validateToken(secret_go).authorized);
}
