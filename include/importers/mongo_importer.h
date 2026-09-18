/**
 * @file mongo_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/importer_interface.h"
#include "plugins/plugin_interface.h"
#include <atomic>

namespace themis {
namespace importers {

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

    std::string configured_collection_;

    // -----------------------------------------------------------------------
    // Parsing helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Parse Json Lines.
     * @param[in] file_path Path to the file.
     * @param[in] collection Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in,out] callback Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseJsonLines(const std::string& file_path,
                        const std::string& collection,
                        const ImportOptions& options,
                        ImportStats& stats,
                        ProgressCallback& callback);

    /**
     * @brief Parse Json Array.
     * @param[in] file_path Path to the file.
     * @param[in] collection Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in,out] callback Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseJsonArray(const std::string& file_path,
                        const std::string& collection,
                        const ImportOptions& options,
                        ImportStats& stats,
                        ProgressCallback& callback);

    /**
     * @brief Import Document.
     * @param[in] doc Input parameter.
     * @param[in] collection Input parameter.
     * @param[in] options Input parameter.
     * @param[in,out] stats Input/output parameter.
     * @param[in] doc_index Input parameter.
     * @return True when the operation succeeds.
     */
    bool importDocument(const json& doc,
                        const std::string& collection,
                        const ImportOptions& options,
                        ImportStats& stats,
                        size_t doc_index);

    /**
     * @brief ----------------------------------------------------------------------- Type mapping / BSON extended JSON helpers -----------------------------------------------------------------------
     * @param[in] value Input parameter.
     * @return Return value.
     */

    static std::string inferThemisType(const json& value);

    /**
     * @brief Unwrap Bson Value.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static json unwrapBsonValue(const json& value);

    /**
     * @brief Unwrap Document.
     * @param[in] doc Input parameter.
     * @return Return value.
     */
    static json unwrapDocument(const json& doc);

    // -----------------------------------------------------------------------
    // Utility helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Collection From Path.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static std::string collectionFromPath(const std::string& path);

    /**
     * @brief Should Import Collection.
     * @param[in] collection Input parameter.
     * @param[in] options Input parameter.
     * @return True when the operation succeeds.
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

    /**
     * @brief Report Progress.
     * @param[in,out] callback Input/output parameter.
     * @param[in] stage Input parameter.
     * @param[in] current Input parameter.
     * @param[in] total Input parameter.
     */
    void reportProgress(ProgressCallback& callback,
                        const std::string& stage,
                        size_t current, size_t total);
};

// ---------------------------------------------------------------------------
// Plugin wrapper
// ---------------------------------------------------------------------------

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
