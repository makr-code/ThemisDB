/// @file test_gate_result.cpp
/// @brief Focused unit tests for GateResult, LicenseDenialReason, and LicenseInfo
///        (v1.7.1 additions to include/themis/runtime_license_gate.h and
///        include/themis/license_info.h).
///
/// Test groups:
///  1. GateResult defaults
///  2. GateResult::message() for every LicenseDenialReason value
///  3. GateResult operator bool()
///  4. RuntimeLicenseGate::checkFeature() — community/unknown features
///  5. RuntimeLicenseGate::checkFeature() — compile-time gate (TIER_TOO_LOW)
///  6. RuntimeLicenseGate::checkFeature() — active license
///  7. RuntimeLicenseGate::checkFeature() — expired license
///  8. RuntimeLicenseGate::checkFeature() — invalid/offline license (SIGNATURE_MISMATCH)
///  9. RuntimeLicenseGate::checkFeature() — uninitialized gate
/// 10. LicenseInfo::remaining_grace_days() boundary cases

#include <gtest/gtest.h>
#include "themis/runtime_license_gate.h"
#include "themis/license_info.h"
#include "themis/edition.h"

#include <ctime>
#include <string>

using namespace themis::license;

// ============================================================================
// Helpers
// ============================================================================

static LicenseActivationResult makeActivation(bool success,
                                               const std::string& status,
                                               int grace_days = 0) {
    LicenseActivationResult r;
    r.success              = success;
    r.status               = status;
    r.grace_days_remaining = grace_days;
    return r;
}

static LicenseData makeLicenseData(const std::string& expiry_date,
                                   const std::string& edition = "ENTERPRISE") {
    LicenseData ld;
    ld.organization_name = "TestOrg";
    ld.license_key       = "THEMIS-ENT-TEST-0000";
    ld.edition           = edition;
    ld.contact_email     = "support@test.invalid";
    ld.expiry_date       = expiry_date;
    ld.max_nodes         = 10;
    ld.max_cores         = -1;
    ld.max_storage_tb    = -1;
    return ld;
}

/// Returns a date string N days from today in YYYY-MM-DD format.
static std::string dateFromNow(int offset_days) {
    std::time_t now = std::time(nullptr);
    std::time_t target = now + static_cast<std::time_t>(offset_days) * 86400;
    struct tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &target);
#else
    gmtime_r(&target, &tm_buf);
#endif
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday);
    return buf;
}

// ============================================================================
// 1. GateResult defaults
// ============================================================================

TEST(GateResult, DefaultNotAllowed) {
    GateResult r;
    EXPECT_FALSE(r.allowed);
}

TEST(GateResult, DefaultDenialReasonIsNone) {
    GateResult r;
    EXPECT_EQ(r.denial_reason, LicenseDenialReason::NONE);
}

// ============================================================================
// 2. GateResult::message() for every LicenseDenialReason value
// ============================================================================

TEST(GateResult, MessageNone) {
    GateResult r;
    r.allowed       = true;
    r.denial_reason = LicenseDenialReason::NONE;
    EXPECT_EQ(r.message(), "Feature is allowed.");
}

TEST(GateResult, MessageTierTooLow) {
    GateResult r;
    r.allowed       = false;
    r.denial_reason = LicenseDenialReason::TIER_TOO_LOW;
    const auto msg = r.message();
    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("tier"), std::string::npos);
}

TEST(GateResult, MessageLicenseExpired) {
    GateResult r;
    r.allowed       = false;
    r.denial_reason = LicenseDenialReason::LICENSE_EXPIRED;
    const auto msg = r.message();
    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("expired"), std::string::npos);
}

TEST(GateResult, MessageSignatureMismatch) {
    GateResult r;
    r.allowed       = false;
    r.denial_reason = LicenseDenialReason::SIGNATURE_MISMATCH;
    const auto msg = r.message();
    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("signature"), std::string::npos);
}

TEST(GateResult, MessageNodeLimitExceeded) {
    GateResult r;
    r.allowed       = false;
    r.denial_reason = LicenseDenialReason::NODE_LIMIT_EXCEEDED;
    const auto msg = r.message();
    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("node"), std::string::npos);
}

TEST(GateResult, MessageStorageLimitExceeded) {
    GateResult r;
    r.allowed       = false;
    r.denial_reason = LicenseDenialReason::STORAGE_LIMIT_EXCEEDED;
    const auto msg = r.message();
    EXPECT_FALSE(msg.empty());
    EXPECT_NE(msg.find("storage"), std::string::npos);
}

// ============================================================================
// 3. GateResult operator bool()
// ============================================================================

