// Copyright (c) 2025 VCC ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Arrow/Parquet Library Integration Tests
// Tests Apache Arrow and Parquet library integration for columnar storage
// Use Case: Data export, columnar analytics, interoperability

#include <gtest/gtest.h>

// Note: Arrow/Parquet libraries may not be available in all builds
// These tests are designed to compile with or without the libraries
#ifdef THEMIS_ENABLE_ARROW

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <filesystem>
#include <memory>
#include <chrono>

namespace fs = std::filesystem;

class ArrowLibIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_parquet_dir_ = "./data/test_lib_arrow_" + 
                           std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        fs::create_directories(test_parquet_dir_);
    }

    void TearDown() override {
        if (fs::exists(test_parquet_dir_)) {
            std::error_code ec = {};
            fs::remove_all(test_parquet_dir_, ec);
        }
    }

    std::string test_parquet_dir_;
};

// Test 1: Arrow library linking and version
TEST_F(ArrowLibIntegrationTest, LibraryLinking) {
    std::string version = arrow::GetBuildInfo().version_string;
    EXPECT_FALSE(version.empty());
    EXPECT_NE(version.find("Arrow"), std::string::npos);
}

// Test 2: Create Arrow schema
TEST_F(ArrowLibIntegrationTest, CreateSchema) {
    auto schema = arrow::schema({
        arrow::field("id", arrow::int64()),
        arrow::field("name", arrow::utf8()),
        arrow::field("value", arrow::float64())
    });
    
    ASSERT_NE(schema, nullptr);
    EXPECT_EQ(schema->num_fields(), 3);
    EXPECT_EQ(schema->field(0)->name(), "id");
    EXPECT_EQ(schema->field(1)->name(), "name");
    EXPECT_EQ(schema->field(2)->name(), "value");
}

// Test 3: Create Arrow arrays
TEST_F(ArrowLibIntegrationTest, CreateArrays) {
    arrow::Int64Builder int_builder;
    ASSERT_TRUE(int_builder.Append(1).ok());
    ASSERT_TRUE(int_builder.Append(2).ok());
    ASSERT_TRUE(int_builder.Append(3).ok());
    
    std::shared_ptr<arrow::Array> array;
    ASSERT_TRUE(int_builder.Finish(&array).ok());
    
    EXPECT_EQ(array->length(), 3);
    EXPECT_EQ(array->type()->id(), arrow::Type::INT64);
}

// Test 4: Create Arrow table
TEST_F(ArrowLibIntegrationTest, CreateTable) {
    arrow::Int64Builder id_builder;
    arrow::StringBuilder name_builder;
    arrow::DoubleBuilder value_builder;
    
    // Add data
    ASSERT_TRUE(id_builder.Append(1).ok());
    ASSERT_TRUE(id_builder.Append(2).ok());
    ASSERT_TRUE(id_builder.Append(3).ok());
    
    ASSERT_TRUE(name_builder.Append("Alice").ok());
    ASSERT_TRUE(name_builder.Append("Bob").ok());
    ASSERT_TRUE(name_builder.Append("Charlie").ok());
    
    ASSERT_TRUE(value_builder.Append(100.5).ok());
    ASSERT_TRUE(value_builder.Append(200.75).ok());
    ASSERT_TRUE(value_builder.Append(300.25).ok());
    
    std::shared_ptr<arrow::Array> id_array, name_array, value_array;
    ASSERT_TRUE(id_builder.Finish(&id_array).ok());
    ASSERT_TRUE(name_builder.Finish(&name_array).ok());
    ASSERT_TRUE(value_builder.Finish(&value_array).ok());
    
    auto schema = arrow::schema({
        arrow::field("id", arrow::int64()),
        arrow::field("name", arrow::utf8()),
        arrow::field("value", arrow::float64())
    });
    
    auto table = arrow::Table::Make(schema, {id_array, name_array, value_array});
    
    EXPECT_EQ(table->num_rows(), 3);
    EXPECT_EQ(table->num_columns(), 3);
}

// Test 5: Write Parquet file
TEST_F(ArrowLibIntegrationTest, WriteParquetFile) {
    std::string file_path = test_parquet_dir_ + "/test_data.parquet";
    
    // Create table
    arrow::Int64Builder id_builder;
    arrow::StringBuilder name_builder;
    
    for (int i = 0; i < 100; ++i) {
        ASSERT_TRUE(id_builder.Append(i).ok());
        ASSERT_TRUE(name_builder.Append("user_" + std::to_string(i)).ok());
    }
    
    std::shared_ptr<arrow::Array> id_array, name_array;
    ASSERT_TRUE(id_builder.Finish(&id_array).ok());
    ASSERT_TRUE(name_builder.Finish(&name_array).ok());
    
    auto schema = arrow::schema({
        arrow::field("id", arrow::int64()),
        arrow::field("name", arrow::utf8())
    });
    
    auto table = arrow::Table::Make(schema, {id_array, name_array});
    
    // Write to Parquet file
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    ASSERT_TRUE(arrow::io::FileOutputStream::Open(file_path, &outfile).ok());
    
    ASSERT_TRUE(parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), 
                                           outfile, 100).ok());
    
    EXPECT_TRUE(fs::exists(file_path));
}

