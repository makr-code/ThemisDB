#include <gtest/gtest.h>
#include "security/transport_security_checker.h"
#include <cstdlib>
#include <string>
#include <optional>

// Windows-compatible environment variable macros
#ifdef _WIN32
    #define SETENV(name, value) _putenv_s(name, value)
    #define UNSETENV(name) _putenv(name "=")
#else
    #define SETENV(name, value) setenv(name, value, 1)
    #define UNSETENV(name) unsetenv(name)
#endif

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
        // Save original environment variables as copies
        const char* prod_mode = std::getenv("THEMIS_PRODUCTION_MODE");
        if (prod_mode) {
            saved_prod_mode_ = std::string(prod_mode);
        }
        
        const char* environment = std::getenv("THEMIS_ENVIRONMENT");
        if (environment) {
            saved_environment_ = std::string(environment);
        }
        
        // Clear environment for clean test state
        #ifdef _WIN32
        _putenv("THEMIS_PRODUCTION_MODE=");
        _putenv("THEMIS_ENVIRONMENT=");
        #else
        unsetenv("THEMIS_PRODUCTION_MODE");
        unsetenv("THEMIS_ENVIRONMENT");
        #endif
    }
    
    void TearDown() override {
        // Restore original environment from copies
        #ifdef _WIN32
        if (saved_prod_mode_.has_value()) {
            _putenv_s("THEMIS_PRODUCTION_MODE", saved_prod_mode_.value().c_str());
        } else {
            _putenv("THEMIS_PRODUCTION_MODE=");
        }
        
        if (saved_environment_.has_value()) {
            _putenv_s("THEMIS_ENVIRONMENT", saved_environment_.value().c_str());
        } else {
            _putenv("THEMIS_ENVIRONMENT=");
        }
        #else
        if (saved_prod_mode_.has_value()) {
            setenv("THEMIS_PRODUCTION_MODE", saved_prod_mode_.value().c_str(), 1);
        } else {
            unsetenv("THEMIS_PRODUCTION_MODE");
        }
        
        if (saved_environment_.has_value()) {
            setenv("THEMIS_ENVIRONMENT", saved_environment_.value().c_str(), 1);
        } else {
            unsetenv("THEMIS_ENVIRONMENT");
        }
        #endif
    }
    
    std::optional<std::string> saved_prod_mode_;
    std::optional<std::string> saved_environment_;
};

// =============================================================================
// Production Mode Detection Tests
// =============================================================================

TEST_F(TransportSecurityCheckerTest, DetectProductionModeFromTHEMIS_PRODUCTION_MODE_true) {
    SETENV("THEMIS_PRODUCTION_MODE", "true");
    EXPECT_TRUE(TransportSecurityChecker::isProductionMode());
}

TEST_F(TransportSecurityCheckerTest, DetectProductionModeFromTHEMIS_PRODUCTION_MODE_1) {
    SETENV("THEMIS_PRODUCTION_MODE", "1");
    EXPECT_TRUE(TransportSecurityChecker::isProductionMode());
}

TEST_F(TransportSecurityCheckerTest, DetectProductionModeFromTHEMIS_PRODUCTION_MODE_production) {
    SETENV("THEMIS_PRODUCTION_MODE", "production");
    EXPECT_TRUE(TransportSecurityChecker::isProductionMode());
}

TEST_F(TransportSecurityCheckerTest, DetectDevelopmentModeFromTHEMIS_PRODUCTION_MODE_false) {
    SETENV("THEMIS_PRODUCTION_MODE", "false");
    EXPECT_FALSE(TransportSecurityChecker::isProductionMode());
}

TEST_F(TransportSecurityCheckerTest, DetectProductionModeFromTHEMIS_ENVIRONMENT_production) {
    UNSETENV("THEMIS_PRODUCTION_MODE");
    SETENV("THEMIS_ENVIRONMENT", "production");
    EXPECT_TRUE(TransportSecurityChecker::isProductionMode());
}

TEST_F(TransportSecurityCheckerTest, DetectProductionModeFromTHEMIS_ENVIRONMENT_prod) {
    UNSETENV("THEMIS_PRODUCTION_MODE");
    SETENV("THEMIS_ENVIRONMENT", "prod");
    EXPECT_TRUE(TransportSecurityChecker::isProductionMode());
}

TEST_F(TransportSecurityCheckerTest, DetectDevelopmentModeByDefault) {
    UNSETENV("THEMIS_PRODUCTION_MODE");
    UNSETENV("THEMIS_ENVIRONMENT");
    EXPECT_FALSE(TransportSecurityChecker::isProductionMode());
}

// =============================================================================
// Override Flag Detection Tests
// =============================================================================

TEST_F(TransportSecurityCheckerTest, DetectOverrideFlag) {
    const char* argv[] = {"themis_server", "--allow-insecure-wire-protocol"};
    EXPECT_TRUE(TransportSecurityChecker::hasAllowInsecureFlag(2, argv));
}

TEST_F(TransportSecurityCheckerTest, NoOverrideFlagWithoutFlag) {
    const char* argv[] = {"themis_server", "--other-flag"};
    EXPECT_FALSE(TransportSecurityChecker::hasAllowInsecureFlag(2, argv));
}

