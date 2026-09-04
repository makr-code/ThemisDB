/**
 * @file wikipedia_dump_reader.cpp
 * @brief Streaming XML reader for Wikipedia database dumps.
 *
 * Implements low-level XML parsing of bz2-compressed Wikipedia dump files,
 * yielding raw page records to the transformation pipeline.
 */

#include "importers/wikipedia_pipeline.hpp"

#include <string>

namespace themis::importers {

bool WikipediaIngestionPipeline::parseSourceStream(
    std::istream& stream,
    const WikipediaDumpSource& source,
    ImportStats& stats,
    const ImportOptions& options,
    bool incremental) {
    std::string line = {};
    std::string page_block = {};
    bool inside_page = false;
    size_t page_index = 0;
    const size_t resume_pages = (incremental &&
        checkpoint_state_.source_path == source.source_path)
        ? checkpoint_state_.processed_pages
        : 0;

    while (std::getline(stream, line)) {
        if (cancel_requested_.load()) {
            stats.warnings.emplace_back("Wikipedia import cancelled before completion");
            return false;
        }

        if (!inside_page) {
            if (line.find("<page>") != std::string::npos) {
                inside_page = true;
                page_block.clear();
                page_block.append(line).push_back('\n');
            }
            continue;
        }

        page_block.append(line).push_back('\n');
        if (line.find("</page>") == std::string::npos) {
            continue;
        }

        inside_page = false;
        ++page_index;

        if (resume_pages > 0 && page_index <= resume_pages) {
            checkpoint_state_.processed_pages = page_index;
            continue;
        }

        std::string error = {};
        auto parsed_page = parseXmlPageBlock(page_block, source, error);
        if (!parsed_page.has_value()) {
            ++stats.failed_records;
            stats.structured_errors.push_back({
                ImportErrorCode::PARSE_COPY_ROW,
                config_.strict_mode ? ImportErrorSeverity::CRITICAL : ImportErrorSeverity::ERROR,
                error,
                source.source_path
            });
            recordDeadLetter({source.source_path, error, page_block.substr(0, 512)}, options);
            if (config_.strict_mode || !config_.best_effort || !options.continue_on_error) {
                return false;
            }
            continue;
        }

        applyParsedPage(*parsed_page, stats, options, incremental);
        checkpoint_state_.processed_pages = page_index;
        checkpoint_state_.last_page_id = parsed_page->page.page_id;
        checkpoint_state_.last_page_title = parsed_page->page.title;
        checkpoint_state_.processed_revisions = snapshot_.revisions.size();
        checkpoint_state_.updated_at = nowIso8601();

        if (checkpoint_store_.hasPath() &&
            config_.checkpoint_interval_pages > 0 &&
            (page_index % config_.checkpoint_interval_pages == 0)) {
            (void)checkpoint_store_.save(checkpoint_state_);
        }
    }

    return true;
}

} // namespace themis::importers
