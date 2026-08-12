#include <gtest/gtest.h>
#include "security/usb_admin_authenticator.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>

using namespace themis::security;
namespace fs = std::filesystem;

class USBAdminAuthenticatorTest : public ::testing::Test {
protected:
    std::string test_mount_path_;
    std::string license_file_path_;
    
    void SetUp() override {
        // Create temporary directory for testing
        test_mount_path_ = (fs::temp_directory_path() / "themis_usb_test").string();
        fs::create_directories(test_mount_path_);
        license_file_path_ = test_mount_path_ + "/themis_admin.lic";
    }
    
    void TearDown() override {
        // Clean up test directory
        if (fs::exists(test_mount_path_)) {
            fs::remove_all(test_mount_path_);
        }
    }
    
    void createValidLicense(const std::string& hw_id = "TEST-HARDWARE-ID-12345678") {
        nlohmann::json license = {
            {"license_key", "THEMIS-ENT-ADMIN-TEST-12345678"},
            {"organization", "Test Organization"},
            {"hardware_id", hw_id},
            {"issued_date", "2026-01-01"},
            {"expiry_date", "2027-12-31"},
            {"admin_scopes", nlohmann::json::array({"admin", "config:write", "cdc:admin"})},
            {"signature", "TEST-SIGNATURE-12345"}
        };
        
        std::ofstream file(license_file_path_);
        file << license.dump(2);
        file.close();
    }
    
    void createExpiredLicense() {
        nlohmann::json license = {
            {"license_key", "THEMIS-ENT-ADMIN-TEST-12345678"},
            {"organization", "Test Organization"},
            {"hardware_id", "TEST-HARDWARE-ID-12345678"},
            {"issued_date", "2020-01-01"},
            {"expiry_date", "2021-12-31"},  // Expired
            {"admin_scopes", nlohmann::json::array({"admin", "config:write"})},
            {"signature", "TEST-SIGNATURE-12345"}
        };
        
        std::ofstream file(license_file_path_);
        file << license.dump(2);
        file.close();
    }
};

TEST_F(USBAdminAuthenticatorTest, InitializeSucceeds) {
    USBAdminConfig config;
    config.mount_path = test_mount_path_;
    config.require_usb_for_admin = true;
    
    USBAdminAuthenticator auth(config);
    EXPECT_TRUE(auth.initialize());
}

TEST_F(USBAdminAuthenticatorTest, NoUSBDetectedInitially) {
    USBAdminConfig config;
    config.mount_path = test_mount_path_;
    config.require_usb_for_admin = true;
    
    USBAdminAuthenticator auth(config);
    auth.initialize();
    
    // No license file created yet
    EXPECT_FALSE(auth.isAdminUSBPresent());
}

TEST_F(USBAdminAuthenticatorTest, ValidUSBDetected) {
    createValidLicense();
    
    USBAdminConfig config;
    config.mount_path = test_mount_path_;
    config.require_usb_for_admin = true;
    
    USBAdminAuthenticator auth(config);
    // Inject verifier: accept any license with non-empty key and signature.
    // Required because the embedded RSA public key is a placeholder and the
    // test license does not carry a real hardware-bound signature.
    auth.setLicenseVerifierFn([](const USBAdminLicense& lic, const std::string&) {
        return !lic.license_key.empty() && !lic.signature.empty();
    });
    auth.initialize();
    
    EXPECT_TRUE(auth.refreshUSBStatus());
    EXPECT_TRUE(auth.isAdminUSBPresent());
    
    auto license = auth.getCurrentLicense();
    ASSERT_TRUE(license.has_value());
    EXPECT_EQ(license->license_key, "THEMIS-ENT-ADMIN-TEST-12345678");
    EXPECT_EQ(license->organization, "Test Organization");
}

TEST_F(USBAdminAuthenticatorTest, AdminOperationAllowedWithValidUSB) {
    createValidLicense();
    
    USBAdminConfig config;
    config.mount_path = test_mount_path_;
    config.require_usb_for_admin = true;
    
    USBAdminAuthenticator auth(config);
    auth.setLicenseVerifierFn([](const USBAdminLicense& lic, const std::string&) {
        return !lic.license_key.empty() && !lic.signature.empty();
    });
    auth.initialize();
    auth.refreshUSBStatus();
    
    EXPECT_TRUE(auth.validateAdminOperation("admin", "test_user"));
    EXPECT_TRUE(auth.validateAdminOperation("config:write", "test_user"));
    EXPECT_TRUE(auth.validateAdminOperation("cdc:admin", "test_user"));
}

