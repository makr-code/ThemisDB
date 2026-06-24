#include <gtest/gtest.h>

#include "importers/importer_plugin_api.h"
#include "importers/wikipedia_plugin.hpp"

#include <filesystem>
#include <fstream>

namespace {
std::string writeMiniDump(const std::filesystem::path& path, bool updated_alpha = false) {
    const char* alpha_revision_id = updated_alpha ? "12" : "11";
    const char* alpha_text = updated_alpha
        ? "[[Beta]] [[Gamma]] [[Category:Examples]]"
        : "[[Beta]] [[Category:Examples]]";
    std::ofstream dump(path);
    dump << R"(<mediawiki>
<page>
  <title>Alpha</title>
  <ns>0</ns>
  <id>1</id>
  <revision>
    <id>)" << alpha_revision_id << R"(</id>
    <timestamp>2026-01-01T00:00:00Z</timestamp>
    <sha1>alpha-sha1</sha1>
    <text xml:space="preserve">)" << alpha_text << R"(</text>
  </revision>
</page>
<page>
  <title>Beta</title>
  <ns>0</ns>
  <id>2</id>
  <redirect title="Alpha" />
  <revision>
    <id>21</id>
    <timestamp>2026-01-02T00:00:00Z</timestamp>
    <sha1>beta-sha1</sha1>
    <text xml:space="preserve">#REDIRECT [[Alpha]]</text>
  </revision>
</page>
</mediawiki>
)";
    return path.string();
}
} // namespace

TEST(WikipediaIngestionPluginTest, RegistersWithImporterRegistry) {
    auto plugin = themis::importers::ImporterPluginRegistry::instance().create("wikipedia_ingest");
    ASSERT_NE(nullptr, plugin);
    EXPECT_STREQ("wikipedia_ingest", plugin->getName());
}

TEST(WikipediaIngestionPluginTest, FullImportIncrementalUpdateAndExportWork) {
    namespace fs = std::filesystem;
    const fs::path temp_dir = fs::temp_directory_path() / "themisdb-wikipedia-ingest-test";
    fs::create_directories(temp_dir);

    const fs::path dump_path = temp_dir / "mini.xml";
    const fs::path dump_path_v2 = temp_dir / "mini-v2.xml";
    const fs::path checkpoint_path = temp_dir / "checkpoint.json";
    const fs::path export_path = temp_dir / "wikipedia.db";
    const fs::path manifest_path = temp_dir / "manifest.json";

    writeMiniDump(dump_path, false);
    writeMiniDump(dump_path_v2, true);

    themis::importers::WikipediaIngestionConfig config;
    config.checkpoint_path = checkpoint_path.string();
    config.export_config.database_path = export_path.string();
    config.export_config.manifest_path = manifest_path.string();

    themis::importers::WikipediaIngestionPlugin plugin(config);
    ASSERT_TRUE(plugin.init());

    themis::importers::ImportOptions options;
    options.checkpoint_file = checkpoint_path.string();

    const auto first_stats = plugin.runFullImport(
        {.source_path = dump_path.string(), .source_id = "mini-1", .language = "en", .dump_date = "2026-01-01", .producer_hint = "unit-test"},
        options);
    EXPECT_EQ(2u, first_stats.imported_records);
    EXPECT_EQ(0u, first_stats.failed_records);

    const auto validation = plugin.validateDatabase();
    EXPECT_TRUE(validation.success);

    const auto graph_summary = plugin.rebuildProjection(themis::importers::WikipediaProjectionModel::GRAPH);
    EXPECT_GE(graph_summary.graph_edges, 2u);

    const auto manifest = plugin.exportPortable(export_path.string(), manifest_path.string());
    EXPECT_TRUE(fs::exists(export_path));
    EXPECT_TRUE(fs::exists(manifest_path));
    EXPECT_EQ(2u, manifest.row_counts.at("wiki_page"));
    EXPECT_EQ(2u, plugin.pipeline().snapshot().pages.size());

    const auto second_stats = plugin.runIncrementalUpdate(
        {.source_path = dump_path_v2.string(), .source_id = "mini-2", .language = "en", .dump_date = "2026-01-02", .producer_hint = "unit-test"},
        options);
    EXPECT_EQ(1u, second_stats.imported_records);
    EXPECT_GE(plugin.pipeline().snapshot().revisions.size(), 3u);

    const auto schema = plugin.getSourceSchema(dump_path.string());
    EXPECT_TRUE(schema.contains("canonical_core_tables"));
    EXPECT_TRUE(schema.contains("portable_export"));
}
