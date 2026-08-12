#include <gtest/gtest.h>
#include "exporters/huggingface_exporter.h"
#include "exporters/exporter_errors.h"
#include "storage/base_entity.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <ctime>

using namespace themis::exporters;
using namespace themis;
using json = nlohmann::json;

class HuggingFaceExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto temp_base = std::filesystem::temp_directory_path();
        test_dir_ = (temp_base / ("themis_hf_test_" + std::to_string(std::time(nullptr)))).string();
        std::filesystem::create_directories(test_dir_);
        createTestEntities();
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    void createTestEntities() {
        for (int i = 0; i < 5; i++) {
            BaseEntity entity;
            entity.setPrimaryKey("entity_" + std::to_string(i));
            entity.setField("instruction", "Instruction " + std::to_string(i));
            entity.setField("input",       "Input " + std::to_string(i));
            entity.setField("output",      "Output " + std::to_string(i));
            // Keep aliases for JSONLLLMExporter default mapping (question/context/answer).
            entity.setField("question",    "Instruction " + std::to_string(i));
            entity.setField("context",     "Input " + std::to_string(i));
            entity.setField("answer",      "Output " + std::to_string(i));
            entity.setField("score",       static_cast<double>(i) * 0.2);
            test_entities_.push_back(entity);
        }
    }

    std::string readFile(const std::string& path) {
        std::ifstream f(path);
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    std::vector<std::string> readLines(const std::string& path) {
        std::vector<std::string> lines;
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty()) lines.push_back(line);
        }
        return lines;
    }

    std::string test_dir_;
    std::vector<BaseEntity> test_entities_;
};

// ---------------------------------------------------------------------------
// Directory structure tests
// ---------------------------------------------------------------------------

TEST_F(HuggingFaceExporterTest, CreatesDatasetDirectory) {
    HuggingFaceExporterConfig config;
    config.dataset_name = "test_dataset";
    config.jsonl_config.quality.min_text_length = 0;
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/my_dataset";

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_GT(stats.exported_entities, 0);
    EXPECT_EQ(stats.failed_entities, 0);

    // Check root directory exists
    EXPECT_TRUE(std::filesystem::exists(options.output_path));

    // Check data subdirectory exists
    EXPECT_TRUE(std::filesystem::exists(options.output_path + "/data"));
}

TEST_F(HuggingFaceExporterTest, CreatesDatasetInfoJson) {
    HuggingFaceExporterConfig config;
    config.dataset_name = "test_dataset";
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/info_test";

    auto stats = exporter.exportEntities(test_entities_, options);

    const std::string info_path = options.output_path + "/dataset_info.json";
    EXPECT_TRUE(std::filesystem::exists(info_path));

    // Validate JSON structure
    auto info_content = readFile(info_path);
    EXPECT_FALSE(info_content.empty());

    auto j = json::parse(info_content);
    EXPECT_TRUE(j.contains("features"));
    EXPECT_TRUE(j.contains("splits"));
    EXPECT_TRUE(j.contains("download_size"));
    EXPECT_TRUE(j.contains("dataset_size"));
    EXPECT_TRUE(j.contains("builder_name"));
    EXPECT_EQ(j["builder_name"], "json");
}

TEST_F(HuggingFaceExporterTest, DatasetInfoContainsSplitInfo) {
    HuggingFaceExporterConfig config;
    config.dataset_name = "my_test";
    config.split_name   = "train";
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/split_test";

    auto stats = exporter.exportEntities(test_entities_, options);

    auto j = json::parse(readFile(options.output_path + "/dataset_info.json"));
    ASSERT_TRUE(j["splits"].contains("train"));
    EXPECT_EQ(j["splits"]["train"]["name"], "train");
    EXPECT_EQ(j["splits"]["train"]["num_examples"], stats.exported_entities);
}

TEST_F(HuggingFaceExporterTest, CustomSplitName) {
    HuggingFaceExporterConfig config;
    config.split_name = "validation";
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/validation_test";

    exporter.exportEntities(test_entities_, options);

    // Data file should use the split name
    EXPECT_TRUE(std::filesystem::exists(
        options.output_path + "/data/validation-00000-of-00001.jsonl"
    ));

    auto j = json::parse(readFile(options.output_path + "/dataset_info.json"));
    EXPECT_TRUE(j["splits"].contains("validation"));
}

