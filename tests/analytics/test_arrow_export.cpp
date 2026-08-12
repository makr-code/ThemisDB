#include <gtest/gtest.h>
#include "analytics/arrow_export.h"
#include "analytics/analytics_export.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>

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

    // The default (JSONCSVExporter) always supports JSON and CSV
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::JSON));
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::CSV));

    // The default exporter does not handle Arrow formats regardless of Arrow support
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::FMT_ARROW_IPC));
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::FMT_ARROW_PARQUET));

#ifdef THEMIS_HAS_ARROW
    // Format-specific Arrow exporters support their own format
    auto ipc_exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_IPC);
    EXPECT_TRUE(ipc_exporter->supportsFormat(ExportFormat::FMT_ARROW_IPC));
    auto parquet_exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_PARQUET);
    EXPECT_TRUE(parquet_exporter->supportsFormat(ExportFormat::FMT_ARROW_PARQUET));
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

// ===== CSV Sanitization Tests =====

TEST_F(ArrowExportTest, CSVExport_FormulaInjection_EqualSign) {
    // Strings starting with '=' must be quoted to prevent formula injection
    ArrowRecordBatch batch;
    batch.addColumn({"formula", ArrowRecordBatch::DataType::STRING, false});
    batch.appendRow({std::string("=SUM(A1:A10)")});

    auto exporter = ExporterFactory::createDefaultExporter();
    ExportOptions options;
    options.format = ExportFormat::CSV;

    std::string csv = exporter->exportToString(batch, options);

    // The value must be quoted in the output
    EXPECT_NE(csv.find("\"=SUM(A1:A10)\""), std::string::npos);
}

TEST_F(ArrowExportTest, CSVExport_FormulaInjection_PlusSign) {
    ArrowRecordBatch batch;
    batch.addColumn({"cmd", ArrowRecordBatch::DataType::STRING, false});
    batch.appendRow({std::string("+CMD|' /C calc'!A0")});

    auto exporter = ExporterFactory::createDefaultExporter();
    ExportOptions options;
    options.format = ExportFormat::CSV;

    std::string csv = exporter->exportToString(batch, options);

    // Must be quoted
    EXPECT_NE(csv.find("\"+CMD"), std::string::npos);
    // Verify it is indeed enclosed in double-quotes
    auto pos = csv.find('+');
    ASSERT_NE(pos, std::string::npos);
    EXPECT_GT(pos, 0u);
    EXPECT_EQ(csv[pos - 1], '"');
}

TEST_F(ArrowExportTest, CSVExport_FormulaInjection_AtSign) {
    ArrowRecordBatch batch;
    batch.addColumn({"ref", ArrowRecordBatch::DataType::STRING, false});
    batch.appendRow({std::string("@SUM(1+1)")});

    auto exporter = ExporterFactory::createDefaultExporter();
    ExportOptions options;
    options.format = ExportFormat::CSV;

    std::string csv = exporter->exportToString(batch, options);

    EXPECT_NE(csv.find("\"@SUM"), std::string::npos);
}

TEST_F(ArrowExportTest, CSVExport_FormulaInjection_MinusSign) {
    ArrowRecordBatch batch;
    batch.addColumn({"val", ArrowRecordBatch::DataType::STRING, false});
    batch.appendRow({std::string("-1+2")});

    auto exporter = ExporterFactory::createDefaultExporter();
    ExportOptions options;
    options.format = ExportFormat::CSV;

    std::string csv = exporter->exportToString(batch, options);

    EXPECT_NE(csv.find("\"-1+2\""), std::string::npos);
}

TEST_F(ArrowExportTest, CSVExport_NewlineInString_IsQuoted) {
    // Strings containing newlines must be quoted (RFC 4180)
    ArrowRecordBatch batch;
    batch.addColumn({"notes", ArrowRecordBatch::DataType::STRING, false});
    batch.appendRow({std::string("line1\nline2")});

    auto exporter = ExporterFactory::createDefaultExporter();
    ExportOptions options;
    options.format = ExportFormat::CSV;

    std::string csv = exporter->exportToString(batch, options);

    // The embedded newline value must be wrapped in double-quotes
    EXPECT_NE(csv.find("\"line1\nline2\""), std::string::npos);
}

