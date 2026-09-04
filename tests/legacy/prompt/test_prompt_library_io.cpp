/**
 * @file test_prompt_library_io.cpp
 * @brief Unit tests for PromptLibraryIO (v2.0.0).
 *
 * Acceptance criteria:
 *  AC-1   PromptLibraryBundle default-constructs without error.
 *  AC-2   computeChecksum() returns a 16-character hex string for empty templates.
 *  AC-3   computeChecksum() returns a 16-character hex string for non-empty templates.
 *  AC-4   verifyChecksum() returns true when checksum matches.
 *  AC-5   verifyChecksum() returns false when checksum is tampered.
 *  AC-6   exportToJson() returns a non-empty string.
 *  AC-7   exportToJson() embeds checksum in the JSON output.
 *  AC-8   exported JSON contains all required top-level keys.
 *  AC-9   importFromJson() returns nullopt on empty string.
 *  AC-10  importFromJson() returns nullopt on malformed JSON.
 *  AC-11  importFromJson() round-trips name, description, version.
 *  AC-12  importFromJson() round-trips zero templates.
 *  AC-13  importFromJson() round-trips multiple templates (id, name, content, active).
 *  AC-14  importFromJson() preserves template.metadata.
 *  AC-15  exportToYaml() returns a non-empty string.
 *  AC-16  exported YAML contains "name:" and "templates:" keys.
 *  AC-17  importFromYaml() round-trips name, description, version.
 *  AC-18  importFromYaml() round-trips template content.
 *  AC-19  importFromYaml() returns nullopt on empty string.
 *  AC-20  exportToFile() creates a JSON file and returns ExportResult::success.
 *  AC-21  exportToFile() creates a YAML file (ExportFormat::YAML).
 *  AC-22  exportToFile() auto-detects YAML from .yaml extension.
 *  AC-23  importFromFile() reads and parses a JSON file.
 *  AC-24  importFromFile() reads and parses a YAML file.
 *  AC-25  importFromFile() returns success=false for non-existent file.
 *  AC-26  importFromFile() validates checksum; sets checksum_valid=false on mismatch.
 *  AC-27  round-trip JSON: export→import yields identical template count.
 *  AC-28  round-trip YAML: export→import yields identical template count.
 *  AC-29  exportToJson() auto-computes checksum when bundle.checksum is empty.
 *  AC-30  PromptLibraryBundle::fromJson(toJson()) round-trips all fields.
 */

#include <gtest/gtest.h>

#include "prompt_engineering/prompt_library_io.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace themis::prompt_engineering;

// ============================================================================
// Helpers
// ============================================================================

namespace {

PromptManager::PromptTemplate makeTemplate(const std::string& id,
                                            const std::string& name = "T",
                                            const std::string& content = "Hello {x}",
                                            bool active = true) {
    PromptManager::PromptTemplate t;
    t.id      = id;
    t.name    = name;
    t.version = "v1";
    t.content = content;
    t.description = "test template";
    t.active  = active;
    return t;
}

PromptLibraryBundle makeBundle(const std::string& name = "lib",
                                const std::string& version = "1.0.0",
                                std::size_t n_templates = 0) {
    PromptLibraryBundle b;
    b.name        = name;
    b.description = "A test library";
    b.version     = version;
    for (std::size_t i = 0; i < n_templates; ++i) {
        b.templates.push_back(
            makeTemplate("tpl-" + std::to_string(i), "Tpl " + std::to_string(i)));
    }
    return b;
}

// Temp file helper — creates a path in the system temp directory.
std::string tempPath(const std::string& name) {
    return (std::filesystem::temp_directory_path() / name).string();
}

} // anonymous namespace

// ============================================================================
// AC-1  Default construction
// ============================================================================

TEST(PromptLibraryIOTest, DefaultConstruction) {
    EXPECT_NO_THROW(PromptLibraryBundle b);
}

// ============================================================================
// AC-2  computeChecksum() — 16-char hex for empty templates
// ============================================================================