TEST_F(USBAdminAuthenticatorTest, AdminOperationDeniedWithoutUSB) {
    // No license file created
    
    USBAdminConfig config;
    config.mount_path = test_mount_path_;
    config.require_usb_for_admin = true;
    
    USBAdminAuthenticator auth(config);
    auth.initialize();
    
    // Should be denied - no USB present
    EXPECT_FALSE(auth.validateAdminOperation("admin", "test_user"));
    
    auto metrics = auth.getMetrics();
    EXPECT_GT(metrics.admin_ops_denied_no_usb, 0);
}

TEST_F(USBAdminAuthenticatorTest, AdminOperationDeniedWithExpiredLicense) {
    createExpiredLicense();
    
    USBAdminConfig config;
    config.mount_path = test_mount_path_;
    config.require_usb_for_admin = true;
    
    USBAdminAuthenticator auth(config);
    auth.initialize();
    auth.refreshUSBStatus();
    
    // Should be denied - license expired
    EXPECT_FALSE(auth.validateAdminOperation("admin", "test_user"));
    
    auto metrics = auth.getMetrics();
    EXPECT_GT(metrics.admin_ops_denied_expired, 0);
}

TEST_F(USBAdminAuthenticatorTest, AdminOperationDeniedForUnauthorizedScope) {
    createValidLicense();  // Has admin, config:write, cdc:admin
    
    USBAdminConfig config;
    config.mount_path = test_mount_path_;
    config.require_usb_for_admin = true;
    
    USBAdminAuthenticator auth(config);
    auth.initialize();
    auth.refreshUSBStatus();
    
    // Should be allowed for scopes in license
    EXPECT_TRUE(auth.validateAdminOperation("admin", "test_user"));
    
    // Should be denied for scope not in license
    EXPECT_FALSE(auth.validateAdminOperation("admin:unknown", "test_user"));
}

TEST_F(USBAdminAuthenticatorTest, LockoutAfterFailedAttempts) {
    // No license file created
    
    USBAdminConfig config;
    config.mount_path = test_mount_path_;
    config.require_usb_for_admin = true;
    config.max_validation_attempts = 3;
    config.lockout_duration = std::chrono::seconds(2);
    
    USBAdminAuthenticator auth(config);
    auth.initialize();
    
    // Make 3 failed attempts
    for (int i = 0; i < 3; i++) {
        EXPECT_FALSE(auth.validateAdminOperation("admin", "test_user"));
    }
    
    // Should now be locked out
    EXPECT_TRUE(auth.isLockedOut());
    
    // Even with valid USB, should be denied during lockout
    createValidLicense();
    auth.refreshUSBStatus();
    EXPECT_FALSE(auth.validateAdminOperation("admin", "test_user"));
    
    auto metrics = auth.getMetrics();
    EXPECT_GT(metrics.admin_ops_denied_lockout, 0);
}

TEST_F(USBAdminAuthenticatorTest, NoUSBRequirementWhenDisabled) {
    // No license file created
    
    USBAdminConfig config;
    config.mount_path = test_mount_path_;
    config.require_usb_for_admin = false;  // Disabled
    
    USBAdminAuthenticator auth(config);
    auth.initialize();
    
    // Should be allowed even without USB
    EXPECT_TRUE(auth.validateAdminOperation("admin", "test_user"));
    
    auto metrics = auth.getMetrics();
    EXPECT_GT(metrics.admin_ops_allowed, 0);
}

TEST_F(USBAdminAuthenticatorTest, MetricsTracking) {
    createValidLicense();
    
    USBAdminConfig config;
    config.mount_path = test_mount_path_;
    config.require_usb_for_admin = true;
    
    USBAdminAuthenticator auth(config);
    auth.setLicenseVerifierFn([](const USBAdminLicense& lic, const std::string&) {
        return !lic.license_key.empty() && !lic.signature.empty();
    });
    auth.initialize();
    
    auto metrics_before = auth.getMetrics();
    
    auth.refreshUSBStatus();
    auth.validateAdminOperation("admin", "test_user");
    
    auto metrics_after = auth.getMetrics();
    
    EXPECT_GT(metrics_after.usb_mount_checks, metrics_before.usb_mount_checks);
    EXPECT_GT(metrics_after.admin_ops_allowed, metrics_before.admin_ops_allowed);
}

TEST_F(USBAdminAuthenticatorTest, HardwareMismatchDeniesAccess) {
    // Create license with different hardware ID
    createValidLicense("DIFFERENT-HARDWARE-ID");
    
    USBAdminConfig config;
    config.mount_path = test_mount_path_;
    config.require_usb_for_admin = true;
    
    USBAdminAuthenticator auth(config);
    auth.initialize();
    
    // Should fail to load license due to hardware mismatch
    EXPECT_FALSE(auth.refreshUSBStatus());
    EXPECT_FALSE(auth.isAdminUSBPresent());
}

