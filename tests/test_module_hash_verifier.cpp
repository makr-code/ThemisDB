/// @file test_module_hash_verifier.cpp
/// @brief Unit tests for ModuleHashVerifier – SHA-256 module integrity checks
///
/// Tests verify:
/// - SHA-256 computation (positive, negative, non-existent file)
/// - Static verifyHash helper
/// - Manifest load / save round-trip
/// - addExpectedHash / manifestSize / clearManifest
/// - verifyModule with matching hash, mismatched hash, missing manifest entry

#include <gtest/gtest.h>
#include "themis/module_hash_verifier.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <filesystem>

using namespace themis::modules;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Write @p content to a unique temporary file and return its path.
/// Uses the process ID and a counter to avoid collisions between tests.
/// Caller must call std::remove() on the returned path when done.
std::string writeTempFile(const std::string& content,
                          const std::string& suffix = ".tmp") {
    const auto base = std::filesystem::temp_directory_path()
                      / ("themis_test_hash_" +
                         std::to_string(static_cast<long>(::getpid())) +
                         "_" + suffix);
    const std::string path = base.string();
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write(content.data(), static_cast<std::streamsize>(content.size()));
    return path;
}

} // namespace

// ---------------------------------------------------------------------------
// computeSHA256 tests
// ---------------------------------------------------------------------------

TEST(ModuleHashVerifier, ComputeSHA256EmptyFile) {
    const std::string path = writeTempFile("", "empty.bin");
    // SHA-256 of zero bytes is the well-known constant.
    const std::string expected =
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

    EXPECT_EQ(ModuleHashVerifier::computeSHA256(path), expected);
    std::remove(path.c_str());
}

TEST(ModuleHashVerifier, ComputeSHA256KnownContent) {
    // SHA-256("abc") = ba7816bf...
    const std::string path = writeTempFile("abc", "abc.bin");
    const std::string expected =
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";

    EXPECT_EQ(ModuleHashVerifier::computeSHA256(path), expected);
    std::remove(path.c_str());
}

TEST(ModuleHashVerifier, ComputeSHA256NonExistentFile) {
    // Should return empty string on failure.
    EXPECT_TRUE(
        ModuleHashVerifier::computeSHA256("/nonexistent/no/such/file.so")
            .empty());
}

TEST(ModuleHashVerifier, ComputeSHA256LargerContent) {
    // Write 1 MiB of 0x42 bytes and verify we get a non-empty hash.
    const std::string content(1024 * 1024, '\x42');
    const std::string path = writeTempFile(content, "large.bin");
    const std::string hash = ModuleHashVerifier::computeSHA256(path);

    EXPECT_EQ(hash.size(), 64u);  // 32 bytes = 64 hex chars
    EXPECT_FALSE(hash.empty());
    std::remove(path.c_str());
}

// ---------------------------------------------------------------------------
// Static verifyHash tests
// ---------------------------------------------------------------------------

TEST(ModuleHashVerifier, VerifyHashMatch) {
    const std::string path = writeTempFile("hello world", "verify_match.bin");
    const std::string hash = ModuleHashVerifier::computeSHA256(path);

    EXPECT_TRUE(ModuleHashVerifier::verifyHash(path, hash));
    std::remove(path.c_str());
}

TEST(ModuleHashVerifier, VerifyHashMismatch) {
    const std::string path = writeTempFile("hello world", "verify_mismatch.bin");

    EXPECT_FALSE(ModuleHashVerifier::verifyHash(path, "deadbeef"));
    std::remove(path.c_str());
}

TEST(ModuleHashVerifier, VerifyHashNonExistentFile) {
    EXPECT_FALSE(
        ModuleHashVerifier::verifyHash("/no/such/module.so", "deadbeef"));
}

// ---------------------------------------------------------------------------
// Manifest management tests
// ---------------------------------------------------------------------------

TEST(ModuleHashVerifier, InitialManifestIsEmpty) {
    ModuleHashVerifier v;
    EXPECT_EQ(v.manifestSize(), 0u);
}

TEST(ModuleHashVerifier, AddExpectedHashIncreasesSize) {
    ModuleHashVerifier v;
    v.addExpectedHash("themis_storage", "aabbcc");
    EXPECT_EQ(v.manifestSize(), 1u);

    v.addExpectedHash("themis_query", "112233");
    EXPECT_EQ(v.manifestSize(), 2u);
}

TEST(ModuleHashVerifier, AddExpectedHashOverwritesExisting) {
    ModuleHashVerifier v;
    v.addExpectedHash("mod_a", "hash1");
    v.addExpectedHash("mod_a", "hash2");
    EXPECT_EQ(v.manifestSize(), 1u);  // same key updated, not added
}

TEST(ModuleHashVerifier, ClearManifestEmptiesMap) {
    ModuleHashVerifier v;
    v.addExpectedHash("mod_a", "hash1");
    v.addExpectedHash("mod_b", "hash2");
    EXPECT_EQ(v.manifestSize(), 2u);

    v.clearManifest();
    EXPECT_EQ(v.manifestSize(), 0u);
}

TEST(ModuleHashVerifier, GetExpectedHashPresent) {
    ModuleHashVerifier v;
    v.addExpectedHash("themis_storage", "aabbccdd");

    const auto result = v.getExpectedHash("themis_storage");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "aabbccdd");
}

