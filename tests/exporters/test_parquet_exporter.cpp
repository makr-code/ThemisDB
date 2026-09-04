#include <gtest/gtest.h>
#include "exporters/parquet_exporter.h"
#include "exporters/exporter_errors.h"
#include "exporters/exporter_metrics.h"
#include "storage/base_entity.h"
#include <fstream>
#include <filesystem>
#include <ctime>

using namespace themis::exporters;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class ParquetExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto temp_base = std::filesystem::temp_directory_path();
        test_dir_ = (temp_base /
            ("themis_parquet_test_" + std::to_string(std::time(nullptr))))
            .string();
        std::filesystem::create_directories(test_dir_);
        createTestEntities();
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    void createTestEntities() {
        for (int i = 0; i < 10; ++i) {
            BaseEntity e;
            e.setPrimaryKey("entity_" + std::to_string(i));
            e.setField("instruction",
                       "Instruction for item " + std::to_string(i));
            e.setField("output",
                       "Output for item " + std::to_string(i));
            e.setField("score",
                       static_cast<double>(i) / 10.0);
            e.setField("count",
                       static_cast<int64_t>(i));
            test_entities_.push_back(e);
        }
        // Add a duplicate (same PK as entity_0) for deduplication tests
        BaseEntity dup = test_entities_[0];
        dup.setPrimaryKey("entity_0");  // same PK → duplicate
        test_entities_.push_back(dup);
    }

    /// Read the first 4 bytes of a file and verify it starts with PAR1
    bool isValidParquetMagic(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) {
          return false;
        }
        char magic[4] = {};
        f.read(magic, 4);
        return f.gcount() == 4 &&
               magic[0] == 'P' && magic[1] == 'A' &&
               magic[2] == 'R' && magic[3] == '1';
    }

    /// Read the trailing 4 bytes (before final magic) and verify PAR1 footer
    bool hasValidParquetFooter(const std::string& path) {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f.is_open()) {
          return false;
        }
        auto sz = f.tellg();
        if (sz < 8) {
          return false;
        }
        f.seekg(-4, std::ios::end);
        char magic[4] = {};
        f.read(magic, 4);
        return magic[0] == 'P' && magic[1] == 'A' &&
               magic[2] == 'R' && magic[3] == '1';
    }

    std::string test_dir_;
    std::vector<BaseEntity> test_entities_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Basic export tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ParquetExporterTest, BasicExport) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/basic.parquet";

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_EQ(stats.total_entities, test_entities_.size());
    EXPECT_GT(stats.exported_entities, 0u);
    EXPECT_GT(stats.bytes_written, 0u);
    EXPECT_GE(stats.duration.count(), 0);

    // File must exist
    EXPECT_TRUE(std::filesystem::exists(options.output_path));

    // Valid Parquet magic bytes
    EXPECT_TRUE(isValidParquetMagic(options.output_path));
    EXPECT_TRUE(hasValidParquetFooter(options.output_path));
}

TEST_F(ParquetExporterTest, ExportProducesNonEmptyFile) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/nonempty.parquet";

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_GT(std::filesystem::file_size(options.output_path), 8u);  // > 2×magic
}

TEST_F(ParquetExporterTest, EmptyEntitiesProducesValidParquet) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/empty.parquet";

    std::vector<BaseEntity> empty;
    auto stats = exporter.exportEntities(empty, options);

    EXPECT_EQ(stats.exported_entities, 0u);
    EXPECT_TRUE(std::filesystem::exists(options.output_path));
    EXPECT_TRUE(isValidParquetMagic(options.output_path));
    EXPECT_TRUE(hasValidParquetFooter(options.output_path));
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ParquetExporterTest, StatsTotalEntitiesMatchesInput) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/stats.parquet";

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_EQ(stats.total_entities, test_entities_.size());
}

TEST_F(ParquetExporterTest, StatsMetricsAttached) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/metrics_stats.parquet";

    auto stats = exporter.exportEntities(test_entities_, options);

    ASSERT_NE(stats.metrics, nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// Column selection tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ParquetExporterTest, ExportWithIncludeColumns) {
    ParquetExportConfig config;
    config.include_columns = {"instruction", "output"};
    ParquetExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/include_columns.parquet";

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_GT(stats.exported_entities, 0u);
    EXPECT_TRUE(isValidParquetMagic(options.output_path));
}

