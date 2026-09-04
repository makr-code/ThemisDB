/*
 * @file test_ingestion_schema_validation.cpp
 * @brief Unit tests for per-source schema validation before write (Issue #1896).
 *
 * Tests cover:
 *  - SchemaConfig / SchemaFieldRule / DocumentValidationResult structs
 *  - buildValidatorFromSchema via IngestionManager::setSchemaConfig
 *  - FileSystemIngester with schema validation
 *  - IngestionBuilder::withSchemaValidation
 *  - IngestionManager::getSchemaConfig / setSchemaConfig API
 */

#include <gtest/gtest.h>
#include "ingestion/ingestion_manager.h"
#include "ingestion/filesystem_ingester.h"
#include "ingestion/api_connector.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace themis::ingestion;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Write content to a temporary file and return its path.
static std::string writeTmp(const fs::path& dir, const std::string& name,
                             const std::string& content) {
    fs::create_directories(dir);
    auto p = dir / name;
    std::ofstream f(p);
    f << content;
    return p.string();
}

} // namespace

// ============================================================================
// SchemaConfig – struct behaviour
// ============================================================================

TEST(SchemaConfigTest, DefaultIsDisabled) {
    SchemaConfig sc;
    EXPECT_FALSE(sc.isEnabled());
}

TEST(SchemaConfigTest, MinLengthEnablesSchema) {
    SchemaConfig sc;
    sc.min_content_length = 10;
    EXPECT_TRUE(sc.isEnabled());
}

TEST(SchemaConfigTest, MaxLengthEnablesSchema) {
    SchemaConfig sc;
    sc.max_content_length = 1000;
    EXPECT_TRUE(sc.isEnabled());
}

TEST(SchemaConfigTest, PatternEnablesSchema) {
    SchemaConfig sc;
    sc.required_content_pattern = ".*keyword.*";
    EXPECT_TRUE(sc.isEnabled());
}

TEST(SchemaConfigTest, FieldRulesEnableSchema) {
    SchemaConfig sc;
    sc.fields["title"] = SchemaFieldRule{};
    EXPECT_TRUE(sc.isEnabled());
}

// ============================================================================
// DocumentValidationResult – struct behaviour
// ============================================================================

TEST(DocumentValidationResultTest, DefaultIsValid) {
    DocumentValidationResult r;
    EXPECT_TRUE(r.is_valid);
    EXPECT_TRUE(r.violations.empty());
}

TEST(DocumentValidationResultTest, AddViolationSetsInvalid) {
    DocumentValidationResult r;
    r.addViolation("field_a", "missing");
    EXPECT_FALSE(r.is_valid);
    ASSERT_EQ(r.violations.size(), 1u);
    EXPECT_EQ(r.violations[0].field, "field_a");
    EXPECT_EQ(r.violations[0].message, "missing");
}

TEST(DocumentValidationResultTest, SummaryMultipleViolations) {
    DocumentValidationResult r;
    r.addViolation("a", "too short");
    r.addViolation("", "bad pattern");
    EXPECT_NE(r.summary().find("a: too short"), std::string::npos);
    EXPECT_NE(r.summary().find("bad pattern"), std::string::npos);
}

// ============================================================================
// IngestionManager – setSchemaConfig / getSchemaConfig API
// ============================================================================

TEST(IngestionManagerSchemaTest, GetSchemaConfigReturnsFalseWhenNotSet) {
    IngestionManager mgr("test_db");
    SchemaConfig out;
    EXPECT_FALSE(mgr.getSchemaConfig("nonexistent", out));
}

TEST(IngestionManagerSchemaTest, SetAndGetSchemaConfig) {
    IngestionManager mgr("test_db");

    SchemaConfig sc;
    sc.min_content_length = 5;
    mgr.setSchemaConfig("src1", sc);

    SchemaConfig out;
    ASSERT_TRUE(mgr.getSchemaConfig("src1", out));
    EXPECT_EQ(out.min_content_length, 5u);
}

TEST(IngestionManagerSchemaTest, SetDisabledSchemaRemovesEntry) {
    IngestionManager mgr("test_db");

    SchemaConfig sc;
    sc.min_content_length = 5;
    mgr.setSchemaConfig("src1", sc);

    // Replace with a disabled schema → should be removed
    mgr.setSchemaConfig("src1", SchemaConfig{});

    SchemaConfig out;
    EXPECT_FALSE(mgr.getSchemaConfig("src1", out));
}

