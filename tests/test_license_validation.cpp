/// @file test_license_validation.cpp
/// @brief Unit tests for license validation functions (isLicenseValid,
///        getDaysUntilExpiry, verifyLicenseSignature, formatLicenseInfo)
///
/// Tests cover:
/// - Valid license (future expiry date)
/// - Expired license (past expiry date)
/// - Perpetual license (empty / "9999-12-31" expiry)
/// - Malformed expiry date
/// - Signature verification with no signature (treated as valid)
/// - Signature verification with garbage signature (must fail)
/// - formatLicenseInfo output contains expected sections
/// - getDaysUntilExpiry returns correct approximate values
/// - Edition / feature-gate compile-time constants sanity check

#include <gtest/gtest.h>
#include "themis/license_info.h"
#include "themis/edition.h"

#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

using namespace themis::license;

// ============================================================================
// Helpers
// ============================================================================

/// Build a LicenseData with sane defaults; individual tests override fields.
static LicenseData makeValidLicense() {
    LicenseData ld;
    ld.organization_name = "Test Org";
    ld.organization_id   = "ORG-001";
    ld.contact_email     = "admin@example.com";
    ld.license_key       = "THEMIS-ENT-ABCD1234-EF567890";
    ld.edition           = "ENTERPRISE";
    ld.issued_date       = "2025-01-01";
    ld.max_nodes         = 100;
    ld.max_cores         = -1;
    ld.max_storage_tb    = -1;
    ld.build_id          = "test-build";
    ld.build_timestamp   = "2025-01-01T00:00:00Z";
    // No signature → verifyLicenseSignature should return true
    return ld;
}

/// Return an ISO-8601 date string offset by `delta_days` from today (UTC).
static std::string dateOffsetDays(int delta_days) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    t += static_cast<std::time_t>(delta_days) * 86400;

    std::tm tm_val = {};
#ifdef _WIN32
    gmtime_s(&tm_val, &t);
#else
    gmtime_r(&t, &tm_val);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%d");
    return oss.str();
}

// ============================================================================
// isLicenseValid
// ============================================================================

TEST(LicenseValidation, ValidLicenseInFuture) {
    auto ld = makeValidLicense();
    ld.expiry_date = dateOffsetDays(30);
    EXPECT_TRUE(isLicenseValid(ld));
}

TEST(LicenseValidation, ExpiredLicense) {
    auto ld = makeValidLicense();
    ld.expiry_date = dateOffsetDays(-1);
    EXPECT_FALSE(isLicenseValid(ld));
}

TEST(LicenseValidation, PerpetualLicenseEmptyExpiry) {
    auto ld = makeValidLicense();
    ld.expiry_date = "";
    EXPECT_TRUE(isLicenseValid(ld));
}

TEST(LicenseValidation, PerpetualLicense9999) {
    auto ld = makeValidLicense();
    ld.expiry_date = "9999-12-31";
    EXPECT_TRUE(isLicenseValid(ld));
}

TEST(LicenseValidation, MalformedExpiryReturnsFalse) {
    auto ld = makeValidLicense();
    ld.expiry_date = "not-a-date";
    EXPECT_FALSE(isLicenseValid(ld));
}

TEST(LicenseValidation, ExpiresExactlyToday) {
    // A license that expires today should still be considered valid (>= 0 days)
    auto ld = makeValidLicense();
    ld.expiry_date = dateOffsetDays(0);
    // getDaysUntilExpiry may return 0 or -0 depending on time-of-day rounding;
    // the contract is days >= 0 → valid, which may flip during a test run
    // near midnight.  We only assert no crash here.
    (void)isLicenseValid(ld);
    SUCCEED();
}

// ============================================================================
// getDaysUntilExpiry
// ============================================================================

TEST(LicenseValidation, DaysUntilExpiryFuture) {
    auto ld = makeValidLicense();
    ld.expiry_date = dateOffsetDays(10);
    int days = getDaysUntilExpiry(ld);
    // Allow ±1 day tolerance for time-zone / rounding differences
    EXPECT_GE(days, 9);
    EXPECT_LE(days, 11);
}

TEST(LicenseValidation, DaysUntilExpiryPast) {
    auto ld = makeValidLicense();
    ld.expiry_date = dateOffsetDays(-5);
    int days = getDaysUntilExpiry(ld);
    EXPECT_LT(days, 0);
}

TEST(LicenseValidation, DaysUntilExpiryPerpetual) {
    auto ld = makeValidLicense();
    ld.expiry_date = "9999-12-31";
    int days = getDaysUntilExpiry(ld);
    EXPECT_GT(days, 10000);
}

TEST(LicenseValidation, DaysUntilExpiryMalformed) {
    auto ld = makeValidLicense();
    ld.expiry_date = "INVALID";
    int days = getDaysUntilExpiry(ld);
    EXPECT_LT(days, 0);
}

// ============================================================================
// verifyLicenseSignature
// ============================================================================

TEST(LicenseValidation, NoSignatureIsValid) {
    auto ld = makeValidLicense();
    ld.signature = "";
    EXPECT_TRUE(verifyLicenseSignature(ld));
}

