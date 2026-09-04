/*
 * test_config_migration_scanner.cpp
 *
 * Unit tests for the config_migration_scanner logic (shouldScanFile,
 * scanFile, fixFile, printText, printJson, printCsv, formatTimePoint)
 * exposed via config/config_migration_scanner_impl.h.
 *
 * Tests create temporary file trees in /tmp, exercise the scanning and
 * fix logic, and verify the expected results.
 */

#include <gtest/gtest.h>
#include "config/config_migration_scanner_impl.h"

#include <filesystem>
#include <fstream>
#include <sstream>
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

TEST_F(ConfigMigrationScannerTest, FixFileDryRunPrintsWouldUpdateMessage) {
    auto f = test_dir_ / "dryrun_msg.yaml";
    writeFile(f, "path: config/lora_training_config.yaml\n");

    auto matches = cms::scanFile(f);
    ASSERT_FALSE(matches.empty());

    // Capture stdout to verify the [dry-run] message is printed
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    bool ok = cms::fixFile(f, matches, /*dry_run=*/true);
    std::cout.rdbuf(old);

    EXPECT_TRUE(ok);
    std::string output = oss.str();
    EXPECT_NE(output.find("[dry-run]"), std::string::npos)
        << "dry-run mode must print a '[dry-run]' notice to stdout";
    EXPECT_NE(output.find(f.string()), std::string::npos)
        << "dry-run message must include the affected file path";
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
    // Verify deprecated_date and removal_date are in YYYY-MM-DD format.
    // config/lora_training_config.yaml is a known mapped path with complete metadata.
    auto f = test_dir_ / "dates.yaml";
    writeFile(f, "cfg: config/lora_training_config.yaml\n");

    auto matches = cms::scanFile(f);
    bool found = false;
    for (const auto& m : matches) {
        if (m.legacy_path == "config/lora_training_config.yaml") {
            found = true;
            // Known mapped paths must have both dates populated
            EXPECT_FALSE(m.deprecated_date.empty()) << "deprecated_date must be set for a known legacy path";
            EXPECT_FALSE(m.removal_date.empty())    << "removal_date must be set for a known legacy path";
            // Date format check: YYYY-MM-DD (length 10, contains dashes at positions 4 and 7)
            EXPECT_EQ(m.deprecated_date.size(), 10u);
            EXPECT_EQ(m.deprecated_date[4], '-');
            EXPECT_EQ(m.deprecated_date[7], '-');
            EXPECT_EQ(m.removal_date.size(), 10u);
            EXPECT_EQ(m.removal_date[4], '-');
            EXPECT_EQ(m.removal_date[7], '-');
        }
    }
    EXPECT_TRUE(found) << "Expected config/lora_training_config.yaml to be found";
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
        if (!entry.is_regular_file()) {
          continue;
        }
        if (!cms::shouldScanFile(entry.path())) {
          continue;
        }
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
        if (!entry.is_regular_file()) {
          continue;
        }
        if (!cms::shouldScanFile(entry.path())) {
          continue;
        }
        auto file_matches = cms::scanFile(entry.path());
        all_matches.insert(all_matches.end(),
                           std::make_move_iterator(file_matches.begin()),
                           std::make_move_iterator(file_matches.end()));
    }
    // .cpp and .md files should not be scanned
    EXPECT_TRUE(all_matches.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// formatTimePoint tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(FormatTimePointTest, NulloptReturnsEmptyString) {
    EXPECT_EQ(cms::formatTimePoint(std::nullopt), "");
}

TEST(FormatTimePointTest, ValidDateFormatsAsYyyyMmDd) {
    // Construct 2025-06-15 as a time_point
    std::chrono::year_month_day ymd{
        std::chrono::year{2025},
        std::chrono::June,
        std::chrono::day{15}
    };
    auto tp = static_cast<std::chrono::system_clock::time_point>(
        std::chrono::sys_days{ymd});
    std::string result = cms::formatTimePoint(tp);
    EXPECT_EQ(result, "2025-06-15");
}

