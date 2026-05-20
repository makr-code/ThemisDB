/**
 * @file test_ai_plugin_generator.cpp
 * @brief Minimum coverage tests for AIPluginGenerator (UNUSED_FUNCTIONS_REPORT KEEP).
 *
 * Acceptance criteria:
 *   APG-01  Construction with default Config does not throw.
 *   APG-02  validatePrompt with empty description returns an error.
 *   APG-03  validatePrompt with a valid description returns success.
 *   APG-04  validatePrompt with oversized description (>8192 chars) returns error.
 *   APG-05  generatePlugin propagates validatePrompt errors (empty description).
 *   APG-06  generatePlugin with a valid prompt returns generated plugin payload.
 */

#include <gtest/gtest.h>
#include "ai/ai_plugin_generator.h"
#include <string>

using namespace themis::plugins::ai;

namespace {

AIPluginGenerator makeGenerator() {
    AIPluginGenerator::Config cfg;
    cfg.llm_endpoint = "http://test.invalid:18080";  // unreachable, clearly-named test fixture
    cfg.endpoint_invoker = [](const std::string& endpoint, const nlohmann::json& request) -> Result<nlohmann::json> {
        if (endpoint.empty() || !request.contains("description")) {
            return tl::unexpected(Error(
                themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                "Invalid test request"));
        }
        return nlohmann::json{
            {"plugin", {
                {"name", "generated_test_plugin"},
                {"version", "1.0.0"},
                {"description", request.value("description", std::string{})},
                {"type", static_cast<int>(themis::plugins::PluginType::BLOB_STORAGE)},
                {"header_code", "#pragma once\nclass GeneratedPlugin {};"},
                {"implementation_code", "void generated_plugin_entry() {}"},
                {"test_code", "TEST(GeneratedPlugin, Smoke) { SUCCEED(); }"},
                {"cmake_code", "add_library(generated_plugin SHARED generated_plugin.cpp)"},
                {"build_dependencies", nlohmann::json::array({"fmt"})},
                {"passed_security_checks", true},
                {"security_report", "ok"}
            }}
        };
    };
    return AIPluginGenerator(cfg);
}

PluginGenerationPrompt validPrompt() {
    PluginGenerationPrompt p;
    p.description   = "Generate a simple logging storage plugin for ThemisDB.";
    p.type          = themis::plugins::PluginType::BLOB_STORAGE;
    p.llm_model     = LLMModel::CODE_LLAMA;
    p.security_level = SecurityLevel::HIGH;
    return p;
}

} // namespace

// APG-01: Construction does not throw.
TEST(AIPluginGeneratorTest, APG01_ConstructionDoesNotThrow) {
    EXPECT_NO_THROW({
        auto gen = makeGenerator();
        (void)gen;
    });
}

// APG-02: validatePrompt with empty description returns error.
TEST(AIPluginGeneratorTest, APG02_ValidatePromptEmptyDescriptionFails) {
    auto gen = makeGenerator();
    PluginGenerationPrompt p = validPrompt();
    p.description = "";
    auto result = gen.validatePrompt(p);
    EXPECT_FALSE(result.has_value()) << "Empty description should produce an error";
    EXPECT_FALSE(result.error().message().empty());
}

// APG-03: validatePrompt with a valid description returns success.
TEST(AIPluginGeneratorTest, APG03_ValidatePromptValidDescriptionSucceeds) {
    auto gen = makeGenerator();
    auto result = gen.validatePrompt(validPrompt());
    EXPECT_TRUE(result.has_value()) << "Valid prompt should pass validation: "
                                    << (result ? "" : result.error().message());
}

// APG-04: validatePrompt rejects descriptions longer than 8192 chars.
TEST(AIPluginGeneratorTest, APG04_ValidatePromptOversizedDescriptionFails) {
    auto gen = makeGenerator();
    PluginGenerationPrompt p = validPrompt();
    p.description = std::string(8193, 'x');
    auto result = gen.validatePrompt(p);
    EXPECT_FALSE(result.has_value()) << "Oversized description should fail validation";
}

// APG-05: generatePlugin propagates validatePrompt error for empty description.
TEST(AIPluginGeneratorTest, APG05_GeneratePluginPropagatesValidationError) {
    auto gen = makeGenerator();
    PluginGenerationPrompt p = validPrompt();
    p.description = "";
    auto result = gen.generatePlugin(p);
    EXPECT_FALSE(result.has_value()) << "generatePlugin must propagate validation errors";
}

// APG-06: generatePlugin with valid prompt returns generated plugin payload.
TEST(AIPluginGeneratorTest, APG06_GeneratePluginReturnsGeneratedPlugin) {
    auto gen = makeGenerator();
    auto result = gen.generatePlugin(validPrompt());
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ(result->manifest.name, "generated_test_plugin");
    EXPECT_FALSE(result->header_code.empty());
    EXPECT_FALSE(result->implementation_code.empty());
    EXPECT_TRUE(result->passed_security_checks);
}
