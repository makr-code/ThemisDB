/*
 * ThemisDB | File: test_join_exporter.cpp | Version: 0.0.12
 * Maturity: 🟢 PRODUCTION-READY | Score: 97/100
 * Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <gtest/gtest.h>
#include "exporters/join_exporter.h"
#include "exporters/exporter_errors.h"
#include "exporters/exporter_interface.h"
#include "storage/base_entity.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace themis::exporters;
using namespace themis;
using json = nlohmann::json;
namespace fs = std::filesystem;

// ── Helpers ──────────────────────────────────────────────────────────────────

static BaseEntity makeDoc(const std::string& key,
                           const std::string& content,
                           const std::string& join_key) {
    BaseEntity e;
    e.setPrimaryKey(key);
    e.setField("_key",    std::string(join_key));
    e.setField("content", std::string(content));
    return e;
}

static BaseEntity makeAnnotation(const std::string& key,
                                  const std::string& doc_id,
                                  const std::string& label,
                                  double score = 1.0) {
    BaseEntity e;
    e.setPrimaryKey(key);
    e.setField("doc_id", std::string(doc_id));
    e.setField("label",  std::string(label));
    e.setField("score",  score);
    return e;
}

static std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

// ── Fixture ───────────────────────────────────────────────────────────────────

class JoinExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = (fs::temp_directory_path() /
                     ("themis_join_exporter_test_" +
                      std::to_string(
                          std::chrono::steady_clock::now().time_since_epoch().count())))
                        .string();
        fs::create_directories(test_dir_);

        // Left: 4 documents
        left_ = {
            makeDoc("d1", "hello world",   "k1"),
            makeDoc("d2", "foo bar",       "k2"),
            makeDoc("d3", "baz qux",       "k3"),
            makeDoc("d4", "no annotation", "k4"),
        };

        // Right: 3 annotations (d4 deliberately has no match)
        right_ = {
            makeAnnotation("a1", "k1", "positive", 0.9),
            makeAnnotation("a2", "k2", "negative", 0.7),
            makeAnnotation("a3", "k3", "neutral",  0.5),
        };
    }

    void TearDown() override {
        if (fs::exists(test_dir_)) fs::remove_all(test_dir_);
    }

    std::string outPath(const std::string& name) const {
        return test_dir_ + "/" + name;
    }

    std::string test_dir_;
    std::vector<BaseEntity> left_;
    std::vector<BaseEntity> right_;
};

// ── AC-4: JoinExportConfig struct ─────────────────────────────────────────────

TEST(JoinExportConfigTest, DefaultValues) {
    JoinExportConfig cfg;
    EXPECT_EQ(cfg.left_key_field,  "_key");
    EXPECT_EQ(cfg.right_key_field, "_key");
    EXPECT_TRUE(cfg.join_predicate.empty());
    EXPECT_TRUE(cfg.output_fields.empty());
    EXPECT_EQ(cfg.right_side_memory_limit_bytes, 1ULL * 1024 * 1024 * 1024);
    EXPECT_FALSE(cfg.pii_config.enable_detection);
}

TEST(JoinExportConfigTest, FieldsCanBeSet) {
    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "id";
    cfg.right_key_field  = "doc_id";
    cfg.join_predicate   = "doc.score > 0.5";
    cfg.output_fields    = {"content", "label"};
    cfg.right_side_memory_limit_bytes = 512ULL * 1024 * 1024;

    EXPECT_EQ(cfg.left_collection,  "docs");
    EXPECT_EQ(cfg.right_key_field,  "doc_id");
    EXPECT_EQ(cfg.join_predicate,   "doc.score > 0.5");
    EXPECT_EQ(cfg.output_fields.size(), 2u);
    EXPECT_EQ(cfg.right_side_memory_limit_bytes, 512ULL * 1024 * 1024);
}

// ── AC-5: basic inner join ────────────────────────────────────────────────────

TEST_F(JoinExporterTest, BasicInnerJoin) {
    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "doc_id";

    JoinExporter exporter(cfg);
    exporter.setRightCollection(right_);

    ExportOptions opts;
    opts.output_path = outPath("basic.jsonl");

    auto stats = exporter.exportEntities(left_, opts);

    // d4 has no matching annotation → 3 exported, 1 skipped
    EXPECT_EQ(stats.exported_entities, 3u);
    EXPECT_EQ(stats.total_entities,    4u);
    EXPECT_EQ(stats.skipped_entities,  1u);
    EXPECT_EQ(stats.failed_entities,   0u);
    EXPECT_GT(stats.bytes_written,     0u);

    const auto lines = readLines(opts.output_path);
    EXPECT_EQ(lines.size(), 3u);

    // Every output line must be valid JSON.
    for (const auto& l : lines) {
        EXPECT_NO_THROW({
            auto parsed = json::parse(l);
            static_cast<void>(parsed);
        });
    }
}

TEST_F(JoinExporterTest, MergedDocContainsBothSideFields) {
    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "doc_id";
    // Only select these fields to avoid ambiguity on "_key"
    cfg.output_fields = {"content", "label", "score"};

    JoinExporter exporter(cfg);
    exporter.setRightCollection(right_);

    ExportOptions opts;
    opts.output_path = outPath("fields.jsonl");

    auto stats = exporter.exportEntities(left_, opts);
    EXPECT_EQ(stats.exported_entities, 3u);

    const auto lines = readLines(opts.output_path);
    ASSERT_EQ(lines.size(), 3u);

    const auto row0 = json::parse(lines[0]);
    EXPECT_TRUE(row0.contains("content"));
    EXPECT_TRUE(row0.contains("label"));
    EXPECT_TRUE(row0.contains("score"));
}

// ── AC-5: output_fields aliases ───────────────────────────────────────────────

TEST_F(JoinExporterTest, OutputFieldsSelectAndRename) {
    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "doc_id";
    cfg.output_fields    = {"content:text", "label:category"};

    JoinExporter exporter(cfg);
    exporter.setRightCollection(right_);

    ExportOptions opts;
    opts.output_path = outPath("renamed.jsonl");

    auto stats = exporter.exportEntities(left_, opts);
    EXPECT_EQ(stats.exported_entities, 3u);

    const auto lines = readLines(opts.output_path);
    ASSERT_EQ(lines.size(), 3u);

    const auto row0 = json::parse(lines[0]);
    EXPECT_TRUE(row0.contains("text"));
    EXPECT_TRUE(row0.contains("category"));
    EXPECT_FALSE(row0.contains("content"));
    EXPECT_FALSE(row0.contains("label"));
}

TEST_F(JoinExporterTest, OutputFieldsQualifiedLeftRight) {
    // Create two entities that share a field name "title"
    BaseEntity l;
    l.setPrimaryKey("l1");
    l.setField("_key",  std::string("x"));
    l.setField("title", std::string("doc_title"));

    BaseEntity r;
    r.setPrimaryKey("r1");
    r.setField("_key",  std::string("x"));
    r.setField("title", std::string("ann_title"));

    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "_key";
    cfg.output_fields    = {"left.title:doc_title", "right.title:ann_title"};

    JoinExporter exporter(cfg);
    exporter.setRightCollection({r});

    ExportOptions opts;
    opts.output_path = outPath("qualified.jsonl");

    auto stats = exporter.exportEntities({l}, opts);
    EXPECT_EQ(stats.exported_entities, 1u);

    const auto lines = readLines(opts.output_path);
    ASSERT_EQ(lines.size(), 1u);
    const auto row0 = json::parse(lines[0]);
    EXPECT_EQ(row0["doc_title"].get<std::string>(), "doc_title");
    EXPECT_EQ(row0["ann_title"].get<std::string>(), "ann_title");
}

// ── AC-5: join_predicate filter ───────────────────────────────────────────────

TEST_F(JoinExporterTest, JoinPredicateFiltersRows) {
    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "doc_id";
    cfg.join_predicate   = "doc.score >= 0.8";  // should pass k1(0.9), reject k2(0.7),k3(0.5)
    cfg.output_fields    = {"content", "label", "score"};

    JoinExporter exporter(cfg);
    exporter.setRightCollection(right_);

    ExportOptions opts;
    opts.output_path = outPath("predicate.jsonl");

    auto stats = exporter.exportEntities(left_, opts);
    EXPECT_EQ(stats.exported_entities, 1u);

    const auto lines = readLines(opts.output_path);
    ASSERT_EQ(lines.size(), 1u);
    const auto row0 = json::parse(lines[0]);
    EXPECT_EQ(row0["label"].get<std::string>(), "positive");
}

TEST_F(JoinExporterTest, ExportOptionsFilterExpressionFiltersMergedRows) {
    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "doc_id";
    cfg.output_fields    = {"content", "label", "score"};

    JoinExporter exporter(cfg);
    exporter.setRightCollection(right_);

    ExportOptions opts;
    opts.output_path = outPath("options_filter_expression.jsonl");
    opts.filter_expression = "doc.score >= 0.8";

    auto stats = exporter.exportEntities(left_, opts);
    EXPECT_EQ(stats.exported_entities, 1u);

    const auto lines = readLines(opts.output_path);
    ASSERT_EQ(lines.size(), 1u);
    const auto row0 = json::parse(lines[0]);
    EXPECT_EQ(row0["label"].get<std::string>(), "positive");
}

// ── AC-7: error cases ─────────────────────────────────────────────────────────

TEST(JoinExporterErrorTest, EmptyLeftCollectionThrows) {
    JoinExportConfig cfg;
    cfg.left_collection  = "";
    cfg.right_collection = "annotations";

    JoinExporter exporter(cfg);
    exporter.setRightCollection({});

    ExportOptions opts;
    opts.output_path = (fs::temp_directory_path() / "join_err_left.jsonl").string();

    EXPECT_THROW({
        try {
            exporter.exportEntities({}, opts);
        } catch (const ExporterException& e) {
            EXPECT_EQ(e.getErrorCode(),
                      errors::ErrorCode::ERR_EXPORT_CONFIG_INVALID);
            throw;
        }
    }, ExporterException);
}

TEST(JoinExporterErrorTest, EmptyRightCollectionSetRightThrows) {
    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "";

    JoinExporter exporter(cfg);

    EXPECT_THROW({
        try {
            exporter.setRightCollection({});
        } catch (const ExporterException& e) {
            EXPECT_EQ(e.getErrorCode(),
                      errors::ErrorCode::ERR_EXPORT_CONFIG_INVALID);
            throw;
        }

        TEST(JoinExporterErrorTest, ExportWithoutSetRightCollectionFailsClosed) {
            JoinExportConfig cfg;
            cfg.left_collection  = "docs";
            cfg.right_collection = "annotations";
            cfg.left_key_field   = "_key";
            cfg.right_key_field  = "_key";

            JoinExporter exporter(cfg);

            ExportOptions opts;
            opts.output_path = (fs::temp_directory_path() / "join_err_right_not_loaded.jsonl").string();
            opts.continue_on_error = false;

            BaseEntity l;
            l.setPrimaryKey("l1");
            l.setField("_key", std::string("x"));

            EXPECT_THROW({
                try {
                    exporter.exportEntities({l}, opts);
                } catch (const ExporterException& e) {
                    EXPECT_EQ(e.getErrorCode(),
                              errors::ErrorCode::ERR_EXPORT_CONFIG_INVALID);
                    throw;
                }
            }, ExporterException);
        }
    }, ExporterException);
}

TEST(JoinExporterErrorTest, InvalidJoinPredicateThrows) {
    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "_key";
    cfg.join_predicate   = "doc.INVALID (((";  // intentionally bad

    JoinExporter exporter(cfg);
    exporter.setRightCollection({});

    ExportOptions opts;
    opts.output_path = (fs::temp_directory_path() / "join_err_pred.jsonl").string();

    EXPECT_THROW({
        try {
            exporter.exportEntities({}, opts);
        } catch (const ExporterException& e) {
            EXPECT_EQ(e.getErrorCode(),
                      errors::ErrorCode::ERR_EXPORT_JOIN_PREDICATE_INVALID);
            throw;
        }
    }, ExporterException);
}

TEST(JoinExporterErrorTest, AmbiguousFieldWithoutAliasThrows) {
    BaseEntity l;
    l.setPrimaryKey("l1");
    l.setField("_key",  std::string("x"));
    l.setField("title", std::string("doc_title"));

    BaseEntity r;
    r.setPrimaryKey("r1");
    r.setField("_key",  std::string("x"));
    r.setField("title", std::string("ann_title"));

    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "_key";
    // No output_fields → all fields included → "title" is ambiguous

    JoinExporter exporter(cfg);
    exporter.setRightCollection({r});

    ExportOptions opts;
    opts.output_path = (fs::temp_directory_path() / "join_err_ambig.jsonl").string();
    opts.continue_on_error = false;

    EXPECT_THROW({
        try {
            exporter.exportEntities({l}, opts);
        } catch (const ExporterException& e) {
            EXPECT_EQ(e.getErrorCode(),
                      errors::ErrorCode::ERR_EXPORT_JOIN_AMBIGUOUS_FIELD);
            throw;
        }
    }, ExporterException);
}

TEST(JoinExporterErrorTest, MemoryLimitExceededThrows) {
    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "_key";
    cfg.right_side_memory_limit_bytes = 1;  // 1 byte limit → first row exceeds it

    JoinExporter exporter(cfg);

    BaseEntity r;
    r.setPrimaryKey("r1");
    r.setField("_key", std::string("x"));
    r.setField("data", std::string("some data here"));

    EXPECT_THROW({
        try {
            exporter.setRightCollection({r});
        } catch (const ExporterException& e) {
            EXPECT_EQ(e.getErrorCode(),
                      errors::ErrorCode::ERR_EXPORT_JOIN_MEMORY_LIMIT);
            throw;
        }
    }, ExporterException);
}

TEST(JoinExporterErrorTest, AmbiguousFieldInOutputFieldsWithoutAliasThrows) {
    BaseEntity l;
    l.setPrimaryKey("l1");
    l.setField("_key",  std::string("x"));
    l.setField("title", std::string("left_title"));

    BaseEntity r;
    r.setPrimaryKey("r1");
    r.setField("_key",  std::string("x"));
    r.setField("title", std::string("right_title"));

    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "_key";
    cfg.output_fields    = {"title"};  // unqualified, ambiguous

    JoinExporter exporter(cfg);
    exporter.setRightCollection({r});

    ExportOptions opts;
    opts.output_path = (fs::temp_directory_path() / "join_err_ambig2.jsonl").string();
    opts.continue_on_error = false;

    EXPECT_THROW({
        try {
            exporter.exportEntities({l}, opts);
        } catch (const ExporterException& e) {
            EXPECT_EQ(e.getErrorCode(),
                      errors::ErrorCode::ERR_EXPORT_JOIN_AMBIGUOUS_FIELD);
            throw;
        }
    }, ExporterException);
}

// ── AC-6: PII detection on merged record ─────────────────────────────────────

TEST_F(JoinExporterTest, PIIDetectionOnMergedRecord) {
    // Embed a fake SSN in the right side annotation.
    BaseEntity r_pii;
    r_pii.setPrimaryKey("r_pii");
    r_pii.setField("doc_id", std::string("k1"));
    r_pii.setField("label",  std::string("test"));
    r_pii.setField("note",   std::string("SSN: 123-45-6789"));

    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "doc_id";
    cfg.output_fields    = {"content", "label", "note"};
    cfg.pii_config.enable_detection  = true;
    cfg.pii_config.enable_redaction  = true;
    cfg.pii_config.detect_ssn        = true;
    cfg.pii_config.redaction_strategy = "mask";

    JoinExporter exporter(cfg);
    exporter.setRightCollection({r_pii});

    ExportOptions opts;
    opts.output_path = outPath("pii.jsonl");

    auto stats = exporter.exportEntities({left_[0]}, opts);  // only d1 (k1 matches)
    EXPECT_EQ(stats.exported_entities, 1u);

    const auto lines = readLines(opts.output_path);
    ASSERT_EQ(lines.size(), 1u);

    // The SSN should have been redacted.
    EXPECT_EQ(lines[0].find("123-45-6789"), std::string::npos);
}

TEST_F(JoinExporterTest, PIIDetectionFailOnPIIThrows) {
    BaseEntity r_pii;
    r_pii.setPrimaryKey("r_pii");
    r_pii.setField("doc_id", std::string("k1"));
    r_pii.setField("note",   std::string("card: 4111-1111-1111-1111"));

    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "doc_id";
    cfg.output_fields    = {"content", "note"};
    cfg.pii_config.enable_detection  = true;
    cfg.pii_config.enable_redaction  = false;
    cfg.pii_config.fail_on_pii       = true;
    cfg.pii_config.detect_credit_card = true;

    JoinExporter exporter(cfg);
    exporter.setRightCollection({r_pii});

    ExportOptions opts;
    opts.output_path       = outPath("pii_fail.jsonl");
    opts.continue_on_error = false;

    EXPECT_THROW({
        try {
            exporter.exportEntities({left_[0]}, opts);
        } catch (const ExporterException& e) {
            EXPECT_EQ(e.getErrorCode(),
                      errors::ErrorCode::ERR_EXPORT_PII_VIOLATION);
            throw;
        }
    }, ExporterException);
}

// ── AC-8/9: throughput ≥ 50 000 merged docs/sec ──────────────────────────────

TEST_F(JoinExporterTest, Throughput_50kDocsPerSecond) {
    constexpr size_t N = 100'000;

    std::vector<BaseEntity> large_left;
    std::vector<BaseEntity> large_right;
    large_left.reserve(N);
    large_right.reserve(N);

    for (size_t i = 0; i < N; ++i) {
        const std::string k = std::to_string(i);

        BaseEntity l;
        l.setPrimaryKey("l" + k);
        l.setField("_key",    std::string(k));
        l.setField("content", std::string("document content " + k));
        large_left.push_back(l);

        BaseEntity r;
        r.setPrimaryKey("r" + k);
        r.setField("_key",  std::string(k));
        r.setField("label", std::string("label_" + k));
        large_right.push_back(r);
    }

    JoinExportConfig cfg;
    cfg.left_collection  = "large_docs";
    cfg.right_collection = "large_annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "_key";
    cfg.output_fields    = {"content", "label"};

    JoinExporter exporter(cfg);
    exporter.setRightCollection(large_right);

    ExportOptions opts;
    opts.output_path = outPath("throughput.jsonl");

    const auto t0    = std::chrono::steady_clock::now();
    const auto stats = exporter.exportEntities(large_left, opts);
    const auto t1    = std::chrono::steady_clock::now();

    EXPECT_EQ(stats.exported_entities, N);
    EXPECT_EQ(stats.failed_entities,   0u);

    const double elapsed_sec =
        std::chrono::duration<double>(t1 - t0).count();
    const double docs_per_sec = static_cast<double>(N) / elapsed_sec;

    // AC-8: ≥ 50 000 merged docs/sec
    EXPECT_GE(docs_per_sec, 50'000.0)
        << "JoinExporter throughput " << docs_per_sec
        << " docs/s is below the 50 000 docs/s requirement";
}

// ── AC-9: right-side memory budget ≤ 1 GiB ───────────────────────────────────

TEST(JoinExporterMemoryTest, MemoryBudgetDefault1GiB) {
    // Default limit is 1 GiB; confirm it is stored correctly.
    JoinExportConfig cfg;
    EXPECT_EQ(cfg.right_side_memory_limit_bytes, 1ULL * 1024 * 1024 * 1024);
}

TEST(JoinExporterMemoryTest, CustomMemoryBudget) {
    JoinExportConfig cfg;
    cfg.right_side_memory_limit_bytes = 256ULL * 1024 * 1024;
    EXPECT_EQ(cfg.right_side_memory_limit_bytes, 256ULL * 1024 * 1024);
}

// ── Exporter interface ────────────────────────────────────────────────────────

TEST(JoinExporterInterfaceTest, NameAndVersion) {
    JoinExporter exporter;
    EXPECT_EQ(exporter.getName(),    "join_exporter");
    EXPECT_EQ(exporter.getVersion(), "1.0.0");
}

TEST(JoinExporterInterfaceTest, SupportedFormats) {
    JoinExporter exporter;
    const auto fmts = exporter.getSupportedFormats();
    EXPECT_NE(std::find(fmts.begin(), fmts.end(), "jsonl"), fmts.end());
    EXPECT_NE(std::find(fmts.begin(), fmts.end(), "join_jsonl"), fmts.end());
}

TEST(JoinExporterInterfaceTest, MetricsAccessible) {
    JoinExporter exporter;
    EXPECT_NE(exporter.getMetrics(), nullptr);
}

// ── Right collection can be replaced ─────────────────────────────────────────

TEST_F(JoinExporterTest, SetRightCollectionReplacesTable) {
    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "doc_id";
    cfg.output_fields    = {"content", "label"};

    JoinExporter exporter(cfg);

    // First load — all 3 annotations present.
    exporter.setRightCollection(right_);

    ExportOptions opts;
    opts.output_path = outPath("replace1.jsonl");
    auto stats1 = exporter.exportEntities(left_, opts);
    EXPECT_EQ(stats1.exported_entities, 3u);

    // Replace with only 1 annotation.
    exporter.setRightCollection({right_[0]});
    opts.output_path = outPath("replace2.jsonl");
    auto stats2 = exporter.exportEntities(left_, opts);
    EXPECT_EQ(stats2.exported_entities, 1u);
}

// ── Empty collections ─────────────────────────────────────────────────────────

TEST_F(JoinExporterTest, EmptyLeftProducesNoOutput) {
    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "doc_id";

    JoinExporter exporter(cfg);
    exporter.setRightCollection(right_);

    ExportOptions opts;
    opts.output_path = outPath("empty_left.jsonl");
    auto stats = exporter.exportEntities({}, opts);

    EXPECT_EQ(stats.total_entities,    0u);
    EXPECT_EQ(stats.exported_entities, 0u);
}

TEST_F(JoinExporterTest, EmptyRightProducesNoOutput) {
    JoinExportConfig cfg;
    cfg.left_collection  = "docs";
    cfg.right_collection = "annotations";
    cfg.left_key_field   = "_key";
    cfg.right_key_field  = "doc_id";

    JoinExporter exporter(cfg);
    exporter.setRightCollection({});

    ExportOptions opts;
    opts.output_path = outPath("empty_right.jsonl");
    auto stats = exporter.exportEntities(left_, opts);

    EXPECT_EQ(stats.total_entities,    4u);
    EXPECT_EQ(stats.exported_entities, 0u);
    EXPECT_EQ(stats.skipped_entities,  4u);
}