TEST_F(ArrowExportTest, CSVExport_CarriageReturnInString_IsQuoted) {
    ArrowRecordBatch batch;
    batch.addColumn({"data", ArrowRecordBatch::DataType::STRING, false});
    batch.appendRow({std::string("before\rafter")});

    auto exporter = ExporterFactory::createDefaultExporter();
    ExportOptions options;
    options.format = ExportFormat::CSV;

    std::string csv = exporter->exportToString(batch, options);

    EXPECT_NE(csv.find("\"before\rafter\""), std::string::npos);
}

TEST_F(ArrowExportTest, CSVExport_SafeString_NotQuoted) {
    // Ordinary strings that don't need quoting should remain unquoted
    ArrowRecordBatch batch;
    batch.addColumn({"name", ArrowRecordBatch::DataType::STRING, false});
    batch.appendRow({std::string("hello world")});

    auto exporter = ExporterFactory::createDefaultExporter();
    ExportOptions options;
    options.format = ExportFormat::CSV;

    std::string csv = exporter->exportToString(batch, options);

    EXPECT_NE(csv.find("hello world"), std::string::npos);
    // Should not be wrapped in extra quotes
    EXPECT_EQ(csv.find("\"hello world\""), std::string::npos);
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

#ifdef THEMIS_HAS_ARROW
    // With Arrow, createExporter(FMT_ARROW_IPC) returns a real ArrowIPCExporter
    auto exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_IPC);
    ASSERT_NE(exporter, nullptr);

    ExportOptions options;
    options.format = ExportFormat::FMT_ARROW_IPC;
    std::string result = exporter->exportToString(batch, options);

    EXPECT_FALSE(result.empty());
    // With Arrow, should get binary data (no ERROR prefix)
    EXPECT_EQ(result.find("ERROR"), std::string::npos);
#else
    // Without Arrow, createExporter for Arrow formats must throw
    EXPECT_THROW(ExporterFactory::createExporter(ExportFormat::FMT_ARROW_IPC), std::runtime_error);
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

#ifdef THEMIS_HAS_ARROW
    auto exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_IPC);
    ASSERT_NE(exporter, nullptr);

    ExportOptions options;
    options.format = ExportFormat::FMT_ARROW_IPC;

    std::string output_path = test_dir_ + "/test_arrow.ipc";
    auto result = exporter->exportToFile(batch, output_path, options);

    // With Arrow support
    EXPECT_EQ(result.status, ExportStatus::SUCCESS);
    EXPECT_EQ(result.rows_exported, 3);
    EXPECT_GT(result.bytes_written, 0);
    EXPECT_TRUE(std::filesystem::exists(output_path));
#else
    // Without Arrow support, factory throws
    EXPECT_THROW(ExporterFactory::createExporter(ExportFormat::FMT_ARROW_IPC), std::runtime_error);
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

#ifdef THEMIS_HAS_ARROW
    auto exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_PARQUET);
    ASSERT_NE(exporter, nullptr);

    ExportOptions options;
    options.format = ExportFormat::FMT_ARROW_PARQUET;
    options.compress = true;
    options.compression_codec = "snappy";

    std::string output_path = test_dir_ + "/test_data.parquet";
    auto result = exporter->exportToFile(batch, output_path, options);

    EXPECT_EQ(result.status, ExportStatus::SUCCESS);
    EXPECT_EQ(result.rows_exported, 100);
    EXPECT_GT(result.bytes_written, 0);
    EXPECT_TRUE(std::filesystem::exists(output_path));

    // Verify compression worked (file size should be reasonable)
    size_t file_size = std::filesystem::file_size(output_path);
    EXPECT_LT(file_size, 10000);  // Should be small due to compression
#else
    EXPECT_THROW(ExporterFactory::createExporter(ExportFormat::FMT_ARROW_PARQUET), std::runtime_error);
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

#ifdef THEMIS_HAS_ARROW
    auto exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_FEATHER);
    ASSERT_NE(exporter, nullptr);

    ExportOptions options;
    options.format = ExportFormat::FMT_ARROW_FEATHER;

    std::string output_path = test_dir_ + "/test_data.feather";
    auto result = exporter->exportToFile(batch, output_path, options);

    EXPECT_EQ(result.status, ExportStatus::SUCCESS);
    EXPECT_EQ(result.rows_exported, 3);
    EXPECT_GT(result.bytes_written, 0);
    EXPECT_TRUE(std::filesystem::exists(output_path));
