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
 *   APG-06  generatePlugin with a valid prompt returns a structured error
 *           (Phase-1: LLM endpoint not yet wired).
 */

#include <gtest/gtest.h>
#include "ai/ai_plugin_generator.h"
#include <string>

using namespace themis::plugins::ai;

namespace {

AIPluginGenerator makeGenerator() {
    AIPluginGenerator::Config cfg;
    cfg.llm_endpoint = "http://test.invalid:18080";  // unreachable, clearly-named test fixture
    return AIPluginGenerator(cfg);
}

AIPluginGenerator makeGeneratorWithEndpointFn(AIPluginGenerator::EndpointInvokeFn fn) {
    AIPluginGenerator::Config cfg;
    cfg.llm_endpoint = "http://mock-endpoint.invalid/generate";
    cfg.endpoint_invoke_fn = std::move(fn);
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

// APG-06: generatePlugin with valid prompt returns Phase-1 "not wired" error.
TEST(AIPluginGeneratorTest, APG06_GeneratePluginReturnsPhase1Error) {
    auto gen = makeGenerator();
    auto result = gen.generatePlugin(validPrompt());
    // Unreachable endpoint without a test callback should fail with a structured error.
    EXPECT_FALSE(result.has_value())
        << "generatePlugin should return error when endpoint cannot be reached";
    EXPECT_FALSE(result.error().message().empty());
}

// APG-07: generatePlugin returns a parsed GeneratedPlugin when endpoint callback succeeds.
TEST(AIPluginGeneratorTest, APG07_GeneratePluginParsesEndpointResponse) {
    auto gen = makeGeneratorWithEndpointFn(
        [](const std::string&, const std::string&, long) -> themis::Result<std::string> {
            json payload = {
                {"name", "generated_demo_plugin"},
                {"version", "1.2.3"},
                {"description", "Generated from test callback"},
                {"header_code", "// header"},
                {"implementation_code", "int generated() { return 42; }"},
                {"test_code", "// tests"},
                {"cmake_code", "# cmake"},
                {"build_dependencies", json::array({"fmt", "spdlog"})},
                {"passed_security_checks", true},
                {"security_report", "ok"}
            };
            return payload.dump();
        });

    auto result = gen.generatePlugin(validPrompt());
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ(result->manifest.name, "generated_demo_plugin");
    EXPECT_EQ(result->manifest.version, "1.2.3");
    EXPECT_EQ(result->implementation_code, "int generated() { return 42; }");
    EXPECT_EQ(result->build_dependencies.size(), 2u);
    EXPECT_TRUE(result->passed_security_checks);
}

// APG-08: endpoint responses missing implementation_code are rejected.
TEST(AIPluginGeneratorTest, APG08_GeneratePluginRejectsMissingImplementationCode) {
    auto gen = makeGeneratorWithEndpointFn(
        [](const std::string&, const std::string&, long) -> themis::Result<std::string> {
            json payload = {
                {"name", "incomplete_plugin"},
                {"header_code", "// header only"}
            };
            return payload.dump();
        });

    auto result = gen.generatePlugin(validPrompt());
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("implementation_code"), std::string::npos);
}
