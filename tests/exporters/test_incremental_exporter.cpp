#include <gtest/gtest.h>
#include "exporters/incremental_exporter.h"
#include "exporters/exporter_metrics.h"
#include "storage/base_entity.h"
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

using namespace themis::exporters;
using namespace themis;
using json = nlohmann::json;

namespace {
int current_process_id() {
#if defined(_WIN32)
    return _getpid();
#else
    return getpid();
#endif
}
}

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class IncrementalExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto temp_base = std::filesystem::temp_directory_path();
        test_dir_ = (temp_base /
            ("themis_incremental_test_" + std::to_string(std::time(nullptr)) +
             "_" + std::to_string(current_process_id())))
            .string();
        std::filesystem::create_directories(test_dir_);
        createTestEntities(10);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    /// Create entities with _seq field values 1..count
    void createTestEntities(int count) {
        test_entities_.clear();
        for (int i = 1; i <= count; i++) {
            BaseEntity entity;
            entity.setPrimaryKey("entity_" + std::to_string(i));
            entity.setField("_seq",    static_cast<int64_t>(i));
            entity.setField("content", "Content for entity " + std::to_string(i));
            entity.setField("score",   static_cast<double>(i) / count);
            test_entities_.push_back(entity);
        }
    }

    std::vector<std::string> readLines(const std::string& path) {
        std::vector<std::string> lines;
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty()) lines.push_back(line);
        }
        return lines;
    }

    std::string watermarkPath() const {
        return test_dir_ + "/watermark.json";
    }

    std::string outputPath() const {
        return test_dir_ + "/output.jsonl";
    }

    std::string test_dir_;
    std::vector<BaseEntity> test_entities_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Full export (no watermark)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncrementalExporterTest, FullExportWithoutWatermarkFile) {
    // No watermark_path set → full export every time
    IncrementalExportConfig cfg;
    cfg.sequence_field = "_seq";
    // watermark_path left empty
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();

    auto stats = exporter.exportEntities(test_entities_, opts);

    EXPECT_EQ(stats.total_entities, test_entities_.size());
    EXPECT_EQ(stats.exported_entities, test_entities_.size());
    EXPECT_EQ(stats.skipped_entities, 0u);
    EXPECT_EQ(stats.failed_entities, 0u);
    EXPECT_GT(stats.bytes_written, 0u);

    auto lines = readLines(outputPath());
    EXPECT_EQ(lines.size(), test_entities_.size());
}

TEST_F(IncrementalExporterTest, FullExportWritesWatermark) {
    IncrementalExportConfig cfg;
    cfg.sequence_field = "_seq";
    cfg.watermark_path = watermarkPath();
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();

    auto stats = exporter.exportEntities(test_entities_, opts);

    EXPECT_EQ(stats.exported_entities, test_entities_.size());
    EXPECT_TRUE(std::filesystem::exists(watermarkPath()));

    // Watermark file should record the highest sequence (10)
    std::ifstream wf(watermarkPath());
    json wj;
    wf >> wj;
    EXPECT_EQ(wj["last_sequence"].get<int64_t>(), 10);
    EXPECT_EQ(wj["exported_count"].get<size_t>(), test_entities_.size());
    EXPECT_FALSE(wj["last_export_time"].get<std::string>().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Incremental / delta export
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncrementalExporterTest, DeltaExportSkipsEntitiesBelowWatermark) {
    // Pre-write a watermark at sequence 5
    {
        std::ofstream wf(watermarkPath());
        json wj;
        wj["last_sequence"]    = 5;
        wj["last_export_time"] = "2026-01-01T00:00:00Z";
        wj["exported_count"]   = 5;
        wf << wj.dump(2) << '\n';
    }

    IncrementalExportConfig cfg;
    cfg.sequence_field = "_seq";
    cfg.watermark_path = watermarkPath();
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();

    auto stats = exporter.exportEntities(test_entities_, opts);

    // Entities 6-10 should be exported (5 entities)
    EXPECT_EQ(stats.total_entities, 10u);
    EXPECT_EQ(stats.exported_entities, 5u);
    EXPECT_EQ(stats.skipped_entities, 5u);

    auto lines = readLines(outputPath());
    EXPECT_EQ(lines.size(), 5u);

    // Verify only high-sequence entities appear
    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_GT(j["_seq"].get<int64_t>(), 5);
    }
}

TEST_F(IncrementalExporterTest, WatermarkUpdatedAfterDeltaExport) {
    // Pre-write a watermark at sequence 5
    {
        std::ofstream wf(watermarkPath());
        json wj;
        wj["last_sequence"]    = 5;
        wj["last_export_time"] = "2026-01-01T00:00:00Z";
        wj["exported_count"]   = 5;
        wf << wj.dump(2) << '\n';
    }

    IncrementalExportConfig cfg;
    cfg.sequence_field = "_seq";
    cfg.watermark_path = watermarkPath();
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();
    exporter.exportEntities(test_entities_, opts);

    // Watermark should now be 10
    std::ifstream wf(watermarkPath());
    json wj;
    wf >> wj;
    EXPECT_EQ(wj["last_sequence"].get<int64_t>(), 10);
}

