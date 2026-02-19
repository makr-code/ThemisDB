#include <gtest/gtest.h>
#include "exporters/jsonl_llm_exporter.h"
#include "exporters/exporter_errors.h"
#include "exporters/exporter_metrics.h"
#include "utils/error_registry.h"
#include "storage/base_entity.h"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <ctime>

using namespace themis::exporters;
using namespace themis;
using json = nlohmann::json;

class JSONLLLMExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test output directory with unique name
        auto temp_base = std::filesystem::temp_directory_path();
        test_dir_ = temp_base / ("themis_exporter_test_" + std::to_string(std::time(nullptr)));
        std::filesystem::create_directories(test_dir_);
        
        // Create test entities
        createTestEntities();
    }
    
    void TearDown() override {
        // Clean up test files
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }
    
    void createTestEntities() {
        // Create sample entities for testing
        for (int i = 0; i < 10; i++) {
            BaseEntity entity;
            entity.setPrimaryKey("entity_" + std::to_string(i));
            entity.setField("question", "What is the answer to question " + std::to_string(i) + "?");
            entity.setField("answer", "This is the answer to question " + std::to_string(i));
            entity.setField("context", "Context for question " + std::to_string(i));
            entity.setField("importance", static_cast<double>(i % 3 + 1) / 3.0);
            
            test_entities_.push_back(entity);
        }
        
        // Add a duplicate entity for duplicate detection testing
        BaseEntity duplicate = test_entities_[0];
        duplicate.setPrimaryKey("duplicate_entity");
        test_entities_.push_back(duplicate);
    }
    
    std::vector<std::string> readLinesFromFile(const std::string& path) {
        std::vector<std::string> lines;
        std::ifstream file(path);
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
        return lines;
    }
    
    std::string test_dir_;
    std::vector<BaseEntity> test_entities_;
};

// ===== Basic Export Tests =====

TEST_F(JSONLLLMExporterTest, BasicExportInstructionTuning) {
    JSONLLLMConfig config;
    config.style = JSONLFormat::Style::INSTRUCTION_TUNING;
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/basic_export.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    EXPECT_EQ(stats.total_entities, test_entities_.size());
    EXPECT_GT(stats.exported_entities, 0);
    EXPECT_EQ(stats.failed_entities, 0);
    EXPECT_GT(stats.bytes_written, 0);
    EXPECT_GT(stats.duration.count(), 0);
    
    // Verify file exists
    EXPECT_TRUE(std::filesystem::exists(options.output_path));
    
    // Verify lines were written
    auto lines = readLinesFromFile(options.output_path);
    EXPECT_GT(lines.size(), 0);
    
    // Verify each line is valid JSON
    for (const auto& line : lines) {
        EXPECT_NO_THROW({
            auto j = json::parse(line);
            EXPECT_TRUE(j.contains("instruction"));
            EXPECT_TRUE(j.contains("output"));
        });
    }
}

TEST_F(JSONLLLMExporterTest, ExportChatCompletionStyle) {
    JSONLLLMConfig config;
    config.style = JSONLFormat::Style::CHAT_COMPLETION;
    config.field_mapping.user_field = "question";
    config.field_mapping.assistant_field = "answer";
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/chat_export.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    EXPECT_GT(stats.exported_entities, 0);
    
    // Verify chat format
    auto lines = readLinesFromFile(options.output_path);
    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_TRUE(j.contains("messages"));
        EXPECT_TRUE(j["messages"].is_array());
    }
}

TEST_F(JSONLLLMExporterTest, ExportTextCompletionStyle) {
    JSONLLLMConfig config;
    config.style = JSONLFormat::Style::TEXT_COMPLETION;
    config.field_mapping.text_field = "answer";
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/text_export.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    EXPECT_GT(stats.exported_entities, 0);
    
    // Verify text format
    auto lines = readLinesFromFile(options.output_path);
    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_TRUE(j.contains("text"));
    }
}

// ===== Error Handling Tests =====