TEST(IngestionManagerSchemaTest, SchemaIsPerSource) {
    IngestionManager mgr("test_db");

    SchemaConfig sc1;
    sc1.min_content_length = 10;
    mgr.setSchemaConfig("src_a", sc1);

    SchemaConfig sc2;
    sc2.max_content_length = 100;
    mgr.setSchemaConfig("src_b", sc2);

    SchemaConfig out_a, out_b;
    ASSERT_TRUE(mgr.getSchemaConfig("src_a", out_a));
    ASSERT_TRUE(mgr.getSchemaConfig("src_b", out_b));
    EXPECT_EQ(out_a.min_content_length, 10u);
    EXPECT_EQ(out_b.max_content_length, 100u);
}

// ============================================================================
// FileSystemIngester – schema validation integration
// ============================================================================

class FSIngesterSchemaTest : public ::testing::Test {
protected:
    fs::path tmp_dir_;

    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "themis_schema_test";
        fs::remove_all(tmp_dir_);
        fs::create_directories(tmp_dir_);
    }
    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }
};

TEST_F(FSIngesterSchemaTest, AllDocumentsPassWithNoValidator) {
    writeTmp(tmp_dir_, "a.txt", "hello world");
    writeTmp(tmp_dir_, "b.txt", "another doc");

    FileSystemIngester ing;
    SourceConfig cfg;
    cfg.source_id = "fs_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    ASSERT_TRUE(ing.initialize(cfg));

    auto stats = ing.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u);
    EXPECT_EQ(stats.documents_failed,    0u);
}

TEST_F(FSIngesterSchemaTest, MinLengthRejectsShortDocument) {
    writeTmp(tmp_dir_, "short.txt", "hi");          // 2 bytes → fails min 10
    writeTmp(tmp_dir_, "long.txt",  "hello world"); // 11 bytes → passes

    // Use IngestionManager to exercise the production buildValidatorFromSchema path
    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "fs_schema";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    SchemaConfig sc;
    sc.min_content_length = 10;
    mgr.setSchemaConfig("fs_schema", sc);

    auto stats = mgr.ingestSource("fs_schema");
    EXPECT_EQ(stats.documents_processed, 1u); // only long.txt
    EXPECT_EQ(stats.documents_failed,    1u); // short.txt rejected
}

TEST_F(FSIngesterSchemaTest, MaxLengthRejectsLongDocument) {
    writeTmp(tmp_dir_, "long.txt",  std::string(200, 'x')); // 200 bytes → fails max 100
    writeTmp(tmp_dir_, "short.txt", std::string(50, 'y'));   // 50 bytes  → passes

    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "fs_schema";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    SchemaConfig sc;
    sc.max_content_length = 100;
    mgr.setSchemaConfig("fs_schema", sc);

    auto stats = mgr.ingestSource("fs_schema");
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed,    1u);
}

TEST_F(FSIngesterSchemaTest, ValidationFailureRecordsSchemaError) {
    writeTmp(tmp_dir_, "bad.txt", "x"); // 1 byte → fails min 5

    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "fs_schema";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    SchemaConfig sc;
    sc.min_content_length = 5;
    mgr.setSchemaConfig("fs_schema", sc);

    auto stats = mgr.ingestSource("fs_schema");
    EXPECT_EQ(stats.documents_failed, 1u);
    bool found_schema_error = false;
    for (const auto& err : stats.errors) {
        if (err.code == IngestionErrorCode::SCHEMA_VALIDATION_FAILED) {
            found_schema_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_schema_error);
}

TEST_F(FSIngesterSchemaTest, DirectValidatorCallbackHookWorks) {
    // Tests the setDocumentValidator() hook directly without IngestionManager
    writeTmp(tmp_dir_, "doc.txt", "hello");

    FileSystemIngester ing;
    SourceConfig cfg;
    cfg.source_id = "fs_direct";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    ASSERT_TRUE(ing.initialize(cfg));

    // A validator that always rejects
    ing.setDocumentValidator([](const std::string&) -> DocumentValidationResult {
        DocumentValidationResult r;
        r.addViolation("", "always reject");
        return r;
    });

    auto stats = ing.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed,    1u);
}

TEST_F(FSIngesterSchemaTest, EmptyValidatorFunctionDoesNotRejectDocs) {
    writeTmp(tmp_dir_, "doc.txt", "hello");

    FileSystemIngester ing;
    SourceConfig cfg;
    cfg.source_id = "fs_schema";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    ASSERT_TRUE(ing.initialize(cfg));

    // Set an empty (disabled) validator — no rejection expected
    ing.setDocumentValidator(DocumentValidatorFn{});

    auto stats = ing.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed,    0u);
}

