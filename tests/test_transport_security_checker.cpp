#include <gtest/gtest.h>
#include "security/transport_security_checker.h"
#include <cstdlib>
#include <string>

using namespace themis::security;

/**
 * Test suite for TransportSecurityChecker
 * 
 * Tests production mode detection and enforcement of TLS requirements
 * for Wire Protocol transport security.
 */
class TransportSecurityCheckerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Save original environment variables
        saved_prod_mode_ = std::getenv("THEMIS_PRODUCTION_MODE");
        saved_environment_ = std::getenv("THEMIS_ENVIRONMENT");
        
        // Clear environment for clean test state
        unsetenv("THEMIS_PRODUCTION_MODE");
        unsetenv("THEMIS_ENVIRONMENT");
    }
    
    void TearDown() override {
        // Restore original environment
        if (saved_prod_mode_) {
            setenv("THEMIS_PRODUCTION_MODE", saved_prod_mode_, 1);
        } else {
            unsetenv("THEMIS_PRODUCTION_MODE");
        }
        
        if (saved_environment_) {
            setenv("THEMIS_ENVIRONMENT", saved_environment_, 1);
        } else {
            unsetenv("THEMIS_ENVIRONMENT");
        }
    }
    
    const char* saved_prod_mode_ = nullptr;
    const char* saved_environment_ = nullptr;
};

// =============================================================================
// Production Mode Detection Tests
// =============================================================================

TEST_F(TransportSecurityCheckerTest, DetectProductionModeFromTHEMIS_PRODUCTION_MODE_true) {
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    EXPECT_TRUE(TransportSecurityChecker::isProductionMode());
}

TEST_F(TransportSecurityCheckerTest, DetectProductionModeFromTHEMIS_PRODUCTION_MODE_1) {
    setenv("THEMIS_PRODUCTION_MODE", "1", 1);
    EXPECT_TRUE(TransportSecurityChecker::isProductionMode());
}

TEST_F(TransportSecurityCheckerTest, DetectProductionModeFromTHEMIS_PRODUCTION_MODE_production) {
    setenv("THEMIS_PRODUCTION_MODE", "production", 1);
    EXPECT_TRUE(TransportSecurityChecker::isProductionMode());
}

TEST_F(TransportSecurityCheckerTest, DetectDevelopmentModeFromTHEMIS_PRODUCTION_MODE_false) {
    setenv("THEMIS_PRODUCTION_MODE", "false", 1);
    EXPECT_FALSE(TransportSecurityChecker::isProductionMode());
}

TEST_F(TransportSecurityCheckerTest, DetectProductionModeFromTHEMIS_ENVIRONMENT_production) {
    unsetenv("THEMIS_PRODUCTION_MODE");
    setenv("THEMIS_ENVIRONMENT", "production", 1);
    EXPECT_TRUE(TransportSecurityChecker::isProductionMode());
}

TEST_F(TransportSecurityCheckerTest, DetectProductionModeFromTHEMIS_ENVIRONMENT_prod) {
    unsetenv("THEMIS_PRODUCTION_MODE");
    setenv("THEMIS_ENVIRONMENT", "prod", 1);
    EXPECT_TRUE(TransportSecurityChecker::isProductionMode());
}

TEST_F(TransportSecurityCheckerTest, DetectDevelopmentModeByDefault) {
    unsetenv("THEMIS_PRODUCTION_MODE");
    unsetenv("THEMIS_ENVIRONMENT");
    EXPECT_FALSE(TransportSecurityChecker::isProductionMode());
}

// =============================================================================
// Override Flag Detection Tests
// =============================================================================

TEST_F(TransportSecurityCheckerTest, DetectOverrideFlag) {
    const char* argv[] = {"themis_server", "--allow-insecure-wire-protocol"};
    EXPECT_TRUE(TransportSecurityChecker::hasAllowInsecureFlag(2, const_cast<char**>(argv)));
}

TEST_F(TransportSecurityCheckerTest, NoOverrideFlagWithoutFlag) {
    const char* argv[] = {"themis_server", "--other-flag"};
    EXPECT_FALSE(TransportSecurityChecker::hasAllowInsecureFlag(2, const_cast<char**>(argv)));
}

TEST_F(TransportSecurityCheckerTest, NoOverrideFlagWithNoArgs) {
    const char* argv[] = {"themis_server"};
    EXPECT_FALSE(TransportSecurityChecker::hasAllowInsecureFlag(1, const_cast<char**>(argv)));
}

// =============================================================================
// Transport Security Validation Tests
// =============================================================================