TEST(FormatTimePointTest, SingleDigitMonthAndDayArePadded) {
    std::chrono::year_month_day ymd{
        std::chrono::year{2026},
        std::chrono::January,
        std::chrono::day{5}
    };
    auto tp = static_cast<std::chrono::system_clock::time_point>(
        std::chrono::sys_days{ymd});
    std::string result = cms::formatTimePoint(tp);
    EXPECT_EQ(result, "2026-01-05");
}

// ─────────────────────────────────────────────────────────────────────────────
// Output formatter tests (printText, printJson, printCsv)
// Captures stdout by temporarily redirecting std::cout.
// ─────────────────────────────────────────────────────────────────────────────

// Helper: captures std::cout output for a callable.
template <typename Fn>
static std::string captureStdout(Fn fn) {
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    fn();
    std::cout.rdbuf(old);
    return oss.str();
}

// Build a minimal ScanMatch for use in formatter tests.
static cms::ScanMatch makeScanMatch(const std::string& file,
                                    int line,
                                    const std::string& legacy,
                                    const std::string& new_p,
                                    const std::string& category,
                                    bool removal_due = false,
                                    const std::string& depr_date = "2024-01-01",
                                    const std::string& rm_date   = "2026-06-30",
                                    const std::string& guide     = "docs/config_migration_guide.md") {
    cms::ScanMatch m;
    m.file                = file;
    m.line_number         = line;
    m.legacy_path         = legacy;
    m.new_path            = new_p;
    m.category            = category;
    m.removal_due         = removal_due;
    m.deprecated_date     = depr_date;
    m.removal_date        = rm_date;
    m.migration_guide_url = guide;
    return m;
}

// ── printText ────────────────────────────────────────────────────────────────

TEST(PrintTextTest, EmptyMatchesProducesNoOutput) {
    std::string out = captureStdout([]{ cms::printText({}); });
    EXPECT_TRUE(out.empty());
}

TEST(PrintTextTest, SingleMatchContainsExpectedFields) {
    auto m = makeScanMatch("deploy.yaml", 3,
                           "config/pii_patterns.yaml",
                           "config/security/pii_patterns.yaml",
                           "security");
    std::string out = captureStdout([&]{ cms::printText({m}); });

    EXPECT_NE(out.find("deploy.yaml"), std::string::npos);
    EXPECT_NE(out.find(":3:"), std::string::npos);
    EXPECT_NE(out.find("security"), std::string::npos);
    EXPECT_NE(out.find("config/pii_patterns.yaml"), std::string::npos);
    EXPECT_NE(out.find("config/security/pii_patterns.yaml"), std::string::npos);
    EXPECT_NE(out.find("2026-06-30"), std::string::npos);
    EXPECT_NE(out.find("docs/config_migration_guide.md"), std::string::npos);
}

TEST(PrintTextTest, OverdueFlagAppearsWhenRemovalDue) {
    auto m = makeScanMatch("old.yaml", 1,
                           "config/pii_patterns.yaml",
                           "config/security/pii_patterns.yaml",
                           "security",
                           /*removal_due=*/true,
                           "2023-01-01", "2024-01-01");
    std::string out = captureStdout([&]{ cms::printText({m}); });
    EXPECT_NE(out.find("OVERDUE"), std::string::npos);
}

TEST(PrintTextTest, OverdueFlagAbsentWhenNotDue) {
    auto m = makeScanMatch("new.yaml", 1,
                           "config/pii_patterns.yaml",
                           "config/security/pii_patterns.yaml",
                           "security",
                           /*removal_due=*/false);
    std::string out = captureStdout([&]{ cms::printText({m}); });
    EXPECT_EQ(out.find("OVERDUE"), std::string::npos);
}

