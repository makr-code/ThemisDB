#include <gtest/gtest.h>
#include "exporters/arrow_ipc_exporter.h"
#include "exporters/exporter_errors.h"
#include "exporters/exporter_metrics.h"
#include "storage/base_entity.h"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace themis::exporters;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Verify the 8-byte Arrow magic bytes ("ARROW1\0\0") at position `offset`.
static bool hasArrowMagic(const std::string& path, std::streamoff offset) {
    static const uint8_t kMagic[8] = {0x41,0x52,0x52,0x4f,0x57,0x31,0x00,0x00};
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
      return false;
    }
    f.seekg(offset);
    if (!f.good()) {
      return false;
    }
    uint8_t buf[8] = {};
    f.read(reinterpret_cast<char*>(buf), 8);
    if (f.gcount() < 8) {
      return false;
    }
    return std::memcmp(buf, kMagic, 8) == 0;
}

/// Read the trailing 8 bytes and verify they are the Arrow magic.
static bool hasTrailingArrowMagic(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) {
      return false;
    }
    auto sz = f.tellg();
    if (sz < 8) {
      return false;
    }
    f.seekg(-8, std::ios::end);
    uint8_t buf[8] = {};
    f.read(reinterpret_cast<char*>(buf), 8);
    static const uint8_t kMagic[8] = {0x41,0x52,0x52,0x4f,0x57,0x31,0x00,0x00};
    return std::memcmp(buf, kMagic, 8) == 0;
}

/// Read the first 4 bytes after the leading magic and verify they are
/// the continuation marker (0xFFFFFFFF = -1 as LE int32).
static bool hasSchemaMarker(const std::string& path, bool is_file_format) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
      return false;
    }
    if (is_file_format) f.seekg(8);  // skip 8-byte file magic
    char buf[4] = {};
    f.read(buf, 4);
    if (f.gcount() < 4) {
      return false;
    }
    // Continuation marker is -1 (LE int32) = FF FF FF FF
    return (static_cast<uint8_t>(buf[0]) == 0xFF &&
            static_cast<uint8_t>(buf[1]) == 0xFF &&
            static_cast<uint8_t>(buf[2]) == 0xFF &&
            static_cast<uint8_t>(buf[3]) == 0xFF);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class ArrowIPCExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto tmp = std::filesystem::temp_directory_path();
        test_dir_ = (tmp / ("themis_arrow_test_" +
                            std::to_string(std::time(nullptr)))).string();
        std::filesystem::create_directories(test_dir_);
        createTestEntities();
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    void createTestEntities(int count = 10) {
        test_entities_.clear();
        for (int i = 0; i < count; ++i) {
            BaseEntity e;
            e.setPrimaryKey("entity_" + std::to_string(i));
            e.setField("instruction", "Instruction " + std::to_string(i));
            e.setField("output",      "Output "      + std::to_string(i));
            e.setField("score",       static_cast<double>(i) / count);
            e.setField("count",       static_cast<int64_t>(i));
            test_entities_.push_back(e);
        }
    }

    std::string test_dir_;
    std::vector<BaseEntity> test_entities_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Basic export (Arrow IPC File format)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ArrowIPCExporterTest, BasicFileExport) {
    ArrowIPCExporter exporter;

    ExportOptions opts;
    opts.output_path = test_dir_ + "/basic.arrow";

    auto stats = exporter.exportEntities(test_entities_, opts);

    EXPECT_EQ(stats.total_entities, test_entities_.size());
    EXPECT_EQ(stats.exported_entities, test_entities_.size());
    EXPECT_GT(stats.bytes_written, 0u);
    EXPECT_TRUE(std::filesystem::exists(opts.output_path));
}

TEST_F(ArrowIPCExporterTest, FileFormatHasLeadingMagic) {
    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = test_dir_ + "/magic.arrow";
    exporter.exportEntities(test_entities_, opts);

    EXPECT_TRUE(hasArrowMagic(opts.output_path, 0));
}

TEST_F(ArrowIPCExporterTest, FileFormatHasTrailingMagic) {
    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = test_dir_ + "/trailing_magic.arrow";
    exporter.exportEntities(test_entities_, opts);

    EXPECT_TRUE(hasTrailingArrowMagic(opts.output_path));
}

TEST_F(ArrowIPCExporterTest, FileFormatHasSchemaMarker) {
    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = test_dir_ + "/schema_marker.arrow";
    exporter.exportEntities(test_entities_, opts);

    EXPECT_TRUE(hasSchemaMarker(opts.output_path, /*is_file_format=*/true));
}

