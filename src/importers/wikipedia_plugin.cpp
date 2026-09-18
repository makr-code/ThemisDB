/**
 * @file wikipedia_plugin.cpp
 * @brief Wikipedia importer plugin registration implementation.
 *
 * Implements WikipediaPlugin::create() and the plugin factory function
 * used by the dynamic plugin loader.
 */

#include "importers/wikipedia_plugin.hpp"

#include "plugins/plugin_registry.h"

#include <chrono>
#include <fstream>
#include <future>
#include <iterator>
#include <sstream>
#include <thread>

namespace themis::importers {

namespace {
constexpr const char* kWikipediaPluginName = "wikipedia_ingest";
constexpr const char* kWikipediaPluginVersion = "0.1.0";
} // namespace

WikipediaIngestionPlugin::WikipediaIngestionPlugin(WikipediaIngestionConfig config)
    : config_(std::move(config))
    , pipeline_(config_) {}

WikipediaIngestionPlugin::~WikipediaIngestionPlugin() {
    shutdown();
}

const char* WikipediaIngestionPlugin::getName() const {
    return kWikipediaPluginName;
}

const char* WikipediaIngestionPlugin::getVersion() const {
    return kWikipediaPluginVersion;
}

std::vector<std::string> WikipediaIngestionPlugin::getSupportedTypes() const {
    return {"wikipedia", "wikipedia-xml", "wikipedia-dump"};
}

plugins::PluginCapabilities WikipediaIngestionPlugin::getCapabilities() const {
    plugins::PluginCapabilities capabilities;
    capabilities.supports_streaming = true;
    capabilities.supports_batching = true;
    capabilities.thread_safe = true;
    return capabilities;
}

/**
 * @brief Initialize.
 * @param[in] config Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: empty(), WikipediaIngestionConfig::fromJson(), json::parse(), setConfig(), init().
 */
bool WikipediaIngestionPlugin::initialize(const std::string& config) {
    if (!config.empty()) {
        try {
            config_ = WikipediaIngestionConfig::fromJson(json::parse(config));
            pipeline_.setConfig(config_);
        } catch (...) {
            return false;
        }
    }
    return init();
}

/**
 * @brief Validate Source.
 * @param[in] source_path Path to the source.
 * @param[in,out] errors Input/output parameter.
 * @return True when the operation succeeds.
 * @details Calls: empty(), emplace_back(), stream(), is_open(), header(), find().
 */
bool WikipediaIngestionPlugin::validateSource(
    const std::string& source_path,
    std::vector<std::string>& errors) {
    if (source_path.empty()) {
        errors.emplace_back("source_path must not be empty");
        return false;
    }

    std::ifstream stream(source_path);
    if (!stream.is_open()) {
        errors.emplace_back("source_path is not readable");
        return false;
    }

    std::string header((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (header.find("<mediawiki") == std::string::npos &&
        header.find("<page>") == std::string::npos) {
        errors.emplace_back("source does not look like a Wikimedia XML dump");
        return false;
    }

    return true;
}

/**
 * @brief Import Data.
 * @param[in] source_path Path to the source.
 * @param[in] options Input parameter.
 * @param[in] cb Input parameter.
 * @return Return value.
 * @details Calls: permission_check(), emplace_back(), push_back(), init(), cb(), empty(), runIncrementalUpdate(), runFullImport().
 */
ImportStats WikipediaIngestionPlugin::importData(
    const std::string& source_path,
    const ImportOptions& options,
    ProgressCallback cb) {
    if (options.permission_check && !options.permission_check("import", "write")) {
        ImportStats stats;
        stats.errors.emplace_back("permission denied for Wikipedia import");
        stats.structured_errors.push_back({
            ImportErrorCode::PERMISSION_DENIED,
            ImportErrorSeverity::CRITICAL,
            "permission denied for Wikipedia import",
            source_path
        });
        return stats;
    }

    if (!init()) {
        ImportStats stats;
        stats.errors.emplace_back("failed to initialize Wikipedia importer");
        return stats;
    }

    WikipediaDumpSource source;
    source.source_path = source_path;
    source.source_id = source_path;
    source.producer_hint = "direct-dump";

    if (cb) {
        cb("wikipedia.import.start", 0, 0);
    }

    ImportStats stats = options.update_existing || !options.delta_hash_file.empty()
        ? runIncrementalUpdate(source, options)
        : runFullImport(source, options);

    if (cb) {
        cb("wikipedia.import.finish", stats.imported_records + stats.skipped_records, stats.total_records);
    }

    return stats;
}

/**
 * @brief Import Data Async.
 * @param[in] source_path Path to the source.
 * @param[in] options Input parameter.
 * @return Return value.
 * @details Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), str(), store(), setStage(), get_future(), share().
 */
std::shared_ptr<ImportHandle> WikipediaIngestionPlugin::importDataAsync(
    const std::string& source_path,
    const ImportOptions& options) {
    auto handle = std::make_shared<ImportHandle>();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::ostringstream id = {};
    id << kWikipediaPluginName << '-' << now_ms;
    handle->id = id.str();
    handle->source_path = source_path;
    handle->started_at_ms = now_ms;
    handle->running.store(true);
    handle->setStage("starting");

    auto promise = std::make_shared<std::promise<ImportStats>>();
    handle->future = promise->get_future().share();

    std::thread([this, handle, promise, source_path, options]() mutable {
        try {
            handle->setStage("importing");
            ImportStats stats = importData(source_path, options);
            handle->current_records.store(stats.imported_records + stats.skipped_records);
            handle->total_records.store(stats.total_records);
            handle->running.store(false);
            handle->finished_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            handle->setStage("completed");
            promise->set_value(stats);
        } catch (const std::exception& ex) {
            ImportStats stats;
            stats.errors.emplace_back(ex.what());
            stats.failed_records = 1;
            handle->running.store(false);
            handle->finished_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            handle->setStage("failed");
            promise->set_value(stats);
        }
    }).detach();

    return handle;
}

/**
 * @brief Cancel.
 * @details Calls: store().
 */
void WikipediaIngestionPlugin::cancel() {
    cancel_requested_.store(true);
    pipeline_.cancel();
}

/**
 * @brief Get Source Schema.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: sourceSchema().
 */
json WikipediaIngestionPlugin::getSourceSchema(const std::string& /*source_path*/) {
    return pipeline_.sourceSchema();
}

/**
 * @brief Shutdown.
 * @details Implements shutdown without additional internal calls.
 */
void WikipediaIngestionPlugin::shutdown() {
    pipeline_.shutdown();
}

/**
 * @brief Init.
 * @return True when the operation succeeds.
 * @details Calls: initialize().
 */
bool WikipediaIngestionPlugin::init() {
    return pipeline_.initialize();
}

/**
 * @brief Run Full Import.
 * @param[in] source Input parameter.
 * @param[in] options Input parameter.
 * @return Return value.
 * @details Implements runFullImport without additional internal calls.
 */
ImportStats WikipediaIngestionPlugin::runFullImport(
    const WikipediaDumpSource& source,
    const ImportOptions& options) {
    return pipeline_.runFullImport(source, options);
}

/**
 * @brief Run Incremental Update.
 * @param[in] source Input parameter.
 * @param[in] options Input parameter.
 * @return Return value.
 * @details Implements runIncrementalUpdate without additional internal calls.
 */
ImportStats WikipediaIngestionPlugin::runIncrementalUpdate(
    const WikipediaDumpSource& source,
    const ImportOptions& options) {
    return pipeline_.runIncrementalUpdate(source, options);
}

/**
 * @brief Rebuild Projection.
 * @param[in] model Input parameter.
 * @return Return value.
 * @details Implements rebuildProjection without additional internal calls.
 */
WikipediaProjectionSummary WikipediaIngestionPlugin::rebuildProjection(WikipediaProjectionModel model) {
    return pipeline_.rebuildProjection(model);
}

WikipediaValidationReport WikipediaIngestionPlugin::validateDatabase() const {
    return pipeline_.validate();
}

/**
 * @brief Export Portable.
 * @param[in] database_path Path to the database.
 * @param[in] manifest_path Path to the manifest.
 * @return Return value.
 * @details Implements exportPortable without additional internal calls.
 */
WikipediaManifest WikipediaIngestionPlugin::exportPortable(
    const std::string& database_path,
    const std::string& manifest_path) {
    return pipeline_.exportPortable(database_path, manifest_path);
}

const WikipediaIngestionPipeline& WikipediaIngestionPlugin::pipeline() const {
    return pipeline_;
}

/**
 * @brief Pipeline.
 * @return Return value.
 * @details Implements pipeline without additional internal calls.
 */
WikipediaIngestionPipeline& WikipediaIngestionPlugin::pipeline() {
    return pipeline_;
}

/**
 * @brief Register Plugin.
 * @details Calls: ImporterPluginRegistry::instance(), registerFactory().
 */
void WikipediaIngestionPlugin::registerPlugin() {
    ImporterPluginRegistry::instance().registerFactory(
        kWikipediaPluginName,
        []() -> std::shared_ptr<IImporter> {
            return std::make_shared<WikipediaIngestionPlugin>();
        });
    plugins::PluginRegistry::registerFactory<IImporter>(
        kWikipediaPluginName,
        []() {
            return std::make_unique<WikipediaIngestionPlugin>();
        });
}

/**
 * @brief Unregister Plugin.
 * @details Calls: ImporterPluginRegistry::instance(), unregisterFactory().
 */
void WikipediaIngestionPlugin::unregisterPlugin() {
    ImporterPluginRegistry::instance().unregisterFactory(kWikipediaPluginName);
    plugins::PluginRegistry::unregisterFactory<IImporter>(kWikipediaPluginName);
}

namespace {
const bool kWikipediaPluginRegistered = []() {
    WikipediaIngestionPlugin::registerPlugin();
    return true;
}();
} // namespace

} // namespace themis::importers