// ============================================================================
// IngestionManager – end-to-end via ingestSource with schema
// ============================================================================

class IngestionManagerSchemaE2ETest : public ::testing::Test {
protected:
    fs::path tmp_dir_;

    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "themis_mgr_schema_test";
        fs::remove_all(tmp_dir_);
        fs::create_directories(tmp_dir_);
    }
    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }
};

TEST_F(IngestionManagerSchemaE2ETest, NoSchemaAllDocsPass) {
    writeTmp(tmp_dir_, "a.txt", "small");
    writeTmp(tmp_dir_, "b.txt", "also small");

    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "e2e_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    auto stats = mgr.ingestSource("e2e_src");
    EXPECT_EQ(stats.documents_processed, 2u);
    EXPECT_EQ(stats.documents_failed,    0u);
}

TEST_F(IngestionManagerSchemaE2ETest, MinLengthSchemaRejectsShortDocs) {
    writeTmp(tmp_dir_, "short.txt", "hi");             // 2 bytes < 10 → rejected
    writeTmp(tmp_dir_, "long.txt",  "hello world!!"); // 13 bytes ≥ 10 → accepted

    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "e2e_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    SchemaConfig sc;
    sc.min_content_length = 10;
    mgr.setSchemaConfig("e2e_src", sc);

    auto stats = mgr.ingestSource("e2e_src");
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed,    1u);
}

TEST_F(IngestionManagerSchemaE2ETest, PatternSchemaRejectsNonMatchingDocs) {
    writeTmp(tmp_dir_, "match.txt",    "keyword present here");
    writeTmp(tmp_dir_, "no_match.txt", "nothing relevant");

    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "e2e_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    SchemaConfig sc;
    sc.required_content_pattern = "keyword";
    mgr.setSchemaConfig("e2e_src", sc);

    auto stats = mgr.ingestSource("e2e_src");
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed,    1u);
}

TEST_F(IngestionManagerSchemaE2ETest, RequiredJsonFieldRejection) {
    // JSON documents: one with "title", one without
    writeTmp(tmp_dir_, "good.json",    R"({"title":"Hello","body":"World"})");
    writeTmp(tmp_dir_, "missing.json", R"({"body":"No title here"})");

    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "e2e_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    SchemaConfig sc;
    SchemaFieldRule title_rule;
    title_rule.required = true;
    sc.fields["title"] = title_rule;
    mgr.setSchemaConfig("e2e_src", sc);

    auto stats = mgr.ingestSource("e2e_src");
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed,    1u);

    // The rejection should be recorded with the SCHEMA_VALIDATION_FAILED code
    bool found = false;
    for (const auto& err : stats.errors) {
        if (err.code == IngestionErrorCode::SCHEMA_VALIDATION_FAILED) {
            found = true;
            EXPECT_NE(err.message.find("title"), std::string::npos)
                << "error message should mention the failing field";
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected SCHEMA_VALIDATION_FAILED error in stats";
}

TEST_F(IngestionManagerSchemaE2ETest, StringMinLengthFieldRejection) {
    writeTmp(tmp_dir_, "long_title.json",  R"({"title":"Long enough title"})");
    writeTmp(tmp_dir_, "short_title.json", R"({"title":"Hi"})");

    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "e2e_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    SchemaConfig sc;
    SchemaFieldRule title_rule;
    title_rule.required      = true;
    title_rule.expected_type = SchemaFieldType::STRING;
    title_rule.min_length    = 10;
    sc.fields["title"] = title_rule;
    mgr.setSchemaConfig("e2e_src", sc);

    auto stats = mgr.ingestSource("e2e_src");
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed,    1u);
}

TEST_F(IngestionManagerSchemaE2ETest, StringMaxLengthFieldRejection) {
    writeTmp(tmp_dir_, "ok.json",   R"({"title":"Short"})");
    writeTmp(tmp_dir_, "long.json", R"({"title":"This title is way too long to pass"})");

    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "e2e_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    SchemaConfig sc;
    SchemaFieldRule r;
    r.max_length = 10;
    sc.fields["title"] = r;
    mgr.setSchemaConfig("e2e_src", sc);

    auto stats = mgr.ingestSource("e2e_src");
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed,    1u);
}

