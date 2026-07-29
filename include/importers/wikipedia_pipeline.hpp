/**
 * @file wikipedia_pipeline.hpp
 * @brief End-to-end Wikipedia ingestion pipeline orchestrator.
 *
 * Wires together the XML reader, transformer, validator, and ThemisDB
 * ingest client into a resumable, checkpointed import pipeline.
 */

#pragma once

#include "importers/importer_interface.h"
#include "importers/wikipedia_checkpoint.hpp"
#include "importers/wikipedia_config.hpp"
#include "importers/wikipedia_types.hpp"

#include <atomic>
#include <istream>
#include <mutex>
#include <optional>
#include <string>

namespace themis::importers {

/**
 * @brief Streaming Wikipedia ingestion pipeline with canonical-core + projection rebuild support.
 *
 * The pipeline consumes Wikimedia dump input incrementally, upserts into an
 * in-memory canonical relational core, tracks dirty pages for delta refreshes,
 * rebuilds graph/vector/process/timeseries projections, validates integrity,
 * and writes a portable `wikipedia.db` artifact plus sidecar manifest.
 */
class WikipediaIngestionPipeline {
public:
    explicit WikipediaIngestionPipeline(WikipediaIngestionConfig config = {});

    [[nodiscard]] bool initialize();
    void shutdown();
    [[nodiscard]] bool isInitialized() const;

    void setConfig(const WikipediaIngestionConfig& config);
    [[nodiscard]] const WikipediaIngestionConfig& config() const;

    [[nodiscard]] ImportStats runFullImport(
        const WikipediaDumpSource& source,
        const ImportOptions& options = ImportOptions{});
    [[nodiscard]] ImportStats runIncrementalUpdate(
        const WikipediaDumpSource& source,
        const ImportOptions& options = ImportOptions{});
    [[nodiscard]] WikipediaProjectionSummary rebuildProjection(WikipediaProjectionModel model);
    [[nodiscard]] WikipediaProjectionSummary rebuildAllProjections();
    [[nodiscard]] WikipediaValidationReport validate() const;
    [[nodiscard]] WikipediaManifest exportPortable(
        const std::string& database_path,
        const std::string& manifest_path = std::string{});
    [[nodiscard]] json sourceSchema() const;

    [[nodiscard]] const WikipediaDatasetSnapshot& snapshot() const;
    [[nodiscard]] const WikipediaCheckpointState& checkpointState() const;
    [[nodiscard]] const WikipediaManifest& lastManifest() const;

    void cancel();

private:
    [[nodiscard]] ImportStats executeImport(
        const WikipediaDumpSource& source,
        const ImportOptions& options,
        bool incremental);
    [[nodiscard]] WikipediaProjectionSummary rebuildAllProjectionsUnlocked();
    [[nodiscard]] bool parseSourceStream(
        std::istream& stream,
        const WikipediaDumpSource& source,
        ImportStats& stats,
        const ImportOptions& options,
        bool incremental);
    [[nodiscard]] std::optional<WikipediaParsedPage> parseXmlPageBlock(
        const std::string& page_block,
        const WikipediaDumpSource& source,
        std::string& error) const;
    void applyParsedPage(
        const WikipediaParsedPage& parsed_page,
        ImportStats& stats,
        const ImportOptions& options,
        bool incremental);
    void removeExistingPageDerivedRows(uint64_t page_id);
    void markDirtyPage(uint64_t page_id, const std::string& reason);
    [[nodiscard]] std::vector<WikipediaRevisionRecord> revisionsForPage(uint64_t page_id) const;
    [[nodiscard]] WikipediaProjectionSummary projectGraphDirtyPages();
    [[nodiscard]] WikipediaProjectionSummary projectVectorDirtyPages();
    [[nodiscard]] WikipediaProjectionSummary projectTimeSeriesDirtyPages();
    [[nodiscard]] WikipediaProjectionSummary projectProcessDirtyPages();
    [[nodiscard]] size_t relationalRowCount() const;
    [[nodiscard]] WikipediaValidationReport validateUnlocked() const;
    void recordDeadLetter(const WikipediaDeadLetterRecord& record, const ImportOptions& options);
    [[nodiscard]] std::string nowIso8601() const;
    void syncCheckpointStore(const ImportOptions& options);

    WikipediaIngestionConfig config_;
    WikipediaDatasetSnapshot snapshot_;
    WikipediaDumpSource last_source_;
    WikipediaManifest last_manifest_;
    WikipediaCheckpointState checkpoint_state_;
    WikipediaCheckpointStore checkpoint_store_;
    mutable std::mutex mutex_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> cancel_requested_{false};
};

} // namespace themis::importers