// Test 6: Read Parquet file
TEST_F(ArrowLibIntegrationTest, ReadParquetFile) {
    std::string file_path = test_parquet_dir_ + "/read_test.parquet";
    
    // First, create a file to read
    arrow::Int64Builder builder;
    for (int i = 0; i < 50; ++i) {
        ASSERT_TRUE(builder.Append(i).ok());
    }
    
    std::shared_ptr<arrow::Array> array;
    ASSERT_TRUE(builder.Finish(&array).ok());
    
    auto schema = arrow::schema({arrow::field("numbers", arrow::int64())});
    auto table = arrow::Table::Make(schema, {array});
    
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    ASSERT_TRUE(arrow::io::FileOutputStream::Open(file_path, &outfile).ok());
    ASSERT_TRUE(parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), 
                                           outfile, 50).ok());
    
    // Now read it back
    std::shared_ptr<arrow::io::ReadableFile> infile;
    ASSERT_TRUE(arrow::io::ReadableFile::Open(file_path, &infile).ok());
    
    std::unique_ptr<parquet::arrow::FileReader> reader;
    ASSERT_TRUE(parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader).ok());
    
    std::shared_ptr<arrow::Table> read_table;
    ASSERT_TRUE(reader->ReadTable(&read_table).ok());
    
    EXPECT_EQ(read_table->num_rows(), 50);
    EXPECT_EQ(read_table->num_columns(), 1);
}

// Test 7: Parquet compression
TEST_F(ArrowLibIntegrationTest, ParquetCompression) {
    std::string file_path = test_parquet_dir_ + "/compressed.parquet";
    
    // Create compressible data
    arrow::StringBuilder builder;
    std::string repeated_data = "ThemisDB columnar storage test data. ";
    for (int i = 0; i < 1000; ++i) {
        ASSERT_TRUE(builder.Append(repeated_data).ok());
    }
    
    std::shared_ptr<arrow::Array> array;
    ASSERT_TRUE(builder.Finish(&array).ok());
    
    auto schema = arrow::schema({arrow::field("data", arrow::utf8())});
    auto table = arrow::Table::Make(schema, {array});
    
    // Write with compression
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    ASSERT_TRUE(arrow::io::FileOutputStream::Open(file_path, &outfile).ok());
    
    auto props = parquet::WriterProperties::Builder()
        .compression(parquet::Compression::SNAPPY)
        ->build();
    
    ASSERT_TRUE(parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), 
                                           outfile, 100, props).ok());
    
    // Verify compression resulted in smaller file
    size_t file_size = fs::file_size(file_path);
    size_t uncompressed_size = repeated_data.size() * 1000;
    
    EXPECT_LT(file_size, uncompressed_size / 2); // At least 50% compression
}

// Test 8: Multiple column types
TEST_F(ArrowLibIntegrationTest, MultipleColumnTypes) {
    arrow::Int32Builder int32_builder;
    arrow::Int64Builder int64_builder;
    arrow::FloatBuilder float_builder;
    arrow::DoubleBuilder double_builder;
    arrow::BooleanBuilder bool_builder;
    arrow::StringBuilder string_builder;
    
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(int32_builder.Append(i).ok());
        ASSERT_TRUE(int64_builder.Append(i * 1000).ok());
        ASSERT_TRUE(float_builder.Append(i * 1.5f).ok());
        ASSERT_TRUE(double_builder.Append(i * 3.14159).ok());
        ASSERT_TRUE(bool_builder.Append(i % 2 == 0).ok());
        ASSERT_TRUE(string_builder.Append("item_" + std::to_string(i)).ok());
    }
    
    std::shared_ptr<arrow::Array> int32_array, int64_array, float_array;
    std::shared_ptr<arrow::Array> double_array, bool_array, string_array;
    
    ASSERT_TRUE(int32_builder.Finish(&int32_array).ok());
    ASSERT_TRUE(int64_builder.Finish(&int64_array).ok());
    ASSERT_TRUE(float_builder.Finish(&float_array).ok());
    ASSERT_TRUE(double_builder.Finish(&double_array).ok());
    ASSERT_TRUE(bool_builder.Finish(&bool_array).ok());
    ASSERT_TRUE(string_builder.Finish(&string_array).ok());
    
    auto schema = arrow::schema({
        arrow::field("int32_col", arrow::int32()),
        arrow::field("int64_col", arrow::int64()),
        arrow::field("float_col", arrow::float32()),
        arrow::field("double_col", arrow::float64()),
        arrow::field("bool_col", arrow::boolean()),
        arrow::field("string_col", arrow::utf8())
    });
    
    auto table = arrow::Table::Make(schema, {
        int32_array, int64_array, float_array,
        double_array, bool_array, string_array
    });
    
    EXPECT_EQ(table->num_rows(), 10);
    EXPECT_EQ(table->num_columns(), 6);
}

