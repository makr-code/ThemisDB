#include <gtest/gtest.h>
#include "security/hsm_provider.h"
#include <cstdlib>

using namespace themis::security;

/**
 * HSM Stub Gating Tests
 * 
 * Tests that the HSM stub provider properly enforces security gating:
 * - Requires explicit opt-in via THEMIS_ALLOW_HSM_STUB
 * - Fails in production mode without opt-in
 * - Detects production environment indicators
 */

class HSMStubGatingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear all relevant environment variables before each test
        #ifdef _WIN32
            _putenv_s("THEMIS_ALLOW_HSM_STUB", "");
            _putenv_s("THEMIS_PRODUCTION_MODE", "");
            _putenv_s("THEMIS_ENVIRONMENT", "");
            _putenv_s("ENVIRONMENT", "");
            _putenv_s("NODE_ENV", "");
        #else
            unsetenv("THEMIS_ALLOW_HSM_STUB");
            unsetenv("THEMIS_PRODUCTION_MODE");
            unsetenv("THEMIS_ENVIRONMENT");
            unsetenv("ENVIRONMENT");
            unsetenv("NODE_ENV");
        #endif
    }
    
    void TearDown() override {
        // Clean up environment after test
        SetUp();
    }
    
    void setEnv(const char* name, const char* value) {
        #ifdef _WIN32
            _putenv_s(name, value);
        #else
            setenv(name, value, 1);
        #endif
    }
};

TEST_F(HSMStubGatingTest, AllowsStubInDevelopmentWithOptIn) {
    // Set explicit opt-in
    setEnv("THEMIS_ALLOW_HSM_STUB", "1");
    
    HSMConfig config;
    config.library_path = "/nonexistent/stub.so";
    HSMProvider hsm(config);
    
    // Should succeed with opt-in
    EXPECT_TRUE(hsm.initialize());
    EXPECT_TRUE(hsm.isStubProvider());
    EXPECT_TRUE(hsm.isReady());
}

TEST_F(HSMStubGatingTest, FailsInProductionModeWithoutOptIn) {
    // Enable production mode
    setEnv("THEMIS_PRODUCTION_MODE", "1");
    
    HSMConfig config;
    config.library_path = "/nonexistent/stub.so";
    HSMProvider hsm(config);
    
    // Should fail in production mode
    EXPECT_FALSE(hsm.initialize());
    EXPECT_TRUE(hsm.isStubProvider());
    EXPECT_FALSE(hsm.isReady());
    
    // Check error message
    std::string error = hsm.getLastError();
    EXPECT_NE(error.find("production mode"), std::string::npos);
}

TEST_F(HSMStubGatingTest, FailsInProductionModeEvenWithOptIn) {
    // Enable production mode AND opt-in
    setEnv("THEMIS_PRODUCTION_MODE", "1");
    setEnv("THEMIS_ALLOW_HSM_STUB", "1");
    
    HSMConfig config;
    config.library_path = "/nonexistent/stub.so";
    HSMProvider hsm(config);
    
    // Production mode takes precedence - should still fail
    EXPECT_FALSE(hsm.initialize());
    EXPECT_FALSE(hsm.isReady());
}

TEST_F(HSMStubGatingTest, DetectsProductionEnvironment) {
    // Set production environment indicator
    setEnv("ENVIRONMENT", "production");
    
    HSMConfig config;
    config.library_path = "/nonexistent/stub.so";
    HSMProvider hsm(config);
    
    // Should fail without opt-in
    EXPECT_FALSE(hsm.initialize());
    EXPECT_FALSE(hsm.isReady());
    
    std::string error = hsm.getLastError();
    EXPECT_NE(error.find("production environment"), std::string::npos);
}

TEST_F(HSMStubGatingTest, DetectsNodeEnvProduction) {
    // Set NODE_ENV=production (common in Node.js environments)
    setEnv("NODE_ENV", "production");
    
    HSMConfig config;
    config.library_path = "/nonexistent/stub.so";
    HSMProvider hsm(config);
    
    // Should fail without opt-in
    EXPECT_FALSE(hsm.initialize());
    EXPECT_FALSE(hsm.isReady());
}