TEST(GateResult, OperatorBoolTrue) {
    GateResult r;
    r.allowed = true;
    EXPECT_TRUE(static_cast<bool>(r));
    if (r) { SUCCEED(); } else { FAIL() << "operator bool should return true"; }
}

TEST(GateResult, OperatorBoolFalse) {
    GateResult r;
    r.allowed = false;
    EXPECT_FALSE(static_cast<bool>(r));
}

// ============================================================================
// 4. RuntimeLicenseGate::checkFeature() — community/unknown features
// ============================================================================

class GateResultCheckFeatureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset gate to a known state: active Enterprise license
        auto act = makeActivation(true, "active");
        act.refreshed_license = makeLicenseData("2099-12-31");
        RuntimeLicenseGate::instance().initialize(act, act.refreshed_license);
    }
};

TEST_F(GateResultCheckFeatureTest, CommunityFeatureAlwaysAllowed) {
    // "community_query" is not in the Enterprise gate list → always allowed
    auto r = RuntimeLicenseGate::instance().checkFeature("community_query");
    EXPECT_TRUE(r.allowed);
    EXPECT_EQ(r.denial_reason, LicenseDenialReason::NONE);
}

TEST_F(GateResultCheckFeatureTest, UnknownFeatureNameAlwaysAllowed) {
    auto r = RuntimeLicenseGate::instance().checkFeature("no_such_feature_xyz");
    EXPECT_TRUE(r.allowed);
    EXPECT_EQ(r.denial_reason, LicenseDenialReason::NONE);
}

// ============================================================================
// 5. RuntimeLicenseGate::checkFeature() — compile-time TIER_TOO_LOW
// ============================================================================

TEST_F(GateResultCheckFeatureTest, CompileTimeGateTierTooLow) {
    // If this binary was compiled without a gated Enterprise feature,
    // checkFeature() must return TIER_TOO_LOW.
    // We check for "hsm" which is always gated in Enterprise/Hyperscaler.
    auto r = RuntimeLicenseGate::instance().checkFeature("hsm");
    if (!themis::edition::IsFeatureEnabled("hsm")) {
        EXPECT_FALSE(r.allowed);
        EXPECT_EQ(r.denial_reason, LicenseDenialReason::TIER_TOO_LOW);
    } else {
        // Enterprise/Hyperscaler binary: HSM should be allowed with active license
        EXPECT_TRUE(r.allowed);
    }
}

// ============================================================================
// 6. RuntimeLicenseGate::checkFeature() — active license
// ============================================================================

TEST_F(GateResultCheckFeatureTest, ActiveLicenseAllowsFeatureWhenCompileTimeOn) {
    // Any feature enabled at compile-time should be allowed with active license
    for (const char* feat : {"enterprise_plugins", "multi_master",
                             "field_encryption", "rbac", "hsm"}) {
        if (themis::edition::IsFeatureEnabled(feat)) {
            auto r = RuntimeLicenseGate::instance().checkFeature(feat);
            EXPECT_TRUE(r.allowed) << "Feature " << feat << " should be allowed";
            EXPECT_EQ(r.denial_reason, LicenseDenialReason::NONE) << feat;
        }
    }
}

// ============================================================================
// 7. RuntimeLicenseGate::checkFeature() — expired license → LICENSE_EXPIRED
// ============================================================================

TEST(GateResultCheckFeatureExpired, ExpiredLicenseGivesLicenseExpiredReason) {
    auto act = makeActivation(false, "expired");
    RuntimeLicenseGate::instance().initialize(act);

    // Pick a feature that is in the Enterprise gate list
    const char* feat = "field_encryption";
    if (!themis::edition::IsFeatureEnabled(feat)) {
        // Community binary: denial is TIER_TOO_LOW not LICENSE_EXPIRED
        auto r = RuntimeLicenseGate::instance().checkFeature(feat);
        EXPECT_FALSE(r.allowed);
        EXPECT_EQ(r.denial_reason, LicenseDenialReason::TIER_TOO_LOW);
    } else {
        auto r = RuntimeLicenseGate::instance().checkFeature(feat);
        EXPECT_FALSE(r.allowed);
        EXPECT_EQ(r.denial_reason, LicenseDenialReason::LICENSE_EXPIRED);
    }
}

// ============================================================================
// 8. RuntimeLicenseGate::checkFeature() — invalid/offline → SIGNATURE_MISMATCH
// ============================================================================