// ============================================================================
// Challenge-Response Security Tests
//
// These tests verify that validateChallengeResponse() correctly enforces:
//   - Challenge must have been issued by createChallenge()
//   - Challenge is one-time-use (replay protection)
//   - Challenge expires after challenge_ttl
//   - Response must be a valid HMAC-SHA256 of the challenge
// ============================================================================

/**
 * @brief Helper: compute HMAC-SHA256(key, message) as lowercase hex.
 *
 * Mirrors the production implementation so tests can generate valid responses.
 */
static std::string computeHmacResponse(const std::string& license_key,
                                            const std::string& challenge) {
    unsigned char hmac_out[EVP_MAX_MD_SIZE];
    unsigned int  hmac_len = 0;
    HMAC(EVP_sha256(),
         license_key.data(), static_cast<int>(license_key.size()),
         reinterpret_cast<const unsigned char*>(challenge.data()), challenge.size(),
         hmac_out, &hmac_len);

    std::ostringstream oss;
    for (unsigned int i = 0; i < hmac_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hmac_out[i]);
    }
    return oss.str();
}

class USBChallengeResponseTest : public USBAdminAuthenticatorTest {
protected:
    /// License key as stored in the test license JSON (must match createValidLicense()).
    static constexpr const char* kLicenseKey = "THEMIS-ENT-ADMIN-TEST-12345678";

    USBAdminAuthenticator makeAuthWithValidUSB() {
        createValidLicense();
        USBAdminConfig config;
        config.mount_path             = test_mount_path_;
        config.require_usb_for_admin  = true;
        config.challenge_ttl          = std::chrono::seconds(300);
        USBAdminAuthenticator auth(config);
        // Inject verifier: bypass placeholder RSA key and hardware check so
        // tests can exercise the challenge-response logic in isolation.
        // STUB/SIMULATION NOTE:
        // Purpose: allow challenge-response tests to load a license without
        //   a real RSA-signed USB token.
        // Activation: test context only.
        // Production Delta: real verifier checks RSA signature + hardware ID.
        // Removal Plan: remove when real signed USB tokens are available in CI.
        auth.setLicenseVerifierFn([](const USBAdminLicense& lic, const std::string&) {
            return !lic.license_key.empty() && !lic.signature.empty();
        });
        auth.initialize();
        auth.refreshUSBStatus();
        return auth;
    }
};

TEST_F(USBChallengeResponseTest, ValidHMAC_AcceptsCorrectResponse) {
    auto auth = makeAuthWithValidUSB();
    std::string challenge = auth.createChallenge();
    std::string response  = computeHmacResponse(kLicenseKey, challenge);

    EXPECT_TRUE(auth.validateChallengeResponse(challenge, response))
        << "A correct HMAC-SHA256 response must be accepted";
}

TEST_F(USBChallengeResponseTest, WrongResponse_Rejected) {
    auto auth = makeAuthWithValidUSB();
    std::string challenge = auth.createChallenge();
    EXPECT_FALSE(auth.validateChallengeResponse(challenge, "deadbeef"))
        << "An incorrect response must be rejected";
}

TEST_F(USBChallengeResponseTest, OneTimeUse_SecondValidationFails) {
    auto auth = makeAuthWithValidUSB();
    std::string challenge = auth.createChallenge();
    std::string response  = computeHmacResponse(kLicenseKey, challenge);

    // First validation succeeds
    ASSERT_TRUE(auth.validateChallengeResponse(challenge, response));

    // Replay attempt: same challenge + same response must be rejected
    EXPECT_FALSE(auth.validateChallengeResponse(challenge, response))
        << "Replay attack: used challenge must not validate a second time";
}

TEST_F(USBChallengeResponseTest, UnknownChallenge_Rejected) {
    auto auth = makeAuthWithValidUSB();
    // Fabricated challenge that was never issued by this instance
    std::string fake_challenge = std::string(64, 'a');
    std::string response       = computeHmacResponse(kLicenseKey, fake_challenge);

    EXPECT_FALSE(auth.validateChallengeResponse(fake_challenge, response))
        << "A challenge that was not issued by this instance must be rejected";
}

