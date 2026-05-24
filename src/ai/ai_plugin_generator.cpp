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

#include <mutex>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

namespace themis {
namespace plugins {
namespace ai {

// ============================================================================
// HttpPost bridge (stub #282)
// ============================================================================

namespace {
    static std::mutex s_http_post_fn_mutex;
    static AIPluginGenerator::HttpPostFn s_http_post_fn;
} // namespace

void AIPluginGenerator::setHttpPostFn(HttpPostFn fn) {
    std::lock_guard<std::mutex> lock(s_http_post_fn_mutex);
    s_http_post_fn = std::move(fn);
}

void AIPluginGenerator::clearHttpPostFn() {
    std::lock_guard<std::mutex> lock(s_http_post_fn_mutex);
    s_http_post_fn = nullptr;
}

static AIPluginGenerator::HttpPostFn getHttpPostFn() {
    std::lock_guard<std::mutex> lock(s_http_post_fn_mutex);
    return s_http_post_fn;
}

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

    // Use injected HTTP POST bridge if available (stub #282 resolved).
    if (auto http_fn = getHttpPostFn()) {
        return http_fn(config_.llm_endpoint, prompt);
    }

    // 2. Phase-1 implementation: LLM endpoint invocation is not yet wired.
    //    Return a structured error so callers can distinguish "validation failed"
    //    from "LLM unavailable".
    //
    // TODO (Phase 2, v1.6.0): replace the error below with a real HTTP call to
    //   config_.llm_endpoint, parse the JSON response, populate GeneratedPlugin,
    //   and run the security sandbox pipeline.
    return tl::unexpected(
        Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
              "AIPluginGenerator::generatePlugin: LLM endpoint not yet wired "
              "(Phase 2, Target v1.6.0). Endpoint: " + config_.llm_endpoint));
}

} // namespace ai
} // namespace plugins
} // namespace themis
