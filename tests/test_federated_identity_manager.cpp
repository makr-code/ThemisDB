/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_federated_identity_manager.cpp                ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 04:03:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     456                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 497f5c0a7  2026-02-24  fix(auth): audit fixes – ROADMAP status, token size guard... ║
    • a0247640c  2026-02-24  feat(auth): implement federated identity across multiple ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/bn.h>

#include "auth/federated_identity_manager.h"
#include "auth/auth_error.h"

using namespace themis::auth;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Shared helpers (base64url + RSA key fixture + token builder)
// Copied from test_oidc_provider.cpp to keep this test self-contained.
// ---------------------------------------------------------------------------

static std::string b64url_fed(const std::vector<uint8_t>& in) {
    static const char* tbl =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64;
    b64.reserve(((in.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= in.size()) {
        uint32_t n = (in[i] << 16) | (in[i + 1] << 8) | in[i + 2];
        b64.push_back(tbl[(n >> 18) & 63]);
        b64.push_back(tbl[(n >> 12) & 63]);
        b64.push_back(tbl[(n >>  6) & 63]);
        b64.push_back(tbl[(n      ) & 63]);
        i += 3;
    }
    if (i + 1 == in.size()) {
        uint32_t n = in[i] << 16;
        b64.push_back(tbl[(n >> 18) & 63]);
        b64.push_back(tbl[(n >> 12) & 63]);
        b64.push_back('=');
        b64.push_back('=');
    } else if (i + 2 == in.size()) {
        uint32_t n = (in[i] << 16) | (in[i + 1] << 8);
        b64.push_back(tbl[(n >> 18) & 63]);
        b64.push_back(tbl[(n >> 12) & 63]);
        b64.push_back(tbl[(n >>  6) & 63]);
        b64.push_back('=');
    }
    for (char& c : b64) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!b64.empty() && b64.back() == '=') b64.pop_back();
    return b64;
}

struct RSAFixtureFed {
    RSA*      rsa   = nullptr;
    EVP_PKEY* pkey  = nullptr;
    BIGNUM*   bn    = nullptr;

    RSAFixtureFed() {
        bn = BN_new();
        if (!bn) throw std::runtime_error("BN_new failed");
        if (BN_set_word(bn, RSA_F4) != 1)
            throw std::runtime_error("BN_set_word failed");
        rsa = RSA_new();
        if (!rsa) throw std::runtime_error("RSA_new failed");
        if (RSA_generate_key_ex(rsa, 2048, bn, nullptr) != 1)
            throw std::runtime_error("RSA_generate_key_ex failed");
        pkey = EVP_PKEY_new();
        if (!pkey) throw std::runtime_error("EVP_PKEY_new failed");
        if (EVP_PKEY_assign_RSA(pkey, rsa) != 1)
            throw std::runtime_error("EVP_PKEY_assign_RSA failed");
    }
    ~RSAFixtureFed() {
        if (pkey) EVP_PKEY_free(pkey);
        if (bn)   BN_free(bn);
    }
};

static std::string signRS256Fed(EVP_PKEY* pkey, const std::string& data) {
    EVP_MD_CTX* mctx = EVP_MD_CTX_new();
    if (!mctx) throw std::runtime_error("EVP_MD_CTX_new failed");
    if (EVP_DigestSignInit(mctx, nullptr, EVP_sha256(), nullptr, pkey) != 1)
        throw std::runtime_error("EVP_DigestSignInit failed");
    if (EVP_DigestSignUpdate(mctx, data.data(), data.size()) != 1)
        throw std::runtime_error("EVP_DigestSignUpdate failed");
    size_t siglen = 0;
    if (EVP_DigestSignFinal(mctx, nullptr, &siglen) != 1)
        throw std::runtime_error("EVP_DigestSignFinal (size) failed");
    std::vector<uint8_t> sig(siglen);
    if (EVP_DigestSignFinal(mctx, sig.data(), &siglen) != 1)
        throw std::runtime_error("EVP_DigestSignFinal failed");
    sig.resize(siglen);
    EVP_MD_CTX_free(mctx);
    return b64url_fed(sig);
}

static json makeJWKSFed(RSA* rsa) {
    const BIGNUM* n = nullptr;
    const BIGNUM* e = nullptr;
    RSA_get0_key(rsa, &n, &e, nullptr);
    std::vector<uint8_t> nb(BN_num_bytes(n)), eb(BN_num_bytes(e));
    BN_bn2bin(n, nb.data());
    BN_bn2bin(e, eb.data());
    json jwk = {{"kty","RSA"},{"kid","k1"},{"alg","RS256"},
                {"use","sig"},{"n",b64url_fed(nb)},{"e",b64url_fed(eb)}};
    return {{"keys", json::array({jwk})}};
}

static std::string buildTokenFed(EVP_PKEY* pkey, const json& payload,
                                  const std::string& kid = "k1") {
    json header = {{"alg","RS256"},{"typ","JWT"},{"kid",kid}};
    std::string h_str = header.dump();
    std::string p_str = payload.dump();
    std::string h = b64url_fed(std::vector<uint8_t>(h_str.begin(), h_str.end()));
    std::string p = b64url_fed(std::vector<uint8_t>(p_str.begin(), p_str.end()));
    std::string unsigned_token = h + "." + p;
    return unsigned_token + "." + signRS256Fed(pkey, unsigned_token);
}

// Build a minimal discovery document JSON string for a given issuer.
static json makeDiscoveryFed(const std::string& issuer,
                              const std::string& jwks_uri,
                              bool include_device = false) {
    json doc = {
        {"issuer",                 issuer},
        {"jwks_uri",               jwks_uri},
        {"authorization_endpoint", issuer + "/authorize"},
        {"token_endpoint",         issuer + "/token"},
    };
    if (include_device) {
        doc["device_authorization_endpoint"] = issuer + "/device";
    }
    return doc;
}

// Build OIDCProviderConfig for a given issuer
static OIDCProviderConfig makeConfig(const std::string& issuer,
                                     const std::string& audience = "") {
    OIDCProviderConfig cfg;
    cfg.issuer_url        = issuer;
    cfg.client_id         = "themisdb";
    cfg.expected_audience = audience;
    return cfg;
}

// ---------------------------------------------------------------------------
// Tests: realm management
// ---------------------------------------------------------------------------

TEST(FederatedIdentityManagerTest, EmptyManagerHasNoRealms) {
    FederatedIdentityManager mgr;
    EXPECT_EQ(mgr.realmCount(), 0u);
    EXPECT_TRUE(mgr.realmIssuers().empty());
}

TEST(FederatedIdentityManagerTest, AddRealmIncreasesCount) {
    FederatedIdentityManager mgr;
    // Use a mock HTTP function so no real network call is attempted
    mgr.setHttpGetForTesting([](const std::string&) -> std::string {
        return "{}"; // will never be called in these tests
    });

    OIDCProviderConfig cfg = makeConfig("https://idp.example.com/realms/prod");
    mgr.addRealm(cfg);
    EXPECT_EQ(mgr.realmCount(), 1u);
    EXPECT_TRUE(mgr.hasRealm("https://idp.example.com/realms/prod"));
}

TEST(FederatedIdentityManagerTest, TrailingSlashNormalized) {
    FederatedIdentityManager mgr;
    OIDCProviderConfig cfg = makeConfig("https://idp.example.com/realms/prod/");
    mgr.addRealm(cfg);
    // Lookup with and without trailing slash must both succeed
    EXPECT_TRUE(mgr.hasRealm("https://idp.example.com/realms/prod"));
    EXPECT_TRUE(mgr.hasRealm("https://idp.example.com/realms/prod/"));
}

TEST(FederatedIdentityManagerTest, DuplicateRealmThrows) {
    FederatedIdentityManager mgr;
    OIDCProviderConfig cfg = makeConfig("https://idp.example.com/realms/prod");
    mgr.addRealm(cfg);
    EXPECT_THROW(mgr.addRealm(cfg), AuthException);
}

TEST(FederatedIdentityManagerTest, EmptyIssuerThrows) {
    FederatedIdentityManager mgr;
    OIDCProviderConfig cfg = makeConfig("");
    EXPECT_THROW(mgr.addRealm(cfg), AuthException);
}

TEST(FederatedIdentityManagerTest, MultipleRealmsRegistered) {
    FederatedIdentityManager mgr;
    mgr.addRealm(makeConfig("https://idp.example.com/realms/prod"));
    mgr.addRealm(makeConfig("https://idp.example.com/realms/dev"));
    mgr.addRealm(makeConfig("https://other.example.com"));
    EXPECT_EQ(mgr.realmCount(), 3u);

    const auto issuers = mgr.realmIssuers();
    EXPECT_EQ(issuers.size(), 3u);
}

TEST(FederatedIdentityManagerTest, RemoveRealmDecreasesCount) {
    FederatedIdentityManager mgr;
    mgr.addRealm(makeConfig("https://idp.example.com/realms/prod"));
    mgr.addRealm(makeConfig("https://idp.example.com/realms/dev"));
    EXPECT_EQ(mgr.realmCount(), 2u);

    EXPECT_TRUE(mgr.removeRealm("https://idp.example.com/realms/prod"));
    EXPECT_EQ(mgr.realmCount(), 1u);
    EXPECT_FALSE(mgr.hasRealm("https://idp.example.com/realms/prod"));
    EXPECT_TRUE(mgr.hasRealm("https://idp.example.com/realms/dev"));
}

TEST(FederatedIdentityManagerTest, RemoveNonExistentRealmReturnsFalse) {
    FederatedIdentityManager mgr;
    EXPECT_FALSE(mgr.removeRealm("https://idp.example.com/realms/prod"));
}

TEST(FederatedIdentityManagerTest, RemoveWithTrailingSlashWorks) {
    FederatedIdentityManager mgr;
    mgr.addRealm(makeConfig("https://idp.example.com/realms/prod"));
    EXPECT_TRUE(mgr.removeRealm("https://idp.example.com/realms/prod/"));
    EXPECT_EQ(mgr.realmCount(), 0u);
}

TEST(FederatedIdentityManagerTest, RealmProviderThrowsForUnknownIssuer) {
    FederatedIdentityManager mgr;
    EXPECT_THROW(mgr.realmProvider("https://idp.example.com"), AuthException);
}

// ---------------------------------------------------------------------------
// Tests: validateToken – issuer routing
// ---------------------------------------------------------------------------

TEST(FederatedIdentityManagerTest, ValidateTokenNoRealmsThrows) {
    FederatedIdentityManager mgr;

    // Build a minimal JWT (payload only needs "iss"; signature not checked yet)
    RSAFixtureFed fix;
    const std::string issuer = "https://idp.example.com";
    const int64_t exp = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) + 300;
    json payload = {{"sub","u1"},{"iss",issuer},{"aud","themisdb"},{"exp",exp}};
    std::string token = buildTokenFed(fix.pkey, payload);

    EXPECT_THROW(mgr.validateToken(token), AuthException);
}

