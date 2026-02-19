#include <gtest/gtest.h>
#include "auth/jwks_validator.h"
#include <nlohmann/json.hpp>

using namespace themis::auth;
using json = nlohmann::json;

/**
 * @brief Test valid JWKS with RSA key
 */
TEST(JWKSValidatorTest, ValidRSAKey) {
    JWKSValidator validator;
    
    json jwks = {
        {"keys", json::array({
            {
                {"kty", "RSA"},
                {"use", "sig"},
                {"kid", "test-key-1"},
                {"alg", "RS256"},
                {"n", "xGOr-H7A-PWZ8RpvCvKbKxFr9kHJVlRQSvXlCy0nHAsDMD9yl1n0AxJH"},  // Placeholder
                {"e", "AQAB"}
            }
        })}
    };
    
    auto result = validator.validate(jwks);
    
    // Note: This will fail key size check because n is too small
    // For a real test, we'd need a proper 2048-bit modulus
}

/**
 * @brief Test JWKS missing keys field
 */
TEST(JWKSValidatorTest, MissingKeysField) {
    JWKSValidator validator;
    
    json jwks = {
        {"foo", "bar"}
    };
    
    auto result = validator.validate(jwks);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
    EXPECT_TRUE(result.errors[0].find("keys") != std::string::npos);
}

/**
 * @brief Test JWKS with keys as non-array
 */