#else
    EXPECT_THROW(ExporterFactory::createExporter(ExportFormat::FMT_ARROW_FEATHER), std::runtime_error);
#endif
}

TEST_F(ArrowExportTest, ArrowStringExport) {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"name", ArrowRecordBatch::DataType::STRING, false});
    
    batch.appendRow({int64_t(1), std::string("Test")});
    batch.appendRow({int64_t(2), std::string("Data")});

#ifdef THEMIS_HAS_ARROW
    auto exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_IPC);
    ASSERT_NE(exporter, nullptr);

    ExportOptions options;
    options.format = ExportFormat::FMT_ARROW_IPC;

    std::string result = exporter->exportToString(batch, options);

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.find("ERROR"), std::string::npos);
#else
    EXPECT_THROW(ExporterFactory::createExporter(ExportFormat::FMT_ARROW_IPC), std::runtime_error);
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

    // JSON and CSV should always be supported by the default exporter
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::JSON));
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::CSV));

    // Default exporter (JSONCSVExporter) does not support Arrow formats
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::FMT_ARROW_IPC));
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::FMT_ARROW_PARQUET));
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::FMT_ARROW_FEATHER));

#ifdef THEMIS_HAS_ARROW
    // Arrow-specific exporters support their respective formats only
    auto ipc_exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_IPC);
    EXPECT_TRUE(ipc_exporter->supportsFormat(ExportFormat::FMT_ARROW_IPC));
    auto parquet_exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_PARQUET);
    EXPECT_TRUE(parquet_exporter->supportsFormat(ExportFormat::FMT_ARROW_PARQUET));
    auto feather_exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_FEATHER);
    EXPECT_TRUE(feather_exporter->supportsFormat(ExportFormat::FMT_ARROW_FEATHER));
#endif
}

TEST_F(ArrowExportTest, ExporterInfo) {
    auto exporter = ExporterFactory::createDefaultExporter();
    std::string info = exporter->getExporterInfo();

    EXPECT_FALSE(info.empty());
    EXPECT_NE(info.find("JSONCSVExporter"), std::string::npos);
    EXPECT_NE(info.find("JSON/CSV"), std::string::npos);
}

#ifdef THEMIS_HAS_ARROW
// ===== ExporterFactory concrete-type tests =====

TEST_F(ArrowExportTest, CreateExporterParquetReturnsParquetType) {
    // When THEMIS_HAS_ARROW is defined, createExporter(FMT_ARROW_PARQUET) must
    // return a real ParquetExporter, not a stub or generic fallback.
    auto exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_PARQUET);

    ASSERT_NE(exporter, nullptr);

    // The exporter must claim support for Parquet and no other formats
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::FMT_ARROW_PARQUET));
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::JSON));
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::CSV));
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::FMT_ARROW_IPC));
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::FMT_ARROW_FEATHER));

    // Info string must identify the concrete ParquetExporter class
    std::string info = exporter->getExporterInfo();
    EXPECT_NE(info.find("ParquetExporter"), std::string::npos);
}

TEST_F(ArrowExportTest, CreateExporterArrowIPCReturnsIPCType) {
    auto exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_IPC);

    ASSERT_NE(exporter, nullptr);
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::FMT_ARROW_IPC));
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::JSON));

    std::string info = exporter->getExporterInfo();
    EXPECT_NE(info.find("ArrowIPCExporter"), std::string::npos);
}

TEST_F(ArrowExportTest, CreateExporterFeatherReturnsFeatherType) {
    auto exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_FEATHER);

    ASSERT_NE(exporter, nullptr);
    EXPECT_TRUE(exporter->supportsFormat(ExportFormat::FMT_ARROW_FEATHER));
    EXPECT_FALSE(exporter->supportsFormat(ExportFormat::JSON));

    std::string info = exporter->getExporterInfo();
    EXPECT_NE(info.find("FeatherExporter"), std::string::npos);
}

#else // !THEMIS_HAS_ARROW

TEST_F(ArrowExportTest, CreateExporterArrowFormatsThrowWithoutArrow) {
    EXPECT_THROW(ExporterFactory::createExporter(ExportFormat::FMT_ARROW_IPC), std::runtime_error);
    EXPECT_THROW(ExporterFactory::createExporter(ExportFormat::FMT_ARROW_PARQUET), std::runtime_error);
    EXPECT_THROW(ExporterFactory::createExporter(ExportFormat::FMT_ARROW_FEATHER), std::runtime_error);
}

