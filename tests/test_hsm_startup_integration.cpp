/**
 * Integration test for HSM security warning at startup
 * 
 * Tests that the server properly displays warnings when stub HSM is active.
 * This test validates the FIND-002 security implementation.
 */

#include <gtest/gtest.h>

#ifdef _WIN32
#define setenv(name, value, overwrite) _putenv_s(name, value)
#define unsetenv(name) _putenv_s(name, "")
#endif
#include "security/hsm_provider.h"
#include "security/hsm_security_checker.h"
#include "security/hsm_security_metrics.h"
#include <cstdlib>
#include <thread>
#include <chrono>
#include <atomic>
#include <optional>

using namespace themis::security;

/**
 * HSM Startup Integration Tests
 * 
 * These tests verify that:
 * 1. Startup warning is displayed when stub HSM is active
 * 2. Warning can be suppressed with --allow-stub-hsm flag
 * 3. Production mode blocks stub HSM without flag
 * 4. Metrics are properly exposed
 */
class HSMStartupIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Save original environment (store copies to avoid pointer invalidation)
        const char* prod_mode = std::getenv("THEMIS_PRODUCTION_MODE");
        if (prod_mode) {
            saved_prod_mode_ = std::string(prod_mode);
        }
        
        const char* environment = std::getenv("THEMIS_ENVIRONMENT");
        if (environment) {
            saved_environment_ = std::string(environment);
        }
    }
    
    void TearDown() override {
        // Restore original environment
        if (saved_prod_mode_.has_value()) {
            setenv("THEMIS_PRODUCTION_MODE", saved_prod_mode_->c_str(), 1);
        } else {
            unsetenv("THEMIS_PRODUCTION_MODE");
        }
        
        if (saved_environment_.has_value()) {
            setenv("THEMIS_ENVIRONMENT", saved_environment_->c_str(), 1);
        } else {
            unsetenv("THEMIS_ENVIRONMENT");
        }
    }
    
    std::optional<std::string> saved_prod_mode_;
    std::optional<std::string> saved_environment_;
};

/**
 * Test that stub HSM initialization succeeds in development mode
 */
TEST_F(HSMStartupIntegrationTest, StubHSM_DevelopmentMode_Succeeds) {
    unsetenv("THEMIS_PRODUCTION_MODE");
    unsetenv("THEMIS_ENVIRONMENT");
    
    HSMConfig config;
    config.library_path = "";  // Empty = stub provider
    
    HSMProvider hsm(config);
    EXPECT_TRUE(hsm.initialize());
    EXPECT_TRUE(hsm.isStubProvider());
    EXPECT_TRUE(hsm.isReady());
    
    const char* argv[] = {"themis_server"};
    EXPECT_TRUE(HSMSecurityChecker::validateProductionSafety(hsm, 1, const_cast<char**>(argv)));
}

/**
 * Test that stub HSM is blocked in production mode without flag
 */
TEST_F(HSMStartupIntegrationTest, StubHSM_ProductionMode_Blocked) {
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    HSMConfig config;
    config.library_path = "";  // Stub provider
    
    HSMProvider hsm(config);
    hsm.initialize();
    
    const char* argv[] = {"themis_server"};
    // Should fail in production without --allow-stub-hsm flag
    EXPECT_FALSE(HSMSecurityChecker::validateProductionSafety(hsm, 1, const_cast<char**>(argv)));
}

/**
 * Test that --allow-stub-hsm flag allows stub in production (with warning)
 */
TEST_F(HSMStartupIntegrationTest, StubHSM_ProductionMode_AllowedWithFlag) {
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    HSMConfig config;
    config.library_path = "";  // Stub provider
    
    HSMProvider hsm(config);
    hsm.initialize();
    
    const char* argv[] = {"themis_server", "--allow-stub-hsm"};
    // Should succeed with flag (but logs warning)
    EXPECT_TRUE(HSMSecurityChecker::validateProductionSafety(hsm, 2, const_cast<char**>(argv)));
}

/**
 * Test periodic warning message generation
 */
TEST_F(HSMStartupIntegrationTest, PeriodicWarning_StubActive_ReturnsMessage) {
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    HSMConfig config;
    config.library_path = "";  // Stub provider
    
    HSMProvider hsm(config);
    hsm.initialize();
    
    std::string warning = HSMSecurityChecker::getPeriodicWarning(hsm);
    EXPECT_FALSE(warning.empty());
    EXPECT_NE(warning.find("SECURITY ALERT"), std::string::npos);
    EXPECT_NE(warning.find("HSM stub provider"), std::string::npos);
}

