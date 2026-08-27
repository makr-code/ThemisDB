/**
 * @file aql_ingestion_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "toolbox/ingestion_toolbox.h"
#include "ingestion/base_entity.h"
#include "ingestion/ingestion_sinks.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace aql {

// ─────────────────────────────────────────────────────────────────────────────
// AQLIngestionBridge
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Bridge that connects the `toolbox::IngestionToolbox` to the AQL
 *        subsystem, enabling opt-in ingestion enrichment at query time.
 *
 * ## Dependency direction
 * @code
 *   aql/  →  toolbox/  →  ingestion/
 * @endcode
 * `ingestion/` never imports `aql/` or `toolbox/` headers.
 *
 * ## Capabilities
 *
 * ### 1. `enrichInsertPayload(json&)`
 * When an AQL `INSERT` or `UPSERT` operation carries a document with a
 * `"text"` field (or a field named via the configured key), this method:
 *  1. Feeds the text through the `WorkflowEngine` via
 *     `IngestionToolbox::extractEntities()`.
 *  2. Serialises the resulting `BaseEntity` list and appends it to the
 *     document under the key `"_entities"`.
 *  3. If an `IGraphWriter` sink was provided at construction, the entities
 *     and their relations are also written to the graph store.
 *  4. Returns a concise context string (entity types + IDs) suitable for
 *     injection into an LLM prompt.
 *
 * If the document has no text field, or if the toolbox is unavailable,
 * the method is a safe no-op and returns an empty string.
 *
 * ### 2. `extractEntitiesForContext(text)`
 * Delegates directly to `IngestionToolbox::extractEntities()` and returns
 * the `BaseEntity` nodes for use as NL→AQL context.
 *
 * ### 3. `buildEntityContext(entities)`
 * Converts a `BaseEntity` list to a compact string that can be appended to
 * an LLM schema context, for example:
 * @code
 * "Extracted entities: LEGAL_PROVISION law:BGB:§823, ORGANIZATION org:abc12"
 * @endcode
 *
 * ## Thread-safety
 * All public methods are thread-safe; the bridge holds no mutable state
 * beyond the constructor-injected shared pointers.
 *
 * ## Usage
 * @code
 * auto toolbox = themis::toolbox::IngestionToolbox::createDefault();
 * auto bridge  = std::make_shared<AQLIngestionBridge>(toolbox);
 * handler.setIngestionBridge(bridge);
 * @endcode
 */
class AQLIngestionBridge {
public:
    /**
     * @brief Construct a bridge backed by @p toolbox.
     *
     * @param toolbox       Shared toolbox instance; must not be null.
     * @param graph_writer  Optional graph-store sink.  When provided,
     *                      `enrichInsertPayload()` writes extracted
     *                      entities/relations to the graph store.
     *                      Pass `nullptr` to skip graph writes.
     * @param text_field_key  Name of the JSON document field that holds the
     *                        text to enrich.  Default: @c "text".
     */
    explicit AQLIngestionBridge(
        std::shared_ptr<toolbox::IngestionToolbox>    toolbox,
        std::shared_ptr<ingestion::IGraphWriter>      graph_writer    = nullptr,
        std::string                                   text_field_key  = "text"
    );

    ~AQLIngestionBridge() noexcept;

    // Non-copyable, movable
    AQLIngestionBridge(const AQLIngestionBridge&) = delete;
    AQLIngestionBridge& operator=(const AQLIngestionBridge&) = delete;
    AQLIngestionBridge(AQLIngestionBridge&&) noexcept;
    AQLIngestionBridge& operator=(AQLIngestionBridge&&) noexcept;

    // ── Core operations ───────────────────────────────────────────────────────

    /**
     * @brief Enrich a JSON document payload through the ingestion workflow.
     *
     * If @p payload is a JSON object containing the configured `text_field_key`,
     * the text is processed by the `WorkflowEngine`.  The resulting entities
     * are appended to @p payload under the key `"_entities"` (as a JSON
     * array of objects with `"id"`, `"type"`, and `"text"` fields).
     *
     * If an `IGraphWriter` sink was supplied at construction, the entities
     * and relations are also written to the graph store.
     *
     * @param payload  JSON document to enrich (modified in place).
     * @return Compact entity context string (empty when no enrichment occurred).
     */
    std::string enrichInsertPayload(nlohmann::json& payload);

    /**
     * @brief Extract entities from @p text for use as NL→AQL context.
     *
     * Thin wrapper around `IngestionToolbox::extractEntities()`.
     *
     * @param text  UTF-8 text to process.
     * @return Extracted and normalised entity nodes; empty when extraction
     *         yields no results or the text is empty.
     */
    std::vector<ingestion::BaseEntity> extractEntitiesForContext(
        const std::string& text
    );

    /**
     * @brief Build a compact LLM context string from a list of entities.
     *
     * Formats the entity list as a single line for injection into an LLM
     * schema context prompt.  Example output:
     * @code
     * "Extracted entities: LEGAL_PROVISION law:BGB:§823 | ORGANIZATION org:abc12"
     * @endcode
     * Returns an empty string when @p entities is empty.
     *
     * @param entities  Entities to summarise.
     * @return Formatted context string; empty when entities is empty.
     */
    static std::string buildEntityContext(
        const std::vector<ingestion::BaseEntity>& entities
    );

    // ── Accessors ─────────────────────────────────────────────────────────────

    /**
     * @brief Return the backing `IngestionToolbox`.
     */
    std::shared_ptr<toolbox::IngestionToolbox> toolbox() const;

    /**
     * @brief Return the configured graph-writer sink (may be null).
     */
    std::shared_ptr<ingestion::IGraphWriter> graphWriter() const;

private:
    std::shared_ptr<toolbox::IngestionToolbox> toolbox_;
    std::shared_ptr<ingestion::IGraphWriter>   graph_writer_;
    std::string                                text_field_key_;

    /// Map an EntityType enum value to a short display string.
    static std::string entityTypeName(ingestion::EntityType et);
};

} // namespace aql
} // namespace themis
