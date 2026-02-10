#include <gtest/gtest.h>
#include "security/hsm_provider.h"
#include "security/hsm_security_checker.h"
#include <cstdlib>

#ifdef _WIN32
#define setenv(name, value, overwrite) _putenv_s(name, value)
#define unsetenv(name) _putenv_s(name, "")
#endif

using namespace themis::security;

/**
 * HSM Security Checker Tests
 * 
 * Tests for production mode detection and safety enforcement.
 * Addresses FIND-002 requirements.
 */

class HSMSecurityCheckerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Save original environment
        saved_prod_mode_ = std::getenv("THEMIS_PRODUCTION_MODE");
        saved_environment_ = std::getenv("THEMIS_ENVIRONMENT");
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
    
    const char* saved_prod_mode_;
    const char* saved_environment_;
};

TEST_F(HSMSecurityCheckerTest, ProductionModeDetection_NotSet) {
    unsetenv("THEMIS_PRODUCTION_MODE");
    unsetenv("THEMIS_ENVIRONMENT");
    
    EXPECT_FALSE(HSMSecurityChecker::isProductionMode());
}

TEST_F(HSMSecurityCheckerTest, ProductionModeDetection_True) {
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    EXPECT_TRUE(HSMSecurityChecker::isProductionMode());
}

TEST_F(HSMSecurityCheckerTest, ProductionModeDetection_One) {
    setenv("THEMIS_PRODUCTION_MODE", "1", 1);
    EXPECT_TRUE(HSMSecurityChecker::isProductionMode());
}

TEST_F(HSMSecurityCheckerTest, ProductionModeDetection_Production) {
    setenv("THEMIS_PRODUCTION_MODE", "production", 1);
    EXPECT_TRUE(HSMSecurityChecker::isProductionMode());
}

TEST_F(HSMSecurityCheckerTest, ProductionModeDetection_EnvironmentVar) {
    unsetenv("THEMIS_PRODUCTION_MODE");
    setenv("THEMIS_ENVIRONMENT", "production", 1);
    EXPECT_TRUE(HSMSecurityChecker::isProductionMode());
}

TEST_F(HSMSecurityCheckerTest, ProductionModeDetection_EnvironmentProd) {
    unsetenv("THEMIS_PRODUCTION_MODE");
    setenv("THEMIS_ENVIRONMENT", "prod", 1);
    EXPECT_TRUE(HSMSecurityChecker::isProductionMode());
}

TEST_F(HSMSecurityCheckerTest, ProductionModeDetection_Development) {
    setenv("THEMIS_PRODUCTION_MODE", "false", 1);
    EXPECT_FALSE(HSMSecurityChecker::isProductionMode());
}

TEST_F(HSMSecurityCheckerTest, AllowStubFlag_NotPresent) {
    const char* argv[] = {"themis_server", "--config", "config.yaml"};
    int argc = 3;
    
    EXPECT_FALSE(HSMSecurityChecker::hasAllowStubFlag(argc, const_cast<char**>(argv)));
}

TEST_F(HSMSecurityCheckerTest, AllowStubFlag_Present) {
    const char* argv[] = {"themis_server", "--allow-stub-hsm", "--config", "config.yaml"};
    int argc = 4;
    
    EXPECT_TRUE(HSMSecurityChecker::hasAllowStubFlag(argc, const_cast<char**>(argv)));
}

TEST_F(HSMSecurityCheckerTest, AllowStubFlag_LastArg) {
    const char* argv[] = {"themis_server", "--config", "config.yaml", "--allow-stub-hsm"};
    int argc = 4;
    
    EXPECT_TRUE(HSMSecurityChecker::hasAllowStubFlag(argc, const_cast<char**>(argv)));
}

TEST_F(HSMSecurityCheckerTest, ValidateProductionSafety_DevelopmentMode) {
    unsetenv("THEMIS_PRODUCTION_MODE");
    
    HSMConfig config;
    config.library_path = "";  // Force stub
    HSMProvider hsm(config);
    hsm.initialize();
    
    const char* argv[] = {"themis_server"};
    int argc = 1;
    
    // Should always pass in development mode
    EXPECT_TRUE(HSMSecurityChecker::validateProductionSafety(hsm, argc, const_cast<char**>(argv)));
}

TEST_F(HSMSecurityCheckerTest, ValidateProductionSafety_ProductionWithStubBlocked) {
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    HSMConfig config;
    config.library_path = "";  // Force stub
    HSMProvider hsm(config);
    hsm.initialize();
    
    const char* argv[] = {"themis_server"};
    int argc = 1;
    
    // Should fail in production with stub provider
    EXPECT_FALSE(HSMSecurityChecker::validateProductionSafety(hsm, argc, const_cast<char**>(argv)));
}

TEST_F(HSMSecurityCheckerTest, ValidateProductionSafety_ProductionWithStubOverride) {
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    HSMConfig config;
    config.library_path = "";  // Force stub
    HSMProvider hsm(config);
    hsm.initialize();
    
    const char* argv[] = {"themis_server", "--allow-stub-hsm"};
    int argc = 2;
    
    // Should pass with override flag (but log warnings)
    EXPECT_TRUE(HSMSecurityChecker::validateProductionSafety(hsm, argc, const_cast<char**>(argv)));
}

TEST_F(HSMSecurityCheckerTest, PeriodicWarning_DevelopmentMode) {
    unsetenv("THEMIS_PRODUCTION_MODE");
    
    HSMConfig config;
    config.library_path = "";  // Force stub
    HSMProvider hsm(config);
    hsm.initialize();
    
    // No warning in development mode
    std::string warning = HSMSecurityChecker::getPeriodicWarning(hsm);
    EXPECT_TRUE(warning.empty());
}

TEST_F(HSMSecurityCheckerTest, PeriodicWarning_ProductionWithStub) {
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    HSMConfig config;
    config.library_path = "";  // Force stub
    HSMProvider hsm(config);
    hsm.initialize();
    
    // Should return warning message in production with stub
    std::string warning = HSMSecurityChecker::getPeriodicWarning(hsm);
    EXPECT_FALSE(warning.empty());
    EXPECT_NE(warning.find("PRODUCTION SECURITY ALERT"), std::string::npos);
    EXPECT_NE(warning.find("HSM stub provider"), std::string::npos);
}

TEST_F(HSMSecurityCheckerTest, IntegrationExample) {
    // This test demonstrates the complete flow
    
    // 1. Set production mode
    setenv("THEMIS_PRODUCTION_MODE", "true", 1);
    
    // 2. Create stub HSM
    HSMConfig config;
    config.library_path = "";  // Force stub
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());
    ASSERT_TRUE(hsm.isStubProvider());
    
    // 3. Without override flag - should fail
    {
        const char* argv[] = {"themis_server"};
        int argc = 1;
        EXPECT_FALSE(HSMSecurityChecker::validateProductionSafety(hsm, argc, const_cast<char**>(argv)));
    }
    
    // 4. With override flag - should pass but warn
    {
        const char* argv[] = {"themis_server", "--allow-stub-hsm"};
        int argc = 2;
        EXPECT_TRUE(HSMSecurityChecker::validateProductionSafety(hsm, argc, const_cast<char**>(argv)));
    }
    
    // 5. Periodic warning should be active
    std::string warning = HSMSecurityChecker::getPeriodicWarning(hsm);
    EXPECT_FALSE(warning.empty());
}
