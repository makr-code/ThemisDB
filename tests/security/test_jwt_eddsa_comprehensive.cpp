/**
 * @file test_jwt_eddsa_comprehensive.cpp
 * @brief Comprehensive tests for EdDSA (Ed25519) JWT validation
 *
 * Tests cover:
 *  - Valid EdDSA token is accepted
 *  - Expired EdDSA token is rejected
 *  - Wrong Ed25519 signature is rejected
 *  - Tampered payload is rejected
 *  - Wrong JWK (RSA/EC type instead of OKP) is rejected
 *  - Missing "crv" field in JWK is rejected
 *  - Wrong curve (Ed448 crv) in JWK is rejected
 *  - Key-id (kid) revocation works for EdDSA tokens
 *  - alg:none attack is rejected even with valid EdDSA JWK present
 *  - Malformed "x" field (wrong length) is rejected
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <sstream>

#include "auth/jwt_validator.h"

using namespace themis::auth;

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Base64url helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::string b64url(const std::vector<uint8_t>& in) {
    static const char* tbl =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64;
    b64.reserve(((in.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= in.size()) {
        uint32_t n = ((uint32_t)in[i]<<16) | ((uint32_t)in[i+1]<<8) | in[i+2];
        b64.push_back(tbl[(n>>18)&63]); b64.push_back(tbl[(n>>12)&63]);
        b64.push_back(tbl[(n>> 6)&63]); b64.push_back(tbl[ n     &63]);
        i += 3;
    }
    if (i + 1 == in.size()) {
        uint32_t n = (uint32_t)in[i] << 16;
        b64.push_back(tbl[(n>>18)&63]); b64.push_back(tbl[(n>>12)&63]);
        b64.push_back('='); b64.push_back('=');
    } else if (i + 2 == in.size()) {
        uint32_t n = ((uint32_t)in[i]<<16) | ((uint32_t)in[i+1]<<8);
        b64.push_back(tbl[(n>>18)&63]); b64.push_back(tbl[(n>>12)&63]);
        b64.push_back(tbl[(n>> 6)&63]); b64.push_back('=');
    }
    for (char& c : b64) { if (c=='+') c='-'; else if (c=='/') c='_'; }
    while (!b64.empty() && b64.back()=='=') {
      b64.pop_back();
    }
    return b64;
}

static std::string b64urlStr(const std::string& s) {
    return b64url(std::vector<uint8_t>(s.begin(), s.end()));
}

// ─────────────────────────────────────────────────────────────────────────────
// Ed25519 key fixture
// ─────────────────────────────────────────────────────────────────────────────

struct Ed25519Fixture {
    EVP_PKEY* priv_pkey = nullptr;   ///< private key for signing
    EVP_PKEY* pub_pkey  = nullptr;   ///< public key for JWK construction
    std::vector<uint8_t> pub_bytes;  ///< raw 32-byte public key

    Ed25519Fixture() {
        // Generate Ed25519 key pair
        EVP_PKEY_CTX* kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
        if (!kctx) {
          throw std::runtime_error("EVP_PKEY_CTX_new_id failed");
        }
        if (EVP_PKEY_keygen_init(kctx) != 1 ||
            EVP_PKEY_keygen(kctx, &priv_pkey) != 1) {
            EVP_PKEY_CTX_free(kctx);
            throw std::runtime_error("Ed25519 key generation failed");
        }
        EVP_PKEY_CTX_free(kctx);

        // Extract public key bytes
        size_t pub_len = 32;
        pub_bytes.resize(32);
        if (EVP_PKEY_get_raw_public_key(priv_pkey, pub_bytes.data(), &pub_len) != 1)
            throw std::runtime_error("EVP_PKEY_get_raw_public_key failed");
        pub_bytes.resize(pub_len);

        // Build a public-only EVP_PKEY for reference
        pub_pkey = EVP_PKEY_new_raw_public_key(
            EVP_PKEY_ED25519, nullptr, pub_bytes.data(), pub_bytes.size());
        if (!pub_pkey) {
          throw std::runtime_error("pub EVP_PKEY creation failed");
        }
    }

    ~Ed25519Fixture() {
        EVP_PKEY_free(priv_pkey);
        EVP_PKEY_free(pub_pkey);
    }

    /// Build an OKP JWK for this Ed25519 public key
    nlohmann::json buildJwk(const std::string& kid = "ed-test-kid") const {
        return {
            {"kty", "OKP"},
            {"crv", "Ed25519"},
            {"kid", kid},
            {"x",   b64url(pub_bytes)}
        };
    }

    /// Sign a string (header_payload) and return the raw 64-byte signature
    std::vector<uint8_t> sign(const std::string& msg) const {
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) {
          throw std::runtime_error("EVP_MD_CTX_new failed");
        }
        if (EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, priv_pkey) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("EVP_DigestSignInit failed");
        }
        size_t sig_len = 0;
        EVP_DigestSign(ctx, nullptr, &sig_len,
                       reinterpret_cast<const unsigned char*>(msg.data()),
                       msg.size());
        std::vector<uint8_t> sig(sig_len);
        if (EVP_DigestSign(ctx, sig.data(), &sig_len,
                           reinterpret_cast<const unsigned char*>(msg.data()),
                           msg.size()) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("EVP_DigestSign failed");
        }
        EVP_MD_CTX_free(ctx);
        sig.resize(sig_len);
        return sig;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// JWT construction helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::string buildToken(const Ed25519Fixture& fix,
                              const std::string& kid,
                              int exp_delta_seconds = 3600,
                              const std::string& alg = "EdDSA") {
    nlohmann::json hdr = {{"alg", alg}, {"typ", "JWT"}, {"kid", kid}};
    auto now = std::chrono::system_clock::now();
    auto iat = std::chrono::duration_cast<std::chrono::seconds>(
                   now.time_since_epoch()).count();
    nlohmann::json pay = {
        {"sub",   "user123"},
        {"iss",   "https://auth.example.com"},
        {"iat",   iat},
        {"exp",   iat + exp_delta_seconds}
    };
    std::string header_payload =
        b64urlStr(hdr.dump()) + "." + b64urlStr(pay.dump());
    auto sig = fix.sign(header_payload);
    return header_payload + "." + b64url(sig);
}

static nlohmann::json buildJwks(const nlohmann::json& jwk) {
    return {{"keys", nlohmann::json::array({jwk})}};
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(JWTEdDSA, ValidEdDSATokenIsAccepted) {
    Ed25519Fixture fix;
    JWTValidator validator("https://unused.example.com/jwks");
    validator.setJWKSForTesting(buildJwks(fix.buildJwk("kid1")));

    auto token = buildToken(fix, "kid1");
    EXPECT_NO_THROW({
        auto claims = validator.parseAndValidate(token);
        EXPECT_EQ(claims.sub, "user123");
    });
}

TEST(JWTEdDSA, ExpiredEdDSATokenIsRejected) {
    Ed25519Fixture fix;
    JWTValidator validator("https://unused.example.com/jwks");
    validator.setJWKSForTesting(buildJwks(fix.buildJwk("kid1")));

    auto token = buildToken(fix, "kid1", -10);  // expired 10 s ago
    EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error);
}

TEST(JWTEdDSA, WrongSignatureIsRejected) {
    Ed25519Fixture fix;
    JWTValidator validator("https://unused.example.com/jwks");
    validator.setJWKSForTesting(buildJwks(fix.buildJwk("kid1")));

    auto token = buildToken(fix, "kid1");
    // Corrupt the last byte of the signature part
    auto dot1 = token.rfind('.');
    token.back() ^= 'X';  // flip a character in the signature
    EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error);
}

TEST(JWTEdDSA, TamperedPayloadIsRejected) {
    Ed25519Fixture fix;
    JWTValidator validator("https://unused.example.com/jwks");
    validator.setJWKSForTesting(buildJwks(fix.buildJwk("kid1")));

    auto token = buildToken(fix, "kid1");
    // Replace the payload with a different base64url string
    auto dot1 = token.find('.');
    auto dot2 = token.find('.', dot1 + 1);
    std::string new_payload = b64urlStr(R"({"sub":"attacker","exp":9999999999})");
    token = token.substr(0, dot1 + 1) + new_payload + token.substr(dot2);
    EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error);
}

TEST(JWTEdDSA, RsaJwkForEdDSAHeaderIsRejected) {
    Ed25519Fixture fix;
    JWTValidator validator("https://unused.example.com/jwks");

    // Provide an RSA JWK instead of OKP
    nlohmann::json rsa_jwk = {
        {"kty", "RSA"}, {"kid", "kid1"},
        {"n", "0vx7agoebGcQSuuPiLJXZptN9nndrQmbXEps2aiAFbWhM78LhWx4cbbfAAt"
              "VT86zwu1RK7aPFFxuhDR1L6tSoc_BJECPebWKRXjBZCiFV4n3oknjhMstn64t"
              "Z_2W-5JsGY4Hc5n9yBXArwl93lqt7_RN5w6Cf0h4QyQ5v-65YGjQR0_FDW2Q"
              "vzqY368QQMicAtaSqzs8KJZgnYb9c7d0zgdAZHzu6qMQvRL5hajrn1n91Cbrg"},
        {"e", "AQAB"}
    };
    validator.setJWKSForTesting(buildJwks(rsa_jwk));

    auto token = buildToken(fix, "kid1");
    EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error);
}

TEST(JWTEdDSA, MissingCrvFieldIsRejected) {
    Ed25519Fixture fix;
    JWTValidator validator("https://unused.example.com/jwks");

    nlohmann::json bad_jwk = {
        {"kty", "OKP"}, {"kid", "kid1"},
        // "crv" deliberately omitted
        {"x", b64url(fix.pub_bytes)}
    };
    validator.setJWKSForTesting(buildJwks(bad_jwk));
    EXPECT_THROW(validator.parseAndValidate(buildToken(fix, "kid1")), std::runtime_error);
}

TEST(JWTEdDSA, WrongCurveInJwkIsRejected) {
    Ed25519Fixture fix;
    JWTValidator validator("https://unused.example.com/jwks");

    nlohmann::json bad_jwk = {
        {"kty", "OKP"}, {"crv", "Ed448"}, {"kid", "kid1"},
        {"x", b64url(fix.pub_bytes)}
    };
    validator.setJWKSForTesting(buildJwks(bad_jwk));
    EXPECT_THROW(validator.parseAndValidate(buildToken(fix, "kid1")), std::runtime_error);
}

TEST(JWTEdDSA, MalformedXFieldTooShortIsRejected) {
    Ed25519Fixture fix;
    JWTValidator validator("https://unused.example.com/jwks");

    // "x" only 16 bytes instead of 32
    std::vector<uint8_t> short_x(16, 0x42);
    nlohmann::json bad_jwk = {
        {"kty", "OKP"}, {"crv", "Ed25519"}, {"kid", "kid1"},
        {"x", b64url(short_x)}
    };
    validator.setJWKSForTesting(buildJwks(bad_jwk));
    EXPECT_THROW(validator.parseAndValidate(buildToken(fix, "kid1")), std::runtime_error);
}

TEST(JWTEdDSA, KidRevocationRejectsToken) {
    Ed25519Fixture fix;
    JWTValidator validator("https://unused.example.com/jwks");
    validator.setJWKSForTesting(buildJwks(fix.buildJwk("kid-revoke")));

    auto token = buildToken(fix, "kid-revoke");
    // Validate first – should succeed
    EXPECT_NO_THROW(validator.parseAndValidate(token));

    // Revoke the kid
    validator.revokeKid("kid-revoke");
    EXPECT_TRUE(validator.isKidRevoked("kid-revoke"));

    // Now the same token must be rejected
    EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error);
}

TEST(JWTEdDSA, AlgNoneAttackRejectedWithEdDSAJwk) {
    Ed25519Fixture fix;
    JWTValidator validator("https://unused.example.com/jwks");
    validator.setJWKSForTesting(buildJwks(fix.buildJwk("kid1")));

    // Craft an alg:none token – no signature part
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(
                   now.time_since_epoch()).count() + 3600;
    nlohmann::json hdr = {{"alg", "none"}, {"typ", "JWT"}, {"kid", "kid1"}};
    nlohmann::json pay = {{"sub", "attacker"}, {"exp", exp}};
    std::string hp = b64urlStr(hdr.dump()) + "." + b64urlStr(pay.dump());
    std::string none_token = hp + ".";
    EXPECT_THROW(validator.parseAndValidate(none_token), std::runtime_error);
}

TEST(JWTEdDSA, DifferentKeyPairSignatureIsRejected) {
    // Sign with one key but validate with a different public key
    Ed25519Fixture signer;
    Ed25519Fixture other;   // different key pair

    JWTValidator validator("https://unused.example.com/jwks");
    // Provide the *other* public key in the JWKS
    validator.setJWKSForTesting(buildJwks(other.buildJwk("kid1")));

    auto token = buildToken(signer, "kid1");
    EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error);
}

TEST(JWTEdDSA, MultipleKeysSameKidUsesFirst) {
    Ed25519Fixture fix;
    JWTValidator validator("https://unused.example.com/jwks");
    // Two keys, first is the correct one
    auto jwk1 = fix.buildJwk("kid1");
    Ed25519Fixture other;
    auto jwk2 = other.buildJwk("kid1");  // same kid, different key
    nlohmann::json jwks = {{"keys", {jwk1, jwk2}}};
    validator.setJWKSForTesting(jwks);

    auto token = buildToken(fix, "kid1");
    // Should succeed because findJwkForKid returns the first match
    EXPECT_NO_THROW(validator.parseAndValidate(token));
}
