/*
 * ThemisDB | File: test_ai_plugin_generator.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

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
#include <limits>
#include <string>

using namespace themis::plugins::ai;

namespace {

AIPluginGenerator makeGenerator() {
    AIPluginGenerator::Config cfg;
    cfg.llm_endpoint = "http://test.invalid:18080";  // unreachable, clearly-named test fixture
    cfg.endpoint_invoke_fn = [](const std::string& endpoint,
                                const std::string& request_body,
                                long) -> themis::Result<std::string> {
        json request;
        try {
            request = json::parse(request_body);
        } catch (const std::exception& e) {
            return tl::unexpected(themis::Error(
                themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                std::string("Invalid test request JSON: ") + e.what()));
        }

        if (endpoint.empty() || !request.contains("description")) {
            return tl::unexpected(themis::Error(
                themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                "Invalid test request"));
        }
        return json{
            {"generated_plugin", {
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
        }.dump();
    };
    return AIPluginGenerator(cfg);
}

AIPluginGenerator makeGeneratorFromConfig(AIPluginGenerator::Config cfg) {
    if (cfg.llm_endpoint.empty()) {
        cfg.llm_endpoint = "http://mock-endpoint.invalid/generate";
    }
    if (!cfg.endpoint_invoke_fn) {
        cfg.endpoint_invoke_fn = [](const std::string&, const std::string&, long) -> themis::Result<std::string> {
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
        };
    }
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

// APG-06: generatePlugin with valid prompt returns generated plugin payload.
TEST(AIPluginGeneratorTest, APG06_GeneratePluginReturnsGeneratedPlugin) {
    auto gen = makeGenerator();
    auto result = gen.generatePlugin(validPrompt());
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ(result->manifest.name, "generated_test_plugin");
    EXPECT_EQ(result->manifest.version, "1.0.0");
    EXPECT_EQ(result->build_dependencies.size(), 1u);
    EXPECT_TRUE(result->passed_security_checks);
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

// APG-09: C1 runtime safety-gate accepts output when score meets configured threshold.
TEST(AIPluginGeneratorTest, APG09_C1SafetyGateAcceptsWhenThresholdMet) {
    AIPluginGenerator::Config cfg;
    cfg.enable_c1_cai_safety_gate = true;
    cfg.c1_min_safety_score = 0.80;
    cfg.c1_cai_eval_fn = [](const std::string&, const std::string&) -> themis::Result<double> {
        return 0.91;
    };

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_NE(result->security_report.find("C1 safety gate: pass"), std::string::npos);
}

// APG-10: C1 runtime safety-gate rejects output when score is below threshold.
TEST(AIPluginGeneratorTest, APG10_C1SafetyGateRejectsWhenThresholdMisses) {
    AIPluginGenerator::Config cfg;
    cfg.enable_c1_cai_safety_gate = true;
    cfg.c1_min_safety_score = 0.80;
    cfg.c1_cai_eval_fn = [](const std::string&, const std::string&) -> themis::Result<double> {
        return 0.42;
    };

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("C1 safety gate rejected"), std::string::npos);
}

// APG-11: C2 runtime telemetry hook receives local runtime metrics.
TEST(AIPluginGeneratorTest, APG11_C2FederatedTelemetryReceivesMetrics) {
    bool telemetry_called = false;
    json observed_metrics = json::object();

    AIPluginGenerator::Config cfg;
    cfg.enable_c2_federated_telemetry = true;
    cfg.c2_federated_telemetry_fn = [&](const json& local_metrics) -> themis::Result<void> {
        telemetry_called = true;
        observed_metrics = local_metrics;
        return {};
    };

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_TRUE(telemetry_called);
    EXPECT_TRUE(observed_metrics.contains("implementation_code_bytes"));
    EXPECT_TRUE(observed_metrics.contains("passed_security_checks"));
    EXPECT_NE(result->security_report.find("C2 federated telemetry"), std::string::npos);
}

// APG-12: C1 runtime safety-gate fails closed when callback is missing.
TEST(AIPluginGeneratorTest, APG12_C1SafetyGateMissingCallbackFailsClosed) {
    AIPluginGenerator::Config cfg;
    cfg.enable_c1_cai_safety_gate = true;

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("c1_cai_eval_fn is not configured"), std::string::npos);
}

// APG-13: C1 runtime safety-gate rejects non-finite evaluator outputs.
TEST(AIPluginGeneratorTest, APG13_C1SafetyGateRejectsNonFiniteScore) {
    AIPluginGenerator::Config cfg;
    cfg.enable_c1_cai_safety_gate = true;
    cfg.c1_cai_eval_fn = [](const std::string&, const std::string&) -> themis::Result<double> {
        return std::numeric_limits<double>::quiet_NaN();
    };

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("non-finite"), std::string::npos);
}

// APG-14: C2 telemetry hook fails closed when callback is missing.
TEST(AIPluginGeneratorTest, APG14_C2TelemetryMissingCallbackFailsClosed) {
    AIPluginGenerator::Config cfg;
    cfg.enable_c2_federated_telemetry = true;

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("c2_federated_telemetry_fn is not configured"), std::string::npos);
}

// APG-15: C2 telemetry callback failures are propagated.
TEST(AIPluginGeneratorTest, APG15_C2TelemetryFailurePropagates) {
    AIPluginGenerator::Config cfg;
    cfg.enable_c2_federated_telemetry = true;
    cfg.c2_federated_telemetry_fn = [](const json&) -> themis::Result<void> {
        return tl::unexpected(themis::Error(
            themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
            "telemetry transport offline"));
    };

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("telemetry transport offline"), std::string::npos);
}

// APG-16: C2 telemetry metrics include C1 score when both hooks are enabled.
TEST(AIPluginGeneratorTest, APG16_C2TelemetryIncludesC1SafetyScoreWhenEnabled) {
    json observed_metrics = json::object();

    AIPluginGenerator::Config cfg;
    cfg.enable_c1_cai_safety_gate = true;
    cfg.c1_min_safety_score = 0.8;
    cfg.c1_cai_eval_fn = [](const std::string&, const std::string&) -> themis::Result<double> {
        return 0.88;
    };
    cfg.enable_c2_federated_telemetry = true;
    cfg.c2_federated_telemetry_fn = [&](const json& local_metrics) -> themis::Result<void> {
        observed_metrics = local_metrics;
        return {};
    };

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_TRUE(observed_metrics.contains("c1_safety_score"));
    EXPECT_DOUBLE_EQ(observed_metrics.at("c1_safety_score").get<double>(), 0.88);
}

// APG-17: validatePrompt rejects invalid capability tokens.
TEST(AIPluginGeneratorTest, APG17_ValidatePromptRejectsInvalidCapabilityToken) {
    auto gen = makeGenerator();
    PluginGenerationPrompt p = validPrompt();
    p.required_capabilities = {"vector search"};

    auto result = gen.validatePrompt(p);
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("required_capabilities"), std::string::npos);
}

// APG-18: validatePrompt rejects duplicate dependency tokens.
TEST(AIPluginGeneratorTest, APG18_ValidatePromptRejectsDuplicateDependencies) {
    auto gen = makeGenerator();
    PluginGenerationPrompt p = validPrompt();
    p.dependencies = {"fmt", "fmt"};

    auto result = gen.validatePrompt(p);
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("dependencies contains duplicate"), std::string::npos);
}

// APG-19: generatePlugin fails closed when endpoint is not allow-listed.
TEST(AIPluginGeneratorTest, APG19_GeneratePluginRejectsEndpointOutsideAllowList) {
    AIPluginGenerator::Config cfg;
    cfg.llm_endpoint = "http://mock-endpoint.invalid/generate";
    cfg.allowed_llm_endpoints = {"http://allowed-endpoint.invalid/generate"};
    cfg.endpoint_invoke_fn = [](const std::string&, const std::string&, long) -> themis::Result<std::string> {
        return R"({"implementation_code":"int generated() { return 1; }"})";
    };

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("allow-list"), std::string::npos);
}

// APG-20: generatePlugin rejects oversized endpoint responses.
TEST(AIPluginGeneratorTest, APG20_GeneratePluginRejectsOversizedEndpointResponse) {
    AIPluginGenerator::Config cfg;
    cfg.max_response_body_bytes = 32;
    cfg.endpoint_invoke_fn = [](const std::string&, const std::string&, long) -> themis::Result<std::string> {
        return std::string(256, 'x');
    };

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("response size limit"), std::string::npos);
}

// APG-21: generatePlugin rejects oversized serialized requests.
TEST(AIPluginGeneratorTest, APG21_GeneratePluginRejectsOversizedSerializedRequest) {
    AIPluginGenerator::Config cfg;
    cfg.max_request_body_bytes = 16;
    cfg.endpoint_invoke_fn = [](const std::string&, const std::string&, long) -> themis::Result<std::string> {
        return R"({"implementation_code":"int generated() { return 1; }"})";
    };

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("request size limit"), std::string::npos);
}

// APG-22: sandbox gate fails closed when callback is missing.
TEST(AIPluginGeneratorTest, APG22_SandboxGateMissingCallbackFailsClosed) {
    AIPluginGenerator::Config cfg;
    cfg.enable_sandbox_gate = true;

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("sandbox_verify_fn is not configured"), std::string::npos);
    EXPECT_EQ(gen.getStats().sandbox_rejections, 1u);
}

// APG-23: sandbox gate propagates callback rejections and counts them.
TEST(AIPluginGeneratorTest, APG23_SandboxGateFailurePropagatesAndCountsRejection) {
    AIPluginGenerator::Config cfg;
    cfg.enable_sandbox_gate = true;
    cfg.sandbox_verify_fn = [](const GeneratedPlugin&) -> themis::Result<void> {
        return tl::unexpected(themis::Error(
            themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
            "sandbox policy denied"));
    };

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("sandbox policy denied"), std::string::npos);
    EXPECT_EQ(gen.getStats().sandbox_rejections, 1u);
}

// APG-24: sandbox gate appends pass status when callback succeeds.
TEST(AIPluginGeneratorTest, APG24_SandboxGateSuccessAppendsSecurityReport) {
    AIPluginGenerator::Config cfg;
    cfg.enable_sandbox_gate = true;
    cfg.sandbox_verify_fn = [](const GeneratedPlugin&) -> themis::Result<void> {
        return {};
    };

    auto gen = makeGeneratorFromConfig(std::move(cfg));
    auto result = gen.generatePlugin(validPrompt());
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_NE(result->security_report.find("Sandbox verification: pass"), std::string::npos);
    EXPECT_EQ(gen.getStats().successes, 1u);
}

// APG-25: stats counters track validation, parse, safety, and success outcomes.
TEST(AIPluginGeneratorTest, APG25_StatsCountersTrackOutcomes) {
    auto validation_gen = makeGenerator();
    auto invalid_prompt = validPrompt();
    invalid_prompt.description.clear();
    auto validation_result = validation_gen.generatePlugin(invalid_prompt);
    EXPECT_FALSE(validation_result.has_value());
    auto validation_stats = validation_gen.getStats();
    EXPECT_EQ(validation_stats.validation_errors, 1u);
    EXPECT_EQ(validation_stats.successes, 0u);

    auto parse_gen = makeGeneratorWithEndpointFn(
        [](const std::string&, const std::string&, long) -> themis::Result<std::string> {
            return std::string("not-json");
        });
    auto parse_result = parse_gen.generatePlugin(validPrompt());
    EXPECT_FALSE(parse_result.has_value());
    EXPECT_EQ(parse_gen.getStats().parse_errors, 1u);

    AIPluginGenerator::Config safety_cfg;
    safety_cfg.enable_c1_cai_safety_gate = true;
    safety_cfg.c1_min_safety_score = 0.9;
    safety_cfg.c1_cai_eval_fn = [](const std::string&, const std::string&) -> themis::Result<double> {
        return 0.1;
    };
    auto safety_gen = makeGeneratorFromConfig(std::move(safety_cfg));
    auto safety_result = safety_gen.generatePlugin(validPrompt());
    EXPECT_FALSE(safety_result.has_value());
    EXPECT_EQ(safety_gen.getStats().safety_rejections, 1u);

    auto success_gen = makeGenerator();
    auto success_result = success_gen.generatePlugin(validPrompt());
    ASSERT_TRUE(success_result.has_value()) << success_result.error().message();
    EXPECT_EQ(success_gen.getStats().successes, 1u);
}
