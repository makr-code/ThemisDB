#include <gtest/gtest.h>
#include "auth/auth_error.h"
#include <nlohmann/json.hpp>

using namespace themis::auth;

/**
 * @brief Test basic AuthError creation
 */
TEST(AuthErrorTest, BasicCreation) {
    AuthError error(
        AuthErrorCode::AUTH_TOKEN_INVALID,
        "Token is invalid",
        "Token signature verification failed for kid: abc123"
    );
    
    EXPECT_EQ(error.code(), AuthErrorCode::AUTH_TOKEN_INVALID);
    EXPECT_EQ(error.publicMessage(), "Token is invalid");
    EXPECT_EQ(error.internalMessage(), "Token signature verification failed for kid: abc123");
    EXPECT_FALSE(error.requestId().empty());
}

/**
 * @brief Test AuthError with custom request ID
 */
TEST(AuthErrorTest, CustomRequestId) {
    AuthError error(
        AuthErrorCode::AUTH_TOKEN_EXPIRED,
        "Token expired",
        "",
        "req-12345"
    );
    
    EXPECT_EQ(error.requestId(), "req-12345");
}

/**
 * @brief Test AuthError JSON serialization (public)
 */
TEST(AuthErrorTest, PublicJSONSerialization) {
    AuthError error(
        AuthErrorCode::JWT_INVALID_SIGNATURE,
        "Signature verification failed",
        "Internal details here",
        "req-test"
    );
    
    auto json = error.toPublicJSON();
    
    EXPECT_TRUE(json.contains("error"));
    EXPECT_EQ(json["error"]["code"], static_cast<int>(AuthErrorCode::JWT_INVALID_SIGNATURE));
    EXPECT_EQ(json["error"]["message"], "Signature verification failed");
    EXPECT_EQ(json["error"]["request_id"], "req-test");
    EXPECT_TRUE(json["error"].contains("timestamp"));
    
    // Internal message should NOT be in public JSON
    EXPECT_FALSE(json["error"].contains("internal_message"));
}

/**
 * @brief Test AuthError JSON serialization (internal)
 */
TEST(AuthErrorTest, InternalJSONSerialization) {
    AuthError error(
        AuthErrorCode::JWT_INVALID_SIGNATURE,
        "Signature verification failed",
        "Failed to verify signature with key abc123 from /etc/keys/private.key",
        "req-test"
    );
    
    auto json = error.toInternalJSON();
    
    EXPECT_TRUE(json.contains("error"));
    EXPECT_TRUE(json["error"].contains("internal_message"));
    EXPECT_EQ(json["error"]["internal_message"], 
              "Failed to verify signature with key abc123 from /etc/keys/private.key");
}

/**
 * @brief Test retry-after functionality
 */
TEST(AuthErrorTest, RetryAfter) {
    AuthError error(
        AuthErrorCode::AUTH_RATE_LIMIT_EXCEEDED,
        "Rate limit exceeded",
        "IP 192.168.1.100 exceeded limit"
    );
    
    EXPECT_FALSE(error.retryAfter().has_value());
    
    error.setRetryAfter(std::chrono::seconds(60));
    
    ASSERT_TRUE(error.retryAfter().has_value());
    EXPECT_EQ(error.retryAfter()->count(), 60);
    
    auto json = error.toPublicJSON();
    EXPECT_TRUE(json["error"].contains("retry_after_seconds"));
    EXPECT_EQ(json["error"]["retry_after_seconds"], 60);
}

/**
 * @brief Test email masking
 */
TEST(AuthErrorTest, MaskEmail) {
    std::string input = "User alice@example.com failed authentication";
    std::string masked = AuthError::maskSensitiveData(input);
    
    // Should mask email but preserve domain
    EXPECT_TRUE(masked.find("alice@example.com") == std::string::npos);
    EXPECT_TRUE(masked.find("example.com") != std::string::npos);
    EXPECT_TRUE(masked.find("al***@example.com") != std::string::npos || 
                masked.find("***@example.com") != std::string::npos);
}

