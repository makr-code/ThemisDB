#include <gtest/gtest.h>
#include "prompt_engineering/system_prompt_manager.h"

using namespace themis::prompt_engineering;

// ============================================================================
// Role ↔ string conversion
// ============================================================================

TEST(SystemPromptManagerTest, RoleToStringRoundTrip) {
    EXPECT_EQ(SystemPromptManager::roleToString(Role::DEFAULT),   "DEFAULT");
    EXPECT_EQ(SystemPromptManager::roleToString(Role::USER),      "USER");
    EXPECT_EQ(SystemPromptManager::roleToString(Role::ASSISTANT), "ASSISTANT");
    EXPECT_EQ(SystemPromptManager::roleToString(Role::ADMIN),     "ADMIN");
    EXPECT_EQ(SystemPromptManager::roleToString(Role::SYSTEM),    "SYSTEM");
    EXPECT_EQ(SystemPromptManager::roleToString(Role::CUSTOM),    "CUSTOM");
}

TEST(SystemPromptManagerTest, StringToRoleRoundTrip) {
    EXPECT_EQ(SystemPromptManager::stringToRole("DEFAULT"),   Role::DEFAULT);
    EXPECT_EQ(SystemPromptManager::stringToRole("USER"),      Role::USER);
    EXPECT_EQ(SystemPromptManager::stringToRole("ASSISTANT"), Role::ASSISTANT);
    EXPECT_EQ(SystemPromptManager::stringToRole("ADMIN"),     Role::ADMIN);
    EXPECT_EQ(SystemPromptManager::stringToRole("SYSTEM"),    Role::SYSTEM);
    EXPECT_EQ(SystemPromptManager::stringToRole("CUSTOM"),    Role::CUSTOM);
}

TEST(SystemPromptManagerTest, UnknownRoleStringFallsBackToDefault) {
    EXPECT_EQ(SystemPromptManager::stringToRole("UNKNOWN"), Role::DEFAULT);
    EXPECT_EQ(SystemPromptManager::stringToRole(""),        Role::DEFAULT);
}

// ============================================================================
// Standard role CRUD
// ============================================================================

TEST(SystemPromptManagerTest, SetAndGetPrompt) {
    SystemPromptManager mgr;
    mgr.setPrompt(Role::USER, "You are a helpful assistant.", "1.0");

    auto opt = mgr.getPrompt(Role::USER);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->content, "You are a helpful assistant.");
    EXPECT_EQ(opt->version, "1.0");
    EXPECT_EQ(opt->role,    Role::USER);
    EXPECT_TRUE(opt->active);
}

TEST(SystemPromptManagerTest, GetPromptMissingRoleReturnsNullopt) {
    SystemPromptManager mgr;
    auto opt = mgr.getPrompt(Role::ADMIN);
    EXPECT_FALSE(opt.has_value());
}

TEST(SystemPromptManagerTest, GetPromptContentFallbackWhenMissing) {
    SystemPromptManager mgr;
    std::string content = mgr.getPromptContent(Role::ADMIN, "default fallback");
    EXPECT_EQ(content, "default fallback");
}

TEST(SystemPromptManagerTest, GetPromptContentReturnsStoredContent) {
    SystemPromptManager mgr;
    mgr.setPrompt(Role::ADMIN, "Admin prompt.", "1.0");
    EXPECT_EQ(mgr.getPromptContent(Role::ADMIN), "Admin prompt.");
}

TEST(SystemPromptManagerTest, OverwritePromptWithNewContent) {
    SystemPromptManager mgr;
    mgr.setPrompt(Role::USER, "Old content.", "1.0");
    mgr.setPrompt(Role::USER, "New content.", "2.0");

    auto opt = mgr.getPrompt(Role::USER);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->content, "New content.");
    EXPECT_EQ(opt->version, "2.0");
}

TEST(SystemPromptManagerTest, RemovePromptReturnsTrue) {
    SystemPromptManager mgr;
    mgr.setPrompt(Role::USER, "Prompt.", "1.0");
    EXPECT_TRUE(mgr.removePrompt(Role::USER));
    EXPECT_FALSE(mgr.getPrompt(Role::USER).has_value());
}

TEST(SystemPromptManagerTest, RemoveMissingPromptReturnsFalse) {
    SystemPromptManager mgr;
    EXPECT_FALSE(mgr.removePrompt(Role::ADMIN));
}

// ============================================================================
// Custom role CRUD
// ============================================================================