TEST_F(HSMStubGatingTest, AllowsProductionEnvironmentWithOptIn) {
    // Set production environment but with explicit opt-in
    setEnv("ENVIRONMENT", "production");
    setEnv("THEMIS_ALLOW_HSM_STUB", "1");
    
    HSMConfig config;
    config.library_path = "/nonexistent/stub.so";
    HSMProvider hsm(config);
    
    // Should succeed with explicit opt-in (developer override)
    EXPECT_TRUE(hsm.initialize());
    EXPECT_TRUE(hsm.isReady());
}

TEST_F(HSMStubGatingTest, AllowsStubInDevelopmentEnvironment) {
    // Set development environment (no opt-in needed)
    setEnv("ENVIRONMENT", "development");
    
    HSMConfig config;
    config.library_path = "/nonexistent/stub.so";
    HSMProvider hsm(config);
    
    // Should succeed in development without opt-in
    EXPECT_TRUE(hsm.initialize());
    EXPECT_TRUE(hsm.isReady());
}

TEST_F(HSMStubGatingTest, AllowsStubWithNoEnvironmentSet) {
    // No environment variables set (default local development)
    
    HSMConfig config;
    config.library_path = "/nonexistent/stub.so";
    HSMProvider hsm(config);
    
    // Should succeed (assumes local development)
    EXPECT_TRUE(hsm.initialize());
    EXPECT_TRUE(hsm.isReady());
}

TEST_F(HSMStubGatingTest, StubProviderReturnsCorrectFlag) {
    setEnv("THEMIS_ALLOW_HSM_STUB", "1");
    
    HSMConfig config;
    config.library_path = "/nonexistent/stub.so";
    HSMProvider hsm(config);
    
    EXPECT_TRUE(hsm.initialize());
    
    // Stub provider should always return true for isStubProvider()
    EXPECT_TRUE(hsm.isStubProvider());
}

TEST_F(HSMStubGatingTest, PeriodicSecurityCheckLogsWarning) {
    setEnv("THEMIS_ALLOW_HSM_STUB", "1");
    
    HSMConfig config;
    config.library_path = "/nonexistent/stub.so";
    HSMProvider hsm(config);
    
    EXPECT_TRUE(hsm.initialize());
    
    // Should not throw, just log warnings
    EXPECT_NO_THROW(hsm.periodicSecurityCheck());
}

// Test that the error message includes helpful information
TEST_F(HSMStubGatingTest, ErrorMessageIncludesGuidance) {
    setEnv("THEMIS_PRODUCTION_MODE", "1");
    
    HSMConfig config;
    HSMProvider hsm(config);
    
    EXPECT_FALSE(hsm.initialize());
    
    std::string error = hsm.getLastError();
    // Should mention how to build with real HSM
    EXPECT_NE(error.find("THEMIS_ENABLE_HSM_REAL"), std::string::npos);
}

// THEMIS_ENVIRONMENT=production is a hard failure that cannot be overridden
TEST_F(HSMStubGatingTest, FailsInThemisEnvironmentProduction) {
    setEnv("THEMIS_ENVIRONMENT", "production");
    
    HSMConfig config;
    config.library_path = "/nonexistent/stub.so";
    HSMProvider hsm(config);
    
    EXPECT_FALSE(hsm.initialize());
    EXPECT_FALSE(hsm.isReady());
    
    std::string error = hsm.getLastError();
    EXPECT_NE(error.find("production mode"), std::string::npos);
}

// THEMIS_ENVIRONMENT=production cannot be bypassed with THEMIS_ALLOW_HSM_STUB
TEST_F(HSMStubGatingTest, ThemisEnvironmentProductionCannotBeOverridden) {
    setEnv("THEMIS_ENVIRONMENT", "production");
    setEnv("THEMIS_ALLOW_HSM_STUB", "1");
    
    HSMConfig config;
    config.library_path = "/nonexistent/stub.so";
    HSMProvider hsm(config);
    
    // Themis production environment takes precedence even with opt-in
    EXPECT_FALSE(hsm.initialize());
    EXPECT_FALSE(hsm.isReady());
}

// THEMIS_PRODUCTION_MODE accepts truthy string values beyond "1"
TEST_F(HSMStubGatingTest, FailsWithProductionModeTrueString) {
    setEnv("THEMIS_PRODUCTION_MODE", "true");
    
    HSMConfig config;
    config.library_path = "/nonexistent/stub.so";
    HSMProvider hsm(config);
    
    EXPECT_FALSE(hsm.initialize());
    EXPECT_FALSE(hsm.isReady());
}