TEST(FederatedIdentityManagerTest, ValidateTokenUnknownIssuerThrows) {
    FederatedIdentityManager mgr;
    mgr.addRealm(makeConfig("https://other.example.com"));

    RSAFixtureFed fix;
    const std::string issuer = "https://idp.example.com";
    const int64_t exp = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) + 300;
    json payload = {{"sub","u1"},{"iss",issuer},{"aud","themisdb"},{"exp",exp}};
    std::string token = buildTokenFed(fix.pkey, payload);

    // The token's issuer is not registered
    EXPECT_THROW(mgr.validateToken(token), AuthException);
}

TEST(FederatedIdentityManagerTest, ValidateTokenRoutedToCorrectRealm) {
    // Set up two realms with independent key pairs
    RSAFixtureFed fix_prod;
    RSAFixtureFed fix_dev;

    const std::string prod_issuer = "https://idp.example.com/realms/prod";
    const std::string dev_issuer  = "https://idp.example.com/realms/dev";

    FederatedIdentityManager mgr;

    // Register both realms with mock HTTP that serves the correct JWKS per URL
    mgr.setHttpGetForTesting([&](const std::string& url) -> std::string {
        // Production discovery
        if (url.find(prod_issuer) != std::string::npos) {
            return makeDiscoveryFed(prod_issuer, prod_issuer + "/jwks").dump();
        }
        // Development discovery
        if (url.find(dev_issuer) != std::string::npos) {
            return makeDiscoveryFed(dev_issuer, dev_issuer + "/jwks").dump();
        }
        throw std::runtime_error("unexpected URL: " + url);
    });

    mgr.addRealm(makeConfig(prod_issuer, "themisdb"));
    mgr.addRealm(makeConfig(dev_issuer,  "themisdb"));

    // Inject correct JWKS into each realm's validator
    {
        OIDCDiscoveryDocument doc;
        doc.issuer                 = prod_issuer;
        doc.jwks_uri               = prod_issuer + "/jwks";
        doc.authorization_endpoint = prod_issuer + "/authorize";
        doc.token_endpoint         = prod_issuer + "/token";
        mgr.realmProvider(prod_issuer).setDiscoveryDocumentForTesting(doc);
        mgr.realmProvider(prod_issuer).validator().setJWKSForTesting(
            makeJWKSFed(fix_prod.rsa));
    }
    {
        OIDCDiscoveryDocument doc;
        doc.issuer                 = dev_issuer;
        doc.jwks_uri               = dev_issuer + "/jwks";
        doc.authorization_endpoint = dev_issuer + "/authorize";
        doc.token_endpoint         = dev_issuer + "/token";
        mgr.realmProvider(dev_issuer).setDiscoveryDocumentForTesting(doc);
        mgr.realmProvider(dev_issuer).validator().setJWKSForTesting(
            makeJWKSFed(fix_dev.rsa));
    }

    const int64_t exp = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) + 300;

    // Token signed by prod key
    json prod_payload = {{"sub","prod-user"},{"iss",prod_issuer},
                          {"aud","themisdb"},{"exp",exp}};
    std::string prod_token = buildTokenFed(fix_prod.pkey, prod_payload);

    // Token signed by dev key
    json dev_payload = {{"sub","dev-user"},{"iss",dev_issuer},
                         {"aud","themisdb"},{"exp",exp}};
    std::string dev_token = buildTokenFed(fix_dev.pkey, dev_payload);

    FederatedValidationResult prod_result;
    ASSERT_NO_THROW(prod_result = mgr.validateToken(prod_token));
    EXPECT_EQ(prod_result.claims.sub, "prod-user");
    EXPECT_EQ(prod_result.realm, prod_issuer);

    FederatedValidationResult dev_result;
    ASSERT_NO_THROW(dev_result = mgr.validateToken(dev_token));
    EXPECT_EQ(dev_result.claims.sub, "dev-user");
    EXPECT_EQ(dev_result.realm, dev_issuer);
}

