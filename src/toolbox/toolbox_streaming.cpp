/**
 * @file toolbox_streaming.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "toolbox/toolbox_streaming.h"
#include "toolbox/toolbox_registry.h"

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// Injected toolbox overload
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Extract Entities Stream.
 * @param[in,out] toolbox Input/output parameter.
 * @param[in] text Input parameter.
 * @param[in] mime Input parameter.
 * @param[in] filename Input parameter.
 * @param[in] callback Input parameter.
 */
void extractEntitiesStream(
    IngestionToolbox&     toolbox,
    const std::string&    text,
    const std::string&    mime,
    const std::string&    filename,
    const EntityCallback& callback)
{
    if (text.empty() || !callback) {
        return;
    }

    TextChunker chunker;
    auto chunk_texts = chunker.chunkTexts(text);

    for (const auto& chunk : chunk_texts) {
        if (chunk.empty()) {
            continue;
        }
        auto entities = toolbox.extractEntities(chunk, mime, filename);
        for (const auto& entity : entities) {
            callback(entity);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Global registry overload
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Extract Entities Stream.
 * @param[in] text Input parameter.
 * @param[in] mime Input parameter.
 * @param[in] filename Input parameter.
 * @param[in] callback Input parameter.
 */
void extractEntitiesStream(
    const std::string&    text,
    const std::string&    mime,
    const std::string&    filename,
    const EntityCallback& callback)
{
    auto tb = ToolboxRegistry::instance();
    extractEntitiesStream(*tb, text, mime, filename, callback);
}

} // namespace toolbox
} // namespace themis
