/**
 * @file wikipedia_pipeline.cpp
 * @brief Wikipedia pipeline orchestrator implementation.
 *
 * Implements WikipediaPipeline: stage coordination, back-pressure,
 * checkpoint commits, and graceful shutdown on signal.
 */

#include "importers/wikipedia_pipeline.hpp"

#include "importers/wikipedia_transform.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace themis::importers {

WikipediaIngestionPipeline::WikipediaIngestionPipeline(WikipediaIngestionConfig config)
    : config_(std::move(config))
    , checkpoint_store_(config_.checkpoint_path) {}

bool WikipediaIngestionPipeline::initialize() {
    initialized_.store(true);
    cancel_requested_.store(false);
    return true;
}

void WikipediaIngestionPipeline::shutdown() {
    cancel_requested_.store(false);
    initialized_.store(false);
}

bool WikipediaIngestionPipeline::isInitialized() const {
    return initialized_.load();
}

void WikipediaIngestionPipeline::setConfig(const WikipediaIngestionConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    checkpoint_store_.setPath(config_.checkpoint_path);
}

const WikipediaIngestionConfig& WikipediaIngestionPipeline::config() const {
    return config_;
}

ImportStats WikipediaIngestionPipeline::runFullImport(
    const WikipediaDumpSource& source,
    const ImportOptions& options) {
    return executeImport(source, options, false);
}

ImportStats WikipediaIngestionPipeline::runIncrementalUpdate(
    const WikipediaDumpSource& source,
    const ImportOptions& options) {
    return executeImport(source, options, true);
}

ImportStats WikipediaIngestionPipeline::executeImport(
    const WikipediaDumpSource& source,
    const ImportOptions& options,
    bool incremental) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load()) {
        (void)initialize();
    }

    cancel_requested_.store(false);
    syncCheckpointStore(options);
    last_source_ = source;

    if (checkpoint_store_.exists()) {
        WikipediaCheckpointState state = checkpoint_store_.load();
        if (state.source_path == source.source_path || state.source_path.empty()) {
            checkpoint_state_ = std::move(state);
        }
    }

    ImportStats stats;
    std::ifstream stream(source.source_path);
    if (!stream.is_open()) {
        stats.errors.emplace_back("unable to open Wikipedia dump");
        stats.structured_errors.push_back({
            ImportErrorCode::FILE_OPEN_FAILED,
            ImportErrorSeverity::CRITICAL,
            "unable to open Wikipedia dump",
            source.source_path
        });
        return stats;
    }

    const bool success = parseSourceStream(stream, source, stats, options, incremental);
    if (!success && (config_.strict_mode || !config_.best_effort || !options.continue_on_error)) {
        stats.errors.emplace_back("Wikipedia import aborted in strict mode");
    }

    if (!options.dry_run) {
        auto projection_summary = rebuildAllProjectionsUnlocked();
        stats.relationships_processed = projection_summary.graph_edges;
    }

    checkpoint_state_.source_path = source.source_path;
    checkpoint_state_.source_id = source.source_id;
    checkpoint_state_.imported_pages = stats.imported_records;
    checkpoint_state_.failed_pages = stats.failed_records;
    checkpoint_state_.updated_at = nowIso8601();
    (void)checkpoint_store_.save(checkpoint_state_);

    return stats;
}

WikipediaProjectionSummary WikipediaIngestionPipeline::rebuildProjection(WikipediaProjectionModel model) {
    std::lock_guard<std::mutex> lock(mutex_);
    WikipediaProjectionSummary summary;
    summary.relational_rows = relationalRowCount();
    switch (model) {
        case WikipediaProjectionModel::RELATIONAL_CORE:
            return summary;
        case WikipediaProjectionModel::GRAPH:
            return projectGraphDirtyPages();
        case WikipediaProjectionModel::VECTOR:
            return projectVectorDirtyPages();
        case WikipediaProjectionModel::PROCESS:
            return projectProcessDirtyPages();
        case WikipediaProjectionModel::TIMESERIES:
            return projectTimeSeriesDirtyPages();
    }
    return summary;
}

WikipediaProjectionSummary WikipediaIngestionPipeline::rebuildAllProjections() {
    std::lock_guard<std::mutex> lock(mutex_);
    return rebuildAllProjectionsUnlocked();
}

