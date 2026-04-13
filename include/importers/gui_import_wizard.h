/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gui_import_wizard.h                                ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 04:15:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     289                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
    • 15e6e31437  2026-03-09  feat: implement all features from problem statement ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file gui_import_wizard.h
 * @brief Web-UI driven import wizard for guided multi-source data ingestion.
 *
 * ImportWizard orchestrates a multi-step guided import flow that can be
 * driven by either a REST API (web UI) or programmatically:
 *
 * ## Wizard Steps
 * 1. **SOURCE**     – Select data source type (PostgreSQL, MySQL, S3, flat-file, …)
 * 2. **CONNECT**    – Enter connection details; validate connectivity.
 * 3. **PREVIEW**    – Display a sample of the source data (schema + first 20 rows).
 * 4. **MAP**        – Map source columns → ThemisDB field names / types.
 * 5. **OPTIONS**    – Set conflict-resolution strategy, batch size, dry-run flag.
 * 6. **CONFIRM**    – Show summary; user confirms before import starts.
 * 7. **IMPORT**     – Execute import with streaming progress updates.
 * 8. **DONE**       – Final summary: rows imported, errors, duration.
 *
 * ## REST API (served by ImporterApiHandler in the server module)
 * | Method | Path                               | Action                          |
 * |--------|------------------------------------|----------------------------------|
 * | POST   | /api/v1/import/wizard/session      | Create session → session_id      |
 * | GET    | /api/v1/import/wizard/{id}/state   | Get current step + state JSON    |
 * | PUT    | /api/v1/import/wizard/{id}/step    | Advance/update step with payload |
 * | POST   | /api/v1/import/wizard/{id}/run     | Start actual import              |
 * | GET    | /api/v1/import/wizard/{id}/progress| Streaming import progress        |
 * | DELETE | /api/v1/import/wizard/{id}         | Cancel / delete session          |
 *
 * @note Thread Safety: ImportWizard sessions are NOT thread-safe per instance.
 *   The server layer must ensure one goroutine/thread per session.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
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

/**
 * @brief Current step in the import wizard flow.
 */
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

std::string wizardStepName(WizardStep step);

// ─────────────────────────────────────────────────────────────────────────────
// ImportWizardState
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Column mapping: source column → target field.
 */
struct ColumnMapping {
    std::string source_column;  ///< Column name in the source dataset
    std::string target_field;   ///< Field name in ThemisDB collection
    std::string target_type;    ///< Target type ("string", "int64", "float64", "bool", "datetime")
    bool        skip = false;   ///< If true, skip this column entirely
};

/**
 * @brief Complete wizard state serialisable to/from JSON.
 */
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

    json toJSON() const;
    static ImportWizardState fromJSON(const json& j);
};

// ─────────────────────────────────────────────────────────────────────────────
// ImportWizard
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Server-side import wizard session.
 *
 * Drives one multi-step import flow.  Instantiated by the REST API handler
 * for each POST /api/v1/import/wizard/session call.
 */
class ImportWizard {
public:
    /// Progress callback invoked during the IMPORT step.
    using ProgressCallback = std::function<void(const ImportWizardState&)>;

    /**
     * @brief Configuration for the wizard.
     */
    struct Config {
        /// Maximum rows to include in the PREVIEW step.
        size_t preview_max_rows = 20;
        /// Maximum concurrent import sessions (enforced by ImportWizardManager).
        size_t max_sessions = 50;
        /// Registry of source importer factories (name → factory).
        std::unordered_map<std::string,
            std::function<std::unique_ptr<IImporter>()>> importer_factories;
    };

    explicit ImportWizard();
    explicit ImportWizard(Config config);

    // ── Session lifecycle ─────────────────────────────────────────────────────

    /**
     * @brief Create a new wizard session.
     * @return Unique session ID.
     */
    std::string createSession();

    /**
     * @brief Return the current wizard state for a session.
     */
    const ImportWizardState& getState(const std::string& session_id) const;

    // ── Step handlers ─────────────────────────────────────────────────────────

    /**
     * @brief Set the source type (advances from SOURCE → CONNECT).
     */
    ImportWizardState& setSource(const std::string& session_id,
                                  const std::string& source_type);

    /**
     * @brief Validate connection parameters and load preview schema.
     *
     * Advances from CONNECT → PREVIEW on success.
     * Sets state.error_message and stays on CONNECT on failure.
     */
    ImportWizardState& connect(const std::string& session_id,
                                const json&        connection_params);

    /**
     * @brief Apply user-supplied column mappings.
     *
     * Advances from PREVIEW / MAP → OPTIONS.
     */
    ImportWizardState& setColumnMappings(const std::string&              session_id,
                                          const std::vector<ColumnMapping>& mappings,
                                          const std::string&              target_collection);

    /**
     * @brief Set import options (conflict strategy, batch size, dry-run).
     *
     * Advances to CONFIRM.
     */
    ImportWizardState& setOptions(const std::string& session_id,
                                   const std::string& conflict_strategy,
                                   size_t             batch_size,
                                   bool               dry_run);

    /**
     * @brief Confirm and start the import.
     *
     * Moves to IMPORT and begins asynchronous execution.
     * Progress is delivered via @p on_progress and the final state via
     * the returned future.
     *
     * @param session_id   Session to import.
     * @param on_progress  Callback invoked after each batch.
     */
    void runImport(const std::string& session_id,
                   ProgressCallback   on_progress = {});

    /**
     * @brief Cancel an in-progress import.
     */
    void cancel(const std::string& session_id);

    /**
     * @brief Remove a completed or cancelled session.
     */
    void deleteSession(const std::string& session_id);

    /**
     * @brief List all active session IDs.
     */
    std::vector<std::string> activeSessions() const;

private:
    Config config_;
    std::unordered_map<std::string, ImportWizardState> sessions_;

    ImportWizardState& requireSession(const std::string& session_id);
    std::string generateSessionId() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// ImportWizardManager (process-singleton convenience wrapper)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Process-global import wizard registry.
 *
 * Provides a singleton ImportWizard instance shared by the API handler layer.
 */
class ImportWizardManager {
public:
    static ImportWizardManager& instance();

    void configure(ImportWizard::Config config);
    ImportWizard& wizard();

private:
    ImportWizardManager() = default;
    std::unique_ptr<ImportWizard> wizard_;
    mutable std::mutex mutex_;
};

} // namespace importers
} // namespace themis