TEST(FederatedIdentityManagerTest, ValidateTokenWrongKeyThrows) {
    // Issue a token for prod_realm but sign it with the dev key
    RSAFixtureFed fix_prod;
    RSAFixtureFed fix_dev;   // wrong key

    const std::string prod_issuer = "https://idp.example.com/realms/prod";

    FederatedIdentityManager mgr;
    mgr.addRealm(makeConfig(prod_issuer, "themisdb"));

    OIDCDiscoveryDocument doc;
    doc.issuer                 = prod_issuer;
    doc.jwks_uri               = prod_issuer + "/jwks";
    doc.authorization_endpoint = prod_issuer + "/authorize";
    doc.token_endpoint         = prod_issuer + "/token";
    mgr.realmProvider(prod_issuer).setDiscoveryDocumentForTesting(doc);
    // Inject the *prod* public key but sign with the *dev* private key
    mgr.realmProvider(prod_issuer).validator().setJWKSForTesting(
        makeJWKSFed(fix_prod.rsa));

    const int64_t exp = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) + 300;
    json payload = {{"sub","u1"},{"iss",prod_issuer},{"aud","themisdb"},{"exp",exp}};
    std::string token = buildTokenFed(fix_dev.pkey, payload); // signed with wrong key

    EXPECT_THROW(mgr.validateToken(token), std::runtime_error);
}

