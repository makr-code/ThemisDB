/*
 * test_config_migration_scanner.cpp
 *
 * Unit tests for the config_migration_scanner logic (shouldScanFile,
 * scanFile, fixFile) exposed via config/config_migration_scanner_impl.h.
 *
 * Tests create temporary file trees in /tmp, exercise the scanning and
 * fix logic, and verify the expected results.
 */

#include <gtest/gtest.h>
#include "config/config_migration_scanner_impl.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    ofs << content;
}

static std::string readFile(const fs::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
}

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class ConfigMigrationScannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "themisdb_scanner_test";
        fs::remove_all(test_dir_);
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        fs::remove_all(test_dir_);
    }

    fs::path test_dir_;
};

// ─────────────────────────────────────────────────────────────────────────────
// shouldScanFile tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConfigMigrationScannerTest, ShouldScanYaml) {
    EXPECT_TRUE(cms::shouldScanFile("some/path/config.yaml"));
    EXPECT_TRUE(cms::shouldScanFile("some/path/config.yml"));
}

TEST_F(ConfigMigrationScannerTest, ShouldScanJson) {
    EXPECT_TRUE(cms::shouldScanFile("settings.json"));
}

TEST_F(ConfigMigrationScannerTest, ShouldScanToml) {
    EXPECT_TRUE(cms::shouldScanFile("Cargo.toml"));
}

TEST_F(ConfigMigrationScannerTest, ShouldScanIni) {
    EXPECT_TRUE(cms::shouldScanFile("app.ini"));
}

TEST_F(ConfigMigrationScannerTest, ShouldScanEnv) {
    EXPECT_TRUE(cms::shouldScanFile(".env"));
}

TEST_F(ConfigMigrationScannerTest, ShouldNotScanCppSource) {
    EXPECT_FALSE(cms::shouldScanFile("src/main.cpp"));
}

TEST_F(ConfigMigrationScannerTest, ShouldNotScanHeader) {
    EXPECT_FALSE(cms::shouldScanFile("include/config.h"));
}

TEST_F(ConfigMigrationScannerTest, ShouldNotScanMarkdown) {
    EXPECT_FALSE(cms::shouldScanFile("README.md"));
}

TEST_F(ConfigMigrationScannerTest, ShouldScanCaseInsensitiveExtension) {
    EXPECT_TRUE(cms::shouldScanFile("config.YAML"));
    EXPECT_TRUE(cms::shouldScanFile("config.YML"));
    EXPECT_TRUE(cms::shouldScanFile("data.JSON"));
}

// ─────────────────────────────────────────────────────────────────────────────
// scanFile tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConfigMigrationScannerTest, ScanFileNoMatches) {
    auto f = test_dir_ / "no_legacy.yaml";
    writeFile(f, "database:\n  host: localhost\n  port: 5432\n");

    auto matches = cms::scanFile(f);
    EXPECT_TRUE(matches.empty());
}

TEST_F(ConfigMigrationScannerTest, ScanFileFindsKnownLegacyPath) {
    // "config/lora_training_config.yaml" is a known legacy path
    auto f = test_dir_ / "deploy.yaml";
    writeFile(f, "config_path: config/lora_training_config.yaml\n");

    auto matches = cms::scanFile(f);
    ASSERT_FALSE(matches.empty());

    bool found = false;
    for (const auto& m : matches) {
        if (m.legacy_path == "config/lora_training_config.yaml") {
            found = true;
            EXPECT_EQ(m.new_path, "config/ai_ml/lora_training_config.yaml");
            EXPECT_EQ(m.line_number, 1);
            EXPECT_FALSE(m.category.empty());
            EXPECT_FALSE(m.deprecated_date.empty());
            EXPECT_FALSE(m.removal_date.empty());
        }
    }
    EXPECT_TRUE(found) << "Expected to find config/lora_training_config.yaml in scan results";
}