TEST(SystemPromptManagerTest, SetAndGetCustomPrompt) {
    SystemPromptManager mgr;
    mgr.setCustomPrompt("legal_reviewer", "Review for legal accuracy.", "1.0");

    auto opt = mgr.getCustomPrompt("legal_reviewer");
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->content,     "Review for legal accuracy.");
    EXPECT_EQ(opt->role,        Role::CUSTOM);
    EXPECT_EQ(opt->custom_role, "legal_reviewer");
}

TEST(SystemPromptManagerTest, GetCustomPromptMissingReturnsNullopt) {
    SystemPromptManager mgr;
    EXPECT_FALSE(mgr.getCustomPrompt("nonexistent").has_value());
}

TEST(SystemPromptManagerTest, GetCustomPromptContentFallback) {
    SystemPromptManager mgr;
    EXPECT_EQ(mgr.getCustomPromptContent("missing", "fallback"), "fallback");
}

TEST(SystemPromptManagerTest, RemoveCustomPromptReturnsTrue) {
    SystemPromptManager mgr;
    mgr.setCustomPrompt("role_x", "Content.", "1.0");
    EXPECT_TRUE(mgr.removeCustomPrompt("role_x"));
    EXPECT_FALSE(mgr.getCustomPrompt("role_x").has_value());
}

TEST(SystemPromptManagerTest, RemoveMissingCustomPromptReturnsFalse) {
    SystemPromptManager mgr;
    EXPECT_FALSE(mgr.removeCustomPrompt("ghost"));
}

// ============================================================================
// listPrompts
// ============================================================================

TEST(SystemPromptManagerTest, ListPromptsEmpty) {
    SystemPromptManager mgr;
    EXPECT_TRUE(mgr.listPrompts().empty());
}

TEST(SystemPromptManagerTest, ListPromptsIncludesBothBuiltinAndCustom) {
    SystemPromptManager mgr;
    mgr.setPrompt(Role::USER,  "User prompt.",  "1.0");
    mgr.setPrompt(Role::ADMIN, "Admin prompt.", "1.0");
    mgr.setCustomPrompt("auditor", "Audit prompt.", "1.0");

    auto list = mgr.listPrompts();
    EXPECT_EQ(list.size(), 3u);
}

// ============================================================================
// Rendering (context injection)
// ============================================================================

TEST(SystemPromptManagerTest, RenderPromptInjectsVariables) {
    SystemPromptManager mgr;
    mgr.setPrompt(Role::USER,
                  "You are an assistant for {product} version {version}.",
                  "1.0");

    std::string rendered = mgr.renderPrompt(Role::USER,
        {{"product", "ThemisDB"}, {"version", "1.5.0"}});

    EXPECT_EQ(rendered, "You are an assistant for ThemisDB version 1.5.0.");
}

TEST(SystemPromptManagerTest, RenderPromptMissingRoleReturnsEmpty) {
    SystemPromptManager mgr;
    EXPECT_TRUE(mgr.renderPrompt(Role::ADMIN).empty());
}

TEST(SystemPromptManagerTest, RenderPromptNoContextLeavesPlaceholdersIntact) {
    SystemPromptManager mgr;
    mgr.setPrompt(Role::USER, "Hello {name}.", "1.0");
    // No context supplied: placeholder stays as-is
    std::string rendered = mgr.renderPrompt(Role::USER);
    EXPECT_EQ(rendered, "Hello {name}.");
}

TEST(SystemPromptManagerTest, RenderCustomPromptInjectsVariables) {
    SystemPromptManager mgr;
    mgr.setCustomPrompt("billing", "Billing role for tenant {tenant_id}.", "1.0");

    std::string rendered = mgr.renderCustomPrompt("billing",
        {{"tenant_id", "acme-corp"}});

    EXPECT_EQ(rendered, "Billing role for tenant acme-corp.");
}

TEST(SystemPromptManagerTest, RenderCustomPromptMissingRoleReturnsEmpty) {
    SystemPromptManager mgr;
    EXPECT_TRUE(mgr.renderCustomPrompt("ghost").empty());
}

// ============================================================================
// SystemPrompt serialization
// ============================================================================

TEST(SystemPromptManagerTest, SystemPromptToJsonAndBack) {
    SystemPromptManager mgr;
    mgr.setPrompt(Role::ASSISTANT, "Be helpful.", "2.0");

    auto opt = mgr.getPrompt(Role::ASSISTANT);
    ASSERT_TRUE(opt.has_value());

    auto j   = opt->toJson();
    auto sp2 = SystemPrompt::fromJson(j);

    EXPECT_EQ(sp2.content, "Be helpful.");
    EXPECT_EQ(sp2.version, "2.0");
    EXPECT_EQ(sp2.role,    Role::ASSISTANT);
    EXPECT_TRUE(sp2.active);
}
