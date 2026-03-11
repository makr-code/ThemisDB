/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            postgres_importer.h                                ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:53:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     186                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 11c3fb7c3  2026-02-28  feat(importers): implement dry-run import preview for Pos... ║
    • a3d6da5ac  2026-02-24  feat(importers): implement conflict resolution strategies... ║
    • 625263378  2026-02-23  Resolve code-audit findings: rename dummy→checkpoint_stat... ║
    • 220fc09b2  2026-02-22  Add streaming import API for large datasets without full ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "importers/importer_interface.h"
#include "importers/conflict_resolver.h"
#include "plugins/plugin_interface.h"
#include <regex>
#include <atomic>
#include <unordered_set>

namespace themis {
namespace importers {

/**
 * @brief PostgreSQL pg_dump Importer
 * 
 * Imports data from PostgreSQL pg_dump files (SQL format).
 * Supports:
 * - DDL parsing (CREATE TABLE, CREATE SCHEMA)
 * - DML parsing (INSERT, COPY)
 * - Schema mapping to ThemisDB BaseEntity
 * - Type conversion with configurable user overrides
 * - Batch processing
 * - Structured error reporting
 */
class PostgreSQLImporter : public IImporter {
public:
    PostgreSQLImporter();
    ~PostgreSQLImporter() override;
    
    // IImporter interface
    const char* getName() const override { return "PostgreSQL Importer"; }
    std::vector<std::string> getSupportedTypes() const override;
    bool initialize(const std::string& config) override;
    bool validateSource(const std::string& source_path, std::vector<std::string>& errors) override;
    ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr
    ) override;
    ImportStats importDataStreaming(
        const std::string& source_path,
        const ImportOptions& options,
        RowCallback row_callback
    ) override;
    std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options
    ) override;
    void cancel() override;
    json getSourceSchema(const std::string& source_path) override;
    
private:
    struct TableSchema {
        std::string name;
        std::string schema;
        std::vector<std::string> columns;
        std::map<std::string, std::string> column_types;
        std::vector<std::string> primary_keys;

        /// v2.0: Foreign Key Preservation – extracted FK constraints.
        struct ForeignKeyConstraint {
            std::string constraint_name;        ///< e.g. "fk_orders_user"
            std::vector<std::string> columns;   ///< local column(s), e.g. {"user_id"}
            std::string ref_table;              ///< referenced table, e.g. "users"
            std::vector<std::string> ref_columns; ///< referenced columns, e.g. {"id"}
            std::string on_delete;              ///< ON DELETE action, e.g. "CASCADE" (empty = RESTRICT)
            std::string on_update;              ///< ON UPDATE action, e.g. "SET NULL" (empty = RESTRICT)

            json toJson() const {
                return json{
                    {"constraint_name", constraint_name},
                    {"columns",         columns},
                    {"ref_table",       ref_table},
                    {"ref_columns",     ref_columns},
                    {"on_delete",       on_delete},
                    {"on_update",       on_update}
                };
            }
        };
        std::vector<ForeignKeyConstraint> foreign_keys;  ///< v2.0: preserved FK constraints
    };
    
    std::atomic<bool> cancelled_{false};
    std::map<std::string, TableSchema> schemas_;
    std::map<std::string, std::string> custom_type_map_;  ///< Types from CREATE TYPE statements
    ImportConflictResolver conflict_resolver_;  ///< In-session conflict tracker
    
    // Parsing methods
    bool parseDumpFile(const std::string& file_path, const ImportOptions& options, ImportStats& stats,
                       ProgressCallback& callback);
    bool parseCreateTable(const std::string& sql, TableSchema& schema);
    bool parseInsert(const std::string& sql, const ImportOptions& options, ImportStats& stats,
                     size_t line_number);
    bool parseCopy(std::ifstream& file, const std::string& table_name,
                   const std::vector<std::string>& columns,
                   const ImportOptions& options, ImportStats& stats,
                   std::unordered_set<uint64_t>& delta_hashes);

