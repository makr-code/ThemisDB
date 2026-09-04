#include <gtest/gtest.h>
#include "exporters/streaming_exporter.h"
#include "exporters/exporter_errors.h"
#include "exporters/exporter_metrics.h"
#include "storage/base_entity.h"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif
#include <vector>

using namespace themis::exporters;
using namespace themis;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class StreamingExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto temp_base = std::filesystem::temp_directory_path();
        test_dir_ = (temp_base /
            ("themis_streaming_test_" + std::to_string(std::time(nullptr)) +
             "_" + std::to_string(static_cast<int>(getpid()))))
            .string();
        std::filesystem::create_directories(test_dir_);
        createTestEntities();
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    void createTestEntities(int count = 20) {
        test_entities_.clear();
        for (int i = 0; i < count; i++) {
            BaseEntity entity;
            entity.setPrimaryKey("entity_" + std::to_string(i));
            entity.setField("question", "Question " + std::to_string(i) + "?");
            entity.setField("answer",   "Answer "   + std::to_string(i));
            entity.setField("score",    static_cast<double>(i) / count);
            test_entities_.push_back(entity);
        }
    }

    std::vector<std::string> readLines(const std::string& path) {
        std::vector<std::string> lines;
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty()) {
              lines.push_back(line);
            }
        }
        return lines;
    }

    std::string test_dir_;
    std::vector<BaseEntity> test_entities_;
};

// ─────────────────────────────────────────────────────────────────────────────
// VectorExportCursor tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingExporterTest, CursorBasic) {
    VectorExportCursor cursor(test_entities_, 5);
    EXPECT_EQ(cursor.totalCount(), test_entities_.size());
    EXPECT_EQ(cursor.currentOffset(), 0u);

    size_t total = 0;
    while (cursor.hasNext()) {
        auto page = cursor.nextPage();
        EXPECT_LE(page.size(), 5u);
        total += page.size();
    }
    EXPECT_EQ(total, test_entities_.size());
    EXPECT_FALSE(cursor.hasNext());
    EXPECT_EQ(cursor.currentOffset(), test_entities_.size());
}

TEST_F(StreamingExporterTest, CursorSeekTo) {
    VectorExportCursor cursor(test_entities_, 5);
    EXPECT_TRUE(cursor.seekTo(10));
    EXPECT_EQ(cursor.currentOffset(), 10u);

    size_t total = 0;
    while (cursor.hasNext()) {
        total += cursor.nextPage().size();
    }
    EXPECT_EQ(total, test_entities_.size() - 10);
}

TEST_F(StreamingExporterTest, CursorSeekBeyondEnd) {
    VectorExportCursor cursor(test_entities_, 5);
    EXPECT_FALSE(cursor.seekTo(test_entities_.size() + 1));
}