TEST_F(IncrementalExporterTest, SecondRunExportsNothingWhenNothingChanged) {
    IncrementalExportConfig cfg;
    cfg.sequence_field = "_seq";
    cfg.watermark_path = watermarkPath();
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();

    // First run: export all 10 entities
    exporter.exportEntities(test_entities_, opts);

    // Second run with the same data: nothing should be exported
    const std::string output2 = test_dir_ + "/output2.jsonl";
    opts.output_path = output2;
    auto stats2 = exporter.exportEntities(test_entities_, opts);

    EXPECT_EQ(stats2.exported_entities, 0u);
    EXPECT_EQ(stats2.skipped_entities, 10u);
}

TEST_F(IncrementalExporterTest, NewEntitiesExportedAfterWatermarkSet) {
    IncrementalExportConfig cfg;
    cfg.sequence_field = "_seq";
    cfg.watermark_path = watermarkPath();
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();
    exporter.exportEntities(test_entities_, opts);

    // Add 5 more entities with sequences 11-15
    std::vector<BaseEntity> new_entities = test_entities_;
    for (int i = 11; i <= 15; i++) {
        BaseEntity e;
        e.setPrimaryKey("entity_" + std::to_string(i));
        e.setField("_seq",    static_cast<int64_t>(i));
        e.setField("content", "Content " + std::to_string(i));
        new_entities.push_back(e);
    }

    const std::string output2 = test_dir_ + "/output2.jsonl";
    opts.output_path = output2;
    auto stats2 = exporter.exportEntities(new_entities, opts);

    EXPECT_EQ(stats2.exported_entities, 5u);
    EXPECT_EQ(stats2.skipped_entities, 10u);

    auto lines = readLines(output2);
    EXPECT_EQ(lines.size(), 5u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Watermark is INT64_MIN when no file exists (full-export mode)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncrementalExporterTest, ReadWatermarkReturnsMINWhenNoFile) {
    IncrementalExportConfig cfg;
    cfg.watermark_path = test_dir_ + "/nonexistent_watermark.json";
    IncrementalExporter exporter(cfg);

    EXPECT_EQ(exporter.readWatermark(), std::numeric_limits<int64_t>::min());
}

TEST_F(IncrementalExporterTest, ReadWatermarkReturnsMINWhenPathEmpty) {
    IncrementalExportConfig cfg;
    // watermark_path is empty by default
    IncrementalExporter exporter(cfg);

    EXPECT_EQ(exporter.readWatermark(), std::numeric_limits<int64_t>::min());
}

// ─────────────────────────────────────────────────────────────────────────────
// Entities without the sequence field
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncrementalExporterTest, EntitiesWithoutSequenceFieldAreExportedByDefault) {
    // Pre-write watermark at 5; create entities that have no _seq field
    {
        std::ofstream wf(watermarkPath());
        json wj;
        wj["last_sequence"]    = 5;
        wj["last_export_time"] = "2026-01-01T00:00:00Z";
        wj["exported_count"]   = 5;
        wf << wj.dump(2) << '\n';
    }

    std::vector<BaseEntity> entities_no_seq;
    for (int i = 0; i < 3; i++) {
        BaseEntity e;
        e.setPrimaryKey("no_seq_" + std::to_string(i));
        e.setField("content", "Content without seq " + std::to_string(i));
        entities_no_seq.push_back(e);
    }

    IncrementalExportConfig cfg;
    cfg.sequence_field       = "_seq";
    cfg.watermark_path       = watermarkPath();
    cfg.export_missing_sequence = true;  // default: fail-open
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();

    auto stats = exporter.exportEntities(entities_no_seq, opts);

    // All 3 entities should be exported (no seq = pass-through)
    EXPECT_EQ(stats.exported_entities, 3u);
    EXPECT_EQ(stats.skipped_entities, 0u);
}