TEST(FederatedIdentityManagerTest, ValidateTokenAfterRealmRemovalThrows) {
    RSAFixtureFed fix;
    const std::string issuer = "https://idp.example.com/realms/prod";

    FederatedIdentityManager mgr;
    mgr.addRealm(makeConfig(issuer, "themisdb"));

    OIDCDiscoveryDocument doc;
    doc.issuer                 = issuer;
    doc.jwks_uri               = issuer + "/jwks";
    doc.authorization_endpoint = issuer + "/authorize";
    doc.token_endpoint         = issuer + "/token";
    mgr.realmProvider(issuer).setDiscoveryDocumentForTesting(doc);
    mgr.realmProvider(issuer).validator().setJWKSForTesting(makeJWKSFed(fix.rsa));

    const int64_t exp = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) + 300;
    json payload = {{"sub","u1"},{"iss",issuer},{"aud","themisdb"},{"exp",exp}};
    std::string token = buildTokenFed(fix.pkey, payload);

    // Validation should work before removal
    EXPECT_NO_THROW(mgr.validateToken(token));

    // Remove the realm
    EXPECT_TRUE(mgr.removeRealm(issuer));

    // Now validation must fail because the realm is gone
    EXPECT_THROW(mgr.validateToken(token), AuthException);
}

