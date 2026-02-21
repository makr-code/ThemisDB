/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_arrow_export.cpp                              ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     700                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "analytics/arrow_export.h"
#include "analytics/analytics_export.h"
#include <fstream>
#include <filesystem>

using namespace themis::analytics;

class ArrowExportTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test output directory if it doesn't exist
        test_dir_ = "/tmp/themis_arrow_test";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        // Clean up test files
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::string test_dir_;
};

// ===== ArrowRecordBatch Tests =====

TEST_F(ArrowExportTest, CreateEmptyBatch) {
    ArrowRecordBatch batch;
    
    EXPECT_EQ(batch.rowCount(), 0);
    EXPECT_EQ(batch.columnCount(), 0);
}

TEST_F(ArrowExportTest, AddColumns) {
    ArrowRecordBatch batch;
    
    ArrowRecordBatch::ColumnSchema col1;
    col1.name = "id";
    col1.type = ArrowRecordBatch::DataType::INT64;
    col1.nullable = false;
    
    ArrowRecordBatch::ColumnSchema col2;
    col2.name = "name";
    col2.type = ArrowRecordBatch::DataType::STRING;
    col2.nullable = true;
    
    ArrowRecordBatch::ColumnSchema col3;
    col3.name = "value";
    col3.type = ArrowRecordBatch::DataType::DOUBLE;
    col3.nullable = true;
    
    batch.addColumn(col1);
    batch.addColumn(col2);
    batch.addColumn(col3);
    
    EXPECT_EQ(batch.columnCount(), 3);
    EXPECT_EQ(batch.getColumn(0).schema.name, "id");
    EXPECT_EQ(batch.getColumn(1).schema.name, "name");
    EXPECT_EQ(batch.getColumn(2).schema.name, "value");
}

TEST_F(ArrowExportTest, AppendRows) {
    ArrowRecordBatch batch;
    
    // Define schema
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"name", ArrowRecordBatch::DataType::STRING, true});
    batch.addColumn({"score", ArrowRecordBatch::DataType::DOUBLE, true});
    
    // Append rows
    batch.appendRow({int64_t(1), std::string("Alice"), 95.5});
    batch.appendRow({int64_t(2), std::string("Bob"), 87.3});
    batch.appendRow({int64_t(3), std::string("Charlie"), 92.0});
    
    EXPECT_EQ(batch.rowCount(), 3);
    EXPECT_EQ(batch.columnCount(), 3);
    
    // Verify first row
    const auto& id_col = batch.getColumn(0);
    EXPECT_EQ(std::get<int64_t>(id_col.data[0]), 1);
    EXPECT_FALSE(id_col.null_bitmap[0]);
    
    const auto& name_col = batch.getColumn(1);
    EXPECT_EQ(std::get<std::string>(name_col.data[0]), "Alice");
    
    const auto& score_col = batch.getColumn(2);
    EXPECT_DOUBLE_EQ(std::get<double>(score_col.data[0]), 95.5);
}

TEST_F(ArrowExportTest, AppendRowsWithNulls) {
    ArrowRecordBatch batch;
    
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"optional_field", ArrowRecordBatch::DataType::STRING, true});
    
    // Append row with null value
    batch.appendRow({int64_t(1), nullptr});
    batch.appendRow({int64_t(2), std::string("value")});
    
    EXPECT_EQ(batch.rowCount(), 2);
    
    const auto& opt_col = batch.getColumn(1);
    EXPECT_TRUE(opt_col.null_bitmap[0]);   // First row has null
    EXPECT_FALSE(opt_col.null_bitmap[1]);  // Second row has value
}