TEST_F(HuggingFaceExporterTest, DefaultSplitIsTrainWhenEmpty) {
    HuggingFaceExporterConfig config;
    config.split_name = "";  // empty → default to "train"
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/default_split";

    exporter.exportEntities(test_entities_, options);

    EXPECT_TRUE(std::filesystem::exists(
        options.output_path + "/data/train-00000-of-00001.jsonl"
    ));

    auto j = json::parse(readFile(options.output_path + "/dataset_info.json"));
    EXPECT_TRUE(j["splits"].contains("train"));
}

// ---------------------------------------------------------------------------
// JSONL data file tests
// ---------------------------------------------------------------------------

TEST_F(HuggingFaceExporterTest, DataFileContainsValidJsonl) {
    HuggingFaceExporterConfig config;
    config.jsonl_config.quality.min_text_length = 0;
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/jsonl_test";

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_GT(stats.exported_entities, 0);

    auto lines = readLines(options.output_path + "/data/train-00000-of-00001.jsonl");
    EXPECT_GT(lines.size(), 0);

    for (const auto& line : lines) {
        EXPECT_NO_THROW({
            auto parsed = json::parse(line);
            static_cast<void>(parsed);
        });
    }
}

// ---------------------------------------------------------------------------
// Feature inference tests
// ---------------------------------------------------------------------------

TEST_F(HuggingFaceExporterTest, InfersFeatureTypesFromEntities) {
    HuggingFaceExporterConfig config;
    config.infer_features = true;
    config.features.clear();  // No explicit features
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/infer_features";

    static_cast<void>(exporter.exportEntities(test_entities_, options));

    auto j = json::parse(readFile(options.output_path + "/dataset_info.json"));
    EXPECT_TRUE(j.contains("features"));
    EXPECT_GT(j["features"].size(), 0);

    // "instruction", "input", "output" should be string features
    ASSERT_TRUE(j["features"].contains("instruction"));
    EXPECT_EQ(j["features"]["instruction"]["dtype"], "string");
    EXPECT_EQ(j["features"]["instruction"]["_type"], "Value");
}

TEST_F(HuggingFaceExporterTest, UsesExplicitFeaturesWhenProvided) {
    HuggingFaceExporterConfig config;
    config.features = {
        {"instruction", "string", "Value"},
        {"output",      "string", "Value"},
        {"score",       "float64", "Value"}
    };
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/explicit_features";

    exporter.exportEntities(test_entities_, options);

    auto j = json::parse(readFile(options.output_path + "/dataset_info.json"));
    ASSERT_TRUE(j["features"].contains("instruction"));
    ASSERT_TRUE(j["features"].contains("output"));
    ASSERT_TRUE(j["features"].contains("score"));
    EXPECT_EQ(j["features"]["score"]["dtype"], "float64");
}

// ---------------------------------------------------------------------------
// Dataset card (README.md) tests
// ---------------------------------------------------------------------------

TEST_F(HuggingFaceExporterTest, GeneratesDatasetCard) {
    HuggingFaceExporterConfig config;
    config.generate_dataset_card = true;
    config.dataset_name = "my_dataset";
    config.license      = "mit";
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/card_test";

    exporter.exportEntities(test_entities_, options);

    const std::string readme = options.output_path + "/README.md";
    EXPECT_TRUE(std::filesystem::exists(readme));

    auto content = readFile(readme);
    EXPECT_FALSE(content.empty());

    // Should have YAML front matter
    EXPECT_NE(content.find("---"), std::string::npos);

    // Should contain the license
    EXPECT_NE(content.find("mit"), std::string::npos);
}

TEST_F(HuggingFaceExporterTest, DatasetCardSkippedWhenDisabled) {
    HuggingFaceExporterConfig config;
    config.generate_dataset_card = false;
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/no_card";

    exporter.exportEntities(test_entities_, options);

    EXPECT_FALSE(std::filesystem::exists(options.output_path + "/README.md"));
}

