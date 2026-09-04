/**
 * @file test_jwt_ec_curves_comprehensive.cpp
 * @brief Comprehensive tests for ES384 (P-384/SHA-384) and ES512 (P-521/SHA-512)
 *        JWT algorithm support, plus RS384/RS512 RSA variant coverage.
 *
 * Tests cover:
 *  - Valid ES384 token is accepted
 *  - Valid ES512 token is accepted
 *  - Expired ES384/ES512 tokens are rejected
 *  - Wrong signature is rejected for ES384 and ES512
 *  - Tampered payload is rejected
 *  - Curve/algorithm mismatch is rejected (e.g. ES384 header with P-256 JWK)
 *  - Kid revocation works for ES384 and ES512 tokens
 *  - RS384 and RS512 tokens are accepted
 *  - Cross-algorithm attack: ES256 header with ES384 JWK is rejected
 *  - alg:none attack is rejected for EC curves
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <sstream>
#include <stdexcept>

#include "auth/jwt_validator.h"

using namespace themis::auth;

// ============================================================================
// Base64url helpers
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
    while (!b64.empty() && b64.back() == '=') {
      b64.pop_back();
    }
    return b64;
}

// ============================================================================
// Generic EC test key fixture parameterised by curve NID and coordinate size
// ============================================================================

struct ECCurveFixture {
    int          nid = 0;         ///< e.g. NID_secp384r1
    size_t       coord_size;  ///< bytes per coordinate (48 for P-384, 66 for P-521)
    std::string  crv;         ///< JWK "crv" string ("P-384" / "P-521")
    std::string  alg;         ///< JWT "alg" string ("ES384" / "ES512")

    EC_KEY*   ec_key = nullptr;
    EVP_PKEY* pkey   = nullptr;

    ECCurveFixture(int nid_, size_t coord_size_, const std::string& crv_,
                   const std::string& alg_)
        : nid(nid_), coord_size(coord_size_), crv(crv_), alg(alg_)
    {
        ec_key = EC_KEY_new_by_curve_name(nid);
        if (!ec_key) {
          throw std::runtime_error("EC_KEY_new_by_curve_name failed");
        }
        if (EC_KEY_generate_key(ec_key) != 1)
            throw std::runtime_error("EC_KEY_generate_key failed");
        pkey = EVP_PKEY_new();
        if (!pkey) {
          throw std::runtime_error("EVP_PKEY_new failed");
        }
        if (EVP_PKEY_set1_EC_KEY(pkey, ec_key) != 1)
            throw std::runtime_error("EVP_PKEY_set1_EC_KEY failed");
    }

    ~ECCurveFixture() {
        if (ec_key) {
          EC_KEY_free(ec_key);
        }
        if (pkey) {
          EVP_PKEY_free(pkey);
        }
    }

    // Extract raw x,y coordinates (coord_size bytes each)
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> publicKeyCoords() const {
        const EC_GROUP* grp = EC_KEY_get0_group(ec_key);
        const EC_POINT* pt  = EC_KEY_get0_public_key(ec_key);
        BIGNUM* x = BN_new(); BIGNUM* y = BN_new();
        if (!x || !y) { BN_free(x); BN_free(y); throw std::runtime_error("BN_new failed"); }
        if (EC_POINT_get_affine_coordinates_GFp(grp, pt, x, y, nullptr) != 1) {
            BN_free(x); BN_free(y);
            throw std::runtime_error("EC_POINT_get_affine_coordinates_GFp failed");
        }
        std::vector<uint8_t> xb(coord_size, 0), yb(coord_size, 0);
        BN_bn2binpad(x, xb.data(), (int)coord_size);
        BN_bn2binpad(y, yb.data(), (int)coord_size);
        BN_free(x); BN_free(y);
        return {xb, yb};
    }
};

// Sign header_payload with ECDSA, return base64url(r||s) (JWT raw encoding)
// md: e.g. EVP_sha384() or EVP_sha512()
static std::string sign_EC(EVP_PKEY* pkey, const EVP_MD* md,
                            const std::string& header_payload, size_t coord_size)
{
    EVP_MD_CTX* mctx = EVP_MD_CTX_new();
    if (!mctx) {
      throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    size_t der_len = 0;
    EVP_DigestSignInit(mctx, nullptr, md, nullptr, pkey);
    EVP_DigestSignUpdate(mctx, header_payload.data(), header_payload.size());
    EVP_DigestSignFinal(mctx, nullptr, &der_len);
    std::vector<uint8_t> der_sig(der_len);
    EVP_DigestSignFinal(mctx, der_sig.data(), &der_len);
    der_sig.resize(der_len);
    EVP_MD_CTX_free(mctx);

    // Parse DER-encoded ECDSA_SIG → extract r and s, build r||s
    const unsigned char* der_ptr = der_sig.data();
    ECDSA_SIG* esig = d2i_ECDSA_SIG(nullptr, &der_ptr, (long)der_sig.size());
    if (!esig) {
      throw std::runtime_error("d2i_ECDSA_SIG failed");
    }

    const BIGNUM *r = nullptr, *s = nullptr;
    ECDSA_SIG_get0(esig, &r, &s);
    std::vector<uint8_t> rs(coord_size * 2, 0);
    BN_bn2binpad(r, rs.data(),              (int)coord_size);
    BN_bn2binpad(s, rs.data() + coord_size, (int)coord_size);
    ECDSA_SIG_free(esig);

    return b64url(rs);
}

// Build JWKS with one EC key
static nlohmann::json make_ec_jwks(const ECCurveFixture& fix,
                                   const std::string& kid = "ec-key-1",
                                   const std::string& override_crv = "")
{
    auto [x, y] = fix.publicKeyCoords();
    const std::string crv = override_crv.empty() ? fix.crv : override_crv;
    nlohmann::json jwk = {
        {"kty", "EC"},
        {"crv", crv},
        {"kid", kid},
        {"alg", fix.alg},
        {"use", "sig"},
        {"x",   b64url(x)},
        {"y",   b64url(y)},
    };
    return nlohmann::json{{"keys", nlohmann::json::array({jwk})}};
}

// Build JWT header.payload (unsigned)
static std::string build_token_header_payload(const std::string& alg,
                                              const std::string& kid,
                                              const nlohmann::json& payload)
{
    nlohmann::json header = {{"alg", alg}, {"typ", "JWT"}, {"kid", kid}};
    auto h_str = header.dump();
    auto p_str = payload.dump();
    return b64url(std::vector<uint8_t>(h_str.begin(), h_str.end())) + "." +
           b64url(std::vector<uint8_t>(p_str.begin(), p_str.end()));
}

// Build a valid signed EC token expiring in exp_offset_sec seconds from now
static std::string make_valid_ec_jwt(const ECCurveFixture& fix,
                                     const EVP_MD* md,
                                     const std::string& kid,
                                     int exp_offset_sec = 300)
{
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
    std::string up = build_token_header_payload(fix.alg, kid, payload);
    return up + "." + sign_EC(fix.pkey, md, up, fix.coord_size);
}

// ============================================================================
// RSA fixture for RS384/RS512 tests
// ============================================================================

struct RSAFixture {
    EVP_PKEY* pkey   = nullptr;
    RSA*      rsa_pub = nullptr;

    explicit RSAFixture(int bits = 2048) {
        // Generate RSA key
        pkey = EVP_PKEY_new();
        if (!pkey) {
          throw std::runtime_error("EVP_PKEY_new failed");
        }
        RSA* rsa = RSA_generate_key(bits, RSA_F4, nullptr, nullptr);
        if (!rsa) {
          throw std::runtime_error("RSA_generate_key failed");
        }
        if (EVP_PKEY_assign_RSA(pkey, rsa) != 1) {
            RSA_free(rsa);
            throw std::runtime_error("EVP_PKEY_assign_RSA failed");
        }
        // pkey owns rsa now
        rsa_pub = RSA_new();
        // Copy just public params
        const BIGNUM* n = nullptr; const BIGNUM* e = nullptr;
        RSA_get0_key(rsa, &n, &e, nullptr);
        BIGNUM* nc = BN_dup(n); BIGNUM* ec = BN_dup(e);
        RSA_set0_key(rsa_pub, nc, ec, nullptr);
    }

    ~RSAFixture() {
        if (pkey) {
          EVP_PKEY_free(pkey);
        }
        if (rsa_pub) {
          RSA_free(rsa_pub);
        }
    }

    // Return base64url-encoded n and e for JWK
    std::pair<std::string, std::string> publicComponents() const {
        const BIGNUM* n = nullptr; const BIGNUM* e = nullptr;
        RSA_get0_key(rsa_pub, &n, &e, nullptr);
        std::vector<uint8_t> nb(BN_num_bytes(n)), eb(BN_num_bytes(e));
        BN_bn2bin(n, nb.data());
        BN_bn2bin(e, eb.data());
        return {b64url(nb), b64url(eb)};
    }
};

static nlohmann::json make_rsa_jwks(const RSAFixture& fix,
                                    const std::string& alg,
                                    const std::string& kid = "rsa-key-1")
{
    auto [n_b64, e_b64] = fix.publicComponents();
    nlohmann::json jwk = {
        {"kty", "RSA"},
        {"kid", kid},
        {"alg", alg},
        {"use", "sig"},
        {"n",   n_b64},
        {"e",   e_b64},
    };
    return nlohmann::json{{"keys", nlohmann::json::array({jwk})}};
}

static std::string sign_RSA(EVP_PKEY* pkey, const EVP_MD* md,
                             const std::string& header_payload)
{
    EVP_MD_CTX* mctx = EVP_MD_CTX_new();
    if (!mctx) {
      throw std::runtime_error("EVP_MD_CTX_new failed");
    }
    size_t sig_len = 0;
    EVP_DigestSignInit(mctx, nullptr, md, nullptr, pkey);
    EVP_DigestSignUpdate(mctx, header_payload.data(), header_payload.size());
    EVP_DigestSignFinal(mctx, nullptr, &sig_len);
    std::vector<uint8_t> sig(sig_len);
    EVP_DigestSignFinal(mctx, sig.data(), &sig_len);
    sig.resize(sig_len);
    EVP_MD_CTX_free(mctx);
    return b64url(sig);
}

static std::string make_valid_rsa_jwt(const RSAFixture& fix,
                                      const EVP_MD* md,
                                      const std::string& alg,
                                      const std::string& kid,
                                      int exp_offset_sec = 300)
{
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(
                   now.time_since_epoch()).count() + exp_offset_sec;
    nlohmann::json payload = {
        {"sub",   "bob"},
        {"iss",   "issuerX"},
        {"aud",   "audX"},
        {"exp",   exp},
    };
    std::string up = build_token_header_payload(alg, kid, payload);
    return up + "." + sign_RSA(fix.pkey, md, up);
}

} // anonymous namespace

// ============================================================================
// ES384 Tests
// ============================================================================

class ES384ValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        fix_    = std::make_unique<ECCurveFixture>(NID_secp384r1, 48, "P-384", "ES384");
        JWTValidatorConfig cfg;
        cfg.jwks_url          = "";
        cfg.expected_issuer   = "issuerX";
        cfg.expected_audience = "audX";
        cfg.clock_skew        = std::chrono::seconds(60);
        validator_ = std::make_unique<JWTValidator>(cfg);
        validator_->setJWKSForTesting(make_ec_jwks(*fix_));
    }

    std::unique_ptr<ECCurveFixture> fix_;
    std::unique_ptr<JWTValidator>   validator_;
};

TEST_F(ES384ValidatorTest, ValidToken_Accepted) {
    auto token = make_valid_ec_jwt(*fix_, EVP_sha384(), "ec-key-1");
    EXPECT_NO_THROW({
        auto claims = validator_->parseAndValidate(token);
        EXPECT_EQ(claims.sub, "alice");
        EXPECT_EQ(claims.email, "alice@example.com");
        EXPECT_FALSE(claims.isExpired());
    });
}

TEST_F(ES384ValidatorTest, ValidToken_ClaimsExtracted) {
    auto token = make_valid_ec_jwt(*fix_, EVP_sha384(), "ec-key-1");
    auto claims = validator_->parseAndValidate(token);
    EXPECT_EQ(claims.issuer, "issuerX");
    EXPECT_FALSE(claims.isExpired());
}

TEST_F(ES384ValidatorTest, ExpiredToken_Rejected) {
    auto token = make_valid_ec_jwt(*fix_, EVP_sha384(), "ec-key-1", -10);
    JWTValidatorConfig cfg;
    cfg.expected_issuer   = "issuerX";
    cfg.expected_audience = "audX";
    cfg.clock_skew        = std::chrono::seconds(0);
    JWTValidator strict_val(cfg);
    strict_val.setJWKSForTesting(make_ec_jwks(*fix_));
    EXPECT_THROW(strict_val.parseAndValidate(token), std::runtime_error);
}

TEST_F(ES384ValidatorTest, WrongSignature_Rejected) {
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(
                   now.time_since_epoch()).count() + 300;
    nlohmann::json payload = {{"sub","u"}, {"iss","issuerX"}, {"aud","audX"}, {"exp", exp}};
    auto up = build_token_header_payload("ES384", "ec-key-1", payload);
    // Replace signature with all-zero bytes (wrong signature)
    std::string bad_sig = b64url(std::vector<uint8_t>(96, 0xBB));
    EXPECT_THROW(validator_->parseAndValidate(up + "." + bad_sig), std::runtime_error);
}

TEST_F(ES384ValidatorTest, TamperedPayload_Rejected) {
    auto token = make_valid_ec_jwt(*fix_, EVP_sha384(), "ec-key-1");
    // Split token
    std::vector<std::string> parts;
    std::istringstream ss(token);
    std::string p;
    while (std::getline(ss, p, '.')) {
      parts.push_back(p);
    }
    ASSERT_EQ(parts.size(), 3u);
    // Tamper with payload but keep original signature
    nlohmann::json evil = {{"sub","admin"}, {"iss","issuerX"}, {"aud","audX"}, {"exp", (int64_t)9999999999LL}};
    std::string evil_str = evil.dump();
    std::string tampered_payload = b64url(std::vector<uint8_t>(evil_str.begin(), evil_str.end()));
    EXPECT_THROW(validator_->parseAndValidate(parts[0] + "." + tampered_payload + "." + parts[2]),
                 std::runtime_error);
}

TEST_F(ES384ValidatorTest, WrongKidInJWKS_Rejected) {
    validator_->setJWKSForTesting(make_ec_jwks(*fix_, "other-key"));
    auto token = make_valid_ec_jwt(*fix_, EVP_sha384(), "ec-key-1");
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(ES384ValidatorTest, RevokedKid_Rejected) {
    validator_->revokeKid("ec-key-1");
    auto token = make_valid_ec_jwt(*fix_, EVP_sha384(), "ec-key-1");
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(ES384ValidatorTest, CrossCurveAttack_ES384HeaderWithP256JWK_Rejected) {
    // Build a P-256 JWK with kid="ec-key-1" but alg header says ES384 — curve mismatch
    ECCurveFixture p256(NID_X9_62_prime256v1, 32, "P-256", "ES256");
    nlohmann::json jwk = {
        {"kty", "EC"}, {"crv", "P-256"}, {"kid", "ec-key-1"},
        {"alg", "ES256"}, {"use", "sig"},
    };
    auto [x, y] = p256.publicKeyCoords();
    jwk["x"] = b64url(x); jwk["y"] = b64url(y);
    validator_->setJWKSForTesting(nlohmann::json{{"keys", {jwk}}});
    auto token = make_valid_ec_jwt(*fix_, EVP_sha384(), "ec-key-1");
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(ES384ValidatorTest, DifferentECKey_SignatureRejected) {
    ECCurveFixture other_key(NID_secp384r1, 48, "P-384", "ES384");
    auto token = make_valid_ec_jwt(other_key, EVP_sha384(), "ec-key-1");
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(ES384ValidatorTest, AlgNoneAttack_Rejected) {
    nlohmann::json header  = {{"alg","none"},{"typ","JWT"}};
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(
                   now.time_since_epoch()).count() + 300;
    nlohmann::json payload = {{"sub","admin"}, {"iss","issuerX"}, {"aud","audX"}, {"exp", exp}};

    // Materialize dumps first; using begin/end on different temporaries is undefined behavior.
    const std::string header_str = header.dump();
    const std::string payload_str = payload.dump();
    auto h = b64url(std::vector<uint8_t>(header_str.begin(), header_str.end()));
    auto p = b64url(std::vector<uint8_t>(payload_str.begin(), payload_str.end()));
    EXPECT_THROW(validator_->parseAndValidate(h + "." + p + "."), std::runtime_error);
}

// ============================================================================
// ES512 Tests
// ============================================================================

class ES512ValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        fix_    = std::make_unique<ECCurveFixture>(NID_secp521r1, 66, "P-521", "ES512");
        JWTValidatorConfig cfg;
        cfg.jwks_url          = "";
        cfg.expected_issuer   = "issuerX";
        cfg.expected_audience = "audX";
        cfg.clock_skew        = std::chrono::seconds(60);
        validator_ = std::make_unique<JWTValidator>(cfg);
        validator_->setJWKSForTesting(make_ec_jwks(*fix_));
    }

    std::unique_ptr<ECCurveFixture> fix_;
    std::unique_ptr<JWTValidator>   validator_;
};

TEST_F(ES512ValidatorTest, ValidToken_Accepted) {
    auto token = make_valid_ec_jwt(*fix_, EVP_sha512(), "ec-key-1");
    EXPECT_NO_THROW({
        auto claims = validator_->parseAndValidate(token);
        EXPECT_EQ(claims.sub, "alice");
        EXPECT_FALSE(claims.isExpired());
    });
}

TEST_F(ES512ValidatorTest, ValidToken_ClaimsExtracted) {
    auto token = make_valid_ec_jwt(*fix_, EVP_sha512(), "ec-key-1");
    auto claims = validator_->parseAndValidate(token);
    EXPECT_EQ(claims.issuer, "issuerX");
}

TEST_F(ES512ValidatorTest, ExpiredToken_Rejected) {
    auto token = make_valid_ec_jwt(*fix_, EVP_sha512(), "ec-key-1", -10);
    JWTValidatorConfig cfg;
    cfg.expected_issuer   = "issuerX";
    cfg.expected_audience = "audX";
    cfg.clock_skew        = std::chrono::seconds(0);
    JWTValidator strict_val(cfg);
    strict_val.setJWKSForTesting(make_ec_jwks(*fix_));
    EXPECT_THROW(strict_val.parseAndValidate(token), std::runtime_error);
}

TEST_F(ES512ValidatorTest, WrongSignature_Rejected) {
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(
                   now.time_since_epoch()).count() + 300;
    nlohmann::json payload = {{"sub","u"}, {"iss","issuerX"}, {"aud","audX"}, {"exp", exp}};
    auto up = build_token_header_payload("ES512", "ec-key-1", payload);
    std::string bad_sig = b64url(std::vector<uint8_t>(132, 0xCC));
    EXPECT_THROW(validator_->parseAndValidate(up + "." + bad_sig), std::runtime_error);
}

TEST_F(ES512ValidatorTest, TamperedPayload_Rejected) {
    auto token = make_valid_ec_jwt(*fix_, EVP_sha512(), "ec-key-1");
    std::vector<std::string> parts;
    std::istringstream ss(token); std::string p;
    while (std::getline(ss, p, '.')) {
      parts.push_back(p);
    }
    ASSERT_EQ(parts.size(), 3u);
    nlohmann::json evil = {{"sub","admin"}, {"iss","issuerX"}, {"aud","audX"}, {"exp", (int64_t)9999999999LL}};
    std::string evil_str = evil.dump();
    std::string tampered_payload = b64url(std::vector<uint8_t>(evil_str.begin(), evil_str.end()));
    EXPECT_THROW(validator_->parseAndValidate(parts[0] + "." + tampered_payload + "." + parts[2]),
                 std::runtime_error);
}

TEST_F(ES512ValidatorTest, RevokedKid_Rejected) {
    validator_->revokeKid("ec-key-1");
    auto token = make_valid_ec_jwt(*fix_, EVP_sha512(), "ec-key-1");
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(ES512ValidatorTest, DifferentECKey_SignatureRejected) {
    ECCurveFixture other_key(NID_secp521r1, 66, "P-521", "ES512");
    auto token = make_valid_ec_jwt(other_key, EVP_sha512(), "ec-key-1");
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(ES512ValidatorTest, CrossAlgorithmAttack_ES512HeaderWithP384JWK_Rejected) {
    // A JWK with crv=P-384 in JWKS but the token header claims ES512 (P-521).
    ECCurveFixture p384(NID_secp384r1, 48, "P-384", "ES384");
    validator_->setJWKSForTesting(make_ec_jwks(p384, "ec-key-1", "P-384"));
    auto token = make_valid_ec_jwt(*fix_, EVP_sha512(), "ec-key-1");
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

// ============================================================================
// RS384 Tests
// ============================================================================

class RS384ValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        fix_ = std::make_unique<RSAFixture>(2048);
        JWTValidatorConfig cfg;
        cfg.jwks_url          = "";
        cfg.expected_issuer   = "issuerX";
        cfg.expected_audience = "audX";
        cfg.clock_skew        = std::chrono::seconds(60);
        validator_ = std::make_unique<JWTValidator>(cfg);
        validator_->setJWKSForTesting(make_rsa_jwks(*fix_, "RS384"));
    }

    std::unique_ptr<RSAFixture>   fix_;
    std::unique_ptr<JWTValidator> validator_;
};

TEST_F(RS384ValidatorTest, ValidToken_Accepted) {
    auto token = make_valid_rsa_jwt(*fix_, EVP_sha384(), "RS384", "rsa-key-1");
    EXPECT_NO_THROW({
        auto claims = validator_->parseAndValidate(token);
        EXPECT_EQ(claims.sub, "bob");
        EXPECT_FALSE(claims.isExpired());
    });
}

TEST_F(RS384ValidatorTest, WrongSignature_Rejected) {
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(
                   now.time_since_epoch()).count() + 300;
    nlohmann::json payload = {{"sub","u"}, {"iss","issuerX"}, {"aud","audX"}, {"exp", exp}};
    auto up = build_token_header_payload("RS384", "rsa-key-1", payload);
    std::string bad_sig = b64url(std::vector<uint8_t>(256, 0xDD));
    EXPECT_THROW(validator_->parseAndValidate(up + "." + bad_sig), std::runtime_error);
}

TEST_F(RS384ValidatorTest, ExpiredToken_Rejected) {
    auto token = make_valid_rsa_jwt(*fix_, EVP_sha384(), "RS384", "rsa-key-1", -10);
    JWTValidatorConfig cfg;
    cfg.expected_issuer   = "issuerX";
    cfg.expected_audience = "audX";
    cfg.clock_skew        = std::chrono::seconds(0);
    JWTValidator strict_val(cfg);
    strict_val.setJWKSForTesting(make_rsa_jwks(*fix_, "RS384"));
    EXPECT_THROW(strict_val.parseAndValidate(token), std::runtime_error);
}

// ============================================================================
// RS512 Tests
// ============================================================================

class RS512ValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        fix_ = std::make_unique<RSAFixture>(2048);
        JWTValidatorConfig cfg;
        cfg.jwks_url          = "";
        cfg.expected_issuer   = "issuerX";
        cfg.expected_audience = "audX";
        cfg.clock_skew        = std::chrono::seconds(60);
        validator_ = std::make_unique<JWTValidator>(cfg);
        validator_->setJWKSForTesting(make_rsa_jwks(*fix_, "RS512"));
    }

    std::unique_ptr<RSAFixture>   fix_;
    std::unique_ptr<JWTValidator> validator_;
};

TEST_F(RS512ValidatorTest, ValidToken_Accepted) {
    auto token = make_valid_rsa_jwt(*fix_, EVP_sha512(), "RS512", "rsa-key-1");
    EXPECT_NO_THROW({
        auto claims = validator_->parseAndValidate(token);
        EXPECT_EQ(claims.sub, "bob");
    });
}

TEST_F(RS512ValidatorTest, WrongSignature_Rejected) {
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(
                   now.time_since_epoch()).count() + 300;
    nlohmann::json payload = {{"sub","u"}, {"iss","issuerX"}, {"aud","audX"}, {"exp", exp}};
    auto up = build_token_header_payload("RS512", "rsa-key-1", payload);
    std::string bad_sig = b64url(std::vector<uint8_t>(256, 0xEE));
    EXPECT_THROW(validator_->parseAndValidate(up + "." + bad_sig), std::runtime_error);
}