TEST_F(ArrowExportTest, ToJSON) {
    ArrowRecordBatch batch;
    
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"name", ArrowRecordBatch::DataType::STRING, true});
    batch.addColumn({"active", ArrowRecordBatch::DataType::BOOLEAN, false});
    
    batch.appendRow({int64_t(1), std::string("Test"), true});
    batch.appendRow({int64_t(2), nullptr, false});
    
    std::string json = batch.toJSON();
    
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("\"row_count\": 2"), std::string::npos);
    EXPECT_NE(json.find("\"column_count\": 3"), std::string::npos);
    EXPECT_NE(json.find("\"id\""), std::string::npos);
    EXPECT_NE(json.find("\"name\""), std::string::npos);
    EXPECT_NE(json.find("INT64"), std::string::npos);
    EXPECT_NE(json.find("STRING"), std::string::npos);
    EXPECT_NE(json.find("BOOLEAN"), std::string::npos);
}

TEST_F(ArrowExportTest, GetMetadata) {
    ArrowRecordBatch batch;
    
    batch.addColumn({"col1", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"col2", ArrowRecordBatch::DataType::DOUBLE, true});
    
    batch.appendRow({int64_t(100), 1.5});
    batch.appendRow({int64_t(200), 2.5});
    batch.appendRow({int64_t(300), 3.5});
    
    auto metadata = batch.getMetadata();
    
    EXPECT_EQ(metadata.row_count, 3);
    EXPECT_EQ(metadata.column_count, 2);
    EXPECT_GT(metadata.total_bytes, 0);
    EXPECT_EQ(metadata.schema.size(), 2);
    EXPECT_EQ(metadata.schema[0].name, "col1");
    EXPECT_EQ(metadata.schema[1].name, "col2");
}

TEST_F(ArrowExportTest, ClearBatch) {
    ArrowRecordBatch batch;
    
    batch.addColumn({"col1", ArrowRecordBatch::DataType::INT64, false});
    batch.appendRow({int64_t(1)});
    batch.appendRow({int64_t(2)});
    
    EXPECT_EQ(batch.rowCount(), 2);
    EXPECT_EQ(batch.columnCount(), 1);
    
    batch.clear();
    
    EXPECT_EQ(batch.rowCount(), 0);
    EXPECT_EQ(batch.columnCount(), 0);
}

// ===== Analytics Exporter Tests =====

TEST_F(ArrowExportTest, CreateDefaultExporter) {
    auto exporter = ExporterFactory::createDefaultExporter();
    
    EXPECT_NE(exporter, nullptr);
    EXPECT_FALSE(exporter->getExporterInfo().empty());
}

TEST_F(ArrowExportTest, ExporterSupportsFormats) {
    auto exporter = ExporterFactory::createDefaultExporter();
    
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::JSON));
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::CSV));

#ifdef THEMIS_HAS_ARROW
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::ARROW_IPC));
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::ARROW_PARQUET));
#else
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::ARROW_IPC));
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::ARROW_PARQUET));
#endif
}

TEST_F(ArrowExportTest, ExportToJSONString) {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"value", ArrowRecordBatch::DataType::DOUBLE, true});
    
    batch.appendRow({int64_t(1), 10.5});
    batch.appendRow({int64_t(2), 20.5});
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    ExportOptions options;
    options.format = ExportFormat::JSON;
    
    std::string result = exporter->exportToString(batch, options);
    
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("\"id\""), std::string::npos);
    EXPECT_NE(result.find("\"value\""), std::string::npos);
}

TEST_F(ArrowExportTest, ExportToCSVString) {
    ArrowRecordBatch batch;
    batch.addColumn({"product", ArrowRecordBatch::DataType::STRING, false});
    batch.addColumn({"price", ArrowRecordBatch::DataType::DOUBLE, false});
    batch.addColumn({"quantity", ArrowRecordBatch::DataType::INT64, false});
    
    batch.appendRow({std::string("Apple"), 1.5, int64_t(10)});
    batch.appendRow({std::string("Banana"), 0.8, int64_t(15)});
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    ExportOptions options;
    options.format = ExportFormat::CSV;
    
    std::string result = exporter->exportToString(batch, options);
    
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("product,price,quantity"), std::string::npos);
    EXPECT_NE(result.find("Apple"), std::string::npos);
    EXPECT_NE(result.find("Banana"), std::string::npos);
}

