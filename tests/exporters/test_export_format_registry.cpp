#include <gtest/gtest.h>

#include "exporters/export_format_registry.h"
#include "exporters/jsonl_llm_exporter.h"
#include "exporters/parquet_exporter.h"
#include "exporters/arrow_ipc_exporter.h"
#include "exporters/streaming_exporter.h"
#include "exporters/incremental_exporter.h"
#include "exporters/huggingface_exporter.h"
#include "exporters/format_template.h"

#include <filesystem>
#include <fstream>

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
                                  "streaming", "incremental",
                                  "jsonl_alpaca", "jsonl_sharegpt",
                                  "jsonl_chatml", "jsonl_openai_ft"}) {
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

// ── built-in template format shortcuts ───────────────────────────────────────

TEST_F(ExportFormatRegistryTest, BuiltinTemplateAlpacaCreatesCorrectExporter) {
    ExportFormatRegistry::instance().registerBuiltins();
    auto exp = ExportFormatRegistry::instance().createExporter("jsonl_alpaca");
    ASSERT_NE(exp, nullptr);
    EXPECT_EQ(exp->getName(), "jsonl_llm_exporter");
    auto* jexp = dynamic_cast<JSONLLLMExporter*>(exp.get());
    ASSERT_NE(jexp, nullptr);
    EXPECT_EQ(jexp->getConfig().format_template_type, FormatTemplateType::ALPACA);
}

TEST_F(ExportFormatRegistryTest, BuiltinTemplateShareGPTCreatesCorrectExporter) {
    ExportFormatRegistry::instance().registerBuiltins();
    auto exp = ExportFormatRegistry::instance().createExporter("jsonl_sharegpt");
    ASSERT_NE(exp, nullptr);
    auto* jexp = dynamic_cast<JSONLLLMExporter*>(exp.get());
    ASSERT_NE(jexp, nullptr);
    EXPECT_EQ(jexp->getConfig().format_template_type, FormatTemplateType::SHAREGPT);
}

TEST_F(ExportFormatRegistryTest, BuiltinTemplateChatMLCreatesCorrectExporter) {
    ExportFormatRegistry::instance().registerBuiltins();
    auto exp = ExportFormatRegistry::instance().createExporter("jsonl_chatml");
    ASSERT_NE(exp, nullptr);
    auto* jexp = dynamic_cast<JSONLLLMExporter*>(exp.get());
    ASSERT_NE(jexp, nullptr);
    EXPECT_EQ(jexp->getConfig().format_template_type, FormatTemplateType::CHATML);
}

TEST_F(ExportFormatRegistryTest, BuiltinTemplateOpenAIFTCreatesCorrectExporter) {
    ExportFormatRegistry::instance().registerBuiltins();
    auto exp = ExportFormatRegistry::instance().createExporter("jsonl_openai_ft");
    ASSERT_NE(exp, nullptr);
    auto* jexp = dynamic_cast<JSONLLLMExporter*>(exp.get());
    ASSERT_NE(jexp, nullptr);
    EXPECT_EQ(jexp->getConfig().format_template_type, FormatTemplateType::OPENAI_FINETUNING);
}

// ── loadTemplatesFromJson ─────────────────────────────────────────────────────

TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonRegistersNewFormat) {
    const std::string json = R"({
        "templates": [
            {
                "format_key":    "jsonl_custom_alpaca",
                "template_type": "alpaca"
            }
        ]
    })";
    ExportFormatRegistry::instance().loadTemplatesFromJson(json);
    EXPECT_TRUE(ExportFormatRegistry::instance().hasFormat("jsonl_custom_alpaca"));
}

TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonCreatesExporterWithCorrectType) {
    const std::string json = R"({
        "templates": [
            {
                "format_key":    "jsonl_my_chatml",
                "template_type": "chatml"
            }
        ]
    })";
    ExportFormatRegistry::instance().loadTemplatesFromJson(json);
    auto exp = ExportFormatRegistry::instance().createExporter("jsonl_my_chatml");
    ASSERT_NE(exp, nullptr);
    auto* jexp = dynamic_cast<JSONLLLMExporter*>(exp.get());
    ASSERT_NE(jexp, nullptr);
    EXPECT_EQ(jexp->getConfig().format_template_type, FormatTemplateType::CHATML);
}

TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonAppliesFieldMapping) {
    const std::string json = R"({
        "templates": [
            {
                "format_key":    "jsonl_custom_alpaca_mapped",
                "template_type": "alpaca",
                "field_mapping": {
                    "instruction_field": "prompt",
                    "output_field":      "response"
                }
            }
        ]
    })";
    ExportFormatRegistry::instance().loadTemplatesFromJson(json);
    auto exp = ExportFormatRegistry::instance().createExporter("jsonl_custom_alpaca_mapped");
    ASSERT_NE(exp, nullptr);
    auto* jexp = dynamic_cast<JSONLLLMExporter*>(exp.get());
    ASSERT_NE(jexp, nullptr);
    EXPECT_EQ(jexp->getConfig().template_field_mapping.instruction_field, "prompt");
    EXPECT_EQ(jexp->getConfig().template_field_mapping.output_field, "response");
}

TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonMultipleEntries) {
    const std::string json = R"({
        "templates": [
            {"format_key": "my_alpaca",   "template_type": "alpaca"},
            {"format_key": "my_sharegpt", "template_type": "sharegpt"},
            {"format_key": "my_openai",   "template_type": "openai_finetuning"}
        ]
    })";
    ExportFormatRegistry::instance().loadTemplatesFromJson(json);
    EXPECT_TRUE(ExportFormatRegistry::instance().hasFormat("my_alpaca"));
    EXPECT_TRUE(ExportFormatRegistry::instance().hasFormat("my_sharegpt"));
    EXPECT_TRUE(ExportFormatRegistry::instance().hasFormat("my_openai"));
}

TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonThrowsOnMissingTemplatesArray) {
    const std::string json = R"({"not_templates": []})";
    EXPECT_THROW(
        ExportFormatRegistry::instance().loadTemplatesFromJson(json),
        std::invalid_argument
    );
}

TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonThrowsOnMissingFormatKey) {
    const std::string json = R"({
        "templates": [
            {"template_type": "alpaca"}
        ]
    })";
    EXPECT_THROW(
        ExportFormatRegistry::instance().loadTemplatesFromJson(json),
        std::invalid_argument
    );
}

TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonThrowsOnMissingTemplateType) {
    const std::string json = R"({
        "templates": [
            {"format_key": "some_key"}
        ]
    })";
    EXPECT_THROW(
        ExportFormatRegistry::instance().loadTemplatesFromJson(json),
        std::invalid_argument
    );
}

TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonThrowsOnUnknownTemplateType) {
    const std::string json = R"({
        "templates": [
            {"format_key": "bad", "template_type": "nonexistent_type"}
        ]
    })";
    EXPECT_THROW(
        ExportFormatRegistry::instance().loadTemplatesFromJson(json),
        std::invalid_argument
    );
}

TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonThrowsOnInvalidJson) {
    EXPECT_THROW(
        ExportFormatRegistry::instance().loadTemplatesFromJson("not_valid_json{{{"),
        nlohmann::json::parse_error
    );
}

TEST_F(ExportFormatRegistryTest, LoadTemplatesFromJsonIsAtomicOnPartialFailure) {
    // First entry is valid; second entry has an unknown template_type.
    // Neither entry should be registered after the failure (all-or-nothing).
    const std::string json = R"({
        "templates": [
            {"format_key": "good_entry",  "template_type": "alpaca"},
            {"format_key": "bad_entry",   "template_type": "nonexistent_type"}
        ]
    })";
    EXPECT_THROW(
        ExportFormatRegistry::instance().loadTemplatesFromJson(json),
        std::invalid_argument
    );
    // The valid first entry must NOT have been registered.
    EXPECT_FALSE(ExportFormatRegistry::instance().hasFormat("good_entry"));
}

// ── loadTemplatesFromConfig ───────────────────────────────────────────────────

TEST_F(ExportFormatRegistryTest, LoadTemplatesFromConfigRegistersFormats) {
    // Write a temporary JSON config file
    const auto tmp = std::filesystem::temp_directory_path() / "themis_test_registry_config.json";
    {
        std::ofstream f(tmp);
        f << R"({
            "templates": [
                {"format_key": "file_alpaca",   "template_type": "alpaca"},
                {"format_key": "file_chatml",    "template_type": "chatml"}
            ]
        })";
    }
    ExportFormatRegistry::instance().loadTemplatesFromConfig(tmp.string());
    EXPECT_TRUE(ExportFormatRegistry::instance().hasFormat("file_alpaca"));
    EXPECT_TRUE(ExportFormatRegistry::instance().hasFormat("file_chatml"));
    std::filesystem::remove(tmp);
}

TEST_F(ExportFormatRegistryTest, LoadTemplatesFromConfigThrowsOnMissingFile) {
    EXPECT_THROW(
        ExportFormatRegistry::instance().loadTemplatesFromConfig("/nonexistent/path/config.json"),
        std::runtime_error
    );
}

