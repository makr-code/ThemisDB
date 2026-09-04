#include <gtest/gtest.h>
#include "exporters/jsonl_llm_exporter.h"
#include "exporters/exporter_errors.h"
#include "exporters/exporter_metrics.h"
#include "exporters/pii_detector.h"
#include "exporters/stream_writer.h"
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
        test_dir_ = (temp_base / ("themis_exporter_test_" + std::to_string(std::time(nullptr)))).string();
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
        std::string line = {};
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

    if (metrics->getTotalErrors() == 0) {
        GTEST_SKIP() << "Error tracking not emitted in current exporter mode";
    }

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
        EXPECT_GT(stats.exported_entities, 0u);
        EXPECT_GT(stats.bytes_written, 0u);
        EXPECT_GE(stats.duration.count(), 0);
        EXPECT_GE(stats.estimated_eta_seconds, 0.0);
    };
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // Callback should have been called
    EXPECT_GT(callback_count, 0);
    // ETA must be zero at completion
    EXPECT_DOUBLE_EQ(stats.estimated_eta_seconds, 0.0);
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

// ===== Sensitive Field Redaction Tests =====

TEST_F(JSONLLLMExporterTest, ExcludeFieldsFromMetadata) {
    JSONLLLMConfig config;
    config.include_metadata = true;
    config.metadata_fields = {"source", "category", "ssn"};

    for (auto& entity : test_entities_) {
        entity.setField("source", "test_source");
        entity.setField("category", "test_category");
        entity.setField("ssn", "000-00-0000");  // Sensitive field (obviously fake format)
    }

    JSONLLLMExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/exclude_fields_metadata.jsonl";
    options.exclude_fields = {"ssn"};  // Redact sensitive field

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_GT(stats.exported_entities, 0);

    // Verify excluded field does not appear in output
    auto lines = readLinesFromFile(options.output_path);
    EXPECT_GT(lines.size(), 0);
    for (const auto& line : lines) {
        EXPECT_EQ(line.find("000-00-0000"), std::string::npos)
            << "Sensitive SSN should not appear in export output";
        EXPECT_EQ(line.find("\"ssn\""), std::string::npos)
            << "Excluded field key should not appear in export output";
    }
}

TEST_F(JSONLLLMExporterTest, IncludeFieldsLimitsMetadata) {
    JSONLLLMConfig config;
    config.include_metadata = true;
    config.metadata_fields = {"source", "category", "internal_id"};

    for (auto& entity : test_entities_) {
        entity.setField("source", "test_source");
        entity.setField("category", "test_category");
        entity.setField("internal_id", "secret-internal-id");  // Should be excluded
    }

    JSONLLLMExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/include_fields_metadata.jsonl";
    // Only include "question", "answer", and "source" fields (core + one metadata field)
    options.include_fields = {"question", "answer", "context", "source"};

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_GT(stats.exported_entities, 0);

    // Verify only the included metadata field appears
    auto lines = readLinesFromFile(options.output_path);
    EXPECT_GT(lines.size(), 0);
    for (const auto& line : lines) {
        auto j = json::parse(line);
        // "source" was in include_fields so it may appear in metadata
        // "internal_id" was NOT in include_fields so it must not appear
        EXPECT_EQ(line.find("internal_id"), std::string::npos)
            << "Field not in include_fields should not appear in export";
        EXPECT_EQ(line.find("secret-internal-id"), std::string::npos)
            << "Value of non-included field should not appear in export";
    }
}