/**
 * Test periodic warning is empty with real HSM.
 *
 * This test requires a real HSM library to be available. The path to the
 * library must be provided via the THEMIS_REAL_HSM_LIBRARY environment
 * variable. If the variable is not set, the test is skipped.
 */
TEST_F(HSMStartupIntegrationTest, PeriodicWarning_RealHSM_NoMessage) {
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    const char* real_hsm_lib = std::getenv("THEMIS_REAL_HSM_LIBRARY");
    if (!real_hsm_lib || std::string(real_hsm_lib).empty()) {
        GTEST_SKIP() << "Real HSM library not configured; set THEMIS_REAL_HSM_LIBRARY to run this test.";
    }
    
    HSMConfig config;
    config.library_path = real_hsm_lib;
    
    HSMProvider hsm(config);
    hsm.initialize();
    
    // With a real HSM in production mode, there should be no periodic warning
    std::string warning = HSMSecurityChecker::getPeriodicWarning(hsm);
    EXPECT_TRUE(warning.empty());
}

/**
 * Test HSM security metrics generation
 */
TEST_F(HSMStartupIntegrationTest, SecurityMetrics_StubActive_ProperValues) {
    HSMConfig config;
    config.library_path = "";  // Stub provider
    
    HSMProvider hsm(config);
    hsm.initialize();
    
    std::string metrics = HSMSecurityMetrics::exportMetrics(hsm);
    
    // Check for required metrics
    EXPECT_NE(metrics.find("themis_hsm_insecure_config"), std::string::npos);
    EXPECT_NE(metrics.find("themis_hsm_provider_type"), std::string::npos);
    EXPECT_NE(metrics.find("hsm_security_stub_active"), std::string::npos);
    
    // Verify stub is marked as insecure
    EXPECT_NE(metrics.find("themis_hsm_insecure_config 1"), std::string::npos);
    EXPECT_NE(metrics.find("provider=\"stub\""), std::string::npos);
}

/**
 * Test JSON metrics export
 */
TEST_F(HSMStartupIntegrationTest, SecurityMetrics_JSON_ProperFormat) {
    HSMConfig config;
    config.library_path = "";  // Stub provider
    
    HSMProvider hsm(config);
    hsm.initialize();
    
    std::string json = HSMSecurityMetrics::exportJSON(hsm);
    
    // Check JSON structure
    EXPECT_NE(json.find("\"hsm_security\""), std::string::npos);
    EXPECT_NE(json.find("\"stub_active\": true"), std::string::npos);
    EXPECT_NE(json.find("\"provider_type\": \"stub\""), std::string::npos);
    EXPECT_NE(json.find("\"compliance\""), std::string::npos);
}

/**
 * Test that periodic security check can be called safely
 */
TEST_F(HSMStartupIntegrationTest, PeriodicSecurityCheck_NoThrow) {
    HSMConfig config;
    config.library_path = "";  // Stub provider
    
    HSMProvider hsm(config);
    hsm.initialize();
    
    // Should not throw
    EXPECT_NO_THROW(hsm.periodicSecurityCheck());
}

/**
 * Test compliance status in metrics
 */
TEST_F(HSMStartupIntegrationTest, ComplianceStatus_StubActive_NonCompliant) {
    HSMConfig config;
    config.library_path = "";  // Stub provider
    
    HSMProvider hsm(config);
    hsm.initialize();
    
    std::string metrics = HSMSecurityMetrics::exportMetrics(hsm);
    
    // All compliance standards should be 0 (non-compliant) with stub
    EXPECT_NE(metrics.find("hsm_compliance_status{standard=\"nist_sp_800_53_sc_12\"} 0"), std::string::npos);
    EXPECT_NE(metrics.find("hsm_compliance_status{standard=\"iso_27001_a_8_24\"} 0"), std::string::npos);
    EXPECT_NE(metrics.find("hsm_compliance_status{standard=\"pci_dss_3_6\"} 0"), std::string::npos);
    EXPECT_NE(metrics.find("hsm_compliance_status{standard=\"gdpr_art_32\"} 0"), std::string::npos);
}

/**
 * Main function for standalone test execution
 */