TEST_F(IngestionManagerSchemaE2ETest, FieldPatternRejection) {
    writeTmp(tmp_dir_, "ok.json",  R"({"status":"active"})");
    writeTmp(tmp_dir_, "bad.json", R"({"status":"pending"})");

    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "e2e_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    SchemaConfig sc;
    SchemaFieldRule r;
    r.pattern = "^(active|inactive)$";
    sc.fields["status"] = r;
    mgr.setSchemaConfig("e2e_src", sc);

    auto stats = mgr.ingestSource("e2e_src");
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed,    1u);
}

// ============================================================================
// IngestionBuilder::withSchemaValidation
// ============================================================================

class IngestionBuilderSchemaTest : public ::testing::Test {
protected:
    fs::path tmp_dir_;

    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "themis_builder_schema_test";
        fs::remove_all(tmp_dir_);
        fs::create_directories(tmp_dir_);
    }
    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }
};

TEST_F(IngestionBuilderSchemaTest, WithSchemaValidationPropagatedToBuild) {
    writeTmp(tmp_dir_, "short.txt", "hi");
    writeTmp(tmp_dir_, "long.txt",  "hello world!!");

    SchemaConfig sc;
    sc.min_content_length = 10;

    auto mgr = IngestionBuilder("test_db")
        .withFilesystemSource("bsrc", tmp_dir_.string())
        .withSchemaValidation("bsrc", sc)
        .withTargetCollection("col")
        .build();

    // Verify schema was propagated
    SchemaConfig out;
    ASSERT_TRUE(mgr->getSchemaConfig("bsrc", out));
    EXPECT_EQ(out.min_content_length, 10u);

    auto stats = mgr->ingestSource("bsrc");
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed,    1u);
}

TEST_F(IngestionBuilderSchemaTest, MultipleSourceSchemasAreIndependent) {
    auto dir_a = tmp_dir_ / "a";
    auto dir_b = tmp_dir_ / "b";
    fs::create_directories(dir_a);
    fs::create_directories(dir_b);

    writeTmp(dir_a, "doc.txt", "hi"); // too short for src_a (min 10)
    writeTmp(dir_b, "doc.txt", "hello world and more text"); // fine for src_b (max 100)

    SchemaConfig sc_a;
    sc_a.min_content_length = 10;

    SchemaConfig sc_b;
    sc_b.max_content_length = 100;

    auto mgr = IngestionBuilder("test_db")
        .withFilesystemSource("src_a", dir_a.string())
        .withFilesystemSource("src_b", dir_b.string())
        .withSchemaValidation("src_a", sc_a)
        .withSchemaValidation("src_b", sc_b)
        .build();

    auto stats_a = mgr->ingestSource("src_a");
    auto stats_b = mgr->ingestSource("src_b");

    EXPECT_EQ(stats_a.documents_processed, 0u);
    EXPECT_EQ(stats_a.documents_failed,    1u);

    EXPECT_EQ(stats_b.documents_processed, 1u);
    EXPECT_EQ(stats_b.documents_failed,    0u);
}

// ============================================================================
// GenericApiConnector – setDocumentValidator
// ============================================================================

TEST(ApiConnectorSchemaTest, ValidatorRejectsInvalidDocs) {
    GenericApiConnector connector;
    SourceConfig cfg;
    cfg.source_id = "api_src";
    cfg.type      = SourceType::API;
    cfg.location  = "http://example.com/api";
    cfg.options["text_field"] = "text";
    cfg.options["page_size"]  = "3";

    // Inject a mock HTTP GET that returns 3 documents
    connector.setHttpGetForTesting([](const std::string&,
                                      const std::string&)
                                     -> std::pair<int, std::string> {
        return {200,
                R"({"total":3,"items":[)"
                R"({"text":"keyword in here"},)"
                R"({"text":"no match"},)"
                R"({"text":"another keyword"})"
                R"(]})"};
    });

    ASSERT_TRUE(connector.initialize(cfg));

    // Validator: only accept docs that contain "keyword"
    connector.setDocumentValidator([](const std::string& content) -> DocumentValidationResult {
        DocumentValidationResult r = {};
        if (content.find("keyword") == std::string::npos) {
            r.addViolation("", "missing keyword");
        }
        return r;
    });

    auto stats = connector.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 2u); // 2 contain "keyword"
    EXPECT_EQ(stats.documents_failed,    1u); // 1 rejected
}