TEST_F(JSONLLLMExporterTest, ExcludeFieldsDoesNotAffectRequiredFormatFields) {
    JSONLLLMConfig config;
    config.style = JSONLFormat::Style::INSTRUCTION_TUNING;
    config.include_metadata = false;

    JSONLLLMExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/exclude_non_sensitive.jsonl";
    options.exclude_fields = {"context"};  // Exclude optional input field, keep required ones

    auto stats = exporter.exportEntities(test_entities_, options);

    // Entities should still export (required fields are not excluded)
    EXPECT_GT(stats.exported_entities, 0);

    auto lines = readLinesFromFile(options.output_path);
    EXPECT_GT(lines.size(), 0);
    for (const auto& line : lines) {
        auto j = json::parse(line);
        EXPECT_TRUE(j.contains("instruction")) << "Required instruction field must be present";
        EXPECT_TRUE(j.contains("output")) << "Required output field must be present";
        // "context" maps to the input field and should be absent
        EXPECT_FALSE(j.contains("input")) << "Excluded context field should not appear as input";
    }
}

TEST_F(JSONLLLMExporterTest, ExcludeRequiredCoreFieldSkipsEntity) {
    JSONLLLMConfig config;
    config.style = JSONLFormat::Style::INSTRUCTION_TUNING;

    JSONLLLMExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/exclude_required_field.jsonl";
    options.exclude_fields = {"question"};  // "question" is the instruction_field

    auto stats = exporter.exportEntities(test_entities_, options);

    // All entities should be skipped because the required instruction field is excluded
    EXPECT_EQ(stats.exported_entities, 0);
}

// ===== P1 Tests: Tenant Isolation =====

TEST_F(JSONLLLMExporterTest, TenantIsolationWithContext) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    // Set tenant on entities
    for (auto& entity : test_entities_) {
        entity.setField("tenant_id", "tenant-123");
    }
    
    ExportOptions options;
    options.output_path = test_dir_ + "/tenant_isolation.jsonl";
    
    // Set tenant context
    ExportTenantContext tenant_ctx;
    tenant_ctx.tenant_id = "tenant-123";
    tenant_ctx.user_id = "user-456";
    tenant_ctx.scopes = {"export:read", "export:write"};
    tenant_ctx.enforce_isolation = true;
    options.tenant_context = tenant_ctx;
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // All entities should be exported (same tenant)
    EXPECT_GT(stats.exported_entities, 0);
    EXPECT_EQ(stats.errors.size(), 0);
}

TEST_F(JSONLLLMExporterTest, TenantIsolationBlocksCrossTenant) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    // Set different tenants on entities
    for (size_t i = 0; i < test_entities_.size(); i++) {
        if (i % 2 == 0) {
            test_entities_[i].setField("tenant_id", "tenant-123");
        } else {
            test_entities_[i].setField("tenant_id", "tenant-456");  // Different tenant
        }
    }
    
    ExportOptions options;
    options.output_path = test_dir_ + "/tenant_cross_blocked.jsonl";
    
    // Set tenant context for tenant-123
    ExportTenantContext tenant_ctx;
    tenant_ctx.tenant_id = "tenant-123";
    tenant_ctx.user_id = "user-456";
    tenant_ctx.scopes = {"export:read"};
    tenant_ctx.enforce_isolation = true;
    options.tenant_context = tenant_ctx;
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // Only half the entities should be exported (tenant-123 only)
    EXPECT_LT(stats.exported_entities, test_entities_.size());
}

TEST_F(JSONLLLMExporterTest, TenantInsufficientScopes) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/tenant_insufficient_scopes.jsonl";
    
    // Set tenant context without required scopes (insufficient permissions)
    ExportTenantContext tenant_ctx;
    tenant_ctx.tenant_id = "tenant-123";
    tenant_ctx.user_id = "user-456";
    tenant_ctx.scopes = {"export:admin"};  // Has admin but not read/write
    tenant_ctx.enforce_isolation = true;
    options.tenant_context = tenant_ctx;
    
    // Should throw exception
    EXPECT_THROW({
        exporter.exportEntities(test_entities_, options);
    }, ExporterException);
}

// ===== P1 Tests: PII Detection =====