TEST(ModuleHashVerifier, GetExpectedHashAbsent) {
    ModuleHashVerifier v;  // empty manifest

    EXPECT_FALSE(v.getExpectedHash("nonexistent").has_value());
}

TEST(ModuleHashVerifier, GetExpectedHashAfterClear) {
    ModuleHashVerifier v;
    v.addExpectedHash("mod", "hash");
    EXPECT_TRUE(v.getExpectedHash("mod").has_value());

    v.clearManifest();
    EXPECT_FALSE(v.getExpectedHash("mod").has_value());
}


// loadManifest / saveManifest round-trip
// ---------------------------------------------------------------------------

TEST(ModuleHashVerifier, SaveAndLoadManifestRoundTrip) {
    ModuleHashVerifier v;
    v.addExpectedHash("themis_storage",
                      "e3b0c44298fc1c149afbf4c8996fb924"
                      "27ae41e4649b934ca495991b7852b855");
    v.addExpectedHash("themis_query",
                      "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    const std::string manifestPath = (std::filesystem::temp_directory_path() /
        ("themis_test_manifest_roundtrip_" +
         std::to_string(static_cast<long>(::getpid())) + ".json")).string();
    ASSERT_TRUE(v.saveManifest(manifestPath));

    ModuleHashVerifier v2;
    ASSERT_TRUE(v2.loadManifest(manifestPath));
    EXPECT_EQ(v2.manifestSize(), 2u);

    // Verify the loaded entries produce the same results.
    const std::string emptyPath =
        writeTempFile("", "roundtrip_empty.bin");
    const auto r = v2.verifyModule("themis_storage", emptyPath);
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.computedHash, r.expectedHash);

    std::remove(emptyPath.c_str());
    std::remove(manifestPath.c_str());
}

TEST(ModuleHashVerifier, LoadManifestNonExistentFile) {
    ModuleHashVerifier v;
    EXPECT_FALSE(v.loadManifest("/no/such/manifest.json"));
    EXPECT_EQ(v.manifestSize(), 0u);
}

TEST(ModuleHashVerifier, LoadManifestInvalidJson) {
    const std::string path = writeTempFile("not json {{{", "bad_manifest.json");
    ModuleHashVerifier v;
    EXPECT_FALSE(v.loadManifest(path));
    std::remove(path.c_str());
}

TEST(ModuleHashVerifier, LoadManifestNonObjectJson) {
    const std::string path = writeTempFile("[1, 2, 3]", "array_manifest.json");
    ModuleHashVerifier v;
    EXPECT_FALSE(v.loadManifest(path));
    std::remove(path.c_str());
}

TEST(ModuleHashVerifier, SaveManifestToInvalidPath) {
    ModuleHashVerifier v;
    v.addExpectedHash("mod", "hash");
    EXPECT_FALSE(v.saveManifest("/no/such/directory/manifest.json"));
}

// ---------------------------------------------------------------------------
// verifyModule tests
// ---------------------------------------------------------------------------

TEST(ModuleHashVerifier, VerifyModuleSuccess) {
    const std::string content = "themis module binary data";
    const std::string modPath = writeTempFile(content, "module_ok.so");
    const std::string hash    = ModuleHashVerifier::computeSHA256(modPath);

    ModuleHashVerifier v;
    v.addExpectedHash("themis_test", hash);

    const auto result = v.verifyModule("themis_test", modPath);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.computedHash, hash);
    EXPECT_EQ(result.expectedHash, hash);
    EXPECT_TRUE(result.errorMessage.empty());

    std::remove(modPath.c_str());
}

TEST(ModuleHashVerifier, VerifyModuleHashMismatch) {
    const std::string modPath = writeTempFile("module v1", "module_v1.so");

    ModuleHashVerifier v;
    v.addExpectedHash("themis_test",
                      "0000000000000000000000000000000000000000000000000000000000000000");

    const auto result = v.verifyModule("themis_test", modPath);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.computedHash.empty());
    EXPECT_EQ(result.expectedHash,
              "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_FALSE(result.errorMessage.empty());

    std::remove(modPath.c_str());
}

TEST(ModuleHashVerifier, VerifyModuleNotInManifest) {
    ModuleHashVerifier v;  // empty manifest

    const auto result = v.verifyModule("themis_missing", "/some/path.so");
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.computedHash.empty());
    EXPECT_TRUE(result.expectedHash.empty());
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST(ModuleHashVerifier, VerifyModuleFileNotFound) {
    ModuleHashVerifier v;
    v.addExpectedHash("themis_ghost", "aabbccdd");

    const auto result = v.verifyModule("themis_ghost", "/no/such/file.so");
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.computedHash.empty());
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST(ModuleHashVerifier, VerifyModuleResultFields) {
    const std::string modPath = writeTempFile("test data", "verify_fields.so");
    const std::string hash    = ModuleHashVerifier::computeSHA256(modPath);

    ModuleHashVerifier v;
    v.addExpectedHash("mod", hash);

    const auto r = v.verifyModule("mod", modPath);
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.computedHash.size(), 64u);  // 32 bytes × 2 hex chars
    EXPECT_EQ(r.expectedHash.size(), 64u);

    std::remove(modPath.c_str());
}
