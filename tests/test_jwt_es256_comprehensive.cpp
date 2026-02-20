/**
 * @file test_jwt_es256_comprehensive.cpp
 * @brief Comprehensive tests for ES256 (ECDSA P-256 / SHA-256) JWT validation
 *
 * Tests cover:
 *  - Valid ES256 token is accepted
 *  - Expired ES256 token is rejected
 *  - Wrong signature is rejected
 *  - Tampered payload is rejected
 *  - Unsupported alg (RS256/none) with EC JWK is rejected
 *  - Wrong JWK type (RSA JWK for ES256 header) is rejected
 *  - Wrong curve (P-384 JWK) is rejected
 *  - Kid revocation works for ES256 tokens
 *  - Unsupported algorithm still rejected after adding ES256
 *  - ES256 alg:none attack is rejected
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <sstream>

#include "auth/jwt_validator.h"

using namespace themis::auth;

// ============================================================================
// Base64url helpers (copied from test_jwt_validator.cpp style)
// ============================================================================

namespace {

static std::string b64url(const std::vector<uint8_t>& in) {
    static const char* tbl =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64;
    b64.reserve(((in.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= in.size()) {
        uint32_t n = ((uint32_t)in[i] << 16) | ((uint32_t)in[i+1] << 8) | in[i+2];
        b64.push_back(tbl[(n >> 18) & 63]);
        b64.push_back(tbl[(n >> 12) & 63]);
        b64.push_back(tbl[(n >>  6) & 63]);
        b64.push_back(tbl[ n        & 63]);
        i += 3;
    }
    if (i + 1 == in.size()) {
        uint32_t n = (uint32_t)in[i] << 16;
        b64.push_back(tbl[(n >> 18) & 63]);
        b64.push_back(tbl[(n >> 12) & 63]);
        b64.push_back('='); b64.push_back('=');
    } else if (i + 2 == in.size()) {
        uint32_t n = ((uint32_t)in[i] << 16) | ((uint32_t)in[i+1] << 8);
        b64.push_back(tbl[(n >> 18) & 63]);
        b64.push_back(tbl[(n >> 12) & 63]);
        b64.push_back(tbl[(n >>  6) & 63]);
        b64.push_back('=');
    }
    for (char& c : b64) { if (c == '+') c = '-'; else if (c == '/') c = '_'; }
    while (!b64.empty() && b64.back() == '=') b64.pop_back();
    return b64;
}

// ============================================================================
// EC P-256 test key fixture
// ============================================================================

struct ECFixture {
    EC_KEY*  ec_key = nullptr;
    EVP_PKEY* pkey  = nullptr;

    ECFixture() {
        ec_key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        if (!ec_key) throw std::runtime_error("EC_KEY_new_by_curve_name failed");
        if (EC_KEY_generate_key(ec_key) != 1)
            throw std::runtime_error("EC_KEY_generate_key failed");
        pkey = EVP_PKEY_new();
        if (!pkey) throw std::runtime_error("EVP_PKEY_new failed");
        // EVP_PKEY_set1_EC_KEY increments ec_key's refcount; we keep our own
        // reference so we can call EC_KEY_get0_public_key() directly on ec_key.
        if (EVP_PKEY_set1_EC_KEY(pkey, ec_key) != 1)
            throw std::runtime_error("EVP_PKEY_set1_EC_KEY failed");
    }

    ~ECFixture() {
        // Free our own ec_key reference (EVP_PKEY holds a separate reference)
        if (ec_key) EC_KEY_free(ec_key);
        if (pkey) EVP_PKEY_free(pkey);
    }

    // Extract raw x,y coordinates (32 bytes each for P-256)
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> publicKeyCoords() const {
        const EC_GROUP* grp = EC_KEY_get0_group(ec_key);
        const EC_POINT* pt  = EC_KEY_get0_public_key(ec_key);
        BIGNUM* x = BN_new(); BIGNUM* y = BN_new();
        if (!x || !y) { BN_free(x); BN_free(y); throw std::runtime_error("BN_new failed"); }
        if (EC_POINT_get_affine_coordinates_GFp(grp, pt, x, y, nullptr) != 1) {
            BN_free(x); BN_free(y);
            throw std::runtime_error("EC_POINT_get_affine_coordinates_GFp failed");
        }
        std::vector<uint8_t> xb(32, 0), yb(32, 0);
        BN_bn2binpad(x, xb.data(), 32);
        BN_bn2binpad(y, yb.data(), 32);
        BN_free(x); BN_free(y);
        return {xb, yb};
    }
};

// Sign header_payload with ECDSA P-256, return base64url(r||s) (JWT raw encoding)
static std::string sign_ES256(EVP_PKEY* pkey, const std::string& header_payload) {
    EVP_MD_CTX* mctx = EVP_MD_CTX_new();
    if (!mctx) throw std::runtime_error("EVP_MD_CTX_new failed");

    size_t der_len = 0;
    EVP_DigestSignInit(mctx, nullptr, EVP_sha256(), nullptr, pkey);
    EVP_DigestSignUpdate(mctx, header_payload.data(), header_payload.size());
    EVP_DigestSignFinal(mctx, nullptr, &der_len);
    std::vector<uint8_t> der_sig(der_len);
    EVP_DigestSignFinal(mctx, der_sig.data(), &der_len);
    der_sig.resize(der_len);
    EVP_MD_CTX_free(mctx);

    // Parse DER-encoded ECDSA_SIG → extract r and s, build r||s (32+32 bytes)
    const unsigned char* der_ptr = der_sig.data();
    ECDSA_SIG* esig = d2i_ECDSA_SIG(nullptr, &der_ptr, (long)der_sig.size());
    if (!esig) throw std::runtime_error("d2i_ECDSA_SIG failed");

    const BIGNUM *r = nullptr, *s = nullptr;
    ECDSA_SIG_get0(esig, &r, &s);
    std::vector<uint8_t> rs(64, 0);
    BN_bn2binpad(r, rs.data(),      32);
    BN_bn2binpad(s, rs.data() + 32, 32);
    ECDSA_SIG_free(esig);

    return b64url(rs);
}

// Build JWKS with one EC P-256 key
static nlohmann::json make_ec_jwks(const ECFixture& fix,
                                   const std::string& kid = "ec-key-1",
                                   const std::string& crv = "P-256") {
    auto [x, y] = fix.publicKeyCoords();
    nlohmann::json jwk = {
        {"kty", "EC"},
        {"crv", crv},
        {"kid", kid},
        {"alg", "ES256"},
        {"use", "sig"},
        {"x",   b64url(x)},
        {"y",   b64url(y)},
    };
    return nlohmann::json{{"keys", nlohmann::json::array({jwk})}};
}

// Build JWT header.payload (unsigned)
static std::string build_es256_token(const std::string& kid,
                                     const nlohmann::json& payload) {
    nlohmann::json header = {{"alg","ES256"},{"typ","JWT"},{"kid",kid}};
    auto h_str = header.dump();
    auto p_str = payload.dump();
    return b64url(std::vector<uint8_t>(h_str.begin(), h_str.end())) + "." +
           b64url(std::vector<uint8_t>(p_str.begin(), p_str.end()));
}

// Build a valid signed ES256 token expiring in `exp_offset_sec` seconds from now
static std::string make_valid_es256_jwt(const ECFixture& fix,
                                        const std::string& kid,
                                        int exp_offset_sec = 300) {
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(
                   now.time_since_epoch()).count() + exp_offset_sec;
    nlohmann::json payload = {
        {"sub",   "alice"},
        {"email", "alice@example.com"},
        {"iss",   "issuerX"},
        {"aud",   "audX"},
        {"exp",   exp},
    };
    std::string up = build_es256_token(kid, payload);
    return up + "." + sign_ES256(fix.pkey, up);
}

} // anonymous namespace

// ============================================================================
// Test cases
// ============================================================================

class ES256ValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        JWTValidatorConfig cfg;
        cfg.jwks_url          = "";
        cfg.expected_issuer   = "issuerX";
        cfg.expected_audience = "audX";
        cfg.clock_skew        = std::chrono::seconds(60);
        validator_ = std::make_unique<JWTValidator>(cfg);
        validator_->setJWKSForTesting(make_ec_jwks(fix_));
    }

    ECFixture fix_;
    std::unique_ptr<JWTValidator> validator_;
};

// --- Happy path -----------------------------------------------------------

TEST_F(ES256ValidatorTest, ValidToken_Accepted) {
    auto token = make_valid_es256_jwt(fix_, "ec-key-1");
    EXPECT_NO_THROW({
        auto claims = validator_->parseAndValidate(token);
        EXPECT_EQ(claims.sub, "alice");
        EXPECT_EQ(claims.email, "alice@example.com");
    });
}

TEST_F(ES256ValidatorTest, ValidToken_ClaimsExtracted) {
    auto token = make_valid_es256_jwt(fix_, "ec-key-1");
    auto claims = validator_->parseAndValidate(token);
    EXPECT_EQ(claims.issuer, "issuerX");
    EXPECT_FALSE(claims.isExpired());
}

// --- Negative cases -------------------------------------------------------

TEST_F(ES256ValidatorTest, ExpiredToken_Rejected) {
    auto token = make_valid_es256_jwt(fix_, "ec-key-1", -10);  // expired 10 s ago
    // Use zero clock_skew so expiry check is strict
    JWTValidatorConfig cfg;
    cfg.expected_issuer   = "issuerX";
    cfg.expected_audience = "audX";
    cfg.clock_skew        = std::chrono::seconds(0);
    JWTValidator strict_val(cfg);
    strict_val.setJWKSForTesting(make_ec_jwks(fix_));
    EXPECT_THROW(strict_val.parseAndValidate(token), std::runtime_error);
}

TEST_F(ES256ValidatorTest, WrongSignature_Rejected) {
    auto up = build_es256_token("ec-key-1", {
        {"sub","u"}, {"iss","issuerX"}, {"aud","audX"},
        {"exp", (int64_t)(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() + 300)}
    });
    // Replace signature with garbage
    std::string bad_sig = b64url(std::vector<uint8_t>(64, 0xAA));
    EXPECT_THROW(validator_->parseAndValidate(up + "." + bad_sig), std::runtime_error);
}

TEST_F(ES256ValidatorTest, TamperedPayload_Rejected) {
    // Sign the original, then alter the payload
    ECFixture fix2;
    auto token = make_valid_es256_jwt(fix_, "ec-key-1");
    auto parts = [&]() -> std::vector<std::string> {
        std::vector<std::string> p;
        std::istringstream ss(token);
        std::string part;
        while (std::getline(ss, part, '.')) p.push_back(part);
        return p;
    }();
    ASSERT_EQ(parts.size(), 3u);

    // Tamper payload: build a new header.payload but keep old signature
    nlohmann::json evil = {{"sub","admin"}, {"iss","issuerX"}, {"aud","audX"},
                           {"exp", (int64_t)(9999999999LL)}};
    std::string evil_str = evil.dump();
    std::string tampered_payload = b64url(
        std::vector<uint8_t>(evil_str.begin(), evil_str.end()));
    std::string tampered = parts[0] + "." + tampered_payload + "." + parts[2];
    EXPECT_THROW(validator_->parseAndValidate(tampered), std::runtime_error);
}

TEST_F(ES256ValidatorTest, WrongKidInJWKS_Rejected) {
    // Token uses kid "ec-key-1" but JWKS only has "other-key"
    validator_->setJWKSForTesting(make_ec_jwks(fix_, "other-key"));
    auto token = make_valid_es256_jwt(fix_, "ec-key-1");
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(ES256ValidatorTest, RsaJwkForEs256Header_Rejected) {
    // Inject an RSA JWK but request ES256 – type mismatch
    nlohmann::json rsa_jwk = {
        {"kty","RSA"}, {"kid","ec-key-1"}, {"alg","RS256"}, {"use","sig"},
        {"n","AQAB"}, {"e","AQAB"}
    };
    validator_->setJWKSForTesting(nlohmann::json{{"keys", {rsa_jwk}}});
    auto token = make_valid_es256_jwt(fix_, "ec-key-1");
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(ES256ValidatorTest, WrongCurveInJwk_Rejected) {
    // JWK says P-384 but key is P-256 (crv mismatch)
    validator_->setJWKSForTesting(make_ec_jwks(fix_, "ec-key-1", "P-384"));
    auto token = make_valid_es256_jwt(fix_, "ec-key-1");
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(ES256ValidatorTest, RevokedKid_Rejected) {
    validator_->revokeKid("ec-key-1");
    auto token = make_valid_es256_jwt(fix_, "ec-key-1");
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(ES256ValidatorTest, UnsupportedAlgHS256_Rejected) {
    // Manually craft a token with alg: HS256 which is unsupported
    nlohmann::json header = {{"alg","HS256"},{"typ","JWT"},{"kid","ec-key-1"}};
    nlohmann::json payload = {
        {"sub","u"}, {"iss","issuerX"}, {"aud","audX"},
        {"exp", (int64_t)(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() + 300)}
    };
    auto h = b64url(std::vector<uint8_t>(header.dump().begin(), header.dump().end()));
    auto p = b64url(std::vector<uint8_t>(payload.dump().begin(), payload.dump().end()));
    std::string fake_sig = b64url(std::vector<uint8_t>(32, 0));
    EXPECT_THROW(validator_->parseAndValidate(h + "." + p + "." + fake_sig),
                 std::runtime_error);
}

TEST_F(ES256ValidatorTest, AlgNoneAttack_Rejected) {
    nlohmann::json header = {{"alg","none"},{"typ","JWT"}};
    nlohmann::json payload = {
        {"sub","admin"}, {"iss","issuerX"}, {"aud","audX"},
        {"exp", (int64_t)(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() + 300)}
    };
    auto h = b64url(std::vector<uint8_t>(header.dump().begin(), header.dump().end()));
    auto p = b64url(std::vector<uint8_t>(payload.dump().begin(), payload.dump().end()));
    EXPECT_THROW(validator_->parseAndValidate(h + "." + p + "."), std::runtime_error);
}

TEST_F(ES256ValidatorTest, DifferentECKey_SignatureRejected) {
    // Sign with a different EC key – should not verify against the JWKS key
    ECFixture other_key;
    auto token = make_valid_es256_jwt(other_key, "ec-key-1");  // signed by other_key
    // JWKS still contains fix_'s public key
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}