TEST_F(ParquetExporterTest, ExportWithExcludeColumns) {
    ParquetExportConfig config;
    config.exclude_columns = {"score", "count"};
    ParquetExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/exclude_columns.parquet";

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_GT(stats.exported_entities, 0u);
}

TEST_F(ParquetExporterTest, ExportOptionsIncludeFieldsRespected) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path     = test_dir_ + "/opt_include.parquet";
    options.include_fields  = {"instruction"};

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_GT(stats.exported_entities, 0u);
    EXPECT_TRUE(isValidParquetMagic(options.output_path));
}

// ─────────────────────────────────────────────────────────────────────────────
// Configuration tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ParquetExporterTest, RowGroupSizeConfiguration) {
    ParquetExportConfig config;
    config.row_group_size = 4;  // tiny row groups to exercise flushing
    ParquetExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/row_group.parquet";

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_GT(stats.exported_entities, 0u);
    EXPECT_TRUE(isValidParquetMagic(options.output_path));
}

TEST_F(ParquetExporterTest, CompressionConfigNone) {
    ParquetExportConfig config;
    config.compression = "none";
    ParquetExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/no_compression.parquet";

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_GT(stats.exported_entities, 0u);
}

TEST_F(ParquetExporterTest, SetConfigAndGetConfig) {
    ParquetExporter exporter;

    ParquetExportConfig cfg;
    cfg.row_group_size = 1024;
    cfg.compression    = "gzip";
    exporter.setConfig(cfg);

    EXPECT_EQ(exporter.getConfig().row_group_size, 1024u);
    EXPECT_EQ(exporter.getConfig().compression, "gzip");
}

TEST_F(ParquetExporterTest, ColumnHintsConfiguration) {
    ParquetExportConfig config;
    config.column_hints = {
        {"count",  ParquetColumnType::INT64},
        {"score",  ParquetColumnType::DOUBLE},
        {"instruction", ParquetColumnType::STRING},
    };
    ParquetExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/column_hints.parquet";

    auto stats = exporter.exportEntities(test_entities_, options);
    EXPECT_GT(stats.exported_entities, 0u);
}

TEST_F(ParquetExporterTest, FileMetadataWritten) {
    ParquetExportConfig config;
    config.file_metadata["source"]  = "themis_test";
    config.file_metadata["version"] = "1.0.0";
    ParquetExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/kv_meta.parquet";

    auto stats = exporter.exportEntities(test_entities_, options);
    EXPECT_GT(stats.exported_entities, 0u);
    EXPECT_TRUE(isValidParquetMagic(options.output_path));
}

