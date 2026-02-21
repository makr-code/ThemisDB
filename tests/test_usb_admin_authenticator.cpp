/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_usb_admin_authenticator.cpp                   ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     278                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "security/usb_admin_authenticator.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

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