// ---------------------------------------------------------------------------
// Tests: malformed token handling
// ---------------------------------------------------------------------------

TEST(FederatedIdentityManagerTest, MalformedTokenThrows) {
    FederatedIdentityManager mgr;
    mgr.addRealm(makeConfig("https://idp.example.com"));
    EXPECT_THROW(mgr.validateToken("not-a-jwt"), AuthException);
}

TEST(FederatedIdentityManagerTest, OversizedTokenThrows) {
    FederatedIdentityManager mgr;
    mgr.addRealm(makeConfig("https://idp.example.com"));
    // Build a token that exceeds MAX_JWT_TOKEN_SIZE (16KB) by 1KB
    const std::string huge_token(MAX_JWT_TOKEN_SIZE + 1024, 'A');
    EXPECT_THROW(mgr.validateToken(huge_token), AuthException);
}

TEST(FederatedIdentityManagerTest, TokenMissingIssuerClaimThrows) {
    RSAFixtureFed fix;
    FederatedIdentityManager mgr;
    mgr.addRealm(makeConfig("https://idp.example.com"));

    const int64_t exp = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) + 300;
    json payload = {{"sub","u1"},{"aud","themisdb"},{"exp",exp}};  // no "iss"
    std::string token = buildTokenFed(fix.pkey, payload);

    EXPECT_THROW(mgr.validateToken(token), AuthException);
}

TEST(FederatedIdentityManagerTest, BearerPrefixStripped) {
    RSAFixtureFed fix;
    const std::string issuer = "https://idp.example.com";

    FederatedIdentityManager mgr;
    mgr.addRealm(makeConfig(issuer, "themisdb"));

    OIDCDiscoveryDocument doc;
    doc.issuer                 = issuer;
    doc.jwks_uri               = issuer + "/jwks";
    doc.authorization_endpoint = issuer + "/authorize";
    doc.token_endpoint         = issuer + "/token";
    mgr.realmProvider(issuer).setDiscoveryDocumentForTesting(doc);
    mgr.realmProvider(issuer).validator().setJWKSForTesting(makeJWKSFed(fix.rsa));

    const int64_t exp = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) + 300;
    json payload = {{"sub","u1"},{"iss",issuer},{"aud","themisdb"},{"exp",exp}};
    std::string token = buildTokenFed(fix.pkey, payload);

    FederatedValidationResult result;
    ASSERT_NO_THROW(result = mgr.validateToken("Bearer " + token));
    EXPECT_EQ(result.claims.sub, "u1");
    EXPECT_EQ(result.realm, issuer);
}
