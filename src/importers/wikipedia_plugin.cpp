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

std::shared_ptr<ImportHandle> WikipediaIngestionPlugin::importDataAsync(
    const std::string& source_path,
    const ImportOptions& options) {
    auto handle = std::make_shared<ImportHandle>();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::ostringstream id;
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

void WikipediaIngestionPlugin::cancel() {
    cancel_requested_.store(true);
    pipeline_.cancel();
}

json WikipediaIngestionPlugin::getSourceSchema(const std::string& /*source_path*/) {
    return pipeline_.sourceSchema();
}

void WikipediaIngestionPlugin::shutdown() {
    pipeline_.shutdown();
}

bool WikipediaIngestionPlugin::init() {
    return pipeline_.initialize();
}

ImportStats WikipediaIngestionPlugin::runFullImport(
    const WikipediaDumpSource& source,
    const ImportOptions& options) {
    return pipeline_.runFullImport(source, options);
}

ImportStats WikipediaIngestionPlugin::runIncrementalUpdate(
    const WikipediaDumpSource& source,
    const ImportOptions& options) {
    return pipeline_.runIncrementalUpdate(source, options);
}

WikipediaProjectionSummary WikipediaIngestionPlugin::rebuildProjection(WikipediaProjectionModel model) {
    return pipeline_.rebuildProjection(model);
}

WikipediaValidationReport WikipediaIngestionPlugin::validateDatabase() const {
    return pipeline_.validate();
}

WikipediaManifest WikipediaIngestionPlugin::exportPortable(
    const std::string& database_path,
    const std::string& manifest_path) {
    return pipeline_.exportPortable(database_path, manifest_path);
}

const WikipediaIngestionPipeline& WikipediaIngestionPlugin::pipeline() const {
    return pipeline_;
}

WikipediaIngestionPipeline& WikipediaIngestionPlugin::pipeline() {
    return pipeline_;
}

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
