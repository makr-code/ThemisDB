/**
 * @file gui_import_wizard.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "importers/importer_interface.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Wizard step enumeration
// ─────────────────────────────────────────────────────────────────────────────

enum class WizardStep : uint8_t {
    SOURCE   = 0,
    CONNECT  = 1,
    PREVIEW  = 2,
    MAP      = 3,
    OPTIONS  = 4,
    CONFIRM  = 5,
    IMPORT   = 6,
    DONE     = 7,
};

/**
 * @brief Wizard Step Name.
 * @param[in] step Input parameter.
 * @return Return value.
 */
std::string wizardStepName(WizardStep step);

// ─────────────────────────────────────────────────────────────────────────────
// ImportWizardState
// ─────────────────────────────────────────────────────────────────────────────

struct ColumnMapping {
    std::string source_column;  ///< Column name in the source dataset
    std::string target_field;   ///< Field name in ThemisDB collection
    std::string target_type;    ///< Target type ("string", "int64", "float64", "bool", "datetime")
    bool        skip = false;   ///< If true, skip this column entirely
};

struct ImportWizardState {
    std::string session_id;
    WizardStep  current_step    = WizardStep::SOURCE;
    bool        completed       = false;
    std::string error_message;

    // Step 1: SOURCE
    std::string source_type;    ///< "postgresql", "mysql", "sqlite", "s3", "flatfile", "kafka"

    // Step 2: CONNECT
    json connection_params;     ///< Source-specific connection parameters

    // Step 3: PREVIEW
    json preview_schema;        ///< [{name, type, nullable, sample_values}]
    json preview_rows;          ///< First N rows as array of objects

    // Step 4: MAP
    std::vector<ColumnMapping> column_mappings;
    std::string target_collection; ///< Target ThemisDB collection name

    // Step 5: OPTIONS
    std::string conflict_strategy = "upsert"; ///< "upsert", "skip", "replace", "fail"
    size_t      batch_size        = 1000;
    bool        dry_run           = false;
    bool        enable_schema_validation = true;

    // Step 7: IMPORT progress
    size_t      rows_processed = 0;
    size_t      rows_imported  = 0;
    size_t      rows_failed    = 0;
    double      progress_pct   = 0.0;
    std::string started_at;
    std::string finished_at;

    /**
     * @brief To JSON.
     * @return Return value.
     */
    json toJSON() const;
    /**
     * @brief From JSON.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ImportWizardState fromJSON(const json& j);
};

// ─────────────────────────────────────────────────────────────────────────────
// ImportWizard
// ─────────────────────────────────────────────────────────────────────────────

class ImportWizard {
public:
    using ProgressCallback = std::function<void(const ImportWizardState&)>;

    struct Config {
        size_t preview_max_rows = 20;
        size_t max_sessions = 50;
        std::unordered_map<std::string,
            std::function<std::unique_ptr<IImporter>()>> importer_factories;
    };

    /**
     * @brief Import Wizard.
     * @return Return value.
     */
    explicit ImportWizard();
    /**
     * @brief Import Wizard.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit ImportWizard(Config config);

    /**
     * @brief ── Session lifecycle ─────────────────────────────────────────────────────
     * @return Session token.
     */

    std::string createSession();

    /**
     * @brief Get State.
     * @param[in] session_id Identifier of the session.
     * @return Return value.
     */
    const ImportWizardState& getState(const std::string& session_id) const;

    /**
     * @brief ── Step handlers ─────────────────────────────────────────────────────────
     * @param[in] session_id Identifier of the session.
     * @param[in] source_type Input parameter.
     * @return Return value.
     */

    ImportWizardState& setSource(const std::string& session_id,
                                  const std::string& source_type);

    /**
     * @brief Connect.
     * @param[in] session_id Identifier of the session.
     * @param[in] connection_params Input parameter.
     * @return Return value.
     */
    ImportWizardState& connect(const std::string& session_id,
                                const json&        connection_params);

    /**
     * @brief Set Column Mappings.
     * @param[in] session_id Identifier of the session.
     * @param[in] mappings Input parameter.
     * @param[in] target_collection Input parameter.
     * @return Return value.
     */
    ImportWizardState& setColumnMappings(const std::string&              session_id,
                                          const std::vector<ColumnMapping>& mappings,
                                          const std::string&              target_collection);

    /**
     * @brief Set Options.
     * @param[in] session_id Identifier of the session.
     * @param[in] conflict_strategy Input parameter.
     * @param[in] batch_size Input parameter.
     * @param[in] dry_run Input parameter.
     * @return Return value.
     */
    ImportWizardState& setOptions(const std::string& session_id,
                                   const std::string& conflict_strategy,
                                   size_t             batch_size,
                                   bool               dry_run);

    void runImport(const std::string& session_id,
                   ProgressCallback   on_progress = {});

    /**
     * @brief Cancel.
     * @param[in] session_id Identifier of the session.
     */
    void cancel(const std::string& session_id);

    /**
     * @brief Delete Session.
     * @param[in] session_id Identifier of the session.
     */
    void deleteSession(const std::string& session_id);

    /**
     * @brief Active Sessions.
     * @return Return value.
     */
    std::vector<std::string> activeSessions() const;

private:
    Config config_;
    std::unordered_map<std::string, ImportWizardState> sessions_;

    /**
     * @brief Require Session.
     * @param[in] session_id Identifier of the session.
     * @return Return value.
     */
    ImportWizardState& requireSession(const std::string& session_id);
    /**
     * @brief Generate Session Id.
     * @return Return value.
     */
    std::string generateSessionId() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// ImportWizardManager (process-singleton convenience wrapper)
// ─────────────────────────────────────────────────────────────────────────────

class ImportWizardManager {
public:
    /**
     * @brief Instance.
     * @return Return value.
     */
    static ImportWizardManager& instance();

    /**
     * @brief Configure.
     * @param[in] config Input parameter.
     */
    void configure(ImportWizard::Config config);
    /**
     * @brief Wizard.
     * @return Return value.
     */
    ImportWizard& wizard();

private:
    ImportWizardManager() = default;
    std::unique_ptr<ImportWizard> wizard_;
    mutable std::mutex mutex_;
};

} // namespace importers
} // namespace themis
