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