TEST_F(ArrowExportTest, ExportToFile) {
    ArrowRecordBatch batch;
    batch.addColumn({"timestamp", ArrowRecordBatch::DataType::TIMESTAMP, false});
    batch.addColumn({"temperature", ArrowRecordBatch::DataType::DOUBLE, false});
    
    batch.appendRow({int64_t(1706745600), 22.5});
    batch.appendRow({int64_t(1706745660), 22.7});
    batch.appendRow({int64_t(1706745720), 22.9});
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    ExportOptions options;
    options.format = ExportFormat::JSON;
    
    std::string output_path = test_dir_ + "/test_output.json";
    auto result = exporter->exportToFile(batch, output_path, options);
    
    EXPECT_EQ(result.status, ExportStatus::SUCCESS);
    EXPECT_EQ(result.rows_exported, 3);
    EXPECT_GT(result.bytes_written, 0);
    EXPECT_GT(result.duration_ms, 0.0);
    
    // Verify file exists and has content
    EXPECT_TRUE(std::filesystem::exists(output_path));
    
    std::ifstream infile(output_path);
    std::string content((std::istreambuf_iterator<char>(infile)),
                        std::istreambuf_iterator<char>());
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("timestamp"), std::string::npos);
}

TEST_F(ArrowExportTest, ExportWithCallback) {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    
    for (int i = 0; i < 100; ++i) {
        batch.appendRow({int64_t(i)});
    }
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    size_t total_bytes = 0;
    int callback_count = 0;
    
    auto callback = [&](const std::vector<uint8_t>& chunk) {
        total_bytes += chunk.size();
        callback_count++;
    };
    
    ExportOptions options;
    options.format = ExportFormat::JSON;
    options.batch_size = 1000;
    
    auto result = exporter->exportWithCallback(batch, callback, options);
    
    EXPECT_EQ(result.status, ExportStatus::SUCCESS);
    EXPECT_EQ(result.rows_exported, 100);
    EXPECT_GT(total_bytes, 0);
    EXPECT_GT(callback_count, 0);
}

TEST_F(ArrowExportTest, ExportLargeBatch) {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"value", ArrowRecordBatch::DataType::DOUBLE, false});
    
    // Create a larger dataset
    const size_t num_rows = 1000;
    for (size_t i = 0; i < num_rows; ++i) {
        batch.appendRow({int64_t(i), double(i) * 1.5});
    }
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    ExportOptions options;
    options.format = ExportFormat::CSV;
    
    std::string output_path = test_dir_ + "/large_output.csv";
    auto result = exporter->exportToFile(batch, output_path, options);
    
    EXPECT_EQ(result.status, ExportStatus::SUCCESS);
    EXPECT_EQ(result.rows_exported, num_rows);
    EXPECT_GT(result.bytes_written, num_rows);  // At least 1 byte per row
    
    // Verify file exists
    EXPECT_TRUE(std::filesystem::exists(output_path));
}

TEST_F(ArrowExportTest, ExportPlaceholderArrowFormat) {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.appendRow({int64_t(1)});
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    ExportOptions options;
    options.format = ExportFormat::ARROW_IPC;
    
    std::string result = exporter->exportToString(batch, options);
    
    EXPECT_FALSE(result.empty());
    
#ifdef THEMIS_HAS_ARROW
    // With Arrow, should get binary data
    EXPECT_EQ(result.find("ERROR"), std::string::npos);
#else
    // Without Arrow, should get error message
    EXPECT_NE(result.find("ERROR"), std::string::npos);
    EXPECT_NE(result.find("Arrow"), std::string::npos);
#endif
}

// ===== Integration Tests =====