TEST_F(ConfigMigrationScannerTest, ScanFileReportsCorrectLineNumbers) {
    auto f = test_dir_ / "multi.yaml";
    writeFile(f,
        "line1: value\n"
        "line2: value\n"
        "path: config/pii_patterns.yaml\n"
        "line4: value\n");

    auto matches = cms::scanFile(f);
    bool found = false;
    for (const auto& m : matches) {
        if (m.legacy_path == "config/pii_patterns.yaml") {
            EXPECT_EQ(m.line_number, 3);
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ConfigMigrationScannerTest, ScanFileMultipleLegacyPathsInOneFile) {
    auto f = test_dir_ / "multi_legacy.yaml";
    writeFile(f,
        "a: config/lora_training_config.yaml\n"
        "b: config/pii_patterns.yaml\n");

    auto matches = cms::scanFile(f);

    std::set<std::string> found_paths;
    for (const auto& m : matches) {
        found_paths.insert(m.legacy_path);
    }
    EXPECT_TRUE(found_paths.count("config/lora_training_config.yaml") > 0);
    EXPECT_TRUE(found_paths.count("config/pii_patterns.yaml") > 0);
}

TEST_F(ConfigMigrationScannerTest, ScanFilePopulatesMigrationGuideUrl) {
    auto f = test_dir_ / "guide_check.yaml";
    writeFile(f, "config_path: config/lora_training_config.yaml\n");

    auto matches = cms::scanFile(f);
    bool found = false;
    for (const auto& m : matches) {
        if (m.legacy_path == "config/lora_training_config.yaml") {
            EXPECT_FALSE(m.migration_guide_url.empty());
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ConfigMigrationScannerTest, ScanFileUnreadableReturnsEmpty) {
    auto f = test_dir_ / "nonexistent.yaml";
    // File does not exist
    auto matches = cms::scanFile(f);
    EXPECT_TRUE(matches.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// fixFile tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConfigMigrationScannerTest, FixFileNoMatchesReturnsTrueNoChange) {
    auto f = test_dir_ / "unchanged.yaml";
    const std::string content = "key: value\n";
    writeFile(f, content);

    auto matches = cms::scanFile(f);  // Should be empty
    bool ok = cms::fixFile(f, matches, /*dry_run=*/false);

    EXPECT_TRUE(ok);
    EXPECT_EQ(readFile(f), content);  // File unchanged
    EXPECT_FALSE(fs::exists(fs::path(f.string() + ".bak")));
}

TEST_F(ConfigMigrationScannerTest, FixFileDryRunDoesNotModifyFile) {
    auto f = test_dir_ / "dryrun.yaml";
    const std::string content = "path: config/lora_training_config.yaml\n";
    writeFile(f, content);

    auto matches = cms::scanFile(f);
    ASSERT_FALSE(matches.empty());

    bool ok = cms::fixFile(f, matches, /*dry_run=*/true);

    EXPECT_TRUE(ok);
    EXPECT_EQ(readFile(f), content);                              // File unchanged
    EXPECT_FALSE(fs::exists(fs::path(f.string() + ".bak")));      // No backup created
}

TEST_F(ConfigMigrationScannerTest, FixFileReplacesLegacyPath) {
    auto f = test_dir_ / "fix.yaml";
    writeFile(f, "path: config/lora_training_config.yaml\n");

    auto matches = cms::scanFile(f);
    ASSERT_FALSE(matches.empty());

    bool ok = cms::fixFile(f, matches, /*dry_run=*/false);

    EXPECT_TRUE(ok);
    std::string updated = readFile(f);
    EXPECT_EQ(updated.find("config/lora_training_config.yaml"), std::string::npos)
        << "Legacy path should have been replaced";
    EXPECT_NE(updated.find("config/ai_ml/lora_training_config.yaml"), std::string::npos)
        << "New path should appear in updated file";
}

TEST_F(ConfigMigrationScannerTest, FixFileCreatesBackup) {
    auto f = test_dir_ / "backup_test.yaml";
    const std::string original = "path: config/lora_training_config.yaml\n";
    writeFile(f, original);

    auto matches = cms::scanFile(f);
    ASSERT_FALSE(matches.empty());

    bool ok = cms::fixFile(f, matches, /*dry_run=*/false);

    EXPECT_TRUE(ok);
    fs::path backup = fs::path(f.string() + ".bak");
    ASSERT_TRUE(fs::exists(backup)) << "Backup file should have been created";
    EXPECT_EQ(readFile(backup), original) << "Backup should contain original content";
}

TEST_F(ConfigMigrationScannerTest, FixFileReplacesAllOccurrences) {
    auto f = test_dir_ / "multi_occur.yaml";
    writeFile(f,
        "a: config/pii_patterns.yaml\n"
        "b: config/pii_patterns.yaml\n"
        "c: config/pii_patterns.yaml\n");

    auto matches = cms::scanFile(f);
    ASSERT_FALSE(matches.empty());

    bool ok = cms::fixFile(f, matches, /*dry_run=*/false);
    EXPECT_TRUE(ok);

    std::string updated = readFile(f);
    EXPECT_EQ(updated.find("config/pii_patterns.yaml"), std::string::npos)
        << "All legacy occurrences should be replaced";
}

TEST_F(ConfigMigrationScannerTest, FixFileReplacesMultipleDifferentLegacyPaths) {
    auto f = test_dir_ / "multi_path.yaml";
    writeFile(f,
        "x: config/lora_training_config.yaml\n"
        "y: config/pii_patterns.yaml\n");

    auto matches = cms::scanFile(f);
    ASSERT_GE(matches.size(), 2u);

    bool ok = cms::fixFile(f, matches, /*dry_run=*/false);
    EXPECT_TRUE(ok);

    std::string updated = readFile(f);
    EXPECT_EQ(updated.find("config/lora_training_config.yaml"), std::string::npos);
    EXPECT_EQ(updated.find("config/pii_patterns.yaml"), std::string::npos);
    EXPECT_NE(updated.find("config/ai_ml/lora_training_config.yaml"), std::string::npos);
    EXPECT_NE(updated.find("config/security/pii_patterns.yaml"), std::string::npos);
}

TEST_F(ConfigMigrationScannerTest, FixFileIdempotent) {
    // After fix is applied, scanning again should find no legacy paths
    auto f = test_dir_ / "idempotent.yaml";
    writeFile(f, "path: config/lora_training_config.yaml\n");

    auto matches1 = cms::scanFile(f);
    ASSERT_FALSE(matches1.empty());
    cms::fixFile(f, matches1, /*dry_run=*/false);

    // Second scan should find no legacy paths in the updated file
    auto matches2 = cms::scanFile(f);
    for (const auto& m : matches2) {
        EXPECT_NE(m.legacy_path, "config/lora_training_config.yaml")
            << "Legacy path should not appear after fix";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// cms::ScanMatch field population tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConfigMigrationScannerTest, ScanMatchHasCorrectCategory) {
    auto f = test_dir_ / "cat.yaml";
    writeFile(f, "cfg: config/lora_training_config.yaml\n");

    auto matches = cms::scanFile(f);
    bool found = false;
    for (const auto& m : matches) {
        if (m.legacy_path == "config/lora_training_config.yaml") {
            EXPECT_FALSE(m.category.empty());
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ConfigMigrationScannerTest, ScanMatchHasFormattedDates) {
    // Verify deprecated_date and removal_date are in YYYY-MM-DD format
    auto f = test_dir_ / "dates.yaml";
    writeFile(f, "cfg: config/lora_training_config.yaml\n");

    auto matches = cms::scanFile(f);
    for (const auto& m : matches) {
        if (m.legacy_path == "config/lora_training_config.yaml") {
            // Date format check: YYYY-MM-DD (length 10, contains dashes)
            if (!m.deprecated_date.empty()) {
                EXPECT_EQ(m.deprecated_date.size(), 10u);
                EXPECT_EQ(m.deprecated_date[4], '-');
                EXPECT_EQ(m.deprecated_date[7], '-');
            }
            if (!m.removal_date.empty()) {
                EXPECT_EQ(m.removal_date.size(), 10u);
                EXPECT_EQ(m.removal_date[4], '-');
                EXPECT_EQ(m.removal_date[7], '-');
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Full directory scan (multiple files) tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConfigMigrationScannerTest, RecursiveScanFindsMatchesInSubdirectories) {
    auto sub = test_dir_ / "sub" / "nested";
    writeFile(sub / "deploy.yaml", "path: config/pii_patterns.yaml\n");
    writeFile(sub / "clean.yaml", "key: value\n");

    // Collect matches from all yaml files in test_dir_
    std::vector<cms::ScanMatch> all_matches;
    for (const auto& entry : fs::recursive_directory_iterator(test_dir_,
            fs::directory_options::skip_permission_denied)) {
        if (!entry.is_regular_file()) continue;
        if (!cms::shouldScanFile(entry.path())) continue;
        auto file_matches = cms::scanFile(entry.path());
        all_matches.insert(all_matches.end(),
                           std::make_move_iterator(file_matches.begin()),
                           std::make_move_iterator(file_matches.end()));
    }

    bool found = false;
    for (const auto& m : all_matches) {
        if (m.legacy_path == "config/pii_patterns.yaml") {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Should find match in nested subdirectory";
}

TEST_F(ConfigMigrationScannerTest, NonScanableFilesAreSkipped) {
    writeFile(test_dir_ / "source.cpp", "// config/lora_training_config.yaml\n");
    writeFile(test_dir_ / "readme.md", "config/lora_training_config.yaml\n");

    std::vector<cms::ScanMatch> all_matches;
    for (const auto& entry : fs::recursive_directory_iterator(test_dir_,
            fs::directory_options::skip_permission_denied)) {
        if (!entry.is_regular_file()) continue;
        if (!cms::shouldScanFile(entry.path())) continue;
        auto file_matches = cms::scanFile(entry.path());
        all_matches.insert(all_matches.end(),
                           std::make_move_iterator(file_matches.begin()),
                           std::make_move_iterator(file_matches.end()));
    }
    // .cpp and .md files should not be scanned
    EXPECT_TRUE(all_matches.empty());
}