TEST(PromptLibraryIOTest, ChecksumEmptyTemplates) {
    auto b = makeBundle("lib", "1.0");
    const auto cs = PromptLibraryIO::computeChecksum(b);
    EXPECT_EQ(cs.size(), 16u);
    for (char c : cs) {
        EXPECT_TRUE((c >= ('0' && c <= '9') || (c >= 'a' && c <= 'f')));
    }
}

// ============================================================================
// AC-3  computeChecksum() — 16-char hex for non-empty templates
// ============================================================================

TEST(PromptLibraryIOTest, ChecksumNonEmpty) {
    auto b = makeBundle("lib", "1.0", 3);
    const auto cs = PromptLibraryIO::computeChecksum(b);
    EXPECT_EQ(cs.size(), 16u);
}

// ============================================================================
// AC-4  verifyChecksum() — true when matches
// ============================================================================

TEST(PromptLibraryIOTest, VerifyChecksumMatch) {
    auto b = makeBundle("lib", "1.0", 2);
    b.checksum = PromptLibraryIO::computeChecksum(b);
    EXPECT_TRUE(PromptLibraryIO::verifyChecksum(b));
}

// ============================================================================
// AC-5  verifyChecksum() — false when tampered
// ============================================================================

TEST(PromptLibraryIOTest, VerifyChecksumMismatch) {
    auto b = makeBundle("lib", "1.0", 2);
    b.checksum = "0000000000000000"; // wrong
    EXPECT_FALSE(PromptLibraryIO::verifyChecksum(b));
}

// ============================================================================
// AC-6  exportToJson() — non-empty
// ============================================================================

TEST(PromptLibraryIOTest, ExportJsonNonEmpty) {
    auto b = makeBundle("lib", "1.0", 1);
    EXPECT_FALSE(PromptLibraryIO::exportToJson(b).empty());
}

// ============================================================================
// AC-7  exportToJson() — embeds checksum
// ============================================================================

TEST(PromptLibraryIOTest, ExportJsonEmbedsChecksum) {
    auto b = makeBundle("lib", "1.0", 1);
    const auto json_str = PromptLibraryIO::exportToJson(b);
    const auto j = nlohmann::json::parse(json_str);
    EXPECT_TRUE(j.contains("checksum"));
    EXPECT_FALSE(j["checksum"].get<std::string>().empty());
}

// ============================================================================
// AC-8  exported JSON contains required top-level keys
// ============================================================================

TEST(PromptLibraryIOTest, ExportJsonRequiredKeys) {
    auto b = makeBundle("mylib", "2.0.0", 2);
    const auto j = nlohmann::json::parse(PromptLibraryIO::exportToJson(b));
    EXPECT_TRUE(j.contains("name"));
    EXPECT_TRUE(j.contains("description"));
    EXPECT_TRUE(j.contains("version"));
    EXPECT_TRUE(j.contains("format_version"));
    EXPECT_TRUE(j.contains("created_at"));
    EXPECT_TRUE(j.contains("checksum"));
    EXPECT_TRUE(j.contains("templates"));
}

// ============================================================================
// AC-9  importFromJson() — nullopt on empty string
// ============================================================================

TEST(PromptLibraryIOTest, ImportJsonEmptyNullopt) {
    EXPECT_FALSE(PromptLibraryIO::importFromJson("").has_value());
}

// ============================================================================
// AC-10  importFromJson() — nullopt on malformed JSON
// ============================================================================

TEST(PromptLibraryIOTest, ImportJsonMalformedNullopt) {
    EXPECT_FALSE(PromptLibraryIO::importFromJson("{bad json{{").has_value());
}

// ============================================================================
// AC-11  importFromJson() — round-trips name, description, version
// ============================================================================

TEST(PromptLibraryIOTest, ImportJsonMetadataRoundTrip) {
    auto b = makeBundle("my-contracts", "3.1.0");
    b.description = "Legal contract prompts";
    const auto opt = PromptLibraryIO::importFromJson(PromptLibraryIO::exportToJson(b));
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->name,        "my-contracts");
    EXPECT_EQ(opt->version,     "3.1.0");
    EXPECT_EQ(opt->description, "Legal contract prompts");
}