TEST(GateResultCheckFeatureInvalid, InvalidLicenseGivesSignatureMismatch) {
    auto act = makeActivation(false, "invalid");
    RuntimeLicenseGate::instance().initialize(act);

    const char* feat = "rbac";
    if (!themis::edition::IsFeatureEnabled(feat)) {
        auto r = RuntimeLicenseGate::instance().checkFeature(feat);
        EXPECT_EQ(r.denial_reason, LicenseDenialReason::TIER_TOO_LOW);
    } else {
        auto r = RuntimeLicenseGate::instance().checkFeature(feat);
        EXPECT_FALSE(r.allowed);
        EXPECT_EQ(r.denial_reason, LicenseDenialReason::SIGNATURE_MISMATCH);
    }
}

// ============================================================================
// 9. RuntimeLicenseGate::checkFeature() — uninitialized gate
// ============================================================================

TEST(GateResultCheckFeatureUninitialized, UninitializedGivesSignatureMismatch) {
    // Re-initialise with a never-active result to simulate pre-startup state
    // (We can't truly un-initialize the singleton, so we use an invalid result)
    auto act = makeActivation(false, "offline");
    RuntimeLicenseGate::instance().initialize(act);

    const char* feat = "multi_master";
    if (!themis::edition::IsFeatureEnabled(feat)) {
        // Binary doesn't have the feature → TIER_TOO_LOW
        auto r = RuntimeLicenseGate::instance().checkFeature(feat);
        EXPECT_EQ(r.denial_reason, LicenseDenialReason::TIER_TOO_LOW);
    } else {
        auto r = RuntimeLicenseGate::instance().checkFeature(feat);
        EXPECT_FALSE(r.allowed);
        // "offline" maps to SIGNATURE_MISMATCH
        EXPECT_EQ(r.denial_reason, LicenseDenialReason::SIGNATURE_MISMATCH);
    }
}

// ============================================================================
// 10. LicenseInfo::remaining_grace_days() boundary cases
// ============================================================================

TEST(LicenseInfo, NotYetExpiredReturnsFullGracePeriod) {
    // Expiry is 30 days from now → license is still valid
    LicenseData ld = makeLicenseData(dateFromNow(30));
    LicenseInfo info(ld, 7);
    EXPECT_EQ(info.remaining_grace_days(), 7);
}

TEST(LicenseInfo, ExpiryTodayReturnsFullGracePeriod) {
    // Expiry is today (0 days) → still within valid window (not yet past)
    // The test accepts either 7 (not yet expired) or 6 (just expired today).
    LicenseData ld = makeLicenseData(dateFromNow(0));
    LicenseInfo info(ld, 7);
    int days = info.remaining_grace_days();
    EXPECT_GE(days, 6);
    EXPECT_LE(days, 7);
}

TEST(LicenseInfo, ExpiredOneDayAgoReturnsGraceMinus1) {
    LicenseData ld = makeLicenseData(dateFromNow(-1));
    LicenseInfo info(ld, 7);
    int days = info.remaining_grace_days();
    // 1 day elapsed → 7-1 = 6 remaining (allow ±1 for clock boundary)
    EXPECT_GE(days, 5);
    EXPECT_LE(days, 7);
}

TEST(LicenseInfo, ExpiredBeyondGracePeriodReturnsZero) {
    // Expired 30 days ago with only 7-day grace → 0 remaining
    LicenseData ld = makeLicenseData(dateFromNow(-30));
    LicenseInfo info(ld, 7);
    EXPECT_EQ(info.remaining_grace_days(), 0);
}

TEST(LicenseInfo, EmptyExpiryDateReturnsZero) {
    LicenseData ld = makeLicenseData("");
    LicenseInfo info(ld, 7);
    EXPECT_EQ(info.remaining_grace_days(), 0);
}

TEST(LicenseInfo, CustomGracePeriodRespected) {
    // 14-day grace, license expired 5 days ago → 14-5 = 9 remaining
    LicenseData ld = makeLicenseData(dateFromNow(-5));
    LicenseInfo info(ld, 14);
    int days = info.remaining_grace_days();
    EXPECT_GE(days, 8);
    EXPECT_LE(days, 10);
}

TEST(LicenseInfo, ZeroGracePeriodAlwaysReturnsZero) {
    // With 0-day grace period, even unexpired licenses report 0
    LicenseData ld = makeLicenseData(dateFromNow(10));
    LicenseInfo info(ld, 0);
    EXPECT_EQ(info.remaining_grace_days(), 0);
}

TEST(LicenseInfo, FarFutureExpiryReturnsFullGrace) {
    LicenseData ld = makeLicenseData("2099-12-31");
    LicenseInfo info(ld, 7);
    EXPECT_EQ(info.remaining_grace_days(), 7);
}

TEST(LicenseInfo, DataAccessorReturnsReference) {
    LicenseData ld = makeLicenseData("2099-12-31");
    ld.organization_name = "CheckOrg";
    LicenseInfo info(ld, 7);
    EXPECT_EQ(info.data().organization_name, "CheckOrg");
}
