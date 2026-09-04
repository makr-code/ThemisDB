#include <gtest/gtest.h>
#include "storage/storage_parquet_exporter.h"
#include "storage/columnar_format.h"
#include "utils/error_registry.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <vector>
#include <ctime>

using namespace themis::storage;
using namespace themis;

// ============================================================================
// Helpers
// ============================================================================

static ColumnSegment makeInt32Seg(const std::vector<int32_t>& v) {
    auto r = ColumnSegment::create(
        ColumnType::INT32, v.data(), v.size(), CompressionCodec::NONE);
    EXPECT_TRUE(r.has_value());
    return std::move(*r);
}

static ColumnSegment makeInt64Seg(const std::vector<int64_t>& v) {
    auto r = ColumnSegment::create(
        ColumnType::INT64, v.data(), v.size(), CompressionCodec::NONE);
    EXPECT_TRUE(r.has_value());
    return std::move(*r);
}

static ColumnSegment makeFloat32Seg(const std::vector<float>& v) {
    auto r = ColumnSegment::create(
        ColumnType::FLOAT32, v.data(), v.size(), CompressionCodec::NONE);
    EXPECT_TRUE(r.has_value());
    return std::move(*r);
}

static ColumnSegment makeFloat64Seg(const std::vector<double>& v) {
    auto r = ColumnSegment::create(
        ColumnType::FLOAT64, v.data(), v.size(), CompressionCodec::NONE);
    EXPECT_TRUE(r.has_value());
    return std::move(*r);
}

static ColumnSegment makeBoolSeg(const std::vector<uint8_t>& v) {
    auto r = ColumnSegment::create(
        ColumnType::BOOL, v.data(), v.size(), CompressionCodec::NONE);
    EXPECT_TRUE(r.has_value());
    return std::move(*r);
}

// Parquet magic bytes
static const uint8_t PAR1[4] = {'P', 'A', 'R', '1'};

static bool startWithPAR1(const std::vector<uint8_t>& buf) {
    return buf.size() >= 4 &&
           std::memcmp(buf.data(), PAR1, 4) == 0;
}

static bool endWithPAR1(const std::vector<uint8_t>& buf) {
    return buf.size() >= 4 &&
           std::memcmp(buf.data() + buf.size() - 4, PAR1, 4) == 0;
}

// ============================================================================
// Fixture
// ============================================================================

class StorageParquetExporterFocusedTests : public ::testing::Test {
protected:
    StorageParquetExporter exporter;
    std::string test_dir = {};

    void SetUp() override {
        auto tmp = std::filesystem::temp_directory_path();
        test_dir = (tmp / ("themis_pq_test_" +
                           std::to_string(std::time(nullptr)))).string();
        std::filesystem::create_directories(test_dir);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }
};

// ============================================================================
// PE-1: PAR1 magic
// ============================================================================

TEST_F(StorageParquetExporterFocusedTests, PE1_OutputHasPAR1Magic) {
    std::vector<int32_t> vals = {1, 2, 3};
    auto seg = makeInt32Seg(vals);

    ParquetExportConfig cfg;
    cfg.columns = {{"x", ColumnType::INT32}};

    auto result = exporter.exportToBuffer({{seg}}, cfg);
    ASSERT_TRUE(result.has_value()) << result.error().context();

    EXPECT_TRUE(startWithPAR1(*result)) << "Missing leading PAR1 magic";
    EXPECT_TRUE(endWithPAR1(*result))   << "Missing trailing PAR1 magic";
}

// ============================================================================
// PE-2..6: single column round-trip size checks
// ============================================================================

TEST_F(StorageParquetExporterFocusedTests, PE2_INT32_OutputNonEmpty) {
    std::vector<int32_t> vals(100);
    std::iota(vals.begin(), vals.end(), 0);
    auto seg = makeInt32Seg(vals);

    ParquetExportConfig cfg;
    cfg.columns = {{"col_i32", ColumnType::INT32}};

    auto result = exporter.exportToBuffer({{seg}}, cfg);
    ASSERT_TRUE(result.has_value());
    // Minimum: 4 (header) + data + 4 (LE meta len) + 4 (trailer) = > 12 bytes
    EXPECT_GT(result->size(), 12u);
}