// ============================================================================
// reject_invalid = false (warning-only mode)
// ============================================================================

class RejectInvalidFalseTest : public ::testing::Test {
protected:
    fs::path tmp_dir_;

    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "themis_reject_invalid_test";
        fs::remove_all(tmp_dir_);
        fs::create_directories(tmp_dir_);
    }
    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }
};

TEST_F(RejectInvalidFalseTest, WarningOnlyDoesNotCountAsFailed) {
    writeTmp(tmp_dir_, "short.txt", "hi");          // 2 bytes < 10 → violation but NOT rejected
    writeTmp(tmp_dir_, "long.txt",  "hello world"); // passes

    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "warn_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    SchemaConfig sc;
    sc.min_content_length = 10;
    sc.reject_invalid     = false; // warning-only
    mgr.setSchemaConfig("warn_src", sc);

    auto stats = mgr.ingestSource("warn_src");
    // Both docs must be processed (not failed)
    EXPECT_EQ(stats.documents_processed, 2u);
    EXPECT_EQ(stats.documents_failed,    0u);
}

TEST_F(RejectInvalidFalseTest, WarningOnlyRecordsInfoError) {
    writeTmp(tmp_dir_, "short.txt", "hi"); // 2 bytes < 10 → INFO warning

    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "warn_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    SchemaConfig sc;
    sc.min_content_length = 10;
    sc.reject_invalid     = false;
    mgr.setSchemaConfig("warn_src", sc);

    auto stats = mgr.ingestSource("warn_src");
    EXPECT_EQ(stats.documents_processed, 1u);

    bool found_info = false;
    for (const auto& err : stats.errors) {
        if (err.code == IngestionErrorCode::SCHEMA_VALIDATION_FAILED &&
            err.severity == IngestionErrorSeverity::INFO) {
            found_info = true;
            break;
        }
    }
    EXPECT_TRUE(found_info) << "Expected INFO-level schema warning in stats.errors";
}

TEST_F(RejectInvalidFalseTest, WarningOnlyIncrementsSchemaViolationsMetric) {
    writeTmp(tmp_dir_, "short.txt", "hi"); // violation

    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "warn_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    SchemaConfig sc;
    sc.min_content_length = 10;
    sc.reject_invalid     = false;
    mgr.setSchemaConfig("warn_src", sc);

    auto stats = mgr.ingestSource("warn_src");
    EXPECT_EQ(stats.metrics.schema_violations, 1u);
    EXPECT_EQ(stats.documents_processed, 1u); // still processed
}

TEST_F(RejectInvalidFalseTest, RejectInvalidTrueStillFails) {
    writeTmp(tmp_dir_, "short.txt", "hi"); // violation + reject

    IngestionManager mgr("test_db");
    SourceConfig cfg;
    cfg.source_id = "reject_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir_.string();
    mgr.registerSource(cfg);

    SchemaConfig sc;
    sc.min_content_length = 10;
    sc.reject_invalid     = true; // default – should reject
    mgr.setSchemaConfig("reject_src", sc);

    auto stats = mgr.ingestSource("reject_src");
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed,    1u);
    EXPECT_EQ(stats.metrics.schema_violations, 1u);
}

// ============================================================================
// schema_violations metric in Prometheus export
// ============================================================================

TEST(SchemaMetricsTest, SchemaViolationsExportedInPrometheus) {
    IngestionStats stats;
    stats.documents_processed = 5;
    stats.documents_failed    = 2;
    stats.metrics.schema_violations = 3;

    IngestionMetricsExporter exporter;
    std::string output = exporter.exportText(stats, "src1", "FILESYSTEM");

    EXPECT_NE(output.find("schema_violations_total"), std::string::npos)
        << "Expected schema_violations_total metric in Prometheus output";

    // Verify the specific metric line contains the value 3.
    // The Prometheus line looks like: prefix_schema_violations_total{source_id="src1",...} 3
    auto pos = output.find("schema_violations_total");
    ASSERT_NE(pos, std::string::npos);
    auto newline = output.find('\n', pos);
    ASSERT_NE(newline, std::string::npos);
    // Find the metric value after the label block on the same line
    auto brace_close = output.find('}', pos);
    ASSERT_NE(brace_close, std::string::npos);
    std::string metric_line = output.substr(brace_close, newline - brace_close);
    EXPECT_NE(metric_line.find("3"), std::string::npos)
        << "Expected value 3 in schema_violations_total metric line: " << metric_line;
}
