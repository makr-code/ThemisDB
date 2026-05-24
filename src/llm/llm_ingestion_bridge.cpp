/*
 * ThemisDB | File: llm_ingestion_bridge.cpp | Version: 0.0.2 | Last Modified: 2026-05-18 20:49:49
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 69
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=18 | delta=15 | status=divergent
 * External Severity (v3): C=0, H=12, M=6
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
        return "LlmIngestionBridge (unavailable)";
    }
}

} // namespace llm
} // namespace themis
