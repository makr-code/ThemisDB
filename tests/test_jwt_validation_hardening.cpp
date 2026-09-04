#include <gtest/gtest.h>
#include "auth/jwt_validator.h"
#include <nlohmann/json.hpp>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/bn.h>

using namespace themis::auth;

// Helper: base64url encode
static std::string b64url(const std::vector<uint8_t>& in) {
    static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64;
    b64.reserve(((in.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= in.size()) {
        uint32_t n = (in[i] << 16) | (in[i + 1] << 8) | in[i + 2];
        b64.push_back(tbl[(n >> 18) & 63]);
        b64.push_back(tbl[(n >> 12) & 63]);
        b64.push_back(tbl[(n >> 6) & 63]);
        b64.push_back(tbl[n & 63]);
        i += 3;
    }
    if (i + 1 == in.size()) {
        uint32_t n = (in[i] << 16);
        b64.push_back(tbl[(n >> 18) & 63]);
        b64.push_back(tbl[(n >> 12) & 63]);
    } else if (i + 2 == in.size()) {
        uint32_t n = (in[i] << 16) | (in[i + 1] << 8);
        b64.push_back(tbl[(n >> 18) & 63]);
        b64.push_back(tbl[(n >> 12) & 63]);
        b64.push_back(tbl[(n >> 6) & 63]);
    }
    // Convert to URL form and strip padding
    for (char& c : b64) {
        if (c == '+') {
          c = '-';
        }
        else if (c == '/') c = '_';
    }
    // Remove padding characters for base64url
    while (!b64.empty() && b64.back() == '=') {
        b64.pop_back();
    }
    return b64;
}

// RSA key fixture
struct RSAFixture {
    RSA* rsa = nullptr;
    EVP_PKEY* pkey = nullptr;
    BIGNUM* bn = nullptr;

    RSAFixture() {
        bn = BN_new();
        if (!bn) {
          throw std::runtime_error("BN_new failed");
        }
        if (BN_set_word(bn, RSA_F4) != 1) {
          throw std::runtime_error("BN_set_word failed");
        }
        rsa = RSA_new();
        if (!rsa) {
          throw std::runtime_error("RSA_new failed");
        }
        if (RSA_generate_key_ex(rsa, 2048, bn, nullptr) != 1) {
            throw std::runtime_error("RSA_generate_key_ex failed");
        }
        pkey = EVP_PKEY_new();
        if (!pkey) {
          throw std::runtime_error("EVP_PKEY_new failed");
        }
        if (EVP_PKEY_assign_RSA(pkey, rsa) != 1) {
            throw std::runtime_error("EVP_PKEY_assign_RSA failed");
        }
    }

    ~RSAFixture() {
        if (pkey) {
          EVP_PKEY_free(pkey);
        }
        if (bn) {
          BN_free(bn);
        }
    }
};

// Helper: sign using EVP_DigestSign
static std::string sign_RS256(EVP_PKEY* pkey, const std::string& header_payload) {
    EVP_MD_CTX* mctx = EVP_MD_CTX_new();
    if (!mctx) {
      throw std::runtime_error("EVP_MD_CTX_new failed");
    }
    size_t siglen = 0;
    if (EVP_DigestSignInit(mctx, nullptr, EVP_sha256(), nullptr, pkey) <= 0)
        throw std::runtime_error("EVP_DigestSignInit failed");
    if (EVP_DigestSignUpdate(mctx, header_payload.data(), header_payload.size()) <= 0)
        throw std::runtime_error("EVP_DigestSignUpdate failed");
    if (EVP_DigestSign(mctx, nullptr, &siglen, nullptr, 0) <= 0)
        throw std::runtime_error("EVP_DigestSign failed");
    std::vector<uint8_t> sig(siglen);
    if (EVP_DigestSign(mctx, sig.data(), &siglen, (const unsigned char*)header_payload.data(),
                       header_payload.size()) <= 0)
        throw std::runtime_error("EVP_DigestSign failed");
    EVP_MD_CTX_free(mctx);
    return b64url(sig);
}

// Helper: create test JWKS
static nlohmann::json create_jwks(RSA* rsa, const std::string& kid) {
    const BIGNUM* n = nullptr;
    const BIGNUM* e = nullptr;
    RSA_get0_key(rsa, &n, &e, nullptr);
    
    std::vector<uint8_t> n_bytes(BN_num_bytes(n));
    BN_bn2bin(n, n_bytes.data());
    std::vector<uint8_t> e_bytes(BN_num_bytes(e));
    BN_bn2bin(e, e_bytes.data());
    
    return nlohmann::json{
        {"keys", nlohmann::json::array({
            {
                {"kty", "RSA"},
                {"kid", kid},
                {"use", "sig"},
                {"alg", "RS256"},
                {"n", b64url(n_bytes)},
                {"e", b64url(e_bytes)}
            }
        })}
    };
}

/**
 * @brief Test JWT Validation Failures
 */
class JWTValidationTest : public ::testing::Test {
protected:
    std::unique_ptr<RSAFixture> rsa_fixture_;
    std::unique_ptr<JWTValidator> validator_;
    std::string kid_;
    
    void SetUp() override {
        rsa_fixture_ = std::make_unique<RSAFixture>();
        kid_ = "test-key-id";
        
        JWTValidatorConfig config;
        config.jwks_url = "https://test.example.com/jwks";
        config.expected_issuer = "https://test.example.com";
        config.expected_audience = "test-audience";
        config.clock_skew = std::chrono::seconds(60);
        
        validator_ = std::make_unique<JWTValidator>(config);
        
        // Set JWKS for testing
        auto jwks = create_jwks(rsa_fixture_->rsa, kid_);
        validator_->setJWKSForTesting(jwks);
    }
    
    std::string createToken(const nlohmann::json& header, const nlohmann::json& payload) {
        std::string header_b64 = b64url(std::vector<uint8_t>(header.dump().begin(), header.dump().end()));
        std::string payload_b64 = b64url(std::vector<uint8_t>(payload.dump().begin(), payload.dump().end()));
        std::string header_payload = header_b64 + "." + payload_b64;
        std::string sig = sign_RS256(rsa_fixture_->pkey, header_payload);
        return header_payload + "." + sig;
    }
};

TEST_F(JWTValidationTest, RejectsTokenWithWrongAlgorithm) {
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours(1);
    
    nlohmann::json header = {
        {"alg", "HS256"},  // Wrong algorithm
        {"typ", "JWT"},
        {"kid", kid_}
    };
    
    nlohmann::json payload = {
        {"sub", "user123"},
        {"iss", "https://test.example.com"},
        {"aud", "test-audience"},
        {"exp", std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count()}
    };
    
    std::string token = createToken(header, payload);
    
    EXPECT_THROW({
        try {
            validator_->parseAndValidate(token);
        } catch (const std::runtime_error& e) {
            EXPECT_NE(std::string(e.what()).find("Unsupported alg"), std::string::npos);
            throw;
        }
    }, std::runtime_error);
}

TEST_F(JWTValidationTest, RejectsTokenWithWrongIssuer) {
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours(1);
    
    nlohmann::json header = {
        {"alg", "RS256"},
        {"typ", "JWT"},
        {"kid", kid_}
    };
    
    nlohmann::json payload = {
        {"sub", "user123"},
        {"iss", "https://evil.example.com"},  // Wrong issuer
        {"aud", "test-audience"},
        {"exp", std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count()}
    };
    
    std::string token = createToken(header, payload);
    
    EXPECT_THROW({
        try {
            validator_->parseAndValidate(token);
        } catch (const std::runtime_error& e) {
            EXPECT_NE(std::string(e.what()).find("Issuer mismatch"), std::string::npos);
            throw;
        }
    }, std::runtime_error);
}

TEST_F(JWTValidationTest, RejectsTokenWithWrongAudience) {
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours(1);
    
    nlohmann::json header = {
        {"alg", "RS256"},
        {"typ", "JWT"},
        {"kid", kid_}
    };
    
    nlohmann::json payload = {
        {"sub", "user123"},
        {"iss", "https://test.example.com"},
        {"aud", "wrong-audience"},  // Wrong audience
        {"exp", std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count()}
    };
    
    std::string token = createToken(header, payload);
    
    EXPECT_THROW({
        try {
            validator_->parseAndValidate(token);
        } catch (const std::runtime_error& e) {
            EXPECT_NE(std::string(e.what()).find("Audience mismatch"), std::string::npos);
            throw;
        }
    }, std::runtime_error);
}

TEST_F(JWTValidationTest, RejectsExpiredToken) {
    auto now = std::chrono::system_clock::now();
    auto exp = now - std::chrono::hours(1);  // Expired
    
    nlohmann::json header = {
        {"alg", "RS256"},
        {"typ", "JWT"},
        {"kid", kid_}
    };
    
    nlohmann::json payload = {
        {"sub", "user123"},
        {"iss", "https://test.example.com"},
        {"aud", "test-audience"},
        {"exp", std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count()}
    };
    
    std::string token = createToken(header, payload);
    
    EXPECT_THROW({
        try {
            validator_->parseAndValidate(token);
        } catch (const std::runtime_error& e) {
            EXPECT_NE(std::string(e.what()).find("expired"), std::string::npos);
            throw;
        }
    }, std::runtime_error);
}

TEST_F(JWTValidationTest, RejectsTokenWithMissingExp) {
    nlohmann::json header = {
        {"alg", "RS256"},
        {"typ", "JWT"},
        {"kid", kid_}
    };
    
    nlohmann::json payload = {
        {"sub", "user123"},
        {"iss", "https://test.example.com"},
        {"aud", "test-audience"}
        // Missing exp
    };
    
    std::string token = createToken(header, payload);
    
    EXPECT_THROW({
        try {
            validator_->parseAndValidate(token);
        } catch (const std::runtime_error& e) {
            EXPECT_NE(std::string(e.what()).find("Missing exp"), std::string::npos);
            throw;
        }
    }, std::runtime_error);
}

TEST_F(JWTValidationTest, RejectsTokenNotYetValid) {
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours(1);
    auto nbf = now + std::chrono::hours(2);  // Not valid yet
    
    nlohmann::json header = {
        {"alg", "RS256"},
        {"typ", "JWT"},
        {"kid", kid_}
    };
    
    nlohmann::json payload = {
        {"sub", "user123"},
        {"iss", "https://test.example.com"},
        {"aud", "test-audience"},
        {"exp", std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count()},
        {"nbf", std::chrono::duration_cast<std::chrono::seconds>(nbf.time_since_epoch()).count()}
    };
    
    std::string token = createToken(header, payload);
    
    EXPECT_THROW({
        try {
            validator_->parseAndValidate(token);
        } catch (const std::runtime_error& e) {
            EXPECT_NE(std::string(e.what()).find("not yet valid"), std::string::npos);
            throw;
        }
    }, std::runtime_error);
}

TEST_F(JWTValidationTest, AcceptsValidToken) {
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours(1);
    
    nlohmann::json header = {
        {"alg", "RS256"},
        {"typ", "JWT"},
        {"kid", kid_}
    };
    
    nlohmann::json payload = {
        {"sub", "user123"},
        {"iss", "https://test.example.com"},
        {"aud", "test-audience"},
        {"exp", std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count()},
        {"email", "user@example.com"},
        {"groups", nlohmann::json::array({"admin", "users"})}
    };
    
    std::string token = createToken(header, payload);
    
    EXPECT_NO_THROW({
        auto claims = validator_->parseAndValidate(token);
        EXPECT_EQ(claims.sub, "user123");
        EXPECT_EQ(claims.email, "user@example.com");
        EXPECT_EQ(claims.issuer, "https://test.example.com");
        EXPECT_EQ(claims.groups.size(), 2);
    });
}

/**
 * @brief Test Kid Revocation
 */
class KidRevocationTest : public ::testing::Test {
protected:
    std::unique_ptr<RSAFixture> rsa_fixture_;
    std::unique_ptr<JWTValidator> validator_;
    std::string kid_;
    
    void SetUp() override {
        rsa_fixture_ = std::make_unique<RSAFixture>();
        kid_ = "test-key-id";
        
        JWTValidatorConfig config;
        config.jwks_url = "https://test.example.com/jwks";
        config.expected_issuer = "https://test.example.com";
        config.expected_audience = "test-audience";
        
        validator_ = std::make_unique<JWTValidator>(config);
        
        auto jwks = create_jwks(rsa_fixture_->rsa, kid_);
        validator_->setJWKSForTesting(jwks);
    }
    
    std::string createToken(const std::string& kid) {
        auto now = std::chrono::system_clock::now();
        auto exp = now + std::chrono::hours(1);
        
        nlohmann::json header = {
            {"alg", "RS256"},
            {"typ", "JWT"},
            {"kid", kid}
        };
        
        nlohmann::json payload = {
            {"sub", "user123"},
            {"iss", "https://test.example.com"},
            {"aud", "test-audience"},
            {"exp", std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count()}
        };
        
        std::string header_b64 = b64url(std::vector<uint8_t>(header.dump().begin(), header.dump().end()));
        std::string payload_b64 = b64url(std::vector<uint8_t>(payload.dump().begin(), payload.dump().end()));
        std::string header_payload = header_b64 + "." + payload_b64;
        std::string sig = sign_RS256(rsa_fixture_->pkey, header_payload);
        return header_payload + "." + sig;
    }
};

TEST_F(KidRevocationTest, RejectsRevokedKid) {
    std::string token = createToken(kid_);
    
    // Revoke the kid
    validator_->revokeKid(kid_);
    
    EXPECT_THROW({
        try {
            validator_->parseAndValidate(token);
        } catch (const std::runtime_error& e) {
            EXPECT_NE(std::string(e.what()).find("revoked"), std::string::npos);
            throw;
        }
    }, std::runtime_error);
}

TEST_F(KidRevocationTest, AcceptsNonRevokedKid) {
    std::string token = createToken(kid_);
    
    // Revoke a different kid
    validator_->revokeKid("other-kid");
    
    EXPECT_NO_THROW({
        auto claims = validator_->parseAndValidate(token);
        EXPECT_EQ(claims.sub, "user123");
    });
}

TEST_F(KidRevocationTest, ChecksConfigDenylist) {
    JWTValidatorConfig config;
    config.jwks_url = "https://test.example.com/jwks";
    config.expected_issuer = "https://test.example.com";
    config.expected_audience = "test-audience";
    config.revoked_kids = {kid_};  // Kid in config denylist
    
    auto validator_with_denylist = std::make_unique<JWTValidator>(config);
    auto jwks = create_jwks(rsa_fixture_->rsa, kid_);
    validator_with_denylist->setJWKSForTesting(jwks);
    
    std::string token = createToken(kid_);
    
    EXPECT_THROW({
        validator_with_denylist->parseAndValidate(token);
    }, std::runtime_error);
}

// ============================================================================
// Enhanced Validation Tests - Mandatory Issuer and Audience
// ============================================================================

TEST_F(JWTValidationTest, RejectsMissingIssuerWhenRequired) {
    // Create config with mandatory issuer validation
    JWTValidatorConfig config;
    config.jwks_url = "https://test.example.com/jwks";
    config.expected_issuer = "https://test.example.com";
    config.expected_audience = "test-audience";
    config.require_issuer_validation = true;  // Mandatory issuer validation
    
    auto validator_strict = std::make_unique<JWTValidator>(config);
    auto jwks = create_jwks(rsa_fixture_->rsa, kid_);
    validator_strict->setJWKSForTesting(jwks);
    
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours(1);
    
    nlohmann::json header = {
        {"alg", "RS256"},
        {"typ", "JWT"},
        {"kid", kid_}
    };
    
    // Missing "iss" claim
    nlohmann::json payload = {
        {"sub", "user123"},
        {"aud", "test-audience"},
        {"exp", std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count()}
    };
    
    std::string token = createToken(header, payload);
    
    EXPECT_THROW({
        try {
            validator_strict->parseAndValidate(token);
        } catch (const std::runtime_error& e) {
            EXPECT_NE(std::string(e.what()).find("Missing required iss claim"), std::string::npos);
            throw;
        }
    }, std::runtime_error);
}

TEST_F(JWTValidationTest, RejectsMissingAudienceWhenRequired) {
    // Create config with mandatory audience validation
    JWTValidatorConfig config;
    config.jwks_url = "https://test.example.com/jwks";
    config.expected_issuer = "https://test.example.com";
    config.expected_audience = "test-audience";
    config.require_audience_validation = true;  // Mandatory audience validation
    
    auto validator_strict = std::make_unique<JWTValidator>(config);
    auto jwks = create_jwks(rsa_fixture_->rsa, kid_);
    validator_strict->setJWKSForTesting(jwks);
    
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours(1);
    
    nlohmann::json header = {
        {"alg", "RS256"},
        {"typ", "JWT"},
        {"kid", kid_}
    };
    
    // Missing "aud" claim
    nlohmann::json payload = {
        {"sub", "user123"},
        {"iss", "https://test.example.com"},
        {"exp", std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count()}
    };
    
    std::string token = createToken(header, payload);
    
    EXPECT_THROW({
        try {
            validator_strict->parseAndValidate(token);
        } catch (const std::runtime_error& e) {
            EXPECT_NE(std::string(e.what()).find("Audience mismatch"), std::string::npos);
            throw;
        }
    }, std::runtime_error);
}

TEST_F(JWTValidationTest, AcceptsValidTokenWithMandatoryValidation) {
    // Create config with mandatory issuer and audience validation
    JWTValidatorConfig config;
    config.jwks_url = "https://test.example.com/jwks";
    config.expected_issuer = "https://test.example.com";
    config.expected_audience = "test-audience";
    config.require_issuer_validation = true;
    config.require_audience_validation = true;
    
    auto validator_strict = std::make_unique<JWTValidator>(config);
    auto jwks = create_jwks(rsa_fixture_->rsa, kid_);
    validator_strict->setJWKSForTesting(jwks);
    
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours(1);
    
    nlohmann::json header = {
        {"alg", "RS256"},
        {"typ", "JWT"},
        {"kid", kid_}
    };
    
    // Valid token with both iss and aud claims
    nlohmann::json payload = {
        {"sub", "user123"},
        {"iss", "https://test.example.com"},
        {"aud", "test-audience"},
        {"exp", std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count()}
    };
    
    std::string token = createToken(header, payload);
    
    EXPECT_NO_THROW({
        JWTClaims claims = validator_strict->parseAndValidate(token);
        EXPECT_EQ(claims.sub, "user123");
        EXPECT_EQ(claims.issuer, "https://test.example.com");
        EXPECT_EQ(claims.audience[0], "test-audience");
    });
}