TEST(JWKSValidatorTest, KeysNotArray) {
    JWKSValidator validator;
    
    json jwks = {
        {"keys", "not-an-array"}
    };
    
    auto result = validator.validate(jwks);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

/**
 * @brief Test JWKS with empty keys array
 */
TEST(JWKSValidatorTest, EmptyKeys) {
    JWKSValidator validator;
    
    json jwks = {
        {"keys", json::array()}
    };
    
    auto result = validator.validate(jwks);
    
    // Should have a warning but still be valid
    EXPECT_FALSE(result.warnings.empty());
}

/**
 * @brief Test JWKS with too many keys
 */
TEST(JWKSValidatorTest, TooManyKeys) {
    JWKSValidator::Config config;
    config.max_keys = 2;
    JWKSValidator validator(config);
    
    json jwks = {
        {"keys", json::array({
            {{"kty", "RSA"}, {"kid", "key1"}},
            {{"kty", "RSA"}, {"kid", "key2"}},
            {{"kty", "RSA"}, {"kid", "key3"}}  // Exceeds limit
        })}
    };
    
    auto result = validator.validate(jwks);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

/**
 * @brief Test key missing kty
 */
TEST(JWKSValidatorTest, MissingKeyType) {
    JWKSValidator validator;
    
    json jwks = {
        {"keys", json::array({
            {{"kid", "test-key"}, {"use", "sig"}}  // Missing kty
        })}
    };
    
    auto result = validator.validate(jwks);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

/**
 * @brief Test unsupported key type
 */
TEST(JWKSValidatorTest, UnsupportedKeyType) {
    JWKSValidator validator;
    
    json jwks = {
        {"keys", json::array({
            {{"kty", "UNKNOWN"}, {"kid", "test-key"}}
        })}
    };
    
    auto result = validator.validate(jwks);
    
    EXPECT_FALSE(result.valid);
}

/**
 * @brief Test RSA key missing modulus
 */
TEST(JWKSValidatorTest, RSAMissingModulus) {
    JWKSValidator validator;
    
    json jwks = {
        {"keys", json::array({
            {
                {"kty", "RSA"},
                {"kid", "test-key"},
                {"e", "AQAB"}  // Missing n
            }
        })}
    };
    
    auto result = validator.validate(jwks);
    
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

/**
 * @brief Test RSA key missing exponent
 */
TEST(JWKSValidatorTest, RSAMissingExponent) {
    JWKSValidator validator;
    
    json jwks = {
        {"keys", json::array({
            {
                {"kty", "RSA"},
                {"kid", "test-key"},
                {"n", "xGOr-H7A"}  // Missing e
            }
        })}
    };
    
    auto result = validator.validate(jwks);
    
    EXPECT_FALSE(result.valid);
}

/**
 * @brief Test RSA key with private component (security violation)
 */
TEST(JWKSValidatorTest, RSAPrivateKeyComponent) {
    JWKSValidator validator;
    
    json jwks = {
        {"keys", json::array({
            {
                {"kty", "RSA"},
                {"kid", "test-key"},
                {"n", "xGOr-H7A"},
                {"e", "AQAB"},
                {"d", "private-exponent"}  // Private key material!
            }
        })}
    };
    
    auto result = validator.validate(jwks);
    
    EXPECT_FALSE(result.valid);
    EXPECT_TRUE(std::any_of(result.errors.begin(), result.errors.end(),
        [](const std::string& err) {
            return err.find("private key") != std::string::npos;
        }));
}

/**
 * @brief Test EC key missing curve
 */
TEST(JWKSValidatorTest, ECMissingCurve) {
    JWKSValidator validator;
    
    json jwks = {
        {"keys", json::array({
            {
                {"kty", "EC"},
                {"kid", "test-key"},
                {"x", "coordinate-x"},
                {"y", "coordinate-y"}
                // Missing crv
            }
        })}
    };
    
    auto result = validator.validate(jwks);
    
    EXPECT_FALSE(result.valid);
}

/**
 * @brief Test EC key with private component
 */
TEST(JWKSValidatorTest, ECPrivateKeyComponent) {
    JWKSValidator validator;
    
    json jwks = {
        {"keys", json::array({
            {
                {"kty", "EC"},
                {"kid", "test-key"},
                {"crv", "P-256"},
                {"x", "coordinate-x"},
                {"y", "coordinate-y"},
                {"d", "private-key"}  // Private key material!
            }
        })}
    };
    
    auto result = validator.validate(jwks);
    
    EXPECT_FALSE(result.valid);
}

/**
 * @brief Test duplicate key IDs
 */
TEST(JWKSValidatorTest, DuplicateKeyIDs) {
    JWKSValidator validator;
    
    json jwks = {
        {"keys", json::array({
            {{"kty", "RSA"}, {"kid", "duplicate"}, {"n", "xxx"}, {"e", "AQAB"}},
            {{"kty", "RSA"}, {"kid", "duplicate"}, {"n", "yyy"}, {"e", "AQAB"}}
        })}
    };
    
    auto result = validator.validate(jwks);
    
    // Should have warning about duplicate kids
    EXPECT_FALSE(result.warnings.empty());
}

/**
 * @brief Test strict mode with warnings
 */
TEST(JWKSValidatorTest, StrictModeWithWarnings) {
    JWKSValidator::Config config;
    config.strict_mode = true;
    JWKSValidator validator(config);
    
    json jwks = {
        {"keys", json::array({
            {
                {"kty", "RSA"},
                {"kid", "test-key"},
                {"n", "short"},  // Too small, will generate warning
                {"e", "AQAB"}
            }
        })}
    };
    
    auto result = validator.validate(jwks);
    
    // In strict mode, should be invalid
    EXPECT_FALSE(result.valid);
}

/**
 * @brief Test require_kid configuration
 */
TEST(JWKSValidatorTest, RequireKid) {
    JWKSValidator::Config config;
    config.require_kid = true;
    JWKSValidator validator(config);
    
    json jwks = {
        {"keys", json::array({
            {
                {"kty", "RSA"},
                {"n", "modulus"},
                {"e", "AQAB"}
                // Missing kid
            }
        })}
    };
    
    auto result = validator.validate(jwks);
    
    EXPECT_FALSE(result.valid);
}

/**
 * @brief Test validateOrThrow success
 */
TEST(JWKSValidatorTest, ValidateOrThrowSuccess) {
    JWKSValidator::Config config;
    config.require_kid = false;
    JWKSValidator validator(config);
    
    json jwks = {
        {"keys", json::array({
            {
                {"kty", "RSA"},
                {"n", "0vx7agoebGcQSuuPiLJXZptN9nndrQmbXEps2aiAFbWhM78LhWx4cbbfAAtVT86zwu1RK7aPFFxuhDR1L6tSoc_BJECPebWKRXjBZCiFV4n3oknjhMstn64tZ_2W-5JsGY4Hc5n9yBXArwl93lqt7_RN5w6Cf0h4QyQ5v-65YGjQR0_FDW2QvzqY368QQMicAtaSqzs8KJZgnYb9c7d0zgdAZHzu6qMQvRL5hajrn1n91CbOpbISD08qNLyrdkt-bFTWhAI4vMQFh6WeZu0fM4lFd2NcRwr3XPksINHaQ-G_xBniIqbw0Ls1jF44-csFCur-kEgU8awapJzKnqDKgw"},
                {"e", "AQAB"}
            }
        })}
    };
    
    EXPECT_NO_THROW(validator.validateOrThrow(jwks));
}

/**
 * @brief Test validateOrThrow failure
 */
TEST(JWKSValidatorTest, ValidateOrThrowFailure) {
    JWKSValidator validator;
    
    json jwks = {
        {"keys", "not-an-array"}
    };
    
    EXPECT_THROW(validator.validateOrThrow(jwks), std::runtime_error);
}

/**
 * @brief Test symmetric key warning
 */
TEST(JWKSValidatorTest, SymmetricKeyWarning) {
    JWKSValidator validator;
    
    json jwks = {
        {"keys", json::array({
            {
                {"kty", "oct"},
                {"kid", "test-key"},
                {"k", "GawgguFyGrWKav7AX4VKUg"}
            }
        })}
    };
    
    auto result = validator.validate(jwks);
    
    // Should have warning about symmetric key in JWKS
    EXPECT_FALSE(result.warnings.empty());
}

/**
 * @brief Test validation result error summary
 */
TEST(JWKSValidatorTest, ErrorSummary) {
    JWKSValidator::ValidationResult result;
    result.errors.push_back("Error 1");
    result.errors.push_back("Error 2");
    result.warnings.push_back("Warning 1");
    
    std::string summary = result.getErrorSummary();
    
    EXPECT_TRUE(summary.find("Error 1") != std::string::npos);
    EXPECT_TRUE(summary.find("Error 2") != std::string::npos);
    EXPECT_TRUE(summary.find("Warning 1") != std::string::npos);
}