// ─────────────────────────────────────────────────────────────────────────────
// Deduplication test
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ParquetExporterTest, DeduplicatesByPrimaryKey) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/dedup.parquet";

    // test_entities_ has 10 unique + 1 duplicate of entity_0 = 11 total
    auto stats = exporter.exportEntities(test_entities_, options);

    // Only 10 unique rows should be exported
    EXPECT_EQ(stats.exported_entities, 10u);
    EXPECT_EQ(stats.failed_entities, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Progress callback test
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ParquetExporterTest, ProgressCallbackInvoked) {
    ParquetExporter exporter;

    size_t cb_count = 0;
    ExportOptions options;
    options.output_path       = test_dir_ + "/progress.parquet";
    options.progress_interval = 3;
    options.progress_callback = [&cb_count](const ExportStats&) {
        ++cb_count;
    };

    exporter.exportEntities(test_entities_, options);

    EXPECT_GT(cb_count, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tenant isolation tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ParquetExporterTest, TenantIsolationWithMatchingTenant) {
    ParquetExporter exporter;

    for (auto& e : test_entities_) {
        e.setField("tenant_id", std::string("t-001"));
    }

    ExportOptions options;
    options.output_path = test_dir_ + "/tenant_match.parquet";

    ExportTenantContext ctx;
    ctx.tenant_id       = "t-001";
    ctx.user_id         = "u-001";
    ctx.scopes          = {"export:read"};
    ctx.enforce_isolation = true;
    options.tenant_context = ctx;

    auto stats = exporter.exportEntities(test_entities_, options);

    // All entities belong to t-001 so none should be filtered
    EXPECT_GT(stats.exported_entities, 0u);
    EXPECT_EQ(stats.errors.size(), 0u);
}

TEST_F(ParquetExporterTest, TenantIsolationBlocksCrossTenantRows) {
    ParquetExporter exporter;

    for (size_t i = 0; i < test_entities_.size(); ++i) {
        test_entities_[i].setField("tenant_id",
            std::string(i % 2 == 0 ? "t-001" : "t-002"));
    }

    ExportOptions options;
    options.output_path = test_dir_ + "/tenant_cross.parquet";

    ExportTenantContext ctx;
    ctx.tenant_id       = "t-001";
    ctx.user_id         = "u-001";
    ctx.scopes          = {"export:read"};
    ctx.enforce_isolation = true;
    options.tenant_context = ctx;

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_LT(stats.exported_entities, test_entities_.size());
}

TEST_F(ParquetExporterTest, TenantInsufficientScopesThrows) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/tenant_unauth.parquet";

    ExportTenantContext ctx;
    ctx.tenant_id       = "t-001";
    ctx.user_id         = "u-001";
    ctx.scopes          = {};  // no scopes
    ctx.enforce_isolation = true;
    options.tenant_context = ctx;

    EXPECT_THROW(
        exporter.exportEntities(test_entities_, options),
        ExporterException
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// PII detection / redaction tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ParquetExporterTest, PIIDetectionTracksHits) {
    ParquetExportConfig config;
    config.pii_config.enable_detection = true;
    config.pii_config.enable_redaction = false;
    config.pii_config.fail_on_pii      = false;
    ParquetExporter exporter(config);

    BaseEntity pii_entity;
    pii_entity.setPrimaryKey("pii_1");
    pii_entity.setField("instruction",
                        std::string("Contact me at user@example.com"));
    pii_entity.setField("output", std::string("OK"));

    std::vector<BaseEntity> entities = {pii_entity};

    ExportOptions options;
    options.output_path = test_dir_ + "/pii_detect.parquet";

    auto stats = exporter.exportEntities(entities, options);

    EXPECT_EQ(stats.exported_entities, 1u);
    EXPECT_GT(exporter.getMetrics()->getPIIDetections(), 0u);
    EXPECT_EQ(exporter.getMetrics()->getPIIRedactions(), 0u);
}

TEST_F(ParquetExporterTest, PIIRedactionMask) {
    ParquetExportConfig config;
    config.pii_config.enable_detection   = true;
    config.pii_config.enable_redaction   = true;
    config.pii_config.redaction_strategy = "mask";
    ParquetExporter exporter(config);

    BaseEntity pii_entity;
    pii_entity.setPrimaryKey("pii_2");
    pii_entity.setField("instruction",
                        std::string("Email me at secret@example.com"));
    pii_entity.setField("output", std::string("Noted"));

    std::vector<BaseEntity> entities = {pii_entity};

    ExportOptions options;
    options.output_path = test_dir_ + "/pii_redact.parquet";

    auto stats = exporter.exportEntities(entities, options);

    EXPECT_EQ(stats.exported_entities, 1u);
    EXPECT_GT(exporter.getMetrics()->getPIIDetections(), 0u);
    EXPECT_GT(exporter.getMetrics()->getPIIRedactions(), 0u);
}

TEST_F(ParquetExporterTest, PIIFailOnDetectionThrows) {
    ParquetExportConfig config;
    config.pii_config.enable_detection = true;
    config.pii_config.enable_redaction = false;
    config.pii_config.fail_on_pii      = true;
    ParquetExporter exporter(config);

    BaseEntity pii_entity;
    pii_entity.setPrimaryKey("pii_3");
    pii_entity.setField("instruction",
                        std::string("Call 555-123-4567 for info"));
    pii_entity.setField("output", std::string("Sure"));

    std::vector<BaseEntity> entities = {pii_entity};

    ExportOptions options;
    options.output_path = test_dir_ + "/pii_fail.parquet";

    EXPECT_THROW(
        exporter.exportEntities(entities, options),
        ExporterException
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Error handling tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ParquetExporterTest, InvalidOutputPathErrors) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = "/nonexistent/path/output.parquet";

    auto stats = exporter.exportEntities(test_entities_, options);

    // Should record errors and export no entities
    EXPECT_GT(stats.errors.size(), 0u);
    EXPECT_EQ(stats.exported_entities, 0u);
}

TEST_F(ParquetExporterTest, EmptyOutputPathThrows) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = "";

    EXPECT_THROW(
        exporter.exportEntities(test_entities_, options),
        ConfigException
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Metrics tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ParquetExporterTest, MetricsRecordedAfterExport) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/metrics.parquet";

    exporter.exportEntities(test_entities_, options);

    auto metrics = exporter.getMetrics();
    ASSERT_NE(metrics, nullptr);
    EXPECT_GE(metrics->getExportRate(), 0.0);
}

