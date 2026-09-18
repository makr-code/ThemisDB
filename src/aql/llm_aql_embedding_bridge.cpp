/**
 * @file llm_aql_embedding_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/llm_aql_embedding_bridge.h"
#include <stdexcept>
#include "aql/llm_aql_handler.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace aql {

/**
 * @brief Embed.
 * @param[in] text Input parameter.
 * @return Return value.
 * @details Calls: empty(), executeEmbed(), spdlog::warn(), what().
 */
std::vector<float> LLMAQLEmbeddingBridge::embed(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    try {
        return handler_.executeEmbed(text);
    } catch (const std::exception& e) {
        spdlog::warn("[BRIDGE:ExecutionFailed] LLMAQLEmbeddingBridge::embed(): executeEmbed failed ({}); "
                      "few-shot ranking falls back to Jaccard", e.what());
        return {};
    } catch (...) {
        spdlog::warn("[BRIDGE:ExecutionFailed] LLMAQLEmbeddingBridge::embed(): executeEmbed threw unknown exception; "
                      "few-shot ranking falls back to Jaccard");
        return {};
    }
}

} // namespace aql
} // namespace themis