TEST_F(TransportSecurityCheckerTest, NoOverrideFlagWithNoArgs) {
    const char* argv[] = {"themis_server"};
    EXPECT_FALSE(TransportSecurityChecker::hasAllowInsecureFlag(1, argv));
}

// =============================================================================
// Transport Security Validation Tests
// =============================================================================

TEST_F(TransportSecurityCheckerTest, AllowInsecureTransportInDevelopmentMode) {
    // Development mode (no env vars set)
    UNSETENV("THEMIS_PRODUCTION_MODE");
    UNSETENV("THEMIS_ENVIRONMENT");
    
    const char* argv[] = {"themis_server"};
    bool enable_tls = false;
    
    // Should allow insecure transport in development
    EXPECT_TRUE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 1, argv));
}

TEST_F(TransportSecurityCheckerTest, AllowSecureTransportInDevelopmentMode) {
    // Development mode
    UNSETENV("THEMIS_PRODUCTION_MODE");
    UNSETENV("THEMIS_ENVIRONMENT");
    
    const char* argv[] = {"themis_server"};
    bool enable_tls = true;
    
    // Should allow secure transport in development
    EXPECT_TRUE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 1, argv));
}

TEST_F(TransportSecurityCheckerTest, BlockInsecureTransportInProductionMode) {
    // Production mode
    SETENV("THEMIS_PRODUCTION_MODE", "true");
    
    const char* argv[] = {"themis_server"};
    bool enable_tls = false;
    
    // Should block insecure transport in production
    EXPECT_FALSE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 1, argv));
}

TEST_F(TransportSecurityCheckerTest, AllowSecureTransportInProductionMode) {
    // Production mode
    SETENV("THEMIS_PRODUCTION_MODE", "true");
    
    const char* argv[] = {"themis_server"};
    bool enable_tls = true;
    
    // Should allow secure transport in production
    EXPECT_TRUE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 1, argv));
}

TEST_F(TransportSecurityCheckerTest, AllowInsecureTransportWithOverrideFlag) {
    // Production mode with override flag
    SETENV("THEMIS_PRODUCTION_MODE", "true");
    
    const char* argv[] = {"themis_server", "--allow-insecure-wire-protocol"};
    bool enable_tls = false;
    
    // Should allow with override flag (but log critical warning)
    EXPECT_TRUE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 2, argv));
}

// =============================================================================
// Periodic Warning Tests
// =============================================================================

TEST_F(TransportSecurityCheckerTest, NoWarningInDevelopmentMode) {
    UNSETENV("THEMIS_PRODUCTION_MODE");
    UNSETENV("THEMIS_ENVIRONMENT");
    
    std::string warning = TransportSecurityChecker::getPeriodicWarning(false, "Wire Protocol");
    EXPECT_TRUE(warning.empty());
}

TEST_F(TransportSecurityCheckerTest, NoWarningWhenTLSEnabled) {
    SETENV("THEMIS_PRODUCTION_MODE", "true");
    
    std::string warning = TransportSecurityChecker::getPeriodicWarning(true, "Wire Protocol");
    EXPECT_TRUE(warning.empty());
}

TEST_F(TransportSecurityCheckerTest, WarningWhenTLSDisabledInProduction) {
    SETENV("THEMIS_PRODUCTION_MODE", "true");
    
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
    SETENV("THEMIS_PRODUCTION_MODE", "true");
    
    // Simulate server startup with TLS enabled
    const char* argv[] = {"themis_server", "--config", "config.yaml"};
    bool enable_tls = true;
    
    // Validation should pass
    EXPECT_TRUE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 3, argv));
    
    // No periodic warnings
    std::string warning = TransportSecurityChecker::getPeriodicWarning(enable_tls, "Wire Protocol");
    EXPECT_TRUE(warning.empty());
}

TEST_F(TransportSecurityCheckerTest, FullValidationFlow_ProductionWithoutTLS) {
    // Set production mode
    SETENV("THEMIS_PRODUCTION_MODE", "true");
    
    // Simulate server startup with TLS disabled
    const char* argv[] = {"themis_server", "--config", "config.yaml"};
    bool enable_tls = false;
    
    // Validation should fail (block startup)
    EXPECT_FALSE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 3, argv));
}

TEST_F(TransportSecurityCheckerTest, FullValidationFlow_DevelopmentWithoutTLS) {
    // Development mode (default)
    UNSETENV("THEMIS_PRODUCTION_MODE");
    UNSETENV("THEMIS_ENVIRONMENT");
    
    // Simulate server startup with TLS disabled
    const char* argv[] = {"themis_server", "--config", "config.yaml"};
    bool enable_tls = false;
    
    // Validation should pass in development
    EXPECT_TRUE(TransportSecurityChecker::validateProductionSafety(
        enable_tls, "Wire Protocol", 3, argv));
    
    // No periodic warnings in development
    std::string warning = TransportSecurityChecker::getPeriodicWarning(enable_tls, "Wire Protocol");
    EXPECT_TRUE(warning.empty());
}