TEST_F(ArrowIPCExporterTest, FileFormatMinimumSize) {
    // Arrow IPC File: 8 (magic) + at least one message frame + footer + 4 + 8
    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = test_dir_ + "/min_size.arrow";
    exporter.exportEntities(test_entities_, opts);

    // File must be larger than just the magic bytes (8 + 8 = 16)
    EXPECT_GT(std::filesystem::file_size(opts.output_path), 16u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Stream format
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ArrowIPCExporterTest, StreamFormatExport) {
    ArrowIPCExportConfig cfg;
    cfg.format = ArrowIPCFormat::STREAM;
    ArrowIPCExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = test_dir_ + "/stream.arrows";
    auto stats = exporter.exportEntities(test_entities_, opts);

    EXPECT_EQ(stats.exported_entities, test_entities_.size());
    EXPECT_GT(stats.bytes_written, 0u);
    EXPECT_TRUE(std::filesystem::exists(opts.output_path));
}

TEST_F(ArrowIPCExporterTest, StreamFormatHasSchemaMarker) {
    ArrowIPCExportConfig cfg;
    cfg.format = ArrowIPCFormat::STREAM;
    ArrowIPCExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = test_dir_ + "/stream_marker.arrows";
    exporter.exportEntities(test_entities_, opts);

    // Stream format: no file magic; first bytes are the schema continuation marker
    EXPECT_TRUE(hasSchemaMarker(opts.output_path, /*is_file_format=*/false));
}

TEST_F(ArrowIPCExporterTest, StreamFormatNoLeadingMagic) {
    ArrowIPCExportConfig cfg;
    cfg.format = ArrowIPCFormat::STREAM;
    ArrowIPCExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = test_dir_ + "/no_magic.arrows";
    exporter.exportEntities(test_entities_, opts);

    // Stream format must NOT start with "ARROW1"
    EXPECT_FALSE(hasArrowMagic(opts.output_path, 0));
}

// ─────────────────────────────────────────────────────────────────────────────
// Empty entity list
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ArrowIPCExporterTest, EmptyEntitiesFileFormat) {
    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = test_dir_ + "/empty.arrow";

    std::vector<BaseEntity> empty;
    auto stats = exporter.exportEntities(empty, opts);

    EXPECT_EQ(stats.exported_entities, 0u);
    EXPECT_TRUE(std::filesystem::exists(opts.output_path));
    // File must still have magic bytes
    EXPECT_TRUE(hasArrowMagic(opts.output_path, 0));
    EXPECT_TRUE(hasTrailingArrowMagic(opts.output_path));
}

TEST_F(ArrowIPCExporterTest, EmptyEntitiesStreamFormat) {
    ArrowIPCExportConfig cfg;
    cfg.format = ArrowIPCFormat::STREAM;
    ArrowIPCExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = test_dir_ + "/empty.arrows";

    std::vector<BaseEntity> empty;
    auto stats = exporter.exportEntities(empty, opts);

    EXPECT_EQ(stats.exported_entities, 0u);
    EXPECT_TRUE(std::filesystem::exists(opts.output_path));
    EXPECT_GT(stats.bytes_written, 0u);  // at least schema message + EOS
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ArrowIPCExporterTest, StatsEntityCountMatchesInput) {
    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = test_dir_ + "/stats.arrow";

    auto stats = exporter.exportEntities(test_entities_, opts);
    EXPECT_EQ(stats.total_entities, test_entities_.size());
    EXPECT_EQ(stats.exported_entities, test_entities_.size());
}

TEST_F(ArrowIPCExporterTest, StatsBytesWrittenMatchesFileSize) {
    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = test_dir_ + "/bytes.arrow";

    auto stats = exporter.exportEntities(test_entities_, opts);

    auto file_sz = std::filesystem::file_size(opts.output_path);
    EXPECT_EQ(stats.bytes_written, file_sz);
}

TEST_F(ArrowIPCExporterTest, StatsMetricsAttached) {
    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = test_dir_ + "/metrics.arrow";

    auto stats = exporter.exportEntities(test_entities_, opts);
    ASSERT_NE(stats.metrics, nullptr);
}