TEST_F(IncrementalExporterTest, EntitiesWithoutSequenceFieldSkippedWhenFailClosed) {
    {
        std::ofstream wf(watermarkPath());
        json wj;
        wj["last_sequence"]    = 5;
        wj["last_export_time"] = "2026-01-01T00:00:00Z";
        wj["exported_count"]   = 5;
        wf << wj.dump(2) << '\n';
    }

    std::vector<BaseEntity> entities_no_seq;
    for (int i = 0; i < 3; i++) {
        BaseEntity e;
        e.setPrimaryKey("no_seq_" + std::to_string(i));
        e.setField("content", "Content without seq " + std::to_string(i));
        entities_no_seq.push_back(e);
    }

    IncrementalExportConfig cfg;
    cfg.sequence_field          = "_seq";
    cfg.watermark_path          = watermarkPath();
    cfg.export_missing_sequence = false;  // fail-closed
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();

    auto stats = exporter.exportEntities(entities_no_seq, opts);

    EXPECT_EQ(stats.exported_entities, 0u);
    EXPECT_EQ(stats.skipped_entities, 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Metrics
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncrementalExporterTest, MetricsTrackSkippedEntities) {
    // Watermark at 5 → 5 entities skipped
    {
        std::ofstream wf(watermarkPath());
        json wj;
        wj["last_sequence"]    = 5;
        wj["last_export_time"] = "2026-01-01T00:00:00Z";
        wj["exported_count"]   = 5;
        wf << wj.dump(2) << '\n';
    }

    IncrementalExportConfig cfg;
    cfg.sequence_field = "_seq";
    cfg.watermark_path = watermarkPath();
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();
    exporter.exportEntities(test_entities_, opts);

    ASSERT_NE(exporter.getMetrics(), nullptr);
    EXPECT_EQ(exporter.getMetrics()->getDeltaDocsSkipped(), 5u);
}

TEST_F(IncrementalExporterTest, MetricsJsonContainsDeltaField) {
    IncrementalExporter exporter;

    ExportOptions opts;
    opts.output_path = outputPath();
    exporter.exportEntities(test_entities_, opts);

    auto metrics_json = exporter.getMetrics()->toJson();
    EXPECT_TRUE(metrics_json.contains("exporter_delta_docs_skipped_total"));
    EXPECT_EQ(metrics_json["exporter_delta_docs_skipped_total"].get<size_t>(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Double-type sequence field
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncrementalExporterTest, FloatingPointSequenceFieldRespected) {
    // Pre-write watermark at 5
    {
        std::ofstream wf(watermarkPath());
        json wj;
        wj["last_sequence"]    = 5;
        wj["last_export_time"] = "2026-01-01T00:00:00Z";
        wj["exported_count"]   = 5;
        wf << wj.dump(2) << '\n';
    }

    // Build entities whose _seq is stored as double
    std::vector<BaseEntity> entities;
    for (int i = 1; i <= 10; i++) {
        BaseEntity e;
        e.setPrimaryKey("dbl_" + std::to_string(i));
        e.setField("_seq", static_cast<double>(i));
        e.setField("content", "item");
        entities.push_back(e);
    }

    IncrementalExportConfig cfg;
    cfg.sequence_field = "_seq";
    cfg.watermark_path = watermarkPath();
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();

    auto stats = exporter.exportEntities(entities, opts);

    EXPECT_EQ(stats.exported_entities, 5u);  // sequences 6-10
    EXPECT_EQ(stats.skipped_entities, 5u);   // sequences 1-5
}

// ─────────────────────────────────────────────────────────────────────────────
// Field filtering (include/exclude) passed through
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncrementalExporterTest, FieldFilteringApplied) {
    IncrementalExporter exporter;

    ExportOptions opts;
    opts.output_path     = outputPath();
    opts.include_fields  = {"_seq", "content"};  // exclude "score"

    auto stats = exporter.exportEntities(test_entities_, opts);
    EXPECT_EQ(stats.exported_entities, test_entities_.size());

    auto lines = readLines(outputPath());
    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_TRUE(j.contains("_seq"));
        EXPECT_TRUE(j.contains("content"));
        EXPECT_FALSE(j.contains("score"));
    }
}

TEST_F(IncrementalExporterTest, FilterExpressionAppliedBeforeExportWrite) {
    IncrementalExporter exporter;

    ExportOptions opts;
    opts.output_path = outputPath();
    opts.filter_expression = "doc.score >= 0.7";

    auto stats = exporter.exportEntities(test_entities_, opts);
    EXPECT_EQ(stats.total_entities, test_entities_.size());
    EXPECT_EQ(stats.exported_entities, 4u);
    EXPECT_EQ(stats.skipped_entities, 6u);

    auto lines = readLines(outputPath());
    ASSERT_EQ(lines.size(), 4u);
    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_GE(j["score"].get<double>(), 0.7);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// IExporter metadata
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncrementalExporterTest, GetNameAndVersion) {
    IncrementalExporter exporter;
    EXPECT_EQ(exporter.getName(), "incremental_exporter");
    EXPECT_FALSE(exporter.getVersion().empty());
}

TEST_F(IncrementalExporterTest, SupportedFormats) {
    IncrementalExporter exporter;
    auto formats = exporter.getSupportedFormats();
    EXPECT_FALSE(formats.empty());
    bool has_jsonl = false;
    for (const auto& f : formats) {
        if (f == "jsonl") { has_jsonl = true; break; }
    }
    EXPECT_TRUE(has_jsonl);
}

// ─────────────────────────────────────────────────────────────────────────────
// Progress callback
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncrementalExporterTest, ProgressCallbackInvoked) {
    IncrementalExporter exporter;

    size_t callback_count = 0;
    ExportOptions opts;
    opts.output_path       = outputPath();
    opts.progress_interval = 3;
    opts.progress_callback = [&](const ExportStats& s) {
        callback_count++;
        EXPECT_GT(s.exported_entities, 0u);
    };

    exporter.exportEntities(test_entities_, opts);
    EXPECT_GT(callback_count, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Watermark not updated when no entities are exported
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncrementalExporterTest, WatermarkUnchangedWhenNothingExported) {
    // Set watermark to max sequence so all entities are skipped
    {
        std::ofstream wf(watermarkPath());
        json wj;
        wj["last_sequence"]    = 100;
        wj["last_export_time"] = "2026-01-01T00:00:00Z";
        wj["exported_count"]   = 10;
        wf << wj.dump(2) << '\n';
    }

    IncrementalExportConfig cfg;
    cfg.sequence_field = "_seq";
    cfg.watermark_path = watermarkPath();
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();
    exporter.exportEntities(test_entities_, opts);

    // Watermark should still be 100 (unchanged)
    std::ifstream wf(watermarkPath());
    json wj;
    wf >> wj;
    EXPECT_EQ(wj["last_sequence"].get<int64_t>(), 100);
}

TEST_F(IncrementalExporterTest, WatermarkNotAdvancedOnPartialSizeLimitedScan) {
    // Seed watermark at 0
    {
        std::ofstream wf(watermarkPath());
        json wj;
        wj["last_sequence"]    = 0;
        wj["last_export_time"] = "2026-01-01T00:00:00Z";
        wj["exported_count"]   = 0;
        wf << wj.dump(2) << '\n';
    }

    // Intentionally unsorted by sequence so partial writes can skip lower seqs later in the vector.
    std::vector<BaseEntity> unsorted;
    {
        BaseEntity e1;
        e1.setPrimaryKey("u1");
        e1.setField("_seq", static_cast<int64_t>(100));
        e1.setField("content", "large-seq-first");
        unsorted.push_back(e1);
    }
    {
        BaseEntity e2;
        e2.setPrimaryKey("u2");
        e2.setField("_seq", static_cast<int64_t>(1));
        e2.setField("content", "small-seq-second");
        unsorted.push_back(e2);
    }

    IncrementalExportConfig cfg;
    cfg.sequence_field = "_seq";
    cfg.watermark_path = watermarkPath();
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();
    opts.max_file_size_bytes = 1; // force partial export/early stop

    auto stats = exporter.exportEntities(unsorted, opts);
    EXPECT_LT(stats.exported_entities, unsorted.size());

    // Watermark must remain unchanged (0) after partial scan.
    std::ifstream wf(watermarkPath());
    json wj;
    wf >> wj;
    EXPECT_EQ(wj["last_sequence"].get<int64_t>(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Corrupt watermark file falls back to full export
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncrementalExporterTest, CorruptWatermarkFallsBackToFullExport) {
    {
        std::ofstream wf(watermarkPath());
        wf << "this is not valid JSON {{{{";
    }

    IncrementalExportConfig cfg;
    cfg.sequence_field = "_seq";
    cfg.watermark_path = watermarkPath();
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();

    auto stats = exporter.exportEntities(test_entities_, opts);

    // Should fall back to full export (watermark = INT64_MIN)
    EXPECT_EQ(stats.exported_entities, test_entities_.size());
    EXPECT_EQ(stats.skipped_entities, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// ExportStats::toJson includes skipped_entities
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(IncrementalExporterTest, ExportStatsToJsonContainsSkippedEntities) {
    // Pre-write watermark at 5 → 5 entities will be skipped
    {
        std::ofstream wf(watermarkPath());
        json wj;
        wj["last_sequence"]    = 5;
        wj["last_export_time"] = "2026-01-01T00:00:00Z";
        wj["exported_count"]   = 5;
        wf << wj.dump(2) << '\n';
    }

    IncrementalExportConfig cfg;
    cfg.sequence_field = "_seq";
    cfg.watermark_path = watermarkPath();
    IncrementalExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = outputPath();

    auto stats = exporter.exportEntities(test_entities_, opts);
    EXPECT_EQ(stats.skipped_entities, 5u);

    // toJson() must include the skipped_entities field
    auto j = json::parse(stats.toJson());
    EXPECT_TRUE(j.contains("skipped_entities"));
    EXPECT_EQ(j["skipped_entities"].get<size_t>(), 5u);
    EXPECT_EQ(j["exported_entities"].get<size_t>(), 5u);
}
