/**
 * @file test_ingestion_security.cpp
 * @brief Security audit tests for the ingestion module.
 *
 * Covers:
 *   - isPathTraversalSafe(): rejects paths with ".." components
 *   - FileSystemIngester::initialize(): rejects traversal paths
 *   - Symlink-escape guard during directory walk
 *   - sanitizeOptions(): redacts sensitive credential keys in options maps
 */

#include <gtest/gtest.h>
#include "ingestion/filesystem_ingester.h"
#include "ingestion/ingestion_manager.h"
#include <filesystem>
#include <fstream>
#include <string>

using namespace themis::ingestion;
namespace fs = std::filesystem;

// ============================================================================
// isPathTraversalSafe – unit tests
// ============================================================================

TEST(IsPathTraversalSafeTest, EmptyPathIsSafe) {
    EXPECT_TRUE(isPathTraversalSafe(""));
}

TEST(IsPathTraversalSafeTest, AbsolutePathWithoutTraversalIsSafe) {
    EXPECT_TRUE(isPathTraversalSafe("/tmp/data"));
    EXPECT_TRUE(isPathTraversalSafe("/home/user/documents"));
    EXPECT_TRUE(isPathTraversalSafe("/var/lib/themis/ingestion"));
}

TEST(IsPathTraversalSafeTest, RelativePathWithoutTraversalIsSafe) {
    EXPECT_TRUE(isPathTraversalSafe("data"));
    EXPECT_TRUE(isPathTraversalSafe("data/subdir"));
    EXPECT_TRUE(isPathTraversalSafe("./data/subdir"));
}

TEST(IsPathTraversalSafeTest, DoubleDotComponentRejected) {
    EXPECT_FALSE(isPathTraversalSafe(".."));
    EXPECT_FALSE(isPathTraversalSafe("../etc/passwd"));
    EXPECT_FALSE(isPathTraversalSafe("../../etc/shadow"));
    EXPECT_FALSE(isPathTraversalSafe("/tmp/../etc/passwd"));
    EXPECT_FALSE(isPathTraversalSafe("/data/safe/../../../etc"));
}

TEST(IsPathTraversalSafeTest, EmbeddedDoubleDotRejected) {
    EXPECT_FALSE(isPathTraversalSafe("data/../../../secret"));
    EXPECT_FALSE(isPathTraversalSafe("/var/data/../../etc"));
}

TEST(IsPathTraversalSafeTest, SingleDotComponentIsSafe) {
    // "." is a valid relative reference to current directory – not a traversal
    EXPECT_TRUE(isPathTraversalSafe("."));
    EXPECT_TRUE(isPathTraversalSafe("./data"));
    EXPECT_TRUE(isPathTraversalSafe("/tmp/./data"));
}

TEST(IsPathTraversalSafeTest, WindowsStylePathWithTraversalRejected) {
    // Portable check – ".." is ".." regardless of separator
    EXPECT_FALSE(isPathTraversalSafe("C:\\data\\..\\secret"));
}

// ============================================================================
// FileSystemIngester::initialize – path traversal guard
// ============================================================================

TEST(FileSystemIngesterPathTraversalTest, TraversalPathRejectedByInitialize) {
    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "traversal_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "../../etc/passwd";
    EXPECT_FALSE(ingester.initialize(cfg));
}

TEST(FileSystemIngesterPathTraversalTest, AbsoluteTraversalPathRejected) {
    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "abs_traversal";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/../etc/shadow";
    EXPECT_FALSE(ingester.initialize(cfg));
}

TEST(FileSystemIngesterPathTraversalTest, SafePathAccepted) {
    // Use the system temp directory which is guaranteed to exist.
    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "safe_path";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = fs::temp_directory_path().string();
    EXPECT_TRUE(ingester.initialize(cfg));
}

TEST(FileSystemIngesterPathTraversalTest, RelativeSafePathAccepted) {
    // A relative path without ".." should pass the traversal check.
    // initialize() will return false only if the resolved path does not exist;
    // we just verify the traversal guard itself does not block a clean path.
    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "relative_safe";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "data/subdir";
    // Path probably does not exist, but the traversal check itself passes.
    // The return value depends on fs::exists(); we only care that it is
    // not blocked by the traversal guard (which would always return false for "..").
    // Since "data/subdir" contains no "..", isPathTraversalSafe() returns true.
    EXPECT_TRUE(isPathTraversalSafe(cfg.location));
}

// ============================================================================
// Symlink-escape guard during directory walk
// ============================================================================

TEST(FileSystemIngesterSymlinkTest, SymlinkOutsideBaseIsSkipped) {
    // Create a sandbox directory and a symlink inside it that points outside.
    auto sandbox = fs::temp_directory_path() / "themis_symlink_test";
    auto outside_dir = fs::temp_directory_path() / "themis_symlink_outside";
    fs::create_directories(sandbox);
    fs::create_directories(outside_dir);

    // Create a legitimate text file inside sandbox
    auto inside_file = sandbox / "inside.txt";
    { std::ofstream f(inside_file); f << "inside content\n"; }

    // Create a text file outside the sandbox
    auto outside_file = outside_dir / "outside.txt";
    { std::ofstream f(outside_file); f << "outside content\n"; }

    // Create a symlink inside sandbox pointing to the outside file
    auto symlink_path = sandbox / "link_to_outside.txt";
    std::error_code ec = {};
    fs::create_symlink(outside_file, symlink_path, ec);
    if (ec) {
        // Symlink creation may not be supported (e.g. restricted CI).
        // Skip the test gracefully rather than failing.
        GTEST_SKIP() << "Symlink creation not supported: " << ec.message();
    }

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "symlink_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = sandbox.string();
    cfg.options["recursive"] = "false";
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);

    // Only the legitimate inside.txt should be processed;
    // the symlink pointing outside should be silently skipped.
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed, 0u);

    fs::remove_all(sandbox);
    fs::remove_all(outside_dir);
}