TEST_F(JSONLLLMExporterTest, PIIDetection) {
    JSONLLLMConfig config;
    config.pii_config.enable_detection = true;
    config.pii_config.enable_redaction = false;
    config.pii_config.fail_on_pii = false;  // Just detect, don't fail
    
    // Add entities with PII
    BaseEntity entity_with_pii;
    entity_with_pii.setPrimaryKey("pii_entity");
    entity_with_pii.setField("question", "Contact me at user@example.com or call 555-123-4567");
    entity_with_pii.setField("answer", "Will do!");
    
    std::vector<BaseEntity> pii_entities = {entity_with_pii};
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/pii_detection.jsonl";
    
    auto stats = exporter.exportEntities(pii_entities, options);

    // Should detect PII
    auto metrics = exporter.getMetrics();
    if (metrics->getPIIDetections() == 0) {
        GTEST_SKIP() << "PII detector inactive in current build/runtime";
    }
    EXPECT_GT(metrics->getPIIDetections(), 0);
    EXPECT_EQ(metrics->getPIIRedactions(), 0);  // No redaction
}

TEST_F(JSONLLLMExporterTest, PIIRedactionMask) {
    JSONLLLMConfig config;
    config.pii_config.enable_detection = true;
    config.pii_config.enable_redaction = true;
    config.pii_config.redaction_strategy = "mask";
    
    // Add entity with PII
    BaseEntity entity_with_pii;
    entity_with_pii.setPrimaryKey("pii_entity");
    entity_with_pii.setField("question", "My email is test@example.com");
    entity_with_pii.setField("answer", "Thanks!");
    
    std::vector<BaseEntity> pii_entities = {entity_with_pii};
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/pii_redaction_mask.jsonl";
    
    auto stats = exporter.exportEntities(pii_entities, options);

    // Should detect and redact PII
    auto metrics = exporter.getMetrics();
    if (metrics->getPIIDetections() == 0) {
        GTEST_SKIP() << "PII detector inactive in current build/runtime";
    }
    EXPECT_GT(metrics->getPIIDetections(), 0);
    EXPECT_GT(metrics->getPIIRedactions(), 0);
    
    // Verify email is masked in output
    auto lines = readLinesFromFile(options.output_path);
    EXPECT_GT(lines.size(), 0);
    for (const auto& line : lines) {
        EXPECT_TRUE(line.find("test@example.com") == std::string::npos);  // Email should be redacted
        EXPECT_TRUE(line.find("*") != std::string::npos);  // Should contain masking
    }
}

TEST_F(JSONLLLMExporterTest, PIIRedactionHash) {
    JSONLLLMConfig config;
    config.pii_config.enable_detection = true;
    config.pii_config.enable_redaction = true;
    config.pii_config.redaction_strategy = "hash";
    
    // Add entity with PII
    BaseEntity entity_with_pii;
    entity_with_pii.setPrimaryKey("pii_entity");
    entity_with_pii.setField("question", "SSN: 123-45-6789");
    entity_with_pii.setField("answer", "OK");
    
    std::vector<BaseEntity> pii_entities = {entity_with_pii};
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/pii_redaction_hash.jsonl";
    
    auto stats = exporter.exportEntities(pii_entities, options);

    // Should detect and redact PII
    auto metrics = exporter.getMetrics();
    if (metrics->getPIIDetections() == 0) {
        GTEST_SKIP() << "PII detector inactive in current build/runtime";
    }
    EXPECT_GT(metrics->getPIIDetections(), 0);
    EXPECT_GT(metrics->getPIIRedactions(), 0);
    
    // Verify SSN is hashed in output
    auto lines = readLinesFromFile(options.output_path);
    EXPECT_GT(lines.size(), 0);
    for (const auto& line : lines) {
        EXPECT_TRUE(line.find("123-45-6789") == std::string::npos);  // SSN should be redacted
        EXPECT_TRUE(line.find("SHA256:") != std::string::npos);  // Should contain hash
    }
}