/**
 * @brief Test Kerberos principal masking
 */
TEST(AuthErrorTest, MaskPrincipal) {
    std::string input = "Principal alice@EXAMPLE.COM authentication failed";
    std::string masked = AuthError::maskSensitiveData(input);
    
    // Should mask principal but keep realm
    EXPECT_TRUE(masked.find("alice@EXAMPLE.COM") == std::string::npos);
    EXPECT_TRUE(masked.find("***@EXAMPLE.COM") != std::string::npos);
}

/**
 * @brief Test file path masking
 */
TEST(AuthErrorTest, MaskFilePath) {
    std::string input = "Failed to read keytab from /etc/themisdb/secret.keytab";
    std::string masked = AuthError::maskSensitiveData(input);
    
    // Should mask directory but keep filename
    EXPECT_TRUE(masked.find("/etc/themisdb/secret.keytab") == std::string::npos);
    EXPECT_TRUE(masked.find("secret.keytab") != std::string::npos);
    EXPECT_TRUE(masked.find("***/secret.keytab") != std::string::npos);
}

/**
 * @brief Test IP address masking
 */
TEST(AuthErrorTest, MaskIPAddress) {
    std::string input = "Authentication failed from IP 192.168.1.100";
    std::string masked = AuthError::maskSensitiveData(input);
    
    // Should mask last 3 octets
    EXPECT_TRUE(masked.find("192.168.1.100") == std::string::npos);
    EXPECT_TRUE(masked.find("192.*.*.*") != std::string::npos);
}

/**
 * @brief Test multiple sensitive data types in one string
 */
TEST(AuthErrorTest, MaskMultipleSensitiveData) {
    std::string input = "User alice@example.com from 192.168.1.100 failed to authenticate with keytab /etc/themisdb/service.keytab";
    std::string masked = AuthError::maskSensitiveData(input);
    
    // Should mask all sensitive data
    EXPECT_TRUE(masked.find("alice@example.com") == std::string::npos);
    EXPECT_TRUE(masked.find("192.168.1.100") == std::string::npos);
    EXPECT_TRUE(masked.find("/etc/themisdb/service.keytab") == std::string::npos);
    
    // But preserve some information
    EXPECT_TRUE(masked.find("example.com") != std::string::npos);
    EXPECT_TRUE(masked.find("192.*.*.*") != std::string::npos);
    EXPECT_TRUE(masked.find("service.keytab") != std::string::npos);
}

/**
 * @brief Test AuthError from exception
 */
TEST(AuthErrorTest, FromException) {
    try {
        throw std::runtime_error("Token expired at 2024-01-01");
    } catch (const std::exception& e) {
        auto error = AuthError::fromException(e, "req-test");
        
        EXPECT_EQ(error.code(), AuthErrorCode::AUTH_TOKEN_EXPIRED);
        EXPECT_EQ(error.requestId(), "req-test");
        EXPECT_FALSE(error.publicMessage().empty());
        EXPECT_FALSE(error.internalMessage().empty());
    }
}

/**
 * @brief Test AuthException throwing and catching
 */
TEST(AuthErrorTest, AuthException) {
    AuthError error(
        AuthErrorCode::AUTH_TOKEN_INVALID,
        "Token invalid",
        "Internal details"
    );
    
    try {
        throw AuthException(error);
        FAIL() << "Should have thrown AuthException";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::AUTH_TOKEN_INVALID);
        EXPECT_EQ(e.error().publicMessage(), "Token invalid");
        EXPECT_STREQ(e.what(), "Token invalid");
    }
}

/**
 * @brief Test error code conversion
 */
TEST(AuthErrorTest, ErrorCodeConversion) {
    auto code = toErrorCode(AuthErrorCode::AUTH_TOKEN_INVALID);
    EXPECT_EQ(static_cast<int>(code), 9302);
}

/**
 * @brief Test request ID generation
 */
