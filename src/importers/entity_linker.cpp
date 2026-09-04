/**
 * @file entity_linker.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/entity_linker.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// EntityLink serialisation
// ---------------------------------------------------------------------------

static std::string linkTypeName(LinkType lt) {
    switch (lt) {
        case LinkType::SAME_AS:
            return "same_as";
        case LinkType::DUPLICATE_OF:
            return "duplicate_of";
        case LinkType::SUBSUMED_BY:
            return "subsumed_by";
        case LinkType::MERGED_INTO:
            return "merged_into";
        case LinkType::VERSION_OF:
            return "version_of";
        case LinkType::RELATED_TO:
            return "related_to";
        case LinkType::POSSIBLY_SAME:
            return "possibly_same";
        case LinkType::CROSS_DOMAIN_LINK:
            return "cross_domain_link";
    }
    return "unknown";
}

static std::string resolutionStatusName(ResolutionStatus rs) {
    switch (rs) {
        case ResolutionStatus::UNRESOLVED:
            return "unresolved";
        case ResolutionStatus::RESOLVED:
            return "resolved";
        case ResolutionStatus::MANUAL_REVIEW:
            return "manual_review";
        case ResolutionStatus::ARCHIVED:
            return "archived";
    }
    return "unknown";
}

json EntityLink::toJson() const {
    return json{{"source_id", source_id},
                {"target_id", target_id},
                {"link_type", linkTypeName(link_type)},
                {"status", resolutionStatusName(status)},
                {"confidence", confidence},
                {"matching_evidence", matching_evidence},
                {"matched_fields", matched_fields},
                {"created_at", created_at},
                {"created_by", created_by},
                {"metadata", metadata}};
}

// ---------------------------------------------------------------------------
// EntityLinker
// ---------------------------------------------------------------------------

bool EntityLinker::createLink(const EntityLink &link, const ImportOptions &options) {
    if (link.source_id.empty() || link.target_id.empty()) {
        return false;
    }
    if (options.dry_run) {
        return true; // Pretend success without storing.
    }
    links_.push_back(link);
    return true;
}

ImportStats EntityLinker::linkBatch(const std::string & /*collection_name*/, const std::vector<EntityLink> &links,
                                    const ImportOptions &options, size_t /*batch_size*/
) {
    ImportStats stats;
    stats.total_records = links.size();

    for (const auto &link : links) {
        if (createLink(link, options)) {
            ++stats.imported_records;
        } else {
            ++stats.failed_records;
        }
    }
    return stats;
}

std::vector<LinkAuditEntry> EntityLinker::getLinksForEntity(const std::string &entity_id,
                                                            const std::string &collection_name) const {
    std::vector<LinkAuditEntry> entries = {};

    for (const auto &link : links_) {
        if (link.source_id != entity_id && link.target_id != entity_id) {
            continue;
        }

        // Filter by collection when the link metadata carries it.
        if (!collection_name.empty()) {
            if (link.metadata.contains("collection")) {
                const std::string c
                    = link.metadata["collection"].is_string() ? link.metadata["collection"].get<std::string>() : "";
                if (!c.empty() && c != collection_name) {
                    continue;
                }
            }
        }

        LinkAuditEntry entry;
        entry.link       = link;
        entry.event_type = "created";
        entry.event_at   = link.created_at;
        entries.push_back(std::move(entry));
    }
    return entries;
}

json EntityLinker::exportLinkGraph(const std::string &collection_name, const std::vector<std::string> &entity_ids,
                                   bool include_confidence_scores) const {
    json nodes = json::array();
    json edges = json::array();

    // Collect all relevant entity IDs (seed set or all).
    for (const auto &link : links_) {
        // Collection filter (if metadata present).
        if (!collection_name.empty() && link.metadata.contains("collection")) {
            const std::string c
                = link.metadata["collection"].is_string() ? link.metadata["collection"].get<std::string>() : "";
            if (!c.empty() && c != collection_name) {
                continue;
            }
        }

        // Entity-ID filter.
        if (!entity_ids.empty()) {
            bool src_ok = std::find(entity_ids.begin(), entity_ids.end(), link.source_id) != entity_ids.end();
            bool tgt_ok = std::find(entity_ids.begin(), entity_ids.end(), link.target_id) != entity_ids.end();
            if (!src_ok && !tgt_ok) {
                continue;
            }
        }

        // Add node stubs (deduplication done by downstream consumers).
        nodes.push_back(json{{"id", link.source_id}});
        nodes.push_back(json{{"id", link.target_id}});

        json edge{{"source", link.source_id},
                  {"target", link.target_id},
                  {"link_type", linkTypeName(link.link_type)},
                  {"status", resolutionStatusName(link.status)}};
        if (include_confidence_scores) {
            edge["confidence"] = link.confidence;
        }
        edges.push_back(std::move(edge));
    }

    return json{{"nodes", nodes}, {"edges", edges}};
}

size_t EntityLinker::linkCount() const {
    return static_cast<int>(links_.size());
}

void EntityLinker::clear() {
    links_.clear();
}

} // namespace importers
} // namespace themis
