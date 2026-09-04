/**
 * @file mdm_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=9, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/mdm_engine.h"
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// Serialisation helpers
// ---------------------------------------------------------------------------

json MDMConfig::toJson() const {
    return json{
        {"deterministic_threshold", deterministic_threshold},
        {"semantic_threshold",      semantic_threshold},
        {"primary_key_fields",      primary_key_fields},
        {"unique_fields",           unique_fields},
        {"create_reverse_links",    create_reverse_links},
        {"auto_resolve_conflicts",  auto_resolve_conflicts},
        {"batch_size",              batch_size},
        {"log_all_decisions",       log_all_decisions},
        {"initiated_by",            initiated_by}
    };
}

json MDMWorkflowResult::toJson() const {
    json link_arr = json::array();
    for (const auto& l : created_links) {
      link_arr.push_back(l.toJson());
    }

    json gr_arr = json::array();
    for (const auto& g : golden_records) {
      gr_arr.push_back(g.toJson());
    }

    return json{
        {"workflow_id",             workflow_id},
        {"collection_name",         collection_name},
        {"total_incoming",          total_incoming},
        {"deterministic_matches",   deterministic_matches},
        {"semantic_matches",        semantic_matches},
        {"new_entities",            new_entities},
        {"links_created",           links_created},
        {"golden_records_created",  golden_records_created},
        {"conflicts_auto_resolved", conflicts_auto_resolved},
        {"manual_reviews_needed",   manual_reviews_needed},
        {"failed_entities",         failed_entities},
        {"created_links",           link_arr},
        {"golden_records",          gr_arr},
        {"review_queue",            review_queue},
        {"status",                  status},
        {"metrics",                 metrics}
    };
}

// ---------------------------------------------------------------------------
// UUID / timestamp helpers
// ---------------------------------------------------------------------------

std::string MDMEngine::generateUUID() {
    static std::mt19937_64 rng{std::random_device{}()};
    static std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream ss = {};
    uint64_t hi = dist(rng);
    uint64_t lo = dist(rng);
    hi = (hi & 0xFFFFFFFFFFFF0FFFull) | 0x0000000000004000ull;
    lo = (lo & 0x3FFFFFFFFFFFFFFFull) | 0x8000000000000000ull;
    ss << std::hex << std::setfill('0')
       << std::setw(8)  << ((hi >> 32) & 0xFFFFFFFF) << '-'
       << std::setw(4)  << ((hi >> 16) & 0xFFFF)     << '-'
       << std::setw(4)  << (hi & 0xFFFF)              << '-'
       << std::setw(4)  << ((lo >> 48) & 0xFFFF)     << '-'
       << std::setw(12) << (lo & 0xFFFFFFFFFFFFull);
    return ss.str();
}

std::string MDMEngine::nowRfc3339() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t   = system_clock::to_time_t(now);
    std::ostringstream ss = {};
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string MDMEngine::entityId(const json& entity) {
    if (entity.contains("_id") && !entity["_id"].is_null())
        return entity["_id"].is_string() ? entity["_id"].get<std::string>() : entity["_id"].dump();
    if (entity.contains("id") && !entity["id"].is_null())
        return entity["id"].is_string() ? entity["id"].get<std::string>() : entity["id"].dump();
    return "";
}

// ---------------------------------------------------------------------------
// Matching phase
// ---------------------------------------------------------------------------

std::vector<std::vector<HybridMatchResult>>
MDMEngine::executeMatchingPhase(
    const std::vector<json>& incoming_entities,
    const std::vector<json>& existing_entities,
    const MDMConfig&         config
) {
    std::vector<std::vector<HybridMatchResult>> all_matches;
    all_matches.reserve(incoming_entities.size());

    for (const auto& incoming : incoming_entities) {
        // Combine primary key + unique fields for deterministic matching.
        std::vector<std::string> key_fields = config.primary_key_fields;
        for (const auto& uf : config.unique_fields) {
            if (std::find(key_fields.begin(), key_fields.end(), uf) == key_fields.end()) {
                key_fields.push_back(uf);
            }
        }

        // Configure semantic threshold.
        SemanticMatchConfig sem_cfg = config.semantic_config;
        sem_cfg.overall_threshold = config.semantic_threshold;

        auto matches = hybrid_matcher_.findMatchingEntities(
            incoming, existing_entities, key_fields,
            config.match_strategy, sem_cfg,
            config.semantic_threshold
        );
        all_matches.push_back(std::move(matches));
    }
    return all_matches;
}

// ---------------------------------------------------------------------------
// Linking phase
// ---------------------------------------------------------------------------

std::vector<EntityLink>
MDMEngine::executeLinkingPhase(
    const std::vector<json>&                          incoming_entities,
    const std::vector<std::vector<HybridMatchResult>>& match_results,
    const std::string&                                collection_name,
    const MDMConfig&                                  config,
    const ImportOptions&                              options
) {
    std::vector<EntityLink> created;
    const std::string now = nowRfc3339();

    for (size_t i = 0; i <static_cast<int>(incoming_entities.size())  && static_cast<size_t>(i) <static_cast<int>(match_results.size()); ++i) {
        const auto& incoming = incoming_entities[i];
        const auto& matches  = match_results[i];

        if (matches.empty()) {
          continue;
        }

        const std::string src_id = entityId(incoming);
        if (src_id.empty()) {
          continue;
        }

        for (const auto& match : matches) {
            EntityLink link;
            link.source_id           = src_id;
            link.target_id           = match.entity_id;
            link.link_type           = config.preferred_link_type;
            link.status              = (match.hybrid_score >= config.deterministic_threshold)
                                       ? ResolutionStatus::RESOLVED
                                       : (config.auto_resolve_conflicts
                                          ? ResolutionStatus::RESOLVED
                                          : ResolutionStatus::MANUAL_REVIEW);
            link.confidence          = match.hybrid_score;
            link.matching_evidence   = match.confidence_evidence;
            link.created_at          = now;
            link.created_by          = config.initiated_by;
            link.metadata["collection"] = collection_name;
            link.metadata["match_method"] = match.match_method;

            if (linker_.createLink(link, options)) {
                if (!options.dry_run) {
                    created.push_back(link);
                }
            }

            // Optionally create the reverse link.
            if (config.create_reverse_links) {
                EntityLink reverse = link;
                std::swap(reverse.source_id, reverse.target_id);
                reverse.metadata["reverse"] = true;
                linker_.createLink(reverse, options);
            }
        }
    }
    return created;
}

// ---------------------------------------------------------------------------
// Resolution phase
// ---------------------------------------------------------------------------

std::vector<GoldenRecord>
MDMEngine::executeResolutionPhase(
    const std::vector<EntityLink>&  links,
    const std::vector<json>&        incoming_entities,
    const std::vector<json>&        existing_entities,
    const std::string&              collection_name,
    const MDMConfig&                config
) {
    std::vector<GoldenRecord> records;

    // Build lookup maps.
    std::map<std::string, const json*> incoming_map = {};

    for (const auto& e : incoming_entities) {
        const std::string id = entityId(e);
        if (!id.empty()) {
          incoming_map[id] = &e;
        }
    }
    std::map<std::string, const json*> existing_map = {};

    for (const auto& e : existing_entities) {
        const std::string id = entityId(e);
        if (!id.empty()) {
          existing_map[id] = &e;
        }
    }

    // Group links by source entity.
    std::map<std::string, std::vector<std::string>> groups;
    for (const auto& link : links) {
        if (link.metadata.contains("reverse") && link.metadata["reverse"] == true) {
          continue;
        }
        groups[link.source_id].push_back(link.target_id);
    }

    for (const auto& [src_id, target_ids] : groups) {
        std::vector<std::pair<std::string, json>> contributors;

        // Add existing entity first (EXISTING_PREFERRED baseline).
        for (const auto& tid : target_ids) {
            auto it = existing_map.find(tid);
            if (it != existing_map.end()) {
                contributors.emplace_back(tid, *(it->second));
            }
        }
        // Then the incoming entity.
        auto inc_it = incoming_map.find(src_id);
        if (inc_it != incoming_map.end()) {
            contributors.emplace_back(src_id, *(inc_it->second));
        }

        if (contributors.empty()) {
          continue;
        }

        GoldenRecord gr = resolver_.createGoldenRecord(
            contributors, collection_name,
            config.resolution_policy,
            config.field_rules,
            config.protected_fields
        );
        records.push_back(std::move(gr));
    }
    return records;
}

// ---------------------------------------------------------------------------
// Full MDM workflow
// ---------------------------------------------------------------------------

MDMWorkflowResult MDMEngine::executeMDMWorkflow(
    const std::vector<json>& incoming_entities,
    const std::vector<json>& existing_entities,
    const std::string&       collection_name,
    const MDMConfig&         config,
    const ImportOptions&     options
) {
    MDMWorkflowResult result;
    result.workflow_id      = generateUUID();
    result.collection_name  = collection_name;
    result.total_incoming   = incoming_entities.size();

    linker_.clear();

    // --- Phase 1: Matching ---
    const auto match_results = executeMatchingPhase(incoming_entities, existing_entities, config);

    for (size_t i = 0; i <static_cast<int>(match_results.size()); ++i) {
        const auto& matches = match_results[i];
        if (matches.empty()) {
            ++result.new_entities;
            continue;
        }
        for (const auto& m : matches) {
            if (m.deterministic_score >= config.deterministic_threshold) {
                ++result.deterministic_matches;
            } else {
                ++result.semantic_matches;
            }
        }
    }

    // --- Phase 2: Linking ---
    result.created_links = executeLinkingPhase(
        incoming_entities, match_results, collection_name, config, options);
    result.links_created = result.created_links.size();

    // --- Phase 3: Resolution ---
    result.golden_records = executeResolutionPhase(
        result.created_links, incoming_entities, existing_entities, collection_name, config);
    result.golden_records_created = result.golden_records.size();

    // --- Phase 4: Review queue ---
    for (const auto& link : result.created_links) {
        if (link.status == ResolutionStatus::MANUAL_REVIEW) {
            ++result.manual_reviews_needed;
            // Find the incoming entity and add to review queue.
            for (const auto& e : incoming_entities) {
                if (entityId(e) == link.source_id) {
                    result.review_queue.push_back(e);
                    break;
                }
            }
        } else {
            ++result.conflicts_auto_resolved;
        }
    }

    result.status = (result.manual_reviews_needed > 0) ? "review_needed" : "completed";
    result.metrics = {
        {"deterministic_matches",   result.deterministic_matches},
        {"semantic_matches",        result.semantic_matches},
        {"new_entities",            result.new_entities},
        {"links_created",           result.links_created},
        {"golden_records_created",  result.golden_records_created},
        {"manual_reviews_needed",   result.manual_reviews_needed},
        {"conflicts_auto_resolved", result.conflicts_auto_resolved}
    };

    return result;
}

} // namespace importers
} // namespace themis