TEST_F(ArrowExportTest, EndToEndWorkflow) {
    // 1. Create batch with sample analytics data
    ArrowRecordBatch batch;
    batch.addColumn({"date", ArrowRecordBatch::DataType::STRING, false});
    batch.addColumn({"region", ArrowRecordBatch::DataType::STRING, false});
    batch.addColumn({"sales", ArrowRecordBatch::DataType::DOUBLE, false});
    batch.addColumn({"units", ArrowRecordBatch::DataType::INT64, false});
    
    batch.appendRow({std::string("2024-01-01"), std::string("North"), 1500.0, int64_t(50)});
    batch.appendRow({std::string("2024-01-01"), std::string("South"), 2000.0, int64_t(75)});
    batch.appendRow({std::string("2024-01-02"), std::string("North"), 1800.0, int64_t(60)});
    batch.appendRow({std::string("2024-01-02"), std::string("South"), 2200.0, int64_t(80)});
    
    // 2. Get metadata
    auto metadata = batch.getMetadata();
    EXPECT_EQ(metadata.row_count, 4);
    EXPECT_EQ(metadata.column_count, 4);
    
    // 3. Export to JSON
    auto exporter = ExporterFactory::createDefaultExporter();
    
    ExportOptions json_options;
    json_options.format = ExportFormat::JSON;
    
    std::string json_path = test_dir_ + "/analytics.json";
    auto json_result = exporter->exportToFile(batch, json_path, json_options);
    EXPECT_EQ(json_result.status, ExportStatus::SUCCESS);
    
    // 4. Export to CSV
    ExportOptions csv_options;
    csv_options.format = ExportFormat::CSV;
    
    std::string csv_path = test_dir_ + "/analytics.csv";
    auto csv_result = exporter->exportToFile(batch, csv_path, csv_options);
    EXPECT_EQ(csv_result.status, ExportStatus::SUCCESS);
    
    // 5. Verify both files exist
    EXPECT_TRUE(std::filesystem::exists(json_path));
    EXPECT_TRUE(std::filesystem::exists(csv_path));
}

// ===== Arrow Export Tests (when THEMIS_HAS_ARROW is enabled) =====

TEST_F(ArrowExportTest, ArrowIPCExport) {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"value", ArrowRecordBatch::DataType::DOUBLE, false});
    batch.addColumn({"name", ArrowRecordBatch::DataType::STRING, true});
    
    batch.appendRow({int64_t(1), 10.5, std::string("Alice")});
    batch.appendRow({int64_t(2), 20.5, std::string("Bob")});
    batch.appendRow({int64_t(3), 30.5, nullptr});
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    ExportOptions options;
    options.format = ExportFormat::ARROW_IPC;
    
    std::string output_path = test_dir_ + "/test_arrow.ipc";
    auto result = exporter->exportToFile(batch, output_path, options);
    
#ifdef THEMIS_HAS_ARROW
    // With Arrow support
    EXPECT_EQ(result.status, ExportStatus::SUCCESS);
    EXPECT_EQ(result.rows_exported, 3);
    EXPECT_GT(result.bytes_written, 0);
    EXPECT_TRUE(std::filesystem::exists(output_path));
#else
    // Without Arrow support
    EXPECT_EQ(result.status, ExportStatus::NOT_SUPPORTED);
    EXPECT_FALSE(result.message.empty());
    EXPECT_NE(result.message.find("Arrow"), std::string::npos);
#endif
}