// ============================================================================
// AC-12  importFromJson() — round-trips zero templates
// ============================================================================

TEST(PromptLibraryIOTest, ImportJsonZeroTemplates) {
    auto b = makeBundle("empty-lib", "1.0");
    const auto opt = PromptLibraryIO::importFromJson(PromptLibraryIO::exportToJson(b));
    ASSERT_TRUE(opt.has_value());
    EXPECT_TRUE(opt->templates.empty());
}

// ============================================================================
// AC-13  importFromJson() — round-trips multiple templates
// ============================================================================

TEST(PromptLibraryIOTest, ImportJsonMultipleTemplates) {
    auto b = makeBundle("lib", "1.0", 3);
    b.templates[1].active = false;
    const auto opt = PromptLibraryIO::importFromJson(PromptLibraryIO::exportToJson(b));
    ASSERT_TRUE(opt.has_value());
    ASSERT_EQ(opt->templates.size(), 3u);
    EXPECT_EQ(opt->templates[0].id, "tpl-0");
    EXPECT_EQ(opt->templates[2].id, "tpl-2");
    EXPECT_FALSE(opt->templates[1].active);
}

// ============================================================================
// AC-14  importFromJson() — preserves template.metadata
// ============================================================================

TEST(PromptLibraryIOTest, ImportJsonMetadataPreserved) {
    auto b = makeBundle("lib", "1.0", 1);
    b.templates[0].metadata = {{"experiment_id", "exp-7"}, {"priority", 3}};
    const auto opt = PromptLibraryIO::importFromJson(PromptLibraryIO::exportToJson(b));
    ASSERT_TRUE(opt.has_value());
    ASSERT_FALSE(opt->templates.empty());
    EXPECT_EQ(opt->templates[0].metadata.value("experiment_id", ""), "exp-7");
    EXPECT_EQ(opt->templates[0].metadata.value("priority", 0), 3);
}

// ============================================================================
// AC-15  exportToYaml() — non-empty
// ============================================================================

TEST(PromptLibraryIOTest, ExportYamlNonEmpty) {
    auto b = makeBundle("lib", "1.0", 1);
    EXPECT_FALSE(PromptLibraryIO::exportToYaml(b).empty());
}

// ============================================================================
// AC-16  exported YAML contains "name:" and "templates:"
// ============================================================================

TEST(PromptLibraryIOTest, ExportYamlContainsKeys) {
    auto b = makeBundle("my-lib", "1.0", 2);
    const auto yaml_str = PromptLibraryIO::exportToYaml(b);
    EXPECT_NE(yaml_str.find("name:"), std::string::npos);
    EXPECT_NE(yaml_str.find("templates:"), std::string::npos);
    EXPECT_NE(yaml_str.find("checksum:"), std::string::npos);
}

// ============================================================================
// AC-17  importFromYaml() — round-trips name, description, version
// ============================================================================

TEST(PromptLibraryIOTest, ImportYamlMetadataRoundTrip) {
    auto b = makeBundle("yaml-lib", "4.0.0");
    b.description = "YAML test library";
    const auto opt = PromptLibraryIO::importFromYaml(
        PromptLibraryIO::exportToYaml(b));
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->name,        "yaml-lib");
    EXPECT_EQ(opt->version,     "4.0.0");
    EXPECT_EQ(opt->description, "YAML test library");
}

// ============================================================================
// AC-18  importFromYaml() — round-trips template content
// ============================================================================

TEST(PromptLibraryIOTest, ImportYamlTemplateContent) {
    auto b = makeBundle("lib", "1.0", 2);
    b.templates[0].content = "Summarise: {document}";
    b.templates[1].content = "Translate to {lang}: {text}";
    const auto opt = PromptLibraryIO::importFromYaml(
        PromptLibraryIO::exportToYaml(b));
    ASSERT_TRUE(opt.has_value());
    ASSERT_EQ(opt->templates.size(), 2u);
    EXPECT_EQ(opt->templates[0].content, "Summarise: {document}");
    EXPECT_EQ(opt->templates[1].content, "Translate to {lang}: {text}");
}

