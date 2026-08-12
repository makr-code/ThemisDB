/**
 * @file mdm_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Configuration for an MDM workflow execution.
 */
struct MDMConfig {
    // -----------------------------------------------------------------------
    // Matching
    // -----------------------------------------------------------------------
    HybridEntityMatcher::MatchStrategy match_strategy =
        HybridEntityMatcher::MatchStrategy::DETERMINISTIC_FIRST;
    double deterministic_threshold = 1.0;  ///< Minimum score for deterministic match acceptance
    double semantic_threshold      = 0.85; ///< Minimum semantic score to accept a match

    SemanticMatchConfig semantic_config;   ///< Detailed semantic matching settings

    /// Field names used for deterministic primary-key matching.
    std::vector<std::string> primary_key_fields;

    /// Field names with unique constraints (deterministic matching).
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

    json toJson() const;
};

/**
 * @brief Accumulated results of a single MDM workflow run.
 */
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

    /// Entities that need manual review (below auto-resolve threshold).
    std::vector<json> review_queue;

    std::string status;  ///< "completed" | "review_needed" | "failed"
    json        metrics;

    json toJson() const;
};

/**
 * @brief Orchestrates the full MDM workflow: match → link → resolve → audit.
 *
 * The engine is stateless between calls.  Each call to executeMDMWorkflow()
 * is independent.
 *
 * Thread-safety: MDMEngine instances are not thread-safe.  Use one instance
 * per import session.
 */
class MDMEngine {
public:
    MDMEngine() = default;

    /**
     * @brief Execute the complete MDM workflow for a batch of incoming entities.
     *
     * Phases:
     *   1. Matching   – find existing entities using the configured strategy.
     *   2. Linking    – create EntityLink records for matched pairs.
     *   3. Resolution – produce GoldenRecords for matched groups.
     *   4. Audit      – return structured audit events via the result object.
     *
     * @param incoming_entities  Entities freshly parsed from the import source.
     * @param existing_entities  Existing ThemisDB entities to match against.
     * @param collection_name    Target collection name.
     * @param config             MDM configuration.
     * @param options            Import options (used for dry-run / logging).
     * @return                   Workflow result including all produced artefacts.
     */
    MDMWorkflowResult executeMDMWorkflow(
        const std::vector<json>& incoming_entities,
        const std::vector<json>& existing_entities,
        const std::string&       collection_name,
        const MDMConfig&         config,
        const ImportOptions&     options
    );

    /**
     * @brief Matching phase only: returns match results for each incoming entity.
     *
     * @param incoming_entities  Entities to match.
     * @param existing_entities  Candidate existing entities.
     * @param config             MDM configuration.
     * @return                   One HybridMatchResult vector per incoming entity.
     */
    std::vector<std::vector<HybridMatchResult>> executeMatchingPhase(
        const std::vector<json>& incoming_entities,
        const std::vector<json>& existing_entities,
        const MDMConfig&         config
    );

    /**
     * @brief Linking phase only: creates entity links from prior match results.
     *
     * @param incoming_entities  Original incoming entities.
     * @param match_results      Per-entity match results from executeMatchingPhase().
     * @param collection_name    Target collection.
     * @param config             MDM configuration.
     * @param options            Import options.
     * @return                   All EntityLink objects that were created.
     */
    std::vector<EntityLink> executeLinkingPhase(
        const std::vector<json>&                          incoming_entities,
        const std::vector<std::vector<HybridMatchResult>>& match_results,
        const std::string&                                collection_name,
        const MDMConfig&                                  config,
        const ImportOptions&                              options
    );

    /**
     * @brief Resolution phase only: builds golden records from link groups.
     *
     * @param links              Links produced by executeLinkingPhase().
     * @param incoming_entities  Original incoming entities (source data).
     * @param existing_entities  Existing entity data.
     * @param collection_name    Target collection.
     * @param config             MDM configuration.
     * @return                   Golden records produced.
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

    static std::string generateUUID();
    static std::string nowRfc3339();
    static std::string entityId(const json& entity);
};

} // namespace importers
} // namespace themis
