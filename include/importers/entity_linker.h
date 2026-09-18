/**
 * @file entity_linker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/importer_interface.h"
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

enum class LinkType {
    SAME_AS,          ///< OWL sameAs: logically identical records
    DUPLICATE_OF,     ///< Exact copy of an existing record
    SUBSUMED_BY,      ///< The source was absorbed by the target
    MERGED_INTO,      ///< Multiple records merged into one golden record
    VERSION_OF,       ///< A newer version of an existing record
    RELATED_TO,       ///< Loose relationship, not necessarily the same entity
    POSSIBLY_SAME,    ///< Low-confidence match; requires manual review
    CROSS_DOMAIN_LINK ///< Same entity referenced from different source systems
};

enum class ResolutionStatus {
    UNRESOLVED,    ///< Link exists but the conflict has not been addressed
    RESOLVED,      ///< Link confirmed; golden record defined
    MANUAL_REVIEW, ///< Awaiting user decision
    ARCHIVED       ///< Link is obsolete (e.g., superseded by a later merge)
};

struct EntityLink {
    std::string                  source_id;       ///< Incoming entity ID (from import)
    std::string                  target_id;       ///< Existing ThemisDB entity ID
    LinkType                     link_type     = LinkType::SAME_AS;
    ResolutionStatus             status        = ResolutionStatus::UNRESOLVED;
    double                       confidence    = 0.0;   ///< 0.0–1.0
    json                         matching_evidence;     ///< Why were these entities linked?
    std::vector<std::string>     matched_fields;
    std::string                  created_at;     ///< RFC 3339 timestamp
    std::string                  created_by;     ///< "importer_v2.2" or user identifier
    json                         metadata;

    /**
     * @brief To Json.
     * @return Return value.
     */
    json toJson() const;
};

struct LinkAuditEntry {
    EntityLink  link;
    std::string event_type;   ///< "created" | "updated" | "archived"
    std::string event_at;     ///< RFC 3339
};

class EntityLinker {
public:
    EntityLinker() = default;

    /**
     * @brief Create Link.
     * @param[in] link Input parameter.
     * @param[in] options Input parameter.
     * @return True when the operation succeeds.
     */
    bool createLink(
        const EntityLink&    link,
        const ImportOptions& options
    );

    ImportStats linkBatch(
        const std::string&             collection_name,
        const std::vector<EntityLink>& links,
        const ImportOptions&           options,
        size_t                         batch_size = 1000
    );

    /**
     * @brief Get Links For Entity.
     * @param[in] entity_id Identifier of the entity.
     * @param[in] collection_name Name of the collection.
     * @return Return value.
     */
    std::vector<LinkAuditEntry> getLinksForEntity(
        const std::string& entity_id,
        const std::string& collection_name
    ) const;

    json exportLinkGraph(
        const std::string&              collection_name,
        const std::vector<std::string>& entity_ids,
        bool                            include_confidence_scores = true
    ) const;

    /**
     * @brief Link Count.
     * @return Return value.
     */
    size_t linkCount() const;

    /**
     * @brief Clear.
     */
    void clear();

private:
    // All links created during this session, keyed by source_id.
    std::vector<EntityLink> links_;
};

} // namespace importers
} // namespace themis