TEST(PrintTextTest, MultipleMatchesProduceMultipleLines) {
    std::vector<cms::ScanMatch> matches = {
        makeScanMatch("a.yaml", 1, "config/pii_patterns.yaml",
                      "config/security/pii_patterns.yaml", "security"),
        makeScanMatch("b.yaml", 2, "config/lora_training_config.yaml",
                      "config/ai_ml/lora_training_config.yaml", "ai_ml"),
    };
    std::string out = captureStdout([&]{ cms::printText(matches); });

    // Two newlines → two output lines
    int newlines = static_cast<int>(std::count(out.begin(), out.end(), '\n'));
    EXPECT_GE(newlines, 2);
}

// ── printJson ────────────────────────────────────────────────────────────────

TEST(PrintJsonTest, EmptyMatchesProducesEmptyJsonArray) {
    std::string out = captureStdout([]{ cms::printJson({}); });
    EXPECT_NE(out.find('['), std::string::npos);
    EXPECT_NE(out.find(']'), std::string::npos);
    // Should be essentially "[\n]\n"
    EXPECT_EQ(out.find('{'), std::string::npos);
}

TEST(PrintJsonTest, SingleMatchContainsRequiredKeys) {
    auto m = makeScanMatch("cfg.yaml", 5,
                           "config/pii_patterns.yaml",
                           "config/security/pii_patterns.yaml",
                           "security");
    std::string out = captureStdout([&]{ cms::printJson({m}); });

    EXPECT_NE(out.find("\"file\""),            std::string::npos);
    EXPECT_NE(out.find("\"line\""),            std::string::npos);
    EXPECT_NE(out.find("\"legacy_path\""),     std::string::npos);
    EXPECT_NE(out.find("\"new_path\""),        std::string::npos);
    EXPECT_NE(out.find("\"category\""),        std::string::npos);
    EXPECT_NE(out.find("\"deprecated_date\""), std::string::npos);
    EXPECT_NE(out.find("\"removal_date\""),    std::string::npos);
    EXPECT_NE(out.find("\"removal_overdue\""), std::string::npos);
    EXPECT_NE(out.find("\"migration_guide\""), std::string::npos);
}

TEST(PrintJsonTest, SingleMatchLineNumberIsNumeric) {
    auto m = makeScanMatch("cfg.yaml", 42,
                           "config/pii_patterns.yaml",
                           "config/security/pii_patterns.yaml",
                           "security");
    std::string out = captureStdout([&]{ cms::printJson({m}); });
    // "line": 42 — without quotes around 42
    EXPECT_NE(out.find("\"line\": 42"), std::string::npos);
}

TEST(PrintJsonTest, RemovalOverdueTrueWhenDue) {
    auto m = makeScanMatch("old.yaml", 1,
                           "config/pii_patterns.yaml",
                           "config/security/pii_patterns.yaml",
                           "security",
                           /*removal_due=*/true);
    std::string out = captureStdout([&]{ cms::printJson({m}); });
    EXPECT_NE(out.find("\"removal_overdue\": true"), std::string::npos);
}

TEST(PrintJsonTest, RemovalOverdueFalseWhenNotDue) {
    auto m = makeScanMatch("new.yaml", 1,
                           "config/pii_patterns.yaml",
                           "config/security/pii_patterns.yaml",
                           "security",
                           /*removal_due=*/false);
    std::string out = captureStdout([&]{ cms::printJson({m}); });
    EXPECT_NE(out.find("\"removal_overdue\": false"), std::string::npos);
}

TEST(PrintJsonTest, MultipleMatchesNoTrailingCommaOnLastEntry) {
    std::vector<cms::ScanMatch> matches = {
        makeScanMatch("a.yaml", 1, "config/pii_patterns.yaml",
                      "config/security/pii_patterns.yaml", "security"),
        makeScanMatch("b.yaml", 2, "config/lora_training_config.yaml",
                      "config/ai_ml/lora_training_config.yaml", "ai_ml"),
    };
    std::string out = captureStdout([&]{ cms::printJson(matches); });
    // Last object must not be followed by a comma before the closing ']'
    auto last_brace = out.rfind('}');
    auto closing_bracket = out.find(']', last_brace);
    ASSERT_NE(last_brace, std::string::npos);
    ASSERT_NE(closing_bracket, std::string::npos);
    // No comma between last '}' and ']'
    std::string between = out.substr(last_brace + 1, closing_bracket - last_brace - 1);
    EXPECT_EQ(between.find(','), std::string::npos)
        << "Trailing comma found after last JSON object";
}

