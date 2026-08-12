/**
 * @file llm_ingestion_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB — LlmIngestionBridge implementation
 *
 * Bridges ingestion::ITextGenerationBackend to LLMPluginManager::generate().
 * This file is the ONLY place in the codebase where llm/ and ingestion/ are
 * coupled; see include/llm/llm_ingestion_bridge.h for the design rationale.
 */

#include "llm/llm_ingestion_bridge.h"
#include <stdexcept>
#include <spdlog/spdlog.h>
#include "utils/logger.h"

namespace themis {
namespace llm {

std::string LlmIngestionBridge::generate(const std::string& prompt,
                                          int                max_tokens,
                                          double             temperature,
                                          const std::string& lora_adapter) {
    InferenceRequest req;
    req.prompt       = prompt;
    req.model_id     = "default";
    req.max_tokens   = max_tokens;
    req.temperature  = static_cast<float>(temperature);
    req.grammar_type = "json"; // constrain output to valid JSON for legal extraction

    if (!lora_adapter.empty()) {
        req.lora_adapter_id = lora_adapter;
    }

    try {
        auto response = LLMPluginManager::instance().generate(req);
        return response.text;
    } catch (const std::exception& e) {
        spdlog::error("[LlmIngestionBridge] generate() failed: {}", e.what());
        return {};
    }
}

bool LlmIngestionBridge::isAvailable() const {
    try {
        const auto plugins = LLMPluginManager::instance().listPlugins();
        return !plugins.empty();
    } catch (...) {
        THEMIS_WARN("llm_ingestion_bridge: unhandled exception caught");
        return false;
    }
}

std::string LlmIngestionBridge::description() const {
    try {
        const auto plugins = LLMPluginManager::instance().listPlugins();
        if (plugins.empty()) {
            return "LlmIngestionBridge (no plugin loaded)";
        }
        return "LlmIngestionBridge → LLMPluginManager/" + plugins.front();
    } catch (...) {
        THEMIS_WARN("llm_ingestion_bridge: unhandled exception caught");
        return "LlmIngestionBridge (unavailable)";
    }
}

} // namespace llm
} // namespace themis

