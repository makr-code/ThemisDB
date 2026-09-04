/**
 * @file postgres_importer_mdm.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/postgres_importer_mdm.h"

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// buildMDMConfig – translate EntityLinkingConfig → MDMConfig
// ---------------------------------------------------------------------------

MDMConfig PostgreSQLImporterWithMDM::buildMDMConfig(const EntityLinkingConfig& elc)
{
    MDMConfig cfg;

    // Strategy: 0 = DETERMINISTIC_FIRST, 1 = SEMANTIC_FIRST, 2 = WEIGHTED_ENSEMBLE
    switch (elc.strategy) {
        case 1:
            cfg.match_strategy = HybridEntityMatcher::MatchStrategy::SEMANTIC_FIRST;
            break;
        case 2:
            cfg.match_strategy = HybridEntityMatcher::MatchStrategy::WEIGHTED_ENSEMBLE;
            break;
        default:
            cfg.match_strategy = HybridEntityMatcher::MatchStrategy::DETERMINISTIC_FIRST;
            break;
    }

    cfg.deterministic_threshold = elc.deterministic_threshold;
    cfg.semantic_threshold      = elc.semantic_threshold;

    // Resolution policy: 0 = NEWEST_FIRST, 1 = MOST_COMPLETE, 2 = EXISTING_PREFERRED,
    //                     3 = INCOMING_PREFERRED, 4 = RICHEST_MERGE, 5 = CUSTOM_RULES
    switch (elc.resolution_policy) {
        case 0:  cfg.resolution_policy = ResolutionPolicy::NEWEST_FIRST;       break;
        case 1:  cfg.resolution_policy = ResolutionPolicy::MOST_COMPLETE;      break;
        case 2:  cfg.resolution_policy = ResolutionPolicy::EXISTING_PREFERRED; break;
        case 3:  cfg.resolution_policy = ResolutionPolicy::INCOMING_PREFERRED; break;
        case 5:  cfg.resolution_policy = ResolutionPolicy::CUSTOM_RULES;       break;
        default: cfg.resolution_policy = ResolutionPolicy::RICHEST_MERGE;      break;
    }

    cfg.auto_resolve_conflicts = elc.auto_resolve_conflicts;
    cfg.create_reverse_links   = elc.create_reverse_links;
    cfg.protected_fields       = elc.protected_fields;

    // Semantic config: aggregate field weights / algorithms from collection configs
    if (!elc.collection_configs.empty()) {
        const auto& first = elc.collection_configs.begin()->second;
        cfg.semantic_config.field_weights    = first.field_weights;
        cfg.semantic_config.field_algorithms = first.field_algorithms;
        cfg.semantic_config.overall_threshold = first.semantic_threshold;
    } else {
        cfg.semantic_config.overall_threshold = elc.semantic_threshold;
    }

    return cfg;
}

// ---------------------------------------------------------------------------
// importData – standard import + optional MDM phase
// ---------------------------------------------------------------------------

ImportStats PostgreSQLImporterWithMDM::importData(
    const std::string&   source_path,
    const ImportOptions& options,
    ProgressCallback     progress_callback
)
{
    // Clear state from previous run.
    audit_trail_.clear();
    last_mdm_result_ = MDMWorkflowResult{};

    // Phase 1: standard PostgreSQL import.
    ImportStats stats = PostgreSQLImporter::importData(source_path, options, progress_callback);

    // Phase 2: MDM entity linking & deduplication (opt-in).
    if (options.entity_linking.enabled) {
        const MDMConfig mdm_cfg = buildMDMConfig(options.entity_linking);

        // Collect imported entities from the stats JSON blob so we can pass
        // them to the MDM engine.  When no per-row data is available (e.g. the
        // base importer does not populate stats.sample_entities) we work with
        // an empty existing set and let the engine handle new-entity detection.
        std::vector<json> imported_entities;
        if (stats.sample_entities.is_array()) {
            for (const auto& e : stats.sample_entities) {
                imported_entities.push_back(e);
            }
        }

        // In a production integration the "existing_entities" set would be
        // fetched from the ThemisDB collection index.  For now we expose an
        // empty existing set so the MDM engine correctly treats every imported
        // entity as new (no spurious self-matches).
        last_mdm_result_ = applyMDMWorkflow(imported_entities, mdm_cfg, options);

        // Merge MDM counters into the aggregate stats.
        stats.entities_linked    += static_cast<uint64_t>(last_mdm_result_.links_created);
        stats.golden_records     += static_cast<uint64_t>(last_mdm_result_.golden_records_created);
        stats.mdm_reviews_needed += static_cast<uint64_t>(last_mdm_result_.manual_reviews_needed);
    }

    return stats;
}

// ---------------------------------------------------------------------------
// applyMDMWorkflow
// ---------------------------------------------------------------------------

MDMWorkflowResult PostgreSQLImporterWithMDM::applyMDMWorkflow(
    const std::vector<json>& imported_entities,
    const MDMConfig&         config,
    const ImportOptions&     options
)
{
    // Existing entities: in a real deployment these would be fetched from
    // the in-memory index of the target ThemisDB collection.  Since the
    // PostgreSQL importer does not hold a reference to the storage engine,
    // we pass an empty set here.  Callers that do have access to existing
    // entities should instantiate the MDMEngine directly.
    const std::vector<json> existing_entities;

    MDMWorkflowResult result = engine_.executeMDMWorkflow(
        imported_entities,
        existing_entities,
        "imported",
        config,
        options
    );

    // Forward MDM audit events into our trail.
    for (const auto& link : result.created_links) {
        MDMAuditTrail::AuditEvent ev;
        ev.operation        = MDMAuditTrail::Operation::LINK_CREATED;
        ev.collection_name  = "imported";
        ev.source_entity_id = link.source_id;
        ev.target_entity_id = link.target_id;
        ev.confidence_score = link.confidence;
        ev.status           = "completed";
        ev.initiated_by     = config.initiated_by;
        audit_trail_.recordEvent([[maybe_unused]] std::move(ev));
    }

    for (const auto& gr : result.golden_records) {
        MDMAuditTrail::AuditEvent ev;
        ev.operation        = MDMAuditTrail::Operation::GOLDEN_RECORD_CREATED;
        ev.collection_name  = "imported";
        ev.source_entity_id = gr.canonical_id;
        ev.confidence_score = gr.completeness_score;
        ev.status           = "completed";
        ev.initiated_by     = config.initiated_by;
        audit_trail_.recordEvent([[maybe_unused]] std::move(ev));
    }

    return result;
}

} // namespace importers
} // namespace themis
