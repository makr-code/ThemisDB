/**
 * @file toolbox_streaming.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

void extractEntitiesStream(
    IngestionToolbox&     toolbox,
    const std::string&    text,
    const std::string&    mime,
    const std::string&    filename,
    const EntityCallback& callback)
{
    if ([[maybe_unused]] text.empty() || !callback) {
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
            callback([[maybe_unused]] entity);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Global registry overload
// ─────────────────────────────────────────────────────────────────────────────

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