    // v2.0: Foreign Key helpers
    /// Parse a single FOREIGN KEY constraint clause (table-level) and append to schema.
    /// @param constraint_def  The trimmed constraint definition, e.g.
    ///   "CONSTRAINT fk_x FOREIGN KEY (col) REFERENCES tbl(id) ON DELETE CASCADE"
    ///   or bare "FOREIGN KEY (col) REFERENCES tbl(id)"
    /// @return true if a valid FK was parsed and appended.
    bool parseForeignKeyConstraint(const std::string& constraint_def,
                                   TableSchema& schema) const;

    /// Parse an inline column-level REFERENCES clause and append a FK to schema.
    /// @param col_name   The column owning the reference.
    /// @param col_def    Full column definition string, e.g.
    ///   "user_id integer NOT NULL REFERENCES users(id) ON DELETE CASCADE"
    /// @return true if a REFERENCES clause was found and a FK was appended.
    bool parseInlineReference(const std::string& col_name,
                              const std::string& col_def,
                              TableSchema& schema) const;

    /// Parse a standalone ALTER TABLE … ADD CONSTRAINT … FOREIGN KEY statement
    /// and update the corresponding cached schema entry.
    void parseAlterTableAddFk(const std::string& sql,
                               const ImportOptions& options,
                               ImportStats& stats);
    
    // Schema mapping
    std::string mapPostgreSQLTypeToThemis(const std::string& pg_type,
                                          const ImportOptions& options) const;
    bool shouldImportTable(const std::string& table_name, const ImportOptions& options);
    
    // Data conversion
    json convertRowToEntity(const TableSchema& schema, const std::vector<std::string>& values);

    // COPY row helpers
    std::vector<std::string> parseCopyRow(const std::string& line) const;
    std::string unescapeCopyValue(const std::string& val) const;

    // INSERT helpers
    std::vector<std::string> parseInsertValues(const std::string& values_clause) const;
    
    // Error helpers
    void addError(ImportStats& stats, ImportErrorCode code, ImportErrorSeverity severity,
                  const std::string& message, const std::string& location = "") const;

    // Metrics emission helper
    void emitMetric(const ImportOptions& options,
                    const std::string& metric,
                    const std::map<std::string, std::string>& labels,
                    double value) const;

    // Distributed tracing / OTel span emission helper
    void emitSpan(const ImportOptions& options,
                  const std::string& operation,
                  const std::map<std::string, std::string>& attributes,
                  double duration_seconds) const;

    // UTF-8 validation helper
    static bool isValidUtf8(const std::string& s);

    // Checkpoint helpers
    bool loadCheckpoint(const std::string& checkpoint_file, std::streampos& offset,
                        ImportStats& accumulated_stats) const;
    void saveCheckpoint(const std::string& checkpoint_file, std::streampos offset,
                        const ImportStats& stats) const;

    // Quarantine helpers
    void writeQuarantineRow(const std::string& quarantine_file,
                            const std::string& table_name,
                            const std::string& raw_row,
                            const ImportError& error) const;

    // Delta / incremental import helpers
    static uint64_t computeRowHash(const std::string& raw_row,
                                   const std::vector<std::string>& values,
                                   const std::vector<std::string>& key_columns,
                                   const std::vector<std::string>& schema_columns);
    static std::unordered_set<uint64_t> loadDeltaHashes(const std::string& delta_hash_file);
    static void saveDeltaHashes(const std::string& delta_hash_file,
                                const std::unordered_set<uint64_t>& hashes);

    // Progress reporting
    void reportProgress(ProgressCallback& callback, const std::string& stage, size_t current, size_t total);
};

/**
 * @brief PostgreSQL Importer Plugin
 * 
 * Wraps PostgreSQLImporter as a ThemisDB plugin
 */
class PostgreSQLImporterPlugin : public plugins::IThemisPlugin {
public:
    PostgreSQLImporterPlugin();
    ~PostgreSQLImporterPlugin() override = default;
    
    // IThemisPlugin interface
    const char* getName() const override { return "postgres_importer"; }
    const char* getVersion() const override { return "1.7.0"; }
    plugins::PluginType getType() const override { return plugins::PluginType::IMPORTER; }
    plugins::PluginCapabilities getCapabilities() const override;
    bool initialize(const char* config_json) override;
    void shutdown() override;
    void* getInstance() override { return importer_.get(); }
    
private:
    std::unique_ptr<PostgreSQLImporter> importer_;
};

} // namespace importers
} // namespace themis
