/**
 * @file mongo_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/importer_interface.h"
#include "plugins/plugin_interface.h"
#include <atomic>

namespace themis {
namespace importers {

/**
 * @brief MongoDB mongoexport Importer
 *
 * Imports document collections exported by `mongoexport`.
 * Supports:
 * - JSON-Lines (NDJSON) format: one document per line (default mongoexport output)
 * - JSON array format: a single top-level array of documents (--jsonArray flag)
 * - BSON extended JSON v2 type wrappers ($oid, $date, $numberDecimal, $numberLong,
 *   $numberInt, $numberDouble, $binary, $timestamp, $regex, $undefined, $minKey,
 *   $maxKey, $dbPointer, $code, $ref)
 * - Schema mapping to ThemisDB BaseEntity
 * - Type inference from JSON values
 * - Batch processing
 * - Async import via importDataAsync()
 * - Structured error reporting (ImportErrorCode)
 * - Observability: metrics and tracing callbacks
 * - Permission-check callback (ACL enforcement)
 * - include/exclude collection (table) filtering
 * - Dry-run mode
 *
 * The "collection name" is derived from the optional "collection" field in the
 * import configuration JSON, or from the base filename (without extension) of the
 * source file when no explicit name is provided.
 *
 * Example usage:
 * @code
 *   MongoDBImporter importer;
 *   importer.initialize(R"({"collection":"users"})");
 *
 *   ImportOptions opts;
 *   opts.batch_size = 500;
 *   ImportStats stats = importer.importData("/path/to/users.json", opts);
 * @endcode
 */
class MongoDBImporter : public IImporter {
public:
    MongoDBImporter();
    ~MongoDBImporter() override;

    // IImporter interface
    const char* getName() const override { return "MongoDB Importer"; }
    std::vector<std::string> getSupportedTypes() const override;
    bool initialize(const std::string& config) override;
    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override;
    ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr
    ) override;
    std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options
    ) override;
    void cancel() override;
    json getSourceSchema(const std::string& source_path) override;

private:
    std::atomic<bool> cancelled_{false};

    /// Collection name set via initialize() config JSON ("collection" key).
    /// When empty, the base filename of the source path is used.
    std::string configured_collection_;

    // -----------------------------------------------------------------------
    // Parsing helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Import all documents from a JSON-Lines (NDJSON) file.
     *
     * Each non-empty line must be a single JSON object.
     */
    bool parseJsonLines(const std::string& file_path,
                        const std::string& collection,
                        const ImportOptions& options,
                        ImportStats& stats,
                        ProgressCallback& callback);

    /**
     * @brief Import all documents from a JSON array file.
     *
     * The file content must be a single JSON array whose elements are objects.
     */
    bool parseJsonArray(const std::string& file_path,
                        const std::string& collection,
                        const ImportOptions& options,
                        ImportStats& stats,
                        ProgressCallback& callback);

    /**
     * @brief Import a single parsed JSON document.
     *
     * Applies BSON extended-JSON unwrapping, table filtering, dry-run guard,
     * and metrics emission.  Updates stats in place.
     *
     * @return true if the document was accepted (imported or skipped by policy),
     *         false on a hard parse/conversion error.
     */
    bool importDocument(const json& doc,
                        const std::string& collection,
                        const ImportOptions& options,
                        ImportStats& stats,
                        size_t doc_index);

    // -----------------------------------------------------------------------
    // Type mapping / BSON extended JSON helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Map a JSON value's type to a ThemisDB logical type string.
     *
     * Handles BSON extended JSON v2 wrappers and plain JSON primitives.
     */
    static std::string inferThemisType(const json& value);

    /**
     * @brief Unwrap BSON extended JSON v2 type wrappers to scalar values.
     *
     * Converts objects like {"$oid":"..."}, {"$date":{"$numberLong":"..."}},
     * {"$numberDecimal":"..."} into plain JSON scalars or strings.
     *
     * Non-BSON objects are returned unchanged.
     */
    static json unwrapBsonValue(const json& value);

    /**
     * @brief Recursively unwrap all BSON extended JSON values in a document.
     */
    static json unwrapDocument(const json& doc);

    // -----------------------------------------------------------------------
    // Utility helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Derive a collection name from a file path (basename without extension).
     */
    static std::string collectionFromPath(const std::string& path);

    /**
     * @brief Check whether a collection should be imported given the options.
     */
    static bool shouldImportCollection(const std::string& collection,
                                       const ImportOptions& options);

    // -----------------------------------------------------------------------
    // Observability helpers (mirror MySQL/PostgreSQL pattern)
    // -----------------------------------------------------------------------

    void addError(ImportStats& stats,
                  ImportErrorCode code,
                  ImportErrorSeverity severity,
                  const std::string& message,
                  const std::string& location = "") const;

    void emitMetric(const ImportOptions& options,
                    const std::string& metric,
                    const std::map<std::string, std::string>& labels,
                    double value) const;

    void emitSpan(const ImportOptions& options,
                  const std::string& operation,
                  const std::map<std::string, std::string>& attributes,
                  double duration_seconds) const;

    void reportProgress(ProgressCallback& callback,
                        const std::string& stage,
                        size_t current, size_t total);
};

// ---------------------------------------------------------------------------
// Plugin wrapper
// ---------------------------------------------------------------------------

/**
 * @brief MongoDB Importer Plugin
 *
 * Wraps MongoDBImporter as a ThemisDB plugin.
 */
class MongoDBImporterPlugin : public plugins::IThemisPlugin {
public:
    MongoDBImporterPlugin();
    ~MongoDBImporterPlugin() override = default;

    // IThemisPlugin interface
    const char* getName()    const override { return "mongo_importer"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginType getType() const override {
        return plugins::PluginType::IMPORTER;
    }
    plugins::PluginCapabilities getCapabilities() const override;
    bool initialize(const char* config_json) override;
    void shutdown() override;
    void* getInstance() override { return importer_.get(); }

private:
    std::unique_ptr<MongoDBImporter> importer_;
};

} // namespace importers
} // namespace themis
