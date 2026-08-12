/**
 * @file postgres_importer_mdm.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/postgres_importer.h"
#include "importers/mdm_engine.h"
#include "importers/mdm_audit_trail.h"
#include "importers/mdm_metrics.h"

namespace themis {
namespace importers {

/**
 * @brief PostgreSQL importer extended with MDM entity-linking and deduplication.
 *
 * Subclasses PostgreSQLImporter and adds a post-import MDM phase that
 * automatically:
 *   1. Matches incoming entities against existing ThemisDB records.
 *   2. Creates typed entity links for matched pairs.
 *   3. Produces golden records via the configured resolution policy.
 *   4. Records all decisions in an immutable audit trail.
 *
 * MDM processing is only activated when
 * @c ImportOptions::entity_linking::enabled is true.
 *
 * Usage:
 * @code
 * PostgreSQLImporterWithMDM importer;
 *
 * ImportOptions opts;
 * opts.entity_linking.enabled              = true;
 * opts.entity_linking.strategy             = 2; // WEIGHTED_ENSEMBLE
 * opts.entity_linking.semantic_threshold   = 0.85;
 * opts.entity_linking.auto_resolve_conflicts = false;
 *
 * auto stats = importer.importData("dump.sql", opts);
 * std::cout << "Links created: " << stats.entities_linked << "\n";
 * @endcode
 *
 * Thread-safety: not thread-safe; use one instance per import session.
 */
class PostgreSQLImporterWithMDM : public PostgreSQLImporter {
public:
    PostgreSQLImporterWithMDM() = default;
    ~PostgreSQLImporterWithMDM() override = default;

    const char* getName() const override { return "PostgreSQL Importer with MDM"; }

    /**
     * @brief Import data and apply MDM deduplication if enabled.
     *
     * Executes the standard PostgreSQL import (phase 1) via the base class,
     * then—if @c options.entity_linking.enabled—runs the MDM workflow (phase 2)
     * over the imported entities.  The MDM results are merged into the returned
     * @c ImportStats (fields @c entities_linked and @c records_written are
     * updated accordingly).
     *
     * @param source_path         Path to the pg_dump SQL file.
     * @param options             Import options (entity_linking controls MDM).
     * @param progress_callback   Optional progress callback.
     * @return                    Merged import + MDM statistics.
     */
    ImportStats importData(
        const std::string&   source_path,
        const ImportOptions& options,
        ProgressCallback     progress_callback = nullptr
    ) override;

    /**
     * @brief Return the MDM workflow result from the most-recent importData() call.
     *
     * Returns an empty result if MDM was not run or importData() has not been
     * called yet.
     */
    const MDMWorkflowResult& lastMDMResult() const { return last_mdm_result_; }

    /**
     * @brief Return the audit trail accumulated during the most-recent MDM run.
     *
     * The trail is cleared at the start of each importData() call.
     */
    const MDMAuditTrail& auditTrail() const { return audit_trail_; }

private:
    /**
     * @brief Build an MDMConfig from the EntityLinkingConfig in ImportOptions.
     */
    static MDMConfig buildMDMConfig(const EntityLinkingConfig& elc);

    /**
     * @brief Run the MDM workflow over the entities produced by the import.
     *
     * @param imported_entities  Entities produced by the base-class import.
     * @param config             MDM configuration.
     * @param options            Original import options.
     * @return                   MDM workflow result.
     */
    MDMWorkflowResult applyMDMWorkflow(
        const std::vector<json>& imported_entities,
        const MDMConfig&         config,
        const ImportOptions&     options
    );

    MDMEngine          engine_;
    MDMAuditTrail      audit_trail_;
    MDMWorkflowResult  last_mdm_result_;
};

} // namespace importers
} // namespace themis
