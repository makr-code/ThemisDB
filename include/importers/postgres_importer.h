/**
 * @file postgres_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/importer_interface.h"
#include "importers/conflict_resolver.h"
#include "importers/relationship_mapper.h"
#include "plugins/plugin_interface.h"
#include <regex>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace themis {
namespace importers {

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
    // -------------------------------------------------------------------------
    // v2.0 data structures
    // -------------------------------------------------------------------------

    struct CheckConstraint {
        std::string name;        ///< Constraint name (empty for unnamed CHECK)
        std::string expression;  ///< The expression inside CHECK (...)

        json toJson() const {
            return json{{"name", name}, {"expression", expression}};
        }
    };

    struct GeneratedColumnInfo {
        std::string column;      ///< Column name
        std::string expression;  ///< Generation expression (empty for identity)
        std::string generation;  ///< "ALWAYS" or "BY_DEFAULT"
        bool is_identity = false; ///< True when GENERATED … AS IDENTITY
        bool stored      = false; ///< True when STORED (virtual otherwise)

        json toJson() const {
            return json{
                {"column",      column},
                {"expression",  expression},
                {"generation",  generation},
                {"is_identity", is_identity},
                {"stored",      stored}
            };
        }
    };

    struct ExcludeConstraint {
        std::string name;
        std::string index_method;
        struct Element {
            std::string column;        ///< Column or expression
            std::string with_operator; ///< Exclusion operator (e.g. "=", "&&", "<>")
        };
        std::vector<Element> elements;
        std::string definition = {};

        json toJson() const {
            json elems = json::array();
            for (const auto& el : elements) {
                elems.push_back({{"column", el.column}, {"with_operator", el.with_operator}});
            }
            return json{
                {"name",         name},
                {"index_method", index_method},
                {"elements",     elems},
                {"definition",   definition}
            };
        }
    };

    struct ForeignKeyConstraint {
        std::string name;               ///< Constraint name (empty for inline REFERENCES)
        std::string source_column;      ///< Comma-joined source columns, e.g. "user_id"
        std::string target_table;       ///< Referenced table, e.g. "users"
        std::string target_column;      ///< Comma-joined referenced columns, e.g. "id"
        std::string on_delete_action;   ///< CASCADE | SET NULL | RESTRICT | NO ACTION | SET DEFAULT
        std::string on_update_action;   ///< CASCADE | SET NULL | RESTRICT | NO ACTION | SET DEFAULT
        bool deferrable = false;        ///< DEFERRABLE keyword present
        bool initially_deferred = false; ///< INITIALLY DEFERRED (vs. INITIALLY IMMEDIATE)

        json toJson() const {
            return json{
                {"name", name},
                {"source_column", source_column},
                {"target_table", target_table},
                {"target_column", target_column},
                {"on_delete", on_delete_action},
                {"on_update", on_update_action},
                {"deferrable", deferrable},
                {"initially_deferred", initially_deferred}
            };
        }
    };

    struct IndexMetadata {
        std::string name;                        ///< Index name
        std::string type;                        ///< btree | hash | gist | gin | brin (default: btree)
        std::vector<std::string> columns;        ///< Indexed columns in order
        bool unique = false;                     ///< UNIQUE index
        bool partial = false;                    ///< Has WHERE clause
        std::string where_clause;                ///< Partial index WHERE expression

        json toJson() const {
            return json{
                {"name", name},
                {"type", type.empty() ? "btree" : type},
                {"columns", columns},
                {"unique", unique},
                {"partial", partial},
                {"where_clause", where_clause}
            };
        }
    };

    struct TableSchema {
        std::string name = {};
        std::string schema;
        std::vector<std::string> columns;
        std::map<std::string, std::string> column_types;
        std::vector<std::string> primary_keys;

        struct ForeignKeyConstraint {
            std::string constraint_name;        ///< e.g. "fk_orders_user"
            std::vector<std::string> columns;   ///< local column(s), e.g. {"user_id"}
            std::string ref_table;              ///< referenced table, e.g. "users"
            std::vector<std::string> ref_columns; ///< referenced columns, e.g. {"id"}
            std::string on_delete;              ///< ON DELETE action, e.g. "CASCADE" (empty = RESTRICT)
            std::string on_update;              ///< ON UPDATE action, e.g. "SET NULL" (empty = RESTRICT)

            // Compatibility aliases used by older mapper/validation paths.
            std::string name;
            std::string source_column = {};
            std::string target_table = {};
            std::string target_column = {};
            std::string on_delete_action = {};
            std::string on_update_action = {};

            json toJson() const {
                auto joinCols = [](const std::vector<std::string>& v) {
                    std::string out = {};
                    for (size_t i = 0; i < v.size(); ++i) {
                        if (i > 0) {
                          out += ",";
                        }
                        out += v[i];
                    }
                    return out;
                };
                const std::string effective_name = !constraint_name.empty() ? constraint_name : name;
                const std::string effective_src = !source_column.empty() ? source_column : joinCols(columns);
                const std::string effective_tgt_table = !target_table.empty() ? target_table : ref_table;
                const std::string effective_tgt_col = !target_column.empty() ? target_column : joinCols(ref_columns);
                const std::string effective_on_delete = !on_delete_action.empty() ? on_delete_action : on_delete;
                const std::string effective_on_update = !on_update_action.empty() ? on_update_action : on_update;
                return json{
                    {"constraint_name", effective_name},
                    {"columns",         columns},
                    {"ref_table",       effective_tgt_table},
                    {"ref_columns",     ref_columns},
                    {"on_delete",       effective_on_delete},
                    {"on_update",       effective_on_update},
                    {"name",            effective_name},
                    {"source_column",   effective_src},
                    {"target_table",    effective_tgt_table},
                    {"target_column",   effective_tgt_col},
                    {"on_delete_action", effective_on_delete},
                    {"on_update_action", effective_on_update}
                };
            }
        };
        // v2.0 extensions
        std::vector<ForeignKeyConstraint> foreign_keys;       ///< Parsed FK constraints
        std::map<std::string, std::string> column_defaults;   ///< DEFAULT expressions per column
        std::map<std::string, std::string> column_constraints; ///< NOT NULL, UNIQUE inline per column
        std::vector<IndexMetadata> indexes;                   ///< Explicit indexes on this table
        std::map<std::string, std::string> custom_types;      ///< Custom type hints per column

        // v2.1 extensions
        std::vector<CheckConstraint>     check_constraints;   ///< CHECK constraint expressions
        std::vector<GeneratedColumnInfo> generated_columns;   ///< GENERATED / IDENTITY columns
        std::vector<ExcludeConstraint>   exclude_constraints; ///< EXCLUDE constraints (metadata)

        json toJson() const {
            json fk_arr = json::array();
            for (const auto& fk : foreign_keys) {
              fk_arr.push_back(fk.toJson());
            }
            json idx_arr = json::array();
            for (const auto& idx : indexes) {
              idx_arr.push_back(idx.toJson());
            }
            json ck_arr = json::array();
            for (const auto& ck : check_constraints) {
              ck_arr.push_back(ck.toJson());
            }
            json gen_arr = json::array();
            for (const auto& g : generated_columns) {
              gen_arr.push_back(g.toJson());
            }
            json excl_arr = json::array();
            for (const auto& ex : exclude_constraints) {
              excl_arr.push_back(ex.toJson());
            }
            return json{
                {"name", name},
                {"schema", schema},
                {"columns", columns},
                {"column_types", column_types},
                {"primary_keys", primary_keys},
                {"foreign_keys", fk_arr},
                {"column_defaults", column_defaults},
                {"column_constraints", column_constraints},
                {"indexes", idx_arr},
                {"custom_types", custom_types},
                {"check_constraints", ck_arr},
                {"generated_columns", gen_arr},
                {"exclude_constraints", excl_arr}
            };
        }
    };
    
    std::atomic<bool> cancelled_{false};
        mutable std::mutex custom_type_map_mutex_;  ///< Protects custom_type_map_ concurrent access
    std::unordered_map<std::string, TableSchema> schemas_;  ///< O(1) lookup by table name
    std::unordered_map<std::string, std::string> custom_type_map_;  ///< Types from CREATE TYPE
    ImportConflictResolver conflict_resolver_;  ///< In-session conflict tracker
    
    // Parsing methods
    /**
     * @brief Parse Dump File.
     * @param[in] file_path Path to the file.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in,out] callback Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseDumpFile(const std::string& file_path, const ImportOptions& options, ImportStats& stats,
                       ProgressCallback& callback);
    /**
     * @brief Parse Create Table.
     * @param[in] sql Input parameter.
     * @param[in,out] schema Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseCreateTable(const std::string& sql, TableSchema& schema);
    /**
     * @brief Parse Insert.
     * @param[in] sql Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in] line_number Input parameter.
     * @return True when the operation succeeds.
     */
    bool parseInsert(const std::string& sql, const ImportOptions& options, ImportStats& stats,
                     size_t line_number);
    /**
     * @brief Parse Copy.
     * @param[in,out] file Input/output parameter.
     * @param[in] table_name Name of the table.
     * @param[in] columns Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in,out] delta_hashes Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseCopy(std::ifstream& file, const std::string& table_name,
                   const std::vector<std::string>& columns,
                   const ImportOptions& options, ImportStats& stats,
                   std::unordered_set<uint64_t>& delta_hashes);

    /**
     * @brief v2.
     * @param[in] constraint_def Input parameter.
     * @param[in,out] schema Input/output parameter.
     * @return True when the operation succeeds.
     * @details 0: Foreign Key helpers
     */
    bool parseForeignKeyConstraint(const std::string& constraint_def,
                                   TableSchema& schema) const;

    /**
     * @brief Parse Inline Reference.
     * @param[in] col_name Name of the col.
     * @param[in] col_def Input parameter.
     * @param[in,out] schema Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseInlineReference(const std::string& col_name,
                              const std::string& col_def,
                              TableSchema& schema) const;

    /**
     * @brief Parse Alter Table Add Fk.
     * @param[in] sql Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     */
    void parseAlterTableAddFk(const std::string& sql,
                               const ImportOptions& options,
                               ImportStats& stats);
    /**
     * @brief v2.
     * @param[in] constraint_def Input parameter.
     * @param[in,out] fk Input/output parameter.
     * @return True when the operation succeeds.
     * @details 0 parser methods
     */
    bool parseForeignKeyConstraint(const std::string& constraint_def,
                                   ForeignKeyConstraint& fk);
    /**
     * @brief Parse Create Index.
     * @param[in] sql Input parameter.
     * @param[in] table_name Name of the table.
     * @param[in,out] index Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseCreateIndex(const std::string& sql,
                          const std::string& table_name,
                          IndexMetadata& index);
    /**
     * @brief Parse Alter Table Foreign Key.
     * @param[in] sql Input parameter.
     * @param[in,out] out_table Input/output parameter.
     * @param[in,out] fk Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseAlterTableForeignKey(const std::string& sql,
                                   std::string& out_table,
                                   ForeignKeyConstraint& fk);
    /**
     * @brief Validate Foreign Key References.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool validateForeignKeyReferences(const ImportOptions& options,
                                      ImportStats& stats);

    /**
     * @brief v2.
     * @param[in] constraint_def Input parameter.
     * @param[in,out] ck Input/output parameter.
     * @return True when the operation succeeds.
     * @details 1 parser methods
     */
    bool parseCheckConstraint(const std::string& constraint_def, CheckConstraint& ck);
    /**
     * @brief Parse Exclude Constraint.
     * @param[in] constraint_def Input parameter.
     * @param[in,out] excl Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseExcludeConstraint(const std::string& constraint_def, ExcludeConstraint& excl);
    /**
     * @brief Parse Generated Column.
     * @param[in] col_def Input parameter.
     * @param[in] col_name Name of the col.
     * @param[in,out] gen Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseGeneratedColumn(const std::string& col_def, const std::string& col_name,
                              GeneratedColumnInfo& gen);
    
    // Schema mapping
    /**
     * @brief Map Postgre SQLType To Themis.
     * @param[in] pg_type Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::string mapPostgreSQLTypeToThemis(const std::string& pg_type,
                                          const ImportOptions& options) const;
    /**
     * @brief Should Import Table.
     * @param[in] table_name Name of the table.
     * @param[in] options Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldImportTable(const std::string& table_name, const ImportOptions& options);
    
    // Data conversion
    /**
     * @brief Convert Row To Entity.
     * @param[in] schema Input parameter.
     * @param[in] values Input parameter.
     * @return Return value.
     */
    json convertRowToEntity(const TableSchema& schema, const std::vector<std::string>& values);

    // COPY row helpers
    /**
     * @brief Parse Copy Row.
     * @param[in] line Input parameter.
     * @return Return value.
     */
    std::vector<std::string> parseCopyRow(const std::string& line) const;
    /**
     * @brief Unescape Copy Value.
     * @param[in] val Input parameter.
     * @return Return value.
     */
    std::string unescapeCopyValue(const std::string& val) const;

    // INSERT helpers
    /**
     * @brief Parse Insert Values.
     * @param[in] values_clause Input parameter.
     * @return Return value.
     */
    std::vector<std::string> parseInsertValues(const std::string& values_clause) const;
    
    // Error helpers
    void addError(ImportStats& stats, ImportErrorCode code, ImportErrorSeverity severity,
                  const std::string& message, const std::string& location = "") const;
    
    // PHASE-2-HARDENING: Standardized PostgreSQL error reporting
    void addPostgreSQLError(ImportStats& stats, ImportErrorSeverity severity,
                           const std::string& pg_error_msg,
                           const std::string& location = "") const;

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

    /**
     * @brief UTF-8 validation helper
     * @param[in] s Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isValidUtf8(const std::string& s);

    // Checkpoint helpers
    /**
     * @brief Load Checkpoint.
     * @param[in] checkpoint_file Input parameter.
     * @param[in,out] offset Input/output parameter.
     * @param[in,out] accumulated_stats Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool loadCheckpoint(const std::string& checkpoint_file, std::streampos& offset,
                        ImportStats& accumulated_stats) const;
    /**
     * @brief Save Checkpoint.
     * @param[in] checkpoint_file Input parameter.
     * @param[in] offset Input parameter.
     * @param[in] stats Input parameter.
     */
    void saveCheckpoint(const std::string& checkpoint_file, std::streampos offset,
                        const ImportStats& stats) const;

    // Quarantine helpers
    /**
     * @brief Write Quarantine Row.
     * @param[in] quarantine_file Input parameter.
     * @param[in] table_name Name of the table.
     * @param[in] raw_row Input parameter.
     * @param[in] error Input parameter.
     */
    void writeQuarantineRow(const std::string& quarantine_file,
                            const std::string& table_name,
                            const std::string& raw_row,
                            const ImportError& error) const;

    /**
     * @brief Delta / incremental import helpers
     * @param[in] raw_row Input parameter.
     * @param[in] values Input parameter.
     * @param[in] key_columns Input parameter.
     * @param[in] schema_columns Input parameter.
     * @return Return value.
     */
    static uint64_t computeRowHash(const std::string& raw_row,
                                   const std::vector<std::string>& values,
                                   const std::vector<std::string>& key_columns,
                                   const std::vector<std::string>& schema_columns);
    /**
     * @brief Load Delta Hashes.
     * @param[in] delta_hash_file Input parameter.
     * @return Return value.
     */
    static std::unordered_set<uint64_t> loadDeltaHashes(const std::string& delta_hash_file);
    /**
     * @brief Save Delta Hashes.
     * @param[in] delta_hash_file Input parameter.
     * @param[in] hashes Input parameter.
     */
    static void saveDeltaHashes(const std::string& delta_hash_file,
                                const std::unordered_set<uint64_t>& hashes);

    // Progress reporting
    /**
     * @brief Report Progress.
     * @param[in,out] callback Input/output parameter.
     * @param[in] stage Input parameter.
     * @param[in] current Input parameter.
     * @param[in] total Input parameter.
     */
    void reportProgress(ProgressCallback& callback, const std::string& stage, size_t current, size_t total);
};

class PostgreSQLImporterPlugin : public plugins::IThemisPlugin {
public:
    PostgreSQLImporterPlugin();
    ~PostgreSQLImporterPlugin() override = default;
    
    // IThemisPlugin interface
    const char* getName() const override { return "postgres_importer"; }
    const char* getVersion() const override { return "2.0.0"; }
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

