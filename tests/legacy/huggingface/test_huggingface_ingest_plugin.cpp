#include <gtest/gtest.h>

#include "importers/huggingface_ingest_plugin.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace themis { namespace importers { 

TEST(HuggingFaceIngestPlugin, LifecycleAndCanonicalTables) {
    HuggingFaceIngestPlugin plugin;
    EXPECT_FALSE(plugin.isInitialized());
    EXPECT_TRUE(plugin.init());
    EXPECT_TRUE(plugin.isInitialized());

    const auto tables = plugin.canonicalTableNames();
    EXPECT_NE(std::find(tables.begin(), tables.end(), "hf_dataset_catalog"), tables.end());
    EXPECT_NE(std::find(tables.begin(), tables.end(), "training_example"), tables.end());

    plugin.shutdown();
    EXPECT_FALSE(plugin.isInitialized());
}

TEST(HuggingFaceIngestPlugin, FullImportTracksDeadLettersInBestEffortMode) {
    HuggingFaceIngestConfig config;
    config.allowed_licenses = {"cc-by-4.0"};
    config.strict_mode = false;

    HuggingFaceIngestPlugin plugin(config);
    ASSERT_TRUE(plugin.init());

    HuggingFaceImportRequest request;
    request.dataset_name = "legal/de_case_law";
    request.split = "train";
    request.seed_rows = {
        {{"id", "doc-1"}, {"text", "This is a valid legal text with enough length for scoring."}, {"label", "contract"}, {"license", "cc-by-4.0"}},
        {{"id", "doc-2"}, {"text", ""}, {"label", "criminal"}, {"license", "cc-by-4.0"}}};

    const auto report = plugin.runFullImport(request);
    EXPECT_TRUE(report.success);
    EXPECT_EQ(report.imported_documents, 1u);
    EXPECT_EQ(report.failed_records, 1u);
    EXPECT_EQ(plugin.deadLetterCount(), 1u);
}

TEST(HuggingFaceIngestPlugin, IncrementalUpdateMarksDirtyRecords) {
    HuggingFaceIngestConfig config;
    config.allowed_licenses = {"cc-by-4.0"};

    HuggingFaceIngestPlugin plugin(config);
    ASSERT_TRUE(plugin.init());

    HuggingFaceImportRequest full;
    full.dataset_name = "legal/eu";
    full.seed_rows = {{{"id", "doc-1"}, {"text", "Original legal passage A."}, {"label", "tax"}, {"license", "cc-by-4.0"}}};
    const auto full_report = plugin.runFullImport(full);
    ASSERT_TRUE(full_report.success);

    HuggingFaceUpdateRequest delta;
    delta.dataset_name = "legal/eu";
    delta.changed_rows = {{{"id", "doc-1"}, {"text", "Original legal passage A updated with additional facts."}, {"label", "tax"}, {"license", "cc-by-4.0"}}};

    const auto update_report = plugin.runIncrementalUpdate(delta);
    EXPECT_TRUE(update_report.success);
    EXPECT_GE(update_report.dirty_records, 1u);
}

TEST(HuggingFaceIngestPlugin, ExportsDeterministicAdaLoraJsonl) {
    HuggingFaceIngestConfig config;
    config.allowed_licenses = {"cc-by-4.0"};
    config.deterministic_export = true;

    HuggingFaceIngestPlugin plugin(config);
    ASSERT_TRUE(plugin.init());

    HuggingFaceImportRequest request;
    request.dataset_name = "legal/export";
    request.seed_rows = {
        {{"id", "doc-b"}, {"text", "Second legal text sample with sufficient context."}, {"label", "civil"}, {"license", "cc-by-4.0"}},
        {{"id", "doc-a"}, {"text", "First legal text sample with sufficient context."}, {"label", "criminal"}, {"license", "cc-by-4.0"}}};
    ASSERT_TRUE(plugin.runFullImport(request).success);

    const auto output = std::filesystem::temp_directory_path() / "themis_hf_ingest_export.jsonl";
    AdaLoraExportRequest export_request;
    export_request.output_path = output.string();
    export_request.format = AdaLoraExportFormat::INSTRUCTION_TUNING;
    export_request.deterministic = true;
    export_request.include_system_field = true;
    export_request.system_prompt = "legal-assistant";

    const auto export_report = plugin.exportAdaLoraJsonl(export_request);
    ASSERT_TRUE(export_report.success);
    ASSERT_EQ(export_report.exported_examples, 2u);

    std::ifstream in(output);
    ASSERT_TRUE(in.is_open());
    std::string line;
    ASSERT_TRUE(static_cast<bool>(std::getline(in, line)));
    EXPECT_NE(line.find("\"instruction\""), std::string::npos);
    EXPECT_NE(line.find("\"system\":\"legal-assistant\""), std::string::npos);
    std::filesystem::remove(output);
}
} } // namespace themis::importers