TEST_F(ParquetExporterTest, MetricsResetWorks) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/metrics_reset.parquet";
    exporter.exportEntities(test_entities_, options);

    exporter.resetMetrics();
    // After reset, a fresh export should still work
    options.output_path = test_dir_ + "/metrics_reset2.parquet";
    auto stats = exporter.exportEntities(test_entities_, options);
    EXPECT_GT(stats.exported_entities, 0u);
}

TEST_F(ParquetExporterTest, ParquetBytesWrittenCounterTracked) {
    // Verifies the exporter_parquet_bytes_written_total counter is updated
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/bytes_counter.parquet";

    exporter.exportEntities(test_entities_, options);

    EXPECT_GT(exporter.getMetrics()->getParquetBytesWritten(), 0u);
}

TEST_F(ParquetExporterTest, ParquetBytesCounterMatchesBytesWritten) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/bytes_match.parquet";

    auto stats = exporter.exportEntities(test_entities_, options);

    // The Parquet bytes counter must equal the reported bytes_written
    EXPECT_EQ(exporter.getMetrics()->getParquetBytesWritten(),
              stats.bytes_written);
}

TEST_F(ParquetExporterTest, ParquetBytesCounterAccumulatesAcrossExports) {
    ParquetExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/accum1.parquet";
    auto stats1 = exporter.exportEntities(test_entities_, options);

    options.output_path = test_dir_ + "/accum2.parquet";
    auto stats2 = exporter.exportEntities(test_entities_, options);

    size_t expected = stats1.bytes_written + stats2.bytes_written;
    EXPECT_EQ(exporter.getMetrics()->getParquetBytesWritten(), expected);
}

// ─────────────────────────────────────────────────────────────────────────────
// Interface tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ParquetExporterTest, GetName) {
    ParquetExporter exporter;
    EXPECT_EQ(exporter.getName(), "parquet_exporter");
}

TEST_F(ParquetExporterTest, GetVersion) {
    ParquetExporter exporter;
    EXPECT_EQ(exporter.getVersion(), "1.0.0");
}

TEST_F(ParquetExporterTest, GetSupportedFormats) {
    ParquetExporter exporter;
    auto fmts = exporter.getSupportedFormats();
    EXPECT_EQ(fmts.size(), 1u);
    EXPECT_EQ(fmts[0], "parquet");
}

TEST_F(ParquetExporterTest, IsArrowAvailableReturnsBool) {
    // Should not throw; the exact value depends on compile flags
    bool available = ParquetExporter::isArrowAvailable();
    (void)available;  // value is compile-time, just ensure it compiles
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// Large-batch test (exercises row-group flushing logic)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ParquetExporterTest, LargeBatchExport) {
    ParquetExportConfig config;
    config.row_group_size = 8;
    ParquetExporter exporter(config);

    std::vector<BaseEntity> large_batch = {};

    for (int i = 0; i < 40; ++i) {
        BaseEntity e;
        e.setPrimaryKey("large_" + std::to_string(i));
        e.setField("text", std::string("Row ") + std::to_string(i));
        large_batch.push_back(e);
    }

    ExportOptions options;
    options.output_path = test_dir_ + "/large_batch.parquet";

    auto stats = exporter.exportEntities(large_batch, options);

    EXPECT_EQ(stats.exported_entities, 40u);
    EXPECT_TRUE(isValidParquetMagic(options.output_path));
    EXPECT_TRUE(hasValidParquetFooter(options.output_path));
}
