/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_export_format_registry.cpp                    ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-10                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     145                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>

#include "exporters/export_format_registry.h"
#include "exporters/jsonl_llm_exporter.h"
#include "exporters/parquet_exporter.h"
#include "exporters/arrow_ipc_exporter.h"
#include "exporters/streaming_exporter.h"
#include "exporters/incremental_exporter.h"
#include "exporters/huggingface_exporter.h"

using namespace themis::exporters;

class ExportFormatRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Each test starts from a clean registry state.
        ExportFormatRegistry::instance().clear();
    }
    void TearDown() override {
        ExportFormatRegistry::instance().clear();
    }
};

// ── registerBuiltins ─────────────────────────────────────────────────────────

TEST_F(ExportFormatRegistryTest, RegisterBuiltinsPopulatesExpectedFormats) {
    ExportFormatRegistry::instance().registerBuiltins();
    const auto keys = ExportFormatRegistry::instance().registeredFormats();
    EXPECT_FALSE(keys.empty());
    for (const auto& expected : {"jsonl", "llm_jsonl", "parquet", "arrow",
                                  "arrow_stream", "huggingface", "hf_datasets",
                                  "streaming", "incremental"}) {
        EXPECT_TRUE(ExportFormatRegistry::instance().hasFormat(expected))
            << "Missing format: " << expected;
    }
}

TEST_F(ExportFormatRegistryTest, RegisterBuiltinsIsIdempotent) {
    ExportFormatRegistry::instance().registerBuiltins();
    const size_t count1 = ExportFormatRegistry::instance().registeredFormats().size();
    ExportFormatRegistry::instance().registerBuiltins();
    const size_t count2 = ExportFormatRegistry::instance().registeredFormats().size();
    EXPECT_EQ(count1, count2);
}

// ── createExporter ───────────────────────────────────────────────────────────

TEST_F(ExportFormatRegistryTest, CreateExporterJsonlReturnsNonNull) {
    ExportFormatRegistry::instance().registerBuiltins();
    auto exp = ExportFormatRegistry::instance().createExporter("jsonl");
    ASSERT_NE(exp, nullptr);
    EXPECT_EQ(exp->getName(), "jsonl_llm_exporter");
}

TEST_F(ExportFormatRegistryTest, CreateExporterParquetReturnsNonNull) {
    ExportFormatRegistry::instance().registerBuiltins();
    auto exp = ExportFormatRegistry::instance().createExporter("parquet");
    ASSERT_NE(exp, nullptr);
    EXPECT_FALSE(exp->getSupportedFormats().empty());
}

TEST_F(ExportFormatRegistryTest, CreateExporterArrowFileReturnsNonNull) {
    ExportFormatRegistry::instance().registerBuiltins();
    auto exp = ExportFormatRegistry::instance().createExporter("arrow");
    ASSERT_NE(exp, nullptr);
}

TEST_F(ExportFormatRegistryTest, CreateExporterArrowStreamReturnsNonNull) {
    ExportFormatRegistry::instance().registerBuiltins();
    auto exp = ExportFormatRegistry::instance().createExporter("arrow_stream");
    ASSERT_NE(exp, nullptr);
}

TEST_F(ExportFormatRegistryTest, CreateExporterHuggingFaceReturnsNonNull) {
    ExportFormatRegistry::instance().registerBuiltins();
    auto exp = ExportFormatRegistry::instance().createExporter("huggingface");
    ASSERT_NE(exp, nullptr);
    EXPECT_EQ(exp->getName(), "huggingface_exporter");
}

TEST_F(ExportFormatRegistryTest, CreateExporterStreamingReturnsNonNull) {
    ExportFormatRegistry::instance().registerBuiltins();
    auto exp = ExportFormatRegistry::instance().createExporter("streaming");
    ASSERT_NE(exp, nullptr);
}

TEST_F(ExportFormatRegistryTest, CreateExporterIncrementalReturnsNonNull) {
    ExportFormatRegistry::instance().registerBuiltins();
    auto exp = ExportFormatRegistry::instance().createExporter("incremental");
    ASSERT_NE(exp, nullptr);
}

TEST_F(ExportFormatRegistryTest, CreateExporterUnknownFormatThrows) {
    ExportFormatRegistry::instance().registerBuiltins();
    EXPECT_THROW(
        ExportFormatRegistry::instance().createExporter("unknown_xyz"),
        std::invalid_argument
    );
}

// ── hasFormat ────────────────────────────────────────────────────────────────

TEST_F(ExportFormatRegistryTest, HasFormatReturnsFalseBeforeRegister) {
    EXPECT_FALSE(ExportFormatRegistry::instance().hasFormat("jsonl"));
}

TEST_F(ExportFormatRegistryTest, HasFormatReturnsTrueAfterRegister) {
    ExportFormatRegistry::instance().registerFormat(
        "custom_fmt",
        []() -> std::unique_ptr<IExporter> { return std::make_unique<JSONLLLMExporter>(); }
    );
    EXPECT_TRUE(ExportFormatRegistry::instance().hasFormat("custom_fmt"));
}

// ── registerFormat (override) ────────────────────────────────────────────────

TEST_F(ExportFormatRegistryTest, RegisterFormatOverridesExisting) {
    ExportFormatRegistry::instance().registerBuiltins();
    // Override "jsonl" with a streaming exporter
    ExportFormatRegistry::instance().registerFormat(
        "jsonl",
        []() -> std::unique_ptr<IExporter> { return std::make_unique<StreamingExporter>(); }
    );
    auto exp = ExportFormatRegistry::instance().createExporter("jsonl");
    ASSERT_NE(exp, nullptr);
    EXPECT_EQ(exp->getName(), "streaming_exporter");
}

// ── registeredFormats ────────────────────────────────────────────────────────

TEST_F(ExportFormatRegistryTest, RegisteredFormatsIsSorted) {
    ExportFormatRegistry::instance().registerBuiltins();
    const auto keys = ExportFormatRegistry::instance().registeredFormats();
    EXPECT_TRUE(std::is_sorted(keys.begin(), keys.end()));
}

// ── clear ────────────────────────────────────────────────────────────────────

TEST_F(ExportFormatRegistryTest, ClearRemovesAllFormats) {
    ExportFormatRegistry::instance().registerBuiltins();
    ExportFormatRegistry::instance().clear();
    EXPECT_TRUE(ExportFormatRegistry::instance().registeredFormats().empty());
    EXPECT_FALSE(ExportFormatRegistry::instance().hasFormat("jsonl"));
}