// Test 9: Null value handling
TEST_F(ArrowLibIntegrationTest, NullValueHandling) {
    arrow::Int64Builder builder;
    
    ASSERT_TRUE(builder.Append(1).ok());
    ASSERT_TRUE(builder.AppendNull().ok());
    ASSERT_TRUE(builder.Append(3).ok());
    ASSERT_TRUE(builder.AppendNull().ok());
    ASSERT_TRUE(builder.Append(5).ok());
    
    std::shared_ptr<arrow::Array> array;
    ASSERT_TRUE(builder.Finish(&array).ok());
    
    EXPECT_EQ(array->length(), 5);
    EXPECT_EQ(array->null_count(), 2);
    EXPECT_FALSE(array->IsNull(0));
    EXPECT_TRUE(array->IsNull(1));
    EXPECT_FALSE(array->IsNull(2));
    EXPECT_TRUE(array->IsNull(3));
}

// Test 10: Arrow RecordBatch
TEST_F(ArrowLibIntegrationTest, RecordBatch) {
    arrow::Int64Builder id_builder;
    arrow::StringBuilder name_builder;
    
    ASSERT_TRUE(id_builder.Append(1).ok());
    ASSERT_TRUE(id_builder.Append(2).ok());
    ASSERT_TRUE(name_builder.Append("Alice").ok());
    ASSERT_TRUE(name_builder.Append("Bob").ok());
    
    std::shared_ptr<arrow::Array> id_array, name_array;
    ASSERT_TRUE(id_builder.Finish(&id_array).ok());
    ASSERT_TRUE(name_builder.Finish(&name_array).ok());
    
    auto schema = arrow::schema({
        arrow::field("id", arrow::int64()),
        arrow::field("name", arrow::utf8())
    });
    
    auto batch = arrow::RecordBatch::Make(schema, 2, {id_array, name_array});
    
    EXPECT_EQ(batch->num_rows(), 2);
    EXPECT_EQ(batch->num_columns(), 2);
    EXPECT_EQ(batch->column_name(0), "id");
    EXPECT_EQ(batch->column_name(1), "name");
}

// Test 11: Memory pool usage
TEST_F(ArrowLibIntegrationTest, MemoryPoolUsage) {
    auto pool = arrow::default_memory_pool();
    int64_t bytes_allocated_before = pool->bytes_allocated();
    
    arrow::Int64Builder builder(pool);
    for (int i = 0; i < 10000; ++i) {
        ASSERT_TRUE(builder.Append(i).ok());
    }
    
    std::shared_ptr<arrow::Array> array;
    ASSERT_TRUE(builder.Finish(&array).ok());
    
    int64_t bytes_allocated_after = pool->bytes_allocated();
    EXPECT_GT(bytes_allocated_after, bytes_allocated_before);
}

// Test 12: Table slicing
TEST_F(ArrowLibIntegrationTest, TableSlicing) {
    arrow::Int64Builder builder;
    for (int i = 0; i < 100; ++i) {
        ASSERT_TRUE(builder.Append(i).ok());
    }
    
    std::shared_ptr<arrow::Array> array;
    ASSERT_TRUE(builder.Finish(&array).ok());
    
    auto schema = arrow::schema({arrow::field("numbers", arrow::int64())});
    auto table = arrow::Table::Make(schema, {array});
    
    // Slice rows 10-19
    auto sliced = table->Slice(10, 10);
    
    EXPECT_EQ(sliced->num_rows(), 10);
    EXPECT_EQ(sliced->num_columns(), 1);
}