WikipediaProjectionSummary WikipediaIngestionPipeline::rebuildAllProjectionsUnlocked() {
    WikipediaProjectionSummary summary;
    summary.relational_rows = relationalRowCount();

    if (config_.enable_graph_projection) {
        auto graph = projectGraphDirtyPages();
        summary.graph_edges = graph.graph_edges;
    }
    if (config_.enable_vector_projection) {
        auto vector = projectVectorDirtyPages();
        summary.vector_records = vector.vector_records;
    }
    if (config_.enable_process_projection) {
        auto process = projectProcessDirtyPages();
        summary.process_events = process.process_events;
    }
    if (config_.enable_timeseries_projection) {
        auto timeseries = projectTimeSeriesDirtyPages();
        summary.timeseries_points = timeseries.timeseries_points;
    }

    summary.dirty_pages_cleared = snapshot_.dirty_pages.size();
    snapshot_.dirty_pages.clear();
    return summary;
}

const WikipediaDatasetSnapshot& WikipediaIngestionPipeline::snapshot() const {
    return snapshot_;
}

const WikipediaCheckpointState& WikipediaIngestionPipeline::checkpointState() const {
    return checkpoint_state_;
}

const WikipediaManifest& WikipediaIngestionPipeline::lastManifest() const {
    return last_manifest_;
}

void WikipediaIngestionPipeline::cancel() {
    cancel_requested_.store(true);
}

std::vector<WikipediaRevisionRecord> WikipediaIngestionPipeline::revisionsForPage([[maybe_unused]] uint64_t page_id) const {
    std::vector<WikipediaRevisionRecord> revisions;
    for (const auto& [_, revision] : snapshot_.revisions) {
        if (revision.page_id == page_id) {
            revisions.push_back(revision);
        }
    }
    std::sort(revisions.begin(), revisions.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.timestamp < rhs.timestamp;
    });
    return revisions;
}

size_t WikipediaIngestionPipeline::relationalRowCount() const {
    return snapshot_.pages.size() + snapshot_.revisions.size() + snapshot_.links.size() +
        snapshot_.categories.size() + snapshot_.redirects.size() + snapshot_.dead_letters.size();
}

void WikipediaIngestionPipeline::removeExistingPageDerivedRows([[maybe_unused]] uint64_t page_id) {
    auto remove_by_page = [page_id](const auto& row) {
        return row.page_id == page_id;
    };
    snapshot_.links.erase(
        std::remove_if(snapshot_.links.begin(), snapshot_.links.end(),
            [page_id](const WikipediaLinkRecord& row) { return row.from_page_id == page_id; }),
        snapshot_.links.end());
    snapshot_.categories.erase(
        std::remove_if(snapshot_.categories.begin(), snapshot_.categories.end(), remove_by_page),
        snapshot_.categories.end());
    snapshot_.redirects.erase(
        std::remove_if(snapshot_.redirects.begin(), snapshot_.redirects.end(),
            [page_id](const WikipediaRedirectRecord& row) { return row.from_page_id == page_id; }),
        snapshot_.redirects.end());
}

void WikipediaIngestionPipeline::markDirtyPage(uint64_t page_id, const std::string& reason) {
    snapshot_.dirty_pages[page_id] = reason;
}

void WikipediaIngestionPipeline::recordDeadLetter(
    const WikipediaDeadLetterRecord& record,
    const ImportOptions& options) {
    snapshot_.dead_letters.push_back(record);
    const std::string sink_path = !options.quarantine_file.empty()
        ? options.quarantine_file
        : config_.dead_letter_path;
    if (!sink_path.empty()) {
        std::filesystem::create_directories(std::filesystem::path(sink_path).parent_path());
        std::ofstream sink(sink_path, std::ios::app);
        if (sink.is_open()) {
            sink << record.toJson().dump() << '\n';
        }
    }
}

std::string WikipediaIngestionPipeline::nowIso8601() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif
    std::ostringstream output;
    output << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}

void WikipediaIngestionPipeline::syncCheckpointStore(const ImportOptions& options) {
    if (!options.checkpoint_file.empty()) {
        checkpoint_store_.setPath(options.checkpoint_file);
    } else {
        checkpoint_store_.setPath(config_.checkpoint_path);
    }
}

} // namespace themis::importers