TEST_F(ArrowIPCExporterTest, StatsDurationNonNegative) {
    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = test_dir_ + "/duration.arrow";

    auto stats = exporter.exportEntities(test_entities_, opts);
    EXPECT_GE(stats.duration.count(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Column selection
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ArrowIPCExporterTest, IncludeColumns) {
    ArrowIPCExportConfig cfg;
    cfg.include_columns = {"instruction", "output"};
    ArrowIPCExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = test_dir_ + "/include.arrow";

    auto stats = exporter.exportEntities(test_entities_, opts);
    EXPECT_EQ(stats.exported_entities, test_entities_.size());
    EXPECT_GT(stats.bytes_written, 0u);
}

TEST_F(ArrowIPCExporterTest, ExcludeColumns) {
    ArrowIPCExportConfig cfg;
    cfg.exclude_columns = {"score", "count"};
    ArrowIPCExporter exporter(cfg);

    ExportOptions opts;
    opts.output_path = test_dir_ + "/exclude.arrow";

    auto stats = exporter.exportEntities(test_entities_, opts);
    EXPECT_EQ(stats.exported_entities, test_entities_.size());
    EXPECT_GT(stats.bytes_written, 0u);
}

TEST_F(ArrowIPCExporterTest, IncludeColumnsViaOptions) {
    ArrowIPCExporter exporter;

    ExportOptions opts;
    opts.output_path    = test_dir_ + "/opt_include.arrow";
    opts.include_fields = {"instruction"};

    auto stats = exporter.exportEntities(test_entities_, opts);
    EXPECT_GT(stats.bytes_written, 0u);
}

TEST_F(ArrowIPCExporterTest, ExcludeColumnsViaOptions) {
    ArrowIPCExporter exporter;

    ExportOptions opts;
    opts.output_path    = test_dir_ + "/opt_exclude.arrow";
    opts.exclude_fields = {"score"};

    auto stats = exporter.exportEntities(test_entities_, opts);
    EXPECT_GT(stats.bytes_written, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Error cases
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ArrowIPCExporterTest, EmptyOutputPathThrows) {
    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = "";

    EXPECT_THROW(exporter.exportEntities(test_entities_, opts), ConfigException);
}

TEST_F(ArrowIPCExporterTest, InvalidDirectoryThrows) {
    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = "/nonexistent_dir_xyz/out.arrow";

    EXPECT_THROW(exporter.exportEntities(test_entities_, opts), ExportIOException);
}

// ─────────────────────────────────────────────────────────────────────────────
// Progress callback
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ArrowIPCExporterTest, ProgressCallbackInvoked) {
    ArrowIPCExporter exporter;

    ExportOptions opts;
    opts.output_path = test_dir_ + "/progress.arrow";

    bool callback_called = false;
    opts.progress_callback = [&](const ExportStats& s) {
        callback_called = true;
        EXPECT_GT(s.exported_entities, 0u);
    };

    exporter.exportEntities(test_entities_, opts);
    EXPECT_TRUE(callback_called);
}

// ─────────────────────────────────────────────────────────────────────────────
// Metadata and interface
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ArrowIPCExporterTest, GetName) {
    ArrowIPCExporter exporter;
    EXPECT_EQ(exporter.getName(), "arrow_ipc_exporter");
}

TEST_F(ArrowIPCExporterTest, GetVersion) {
    ArrowIPCExporter exporter;
    EXPECT_FALSE(exporter.getVersion().empty());
}

TEST_F(ArrowIPCExporterTest, SupportedFormatsIncludesArrow) {
    ArrowIPCExporter exporter;
    auto fmts = exporter.getSupportedFormats();
    EXPECT_FALSE(fmts.empty());
    bool found = false;
    for (const auto& f : fmts) {
        if (f == "arrow" || f == "arrow_ipc") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(ArrowIPCExporterTest, IsArrowAvailableReturnsBool) {
    // Just verify the function is callable and returns a bool (true or false).
    bool avail = ArrowIPCExporter::isArrowAvailable();
    (void)avail;
    SUCCEED();
}

TEST_F(ArrowIPCExporterTest, SetAndGetConfig) {
    ArrowIPCExporter exporter;
    ArrowIPCExportConfig cfg;
    cfg.format = ArrowIPCFormat::STREAM;
    exporter.setConfig(cfg);
    EXPECT_EQ(exporter.getConfig().format, ArrowIPCFormat::STREAM);
}

TEST_F(ArrowIPCExporterTest, MetricsResetWorks) {
    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = test_dir_ + "/reset.arrow";
    exporter.exportEntities(test_entities_, opts);

    exporter.resetMetrics();
    // After reset, a fresh export should still succeed
    opts.output_path = test_dir_ + "/reset2.arrow";
    auto stats = exporter.exportEntities(test_entities_, opts);
    EXPECT_GT(stats.exported_entities, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Large entity set
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ArrowIPCExporterTest, LargeEntitySet) {
    std::vector<BaseEntity> large;
    for (int i = 0; i < 1000; ++i) {
        BaseEntity e;
        e.setPrimaryKey("e_" + std::to_string(i));
        e.setField("text", "value_" + std::to_string(i));
        large.push_back(e);
    }

    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = test_dir_ + "/large.arrow";

    auto stats = exporter.exportEntities(large, opts);
    EXPECT_EQ(stats.exported_entities, 1000u);
    EXPECT_GT(stats.bytes_written, 0u);
    EXPECT_TRUE(hasArrowMagic(opts.output_path, 0));
    EXPECT_TRUE(hasTrailingArrowMagic(opts.output_path));
}

// ─────────────────────────────────────────────────────────────────────────────
// Overwrite existing file
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ArrowIPCExporterTest, OverwriteExistingFile) {
    ArrowIPCExporter exporter;
    ExportOptions opts;
    opts.output_path = test_dir_ + "/overwrite.arrow";

    // First export
    exporter.exportEntities(test_entities_, opts);
    auto size1 = std::filesystem::file_size(opts.output_path);

    // Second export (overwrite)
    exporter.exportEntities(test_entities_, opts);
    auto size2 = std::filesystem::file_size(opts.output_path);

    EXPECT_GT(size1, 0u);
    EXPECT_GT(size2, 0u);
    // Both exports should produce the same file size for the same data
    EXPECT_EQ(size1, size2);
}