TEST_F(USBChallengeResponseTest, ExpiredChallenge_Rejected) {
    USBAdminConfig config;
    config.mount_path    = test_mount_path_;
    config.challenge_ttl = std::chrono::seconds(0); // TTL = 0 → immediately expired
    createValidLicense();
    USBAdminAuthenticator auth(config);
    auth.initialize();
    auth.refreshUSBStatus();

    std::string challenge = auth.createChallenge();
    std::string response  = computeHmacResponse(kLicenseKey, challenge);

    // With TTL = 0 the challenge should be considered expired immediately
    EXPECT_FALSE(auth.validateChallengeResponse(challenge, response))
        << "A challenge with TTL=0 must be rejected as expired";
}

TEST_F(USBChallengeResponseTest, EmptyResponse_Rejected) {
    auto auth = makeAuthWithValidUSB();
    std::string challenge = auth.createChallenge();
    EXPECT_FALSE(auth.validateChallengeResponse(challenge, ""))
        << "Empty response must be rejected";
}

TEST_F(USBChallengeResponseTest, MultipleDistinctChallenges_EachUsableOnce) {
    auto auth = makeAuthWithValidUSB();

    std::string c1 = auth.createChallenge();
    std::string c2 = auth.createChallenge();
    EXPECT_NE(c1, c2) << "Two challenges must be distinct (CSPRNG)";

    std::string r1 = computeHmacResponse(kLicenseKey, c1);
    std::string r2 = computeHmacResponse(kLicenseKey, c2);

    EXPECT_TRUE(auth.validateChallengeResponse(c1, r1)) << "Challenge 1 must validate";
    EXPECT_TRUE(auth.validateChallengeResponse(c2, r2)) << "Challenge 2 must validate";

    // Replay both
    EXPECT_FALSE(auth.validateChallengeResponse(c1, r1)) << "Replay of c1 must fail";
    EXPECT_FALSE(auth.validateChallengeResponse(c2, r2)) << "Replay of c2 must fail";
}

// ============================================================================
// LicenseVerifierFn injection tests (USB-LV-01 … USB-LV-03)
// ============================================================================

class USBLicenseVerifierTest : public USBAdminAuthenticatorTest {};

// USB-LV-01 — Injected verifier that always accepts → refreshUSBStatus() true
TEST_F(USBLicenseVerifierTest, USB_LV_01_InjectedVerifier_Accepts) {
    createValidLicense();

    USBAdminConfig config;
    config.mount_path            = test_mount_path_;
    config.require_usb_for_admin = true;

    USBAdminAuthenticator auth(config);
    auth.setLicenseVerifierFn([](const USBAdminLicense& lic, const std::string&) {
        // Accept: license has a non-empty key and signature (realistic basic check).
        return !lic.license_key.empty() && !lic.signature.empty();
    });

    EXPECT_TRUE(auth.refreshUSBStatus())
        << "USB-LV-01: injected accept-all verifier must cause refreshUSBStatus() to return true";
    EXPECT_TRUE(auth.isAdminUSBPresent());

    auto lic = auth.getCurrentLicense();
    ASSERT_TRUE(lic.has_value());
    EXPECT_EQ(lic->organization, "Test Organization");
}

// USB-LV-02 — Injected verifier that always rejects → refreshUSBStatus() false
TEST_F(USBLicenseVerifierTest, USB_LV_02_InjectedVerifier_Rejects) {
    createValidLicense();

    USBAdminConfig config;
    config.mount_path            = test_mount_path_;
    config.require_usb_for_admin = true;

    USBAdminAuthenticator auth(config);
    auth.setLicenseVerifierFn([](const USBAdminLicense&, const std::string&) {
        return false; // always reject
    });

    EXPECT_FALSE(auth.refreshUSBStatus())
        << "USB-LV-02: injected reject-all verifier must cause refreshUSBStatus() to return false";
    EXPECT_FALSE(auth.isAdminUSBPresent());
}

// USB-LV-03 — Clearing the verifier fn (nullptr) reverts to built-in RSA path.
//   The built-in path always fails with the placeholder key, so the result is false.
TEST_F(USBLicenseVerifierTest, USB_LV_03_ClearedVerifier_FallsBackToBuiltin) {
    createValidLicense();

    USBAdminConfig config;
    config.mount_path            = test_mount_path_;
    config.require_usb_for_admin = true;

    USBAdminAuthenticator auth(config);

    // Set and then clear verifier → built-in RSA path is restored.
    auth.setLicenseVerifierFn([](const USBAdminLicense&, const std::string&) {
        return true;
    });
    auth.setLicenseVerifierFn(nullptr); // clear

    // Built-in path fails: fake RSA key rejects all real signatures.
    EXPECT_FALSE(auth.refreshUSBStatus())
        << "USB-LV-03: after clearing the verifier fn, the built-in (failing) RSA path must be used";
    EXPECT_FALSE(auth.isAdminUSBPresent());
}