// ============================================================================
// AC-19  importFromYaml() — nullopt on empty string
// ============================================================================

TEST(PromptLibraryIOTest, ImportYamlEmptyNullopt) {
    EXPECT_FALSE(PromptLibraryIO::importFromYaml("").has_value());
}

// ============================================================================
// AC-20  exportToFile() — creates JSON file, success=true
// ============================================================================

TEST(PromptLibraryIOTest, ExportToFileJsonSuccess) {
    const auto path = tempPath("test_lib_io_ac20.json");
    auto b = makeBundle("lib", "1.0", 1);
    const auto r = PromptLibraryIO::exportToFile(b, path, ExportFormat::JSON);
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);
}

// ============================================================================
// AC-21  exportToFile() — creates YAML file (ExportFormat::YAML)
// ============================================================================

TEST(PromptLibraryIOTest, ExportToFileYamlSuccess) {
    const auto path = tempPath("test_lib_io_ac21.txt");
    auto b = makeBundle("lib", "1.0", 1);
    const auto r = PromptLibraryIO::exportToFile(b, path, ExportFormat::YAML);
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);
}

// ============================================================================
// AC-22  exportToFile() — auto-detects YAML from .yaml extension
// ============================================================================

TEST(PromptLibraryIOTest, ExportToFileAutoYaml) {
    const auto path = tempPath("test_lib_io_ac22.yaml");
    auto b = makeBundle("lib", "1.0", 1);
    // Default format is JSON, but .yaml extension overrides.
    const auto r = PromptLibraryIO::exportToFile(b, path);
    EXPECT_TRUE(r.success);

    // Verify it is valid YAML by re-importing.
    PromptLibraryBundle loaded;
    const auto ir = PromptLibraryIO::importFromFile(path, loaded);
    EXPECT_TRUE(ir.success);
    std::filesystem::remove(path);
}

// ============================================================================
// AC-23  importFromFile() — reads and parses a JSON file
// ============================================================================

TEST(PromptLibraryIOTest, ImportFromFileJson) {
    const auto path = tempPath("test_lib_io_ac23.json");
    auto b = makeBundle("json-lib", "1.0.0", 2);
    ASSERT_TRUE(PromptLibraryIO::exportToFile(b, path).success);

    PromptLibraryBundle loaded;
    const auto r = PromptLibraryIO::importFromFile(path, loaded);
    EXPECT_TRUE(r.success);
    EXPECT_EQ(loaded.name, "json-lib");
    EXPECT_EQ(r.templates_loaded, 2u);
    std::filesystem::remove(path);
}

// ============================================================================
// AC-24  importFromFile() — reads and parses a YAML file
// ============================================================================

TEST(PromptLibraryIOTest, ImportFromFileYaml) {
    const auto path = tempPath("test_lib_io_ac24.yaml");
    auto b = makeBundle("yaml-lib", "2.0.0", 3);
    ASSERT_TRUE(PromptLibraryIO::exportToFile(
        b, path, ExportFormat::YAML).success);

    PromptLibraryBundle loaded;
    const auto r = PromptLibraryIO::importFromFile(path, loaded);
    EXPECT_TRUE(r.success);
    EXPECT_EQ(loaded.name, "yaml-lib");
    EXPECT_EQ(r.templates_loaded, 3u);
    std::filesystem::remove(path);
}

// ============================================================================
// AC-25  importFromFile() — success=false for non-existent file
// ============================================================================

TEST(PromptLibraryIOTest, ImportFromFileMissing) {
    PromptLibraryBundle loaded;
    const auto r = PromptLibraryIO::importFromFile("/no/such/file.json", loaded);
    EXPECT_FALSE(r.success);
    EXPECT_FALSE(r.error_message.empty());
}

// ============================================================================
// AC-26  importFromFile() — checksum_valid=false when tampered
// ============================================================================

