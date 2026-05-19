// THEMIS_GAP_STATS: gaps=1 unimpl=1 stub=0 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_ingestion_bridge.cpp                           ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-04-15 18:49:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     83                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • db7df90e31  2026-04-15  feat(ingestion): Google Benchmarks QJ01–QJ11 + SoC/OOP do... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    } catch (const std::exception&) {
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
    } catch (const std::exception&) {
        return "LlmIngestionBridge (unavailable)";
    }
}

} // namespace llm
} // namespace themis
