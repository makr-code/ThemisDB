/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_prompt_manager_validation.cpp                 ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-04-14 11:49:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     142                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_prompt_manager_validation.cpp
 * @brief Tests for PromptManager template validation (issue 2.1)
 */

#include <gtest/gtest.h>
#include "prompt_engineering/prompt_manager.h"

using namespace themis::prompt_engineering;

// ============================================================================
// validateTemplate
// ============================================================================

TEST(PromptManagerValidationTest, ValidTemplate) {
    PromptManager::PromptTemplate t;
    t.name    = "summarize";
    t.version = "v1";
    t.content = "Summarize: {text}";
    t.description = "Summarizes text";

    auto result = PromptManager::validateTemplate(t);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST(PromptManagerValidationTest, MissingName) {
    PromptManager::PromptTemplate t;
    t.name    = "";
    t.version = "v1";
    t.content = "Hello {world}";

    auto result = PromptManager::validateTemplate(t);
    EXPECT_FALSE(result.valid);
    ASSERT_EQ(result.errors.size(), 1u);
    EXPECT_NE(result.errors[0].find("name"), std::string::npos);
}

TEST(PromptManagerValidationTest, MissingContent) {
    PromptManager::PromptTemplate t;
    t.name    = "classify";
    t.version = "v2";
    t.content = "";

    auto result = PromptManager::validateTemplate(t);
    EXPECT_FALSE(result.valid);
    ASSERT_EQ(result.errors.size(), 1u);
    EXPECT_NE(result.errors[0].find("content"), std::string::npos);
}

TEST(PromptManagerValidationTest, MissingVersion) {
    PromptManager::PromptTemplate t;
    t.name    = "translate";
    t.version = "";
    t.content = "Translate: {text}";

    auto result = PromptManager::validateTemplate(t);
    EXPECT_FALSE(result.valid);
    ASSERT_EQ(result.errors.size(), 1u);
    EXPECT_NE(result.errors[0].find("version"), std::string::npos);
}

TEST(PromptManagerValidationTest, MultipleErrors) {
    PromptManager::PromptTemplate t;
    // All required fields missing
    auto result = PromptManager::validateTemplate(t);
    EXPECT_FALSE(result.valid);
    // name, content, version all missing
    EXPECT_EQ(result.errors.size(), 3u);
}

TEST(PromptManagerValidationTest, InvalidMetadata) {
    PromptManager::PromptTemplate t;
    t.name    = "test";
    t.version = "1";
    t.content = "Hello";
    t.metadata = nlohmann::json::array();  // Must be object, not array

    auto result = PromptManager::validateTemplate(t);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.errors.size(), 1u);
    EXPECT_NE(result.errors[0].find("metadata"), std::string::npos);
}

TEST(PromptManagerValidationTest, MissingDescriptionWarning) {
    PromptManager::PromptTemplate t;
    t.name    = "test";
    t.version = "1";
    t.content = "Hello";
    // description intentionally left empty

    auto result = PromptManager::validateTemplate(t);
    EXPECT_TRUE(result.valid);        // Still valid
    EXPECT_EQ(result.errors.size(), 0u);
    EXPECT_EQ(result.warnings.size(), 1u);  // But has a warning
}

// ============================================================================
// createTemplate with validation guard
// ============================================================================

TEST(PromptManagerValidationTest, CreateWithValidTemplate) {
    PromptManager pm;
    PromptManager::PromptTemplate t;
    t.name    = "valid";
    t.version = "1";
    t.content = "Hello {name}";

    auto created = pm.createTemplate(t);
    EXPECT_FALSE(created.id.empty());
}

TEST(PromptManagerValidationTest, CreateWithInvalidTemplate_ReturnsEmptyId) {
    PromptManager pm;
    PromptManager::PromptTemplate t;
    // name and content empty → validation error

    auto created = pm.createTemplate(t);
    EXPECT_TRUE(created.id.empty());  // Sentinel (invalid)
}
