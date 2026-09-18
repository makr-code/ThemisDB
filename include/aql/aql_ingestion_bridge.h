/**
 * @file aql_ingestion_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class AQLIngestionBridge {
public:
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

    /**
     * @brief ── Core operations ───────────────────────────────────────────────────────
     * @param[in,out] payload Input/output parameter.
     * @return Return value.
     */

    std::string enrichInsertPayload(nlohmann::json& payload);

    /**
     * @brief Extract Entities For Context.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<ingestion::BaseEntity> extractEntitiesForContext(
        const std::string& text
    );

    /**
     * @brief Build Entity Context.
     * @param[in] entities Input parameter.
     * @return Return value.
     */
    static std::string buildEntityContext(
        const std::vector<ingestion::BaseEntity>& entities
    );

    /**
     * @brief ── Accessors ─────────────────────────────────────────────────────────────
     * @return Return value.
     */

    std::shared_ptr<toolbox::IngestionToolbox> toolbox() const;

    /**
     * @brief Graph Writer.
     * @return Return value.
     */
    std::shared_ptr<ingestion::IGraphWriter> graphWriter() const;

private:
    std::shared_ptr<toolbox::IngestionToolbox> toolbox_;
    std::shared_ptr<ingestion::IGraphWriter>   graph_writer_;
    std::string                                text_field_key_;

    /**
     * @brief Entity Type Name.
     * @param[in] et Input parameter.
     * @return Return value.
     */
    static std::string entityTypeName(ingestion::EntityType et);
};

} // namespace aql
} // namespace themis