TEST_F(TransportSecurityCheckerTest, AllowInsecureTransportInDevelopmentMode) {
    // Development mode (no env vars set)
    unsetenv("THEMIS_PRODUCTION_MODE");
    unsetenv("THEMIS_ENVIRONMENT");
    
    const char* argv[] = {"themis_server"};
    bool enable_tls = false;
    
    // Should allow insecure transport in development
    EXPECT_TRUE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 1, const_cast<char**>(argv)));
}

TEST_F(TransportSecurityCheckerTest, AllowSecureTransportInDevelopmentMode) {
    // Development mode
    unsetenv("THEMIS_PRODUCTION_MODE");
    unsetenv("THEMIS_ENVIRONMENT");
    
    const char* argv[] = {"themis_server"};
    bool enable_tls = true;
    
    // Should allow secure transport in development
    EXPECT_TRUE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 1, const_cast<char**>(argv)));
}

TEST_F(TransportSecurityCheckerTest, BlockInsecureTransportInProductionMode) {
    // Production mode
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    const char* argv[] = {"themis_server"};
    bool enable_tls = false;
    
    // Should block insecure transport in production
    EXPECT_FALSE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 1, const_cast<char**>(argv)));
}

TEST_F(TransportSecurityCheckerTest, AllowSecureTransportInProductionMode) {
    // Production mode
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    const char* argv[] = {"themis_server"};
    bool enable_tls = true;
    
    // Should allow secure transport in production
    EXPECT_TRUE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 1, const_cast<char**>(argv)));
}

TEST_F(TransportSecurityCheckerTest, AllowInsecureTransportWithOverrideFlag) {
    // Production mode with override flag
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    const char* argv[] = {"themis_server", "--allow-insecure-wire-protocol"};
    bool enable_tls = false;
    
    // Should allow with override flag (but log critical warning)
    EXPECT_TRUE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 2, const_cast<char**>(argv)));
}

// =============================================================================
// Periodic Warning Tests
// =============================================================================

TEST_F(TransportSecurityCheckerTest, NoWarningInDevelopmentMode) {
    unsetenv("THEMIS_PRODUCTION_MODE");
    unsetenv("THEMIS_ENVIRONMENT");
    
    std::string warning = TransportSecurityChecker::getPeriodicWarning(false, "Wire Protocol");
    EXPECT_TRUE(warning.empty());
}

TEST_F(TransportSecurityCheckerTest, NoWarningWhenTLSEnabled) {
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    std::string warning = TransportSecurityChecker::getPeriodicWarning(true, "Wire Protocol");
    EXPECT_TRUE(warning.empty());
}

TEST_F(TransportSecurityCheckerTest, WarningWhenTLSDisabledInProduction) {
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    std::string warning = TransportSecurityChecker::getPeriodicWarning(false, "Wire Protocol");
    EXPECT_FALSE(warning.empty());
    EXPECT_NE(warning.find("PRODUCTION SECURITY ALERT"), std::string::npos);
    EXPECT_NE(warning.find("Wire Protocol"), std::string::npos);
}

// =============================================================================
// Integration Test: Full Security Validation Flow
// =============================================================================

TEST_F(TransportSecurityCheckerTest, FullValidationFlow_ProductionWithTLS) {
    // Set production mode
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    // Simulate server startup with TLS enabled
    const char* argv[] = {"themis_server", "--config", "config.yaml"};
    bool enable_tls = true;
    
    // Validation should pass
    EXPECT_TRUE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 3, const_cast<char**>(argv)));
    
    // No periodic warnings
    std::string warning = TransportSecurityChecker::getPeriodicWarning(enable_tls, "Wire Protocol");
    EXPECT_TRUE(warning.empty());
}

TEST_F(TransportSecurityCheckerTest, FullValidationFlow_ProductionWithoutTLS) {
    // Set production mode
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    // Simulate server startup with TLS disabled
    const char* argv[] = {"themis_server", "--config", "config.yaml"};
    bool enable_tls = false;
    
    // Validation should fail (block startup)
    EXPECT_FALSE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 3, const_cast<char**>(argv)));
}

TEST_F(TransportSecurityCheckerTest, FullValidationFlow_DevelopmentWithoutTLS) {
    // Development mode (default)
    unsetenv("THEMIS_PRODUCTION_MODE");
    unsetenv("THEMIS_ENVIRONMENT");
    
    // Simulate server startup with TLS disabled
    const char* argv[] = {"themis_server", "--config", "config.yaml"};
    bool enable_tls = false;
    
    // Validation should pass in development
    EXPECT_TRUE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 3, const_cast<char**>(argv)));
    
    // No periodic warnings in development
    std::string warning = TransportSecurityChecker::getPeriodicWarning(enable_tls, "Wire Protocol");
    EXPECT_TRUE(warning.empty());
}
