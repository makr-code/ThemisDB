/**
 * @file test_fips_crypto_mode.cpp
 * @brief Unit tests for FipsCryptoMode — FIPS 140-2/3 validated cryptography mode.
 *
 * These tests are self-contained and do NOT require a FIPS-validated OpenSSL
 * build to pass.  When the FIPS provider is not available the tests verify
 * graceful degradation.  When it IS available the tests additionally verify
 * correct activation, self-test execution, and algorithm enforcement.
 */

#include <gtest/gtest.h>
#include "security/fips_crypto_mode.h"

#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// Helpers
// ============================================================================

static bool fipsAvailable() {
    return FipsCryptoMode::instance().isAvailable();
}

// ============================================================================
// FipsCryptoMode — availability and activation
// ============================================================================

TEST(FipsCryptoModeTest, IsAvailableReturnsBool) {
    // Should not throw regardless of whether FIPS provider is installed.
    bool available = false;
    EXPECT_NO_THROW(available = FipsCryptoMode::instance().isAvailable());
    (void)available;
}

TEST(FipsCryptoModeTest, InitiallyDisabled) {
    // Fresh process should not have FIPS active by default.
    // (Re-disable to reset any prior test state.)
    FipsCryptoMode::instance().disable();
    EXPECT_FALSE(FipsCryptoMode::instance().isEnabled());
}

TEST(FipsCryptoModeTest, EnableReturnsFalseWhenProviderMissing) {
    if (fipsAvailable()) {
        GTEST_SKIP() << "FIPS provider is installed; skipping 'missing' path";
    }
    // disable first in case a previous test left it on
    FipsCryptoMode::instance().disable();
    bool result = false;
    EXPECT_NO_THROW(result = FipsCryptoMode::instance().enable());
    EXPECT_FALSE(result);
    EXPECT_FALSE(FipsCryptoMode::instance().isEnabled());
}

TEST(FipsCryptoModeTest, EnableSucceedsWhenProviderAvailable) {
    if (!fipsAvailable()) {
        GTEST_SKIP() << "FIPS provider not installed; skipping activation test";
    }
    FipsCryptoMode::instance().disable();
    bool result = false;
    EXPECT_NO_THROW(result = FipsCryptoMode::instance().enable());
    EXPECT_TRUE(result);
    EXPECT_TRUE(FipsCryptoMode::instance().isEnabled());
    // Clean up
    FipsCryptoMode::instance().disable();
}

TEST(FipsCryptoModeTest, DoubleEnableIsIdempotent) {
    if (!fipsAvailable()) {
        GTEST_SKIP() << "FIPS provider not installed";
    }
    FipsCryptoMode::instance().disable();
    EXPECT_TRUE(FipsCryptoMode::instance().enable());
    EXPECT_NO_THROW(FipsCryptoMode::instance().enable());  // second call — must not throw
    EXPECT_TRUE(FipsCryptoMode::instance().isEnabled());
    FipsCryptoMode::instance().disable();
}

TEST(FipsCryptoModeTest, DisableWhenNotActivedIsNoop) {
    FipsCryptoMode::instance().disable();  // first disable
    EXPECT_NO_THROW(FipsCryptoMode::instance().disable());  // second — no-op
    EXPECT_FALSE(FipsCryptoMode::instance().isEnabled());
}

TEST(FipsCryptoModeTest, EnableThenDisable) {
    if (!fipsAvailable()) {
        GTEST_SKIP() << "FIPS provider not installed";
    }
    FipsCryptoMode::instance().disable();
    ASSERT_TRUE(FipsCryptoMode::instance().enable());
    EXPECT_TRUE(FipsCryptoMode::instance().isEnabled());
    FipsCryptoMode::instance().disable();
    EXPECT_FALSE(FipsCryptoMode::instance().isEnabled());
}

// ============================================================================
// FipsCryptoMode — approved algorithm validation
// ============================================================================

TEST(FipsCryptoModeTest, ApprovedAlgorithmsSetNotEmpty) {
    EXPECT_FALSE(FipsCryptoMode::instance().approvedAlgorithms().empty());
}

TEST(FipsCryptoModeTest, ValidateApprovedAlgorithmsNoThrow) {
    static const std::vector<std::string> approved = {
        "AES-256-GCM", "AES-128-CBC", "AES-256-CBC",
        "AES-256-CTR", "AES-128-GCM",
        "SHA-256", "SHA-384", "SHA-512",
        "HMAC-SHA-256", "HMAC-SHA-384", "HMAC-SHA-512",
        "RSA-2048", "RSA-3072", "RSA-4096",
        "ECDSA-P256", "ECDSA-P384", "ECDSA-P521",
        "ECDH-P256", "ECDH-P384", "ECDH-P521",
        "PBKDF2-SHA-256", "HKDF-SHA-256",
        "CTR_DRBG", "HASH_DRBG", "HMAC_DRBG",
    };
    for (const auto& algo : approved) {
        EXPECT_NO_THROW(FipsCryptoMode::instance().validateAlgorithm(algo))
            << "Algorithm '" << algo << "' should be approved";
    }
}

