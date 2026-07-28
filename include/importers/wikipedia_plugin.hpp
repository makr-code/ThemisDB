/**
 * @file wikipedia_plugin.hpp
 * @brief Plugin entry-point for the Wikipedia importer.
 *
 * Declares WikipediaPlugin that fulfils the ThemisDB IImportPlugin
 * interface, making the Wikipedia importer loadable at runtime.
 */

#pragma once

#include "importers/importer_plugin_api.h"
#include "importers/wikipedia_config.hpp"
#include "importers/wikipedia_pipeline.hpp"

#include <atomic>
#include <memory>

namespace themis::importers {

/**
 * @brief Built-in Wikipedia importer plugin for full import, delta refresh, validation, and export.
 *
 * Public lifecycle methods (`init`, `shutdown`) initialize the pipeline.
 * `runFullImport` and `runIncrementalUpdate` update the canonical relational
 * core, while `rebuildProjection`, `validateDatabase`, and `exportPortable`
 * expose the multi-model transformation and portable-artifact workflow.
 */
class WikipediaIngestionPlugin final : public ImporterPluginBase {
public:
    explicit WikipediaIngestionPlugin(WikipediaIngestionConfig config = {});
    ~WikipediaIngestionPlugin() override;

    [[nodiscard]] const char* getName() const override;
    [[nodiscard]] const char* getVersion() const override;
    [[nodiscard]] std::vector<std::string> getSupportedTypes() const override;
    [[nodiscard]] plugins::PluginCapabilities getCapabilities() const override;

    [[nodiscard]] bool initialize(const std::string& config) override;
    [[nodiscard]] bool validateSource(
        const std::string& source_path,
        std::vector<std::string>& errors) override;
    [[nodiscard]] ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback cb = nullptr) override;
    [[nodiscard]] std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options) override;
    void cancel() override;
    [[nodiscard]] json getSourceSchema(const std::string& source_path) override;
    void shutdown() override;

    [[nodiscard]] bool init();
    [[nodiscard]] ImportStats runFullImport(
        const WikipediaDumpSource& source,
        const ImportOptions& options = ImportOptions{});
    [[nodiscard]] ImportStats runIncrementalUpdate(
        const WikipediaDumpSource& source,
        const ImportOptions& options = ImportOptions{});
    [[nodiscard]] WikipediaProjectionSummary rebuildProjection(WikipediaProjectionModel model);
    [[nodiscard]] WikipediaValidationReport validateDatabase() const;
    [[nodiscard]] WikipediaManifest exportPortable(
        const std::string& database_path,
        const std::string& manifest_path = std::string{});

    [[nodiscard]] const WikipediaIngestionPipeline& pipeline() const;
    [[nodiscard]] WikipediaIngestionPipeline& pipeline();

    static void registerPlugin();
    static void unregisterPlugin();

private:
    WikipediaIngestionConfig config_;
    WikipediaIngestionPipeline pipeline_;
    std::atomic<bool> cancel_requested_{false};
};

} // namespace themis::importers