TEST(PromptLibraryIOTest, ImportFromFileChecksumInvalid) {
    const auto path = tempPath("test_lib_io_ac26.json");
    auto b = makeBundle("lib", "1.0", 1);
    // Export valid bundle.
    ASSERT_TRUE(PromptLibraryIO::exportToFile(b, path).success);

    // Tamper the checksum in the file.
    {
        std::ifstream ifs(path);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        auto j = nlohmann::json::parse(content);
        j["checksum"] = "0000000000000000";
        std::ofstream ofs(path);
        ofs << j.dump(2);
    }

    PromptLibraryBundle loaded;
    const auto r = PromptLibraryIO::importFromFile(path, loaded);
    EXPECT_TRUE(r.success);          // still parseable
    EXPECT_FALSE(r.checksum_valid);  // but checksum mismatch
    std::filesystem::remove(path);
}

// ============================================================================
// AC-27  round-trip JSON: template count preserved
// ============================================================================

TEST(PromptLibraryIOTest, RoundTripJsonTemplateCount) {
    auto b = makeBundle("lib", "1.0", 5);
    const auto path = tempPath("test_lib_io_ac27.json");
    ASSERT_TRUE(PromptLibraryIO::exportToFile(b, path).success);
    PromptLibraryBundle loaded;
    ASSERT_TRUE(PromptLibraryIO::importFromFile(path, loaded).success);
    EXPECT_EQ(loaded.templates.size(), 5u);
    std::filesystem::remove(path);
}

// ============================================================================
// AC-28  round-trip YAML: template count preserved
// ============================================================================

TEST(PromptLibraryIOTest, RoundTripYamlTemplateCount) {
    auto b = makeBundle("lib", "1.0", 4);
    const auto path = tempPath("test_lib_io_ac28.yaml");
    ASSERT_TRUE(PromptLibraryIO::exportToFile(b, path, ExportFormat::YAML).success);
    PromptLibraryBundle loaded;
    ASSERT_TRUE(PromptLibraryIO::importFromFile(path, loaded).success);
    EXPECT_EQ(loaded.templates.size(), 4u);
    std::filesystem::remove(path);
}

// ============================================================================
// AC-29  exportToJson() — auto-computes checksum when bundle.checksum is empty
// ============================================================================

TEST(PromptLibraryIOTest, ExportJsonAutoComputesChecksum) {
    auto b = makeBundle("lib", "1.0", 2);
    EXPECT_TRUE(b.checksum.empty());
    const auto json_str = PromptLibraryIO::exportToJson(b);
    const auto j = nlohmann::json::parse(json_str);
    const auto cs = j["checksum"].get<std::string>();
    EXPECT_EQ(cs.size(), 16u);
    EXPECT_NE(cs, "0000000000000000");
}

// ============================================================================
// AC-30  PromptLibraryBundle::fromJson(toJson()) round-trips all fields
// ============================================================================

TEST(PromptLibraryIOTest, BundleJsonRoundTrip) {
    PromptLibraryBundle b;
    b.name           = "rt-lib";
    b.description    = "Round-trip test";
    b.version        = "5.0.0";
    b.format_version = "1.0";
    b.templates.push_back(makeTemplate("id-1", "Name One", "Content one"));
    b.templates.push_back(makeTemplate("id-2", "Name Two", "Content two", false));
    b.checksum = PromptLibraryIO::computeChecksum(b);

    const auto j    = b.toJson();
    const auto back = PromptLibraryBundle::fromJson(j);

    EXPECT_EQ(back.name,           b.name);
    EXPECT_EQ(back.description,    b.description);
    EXPECT_EQ(back.version,        b.version);
    EXPECT_EQ(back.format_version, b.format_version);
    EXPECT_EQ(back.checksum,       b.checksum);
    ASSERT_EQ(back.templates.size(), 2u);
    EXPECT_EQ(back.templates[0].id,      "id-1");
    EXPECT_EQ(back.templates[1].id,      "id-2");
    EXPECT_EQ(back.templates[1].active,  false);
}