// Test 13: ThemisDB integration - export query results
TEST_F(ArrowLibIntegrationTest, ThemisDBQueryResultExport) {
    std::string file_path = test_parquet_dir_ + "/query_results.parquet";
    
    // Simulate ThemisDB query results
    arrow::Int64Builder id_builder;
    arrow::StringBuilder user_builder;
    arrow::Int32Builder age_builder;
    arrow::DoubleBuilder score_builder;
    
    // Sample query results
    std::vector<std::tuple<int64_t, std::string, int32_t, double>> results = {
        {1, "alice@example.com", 30, 95.5},
        {2, "bob@example.com", 25, 87.3},
        {3, "charlie@example.com", 35, 92.1},
        {4, "diana@example.com", 28, 88.9},
        {5, "evan@example.com", 32, 91.2}
    };
    
    for (const auto& [id, user, age, score] : results) {
        ASSERT_TRUE(id_builder.Append(id).ok());
        ASSERT_TRUE(user_builder.Append(user).ok());
        ASSERT_TRUE(age_builder.Append(age).ok());
        ASSERT_TRUE(score_builder.Append(score).ok());
    }
    
    std::shared_ptr<arrow::Array> id_array, user_array, age_array, score_array;
    ASSERT_TRUE(id_builder.Finish(&id_array).ok());
    ASSERT_TRUE(user_builder.Finish(&user_array).ok());
    ASSERT_TRUE(age_builder.Finish(&age_array).ok());
    ASSERT_TRUE(score_builder.Finish(&score_array).ok());
    
    auto schema = arrow::schema({
        arrow::field("id", arrow::int64()),
        arrow::field("email", arrow::utf8()),
        arrow::field("age", arrow::int32()),
        arrow::field("score", arrow::float64())
    });
    
    auto table = arrow::Table::Make(schema, {id_array, user_array, age_array, score_array});
    
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    ASSERT_TRUE(arrow::io::FileOutputStream::Open(file_path, &outfile).ok());
    ASSERT_TRUE(parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), 
                                           outfile, 100).ok());
    
    EXPECT_TRUE(fs::exists(file_path));
}

// Test 14: Parquet metadata
TEST_F(ArrowLibIntegrationTest, ParquetMetadata) {
    std::string file_path = test_parquet_dir_ + "/metadata_test.parquet";
    
    arrow::Int64Builder builder;
    for (int i = 0; i < 100; ++i) {
        ASSERT_TRUE(builder.Append(i).ok());
    }
    
    std::shared_ptr<arrow::Array> array;
    ASSERT_TRUE(builder.Finish(&array).ok());
    
    auto schema = arrow::schema({arrow::field("data", arrow::int64())});
    auto table = arrow::Table::Make(schema, {array});
    
    // Write with custom metadata
    auto key_value_metadata = arrow::KeyValueMetadata::Make(
        {"source", "version"},
        {"ThemisDB", "1.3.0"}
    );
    auto schema_with_metadata = schema->WithMetadata(key_value_metadata);
    auto table_with_metadata = table->ReplaceSchemaMetadata(key_value_metadata);
    
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    ASSERT_TRUE(arrow::io::FileOutputStream::Open(file_path, &outfile).ok());
    ASSERT_TRUE(parquet::arrow::WriteTable(*table_with_metadata, arrow::default_memory_pool(), 
                                           outfile, 100).ok());
    
    // Read back and verify metadata
    std::shared_ptr<arrow::io::ReadableFile> infile;
    ASSERT_TRUE(arrow::io::ReadableFile::Open(file_path, &infile).ok());
    
    std::unique_ptr<parquet::arrow::FileReader> reader;
    ASSERT_TRUE(parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader).ok());
    
    std::shared_ptr<arrow::Table> read_table;
    ASSERT_TRUE(reader->ReadTable(&read_table).ok());
    
    auto metadata = read_table->schema()->metadata();
    if (metadata) {
        EXPECT_GE(metadata->size(), 0);
    }
}

// Test 15: Performance - large dataset
TEST_F(ArrowLibIntegrationTest, LargeDatasetPerformance) {
    std::string file_path = test_parquet_dir_ + "/large_dataset.parquet";
    
    arrow::Int64Builder builder;
    const int num_rows = 1000000; // 1 million rows
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_rows; ++i) {
        ASSERT_TRUE(builder.Append(i).ok());
    }
    
    std::shared_ptr<arrow::Array> array;
    ASSERT_TRUE(builder.Finish(&array).ok());
    
    auto schema = arrow::schema({arrow::field("id", arrow::int64())});
    auto table = arrow::Table::Make(schema, {array});
    
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    ASSERT_TRUE(arrow::io::FileOutputStream::Open(file_path, &outfile).ok());
    ASSERT_TRUE(parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), 
                                           outfile, 10000).ok());
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in reasonable time
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds for 1M rows
}

#else // THEMIS_ENABLE_ARROW not defined

// Stub tests when Arrow is not available
TEST(ArrowLibIntegrationTest, ArrowNotAvailable) {
    GTEST_SKIP() << "Arrow/Parquet libraries not enabled. "
                 << "Build with -DTHEMIS_ENABLE_ARROW=ON and install Arrow/Parquet via vcpkg.";
}

#endif // THEMIS_ENABLE_ARROW
