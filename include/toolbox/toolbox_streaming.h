/**
 * @file toolbox_streaming.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "toolbox/ingestion_toolbox.h"
#include "toolbox/text_chunker.h"
#include "ingestion/base_entity.h"
#include <functional>
#include <string>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// Callback type
// ─────────────────────────────────────────────────────────────────────────────

/// Callback invoked for each `BaseEntity` extracted from a chunk.
using EntityCallback = std::function<void(const ingestion::BaseEntity&)>;

// ─────────────────────────────────────────────────────────────────────────────
// Core implementation — injected toolbox
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Chunk @p text and stream extracted entities to @p callback.
 *
 * Algorithm:
 *  1. Split @p text with `TextChunker` (default config: chunk_size=512,
 *     overlap=64, strategy=Sentence).
 *  2. For each chunk, call `toolbox.extractEntities(chunk_text, mime, filename)`.
 *  3. Invoke @p callback once per extracted `BaseEntity`, in chunk order.
 *
 * @param toolbox   Injected `IngestionToolbox` to use for extraction.
 * @param text      UTF-8 text to process.
 * @param mime      MIME type hint forwarded to the toolbox (default: "text/plain").
 * @param filename  Filename hint forwarded to the toolbox (default: "input.txt").
 * @param callback  Called once per entity, never called on empty results.
 *                  The callback must not throw; exceptions propagate to the
 *                  caller but leave the streaming loop in an unspecified state.
 */
void extractEntitiesStream(
    IngestionToolbox&   toolbox,
    const std::string&  text,
    const std::string&  mime,
    const std::string&  filename,
    const EntityCallback& callback);

/**
 * @brief Overload with default MIME / filename.
 */
inline void extractEntitiesStream(
    IngestionToolbox&     toolbox,
    const std::string&    text,
    const EntityCallback& callback)
{
    extractEntitiesStream(toolbox, text, "text/plain", "input.txt", callback);
}

// ─────────────────────────────────────────────────────────────────────────────
// Free function — global registry
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Stream entities from @p text using the global `ToolboxRegistry`.
 *
 * Delegates to `ToolboxRegistry::instance()->extractEntities()` per chunk.
 *
 * @param text      UTF-8 text to process.
 * @param mime      MIME type hint (default: "text/plain").
 * @param filename  Filename hint (default: "input.txt").
 * @param callback  Invoked once per extracted entity.
 * @throws std::logic_error when the registry is not initialised.
 */
void extractEntitiesStream(
    const std::string&    text,
    const std::string&    mime,
    const std::string&    filename,
    const EntityCallback& callback);

/**
 * @brief Overload with default MIME / filename.
 */
inline void extractEntitiesStream(
    const std::string&    text,
    const EntityCallback& callback)
{
    extractEntitiesStream(text, "text/plain", "input.txt", callback);
}

} // namespace toolbox
} // namespace themis