#endif // THEMIS_HAS_ARROW

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
    // Test Arrow export with nulls using the dedicated IPC exporter
    auto arrow_exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_IPC);
    ExportOptions arrow_options;
    arrow_options.format = ExportFormat::FMT_ARROW_IPC;
    std::string arrow_path = test_dir_ + "/nulls_test.ipc";
    auto arrow_result = arrow_exporter->exportToFile(batch, arrow_path, arrow_options);
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
    // Test Parquet export with compression using the dedicated Parquet exporter
    auto parquet_exporter = ExporterFactory::createExporter(ExportFormat::FMT_ARROW_PARQUET);
    ExportOptions parquet_options;
    parquet_options.format = ExportFormat::FMT_ARROW_PARQUET;
    parquet_options.compress = true;
    parquet_options.compression_codec = "zstd";
    parquet_options.compression_level = 3;
    
    std::string parquet_path = test_dir_ + "/large_data.parquet";
    auto parquet_result = parquet_exporter->exportToFile(batch, parquet_path, parquet_options);
    EXPECT_EQ(parquet_result.status, ExportStatus::SUCCESS);
    EXPECT_EQ(parquet_result.rows_exported, num_rows);
    EXPECT_GT(parquet_result.bytes_written, 0);
    
    // Verify Parquet file is smaller than CSV due to compression
    size_t csv_size = std::filesystem::file_size(csv_path);
    size_t parquet_size = std::filesystem::file_size(parquet_path);
    EXPECT_LT(parquet_size, csv_size);
#endif
}

// ===== Zero-Copy Buffer Tests =====

TEST_F(ArrowExportTest, ZeroCopyInt64BufferPopulated) {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});

    batch.appendRow({int64_t(10)});
    batch.appendRow({int64_t(20)});
    batch.appendRow({int64_t(30)});

    // int64_buffer should be populated for INT64 columns
    const auto& col = batch.getColumn(0);
    ASSERT_EQ(col.int64_buffer.size(), 3u);
    EXPECT_EQ(col.int64_buffer[0], 10);
    EXPECT_EQ(col.int64_buffer[1], 20);
    EXPECT_EQ(col.int64_buffer[2], 30);
}

TEST_F(ArrowExportTest, ZeroCopyDoubleBufferPopulated) {
    ArrowRecordBatch batch;
    batch.addColumn({"value", ArrowRecordBatch::DataType::DOUBLE, false});

    batch.appendRow({1.1});
    batch.appendRow({2.2});

    const auto& col = batch.getColumn(0);
    ASSERT_EQ(col.double_buffer.size(), 2u);
    EXPECT_DOUBLE_EQ(col.double_buffer[0], 1.1);
    EXPECT_DOUBLE_EQ(col.double_buffer[1], 2.2);
}

TEST_F(ArrowExportTest, ZeroCopyTimestampBufferPopulated) {
    ArrowRecordBatch batch;
    batch.addColumn({"ts", ArrowRecordBatch::DataType::TIMESTAMP, false});

    batch.appendRow({int64_t(1706745600000LL)});
    batch.appendRow({int64_t(1706745660000LL)});

    const auto& col = batch.getColumn(0);
    ASSERT_EQ(col.int64_buffer.size(), 2u);
    EXPECT_EQ(col.int64_buffer[0], 1706745600000LL);
    EXPECT_EQ(col.int64_buffer[1], 1706745660000LL);
}

TEST_F(ArrowExportTest, ZeroCopyStringColumnHasNoTypedBuffer) {
    ArrowRecordBatch batch;
    batch.addColumn({"name", ArrowRecordBatch::DataType::STRING, false});

    batch.appendRow({std::string("Alice")});

    // String columns must not populate typed numeric buffers
    const auto& col = batch.getColumn(0);
    EXPECT_TRUE(col.int64_buffer.empty());
    EXPECT_TRUE(col.double_buffer.empty());
}

TEST_F(ArrowExportTest, ZeroCopyBoolColumnHasNoTypedBuffer) {
    ArrowRecordBatch batch;
    batch.addColumn({"active", ArrowRecordBatch::DataType::BOOLEAN, false});

    batch.appendRow({true});

    const auto& col = batch.getColumn(0);
    EXPECT_TRUE(col.int64_buffer.empty());
    EXPECT_TRUE(col.double_buffer.empty());
}

