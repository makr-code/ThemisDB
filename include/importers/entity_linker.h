/**
 * @file entity_linker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Semantic type of a directional link between two entities.
 */
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

/**
 * @brief Resolution lifecycle status of a link.
 */
enum class ResolutionStatus {
    UNRESOLVED,    ///< Link exists but the conflict has not been addressed
    RESOLVED,      ///< Link confirmed; golden record defined
    MANUAL_REVIEW, ///< Awaiting user decision
    ARCHIVED       ///< Link is obsolete (e.g., superseded by a later merge)
};

/**
 * @brief A directed link between a newly imported entity and an existing one.
 */
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

    json toJson() const;
};

/**
 * @brief Audit entry returned by EntityLinker::getLinksForEntity().
 */
struct LinkAuditEntry {
    EntityLink  link;
    std::string event_type;   ///< "created" | "updated" | "archived"
    std::string event_at;     ///< RFC 3339
};

/**
 * @brief Manages creation and retrieval of entity links in an in-memory store.
 *
 * EntityLinker stores links in memory during an import session.  Callers
 * retrieve the resulting link set via getLinksForEntity() or exportLinkGraph()
 * and persist them to their own storage layer.
 *
 * Thread-safety: EntityLinker is NOT thread-safe.  Use one instance per
 * import worker thread / per importData() call.
 */
class EntityLinker {
public:
    EntityLinker() = default;

    /**
     * @brief Record a single entity link.
     *
     * @param link      Link to create.  Both @c source_id and @c target_id must
     *                  be non-empty.
     * @param options   Import options (used for dry-run check).
     * @return          true on success, false if the link could not be created
     *                  (e.g., empty IDs, dry-run mode).
     */
    bool createLink(
        const EntityLink&    link,
        const ImportOptions& options
    );

    /**
     * @brief Batch-create entity links.
     *
     * @param collection_name   Name of the collection.
     * @param links             Links to create.
     * @param options           Import options.
     * @param batch_size        Number of links to process per internal iteration.
     * @return                  ImportStats with links_created / failed_records counts.
     */
    ImportStats linkBatch(
        const std::string&             collection_name,
        const std::vector<EntityLink>& links,
        const ImportOptions&           options,
        size_t                         batch_size = 1000
    );

    /**
     * @brief Retrieve all links associated with a given entity ID.
     *
     * Returns links where @p entity_id is either the source or the target.
     *
     * @param entity_id         Entity to query.
     * @param collection_name   Collection scope (empty = all collections).
     * @return                  Audit entries for all matching links.
     */
    std::vector<LinkAuditEntry> getLinksForEntity(
        const std::string& entity_id,
        const std::string& collection_name
    ) const;

    /**
     * @brief Export the complete link graph as a JSON structure.
     *
     * Returns a JSON object with "nodes" and "edges" arrays suitable for
     * graph-visualisation tools.
     *
     * @param collection_name           Collection scope.
     * @param entity_ids                Seed entity IDs to include (empty = all).
     * @param include_confidence_scores Whether to include confidence values on edges.
     * @return                          Graph JSON.
     */
    json exportLinkGraph(
        const std::string&              collection_name,
        const std::vector<std::string>& entity_ids,
        bool                            include_confidence_scores = true
    ) const;

    /**
     * @brief Return the total number of links currently stored.
     */
    size_t linkCount() const;

    /**
     * @brief Clear all stored links (used between import sessions).
     */
    void clear();

private:
    // All links created during this session, keyed by source_id.
    std::vector<EntityLink> links_;
};

} // namespace importers
} // namespace themis