TEST(PrintJsonTest, JsonEscapesDoubleQuotesInPaths) {
    // Construct a match with a double-quote in the file path
    auto m = makeScanMatch("path/with\"quote/cfg.yaml", 1,
                           "config/pii_patterns.yaml",
                           "config/security/pii_patterns.yaml",
                           "security");
    std::string out = captureStdout([&]{ cms::printJson({m}); });
    // The quote in the file path should be escaped as \"
    EXPECT_NE(out.find("\\\"quote"), std::string::npos);
}

// ── printCsv ─────────────────────────────────────────────────────────────────

TEST(PrintCsvTest, EmptyMatchesProducesOnlyHeaderRow) {
    std::string out = captureStdout([]{ cms::printCsv({}); });
    // Should contain the header and nothing else
    EXPECT_NE(out.find("file,line,legacy_path,new_path,category"), std::string::npos);
    EXPECT_NE(out.find("deprecated_date,removal_date,removal_overdue,migration_guide"),
              std::string::npos);
    // Only one line (the header)
    int newlines = static_cast<int>(std::count(out.begin(), out.end(), '\n'));
    EXPECT_EQ(newlines, 1);
}

TEST(PrintCsvTest, SingleMatchHasCorrectNumberOfColumns) {
    auto m = makeScanMatch("deploy.yaml", 7,
                           "config/pii_patterns.yaml",
                           "config/security/pii_patterns.yaml",
                           "security");
    std::string out = captureStdout([&]{ cms::printCsv({m}); });

    // Split into lines; skip header, check data line
    std::istringstream iss(out);
    std::string header, data;
    std::getline(iss, header);
    std::getline(iss, data);

    // The CSV format has 9 columns: file, line, legacy_path, new_path, category,
    // deprecated_date, removal_date, removal_overdue, migration_guide.
    // 9 columns → at least 8 commas in the data row (more if any field is quoted).
    constexpr int kExpectedCsvColumns = 9;
    int commas = static_cast<int>(std::count(data.begin(), data.end(), ','));
    EXPECT_GE(commas, kExpectedCsvColumns - 1);
}

TEST(PrintCsvTest, RemovalOverdueColumn) {
    auto m_due = makeScanMatch("old.yaml", 1,
                               "config/pii_patterns.yaml",
                               "config/security/pii_patterns.yaml",
                               "security",
                               /*removal_due=*/true);
    auto m_not = makeScanMatch("new.yaml", 1,
                               "config/pii_patterns.yaml",
                               "config/security/pii_patterns.yaml",
                               "security",
                               /*removal_due=*/false);

    std::string out_due = captureStdout([&]{ cms::printCsv({m_due}); });
    std::string out_not = captureStdout([&]{ cms::printCsv({m_not}); });

    // removal_overdue column: 1 when due, 0 when not
    EXPECT_NE(out_due.find(",1,"), std::string::npos);
    EXPECT_NE(out_not.find(",0,"), std::string::npos);
}

TEST(PrintCsvTest, FieldsWithCommasAreQuoted) {
    // Category with a comma would normally not happen for known paths, but
    // the quoting logic should handle it.
    auto m = makeScanMatch("cfg.yaml", 1,
                           "config/pii_patterns.yaml",
                           "config/security/pii_patterns.yaml",
                           "sec,urity");   // comma in category
    std::string out = captureStdout([&]{ cms::printCsv({m}); });
    // The category field containing a comma must be quoted
    EXPECT_NE(out.find("\"sec,urity\""), std::string::npos);
}
