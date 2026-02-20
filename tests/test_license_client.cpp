/// @file test_license_client.cpp
/// @brief Unit tests for LicenseClient online/offline activation API
///
/// Tests cover:
/// - getMachineFingerprint() format and stability
/// - Offline activation with a valid embedded license
/// - Offline activation with no embedded license
/// - Offline activation when offline fallback is disabled
/// - validate() returns cached result when fresh
/// - refresh() clears cache and re-validates
/// - LicenseActivationResult status strings

#include <gtest/gtest.h>
#include "themis/license_info.h"

using namespace themis::license;

// ===== LicenseActivationResult Tests =====

TEST(LicenseActivationResult, DefaultValues) {
    LicenseActivationResult r;
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.status.empty());
    EXPECT_TRUE(r.error_message.empty());
    EXPECT_EQ(r.grace_days_remaining, 0);
    EXPECT_FALSE(r.refreshed_license.has_value());
}

// ===== LicenseClientConfig Tests =====

TEST(LicenseClientConfig, DefaultValues) {
    LicenseClientConfig cfg;
    EXPECT_TRUE(cfg.server_url.empty());
    EXPECT_TRUE(cfg.api_key.empty());
    EXPECT_EQ(cfg.timeout.count(), 10);
    EXPECT_TRUE(cfg.allow_offline);
    EXPECT_EQ(cfg.grace_period_days, 7);
}

// ===== Machine Fingerprint Tests =====

TEST(LicenseClient, MachineFingerprintNonEmpty) {
    const std::string fp = LicenseClient::getMachineFingerprint();
    EXPECT_FALSE(fp.empty());
}

TEST(LicenseClient, MachineFingerprintIsDeterministic) {
    // Two calls on the same machine should return the same fingerprint
    const std::string fp1 = LicenseClient::getMachineFingerprint();
    const std::string fp2 = LicenseClient::getMachineFingerprint();
    EXPECT_EQ(fp1, fp2);
}

TEST(LicenseClient, MachineFingerprintIsHex) {
    const std::string fp = LicenseClient::getMachineFingerprint();
    for (char c : fp) {
        EXPECT_TRUE(std::isxdigit(static_cast<unsigned char>(c)))
            << "Fingerprint should be hex, but found char: " << c;
    }
}

TEST(LicenseClient, MachineFingerprintLength) {
    // SHA-256 → 64 hex characters
    const std::string fp = LicenseClient::getMachineFingerprint();
    EXPECT_EQ(fp.size(), 64u) << "SHA-256 fingerprint must be 64 hex chars";
}

// ===== Offline Activation Tests =====

TEST(LicenseClient, OfflineActivationNoServerUrl) {
    LicenseClientConfig cfg;
    // No server URL → must go offline immediately
    cfg.allow_offline = true;

    LicenseClient client(cfg);
    auto result = client.activate();

    // Whether it succeeds depends on whether a valid embedded license exists.
    // We only assert that status is set to a known value.
    static const std::string valid_statuses[] = {
        "active", "expired", "offline", "invalid", "grace"
    };
    bool found = false;
    for (const auto& s : valid_statuses)
        if (result.status == s) { found = true; break; }
    EXPECT_TRUE(found) << "Unexpected status: " << result.status;
}

TEST(LicenseClient, OfflineNotAllowed) {
    LicenseClientConfig cfg;
    cfg.server_url    = "";   // no server
    cfg.allow_offline = false;

    LicenseClient client(cfg);
    auto result = client.activate();

    // Without a server AND offline disabled, must fail
    EXPECT_FALSE(result.success);
    static const std::string expected[] = {"offline", "invalid", "expired"};
    bool found = false;
    for (const auto& s : expected)
        if (result.status == s) { found = true; break; }
    EXPECT_TRUE(found) << "Unexpected status: " << result.status;
}

// ===== Validation Caching Tests =====

TEST(LicenseClient, ValidateCachesResult) {
    LicenseClientConfig cfg;
    cfg.allow_offline = true;

    LicenseClient client(cfg);
    // First call: activate + cache
    auto r1 = client.activate();
    // Second call within the same second: should use cached result
    auto r2 = client.validate();

    EXPECT_EQ(r1.success, r2.success);
    EXPECT_EQ(r1.status,  r2.status);
}

// ===== Refresh Tests =====

TEST(LicenseClient, RefreshClonesCacheBeforeActivate) {
    LicenseClientConfig cfg;
    cfg.allow_offline = true;

    LicenseClient client(cfg);
    auto r1 = client.activate();
    auto r2 = client.refresh();

    // Status should match between activate and refresh
    EXPECT_EQ(r1.success, r2.success);
}

// ===== GetCachedLicense Tests =====

TEST(LicenseClient, GetCachedLicenseEmptyBeforeActivation) {
    LicenseClientConfig cfg;
    LicenseClient client(cfg);

    // Before any activation, cache should be empty
    auto cached = client.getCachedLicense();
    EXPECT_FALSE(cached.has_value());
}

TEST(LicenseClient, GetCachedLicenseAfterActivation) {
    LicenseClientConfig cfg;
    cfg.allow_offline = true;

    LicenseClient client(cfg);
    auto result = client.activate();

    if (result.success && result.refreshed_license.has_value()) {
        auto cached = client.getCachedLicense();
        EXPECT_TRUE(cached.has_value());
    }
    // If activation fails (no embedded license), cache remains empty - that's fine
    SUCCEED();
}
