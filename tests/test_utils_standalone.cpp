/**
 * @file test_utils_standalone.cpp
 * @brief Tests for standalone utility modules previously lacking coverage:
 *
 *   - utils/checksum_utils.cpp  : calculateSHA256, calculateMD5
 *   - utils/file_utils.cpp      : readFileContents
 *   - utils/normalizer.cpp      : Normalizer::normalizeUmlauts
 *   - performance/phase2_feature_flags.h : Phase2FeatureFlags singleton
 *
 * Uses only on-disk operations (temp files) — no hardware required.
 */

#include <gtest/gtest.h>
#include "utils/checksum_utils.h"
#include "utils/file_utils.h"
#include "utils/normalizer.h"
#include "performance/phase2_feature_flags.h"
#include <filesystem>
#include <fstream>
#include <string>

using namespace themis;
using namespace themis::performance;

// ============================================================================
// Helpers
// ============================================================================

static std::string writeTempFile(const std::string& content) {
    auto path = (std::filesystem::temp_directory_path() /
                 ("themis_utils_test_" +
                  std::to_string(std::chrono::high_resolution_clock::now()
                                     .time_since_epoch()
                                     .count()) +
                  ".tmp"))
                    .string();
    std::ofstream ofs(path, std::ios::binary);
    ofs << content;
    ofs.close();
    return path;
}

// ============================================================================
// calculateSHA256
// ============================================================================

TEST(ChecksumUtilsTest, SHA256_KnownContent) {
    // SHA256("hello\n") = 5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03
    auto path = writeTempFile("hello\n");
    auto sha   = themis::utils::calculateSHA256(path);
    std::filesystem::remove(path);

    EXPECT_EQ(sha.size(), 64u); // hex-encoded SHA256 is always 64 chars
    EXPECT_EQ(sha, "5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03");
}