TEST_F(ArrowExportTest, ParquetExport) {
    ArrowRecordBatch batch;
    batch.addColumn({"timestamp", ArrowRecordBatch::DataType::TIMESTAMP, false});
    batch.addColumn({"sensor_id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"temperature", ArrowRecordBatch::DataType::DOUBLE, true});
    batch.addColumn({"active", ArrowRecordBatch::DataType::BOOLEAN, false});
    
    for (int i = 0; i < 100; ++i) {
        batch.appendRow({
            int64_t(1706745600 + i * 60),
            int64_t(i % 10),
            i % 3 == 0 ? nullptr : std::variant<std::nullptr_t, int64_t, double, std::string, bool>(22.5 + i * 0.1),
            (i % 2 == 0)
        });
    }
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    ExportOptions options;
    options.format = ExportFormat::ARROW_PARQUET;
    options.compress = true;
    options.compression_codec = "snappy";
    
    std::string output_path = test_dir_ + "/test_data.parquet";
    auto result = exporter->exportToFile(batch, output_path, options);
    
#ifdef THEMIS_HAS_ARROW
    EXPECT_EQ(result.status, ExportStatus::SUCCESS);
    EXPECT_EQ(result.rows_exported, 100);
    EXPECT_GT(result.bytes_written, 0);
    EXPECT_TRUE(std::filesystem::exists(output_path));
    
    // Verify compression worked (file size should be reasonable)
    size_t file_size = std::filesystem::file_size(output_path);
    EXPECT_LT(file_size, 10000);  // Should be small due to compression
#else
    EXPECT_EQ(result.status, ExportStatus::NOT_SUPPORTED);
    EXPECT_FALSE(result.message.empty());
#endif
}

TEST_F(ArrowExportTest, FeatherExport) {
    ArrowRecordBatch batch;
    batch.addColumn({"product_id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"product_name", ArrowRecordBatch::DataType::STRING, false});
    batch.addColumn({"price", ArrowRecordBatch::DataType::DOUBLE, false});
    batch.addColumn({"in_stock", ArrowRecordBatch::DataType::BOOLEAN, false});
    
    batch.appendRow({int64_t(1), std::string("Laptop"), 999.99, true});
    batch.appendRow({int64_t(2), std::string("Mouse"), 29.99, true});
    batch.appendRow({int64_t(3), std::string("Keyboard"), 79.99, false});
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    ExportOptions options;
    options.format = ExportFormat::ARROW_FEATHER;
    
    std::string output_path = test_dir_ + "/test_data.feather";
    auto result = exporter->exportToFile(batch, output_path, options);
    
#ifdef THEMIS_HAS_ARROW
    EXPECT_EQ(result.status, ExportStatus::SUCCESS);
    EXPECT_EQ(result.rows_exported, 3);
    EXPECT_GT(result.bytes_written, 0);
    EXPECT_TRUE(std::filesystem::exists(output_path));
#else
    EXPECT_EQ(result.status, ExportStatus::NOT_SUPPORTED);
#endif
}

TEST_F(ArrowExportTest, ArrowStringExport) {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"name", ArrowRecordBatch::DataType::STRING, false});
    
    batch.appendRow({int64_t(1), std::string("Test")});
    batch.appendRow({int64_t(2), std::string("Data")});
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    ExportOptions options;
    options.format = ExportFormat::ARROW_IPC;
    
    std::string result = exporter->exportToString(batch, options);
    
#ifdef THEMIS_HAS_ARROW
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.find("ERROR"), std::string::npos);
#else
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("ERROR"), std::string::npos);
    EXPECT_NE(result.find("Arrow"), std::string::npos);
#endif
}

// ===== Negative Tests =====

TEST_F(ArrowExportTest, ExportToUnwritablePath) {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.appendRow({int64_t(1)});
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    ExportOptions options;
    options.format = ExportFormat::JSON;
    
    // Try to write to a path that doesn't exist and can't be created
    std::string bad_path = "/nonexistent_directory_12345/subdir/file.json";
    auto result = exporter->exportToFile(batch, bad_path, options);
    
    EXPECT_EQ(result.status, ExportStatus::FAILED);
    EXPECT_FALSE(result.message.empty());
}

TEST_F(ArrowExportTest, FormatSupportCheck) {
    auto exporter = ExporterFactory::createDefaultExporter();
    
    // JSON and CSV should always be supported
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::JSON));
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::CSV));
    
    // Arrow formats depend on compile flag
#ifdef THEMIS_HAS_ARROW
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::ARROW_IPC));
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::ARROW_PARQUET));
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::ARROW_FEATHER));
#else
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::ARROW_IPC));
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::ARROW_PARQUET));
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::ARROW_FEATHER));
#endif
}

TEST_F(ArrowExportTest, ExporterInfo) {
    auto exporter = ExporterFactory::createDefaultExporter();
    std::string info = exporter->getExporterInfo();
    
    EXPECT_FALSE(info.empty());
    EXPECT_NE(info.find("AnalyticsExporter"), std::string::npos);
    
#ifdef THEMIS_HAS_ARROW
    EXPECT_NE(info.find("Arrow enabled"), std::string::npos);
#else
    EXPECT_NE(info.find("JSON/CSV only"), std::string::npos);
#endif
}