// ============================================================================
// sanitizeOptions – API key / credential masking
// ============================================================================

TEST(SanitizeOptionsTest, EmptyMapReturnedUnchanged) {
    std::unordered_map<std::string, std::string> opts;
    auto result = sanitizeOptions(opts);
    EXPECT_TRUE(result.empty());
}

TEST(SanitizeOptionsTest, NonSensitiveKeysPassThrough) {
    std::unordered_map<std::string, std::string> opts = {
        {"page_size",       "100"},
        {"text_field",      "body"},
        {"recursive",       "true"},
        {"pagination_mode", "cursor"}
    };
    auto result = sanitizeOptions(opts);
    EXPECT_EQ(result["page_size"],       "100");
    EXPECT_EQ(result["text_field"],      "body");
    EXPECT_EQ(result["recursive"],       "true");
    EXPECT_EQ(result["pagination_mode"], "cursor");
}

TEST(SanitizeOptionsTest, ApiKeyIsRedacted) {
    std::unordered_map<std::string, std::string> opts = {{"api_key", "sk-supersecret"}};
    auto result = sanitizeOptions(opts);
    EXPECT_EQ(result["api_key"], "***");
}

TEST(SanitizeOptionsTest, TokenIsRedacted) {
    std::unordered_map<std::string, std::string> opts = {{"token", "hf_abc123"}};
    auto result = sanitizeOptions(opts);
    EXPECT_EQ(result["token"], "***");
}

TEST(SanitizeOptionsTest, OAuthSecretsAreRedacted) {
    std::unordered_map<std::string, std::string> opts = {
        {"oauth_client_secret",  "client_secret_value"},
        {"oauth_access_token",   "access_token_value"},
        {"oauth_refresh_token",  "refresh_token_value"},
    };
    auto result = sanitizeOptions(opts);
    EXPECT_EQ(result["oauth_client_secret"],  "***");
    EXPECT_EQ(result["oauth_access_token"],   "***");
    EXPECT_EQ(result["oauth_refresh_token"],  "***");
}

TEST(SanitizeOptionsTest, PasswordAndSecretAreRedacted) {
    std::unordered_map<std::string, std::string> opts = {
        {"password",      "hunter2"},
        {"secret",        "topsecret"},
        {"client_secret", "oauth_secret"},
        {"credentials",   "user:pass"},
        {"auth_token",    "bearer_abc"}
    };
    auto result = sanitizeOptions(opts);
    EXPECT_EQ(result["password"],      "***");
    EXPECT_EQ(result["secret"],        "***");
    EXPECT_EQ(result["client_secret"], "***");
    EXPECT_EQ(result["credentials"],   "***");
    EXPECT_EQ(result["auth_token"],    "***");
}

TEST(SanitizeOptionsTest, EmptyValueNotRedacted) {
    // An empty credential value carries no information; leave as-is to avoid
    // hiding the fact that no credential is configured.
    std::unordered_map<std::string, std::string> opts = {{"api_key", ""}};
    auto result = sanitizeOptions(opts);
    EXPECT_EQ(result["api_key"], "");
}

TEST(SanitizeOptionsTest, WhitespaceOnlyValueIsRedacted) {
    // A whitespace-only value is not empty; it must be redacted in case it
    // represents a partial or accidentally-padded credential.
    std::unordered_map<std::string, std::string> opts = {{"api_key", "   "}};
    auto result = sanitizeOptions(opts);
    EXPECT_EQ(result["api_key"], "***");
}

TEST(SanitizeOptionsTest, OriginalMapIsUnmodified) {
    // sanitizeOptions must not modify the caller's map.
    std::unordered_map<std::string, std::string> opts = {{"api_key", "secret123"}};
    (void)sanitizeOptions(opts);
    EXPECT_EQ(opts["api_key"], "secret123");
}

TEST(SanitizeOptionsTest, MixedMapRedactsOnlySensitiveKeys) {
    std::unordered_map<std::string, std::string> opts = {
        {"endpoint",    "https://api.example.com"},
        {"api_key",     "sk-secret"},
        {"page_size",   "50"},
        {"token",       "mytoken"},
        {"text_field",  "content"}
    };
    auto result = sanitizeOptions(opts);
    EXPECT_EQ(result["endpoint"],   "https://api.example.com");
    EXPECT_EQ(result["api_key"],    "***");
    EXPECT_EQ(result["page_size"],  "50");
    EXPECT_EQ(result["token"],      "***");
    EXPECT_EQ(result["text_field"], "content");
}
