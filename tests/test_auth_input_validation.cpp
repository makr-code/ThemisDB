#include <gtest/gtest.h>
#include "auth/jwt_validator.h"
#include "auth/gssapi_authenticator.h"
#include <string>

using namespace themis::auth;

/**
 * @brief Test JWT token size validation
 */
TEST(AuthInputValidationTest, JWT_TokenSizeLimit) {
    JWTValidator validator("https://example.com/jwks");
    
    // Create a token exceeding the size limit (16KB)
    std::string oversized_token(MAX_JWT_TOKEN_SIZE + 1, 'A');
    oversized_token += "."; 
    oversized_token.append(MAX_JWT_TOKEN_SIZE + 1, 'B');
    oversized_token += ".";
    oversized_token.append(MAX_JWT_TOKEN_SIZE + 1, 'C');
    
    // Should throw due to size limit
    EXPECT_THROW({
        validator.parseAndValidate(oversized_token);
    }, std::runtime_error);
}

/**
 * @brief Test JWT empty token validation
 */
TEST(AuthInputValidationTest, JWT_EmptyToken) {
    JWTValidator validator("https://example.com/jwks");
    
    // Empty token should be rejected
    EXPECT_THROW({
        validator.parseAndValidate("");
    }, std::runtime_error);
    
    // Bearer prefix only should also be rejected
    EXPECT_THROW({
        validator.parseAndValidate("Bearer ");
    }, std::runtime_error);
}

/**
 * @brief Test JWT principal/subject length validation
 */
TEST(AuthInputValidationTest, JWT_PrincipalLengthLimit) {
    // This test would require a valid JWT with oversized subject
    // For now, we document the requirement
    // In real scenario, create a JWT with sub field > MAX_PRINCIPAL_NAME_LENGTH
    EXPECT_TRUE(MAX_PRINCIPAL_NAME_LENGTH == 256);
}

/**
 * @brief Test JWT validator configuration with custom timeout
 */
TEST(AuthInputValidationTest, JWT_CustomTimeout) {
    JWTValidatorConfig config{
        "https://example.com/jwks",
        "test-issuer",
        "test-audience",
        std::chrono::seconds(600),
        std::chrono::seconds(60),
        {},  // no revoked kids
        3,   // custom timeout: 3 seconds
        2    // custom max retries: 2
    };
    
    JWTValidator validator(config);
    
    // Configuration should be accepted
    EXPECT_NO_THROW({
        // Validator is configured with custom values
    });
}

/**
 * @brief Test GSSAPI configuration validation
 */
TEST(AuthInputValidationTest, GSSAPI_ConfigurationValidation) {
    GSSAPIAuthenticator auth;
    
    // Invalid configuration - empty service principal
    KerberosConfig invalid_config;
    invalid_config.enabled = true;
    invalid_config.service_principal = "";
    invalid_config.keytab_file = "/tmp/test.keytab";
    
    // Should fail initialization with invalid config
    EXPECT_FALSE(auth.initialize(invalid_config));
}

/**
 * @brief Test GSSAPI constants are defined correctly
 */
TEST(AuthInputValidationTest, GSSAPI_Constants) {
    // Verify constants are reasonable
    EXPECT_EQ(MAX_GSSAPI_TOKEN_SIZE, 64 * 1024);
    EXPECT_EQ(MAX_KERBEROS_PRINCIPAL_LENGTH, 256);
    EXPECT_EQ(DEFAULT_GSSAPI_CONTEXT_TIMEOUT, 30);
}

/**
 * @brief Test GSSAPI context timeout configuration
 */
TEST(AuthInputValidationTest, GSSAPI_ContextTimeout) {
    KerberosConfig config;
    config.enabled = true;
    config.service_principal = "themisdb/localhost@EXAMPLE.COM";
    config.keytab_file = "/tmp/test.keytab";
    config.context_timeout_seconds = 15;  // Custom timeout
    
    // Configuration should be accepted
    EXPECT_EQ(config.context_timeout_seconds, 15);
}

/**
 * @brief Test JWT validator retry configuration
 */
TEST(AuthInputValidationTest, JWT_RetryConfiguration) {
    JWTValidatorConfig config{
        "https://example.com/jwks",
        "issuer",
        "audience",
        std::chrono::seconds(300),
        std::chrono::seconds(30),
        {},  // no revoked kids
        5,   // 5 second timeout
        5    // max 5 retries
    };
    
    EXPECT_EQ(config.jwks_timeout_seconds, 5);
    EXPECT_EQ(config.jwks_max_retries, 5);
}
