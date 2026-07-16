/**
 * @file llm_aql_embedding_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: llm_aql_embedding_bridge.cpp | Version: 0.0.1 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 99/100 | Lines: 36
 * Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * PR History (last 5): #4789 RAG: make LLMJudge mock-sta... (2026-04-22)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "aql/llm_aql_embedding_bridge.h"
#include <stdexcept>
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