TEST_F(JSONLLLMExporterTest, IOErrorHandling) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = "/invalid/path/that/does/not/exist/export.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // Should have errors
    EXPECT_GT(stats.errors.size(), 0);
    
    // Verify error code is ERR_EXPORT_IO_ERROR
    auto error_code_str = std::to_string(static_cast<int>(themis::errors::ErrorCode::ERR_EXPORT_IO_ERROR));
    EXPECT_TRUE(stats.errors[0].find(error_code_str) != std::string::npos);
}

TEST_F(JSONLLLMExporterTest, ContinueOnError) {
    JSONLLLMConfig config;
    config.quality.min_text_length = 1000; // Make most entities fail
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/continue_on_error.jsonl";
    options.continue_on_error = true;
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // Should have processed all entities despite failures
    EXPECT_EQ(stats.total_entities, test_entities_.size());
}

TEST_F(JSONLLLMExporterTest, MaxErrorsLimit) {
    JSONLLLMConfig config;
    config.quality.min_text_length = 10000; // Make all entities fail
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/max_errors.jsonl";
    options.continue_on_error = true;
    options.max_errors = 5;
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // Should stop at max errors
    EXPECT_LE(stats.errors.size(), options.max_errors);
}

// ===== Metrics Tests =====

TEST_F(JSONLLLMExporterTest, MetricsCollection) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/metrics_test.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // Metrics should be attached to stats
    ASSERT_NE(stats.metrics, nullptr);
    
    // Verify metrics were collected
    auto metrics = exporter.getMetrics();
    EXPECT_GT(metrics->getExportRate(), 0.0);
    EXPECT_GT(metrics->getThroughput(), 0.0);
    EXPECT_GE(metrics->getAverageLatency(), 0.0);
}

TEST_F(JSONLLLMExporterTest, MetricsLatencyPercentiles) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/latency_test.jsonl";
    
    exporter.exportEntities(test_entities_, options);
    
    auto metrics = exporter.getMetrics();
    
    // Latency percentiles should be calculated
    double p50 = metrics->getP50Latency();
    double p95 = metrics->getP95Latency();
    double p99 = metrics->getP99Latency();
    
    EXPECT_GE(p50, 0.0);
    EXPECT_GE(p95, p50);  // P95 should be >= P50
    EXPECT_GE(p99, p95);  // P99 should be >= P95
}

TEST_F(JSONLLLMExporterTest, MetricsToJson) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/metrics_json.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // Verify metrics JSON can be generated
    auto metrics_json = stats.metrics->toJson();
    
    EXPECT_TRUE(metrics_json.contains("total_exports"));
    EXPECT_TRUE(metrics_json.contains("total_entities"));
    EXPECT_TRUE(metrics_json.contains("export_rate_per_sec"));
    EXPECT_TRUE(metrics_json.contains("p50_latency_ms"));
    EXPECT_TRUE(metrics_json.contains("p95_latency_ms"));
    EXPECT_TRUE(metrics_json.contains("p99_latency_ms"));
}

TEST_F(JSONLLLMExporterTest, MetricsErrorTracking) {
    JSONLLLMConfig config;
    config.quality.min_text_length = 10000; // Force failures
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/error_tracking.jsonl";
    options.continue_on_error = true;
    
    exporter.exportEntities(test_entities_, options);
    
    auto metrics = exporter.getMetrics();
    
    // Should have recorded errors
    EXPECT_GT(metrics->getTotalErrors(), 0);
    
    auto errors_by_type = metrics->getErrorsByType();
    EXPECT_GT(errors_by_type.size(), 0);
}

// ===== Quality Filtering Tests =====

TEST_F(JSONLLLMExporterTest, QualityFilterMinLength) {
    JSONLLLMConfig config;
    config.quality.min_text_length = 100;
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/quality_min_length.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // Some entities should be filtered out
    EXPECT_LT(stats.exported_entities, stats.total_entities);
    
    // Metrics should track rejections
    auto metrics = exporter.getMetrics();
    auto rejections = metrics->getQualityFilterRejections();
    EXPECT_GT(rejections.size(), 0);
}

