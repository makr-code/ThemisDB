// THEMIS_GAP_STATS: gaps=3 unimpl=3 stub=0 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_aql_embedding_bridge.cpp                       ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-22                                         ║
  Author:          copilot-swe-agent                                  ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "aql/llm_aql_embedding_bridge.h"
#include "aql/llm_aql_handler.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace aql {

std::vector<float> LLMAQLEmbeddingBridge::embed(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    try {
        return handler_.executeEmbed(text);
    } catch (const std::exception& e) {
        spdlog::debug("LLMAQLEmbeddingBridge::embed(): executeEmbed failed ({}); "
                      "few-shot ranking falls back to Jaccard", e.what());
        return {};
    } catch (...) {
        spdlog::debug("LLMAQLEmbeddingBridge::embed(): executeEmbed threw unknown exception; "
                      "few-shot ranking falls back to Jaccard");
        return {};
    }
}

} // namespace aql
} // namespace themis
