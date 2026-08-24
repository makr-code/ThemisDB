/**
 * @file test_llm_wiki_llm_integration_focused.cpp
 * @brief Group EG — Edition gate and LLM integration tests for llm_wiki.
 *
 * Verifies the edition-gate logic that controls access to LLM Wiki features:
 *   EG1-EG3  getCurrentEdition() and isLLMWikiEnabled() under Community build
 *   EG4-EG5  enforcePluginGate() returns PermissionDenied on Community edition
 *   EG6      isLLMWikiFeatureEnabled() returns false for disabled features
 *   EG7      Status types carry correct codes
 */

#include <gtest/gtest.h>
#include "llm_wiki/edition_gate.h"
#include "llm_wiki/llm_wiki_plugin_interface.h"

using namespace themis::llm_wiki;
using namespace themis::plugins::llm_wiki;

// ── Group EG — Edition Gate ───────────────────────────────────────────────────

// EG1: getCurrentEdition() returns a valid Edition without crashing
TEST(LlmWikiLlmIntegrationFocusedTests, EG1_GetCurrentEdition_DoesNotCrash) {
    ASSERT_NO_THROW({
        auto edition = getCurrentEdition();
        (void)edition;
    });
}

// EG2: isLLMWikiEnabled() returns bool without crashing
TEST(LlmWikiLlmIntegrationFocusedTests, EG2_IsLLMWikiEnabled_DoesNotCrash) {
    ASSERT_NO_THROW({
        bool enabled = isLLMWikiEnabled();
        (void)enabled;
    });
}

// EG3: In a Community build (default CI), LLM Wiki is disabled
// Note: This test is conditioned on the build-time flag; it documents expected
// Community-edition behavior. Enterprise builds may skip this assertion.
TEST(LlmWikiLlmIntegrationFocusedTests, EG3_CommunityBuild_LLMWikiIsDisabled) {
#ifndef THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED
    EXPECT_FALSE(isLLMWikiEnabled());
#else
    GTEST_SKIP() << "Enterprise build: skipping Community-disabled check";
#endif
}

// EG4: enforcePluginGate() returns PermissionDenied on Community edition
TEST(LlmWikiLlmIntegrationFocusedTests, EG4_CommunityBuild_EnforcePluginGate_PermissionDenied) {
#ifndef THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED
    auto status = enforcePluginGate("test_operation");
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code, PluginStatus::Code::PermissionDenied);
#else
    GTEST_SKIP() << "Enterprise build: skipping Community gate check";
#endif
}

// EG5: enforceFeatureGate() returns PermissionDenied for any feature on Community
TEST(LlmWikiLlmIntegrationFocusedTests, EG5_CommunityBuild_EnforceFeatureGate_PermissionDenied) {
#ifndef THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED
    auto status = enforceFeatureGate("llm_wiki_wikipedia");
    EXPECT_FALSE(status.ok());
#else
    GTEST_SKIP() << "Enterprise build: skipping Community feature gate check";
#endif
}

// EG6: isLLMWikiFeatureEnabled() returns false for disabled sub-features
TEST(LlmWikiLlmIntegrationFocusedTests, EG6_DisabledSubFeature_IsNotEnabled) {
#ifndef THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED
    EXPECT_FALSE(isLLMWikiFeatureEnabled("llm_wiki_wikipedia"));
    EXPECT_FALSE(isLLMWikiFeatureEnabled("nonexistent_feature"));
#else
    GTEST_SKIP() << "Enterprise build: skipping feature gate check";
#endif
}

// EG7: Status types carry correct codes
TEST(LlmWikiLlmIntegrationFocusedTests, EG7_StatusTypes_CarryCorrectCodes) {
    auto ok = PluginStatus::Ok();
    EXPECT_TRUE(ok.ok());
    EXPECT_EQ(ok.code, PluginStatus::Code::Ok);

    auto err = PluginStatus::Error("test error");
    EXPECT_FALSE(err.ok());
    EXPECT_EQ(err.code, PluginStatus::Code::Error);
    EXPECT_FALSE(err.message.empty());

    auto denied = PluginStatus::PermissionDenied("access denied");
    EXPECT_FALSE(denied.ok());
    EXPECT_EQ(denied.code, PluginStatus::Code::PermissionDenied);
}

// EG8: Edition enum covers all documented editions
TEST(LlmWikiLlmIntegrationFocusedTests, EG8_EditionEnum_CoversAllEditions) {
    // Each variant must be constructible without UB
    auto community   = Edition::Community;
    auto minimal     = Edition::Minimal;
    auto enterprise  = Edition::Enterprise;
    auto hyperscaler = Edition::Hyperscaler;
    auto military    = Edition::Military;

    // Verify ordering is stable
    EXPECT_NE(community,  enterprise);
    EXPECT_NE(community,  hyperscaler);
    EXPECT_NE(community,  military);
    EXPECT_NE(minimal,    enterprise);
    EXPECT_NE(hyperscaler, military);
}