TEST_F(StorageParquetExporterFocusedTests, PE3_INT64_OutputNonEmpty) {
    std::vector<int64_t> vals(50);
    std::iota(vals.begin(), vals.end(), int64_t(0));
    auto seg = makeInt64Seg(vals);

    ParquetExportConfig cfg;
    cfg.columns = {{"col_i64", ColumnType::INT64}};

    auto result = exporter.exportToBuffer({{seg}}, cfg);
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->size(), 12u);
}

TEST_F(StorageParquetExporterFocusedTests, PE4_FLOAT32_OutputNonEmpty) {
    std::vector<float> vals = {1.0f, 2.0f, 3.0f};
    auto seg = makeFloat32Seg(vals);

    ParquetExportConfig cfg;
    cfg.columns = {{"f32", ColumnType::FLOAT32}};

    auto result = exporter.exportToBuffer({{seg}}, cfg);
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->size(), 12u);
}

TEST_F(StorageParquetExporterFocusedTests, PE5_FLOAT64_OutputNonEmpty) {
    std::vector<double> vals = {1.0, 2.0, 3.0, 4.0};
    auto seg = makeFloat64Seg(vals);

    ParquetExportConfig cfg;
    cfg.columns = {{"f64", ColumnType::FLOAT64}};

    auto result = exporter.exportToBuffer({{seg}}, cfg);
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->size(), 12u);
}

TEST_F(StorageParquetExporterFocusedTests, PE6_BOOL_OutputNonEmpty) {
    std::vector<uint8_t> vals = {1, 0, 1, 1, 0};
    auto seg = makeBoolSeg(vals);

    ParquetExportConfig cfg;
    cfg.columns = {{"flag", ColumnType::BOOL}};

    auto result = exporter.exportToBuffer({{seg}}, cfg);
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->size(), 12u);
}

// ============================================================================
// PE-7: multi-column table
// ============================================================================

TEST_F(StorageParquetExporterFocusedTests, PE7_MultiColumn_INT32_FLOAT64) {
    std::vector<int32_t> ids = {1, 2, 3, 4, 5};
    std::vector<double>  prices = {9.99, 19.99, 4.99, 14.99, 29.99};

    auto seg_id    = makeInt32Seg(ids);
    auto seg_price = makeFloat64Seg(prices);

    ParquetExportConfig cfg;
    cfg.columns = {{"id", ColumnType::INT32}, {"price", ColumnType::FLOAT64}};

    auto result = exporter.exportToBuffer({{seg_id}, {seg_price}}, cfg);
    ASSERT_TRUE(result.has_value()) << result.error().context();
    EXPECT_TRUE(startWithPAR1(*result));
    EXPECT_TRUE(endWithPAR1(*result));
}

// ============================================================================
// PE-8..9: config validation errors
// ============================================================================

TEST_F(StorageParquetExporterFocusedTests, PE8_EmptyColumnsConfig) {
    ParquetExportConfig cfg;
    // cfg.columns is empty

    auto result = exporter.exportToBuffer({}, cfg);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(errors::ErrorCode::ERR_EXPORT_CONFIG_INVALID,
              result.error().code());
}

TEST_F(StorageParquetExporterFocusedTests, PE9_ColumnCountMismatch) {
    std::vector<int32_t> vals = {1, 2, 3};
    auto seg = makeInt32Seg(vals);

    ParquetExportConfig cfg;
    cfg.columns = {{"a", ColumnType::INT32}, {"b", ColumnType::INT64}};
    // Only one segment list but two columns in config
    auto result = exporter.exportToBuffer({{seg}}, cfg);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(errors::ErrorCode::ERR_EXPORT_CONFIG_INVALID,
              result.error().code());
}

// ============================================================================
// PE-10: single-row segment is handled correctly
// ============================================================================

TEST_F(StorageParquetExporterFocusedTests, PE10_SingleRowSegment) {
    // Cannot create a zero-row segment via ColumnSegment::create (requires data).
    // Verify that a single-row segment produces a valid Parquet file.
    std::vector<int32_t> one = {42};
    auto seg = makeInt32Seg(one);

    ParquetExportConfig cfg;
    cfg.columns = {{"v", ColumnType::INT32}};

    auto result = exporter.exportToBuffer({{seg}}, cfg);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(startWithPAR1(*result));
    const auto& s = exporter.lastStats();
    EXPECT_EQ(1u, s.rows_written);
}

// ============================================================================
// PE-11: stats after export
// ============================================================================