TEST(AuthErrorTest, RequestIdGeneration) {
    AuthError error1(AuthErrorCode::AUTH_TOKEN_INVALID, "Message 1");
    AuthError error2(AuthErrorCode::AUTH_TOKEN_INVALID, "Message 2");
    
    // Request IDs should be unique
    EXPECT_NE(error1.requestId(), error2.requestId());
    
    // Should have correct format (auth-XXXXXXXX)
    EXPECT_TRUE(error1.requestId().find("auth-") == 0);
    EXPECT_EQ(error1.requestId().length(), 13);  // "auth-" + 8 hex chars
}

/**
 * @brief Test error registration
 */
TEST(AuthErrorTest, ErrorRegistration) {
    // Register auth errors
    registerAuthErrors();
    
    // Verify some errors are registered
    auto& registry = themis::errors::ErrorRegistry::getInstance();
    
    auto error = registry.getError(toErrorCode(AuthErrorCode::AUTH_TOKEN_INVALID));
    EXPECT_EQ(error.category, "Authentication");
    EXPECT_FALSE(error.message_template.empty());
}

/**
 * @brief Test macro for throwing auth errors
 */
TEST(AuthErrorTest, ThrowMacro) {
    try {
        THROW_AUTH_ERROR(
            AuthErrorCode::AUTH_TOKEN_INVALID,
            "Public message",
            "Internal message"
        );
        FAIL() << "Should have thrown";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::AUTH_TOKEN_INVALID);
        EXPECT_EQ(e.error().publicMessage(), "Public message");
        EXPECT_EQ(e.error().internalMessage(), "Internal message");
    }
}

/**
 * @brief Test macro with request ID
 */
TEST(AuthErrorTest, ThrowMacroWithRequestId) {
    try {
        THROW_AUTH_ERROR_WITH_ID(
            AuthErrorCode::AUTH_RATE_LIMIT_EXCEEDED,
            "Rate limited",
            "Too many requests from 192.168.1.100",
            "req-custom"
        );
        FAIL() << "Should have thrown";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().requestId(), "req-custom");
    }
}

/**
 * @brief Test error logging (just verify it doesn't crash)
 */
TEST(AuthErrorTest, ErrorLogging) {
    AuthError error(
        AuthErrorCode::AUTH_TOKEN_EXPIRED,
        "Token expired",
        "Full details here",
        "req-log-test"
    );
    
    // Should not crash
    EXPECT_NO_THROW(error.logError());
}

/**
 * @brief Test timestamp is set correctly
 */
TEST(AuthErrorTest, Timestamp) {
    auto before = std::chrono::system_clock::now();
    
    AuthError error(
        AuthErrorCode::AUTH_TOKEN_INVALID,
        "Test message"
    );
    
    auto after = std::chrono::system_clock::now();
    
    // Timestamp should be between before and after
    EXPECT_GE(error.timestamp(), before);
    EXPECT_LE(error.timestamp(), after);
}

/**
 * @brief Test all auth error codes are in valid range
 */
TEST(AuthErrorTest, ErrorCodeRange) {
    // All auth error codes should be in 9300-9399 range
    EXPECT_GE(static_cast<int>(AuthErrorCode::AUTH_GENERAL_FAILURE), 9300);
    EXPECT_LE(static_cast<int>(AuthErrorCode::AUTH_NOT_IMPLEMENTED), 9399);
    
    EXPECT_EQ(static_cast<int>(AuthErrorCode::AUTH_GENERAL_FAILURE), 9300);
    EXPECT_EQ(static_cast<int>(AuthErrorCode::JWT_INVALID_FORMAT), 9310);
    EXPECT_EQ(static_cast<int>(AuthErrorCode::GSSAPI_INITIALIZATION_FAILED), 9330);
    EXPECT_EQ(static_cast<int>(AuthErrorCode::MFA_CODE_INVALID), 9350);
    EXPECT_EQ(static_cast<int>(AuthErrorCode::AUTH_RATE_LIMIT_EXCEEDED), 9370);
}
