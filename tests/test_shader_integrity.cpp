// Test: Shader Integrity Verifier Tests (Phase 4.1)
// Validates SHA-256 based SPIR-V shader integrity verification.

#include <gtest/gtest.h>
#include "acceleration/shader_integrity.h"
#include <filesystem>
#include <vector>
#include <cstdint>
#include <fstream>
#include <cstdio>
#include <cstring>

using namespace themis::acceleration;

// ============================================================================
// Fixture: clears registry and strict mode before every test
// ============================================================================
class ShaderIntegrityTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& v = ShaderIntegrityVerifier::instance();
        v.clearRegistry();
        v.setStrictMode(false);
    }
    void TearDown() override {
        auto& v = ShaderIntegrityVerifier::instance();
        v.clearRegistry();
        v.setStrictMode(false);
    }
};

// ============================================================================
// SHA-256 utility
// ============================================================================

TEST_F(ShaderIntegrityTest, SHA256_EmptyInput_KnownHash) {
    // SHA-256("") is universally known
    auto h = ShaderIntegrityVerifier::sha256Hex(nullptr, 0);
    EXPECT_EQ(h, "e3b0c44298fc1c149afbf4c8996fb924"
                 "27ae41e4649b934ca495991b7852b855");
}

TEST_F(ShaderIntegrityTest, SHA256_OutputIs64HexChars) {
    const uint8_t data[4] = {0x03, 0x02, 0x23, 0x07};
    auto h = ShaderIntegrityVerifier::sha256Hex(data, sizeof(data));
    EXPECT_EQ(h.size(), 64u);
    // Must be lower-case hex
    for (char c : h) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
            << "Non hex char: " << c;
    }
}

TEST_F(ShaderIntegrityTest, SHA256_SameInputSameHash) {
    std::vector<uint32_t> spv = {0x07230203u, 0x00010000u, 0xDEADBEEFu};
    auto h1 = ShaderIntegrityVerifier::sha256Hex(spv);
    auto h2 = ShaderIntegrityVerifier::sha256Hex(spv);
    EXPECT_EQ(h1, h2);
    EXPECT_EQ(h1.size(), 64u);
}

TEST_F(ShaderIntegrityTest, SHA256_DifferentInputDifferentHash) {
    std::vector<uint32_t> a = {0x07230203u, 0x00010000u};
    std::vector<uint32_t> b = {0x07230203u, 0x00010001u};
    EXPECT_NE(ShaderIntegrityVerifier::sha256Hex(a),
              ShaderIntegrityVerifier::sha256Hex(b));
}

// ============================================================================
// Registry
// ============================================================================

TEST_F(ShaderIntegrityTest, RegisteredByName) {
    auto& v = ShaderIntegrityVerifier::instance();
    EXPECT_FALSE(v.isRegistered("test.spv"));
    v.registerExpectedHash("test.spv", std::string(64, 'a'));
    EXPECT_TRUE(v.isRegistered("test.spv"));
}

TEST_F(ShaderIntegrityTest, NormalisesToLowerCase) {
    auto& v = ShaderIntegrityVerifier::instance();
    std::vector<uint32_t> spv = {0x07230203u, 0x00010300u, 0xABCDEF01u};
    // Compute real hash (lower-case) and convert to upper-case
    std::string lower = ShaderIntegrityVerifier::sha256Hex(spv);
    std::string upper = lower;
    for (char& c : upper) if (c >= 'a' && c <= 'f') c = static_cast<char>(c - 'a' + 'A');
    // Register the upper-case version; verify should still match after normalisation
    v.registerExpectedHash("upper.spv", upper);
    auto r = v.verify("upper.spv", spv);
    EXPECT_TRUE(r.passed) << "Registry should normalise upper-case hash to lower-case";
}

// ============================================================================
// Verification — non-strict mode (no registered hash → pass with note)
// ============================================================================

TEST_F(ShaderIntegrityTest, NoRegisteredHash_NonStrict_Passes) {
    std::vector<uint32_t> spv = {0x07230203u, 0x00010000u};
    auto r = ShaderIntegrityVerifier::instance().verify("unknown.spv", spv);
    EXPECT_TRUE(r.passed);
    EXPECT_TRUE(r.expectedHash.empty());
    EXPECT_FALSE(r.actualHash.empty());
}

