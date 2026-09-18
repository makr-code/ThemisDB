/**
 * @file mdm_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/entity_matcher.h"
#include "importers/entity_linker.h"
#include "importers/canonical_resolver.h"
#include "importers/importer_interface.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

struct MDMConfig {
    // -----------------------------------------------------------------------
    // Matching
    // -----------------------------------------------------------------------
    HybridEntityMatcher::MatchStrategy match_strategy =
        HybridEntityMatcher::MatchStrategy::DETERMINISTIC_FIRST;
    double deterministic_threshold = 1.0;  ///< Minimum score for deterministic match acceptance
    double semantic_threshold      = 0.85; ///< Minimum semantic score to accept a match

    SemanticMatchConfig semantic_config;   ///< Detailed semantic matching settings

    std::vector<std::string> primary_key_fields;

    std::vector<std::string> unique_fields;

    // -----------------------------------------------------------------------
    // Linking
    // -----------------------------------------------------------------------
    LinkType preferred_link_type    = LinkType::SAME_AS;
    bool     create_reverse_links   = true;  ///< Also store target→source links

    // -----------------------------------------------------------------------
    // Resolution
    // -----------------------------------------------------------------------
    ResolutionPolicy resolution_policy = ResolutionPolicy::RICHEST_MERGE;
    bool             auto_resolve_conflicts = false; ///< false = queue for manual review

    std::map<std::string, FieldRule> field_rules;     ///< Per-field resolution rules
    std::vector<std::string>         protected_fields; ///< Fields never to overwrite

    // -----------------------------------------------------------------------
    // Performance
    // -----------------------------------------------------------------------
    bool   parallelize_matching = true;
    size_t batch_size           = 1000;

    // -----------------------------------------------------------------------
    // Audit
    // -----------------------------------------------------------------------
    bool        log_all_decisions   = true;
    std::string audit_collection    = "mdm_audit_trail";
    std::string initiated_by        = "importer_v2.2"; ///< Tag written to audit events

    /**
     * @brief To Json.
     * @return Return value.
     */
    json toJson() const;
};

struct MDMWorkflowResult {
    std::string workflow_id;           ///< UUID
    std::string collection_name;

    size_t total_incoming     = 0;
    size_t deterministic_matches = 0;
    size_t semantic_matches   = 0;
    size_t new_entities       = 0;
    size_t links_created      = 0;
    size_t golden_records_created = 0;
    size_t conflicts_auto_resolved = 0;
    size_t manual_reviews_needed   = 0;
    size_t failed_entities    = 0;

    std::vector<EntityLink>  created_links;
    std::vector<GoldenRecord> golden_records;

    std::vector<json> review_queue;

    std::string status;  ///< "completed" | "review_needed" | "failed"
    json        metrics;

    /**
     * @brief To Json.
     * @return Return value.
     */
    json toJson() const;
};

class MDMEngine {
public:
    MDMEngine() = default;

    /**
     * @brief Execute MDMWorkflow.
     * @param[in] incoming_entities Input parameter.
     * @param[in] existing_entities Input parameter.
     * @param[in] collection_name Name of the collection.
     * @param[in] config Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    MDMWorkflowResult executeMDMWorkflow(
        const std::vector<json>& incoming_entities,
        const std::vector<json>& existing_entities,
        const std::string&       collection_name,
        const MDMConfig&         config,
        const ImportOptions&     options
    );

    /**
     * @brief Execute Matching Phase.
     * @param[in] incoming_entities Input parameter.
     * @param[in] existing_entities Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    std::vector<std::vector<HybridMatchResult>> executeMatchingPhase(
        const std::vector<json>& incoming_entities,
        const std::vector<json>& existing_entities,
        const MDMConfig&         config
    );

    /**
     * @brief Execute Linking Phase.
     * @param[in] incoming_entities Input parameter.
     * @param[in] match_results Input parameter.
     * @param[in] collection_name Name of the collection.
     * @param[in] config Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::vector<EntityLink> executeLinkingPhase(
        const std::vector<json>&                          incoming_entities,
        const std::vector<std::vector<HybridMatchResult>>& match_results,
        const std::string&                                collection_name,
        const MDMConfig&                                  config,
        const ImportOptions&                              options
    );

    /**
     * @brief Execute Resolution Phase.
     * @param[in] links Input parameter.
     * @param[in] incoming_entities Input parameter.
     * @param[in] existing_entities Input parameter.
     * @param[in] collection_name Name of the collection.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    std::vector<GoldenRecord> executeResolutionPhase(
        const std::vector<EntityLink>&  links,
        const std::vector<json>&        incoming_entities,
        const std::vector<json>&        existing_entities,
        const std::string&              collection_name,
        const MDMConfig&                config
    );

private:
    HybridEntityMatcher   hybrid_matcher_;
    EntityLinker          linker_;
    CanonicalEntityResolver resolver_;

    /**
     * @brief Generate UUID.
     * @return Return value.
     */
    static std::string generateUUID();
    /**
     * @brief Now Rfc3339.
     * @return Return value.
     */
    static std::string nowRfc3339();
    /**
     * @brief Entity Id.
     * @param[in] entity Input parameter.
     * @return Return value.
     */
    static std::string entityId(const json& entity);
};

} // namespace importers
} // namespace themis
