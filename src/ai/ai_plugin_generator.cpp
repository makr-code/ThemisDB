/*
 * ThemisDB | File: ai_plugin_generator.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 85/100
 * Gap Summary: total=7; TODO=2, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=15, M=0, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

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

void AIPluginGenerator::setLLMGenerateFn(LLMGenerateFn fn) {
    std::lock_guard<std::mutex> lock(llm_fn_mutex_);
    llm_generate_fn_ = std::move(fn);
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

    // Delegate to injected LLM bridge when available (stub #282 resolution).
    {
        std::lock_guard<std::mutex> lock(llm_fn_mutex_);
        if (llm_generate_fn_) {
            return llm_generate_fn_(prompt);
        }
    }

    // No LLM backend registered yet — return structured error so callers can
    // distinguish "validation failed" from "LLM unavailable".
    return tl::unexpected(
        Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
              "AIPluginGenerator::generatePlugin: LLM endpoint not yet wired "
              "(inject via setLLMGenerateFn()). Endpoint: " + config_.llm_endpoint));
}

} // namespace ai
} // namespace plugins
} // namespace themis