TEST_F(ArrowExportTest, NullBitmapHandling) {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"optional_value", ArrowRecordBatch::DataType::DOUBLE, true});
    batch.addColumn({"optional_text", ArrowRecordBatch::DataType::STRING, true});
    
    // Mix of null and non-null values
    batch.appendRow({int64_t(1), 10.5, std::string("text1")});
    batch.appendRow({int64_t(2), nullptr, std::string("text2")});
    batch.appendRow({int64_t(3), 30.5, nullptr});
    batch.appendRow({int64_t(4), nullptr, nullptr});
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    // Test CSV export with nulls
    ExportOptions csv_options;
    csv_options.format = ExportFormat::CSV;
    std::string csv_result = exporter->exportToString(batch, csv_options);
    EXPECT_FALSE(csv_result.empty());
    
    // Test JSON export with nulls
    ExportOptions json_options;
    json_options.format = ExportFormat::JSON;
    std::string json_result = exporter->exportToString(batch, json_options);
    EXPECT_FALSE(json_result.empty());
    EXPECT_NE(json_result.find("null"), std::string::npos);
    
#ifdef THEMIS_HAS_ARROW
    // Test Arrow export with nulls
    ExportOptions arrow_options;
    arrow_options.format = ExportFormat::ARROW_IPC;
    std::string arrow_path = test_dir_ + "/nulls_test.ipc";
    auto arrow_result = exporter->exportToFile(batch, arrow_path, arrow_options);
    EXPECT_EQ(arrow_result.status, ExportStatus::SUCCESS);
    EXPECT_EQ(arrow_result.rows_exported, 4);
#endif
}

TEST_F(ArrowExportTest, EmptyBatchExport) {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"value", ArrowRecordBatch::DataType::DOUBLE, false});
    
    // No rows appended
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    // Test JSON export of empty batch
    ExportOptions options;
    options.format = ExportFormat::JSON;
    
    std::string result = exporter->exportToString(batch, options);
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("\"row_count\": 0"), std::string::npos);
}

TEST_F(ArrowExportTest, LargeDatasetExport) {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"value", ArrowRecordBatch::DataType::DOUBLE, false});
    batch.addColumn({"category", ArrowRecordBatch::DataType::STRING, false});
    
    // Create a larger dataset
    const size_t num_rows = 10000;
    for (size_t i = 0; i < num_rows; ++i) {
        batch.appendRow({
            int64_t(i),
            double(i) * 1.5,
            std::string("category_") + std::to_string(i % 10)
        });
    }
    
    auto exporter = ExporterFactory::createDefaultExporter();
    
    // Test CSV export
    ExportOptions csv_options;
    csv_options.format = ExportFormat::CSV;
    std::string csv_path = test_dir_ + "/large_data.csv";
    auto csv_result = exporter->exportToFile(batch, csv_path, csv_options);
    EXPECT_EQ(csv_result.status, ExportStatus::SUCCESS);
    EXPECT_EQ(csv_result.rows_exported, num_rows);
    EXPECT_GT(csv_result.duration_ms, 0.0);
    
#ifdef THEMIS_HAS_ARROW
    // Test Parquet export with compression
    ExportOptions parquet_options;
    parquet_options.format = ExportFormat::ARROW_PARQUET;
    parquet_options.compress = true;
    parquet_options.compression_codec = "zstd";
    parquet_options.compression_level = 3;
    
    std::string parquet_path = test_dir_ + "/large_data.parquet";
    auto parquet_result = exporter->exportToFile(batch, parquet_path, parquet_options);
    EXPECT_EQ(parquet_result.status, ExportStatus::SUCCESS);
    EXPECT_EQ(parquet_result.rows_exported, num_rows);
    EXPECT_GT(parquet_result.bytes_written, 0);
    
    // Verify Parquet file is smaller than CSV due to compression
    size_t csv_size = std::filesystem::file_size(csv_path);
    size_t parquet_size = std::filesystem::file_size(parquet_path);
    EXPECT_LT(parquet_size, csv_size);
#endif
}