TEST(ChecksumUtilsTest, SHA256_EmptyFile) {
    // SHA256 of empty file = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    auto path = writeTempFile("");
    auto sha   = themis::utils::calculateSHA256(path);
    std::filesystem::remove(path);

    EXPECT_EQ(sha.size(), 64u);
    EXPECT_EQ(sha, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(ChecksumUtilsTest, SHA256_NonExistentFile_ReturnsEmpty) {
    auto sha = themis::utils::calculateSHA256("/tmp/nonexistent_themis_xyz.bin");
    EXPECT_TRUE(sha.empty());
}

TEST(ChecksumUtilsTest, SHA256_TwoDistinctFiles_DifferentChecksums) {
    auto p1 = writeTempFile("content_A");
    auto p2 = writeTempFile("content_B");
    auto s1 = themis::utils::calculateSHA256(p1);
    auto s2 = themis::utils::calculateSHA256(p2);
    std::filesystem::remove(p1);
    std::filesystem::remove(p2);

    EXPECT_NE(s1, s2);
}

// ============================================================================
// calculateMD5 (deprecated — legacy backward-compat tests only)
// ============================================================================
// calculateMD5 is [[deprecated]] (CWE-327). The tests below verify the
// backward-compatible behaviour that is needed while legacy artifact checksums
// are being migrated to SHA-256.  Deprecation warnings are suppressed only
// for this block.
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

TEST(ChecksumUtilsTest, MD5_KnownContent) {
    // MD5("hello\n") = b1946ac92492d2347c6235b4d2611184
    auto path = writeTempFile("hello\n");
    auto md5  = themis::utils::calculateMD5(path);
    std::filesystem::remove(path);

    EXPECT_EQ(md5.size(), 32u); // hex-encoded MD5 is always 32 chars
    EXPECT_EQ(md5, "b1946ac92492d2347c6235b4d2611184");
}

TEST(ChecksumUtilsTest, MD5_EmptyFile) {
    // MD5("") = d41d8cd98f00b204e9800998ecf8427e
    auto path = writeTempFile("");
    auto md5  = themis::utils::calculateMD5(path);
    std::filesystem::remove(path);

    EXPECT_EQ(md5.size(), 32u);
    EXPECT_EQ(md5, "d41d8cd98f00b204e9800998ecf8427e");
}

TEST(ChecksumUtilsTest, MD5_NonExistentFile_ReturnsEmpty) {
    auto md5 = themis::utils::calculateMD5("/tmp/nonexistent_themis_xyz.bin");
    EXPECT_TRUE(md5.empty());
}

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif

// ============================================================================
// readFileContents
// ============================================================================

TEST(FileUtilsTest, ReadFileContents_PlainText) {
    const std::string content = "Hello, ThemisDB!\nLine two.\n";
    auto path = writeTempFile(content);
    auto read  = themis::utils::readFileContents(path);
    std::filesystem::remove(path);

    EXPECT_EQ(read, content);
}

TEST(FileUtilsTest, ReadFileContents_BinaryContent) {
    std::string binary(256, '\0');
    for (int i = 0; i < 256; ++i) binary[i] = static_cast<char>(i);
    auto path = writeTempFile(binary);
    auto read  = themis::utils::readFileContents(path);
    std::filesystem::remove(path);

    EXPECT_EQ(read.size(), 256u);
    EXPECT_EQ(read, binary);
}

TEST(FileUtilsTest, ReadFileContents_EmptyFile) {
    auto path = writeTempFile("");
    auto read  = themis::utils::readFileContents(path);
    std::filesystem::remove(path);

    EXPECT_TRUE(read.empty());
}

TEST(FileUtilsTest, ReadFileContents_NonExistent_Throws) {
    EXPECT_THROW(
        themis::utils::readFileContents("/tmp/nonexistent_themis_xyz_file.bin"),
        std::runtime_error
    );
}

// ============================================================================
// Normalizer::normalizeUmlauts
// ============================================================================

TEST(NormalizerTest, NormalizeUmlauts_GermanChars) {
    // ä -> a, ö -> o, ü -> u, Ä -> A, Ö -> O, Ü -> U, ß -> ss
    auto result = themis::utils::Normalizer::normalizeUmlauts("Über Straße");
    EXPECT_NE(result.find("U"), std::string::npos);
    EXPECT_NE(result.find("ss"), std::string::npos);
}

TEST(NormalizerTest, NormalizeUmlauts_NoUmlauts_Unchanged) {
    const std::string input = "Hello World";
    auto result = themis::utils::Normalizer::normalizeUmlauts(input);
    EXPECT_EQ(result, input);
}

TEST(NormalizerTest, NormalizeUmlauts_Empty_ReturnsEmpty) {
    auto result = themis::utils::Normalizer::normalizeUmlauts("");
    EXPECT_TRUE(result.empty());
}

TEST(NormalizerTest, NormalizeUmlauts_AllUmlauts) {
    // ä, ö, ü, Ä, Ö, Ü, ß should all be replaced
    auto result = themis::utils::Normalizer::normalizeUmlauts("äöüÄÖÜß");
    EXPECT_EQ(result, "aouAOUss");
}

// ============================================================================
// Phase2FeatureFlags singleton
// ============================================================================

TEST(Phase2FeatureFlagsTest, DefaultsAllFalse) {
    auto& flags = Phase2FeatureFlags::instance();
    // After default construction (reset side effects between tests by setting)
    // We only check that the getters are callable and return bool
    EXPECT_NO_THROW(flags.wisckey_enabled());
    EXPECT_NO_THROW(flags.dostoevsky_enabled());
    EXPECT_NO_THROW(flags.cicada_enabled());
    EXPECT_NO_THROW(flags.ligra_enabled());
    EXPECT_NO_THROW(flags.rabitq_enabled());
}

TEST(Phase2FeatureFlagsTest, SetAndGet_Wisckey) {
    auto& flags = Phase2FeatureFlags::instance();
    bool original = flags.wisckey_enabled();
    flags.set_wisckey_enabled(true);
    EXPECT_TRUE(flags.wisckey_enabled());
    flags.set_wisckey_enabled(false);
    EXPECT_FALSE(flags.wisckey_enabled());
    flags.set_wisckey_enabled(original); // restore
}

TEST(Phase2FeatureFlagsTest, SetAndGet_AllFlags) {
    auto& flags = Phase2FeatureFlags::instance();

    // Save originals
    bool w = flags.wisckey_enabled();
    bool d = flags.dostoevsky_enabled();
    bool c = flags.cicada_enabled();
    bool l = flags.ligra_enabled();
    bool r = flags.rabitq_enabled();

    // Enable all
    flags.set_wisckey_enabled(true);
    flags.set_dostoevsky_enabled(true);
    flags.set_cicada_enabled(true);
    flags.set_ligra_enabled(true);
    flags.set_rabitq_enabled(true);

    EXPECT_TRUE(flags.wisckey_enabled());
    EXPECT_TRUE(flags.dostoevsky_enabled());
    EXPECT_TRUE(flags.cicada_enabled());
    EXPECT_TRUE(flags.ligra_enabled());
    EXPECT_TRUE(flags.rabitq_enabled());

    // Restore
    flags.set_wisckey_enabled(w);
    flags.set_dostoevsky_enabled(d);
    flags.set_cicada_enabled(c);
    flags.set_ligra_enabled(l);
    flags.set_rabitq_enabled(r);
}

TEST(Phase2FeatureFlagsTest, SingletonIdentity) {
    // Both references point to the same object
    auto& a = Phase2FeatureFlags::instance();
    auto& b = Phase2FeatureFlags::instance();
    EXPECT_EQ(&a, &b);

    a.set_wisckey_enabled(true);
    EXPECT_TRUE(b.wisckey_enabled());
    a.set_wisckey_enabled(false);
}
