/*
 * ThemisDB | File: toolbox_streaming.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