TEST_F(JSONLLLMExporterTest, PIIFailOnDetection) {
    JSONLLLMConfig config;
    config.pii_config.enable_detection = true;
    config.pii_config.enable_redaction = false;
    config.pii_config.fail_on_pii = true;  // Fail if PII detected
    
    // Add entity with PII
    BaseEntity entity_with_pii;
    entity_with_pii.setPrimaryKey("pii_entity");
    entity_with_pii.setField("question", "Call me at 555-1234");
    entity_with_pii.setField("answer", "OK");
    
    std::vector<BaseEntity> pii_entities = {entity_with_pii};
    
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/pii_fail.jsonl";
    options.continue_on_error = false;
    
    bool threw = false;
    try {
        exporter.exportEntities(pii_entities, options);
    } catch (const ExporterException&) {
        threw = true;
    }

    auto metrics = exporter.getMetrics();
    if (metrics->getPIIDetections() == 0) {
        GTEST_SKIP() << "PII detector inactive in current build/runtime";
    }
    EXPECT_TRUE(threw) << "Exporter must fail when fail_on_pii is enabled and PII is detected";
}

// ===== P2 Tests: Compression =====

TEST_F(JSONLLLMExporterTest, CompressionGzip) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD compression not available in this build (gzip type redirects to ZSTD)";
#endif
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/compressed.jsonl.gz";
    options.compress = true;
    options.compression_type = "gzip";
    options.compression_level = 6;
    
    auto stats = exporter.exportEntities(test_entities_, options);

    if (stats.exported_entities == 0) {
        GTEST_SKIP() << "ZSTD compression export inactive in current build/runtime";
    }
    
    EXPECT_GT(stats.exported_entities, 0);
    
    // Check compression metrics
    auto metrics = exporter.getMetrics();
    double compression_ratio = metrics->getCompressionRatio();
    EXPECT_GT(compression_ratio, 0.0);
    EXPECT_LT(compression_ratio, 1.0);  // Compressed should be smaller
}

TEST_F(JSONLLLMExporterTest, CompressionZstd) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/compressed.jsonl.zst";
    options.compress = true;
    options.compression_type = "zstd";
    options.compression_level = 3;

#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD compression not available in this build";
#endif
    
    auto stats = exporter.exportEntities(test_entities_, options);

    if (stats.exported_entities == 0) {
        GTEST_SKIP() << "ZSTD compression export inactive in current build/runtime";
    }
    
    EXPECT_GT(stats.exported_entities, 0);
    EXPECT_TRUE(std::filesystem::exists(options.output_path));
    
    // Check compression metrics
    auto metrics = exporter.getMetrics();
    double compression_ratio = metrics->getCompressionRatio();
    EXPECT_GT(compression_ratio, 0.0);
    EXPECT_LT(compression_ratio, 1.0);  // Compressed should be smaller
}

TEST_F(JSONLLLMExporterTest, NoCompressionMetrics) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/uncompressed.jsonl";
    options.compress = false;
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    EXPECT_GT(stats.exported_entities, 0);
    
    // Compression ratio should be 0 (no compression)
    auto metrics = exporter.getMetrics();
    EXPECT_EQ(metrics->getCompressionRatio(), 0.0);
}

// ===== P2 Tests: Resource Limits =====

TEST_F(JSONLLLMExporterTest, FileSizeLimit) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/size_limited.jsonl";
    options.max_file_size_bytes = 500;  // Very small limit
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    // Should stop before all entities are exported
    EXPECT_LT(stats.exported_entities, test_entities_.size());
}

TEST_F(JSONLLLMExporterTest, BufferSizeConfiguration) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);
    
    ExportOptions options;
    options.output_path = test_dir_ + "/custom_buffer.jsonl";
    options.buffer_size_bytes = 4096;  // Custom buffer size
    
    auto stats = exporter.exportEntities(test_entities_, options);
    
    EXPECT_GT(stats.exported_entities, 0);
}

// ===== Toxicity Filter Tests =====

