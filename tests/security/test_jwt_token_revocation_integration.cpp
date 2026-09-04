/**
 * @file test_jwt_token_revocation_integration.cpp
 * @brief Integration tests for JTI-based per-token revocation in JWTValidator
 *
 * Tests cover:
 *  - JWTClaims now has a jti field
 *  - jti is extracted from the JWT payload
 *  - setTokenBlacklist() wires the blacklist into the validator
 *  - A revoked JTI causes parseAndValidate() to throw
 *  - A token with no jti claim is accepted even when a blacklist is attached
 *  - Detaching (nullptr) the blacklist stops revocation checks
 *  - EdDSA algorithm is now whitelisted (regression for the allowlist bug)
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <chrono>
#include <string>
#include <vector>
#include <stdexcept>

#include "auth/jwt_validator.h"
#include "auth/token_blacklist.h"
#include "utils/logger.h"

using namespace themis::auth;

namespace {

// ── Base64url helpers ──────────────────────────────────────────────────────

static std::string b64url(const std::vector<uint8_t>& in) {
    static const char* tbl =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64 = {};
    b64.reserve(((in.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= in.size()) {
        uint32_t n = ((uint32_t)in[i]<<16)|((uint32_t)in[i+1]<<8)|in[i+2];
        b64.push_back(tbl[(n>>18)&63]); b64.push_back(tbl[(n>>12)&63]);
        b64.push_back(tbl[(n>> 6)&63]); b64.push_back(tbl[n&63]);
        i += 3;
    }
    if (i + 1 == in.size()) {
        uint32_t n = (uint32_t)in[i] << 16;
        b64.push_back(tbl[(n>>18)&63]); b64.push_back(tbl[(n>>12)&63]);
        b64.push_back('='); b64.push_back('=');
    } else if (i + 2 == in.size()) {
        uint32_t n = ((uint32_t)in[i]<<16)|((uint32_t)in[i+1]<<8);
        b64.push_back(tbl[(n>>18)&63]); b64.push_back(tbl[(n>>12)&63]);
        b64.push_back(tbl[(n>>6)&63]);  b64.push_back('=');
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

// ── Minimal ECDSA P-256 key fixture ───────────────────────────────────────

struct ECKey {
    EVP_PKEY* pkey = nullptr;

    ECKey() {
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
        EVP_PKEY_keygen_init(ctx);
        EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1);
        EVP_PKEY_keygen(ctx, &pkey);
        EVP_PKEY_CTX_free(ctx);
    }
    ~ECKey() { if (pkey) EVP_PKEY_free(pkey); }
    ECKey(const ECKey&) = delete;
    ECKey& operator=(const ECKey&) = delete;

    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> publicKeyCoords() const {
        const EC_KEY* ec = EVP_PKEY_get0_EC_KEY(pkey);
        const EC_POINT* pt = EC_KEY_get0_public_key(ec);
        const EC_GROUP* grp = EC_KEY_get0_group(ec);
        BIGNUM* bx = BN_new(); BIGNUM* by = BN_new();
        EC_POINT_get_affine_coordinates_GFp(grp, pt, bx, by, nullptr);
        std::vector<uint8_t> x(32, 0), y(32, 0);
        BN_bn2binpad(bx, x.data(), 32);
        BN_bn2binpad(by, y.data(), 32);
        BN_free(bx); BN_free(by);
        return {x, y};
    }
};

static std::string signES256(EVP_PKEY* pkey, const std::string& msg) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey);
    EVP_DigestSignUpdate(ctx, msg.data(), msg.size());
    size_t sigLen = 0;
    EVP_DigestSignFinal(ctx, nullptr, &sigLen);
    std::vector<uint8_t> der(sigLen);
    EVP_DigestSignFinal(ctx, der.data(), &sigLen);
    EVP_MD_CTX_free(ctx);
    der.resize(sigLen);

    const unsigned char* p = der.data();
    ECDSA_SIG* esig = d2i_ECDSA_SIG(nullptr, &p, (long)der.size());
    const BIGNUM *r = nullptr, *s = nullptr;
    ECDSA_SIG_get0(esig, &r, &s);
    std::vector<uint8_t> rs(64, 0);
    BN_bn2binpad(r, rs.data(),      32);
    BN_bn2binpad(s, rs.data() + 32, 32);
    ECDSA_SIG_free(esig);
    return b64url(rs);
}

static nlohmann::json makeECJwks(const ECKey& key, const std::string& kid = "ec1") {
    auto [x, y] = key.publicKeyCoords();
    nlohmann::json jwk = {
        {"kty","EC"},{"crv","P-256"},{"kid",kid},
        {"alg","ES256"},{"use","sig"},
        {"x",b64url(x)},{"y",b64url(y)}
    };
    return nlohmann::json{{"keys", nlohmann::json::array({jwk})}};
}

static std::string makeES256Token(const ECKey& key,
                                  const std::string& kid,
                                  const nlohmann::json& extra_claims,
                                  int exp_offset_sec = 300) {
    auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    nlohmann::json payload = {
        {"sub",   "alice"},
        {"email", "alice@example.com"},
        {"iss",   "test-issuer"},
        {"aud",   "test-audience"},
        {"exp",   now_sec + exp_offset_sec},
    };
    for (auto it = extra_claims.begin(); it != extra_claims.end(); ++it) {
        payload[it.key()] = it.value();
    }

    nlohmann::json header = {{"alg","ES256"},{"typ","JWT"},{"kid",kid}};
    std::string hp = b64urlStr(header.dump()) + "." + b64urlStr(payload.dump());
    return hp + "." + signES256(key.pkey, hp);
}

static JWTValidatorConfig makeConfig() {
    JWTValidatorConfig cfg;
    cfg.jwks_url          = "";
    cfg.expected_issuer   = "test-issuer";
    cfg.expected_audience = "test-audience";
    cfg.clock_skew        = std::chrono::seconds(60);
    return cfg;
}

} // anonymous namespace

// ============================================================================
// JTI field in JWTClaims
// ============================================================================

TEST(JWTClaimsJtiTest, JtiFieldExistsAndDefaultsToEmpty) {
    JWTClaims claims;
    EXPECT_TRUE(claims.jti.empty());
}

TEST(JWTClaimsJtiTest, JtiCanBeSetAndRetrieved) {
    JWTClaims claims;
    claims.jti = "unique-token-id-42";
    EXPECT_EQ(claims.jti, "unique-token-id-42");
}

// ============================================================================
// JTI extraction from JWT payload
// ============================================================================

class JtiExtractionTest : public ::testing::Test {
protected:
    void SetUp() override {
        val_ = std::make_unique<JWTValidator>(makeConfig());
        val_->setJWKSForTesting(makeECJwks(key_));
    }
    ECKey key_;
    std::unique_ptr<JWTValidator> val_;
};

TEST_F(JtiExtractionTest, JtiClaimIsPopulatedFromPayload) {
    auto token = makeES256Token(key_, "ec1", {{"jti", "tok-abc-123"}});
    auto claims = val_->parseAndValidate(token);
    EXPECT_EQ(claims.jti, "tok-abc-123");
}

TEST_F(JtiExtractionTest, MissingJtiClaimResultsInEmptyJti) {
    auto token = makeES256Token(key_, "ec1", {});
    auto claims = val_->parseAndValidate(token);
    EXPECT_TRUE(claims.jti.empty());
}

// ============================================================================
// setTokenBlacklist integration
// ============================================================================

class JtiRevocationTest : public ::testing::Test {
protected:
    void SetUp() override {
        val_ = std::make_unique<JWTValidator>(makeConfig());
        val_->setJWKSForTesting(makeECJwks(key_));
    }
    ECKey key_;
    std::unique_ptr<JWTValidator> val_;
    TokenBlacklist blacklist_;
};

TEST_F(JtiRevocationTest, ValidTokenAcceptedWithBlacklistAttached) {
    val_->setTokenBlacklist(&blacklist_);
    auto token = makeES256Token(key_, "ec1", {{"jti", "tok-1"}});
    EXPECT_NO_THROW(val_->parseAndValidate(token));
}

TEST_F(JtiRevocationTest, RevokedJtiIsRejected) {
    val_->setTokenBlacklist(&blacklist_);
    auto token = makeES256Token(key_, "ec1", {{"jti", "tok-revoked"}});
    // Revoke before validation
    blacklist_.revoke("tok-revoked",
        std::chrono::system_clock::now() + std::chrono::hours(1));

    EXPECT_THROW(val_->parseAndValidate(token), std::runtime_error);
}

TEST_F(JtiRevocationTest, RejectedTokenMessageMentionsRevocation) {
    val_->setTokenBlacklist(&blacklist_);
    auto token = makeES256Token(key_, "ec1", {{"jti", "tok-bad"}});
    blacklist_.revoke("tok-bad",
        std::chrono::system_clock::now() + std::chrono::hours(1));

    try {
        val_->parseAndValidate(token);
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("revoked"), std::string::npos);
    }
}

TEST_F(JtiRevocationTest, TokenWithNoJtiAcceptedEvenWhenBlacklistAttached) {
    val_->setTokenBlacklist(&blacklist_);
    // Populate blacklist with something unrelated
    blacklist_.revoke("some-other-jti",
        std::chrono::system_clock::now() + std::chrono::hours(1));

    // Token has no jti claim – should be accepted (no jti = no check)
    auto token = makeES256Token(key_, "ec1", {});
    EXPECT_NO_THROW(val_->parseAndValidate(token));
}

TEST_F(JtiRevocationTest, BlacklistAttachedAndNoJtiEmitsWarningOnce) {
    val_->setTokenBlacklist(&blacklist_);
    auto token = makeES256Token(key_, "ec1", {});

    // Use warn_count deltas to avoid perturbing global metrics shared across tests.
    // The warning must fire exactly once per validator lifecycle (deduplication).
    auto before_first = themis::utils::Logger::getMetrics().snapshot();
    EXPECT_NO_THROW(val_->parseAndValidate(token));
    auto after_first = themis::utils::Logger::getMetrics().snapshot();
    EXPECT_EQ(after_first.warn_count - before_first.warn_count, 1u)
        << "Expected exactly one warning on first no-jti token";

    // Second call: warn_count must NOT increase further (deduplication)
    EXPECT_NO_THROW(val_->parseAndValidate(token));
    auto after_second = themis::utils::Logger::getMetrics().snapshot();
    EXPECT_EQ(after_second.warn_count, after_first.warn_count)
        << "Warning should be suppressed on subsequent no-jti tokens";
}

TEST_F(JtiRevocationTest, DetachingBlacklistStopsRevocationCheck) {
    // Attach, revoke a token, then detach – the token should be accepted again
    val_->setTokenBlacklist(&blacklist_);
    auto token = makeES256Token(key_, "ec1", {{"jti", "tok-x"}});
    blacklist_.revoke("tok-x",
        std::chrono::system_clock::now() + std::chrono::hours(1));

    EXPECT_THROW(val_->parseAndValidate(token), std::runtime_error);

    // Detach
    val_->setTokenBlacklist(nullptr);
    EXPECT_NO_THROW(val_->parseAndValidate(token));
}

TEST_F(JtiRevocationTest, ExpiredBlacklistEntryAllowsToken) {
    val_->setTokenBlacklist(&blacklist_);
    auto token = makeES256Token(key_, "ec1", {{"jti", "tok-exp"}});
    // Revoke with expiry in the past (already expired)
    blacklist_.revoke("tok-exp",
        std::chrono::system_clock::now() - std::chrono::seconds(1));
    // pruneExpired removes it
    blacklist_.pruneExpired();

    EXPECT_NO_THROW(val_->parseAndValidate(token));
}

TEST_F(JtiRevocationTest, NoBlacklistAttached_TokenWithJtiAlwaysAccepted) {
    // No setTokenBlacklist() call – revocation is a no-op
    auto token = makeES256Token(key_, "ec1", {{"jti", "tok-any"}});
    EXPECT_NO_THROW(val_->parseAndValidate(token));
}

// ============================================================================
// Real-time invalidation: revocation callback fires on revoke
// ============================================================================

TEST_F(JtiRevocationTest, RevocationCallbackFiredOnRevoke) {
    // Attach a callback to the blacklist that records every notified JTI.
    std::string notified_jti = {};
    blacklist_.setOnRevokeCallback([&notified_jti](const std::string& jti) {
        notified_jti = jti;
    });

    val_->setTokenBlacklist(&blacklist_);
    auto token = makeES256Token(key_, "ec1", {{"jti", "tok-realtime"}});

    // Revoke: the callback must fire synchronously before revoke() returns.
    blacklist_.revoke("tok-realtime",
        std::chrono::system_clock::now() + std::chrono::hours(1));

    EXPECT_EQ(notified_jti, "tok-realtime");
    // Token must also be rejected by the validator.
    EXPECT_THROW(val_->parseAndValidate(token), std::runtime_error);
}

TEST_F(JtiRevocationTest, RevocationCallbackCanQueryBlacklistWithoutDeadlock) {
    // Verify that the callback may call isRevoked() on the same blacklist
    // without deadlocking (the callback is invoked with the mutex released).
    bool callback_saw_revoked = false;
    blacklist_.setOnRevokeCallback([&](const std::string& jti) {
        callback_saw_revoked = blacklist_.isRevoked(jti);
    });

    blacklist_.revoke("tok-reentrant",
        std::chrono::system_clock::now() + std::chrono::hours(1));

    EXPECT_TRUE(callback_saw_revoked);
}

// ============================================================================
// EdDSA allowlist regression test
// ============================================================================

TEST(EdDSAAllowlistRegression, UnsupportedAlgorithmHintIncludesEdDSA) {
    // A token with an unsupported algorithm should produce an error message
    // that lists the SUPPORTED algorithms — which must now include EdDSA.
    JWTValidatorConfig cfg;
    cfg.jwks_url = "";
    cfg.require_issuer_validation = false;
    cfg.require_audience_validation = false;
    JWTValidator val(cfg);

    // Build a token with alg: "HS256" (unsupported) to trigger the early check
    auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    nlohmann::json header  = {{"alg","HS256"},{"typ","JWT"},{"kid","k1"}};
    nlohmann::json payload = {{"sub","u"},{"exp", now_sec + 300}};
    std::string hp = b64urlStr(header.dump()) + "." + b64urlStr(payload.dump());
    std::string token = hp + ".fakesig";

    try {
        val.parseAndValidate(token);
        FAIL() << "Expected exception for unsupported alg";
    } catch (const std::runtime_error& e) {
        std::string msg(e.what());
        // The error message should list EdDSA as a supported algorithm
        EXPECT_NE(msg.find("EdDSA"), std::string::npos)
            << "Error message did not mention EdDSA: " << msg;
    }
}

// ============================================================================
// require_jti enforcement
// ============================================================================

class RequireJtiTest : public ::testing::Test {
protected:
    void SetUp() override {
        JWTValidatorConfig cfg = makeConfig();
        cfg.require_jti = true;
        val_ = std::make_unique<JWTValidator>(cfg);
        val_->setJWKSForTesting(makeECJwks(key_));
    }
    ECKey key_;
    std::unique_ptr<JWTValidator> val_;
};

TEST_F(RequireJtiTest, AcceptsTokenWithJti) {
    auto token = makeES256Token(key_, "ec1", {{"jti", "required-jti-001"}});
    EXPECT_NO_THROW(val_->parseAndValidate(token));
}

TEST_F(RequireJtiTest, RejectsTokenWithoutJti) {
    auto token = makeES256Token(key_, "ec1", {});
    EXPECT_THROW(val_->parseAndValidate(token), std::runtime_error);
}

TEST_F(RequireJtiTest, ErrorMessageMentionsJti) {
    auto token = makeES256Token(key_, "ec1", {});
    try {
        val_->parseAndValidate(token);
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("jti"), std::string::npos)
            << "Error message did not mention jti: " << e.what();
    }
}

TEST(RequireJtiDefaultTest, RequireJtiDefaultsToFalse) {
    JWTValidatorConfig cfg;
    EXPECT_FALSE(cfg.require_jti);
}

TEST(RequireJtiDefaultTest, AcceptsTokenWithoutJtiByDefault) {
    ECKey key;
    JWTValidatorConfig cfg = makeConfig();
    JWTValidator val(cfg);
    val.setJWKSForTesting(makeECJwks(key));

    auto token = makeES256Token(key, "ec1", {});
    EXPECT_NO_THROW(val.parseAndValidate(token));
}
