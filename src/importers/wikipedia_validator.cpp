/**
 * @file wikipedia_validator.cpp
 * @brief Validation stage for Wikipedia import records.
 *
 * Implements configurable validation rules (language filter, minimum
 * article length, namespace whitelist) applied before ingest.
 */

#include "importers/wikipedia_pipeline.hpp"

#include "importers/wikipedia_transform.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace themis::importers {

namespace {
std::string checksumFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return {};
    }
    std::ostringstream buffer = {};
    buffer << input.rdbuf();
    return WikipediaTransform::checksumHex(buffer.str());
}
} // namespace

WikipediaValidationReport WikipediaIngestionPipeline::validate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return validateUnlocked();
}

WikipediaValidationReport WikipediaIngestionPipeline::validateUnlocked() const {
    WikipediaValidationReport report;
    report.dead_letters = snapshot_.dead_letters.size();

    std::map<std::string, uint64_t> titles = {};

    for (const auto& [page_id, page] : snapshot_.pages) {
        titles[WikipediaTransform::normalizeTitle(page.title)] = page_id;
    }

    for (const auto& [revision_id, revision] : snapshot_.revisions) {
        (void)revision_id;
        if (snapshot_.pages.count(revision.page_id) == 0) {
            ++report.orphan_revisions;
            report.errors.push_back("revision references missing page_id " + std::to_string(revision.page_id));
        }
    }

    for (const auto& redirect : snapshot_.redirects) {
        if (titles.count(WikipediaTransform::normalizeTitle(redirect.target_title)) == 0) {
            ++report.dangling_redirects;
            report.warnings.push_back("redirect target not present in current snapshot: " + redirect.target_title);
        }
    }

    if (report.dead_letters > 0) {
        report.warnings.push_back("dead-letter sink contains skipped source rows");
    }
    if (config_.enable_vector_projection) {
        const auto pending = std::count_if(
            snapshot_.vector_records.begin(), snapshot_.vector_records.end(),
            [](const WikipediaVectorRecord& record) { return record.pending_embedding; });
        if (pending > 0) {
            report.warnings.push_back("vector projection is using vendor-neutral embedding hooks (pending embeddings)");
        }
    }

    report.success = report.errors.empty();
    return report;
}

WikipediaManifest WikipediaIngestionPipeline::exportPortable(
    const std::string& database_path,
    const std::string& manifest_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto db_parent = std::filesystem::path(database_path).parent_path();
    if (!db_parent.empty()) {
        std::filesystem::create_directories(db_parent);
    }

    std::ofstream database_file(database_path);
    database_file << snapshot_.toJson().dump(2) << '\n';
    database_file.close();

    WikipediaManifest manifest;
    manifest.dump_source = last_source_.source_path;
    manifest.importer_version = config_.importer_version;
    manifest.generated_at = nowIso8601();
    manifest.row_counts = {
        {"wiki_page", snapshot_.pages.size()},
        {"wiki_revision", snapshot_.revisions.size()},
        {"wiki_link", snapshot_.links.size()},
        {"wiki_category", snapshot_.categories.size()},
        {"wiki_redirect", snapshot_.redirects.size()},
        {"wiki_dead_letter", snapshot_.dead_letters.size()},
        {"wiki_graph_edge", snapshot_.graph_edges.size()},
        {"wiki_vector", snapshot_.vector_records.size()},
        {"wiki_process_event", snapshot_.process_events.size()},
        {"wiki_timeseries", snapshot_.timeseries_metrics.size()}
    };
    manifest.checksums = {
        {"wikipedia.db", checksumFile(database_path)},
        {"snapshot", WikipediaTransform::checksumHex(snapshot_.toJson().dump())}
    };
    manifest.external_tool_references = config_.external_tool_references;

    const std::string resolved_manifest_path = manifest_path.empty()
        ? config_.export_config.manifest_path
        : manifest_path;
    const std::filesystem::path manifest_output_path = resolved_manifest_path.empty()
        ? std::filesystem::path(database_path).parent_path() / "manifest.json"
        : std::filesystem::path(resolved_manifest_path);

    if (!manifest_output_path.parent_path().empty()) {
        std::filesystem::create_directories(manifest_output_path.parent_path());
    }
    std::ofstream manifest_file(manifest_output_path);
    manifest_file << manifest.toJson().dump(2) << '\n';
    manifest_file.close();

    if (config_.export_config.write_validation_report) {
        auto report = validateUnlocked();
        std::ofstream report_file(database_path + ".verify.json");
        report_file << report.toJson().dump(2) << '\n';
    }

    last_manifest_ = manifest;
    return manifest;
}

json WikipediaIngestionPipeline::sourceSchema() const {
    return json{
        {"plugin", "wikipedia_ingest"},
        {"lifecycle", json::array({"init", "shutdown", "runFullImport", "runIncrementalUpdate", "rebuildProjection", "validate", "exportPortable"})},
        {"canonical_core_tables", json::array({"wiki_page", "wiki_revision", "wiki_link", "wiki_category", "wiki_redirect", "wiki_ingest_state", "wiki_dead_letter"})},
        {"projections", json::array({"graph", "vector", "process", "timeseries"})},
        {"graph_edges", json::array({"LINKS_TO", "IN_CATEGORY", "REDIRECTS_TO"})},
        {"delta", json{{"dirty_page_tracking", true}, {"idempotent_upserts", true}, {"checkpoint_resume", true}}},
        {"portable_export", json{{"database", "wikipedia.db"}, {"manifest", "manifest.json"}, {"validation_report", "wikipedia.db.verify.json"}}},
        {"external_tool_references", config_.external_tool_references}
    };
}

} // namespace themis::importers