TEST_F(JSONLLLMExporterTest, QualityFilterMaxLength) {
    JSONLLLMConfig config;
    config.quality.max_text_length = 10; // Very short
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/quality_max_length.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // Most entities should be filtered out
    EXPECT_LT(stats.exported_entities, stats.total_entities);
}

// ===== Duplicate Detection Tests =====

TEST_F(JSONLLLMExporterTest, DuplicateDetection) {
    JSONLLLMConfig config;
    config.quality.skip_duplicates = true;
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/duplicate_test.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // We added a duplicate entity, so exported should be less than total
    EXPECT_LT(stats.exported_entities, stats.total_entities);
    
    // Metrics should track duplicates
    auto metrics = exporter.getMetrics();
    EXPECT_GT(metrics->getTotalDuplicates(), 0);
}

TEST_F(JSONLLLMExporterTest, NoDuplicateDetection) {
    JSONLLLMConfig config;
    config.quality.skip_duplicates = false;
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/no_duplicate_check.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // All entities should be exported (including duplicate)
    EXPECT_EQ(stats.exported_entities, stats.total_entities);
}

// ===== Schema Validation Tests =====

TEST_F(JSONLLLMExporterTest, SchemaValidationDisabled) {
    JSONLLLMConfig config;
    config.structured_gen.enable_schema_validation = false;
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/no_schema_validation.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // All entities should be exported
    EXPECT_GT(stats.exported_entities, 0);
    
    // No schema validation stats
    auto metrics = exporter.getMetrics();
    auto schema_stats = metrics->getSchemaValidationStats();
    EXPECT_EQ(schema_stats.total_validated, 0);
}

// ===== Weighting Tests =====

TEST_F(JSONLLLMExporterTest, WeightingEnabled) {
    JSONLLLMConfig config;
    config.weighting.enable_weights = true;
    config.weighting.weight_field = "importance";
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/weighted_export.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    EXPECT_GT(stats.exported_entities, 0);
    
    // Verify weights in output
    auto lines = readLinesFromFile(options.output_path);
    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_TRUE(j.contains("weight"));
    }
}

TEST_F(JSONLLLMExporterTest, WeightingDisabled) {
    JSONLLLMConfig config;
    config.weighting.enable_weights = false;
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/no_weights.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    EXPECT_GT(stats.exported_entities, 0);
    
    // Weights should not be in output
    auto lines = readLinesFromFile(options.output_path);
    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_FALSE(j.contains("weight"));
    }
}

// ===== Progress Callback Tests =====

TEST_F(JSONLLLMExporterTest, ProgressCallback) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    size_t callback_count = 0;
    
    ExportOptions options;
    options.output_path = test_dir_ + "/progress_test.jsonl";
    options.progress_interval = 3;
    options.progress_callback = [&callback_count](const ExportStats& stats) {
        callback_count++;
    };
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // Callback should have been called
    EXPECT_GT(callback_count, 0);
}

// ===== ExportStats JSON Tests =====

TEST_F(JSONLLLMExporterTest, ExportStatsToJson) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/stats_json.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // Verify JSON generation
    std::string json_str = stats.toJson();
    EXPECT_GT(json_str.size(), 0);
    
    // Parse and verify
    auto j = json::parse(json_str);
    EXPECT_TRUE(j.contains("total_entities"));
    EXPECT_TRUE(j.contains("exported_entities"));
    EXPECT_TRUE(j.contains("bytes_written"));
    EXPECT_TRUE(j.contains("duration_ms"));
    EXPECT_TRUE(j.contains("metrics"));
}

// ===== Metadata Tests =====

TEST_F(JSONLLLMExporterTest, MetadataInclusion) {
    JSONLLLMConfig config;
    config.include_metadata = true;
    config.metadata_fields = {"source", "category"};
    
    // Add metadata to entities
    for (auto& entity : test_entities_) {
        entity.setField("source", "test_source");
        entity.setField("category", "test_category");
    }
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/metadata_test.jsonl";
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    EXPECT_GT(stats.exported_entities, 0);
    
    // Verify metadata in output
    auto lines = readLinesFromFile(options.output_path);
    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_TRUE(j.contains("metadata"));
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