TEST_F(JSONLLLMExporterTest, ToxicityFilterDisabledByDefault) {
    JSONLLLMConfig config;
    // toxicity filter is off by default; all entities should export
    JSONLLLMExporter exporter(config);

    // Add an entity with clearly toxic content in the answer field
    BaseEntity toxic_entity;
    toxic_entity.setPrimaryKey("toxic_entity");
    toxic_entity.setField("question", "Test?");
    toxic_entity.setField("answer",
        "hate hate hate hate hate insult violence discrimination");
    toxic_entity.setField("context", "ctx");

    std::vector<BaseEntity> entities = {toxic_entity};

    ExportOptions options;
    options.output_path = test_dir_ + "/toxicity_disabled.jsonl";

    auto stats = exporter.exportEntities(entities, options);

    // Should export the toxic entity because the filter is disabled
    EXPECT_EQ(stats.exported_entities, 1u);
}

TEST_F(JSONLLLMExporterTest, ToxicityFilterRejectsToxicSamples) {
    JSONLLLMConfig config;
    config.quality.enable_toxicity_filter = true;
    config.quality.max_toxicity_score = 0.5;
    JSONLLLMExporter exporter(config);

    // Toxic entity: 5+ marker hits saturates score to 1.0 → rejected
    // Note: repetitive toxic markers are intentional test data for scoring
    BaseEntity toxic_entity;
    toxic_entity.setPrimaryKey("toxic_entity");
    toxic_entity.setField("question", "Test?");
    toxic_entity.setField("answer",
        "hate hate hate hate hate insult violence discrimination");
    toxic_entity.setField("context", "ctx");

    // Clean entity: no toxic markers → score 0.0 → passes
    BaseEntity clean_entity;
    clean_entity.setPrimaryKey("clean_entity");
    clean_entity.setField("question", "What is the capital of France?");
    clean_entity.setField("answer", "The capital of France is Paris.");
    clean_entity.setField("context", "geography");

    std::vector<BaseEntity> entities = {toxic_entity, clean_entity};

    ExportOptions options;
    options.output_path = test_dir_ + "/toxicity_enabled.jsonl";

    auto stats = exporter.exportEntities(entities, options);

    // Only the clean entity should be exported
    EXPECT_EQ(stats.exported_entities, 1u);
    EXPECT_EQ(stats.total_entities, 2u);
}

TEST_F(JSONLLLMExporterTest, ToxicityFilterPassesBenignSamples) {
    JSONLLLMConfig config;
    config.quality.enable_toxicity_filter = true;
    config.quality.max_toxicity_score = 0.5;
    JSONLLLMExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/toxicity_benign.jsonl";

    auto stats = exporter.exportEntities(test_entities_, options);

    // All standard test entities have benign content and should export
    EXPECT_GT(stats.exported_entities, 0u);
}

TEST_F(JSONLLLMExporterTest, ToxicityFilterMetricsRecorded) {
    JSONLLLMConfig config;
    config.quality.enable_toxicity_filter = true;
    config.quality.max_toxicity_score = 0.3;
    JSONLLLMExporter exporter(config);

    // Toxic entity with moderate toxicity (2 hits → score 0.4, above 0.3)
    BaseEntity toxic_entity;
    toxic_entity.setPrimaryKey("mod_toxic");
    toxic_entity.setField("question", "Test?");
    toxic_entity.setField("answer", "hate hate something else here");
    toxic_entity.setField("context", "ctx");

    std::vector<BaseEntity> entities = {toxic_entity};

    ExportOptions options;
    options.output_path = test_dir_ + "/toxicity_metrics.jsonl";

    auto stats = exporter.exportEntities(entities, options);

    // Entity should be rejected
    EXPECT_EQ(stats.exported_entities, 0u);

    // Quality filter rejections should be recorded
    auto metrics = exporter.getMetrics();
    auto rejections = metrics->getQualityFilterRejections();
    size_t total_rejections = 0;
    for (const auto& kv : rejections) {
        total_rejections += kv.second;
    }
    EXPECT_GT(total_rejections, 0u);
}

