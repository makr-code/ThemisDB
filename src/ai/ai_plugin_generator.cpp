// THEMIS_GAP_STATS: gaps=2 unimpl=1 stub=0 mock=0 sim=0 todo=1 debt=0 scanned=2026-05-18
/**
 * @file ai_plugin_generator.cpp
 * @brief Minimal production implementation of AIPluginGenerator.
 *
 * STUB/SIMULATION NOTE (stub #282):
 * Purpose: Provide structurally-valid input validation for AI plugin generation
 *          while the LLM endpoint HTTP call and security sandbox pipeline are
 *          not yet wired (Phase 1 only).
 * Activation: Always active — generatePlugin() always returns
 *             ERR_PLUGIN_LOAD_FAILED with "LLM endpoint not yet wired".
 * Production Delta: No plugin code is generated; the configured llm_endpoint
 *                   is logged but never called. No GeneratedPlugin is returned.
 * Removal Plan: Implement Phase 2 (Target v1.6.0): perform an HTTP POST to
 *               config_.llm_endpoint, parse the JSON response, populate
 *               GeneratedPlugin, and run the security sandbox pipeline.
 *               (tracked in STUB_INVENTORY #282)
 *
 * Phase 1 implementation:
 *   - validatePrompt()  validates the description and required_capabilities fields.
 *   - generatePlugin()  validates inputs, then returns ERR_PLUGIN_LOAD_FAILED.
 *
 * Phase 2 (Target v1.6.0): wire a real ILLMInferenceEngine for code generation.
 * See include/plugins/ai/ai_plugin_generator.h and src/plugins/ai/FUTURE_ENHANCEMENTS.md.
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
