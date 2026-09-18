/**
 * @file postgres_importer_mdm.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class PostgreSQLImporterWithMDM : public PostgreSQLImporter {
public:
    PostgreSQLImporterWithMDM() = default;
    ~PostgreSQLImporterWithMDM() override = default;

    const char* getName() const override { return "PostgreSQL Importer with MDM"; }

    ImportStats importData(
        const std::string&   source_path,
        const ImportOptions& options,
        ProgressCallback     progress_callback = nullptr
    ) override;

    const MDMWorkflowResult& lastMDMResult() const { return last_mdm_result_; }

    const MDMAuditTrail& auditTrail() const { return audit_trail_; }

private:
    /**
     * @brief Build MDMConfig.
     * @param[in] elc Input parameter.
     * @return Return value.
     */
    static MDMConfig buildMDMConfig(const EntityLinkingConfig& elc);

    /**
     * @brief Apply MDMWorkflow.
     * @param[in] imported_entities Input parameter.
     * @param[in] config Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
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
