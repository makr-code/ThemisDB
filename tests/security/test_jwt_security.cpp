#include <gtest/gtest.h>

#include "auth/jwt_validator.h"

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using themis::auth::JWTValidator;
using themis::auth::JWTValidatorConfig;
using Json = nlohmann::json;

std::string base64Url(std::string_view input) {
    std::string encoded(4 * ((input.size() + 2) / 3), '\0');
    const auto length = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(encoded.data()),
        reinterpret_cast<const unsigned char*>(input.data()),
        static_cast<int>(input.size()));
    if (length < 0) {
        throw std::runtime_error("Failed to base64-encode JWT test data");
    }

    encoded.resize(static_cast<std::size_t>(length));
    for (char& ch : encoded) {
        if (ch == '+') {
            ch = '-';
        } else if (ch == '/') {
            ch = '_';
        }
    }
    while (!encoded.empty() && encoded.back() == '=') {
        encoded.pop_back();
    }
    return encoded;
}

class RSAKey {
public:
    RSAKey() {
        using Context = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;
        Context context(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr), EVP_PKEY_CTX_free);
        if (!context || EVP_PKEY_keygen_init(context.get()) <= 0 ||
            EVP_PKEY_CTX_set_rsa_keygen_bits(context.get(), 2048) <= 0) {
            throw std::runtime_error("Failed to initialize JWT test key generation");
        }

        EVP_PKEY* generated = nullptr;
        if (EVP_PKEY_keygen(context.get(), &generated) <= 0) {
            throw std::runtime_error("Failed to generate JWT test key");
        }
        key_.reset(generated);
    }

    [[nodiscard]] EVP_PKEY* get() const noexcept {
        return key_.get();
    }

private:
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key_{nullptr, EVP_PKEY_free};
};

std::vector<unsigned char> bigNumBytes(const BIGNUM* value) {
    std::vector<unsigned char> bytes(static_cast<std::size_t>(BN_num_bytes(value)));
    if (BN_bn2bin(value, bytes.data()) <= 0) {
        throw std::runtime_error("Failed to encode JWT test key");
    }
    return bytes;
}

Json makeJwk(EVP_PKEY* key, const std::string& kid) {
    using RSAHandle = std::unique_ptr<RSA, decltype(&RSA_free)>;
    RSAHandle rsa(EVP_PKEY_get1_RSA(key), RSA_free);
    if (!rsa) {
        throw std::runtime_error("Failed to read JWT test RSA key");
    }

    const BIGNUM* modulus_raw = nullptr;
    const BIGNUM* exponent_raw = nullptr;
    RSA_get0_key(rsa.get(), &modulus_raw, &exponent_raw, nullptr);
    if (modulus_raw == nullptr || exponent_raw == nullptr) {
        throw std::runtime_error("Failed to read JWT test RSA key");
    }
    const auto modulus_bytes = bigNumBytes(modulus_raw);
    const auto exponent_bytes = bigNumBytes(exponent_raw);
    const auto encode = [](const std::vector<unsigned char>& bytes) {
        return base64Url(std::string_view(
            reinterpret_cast<const char*>(bytes.data()), bytes.size()));
    };

    return {
        {"kty", "RSA"},
        {"kid", kid},
        {"use", "sig"},
        {"alg", "RS256"},
        {"n", encode(modulus_bytes)},
        {"e", encode(exponent_bytes)}
    };
}

Json makeJwks(EVP_PKEY* key, const std::string& kid) {
    return Json{{"keys", Json::array({makeJwk(key, kid)})}};
}

std::string signJwt(EVP_PKEY* key, const Json& header, const Json& payload) {
    const std::string signing_input = base64Url(header.dump()) + "." + base64Url(payload.dump());
    using DigestContext = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
    DigestContext context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!context || EVP_DigestSignInit(context.get(), nullptr, EVP_sha256(), nullptr, key) <= 0 ||
        EVP_DigestSignUpdate(context.get(), signing_input.data(), signing_input.size()) <= 0) {
        throw std::runtime_error("Failed to initialize JWT test signature");
    }

    std::size_t signature_size = 0;
    if (EVP_DigestSignFinal(context.get(), nullptr, &signature_size) <= 0) {
        throw std::runtime_error("Failed to size JWT test signature");
    }
    std::vector<unsigned char> signature(signature_size);
    if (EVP_DigestSignFinal(context.get(), signature.data(), &signature_size) <= 0) {
        throw std::runtime_error("Failed to create JWT test signature");
    }
    const auto encoded_signature = base64Url(std::string_view(
        reinterpret_cast<const char*>(signature.data()), signature_size));
    return signing_input + "." + encoded_signature;
}

std::string makeUnverifiedJwt(const Json& header, const Json& payload) {
    return base64Url(header.dump()) + "." + base64Url(payload.dump()) + "." + base64Url("attacker");
}

std::int64_t unixSeconds(std::chrono::system_clock::time_point time) {
    return std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count();
}

class JWTSecurityTest : public ::testing::Test {
protected:
    static RSAKey& firstKey() {
        static RSAKey key;
        return key;
    }

    static RSAKey& rotatedKey() {
        static RSAKey key;
        return key;
    }

