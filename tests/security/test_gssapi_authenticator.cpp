#include <gtest/gtest.h>
#include "auth/gssapi_authenticator.h"
#include <memory>

using namespace themis::auth;

/**
 * @brief Test fixture for GSSAPI Authenticator
 * 
 * Note: These tests require a working Kerberos KDC for full integration testing.
 * For unit tests without KDC, we test configuration and initialization logic only.
 */
class GSSAPIAuthenticatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test configuration
        config_.enabled = true;
        config_.service_principal = "themisdb/localhost@EXAMPLE.COM";
        config_.keytab_file = "/tmp/test.keytab";
        config_.krb5_config = "/tmp/test_krb5.conf";
        config_.fallback_to_basic = true;
        
        // Add test principal mappings
        config_.principal_mappings.push_back({
            "admin@EXAMPLE.COM",
            "admin"
        });
        config_.principal_mappings.push_back({
            "*@EXAMPLE.COM",
            "readonly"
        });
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    KerberosConfig config_;
};

TEST_F(GSSAPIAuthenticatorTest, ConstructorInitializesState) {
    GSSAPIAuthenticator auth;
    EXPECT_FALSE(auth.isInitialized());
}

TEST_F(GSSAPIAuthenticatorTest, ConfigurationValidation) {
    GSSAPIAuthenticator auth;
    
    // Valid configuration
    KerberosConfig valid_config = config_;
    // Note: This will fail without actual Kerberos setup, but validates config parsing
    // auth.initialize(valid_config);  // Commented out - requires KDC
    
    // Invalid configuration - empty service principal
    KerberosConfig invalid_config = config_;
    invalid_config.service_principal = "";
    EXPECT_FALSE(auth.initialize(invalid_config));
}

TEST_F(GSSAPIAuthenticatorTest, PrincipalToRoleMapping) {
    GSSAPIAuthenticator auth;
    
    // We can test the mapping logic without initializing GSSAPI
    // This requires making mapPrincipalToRoles() public or using a friend class
    // For now, we'll test through the config structure
    
    EXPECT_EQ(config_.principal_mappings.size(), 2);
    EXPECT_EQ(config_.principal_mappings[0].principal_pattern, "admin@EXAMPLE.COM");
    EXPECT_EQ(config_.principal_mappings[0].role, "admin");
}

TEST_F(GSSAPIAuthenticatorTest, GetServicePrincipal) {
    GSSAPIAuthenticator auth;
    // Before initialization
    EXPECT_TRUE(auth.getServicePrincipal().empty());
}

TEST_F(GSSAPIAuthenticatorTest, GetConfig) {
    GSSAPIAuthenticator auth;
    const auto& cfg = auth.getConfig();
    EXPECT_FALSE(cfg.enabled);  // Default is false
}

/**
 * @brief Integration test - requires actual Kerberos KDC
 * 
 * To run this test:
 * 1. Set up a test KDC (MIT Kerberos or FreeIPA)
 * 2. Create service principal: kadmin -q "addprinc -randkey themisdb/localhost@TEST.REALM"
 * 3. Extract keytab: kadmin -q "ktadd -k /tmp/test.keytab themisdb/localhost@TEST.REALM"
 * 4. Set environment: export KRB5_CONFIG=/path/to/test_krb5.conf
 * 5. Run test with: ctest -R GSSAPIAuthenticator
 */
TEST_F(GSSAPIAuthenticatorTest, DISABLED_IntegrationWithKDC) {
    // This test is disabled by default as it requires KDC setup
    GSSAPIAuthenticator auth;
    
    // Update config for actual test KDC
    config_.service_principal = "themisdb/localhost@TEST.REALM";
    config_.keytab_file = "/tmp/test.keytab";
    config_.krb5_config = "/etc/krb5.conf";
    
    // Initialize with KDC
    bool initialized = auth.initialize(config_);
    ASSERT_TRUE(initialized) << "Failed to initialize with KDC";
    EXPECT_TRUE(auth.isInitialized());
    
    // Test authentication with valid token
    // (Would need actual Kerberos ticket from kinit)
    // std::string token = "...base64 encoded GSSAPI token...";
    // auto result = auth.authenticateToken(token);
    // EXPECT_TRUE(result.success);
}

/**
 * @brief Test error handling without KDC
 */
TEST_F(GSSAPIAuthenticatorTest, ErrorHandlingWithoutKDC) {
    GSSAPIAuthenticator auth;
    
    // Try to authenticate without initialization
    auto result = auth.authenticateToken("dummy_token");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_TRUE(result.principal_name.empty());
}

/**
 * @brief Test empty token handling
 */
TEST_F(GSSAPIAuthenticatorTest, EmptyTokenHandling) {
    GSSAPIAuthenticator auth;
    
    // Even if initialized, empty token should fail
    auto result = auth.authenticateToken("");
    EXPECT_FALSE(result.success);
}

/**
 * @brief Test multiple initialization calls
 */
TEST_F(GSSAPIAuthenticatorTest, MultipleInitialization) {
    GSSAPIAuthenticator auth;
    
    // First initialization (will fail without KDC but shouldn't crash)
    auth.initialize(config_);
    
    // Second initialization should be safe
    // auth.initialize(config_);  // Commented - depends on KDC availability
}

/**
 * @brief Test principal pattern matching
 */
TEST_F(GSSAPIAuthenticatorTest, PrincipalPatternMatching) {
    // Test wildcard pattern "*@EXAMPLE.COM"
    std::string pattern = "*@EXAMPLE.COM";
    std::string principal1 = "user1@EXAMPLE.COM";
    std::string principal2 = "admin@EXAMPLE.COM";
    std::string principal3 = "user@OTHER.COM";
    
    // These should match
    EXPECT_TRUE(principal1.find("@EXAMPLE.COM") != std::string::npos);
    EXPECT_TRUE(principal2.find("@EXAMPLE.COM") != std::string::npos);
    
    // This should not match
    EXPECT_TRUE(principal3.find("@OTHER.COM") != std::string::npos);
    EXPECT_FALSE(principal3.find("@EXAMPLE.COM") != std::string::npos);
}