// ============================================================================
// Verification — strict mode (no registered hash → fail)
// ============================================================================

TEST_F(ShaderIntegrityTest, NoRegisteredHash_StrictMode_Fails) {
    auto& v = ShaderIntegrityVerifier::instance();
    v.setStrictMode(true);
    std::vector<uint32_t> spv = {0x07230203u};
    auto r = v.verify("missing.spv", spv);
    EXPECT_FALSE(r.passed);
    EXPECT_NE(r.message.find("strict"), std::string::npos);
}

// ============================================================================
// Verification — correct hash → pass
// ============================================================================

TEST_F(ShaderIntegrityTest, CorrectHash_Passes) {
    std::vector<uint32_t> spv = {0x07230203u, 0x00010300u, 0xCAFEBABEu};
    std::string expected = ShaderIntegrityVerifier::sha256Hex(spv);

    auto& v = ShaderIntegrityVerifier::instance();
    v.registerExpectedHash("l2_distance.comp.spv", expected);

    auto r = v.verify("l2_distance.comp.spv", spv);
    EXPECT_TRUE(r.passed);
    EXPECT_EQ(r.actualHash, expected);
    EXPECT_EQ(r.expectedHash, expected);
}

// ============================================================================
// Verification — tampered bytes → fail
// ============================================================================

TEST_F(ShaderIntegrityTest, TamperedBytes_Fails) {
    std::vector<uint32_t> original  = {0x07230203u, 0x00010300u, 0x12345678u};
    std::vector<uint32_t> tampered  = {0x07230203u, 0x00010300u, 0x87654321u};
    std::string expected = ShaderIntegrityVerifier::sha256Hex(original);

    auto& v = ShaderIntegrityVerifier::instance();
    v.registerExpectedHash("cosine_distance.comp.spv", expected);

    auto r = v.verify("cosine_distance.comp.spv", tampered);
    EXPECT_FALSE(r.passed);
    EXPECT_NE(r.actualHash, expected);
    EXPECT_NE(r.message.find("FAILED"), std::string::npos);
}

// ============================================================================
// Manifest loading
// ============================================================================

TEST_F(ShaderIntegrityTest, LoadManifest_ParsesValidLines) {
    // Write a temporary manifest
    const auto tmpPath = (std::filesystem::temp_directory_path() / "test_shader_manifest.txt").string();
    {
        std::ofstream f(tmpPath);
        f << "# This is a comment\n";
        f << "l2_distance.comp.spv     " << std::string(64, 'a') << "\n";
        f << "cosine_distance.comp.spv " << std::string(64, 'b') << "\n";
        f << "\n";  // empty line
        f << "bad_entry too_short_hash\n";  // too short hash — ignored
    }

    auto& v = ShaderIntegrityVerifier::instance();
    size_t count = v.loadManifest(tmpPath);
    EXPECT_EQ(count, 2u);
    EXPECT_TRUE(v.isRegistered("l2_distance.comp.spv"));
    EXPECT_TRUE(v.isRegistered("cosine_distance.comp.spv"));
    EXPECT_FALSE(v.isRegistered("bad_entry"));

    std::filesystem::remove(tmpPath);
}

TEST_F(ShaderIntegrityTest, LoadManifest_MissingFile_Returns0) {
    size_t count = ShaderIntegrityVerifier::instance()
                       .loadManifest("/tmp/nonexistent_manifest_xyz.txt");
    EXPECT_EQ(count, 0u);
}

// ============================================================================
// Raw-bytes overload
// ============================================================================

TEST_F(ShaderIntegrityTest, VerifyRawBytes_CorrectHash_Passes) {
    const uint8_t data[] = {0x03, 0x02, 0x23, 0x07};  // SPIR-V magic LE
    std::string expected = ShaderIntegrityVerifier::sha256Hex(data, sizeof(data));
    ShaderIntegrityVerifier::instance().registerExpectedHash("raw.spv", expected);

    auto r = ShaderIntegrityVerifier::instance().verify("raw.spv", data, sizeof(data));
    EXPECT_TRUE(r.passed);
}