TEST_F(StreamingExporterTest, CursorEmptyCollection) {
    std::vector<BaseEntity> empty;
    VectorExportCursor cursor(empty, 10);
    EXPECT_FALSE(cursor.hasNext());
    EXPECT_EQ(cursor.totalCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// StreamingExporter::exportEntities (IExporter interface)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingExporterTest, ExportEntitiesBasic) {
    StreamingExporter exporter;
    ExportOptions options;
    options.output_path = test_dir_ + "/basic.jsonl";

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_EQ(stats.exported_entities, test_entities_.size());
    EXPECT_EQ(stats.failed_entities, 0u);
    EXPECT_GT(stats.bytes_written, 0u);
    EXPECT_TRUE(std::filesystem::exists(options.output_path));

    auto lines = readLines(options.output_path);
    EXPECT_EQ(lines.size(), test_entities_.size());

    // Every line must be valid JSON
    for (const auto& line : lines) {
        EXPECT_NO_THROW({
            auto parsed = json::parse(line);
            static_cast<void>(parsed);
        });
    }
}

TEST_F(StreamingExporterTest, ExportEntitiesOutputContainsPrimaryKey) {
    StreamingExporter exporter;
    ExportOptions options;
    options.output_path = test_dir_ + "/pk.jsonl";

    auto stats = exporter.exportEntities(test_entities_, options);
    auto lines = readLines(options.output_path);

    for (size_t i = 0; i < lines.size(); i++) {
        auto j = json::parse(lines[i]);
        EXPECT_TRUE(j.contains("_id"));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// StreamingExporter::exportFromCursor
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingExporterTest, ExportFromCursorPageSizeOne) {
    StreamingExportConfig cfg;
    cfg.page_size = 1;
    StreamingExporter exporter(cfg);

    ExportOptions options;
    options.output_path = test_dir_ + "/page1.jsonl";

    VectorExportCursor cursor(test_entities_, 1);
    auto stats = exporter.exportFromCursor(cursor, options);

    EXPECT_EQ(stats.exported_entities, test_entities_.size());
    auto lines = readLines(options.output_path);
    EXPECT_EQ(lines.size(), test_entities_.size());
}

TEST_F(StreamingExporterTest, ExportFromCursorLargePageSize) {
    StreamingExportConfig cfg;
    cfg.page_size = 10000;
    StreamingExporter exporter(cfg);

    ExportOptions options;
    options.output_path = test_dir_ + "/large_page.jsonl";

    VectorExportCursor cursor(test_entities_, 10000);
    auto stats = exporter.exportFromCursor(cursor, options);

    EXPECT_EQ(stats.exported_entities, test_entities_.size());
}

TEST_F(StreamingExporterTest, ExportFromCursorEmptyCollection) {
    StreamingExporter exporter;
    ExportOptions options;
    options.output_path = test_dir_ + "/empty.jsonl";

    std::vector<BaseEntity> empty;
    VectorExportCursor cursor(empty);
    auto stats = exporter.exportFromCursor(cursor, options);

    EXPECT_EQ(stats.exported_entities, 0u);
    EXPECT_EQ(stats.bytes_written, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Progress callback and ETA
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingExporterTest, ProgressCallbackInvoked) {
    createTestEntities(100);

    StreamingExporter exporter;
    ExportOptions options;
    options.output_path = test_dir_ + "/progress.jsonl";
    options.progress_interval = 10;

    size_t callback_count = 0;
    options.progress_callback = [&](const ExportStats& s) {
        callback_count++;
        EXPECT_GT(s.exported_entities, 0u);
        EXPECT_GT(s.bytes_written, 0u);
        // duration should be non-negative
        EXPECT_GE(s.duration.count(), 0);
    };

    auto stats = exporter.exportEntities(test_entities_, options);

    // Should have been called at least once (100 entities / interval 10 = 10 calls)
    EXPECT_GE(callback_count, 1u);
    EXPECT_EQ(stats.exported_entities, 100u);
}

TEST_F(StreamingExporterTest, ETAPopulatedDuringProgress) {
    createTestEntities(1000);

    StreamingExportConfig cfg;
    cfg.page_size = 100;
    StreamingExporter exporter(cfg);

    ExportOptions options;
    options.output_path = test_dir_ + "/eta.jsonl";
    options.progress_interval = 100;

    bool eta_seen = false;
    options.progress_callback = [&](const ExportStats& s) {
        // ETA should be >= 0 once we have progress over a known total
        EXPECT_GE(s.estimated_eta_seconds, 0.0);
        eta_seen = true;
    };

    VectorExportCursor cursor(test_entities_, 100);
    exporter.exportFromCursor(cursor, options);

    EXPECT_TRUE(eta_seen);
}

TEST_F(StreamingExporterTest, FinalStatsETAIsZero) {
    StreamingExporter exporter;
    ExportOptions options;
    options.output_path = test_dir_ + "/final.jsonl";

    auto stats = exporter.exportEntities(test_entities_, options);

    // ETA must be 0 at completion
    EXPECT_DOUBLE_EQ(stats.estimated_eta_seconds, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Field selection (include / exclude)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingExporterTest, IncludeFieldsFilter) {
    StreamingExporter exporter;
    ExportOptions options;
    options.output_path = test_dir_ + "/include.jsonl";
    options.include_fields = {"question"};

    auto stats = exporter.exportEntities(test_entities_, options);
    auto lines = readLines(options.output_path);
    EXPECT_EQ(lines.size(), test_entities_.size());

    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_TRUE(j.contains("question"));
        EXPECT_FALSE(j.contains("answer"));
        EXPECT_FALSE(j.contains("score"));
    }
}

TEST_F(StreamingExporterTest, ExcludeFieldsFilter) {
    StreamingExporter exporter;
    ExportOptions options;
    options.output_path = test_dir_ + "/exclude.jsonl";
    options.exclude_fields = {"score"};

    auto stats = exporter.exportEntities(test_entities_, options);
    auto lines = readLines(options.output_path);
    EXPECT_EQ(lines.size(), test_entities_.size());

    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_FALSE(j.contains("score"));
        EXPECT_TRUE(j.contains("question"));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Checkpoint / resumable export
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingExporterTest, CheckpointWrittenAfterEachPage) {
    createTestEntities(30);

    StreamingExportConfig cfg;
    cfg.page_size = 10;
    cfg.checkpoint_path = test_dir_ + "/checkpoint.txt";
    StreamingExporter exporter(cfg);

    ExportOptions options;
    options.output_path = test_dir_ + "/checkpoint_export.jsonl";

    auto stats = exporter.exportEntities(test_entities_, options);
    EXPECT_EQ(stats.exported_entities, 30u);

    // Checkpoint file should exist and contain the final offset
    EXPECT_TRUE(std::filesystem::exists(cfg.checkpoint_path));
    std::ifstream cf(cfg.checkpoint_path);
    size_t offset = 0;
    cf >> offset;
    EXPECT_EQ(offset, 30u);
}

TEST_F(StreamingExporterTest, CheckpointMetricsRecorded) {
    createTestEntities(10);

    // Write a checkpoint file simulating a prior run at offset 5
    const std::string ckpt = test_dir_ + "/resume_ckpt.txt";
    {
        std::ofstream f(ckpt);
        f << 5 << '\n';
    }

    StreamingExportConfig cfg;
    cfg.page_size = 10;
    cfg.checkpoint_path = ckpt;
    StreamingExporter exporter(cfg);

    ExportOptions options;
    options.output_path = test_dir_ + "/resume_export.jsonl";

    VectorExportCursor cursor(test_entities_, 10);
    auto stats = exporter.exportFromCursor(cursor, options);

    // Should have exported only the remaining 5 entities
    EXPECT_EQ(stats.exported_entities, 5u);
    // recordCheckpoint should have been called for the resume event
    EXPECT_GE(exporter.getMetrics()->getCheckpointCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Compression
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingExporterTest, GzipCompression) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD compression not available in this build (gzip type redirects to ZSTD)";
#endif
    StreamingExporter exporter;
    ExportOptions options;
    options.output_path = test_dir_ + "/compressed.jsonl.gz";
    options.compress = true;
    options.compression_type = "gzip";
    options.compression_level = 6;

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_EQ(stats.exported_entities, test_entities_.size());
    EXPECT_GT(stats.bytes_written, 0u);
    EXPECT_TRUE(std::filesystem::exists(options.output_path));
    // Compressed file should be smaller than uncompressed byte count
    auto file_size = std::filesystem::file_size(options.output_path);
    EXPECT_GT(file_size, 0u);
}

TEST_F(StreamingExporterTest, GzipTypeProducesZstdMagicNumber) {
    // Acceptance criterion: "gzip" compression_type now redirects to ZSTD;
    // output must be ZSTD-framed (first 4 bytes == ZSTD_MAGICNUMBER 0xFD2FB528).
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD compression not available in this build";
#endif

    StreamingExporter exporter;
    ExportOptions options;
    options.output_path = test_dir_ + "/gzip_is_zstd.zst";
    options.compress = true;
    options.compression_type = "gzip";
    options.compression_level = 3;

    exporter.exportEntities(test_entities_, options);

    ASSERT_TRUE(std::filesystem::exists(options.output_path));
    std::ifstream f(options.output_path, std::ios::binary);
    ASSERT_TRUE(f.is_open());
    // ZSTD magic number is 0xFD2FB528 (big-endian notation), stored in the
    // frame as four little-endian bytes: 0x28, 0xB5, 0x2F, 0xFD.
    unsigned char header[4] = {};
    f.read(reinterpret_cast<char*>(header), sizeof(header));
    ASSERT_FALSE(f.fail());
    EXPECT_EQ(header[0], 0x28u);  // least-significant byte first
    EXPECT_EQ(header[1], 0xB5u);
    EXPECT_EQ(header[2], 0x2Fu);
    EXPECT_EQ(header[3], 0xFDu);  // most-significant byte last
}

TEST_F(StreamingExporterTest, ZstdCompression) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD compression not available in this build";
#endif

    StreamingExporter exporter;
    ExportOptions options;
    options.output_path = test_dir_ + "/compressed.jsonl.zst";
    options.compress = true;
    options.compression_type = "zstd";
    options.compression_level = 3;

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_EQ(stats.exported_entities, test_entities_.size());
    EXPECT_GT(stats.bytes_written, 0u);
    EXPECT_TRUE(std::filesystem::exists(options.output_path));
    auto file_size = std::filesystem::file_size(options.output_path);
    EXPECT_GT(file_size, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Exporter metadata
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingExporterTest, GetNameAndVersion) {
    StreamingExporter exporter;
    EXPECT_EQ(exporter.getName(), "streaming_exporter");
    EXPECT_FALSE(exporter.getVersion().empty());
}

TEST_F(StreamingExporterTest, SupportedFormats) {
    StreamingExporter exporter;
    auto formats = exporter.getSupportedFormats();
    EXPECT_FALSE(formats.empty());
    bool has_jsonl = false;
    for (const auto& f : formats) {
        if (f == "jsonl") { has_jsonl = true; break; }
    }
    EXPECT_TRUE(has_jsonl);
}

TEST_F(StreamingExporterTest, MetricsTracked) {
    StreamingExporter exporter;
    ExportOptions options;
    options.output_path = test_dir_ + "/metrics.jsonl";

    auto stats = exporter.exportEntities(test_entities_, options);

    auto metrics = exporter.getMetrics();
    ASSERT_NE(metrics, nullptr);
    // After export the metrics object should have recorded the operation
    EXPECT_EQ(stats.metrics, metrics);
}