TEST_F(StorageParquetExporterFocusedTests, PE11_StatsPopulatedAfterExport) {
    std::vector<int32_t> vals = {10, 20, 30};
    auto seg = makeInt32Seg(vals);

    ParquetExportConfig cfg;
    cfg.columns = {{"x", ColumnType::INT32}};

    auto result = exporter.exportToBuffer({{seg}}, cfg);
    ASSERT_TRUE(result.has_value());

    const auto& s = exporter.lastStats();
    EXPECT_EQ(3u, s.rows_written);
    EXPECT_EQ(1u, s.columns_written);
    EXPECT_GT(s.bytes_written, 0u);
    EXPECT_GT(s.elapsed_us, 0.0);
}

// ============================================================================
// PE-12: exportToFile writes to disk
// ============================================================================

TEST_F(StorageParquetExporterFocusedTests, PE12_ExportToFile_NonEmpty) {
    std::vector<int32_t> vals = {1, 2, 3};
    auto seg = makeInt32Seg(vals);

    ParquetExportConfig cfg;
    cfg.columns = {{"v", ColumnType::INT32}};

    std::string path = test_dir + "/out.parquet";
    auto res = exporter.exportToFile({{seg}}, cfg, path);
    ASSERT_TRUE(res.has_value()) << res.error().context();

    EXPECT_TRUE(std::filesystem::exists(path));
    auto size = std::filesystem::file_size(path);
    EXPECT_GT(size, 12u);

    // Verify PAR1 magic in file
    std::ifstream f(path, std::ios::binary);
    ASSERT_TRUE(f.is_open());
    char magic[4] = {0};
    f.read(magic, 4);
    EXPECT_EQ(0, std::memcmp(magic, "PAR1", 4));
}

// ============================================================================
// PE-13: exportToFile bad path
// ============================================================================

TEST_F(StorageParquetExporterFocusedTests, PE13_ExportToFile_BadPath) {
    std::vector<int32_t> vals = {1};
    auto seg = makeInt32Seg(vals);

    ParquetExportConfig cfg;
    cfg.columns = {{"v", ColumnType::INT32}};

    // Non-existent directory
    std::string bad = "/nonexistent/dir/that/does/not/exist/out.parquet";
    auto res = exporter.exportToFile({{seg}}, cfg, bad);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(errors::ErrorCode::ERR_EXPORT_IO_ERROR, res.error().code());
}

// ============================================================================
// PE-14: metadata length field
// ============================================================================

TEST_F(StorageParquetExporterFocusedTests, PE14_MetadataLengthFieldPlausible) {
    std::vector<int32_t> vals = {1, 2, 3};
    auto seg = makeInt32Seg(vals);

    ParquetExportConfig cfg;
    cfg.columns = {{"v", ColumnType::INT32}};

    auto result = exporter.exportToBuffer({{seg}}, cfg);
    ASSERT_TRUE(result.has_value());

    const auto& buf = *result;
    // Layout: [4 PAR1][...data...][...meta...][4 LE meta_len][4 PAR1]
    // Read the meta_len field (bytes buf.size()-8 .. buf.size()-5)
    ASSERT_GE(buf.size(), 12u);
    uint32_t meta_len = 0;
    std::memcpy(&meta_len, buf.data() + buf.size() - 8, sizeof(uint32_t));

    // meta_len must be at least 1 and less than total file size
    EXPECT_GT(meta_len, 0u);
    EXPECT_LT(meta_len, static_cast<uint32_t>(buf.size()));
}

// ============================================================================
// PE-15: multiple segments per column (multi-row-group path)
// ============================================================================

TEST_F(StorageParquetExporterFocusedTests, PE15_MultipleSegmentsPerColumn) {
    std::vector<int32_t> a = {1, 2, 3};
    std::vector<int32_t> b = {4, 5, 6};
    auto segA = makeInt32Seg(a);
    auto segB = makeInt32Seg(b);

    ParquetExportConfig cfg;
    cfg.columns = {{"v", ColumnType::INT32}};

    auto result = exporter.exportToBuffer({{segA, segB}}, cfg);
    ASSERT_TRUE(result.has_value()) << result.error().context();
    EXPECT_TRUE(startWithPAR1(*result));
    EXPECT_TRUE(endWithPAR1(*result));

    const auto& s = exporter.lastStats();
    EXPECT_EQ(6u, s.rows_written);  // 3 + 3
}