TEST(FipsCryptoModeTest, ValidateCaseInsensitive) {
    // Algorithm names are accepted in any case.
    EXPECT_NO_THROW(FipsCryptoMode::instance().validateAlgorithm("aes-256-gcm"));
    EXPECT_NO_THROW(FipsCryptoMode::instance().validateAlgorithm("Aes-256-Gcm"));
    EXPECT_NO_THROW(FipsCryptoMode::instance().validateAlgorithm("sha-256"));
}

TEST(FipsCryptoModeTest, ValidateNonApprovedThrows) {
    static const std::vector<std::string> not_approved = {
        "MD5",
        "RC4",
        "DES",
        "3DES",
        "BLOWFISH",
        "CHACHA20-POLY1305",
        "SHA-1",
        "IDEA",
        "ARC4",
    };
    for (const auto& algo : not_approved) {
        EXPECT_THROW(
            FipsCryptoMode::instance().validateAlgorithm(algo),
            FipsPolicyViolation)
            << "Algorithm '" << algo << "' should throw FipsPolicyViolation";
    }
}

TEST(FipsCryptoModeTest, ValidateEmptyStringThrows) {
    EXPECT_THROW(
        FipsCryptoMode::instance().validateAlgorithm(""),
        FipsPolicyViolation);
}

TEST(FipsCryptoModeTest, ValidateUnknownAlgorithmThrows) {
    EXPECT_THROW(
        FipsCryptoMode::instance().validateAlgorithm("UNKNOWN-ALGO-999"),
        FipsPolicyViolation);
}

// ============================================================================
// FipsCryptoMode — self-tests
// ============================================================================

TEST(FipsCryptoModeTest, SelfTestsReturnFalseWhenDisabled) {
    FipsCryptoMode::instance().disable();
    // Self-tests cannot run without the provider loaded.
    EXPECT_FALSE(FipsCryptoMode::instance().runSelfTests());
}

TEST(FipsCryptoModeTest, SelfTestsPassWhenEnabled) {
    if (!fipsAvailable()) {
        GTEST_SKIP() << "FIPS provider not installed";
    }
    FipsCryptoMode::instance().disable();
    ASSERT_TRUE(FipsCryptoMode::instance().enable());
    EXPECT_TRUE(FipsCryptoMode::instance().runSelfTests());
    FipsCryptoMode::instance().disable();
}

// ============================================================================
// FipsCryptoMode — zeroization
// ============================================================================

TEST(FipsCryptoModeTest, ZeroizeErasesMemory) {
    std::vector<uint8_t> secret = {0x01, 0x02, 0x03, 0x04, 0x05};
    FipsCryptoMode::zeroize(secret.data(), secret.size());
    for (auto byte : secret) {
        EXPECT_EQ(byte, 0x00);
    }
}

TEST(FipsCryptoModeTest, ZeroizeNullptrIsNoop) {
    EXPECT_NO_THROW(FipsCryptoMode::zeroize(nullptr, 16));
}

TEST(FipsCryptoModeTest, ZeroizeZeroLengthIsNoop) {
    uint8_t buf[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    EXPECT_NO_THROW(FipsCryptoMode::zeroize(buf, 0));
    // Buffer should be unchanged.
    EXPECT_EQ(buf[0], 0xAAu);
}

// ============================================================================
// FipsCryptoMode — singleton identity
// ============================================================================

TEST(FipsCryptoModeTest, SingletonReturnsSameInstance) {
    FipsCryptoMode* a = &FipsCryptoMode::instance();
    FipsCryptoMode* b = &FipsCryptoMode::instance();
    EXPECT_EQ(a, b);
}

// ============================================================================
// FipsCryptoMode — approved algorithms completeness
// ============================================================================

TEST(FipsCryptoModeTest, AllCoreAlgorithmsPresent) {
    const auto& approved = FipsCryptoMode::instance().approvedAlgorithms();

    // Core symmetric
    EXPECT_TRUE(approved.count("AES-256-GCM"));
    EXPECT_TRUE(approved.count("AES-128-GCM"));
    EXPECT_TRUE(approved.count("AES-256-CBC"));

    // Core hash
    EXPECT_TRUE(approved.count("SHA-256"));
    EXPECT_TRUE(approved.count("SHA-384"));
    EXPECT_TRUE(approved.count("SHA-512"));

    // Core MAC
    EXPECT_TRUE(approved.count("HMAC-SHA-256"));
    EXPECT_TRUE(approved.count("HMAC-SHA-512"));

    // Core asymmetric
    EXPECT_TRUE(approved.count("RSA-2048"));
    EXPECT_TRUE(approved.count("ECDSA-P256"));
    EXPECT_TRUE(approved.count("ECDH-P256"));

    // Core KDF
    EXPECT_TRUE(approved.count("HKDF-SHA-256"));
    EXPECT_TRUE(approved.count("PBKDF2-SHA-256"));

    // Core DRBG
    EXPECT_TRUE(approved.count("CTR_DRBG"));
}
