/**
 * @file test_model_integrity_wave2.cpp
 * @brief Wave 2-A / A1: Unit-Tests für ModelIntegrityVerifier.
 *
 * Verifiziert kryptografische SHA-256-Prüfung von Modelldateien, Manifest-Laden
 * und graceful degradation bei fehlendem Manifest.
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

#include "server/model_integrity_verifier.h"

namespace themis::server::test {

// ---------------------------------------------------------------------------
// Hilfsfunktionen
// ---------------------------------------------------------------------------

/// Schreibt @p content in eine temporäre Datei und gibt den Pfad zurück.
static std::string writeTempFile(const std::string& content, const std::string& suffix = ".bin") {
    std::string path = std::string(std::getenv("TMPDIR") != nullptr ? std::getenv("TMPDIR") : "/tmp")
                       + "/themis_test_" + suffix;
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f << content;
    return path;
}

// ---------------------------------------------------------------------------
// Testfixture
// ---------------------------------------------------------------------------

class ModelIntegrityVerifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Sicherstellen, dass der Manifest-Cache sauber ist
        ModelIntegrityVerifier::clearManifest();
    }
    void TearDown() override {
        ModelIntegrityVerifier::clearManifest();
    }
};

// ---------------------------------------------------------------------------
// Test 1: computeSha256 liefert bekannten Hash für definierten Inhalt
// ---------------------------------------------------------------------------

TEST_F(ModelIntegrityVerifierTest, ComputeSha256KnownValue) {
    // SHA-256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    const std::string path = writeTempFile("", "empty.bin");
    const std::string expected = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    const std::string actual   = ModelIntegrityVerifier::computeSha256(path);
    EXPECT_EQ(actual, expected);
    std::remove(path.c_str());
}

TEST_F(ModelIntegrityVerifierTest, ComputeSha256NonEmptyFile) {
    // SHA-256("hello") = 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824
    const std::string path = writeTempFile("hello", "hello.bin");
    const std::string expected = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";
    const std::string actual   = ModelIntegrityVerifier::computeSha256(path);
    EXPECT_EQ(actual, expected);
    std::remove(path.c_str());
}

// ---------------------------------------------------------------------------
// Test 2: verifyModel — korrekter Hash → true
// ---------------------------------------------------------------------------

TEST_F(ModelIntegrityVerifierTest, VerifyModelCorrectHash) {
    const std::string path = writeTempFile("hello", "verify_ok.bin");
    const std::string expected = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";
    EXPECT_TRUE(ModelIntegrityVerifier::verifyModel(path, expected));
    std::remove(path.c_str());
}

// ---------------------------------------------------------------------------
// Test 3: verifyModel — falscher Hash → false (integrity failure)
// ---------------------------------------------------------------------------

TEST_F(ModelIntegrityVerifierTest, VerifyModelWrongHashFails) {
    const std::string path = writeTempFile("hello", "verify_fail.bin");
    const std::string wrong_hash(64, '0'); // all-zero hex string
    EXPECT_FALSE(ModelIntegrityVerifier::verifyModel(path, wrong_hash));
    std::remove(path.c_str());
}

// ---------------------------------------------------------------------------
// Test 4: Manifest laden — getExpectedHash liefert korrekten Wert
// ---------------------------------------------------------------------------

TEST_F(ModelIntegrityVerifierTest, LoadManifestAndGetHash) {
    const std::string manifest_content = R"({
        "models": {
            "test-model-v1": {
                "sha256": "abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890",
                "path": "/models/test-model-v1.bin"
            }
        }
    })";
    const std::string mpath = writeTempFile(manifest_content, "manifest.json");

    ASSERT_TRUE(ModelIntegrityVerifier::loadManifest(mpath));

    auto hash = ModelIntegrityVerifier::getExpectedHash("test-model-v1");
    ASSERT_TRUE(hash.has_value());
    EXPECT_EQ(*hash, "abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890");

    std::remove(mpath.c_str());
}

// ---------------------------------------------------------------------------
// Test 5: Fehlendes Manifest → graceful degradation (kein Hash, kein Absturz)
// ---------------------------------------------------------------------------

TEST_F(ModelIntegrityVerifierTest, MissingManifestGracefulDegradation) {
    // Kein Manifest geladen → getExpectedHash gibt nullopt zurück
    auto hash = ModelIntegrityVerifier::getExpectedHash("nonexistent-model");
    EXPECT_FALSE(hash.has_value());

    // loadManifest auf nicht-existenter Datei → false, kein throw
    EXPECT_FALSE(ModelIntegrityVerifier::loadManifest("/tmp/nonexistent_manifest_xyz.json"));
}

// ---------------------------------------------------------------------------
// Test 6: verifyModel mit leerem Pfad → false (kein SIGSEGV)
// ---------------------------------------------------------------------------

TEST_F(ModelIntegrityVerifierTest, VerifyModelEmptyPathReturnsFalse) {
    EXPECT_FALSE(ModelIntegrityVerifier::verifyModel("", "abc123"));
    EXPECT_FALSE(ModelIntegrityVerifier::verifyModel("/tmp/does_not_exist.bin", "abc123"));
}

}  // namespace themis::server::test
