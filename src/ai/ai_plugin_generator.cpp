// THEMIS_GAP_STATS: gaps=0 unimpl=0 stub=0 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-20
/**
 * @file ai_plugin_generator.cpp
 * @brief Production implementation of AIPluginGenerator — Phase 2 wired via LlmHttpPostFn.
 *
 * Stub #282 resolved: generatePlugin() now performs an HTTP POST to
 * config_.llm_endpoint via an injected LlmHttpPostFn when one is registered.
 * Without an injected function the call returns ERR_PLUGIN_LOAD_FAILED, making
 * the "Phase 2 not available" state explicit instead of silent.
 *
 * Phase 2 flow:
 *   1. validatePrompt() — input validation (unchanged from Phase 1).
 *   2. Build a JSON request from the PluginGenerationPrompt.
 *   3. POST to config_.llm_endpoint via the injected LlmHttpPostFn.
 *   4. Parse the JSON response; populate GeneratedPlugin.
 *   5. Return ERR_PLUGIN_LOAD_FAILED on network or parse errors.
 *
 * To enable code generation, inject a transport at startup:
 *   generator.setLlmHttpPostFn([](const std::string& url, const std::string& body) {
 *       return myHttpClient.post(url, body);
 *   });
 */

#include "ai/ai_plugin_generator.h"
#include "utils/error_registry.h"
#include "utils/expected.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

namespace themis {
namespace plugins {
namespace ai {

AIPluginGenerator::AIPluginGenerator(const Config& config)
    : config_(config)
{}

AIPluginGenerator::~AIPluginGenerator() = default;

void AIPluginGenerator::setLlmHttpPostFn(LlmHttpPostFn fn) {
    llm_http_post_fn_ = std::move(fn);
}

Result<void> AIPluginGenerator::validatePrompt(const PluginGenerationPrompt& prompt)
{
    if (prompt.description.empty()) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: prompt description must not be empty"));
    }
    if (prompt.description.size() > 8192u) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: prompt description exceeds 8192-character limit"));
    }
    return {};  // success
}

Result<GeneratedPlugin> AIPluginGenerator::generatePlugin(
    const PluginGenerationPrompt& prompt)
{
    // 1. Validate inputs first.
    auto vr = validatePrompt(prompt);
    if (!vr) {
        return tl::unexpected(vr.error());
    }

    spdlog::debug("[AIPluginGenerator] generatePlugin: description='{}' endpoint='{}'",
                  prompt.description.substr(0, 80), config_.llm_endpoint);

    // 2. Phase 2: perform HTTP POST via injected LlmHttpPostFn.
    if (!llm_http_post_fn_.has_value()) {
        // No transport injected — Phase 2 unavailable in this build configuration.
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator::generatePlugin: no LlmHttpPostFn injected. "
                  "Call setLlmHttpPostFn() to enable plugin generation. "
                  "Configured endpoint: " + config_.llm_endpoint));
    }

    // 3. Build the JSON request.
    nlohmann::json req;
    req["description"]           = prompt.description;
    req["required_capabilities"] = prompt.required_capabilities;
    req["dependencies"]          = prompt.dependencies;
    req["generate_tests"]        = prompt.generate_tests;
    req["generate_docs"]         = prompt.generate_docs;
    const std::string body = req.dump();

    // 4. Call the injected transport.
    std::string response;
    try {
        response = (*llm_http_post_fn_)(config_.llm_endpoint, body);
    } catch (const std::exception& ex) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  std::string("AIPluginGenerator::generatePlugin: HTTP POST to '") +
                  config_.llm_endpoint + "' failed: " + ex.what()));
    }

    // 5. Parse the JSON response.
    auto parsed = nlohmann::json::parse(response, nullptr, /*throw_on_error=*/false);
    if (parsed.is_discarded()) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator::generatePlugin: LLM endpoint returned invalid JSON"));
    }

    const std::string code = parsed.value("implementation_code",
                                          parsed.value("code", std::string{}));
    if (code.empty()) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator::generatePlugin: LLM response missing 'implementation_code' field"));
    }

    GeneratedPlugin plugin;
    plugin.header_code          = parsed.value("header_code", std::string{});
    plugin.implementation_code  = code;
    plugin.test_code            = parsed.value("test_code", std::string{});
    plugin.cmake_code           = parsed.value("cmake_code", std::string{});
    plugin.security_report      = parsed.value("security_report", std::string{});
    plugin.passed_security_checks = parsed.value("passed_security_checks", false);
    if (parsed.contains("build_dependencies") && parsed["build_dependencies"].is_array()) {
        plugin.build_dependencies = parsed["build_dependencies"].get<std::vector<std::string>>();
    }

    spdlog::info("[AIPluginGenerator] generatePlugin: plugin generated successfully from '{}'",
                 config_.llm_endpoint);
    return plugin;
}

} // namespace ai
} // namespace plugins
} // namespace themis