TEST_F(ArrowExportTest, GetInt64DataReturnsRawPointer) {
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});

    batch.appendRow({int64_t(100)});
    batch.appendRow({int64_t(200)});
    batch.appendRow({int64_t(300)});

    const int64_t* ptr = batch.getInt64Data(0);
    ASSERT_NE(ptr, nullptr);
    // The pointer should match the vector's internal buffer
    EXPECT_EQ(ptr[0], 100);
    EXPECT_EQ(ptr[1], 200);
    EXPECT_EQ(ptr[2], 300);
}

TEST_F(ArrowExportTest, GetDoubleDataReturnsRawPointer) {
    ArrowRecordBatch batch;
    batch.addColumn({"value", ArrowRecordBatch::DataType::DOUBLE, false});

    batch.appendRow({9.5});
    batch.appendRow({19.5});

    const double* ptr = batch.getDoubleData(0);
    ASSERT_NE(ptr, nullptr);
    EXPECT_DOUBLE_EQ(ptr[0], 9.5);
    EXPECT_DOUBLE_EQ(ptr[1], 19.5);
}

TEST_F(ArrowExportTest, GetInt64DataReturnsNullptrForStringColumn) {
    ArrowRecordBatch batch;
    batch.addColumn({"name", ArrowRecordBatch::DataType::STRING, false});
    batch.appendRow({std::string("x")});

    EXPECT_EQ(batch.getInt64Data(0), nullptr);
}

TEST_F(ArrowExportTest, GetDoubleDataReturnsNullptrForStringColumn) {
    ArrowRecordBatch batch;
    batch.addColumn({"name", ArrowRecordBatch::DataType::STRING, false});
    batch.appendRow({std::string("x")});

    EXPECT_EQ(batch.getDoubleData(0), nullptr);
}

TEST_F(ArrowExportTest, ZeroCopyNullRowStorePlaceholderZero) {
    // Null rows store 0 as placeholder in typed buffers;
    // the null_bitmap correctly marks them as null.
    ArrowRecordBatch batch;
    batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, true});
    batch.addColumn({"val", ArrowRecordBatch::DataType::DOUBLE, true});

    batch.appendRow({int64_t(42), 3.14});
    batch.appendRow({nullptr, nullptr});  // null row
    batch.appendRow({int64_t(99), 2.72});

    const auto& id_col  = batch.getColumn(0);
    const auto& val_col = batch.getColumn(1);

    ASSERT_EQ(id_col.int64_buffer.size(), 3u);
    EXPECT_EQ(id_col.int64_buffer[0], 42);
    EXPECT_EQ(id_col.int64_buffer[1], 0);   // placeholder for null
    EXPECT_EQ(id_col.int64_buffer[2], 99);

    ASSERT_EQ(val_col.double_buffer.size(), 3u);
    EXPECT_DOUBLE_EQ(val_col.double_buffer[0], 3.14);
    EXPECT_DOUBLE_EQ(val_col.double_buffer[1], 0.0);  // placeholder for null
    EXPECT_DOUBLE_EQ(val_col.double_buffer[2], 2.72);

    // null_bitmap should mark the middle row as null
    EXPECT_FALSE(id_col.null_bitmap[0]);
    EXPECT_TRUE(id_col.null_bitmap[1]);
    EXPECT_FALSE(id_col.null_bitmap[2]);
}

TEST_F(ArrowExportTest, ZeroCopyDataMatchesVariantData) {
    // The typed buffer and the variant data must be consistent.
    ArrowRecordBatch batch;
    batch.addColumn({"x", ArrowRecordBatch::DataType::INT64, false});
    batch.addColumn({"y", ArrowRecordBatch::DataType::DOUBLE, false});

    const size_t num_rows = 50;
    for (size_t i = 0; i < num_rows; ++i) {
        batch.appendRow({int64_t(i * 3), double(i) * 0.5});
    }

    const auto& x_col = batch.getColumn(0);
    const auto& y_col = batch.getColumn(1);

    for (size_t i = 0; i < num_rows; ++i) {
        EXPECT_EQ(x_col.int64_buffer[i], std::get<int64_t>(x_col.data[i]));
        EXPECT_DOUBLE_EQ(y_col.double_buffer[i], std::get<double>(y_col.data[i]));
    }
}