    static RSAKey& untrustedKey() {
        static RSAKey key;
        return key;
    }

    void SetUp() override {
        config_.jwks_url = "https://issuer.example.test/jwks";
        config_.expected_issuer = "https://issuer.example.test";
        config_.expected_audience = "themis-api";
        config_.clock_skew = std::chrono::milliseconds::zero();

        validator_ = std::make_unique<JWTValidator>(config_);
        validator_->setJWKSForTesting(makeJwks(firstKey().get(), "trusted-key"));
    }

    Json payload(std::int64_t expiration = unixSeconds(std::chrono::system_clock::now()) + 3600) const {
        return {
            {"sub", "test-user"},
            {"iss", *config_.expected_issuer},
            {"aud", *config_.expected_audience},
            {"exp", expiration}
        };
    }

    std::string token(EVP_PKEY* key, const Json& claims, const std::string& kid = "trusted-key") const {
        return signJwt(key, Json{{"alg", "RS256"}, {"typ", "JWT"}, {"kid", kid}}, claims);
    }

    JWTValidatorConfig config_;
    std::unique_ptr<JWTValidator> validator_;
};

TEST_F(JWTSecurityTest, RejectsNoneAlgorithm) {
    const auto forged = makeUnverifiedJwt(Json{{"alg", "none"}, {"kid", "trusted-key"}}, payload());
    EXPECT_THROW(validator_->parseAndValidate(forged), std::runtime_error);
}

TEST_F(JWTSecurityTest, RejectsMalformedTokens) {
    for (const auto& malformed : {"", "not.a.jwt", "a.b.c.d"}) {
        EXPECT_THROW(validator_->parseAndValidate(malformed), std::runtime_error);
    }
}

TEST_F(JWTSecurityTest, RejectsHmacAlgorithmConfusionAgainstRsaJwks) {
    const auto forged = makeUnverifiedJwt(Json{{"alg", "HS256"}, {"kid", "trusted-key"}}, payload());
    EXPECT_THROW(validator_->parseAndValidate(forged), std::runtime_error);
}

TEST_F(JWTSecurityTest, RejectsExpiredToken) {
    const auto expired = payload(unixSeconds(std::chrono::system_clock::now()) - 3600);
    EXPECT_THROW(validator_->parseAndValidate(token(firstKey().get(), expired)), std::runtime_error);
}

TEST_F(JWTSecurityTest, RejectsWrongIssuer) {
    auto wrong_issuer = payload();
    wrong_issuer["iss"] = "https://attacker.example.test";
    EXPECT_THROW(validator_->parseAndValidate(token(firstKey().get(), wrong_issuer)), std::runtime_error);
}

TEST_F(JWTSecurityTest, RejectsTokenSignedByUntrustedKey) {
    EXPECT_THROW(validator_->parseAndValidate(token(untrustedKey().get(), payload())), std::runtime_error);
}

TEST_F(JWTSecurityTest, RejectsTamperedSignature) {
    auto tampered = token(firstKey().get(), payload());
    const auto signature_start = tampered.rfind('.');
    ASSERT_NE(signature_start, std::string::npos);
    auto& first_signature_character = tampered[signature_start + 1];
    first_signature_character = first_signature_character == 'A' ? 'B' : 'A';

    EXPECT_THROW(validator_->parseAndValidate(tampered), std::runtime_error);
}

TEST_F(JWTSecurityTest, RejectsOldKeyAndAcceptsRotatedKey) {
    const auto old_token = token(firstKey().get(), payload(), "old-key");
    validator_->setJWKSForTesting(makeJwks(firstKey().get(), "old-key"));
    ASSERT_NO_THROW(validator_->parseAndValidate(old_token));

    validator_->setJWKSForTesting(makeJwks(rotatedKey().get(), "new-key"));
    validator_->revokeKid("old-key");

    EXPECT_THROW(validator_->parseAndValidate(old_token), std::runtime_error);
    const auto new_token = token(rotatedKey().get(), payload(), "new-key");
    EXPECT_NO_THROW(validator_->parseAndValidate(new_token));
}

TEST_F(JWTSecurityTest, AcceptsTokenMatchingConfiguredIssuerAudienceAndKey) {
    const auto valid = token(firstKey().get(), payload());
    const auto claims = validator_->parseAndValidate(valid);
    EXPECT_EQ(claims.sub, "test-user");
    EXPECT_EQ(claims.issuer, *config_.expected_issuer);
}

TEST(JWTSecurityPolicyTest, RequiresExpectedIssuerAndAudienceByDefault) {
    JWTValidatorConfig missing_issuer;
    missing_issuer.jwks_url = "https://issuer.example.test/jwks";
    missing_issuer.expected_audience = "themis-api";
    EXPECT_THROW((void)JWTValidator{missing_issuer}, std::runtime_error);

    JWTValidatorConfig missing_audience;
    missing_audience.jwks_url = "https://issuer.example.test/jwks";
    missing_audience.expected_issuer = "https://issuer.example.test";
    EXPECT_THROW((void)JWTValidator{missing_audience}, std::runtime_error);
}

} // namespace