TEST_F(HuggingFaceExporterTest, DatasetCardUsesCustomTemplate) {
    const std::string custom_card = "# Custom card\nMy custom content.\n";
    HuggingFaceExporterConfig config;
    config.generate_dataset_card  = true;
    config.dataset_card_template  = custom_card;
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/custom_card";

    exporter.exportEntities(test_entities_, options);

    auto content = readFile(options.output_path + "/README.md");
    EXPECT_EQ(content, custom_card);
}

TEST_F(HuggingFaceExporterTest, DatasetCardContainsSplitSection) {
    HuggingFaceExporterConfig config;
    config.generate_dataset_card = true;
    config.split_name = "train";
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/card_split";

    exporter.exportEntities(test_entities_, options);

    auto content = readFile(options.output_path + "/README.md");
    EXPECT_NE(content.find("train"), std::string::npos);
    EXPECT_NE(content.find("dataset_info"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Metadata in dataset_info.json
// ---------------------------------------------------------------------------

TEST_F(HuggingFaceExporterTest, DatasetInfoContainsMetadata) {
    HuggingFaceExporterConfig config;
    config.dataset_name = "metadata_test";
    config.description  = "A test dataset";
    config.license      = "apache-2.0";
    config.homepage     = "https://example.com";
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/metadata_test";

    exporter.exportEntities(test_entities_, options);

    auto j = json::parse(readFile(options.output_path + "/dataset_info.json"));
    EXPECT_EQ(j["description"], "A test dataset");
    EXPECT_EQ(j["license"],     "apache-2.0");
    EXPECT_EQ(j["homepage"],    "https://example.com");
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

TEST_F(HuggingFaceExporterTest, ReturnsErrorForEmptyOutputPath) {
    HuggingFaceExporter exporter;

    ExportOptions options;
    options.output_path = "";  // Empty path

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_GT(stats.errors.size(), 0);
    EXPECT_EQ(stats.exported_entities, 0);
}

// ---------------------------------------------------------------------------
// API surface tests
// ---------------------------------------------------------------------------

TEST_F(HuggingFaceExporterTest, SupportedFormats) {
    HuggingFaceExporter exporter;
    auto formats = exporter.getSupportedFormats();
    EXPECT_NE(std::find(formats.begin(), formats.end(), "huggingface"), formats.end());
    EXPECT_NE(std::find(formats.begin(), formats.end(), "hf_datasets"), formats.end());
}

TEST_F(HuggingFaceExporterTest, ExporterNameAndVersion) {
    HuggingFaceExporter exporter;
    EXPECT_EQ(exporter.getName(),    "huggingface_exporter");
    EXPECT_EQ(exporter.getVersion(), "1.0.0");
}

TEST_F(HuggingFaceExporterTest, GetAndSetConfig) {
    HuggingFaceExporterConfig config;
    config.dataset_name = "original";
    HuggingFaceExporter exporter(config);

    EXPECT_EQ(exporter.getConfig().dataset_name, "original");

    HuggingFaceExporterConfig new_config;
    new_config.dataset_name = "updated";
    exporter.setConfig(new_config);
    EXPECT_EQ(exporter.getConfig().dataset_name, "updated");
}

TEST_F(HuggingFaceExporterTest, MetricsAttachedToStats) {
    HuggingFaceExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/metrics_test";

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_NE(stats.metrics, nullptr);
    EXPECT_NE(exporter.getMetrics(), nullptr);
}

TEST_F(HuggingFaceExporterTest, ExportStatsToJson) {
    HuggingFaceExporter exporter;

    ExportOptions options;
    options.output_path = test_dir_ + "/stats_json";

    auto stats = exporter.exportEntities(test_entities_, options);

    auto j_str = stats.toJson();
    EXPECT_FALSE(j_str.empty());

    auto j = json::parse(j_str);
    EXPECT_TRUE(j.contains("exported_entities"));
    EXPECT_TRUE(j.contains("bytes_written"));
}

// ---------------------------------------------------------------------------
// generateDatasetInfoJson standalone
// ---------------------------------------------------------------------------

TEST_F(HuggingFaceExporterTest, GenerateDatasetInfoJsonStandalone) {
    HuggingFaceExporterConfig config;
    config.dataset_name = "standalone";
    config.description  = "Standalone test";
    config.license      = "mit";
    config.split_name   = "train";
    config.features     = {{"text", "string", "Value"}};
    HuggingFaceExporter exporter(config);

    ExportStats stats;
    stats.exported_entities = 42;
    stats.bytes_written     = 4200;

    auto info_json = exporter.generateDatasetInfoJson(stats, "standalone", 4200);
    auto j = json::parse(info_json);

    EXPECT_EQ(j["description"], "Standalone test");
    EXPECT_EQ(j["license"],     "mit");
    EXPECT_TRUE(j["features"].contains("text"));
    EXPECT_EQ(j["splits"]["train"]["num_examples"], 42);
    EXPECT_EQ(j["splits"]["train"]["num_bytes"],    4200);
    EXPECT_EQ(j["splits"]["train"]["dataset_name"], "standalone");
}

// ---------------------------------------------------------------------------
// Audit fix tests
// ---------------------------------------------------------------------------

TEST_F(HuggingFaceExporterTest, DatasetCardYamlEscapesSpecialCharsInLicense) {
    HuggingFaceExporterConfig config;
    config.generate_dataset_card = true;
    // A license value containing characters that would break unquoted YAML
    config.license      = "mit: special \"value\"";
    config.dataset_name = "escape_test";
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/yaml_escape";

    exporter.exportEntities(test_entities_, options);

    auto content = readFile(options.output_path + "/README.md");
    EXPECT_FALSE(content.empty());

    // The raw injection text must NOT appear unquoted in the front matter
    EXPECT_EQ(content.find("license: mit: special"), std::string::npos);

    // The YAML block must still start and end correctly
    EXPECT_EQ(content.substr(0, 3), "---");
    auto second_fence = content.find("---\n\n");
    EXPECT_NE(second_fence, std::string::npos);
}

TEST_F(HuggingFaceExporterTest, DatasetCardYamlEscapesNewlineInTag) {
    HuggingFaceExporterConfig config;
    config.generate_dataset_card = true;
    config.tags = {"good_tag", "tag\nwith_newline"};
    HuggingFaceExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/yaml_newline";

    exporter.exportEntities(test_entities_, options);

    auto content = readFile(options.output_path + "/README.md");
    // The raw newline must not appear inside the YAML front matter block
    // (it is encoded as \n in the quoted string)
    const auto first_fence_end  = content.find('\n');          // end of first "---"
    const auto second_fence_pos = content.find("---\n\n");     // closing "---"
    ASSERT_NE(second_fence_pos, std::string::npos);

    const std::string yaml_block = content.substr(0, second_fence_pos);
    // Ensure the raw tag text "tag\nwith_newline" does NOT appear as a bare newline
    EXPECT_EQ(yaml_block.find("with_newline\n"), std::string::npos);
    (void)first_fence_end;
}

TEST_F(HuggingFaceExporterTest, SetConfigClearsInferredFeatures) {
    HuggingFaceExporterConfig config1;
    config1.infer_features = true;
    HuggingFaceExporter exporter(config1);

    // First export populates inferred_features_
    ExportOptions options;
    options.output_path = test_dir_ + "/setconfig_clear_1";
    exporter.exportEntities(test_entities_, options);

    // The dataset card for export1 should contain the inferred fields
    auto card1 = exporter.generateDatasetCard();
    EXPECT_NE(card1.find("instruction"), std::string::npos);

    // setConfig replaces config AND clears inferred_features_
    HuggingFaceExporterConfig config2;
    config2.infer_features = false;
    config2.features = {};  // No features, inference disabled
    exporter.setConfig(config2);

    // generateDatasetCard must NOT use stale inferred features
    auto card2 = exporter.generateDatasetCard();
    // With no configured features and inference disabled, data fields section should be empty
    EXPECT_EQ(card2.find("- **instruction**"), std::string::npos);
}

TEST_F(HuggingFaceExporterTest, ExportStatsHaveDurationOnEmptyPath) {
    HuggingFaceExporter exporter;

    ExportOptions options;
    options.output_path = "";  // triggers early return

    auto stats = exporter.exportEntities(test_entities_, options);

    EXPECT_GT(stats.errors.size(), 0);
    // duration must be set even in error paths (consistent stats)
    EXPECT_GE(stats.duration.count(), 0);
}