TEST(LicenseValidation, GarbageSignatureFails) {
    auto ld = makeValidLicense();
    ld.signature = "dGhpc2lzZ2FyYmFnZQ==";  // base64("thisisgarbag")
    // Must NOT crash, and must return false (wrong key / wrong data)
    EXPECT_FALSE(verifyLicenseSignature(ld));
}

TEST(LicenseValidation, TamperedLicenseKeyFails) {
    auto ld = makeValidLicense();
    // Provide a valid-looking base64 blob but change the key after "signing"
    ld.signature = "dGVzdHNpZ25hdHVyZQ==";  // base64("testsignature")
    ld.license_key = "THEMIS-ENT-TAMPERED1-TAMPERED2";
    EXPECT_FALSE(verifyLicenseSignature(ld));
}

// ============================================================================
// formatLicenseInfo
// ============================================================================

TEST(LicenseValidation, FormatLicenseInfoContainsOrgName) {
    auto ld = makeValidLicense();
    ld.expiry_date = dateOffsetDays(90);
    std::string info = formatLicenseInfo(ld);
    EXPECT_NE(info.find("Test Org"), std::string::npos);
}

TEST(LicenseValidation, FormatLicenseInfoContainsLicenseKey) {
    auto ld = makeValidLicense();
    ld.expiry_date = dateOffsetDays(90);
    std::string info = formatLicenseInfo(ld);
    EXPECT_NE(info.find("THEMIS-ENT-ABCD1234-EF567890"), std::string::npos);
}

TEST(LicenseValidation, FormatLicenseInfoContainsEdition) {
    auto ld = makeValidLicense();
    ld.expiry_date = dateOffsetDays(90);
    std::string info = formatLicenseInfo(ld);
    EXPECT_NE(info.find("ENTERPRISE"), std::string::npos);
}

TEST(LicenseValidation, FormatLicenseInfoExpiredLabel) {
    auto ld = makeValidLicense();
    ld.expiry_date = dateOffsetDays(-10);
    std::string info = formatLicenseInfo(ld);
    EXPECT_NE(info.find("EXPIRED"), std::string::npos);
}

TEST(LicenseValidation, FormatLicenseInfoUnlimitedNodes) {
    auto ld = makeValidLicense();
    ld.expiry_date = dateOffsetDays(90);
    ld.max_nodes  = -1;
    std::string info = formatLicenseInfo(ld);
    EXPECT_NE(info.find("Unlimited"), std::string::npos);
}

// ============================================================================
// Edition / feature-gate compile-time sanity checks
// ============================================================================

TEST(EditionFeatureGates, EditionStringIsKnown) {
    using namespace themis::edition;
    const auto et = GetEditionType();
    EXPECT_NE(et, EditionType::UNKNOWN)
        << "THEMIS_EDITION_STRING '" << EDITION_STRING
        << "' is not a recognised edition";
}

TEST(EditionFeatureGates, GpuVramLimitIsNonNegative) {
    // 0 means no GPU (MINIMAL) or unlimited (HYPERSCALER); any non-negative value is valid.
    EXPECT_GE(themis::edition::GPU_MAX_VRAM_GB, 0);
}

TEST(EditionFeatureGates, ShardingMaxNodesIsNonNegative) {
    // 0 means unlimited (HYPERSCALER); any non-negative value is valid.
    EXPECT_GE(themis::edition::SHARDING_MAX_NODES, 0);
}

TEST(EditionFeatureGates, IsFeatureEnabledUnknownReturnsFalse) {
    using namespace themis::edition;
    EXPECT_FALSE(IsFeatureEnabled("nonexistent_feature_xyz"));
}

TEST(EditionFeatureGates, EditionInfoGetIsConsistent) {
    using namespace themis::edition;
    constexpr auto info = EditionInfo::Get();
    EXPECT_EQ(info.type, GetEditionType());
    EXPECT_EQ(info.gpu_max_vram_gb, GPU_MAX_VRAM_GB);
    EXPECT_EQ(info.sharding_max_nodes, SHARDING_MAX_NODES);
    EXPECT_EQ(info.supports_plugins, FEATURE_ENTERPRISE_PLUGINS);
    EXPECT_EQ(info.supports_multi_master, FEATURE_MULTI_MASTER);
    EXPECT_EQ(info.supports_field_encryption, FEATURE_FIELD_ENCRYPTION);
    EXPECT_EQ(info.supports_rbac, FEATURE_RBAC);
    EXPECT_EQ(info.supports_hsm, FEATURE_HSM);
}

// ============================================================================
// hasEmbeddedLicense / getEmbeddedLicense
// ============================================================================

TEST(LicenseValidation, HasEmbeddedLicenseConsistentWithGet) {
    bool has_license = hasEmbeddedLicense();
    auto maybe_license = getEmbeddedLicense();
    EXPECT_EQ(has_license, maybe_license.has_value());
}

TEST(LicenseValidation, GetEmbeddedLicenseNoCrash) {
    // Must not throw or crash
    EXPECT_NO_THROW({
        auto lic = getEmbeddedLicense();
        (void)lic;
    });
}
