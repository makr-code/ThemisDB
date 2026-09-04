#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/bn.h>

#include "auth/oidc_provider.h"
#include "auth/auth_error.h"

using namespace themis::auth;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Shared helpers (base64url + RSA key fixture + token builder)
// ---------------------------------------------------------------------------

static std::string b64url(const std::vector<uint8_t>& in) {
    static const char* tbl =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64 = {};
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
        if (c == '+') {
          c = '-';
        }
        else if (c == '/') c = '_';
    }
    while (!b64.empty() && b64.back() == '=') {
      b64.pop_back();
    }
    return b64;
}

struct RSAFixture {
    RSA*      rsa   = nullptr;
    EVP_PKEY* pkey  = nullptr;
    BIGNUM*   bn    = nullptr;

    RSAFixture() {
        bn = BN_new();
        if (!bn) {
          throw std::runtime_error("BN_new failed");
        }
        if (BN_set_word(bn, RSA_F4) != 1)
            throw std::runtime_error("BN_set_word failed");
        rsa = RSA_new();
        if (!rsa) {
          throw std::runtime_error("RSA_new failed");
        }
        if (RSA_generate_key_ex(rsa, 2048, bn, nullptr) != 1)
            throw std::runtime_error("RSA_generate_key_ex failed");
        pkey = EVP_PKEY_new();
        if (!pkey) {
          throw std::runtime_error("EVP_PKEY_new failed");
        }
        if (EVP_PKEY_assign_RSA(pkey, rsa) != 1)
            throw std::runtime_error("EVP_PKEY_assign_RSA failed");
    }
    ~RSAFixture() {
        if (pkey) EVP_PKEY_free(pkey); // frees rsa too
        if (bn) {
          BN_free(bn);
        }
    }
};

static std::string signRS256(EVP_PKEY* pkey, const std::string& data) {
    EVP_MD_CTX* mctx = EVP_MD_CTX_new();
    if (!mctx) {
      throw std::runtime_error("EVP_MD_CTX_new failed");
    }
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
    return b64url(sig);
}

static json makeJWKS(RSA* rsa) {
    const BIGNUM* n = nullptr;
    const BIGNUM* e = nullptr;
    RSA_get0_key(rsa, &n, &e, nullptr);
    std::vector<uint8_t> nb(BN_num_bytes(n)), eb(BN_num_bytes(e));
    BN_bn2bin(n, nb.data());
    BN_bn2bin(e, eb.data());
    json jwk = {{"kty","RSA"},{"kid","k1"},{"alg","RS256"},
                {"use","sig"},{"n",b64url(nb)},{"e",b64url(eb)}};
    return {{"keys", json::array({jwk})}};
}

static std::string buildToken(EVP_PKEY* pkey, const json& payload,
                               const std::string& kid = "k1") {
    json header = {{"alg","RS256"},{"typ","JWT"},{"kid",kid}};
    std::string h_str = header.dump();
    std::string p_str = payload.dump();
    std::string h = b64url(std::vector<uint8_t>(h_str.begin(), h_str.end()));
    std::string p = b64url(std::vector<uint8_t>(p_str.begin(), p_str.end()));
    std::string unsigned_token = h + "." + p;
    return unsigned_token + "." + signRS256(pkey, unsigned_token);
}

// ---------------------------------------------------------------------------
// Helpers to build discovery JSON
// ---------------------------------------------------------------------------

static json makeDiscoveryJson(const std::string& issuer,
                               const std::string& jwks_uri,
                               bool include_device = true) {
    json doc = {
        {"issuer",                 issuer},
        {"jwks_uri",               jwks_uri},
        {"authorization_endpoint", issuer + "/authorize"},
        {"token_endpoint",         issuer + "/token"},
        {"userinfo_endpoint",      issuer + "/userinfo"},
        {"response_types_supported", json::array({"code"})},
        {"scopes_supported",         json::array({"openid","email","profile"})},
        {"grant_types_supported",    json::array({"authorization_code","device_code"})},
        {"id_token_signing_alg_values_supported", json::array({"RS256"})}
    };
    if (include_device) {
        doc["device_authorization_endpoint"] = issuer + "/device_authorization";
    }
    return doc;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// ── Construction ────────────────────────────────────────────────────────────

TEST(OIDCProviderTest, ThrowsOnEmptyIssuer) {
    OIDCProviderConfig cfg;
    cfg.issuer_url = "";
    cfg.client_id  = "client";
    EXPECT_THROW((void)OIDCProvider{cfg}, AuthException);
}

TEST(OIDCProviderTest, ThrowsOnEmptyClientId) {
    OIDCProviderConfig cfg;
    cfg.issuer_url = "https://idp.example.com";
    cfg.client_id  = "";
    EXPECT_THROW((void)OIDCProvider{cfg}, AuthException);
}

// ── Discovery document parsing ───────────────────────────────────────────────

TEST(OIDCProviderTest, DiscoveryViaMockHttp) {
    const std::string issuer = "https://idp.example.com/realms/test";
    OIDCProviderConfig cfg;
    cfg.issuer_url = issuer;
    cfg.client_id  = "myapp";

    OIDCProvider provider(cfg);
    provider.setHttpGetForTesting([&](const std::string& url) -> std::string {
        EXPECT_EQ(url, issuer + "/.well-known/openid-configuration");
        return makeDiscoveryJson(issuer, issuer + "/jwks").dump();
    });

    ASSERT_NO_THROW(provider.discover());
    const auto& doc = provider.discoveryDocument();
    EXPECT_EQ(doc.issuer, issuer);
    EXPECT_EQ(doc.jwks_uri, issuer + "/jwks");
    EXPECT_EQ(doc.token_endpoint, issuer + "/token");
    EXPECT_EQ(doc.device_authorization_endpoint, issuer + "/device_authorization");
}

TEST(OIDCProviderTest, IssuerMismatchThrows) {
    const std::string issuer = "https://idp.example.com/realms/test";
    OIDCProviderConfig cfg;
    cfg.issuer_url = issuer;
    cfg.client_id  = "myapp";

    OIDCProvider provider(cfg);
    provider.setHttpGetForTesting([&](const std::string&) -> std::string {
        // Return a discovery doc with a different issuer
        return makeDiscoveryJson("https://other.example.com",
                                 "https://other.example.com/jwks").dump();
    });

    EXPECT_THROW(provider.discover(), AuthException);
}

TEST(OIDCProviderTest, HttpErrorThrows) {
    OIDCProviderConfig cfg;
    cfg.issuer_url = "https://idp.example.com";
    cfg.client_id  = "myapp";

    OIDCProvider provider(cfg);
    provider.setHttpGetForTesting([](const std::string&) -> std::string {
        throw std::runtime_error("connection refused");
    });

    EXPECT_THROW(provider.discover(), AuthException);
}

TEST(OIDCProviderTest, InvalidJsonThrows) {
    OIDCProviderConfig cfg;
    cfg.issuer_url = "https://idp.example.com";
    cfg.client_id  = "myapp";

    OIDCProvider provider(cfg);
    provider.setHttpGetForTesting([](const std::string&) -> std::string {
        return "not-json";
    });

    EXPECT_THROW(provider.discover(), AuthException);
}

TEST(OIDCProviderTest, TrailingSlashNormalized) {
    const std::string issuer = "https://idp.example.com/realms/test";
    OIDCProviderConfig cfg;
    cfg.issuer_url = issuer + "/";   // trailing slash
    cfg.client_id  = "myapp";

    OIDCProvider provider(cfg);
    std::string requested_url = {};
    provider.setHttpGetForTesting([&](const std::string& url) -> std::string {
        requested_url = url;
        return makeDiscoveryJson(issuer, issuer + "/jwks").dump();
    });

    ASSERT_NO_THROW(provider.discover());
    EXPECT_EQ(requested_url, issuer + "/.well-known/openid-configuration");
}

// ── setDiscoveryDocumentForTesting ──────────────────────────────────────────

TEST(OIDCProviderTest, InjectDiscoveryDocument) {
    OIDCProviderConfig cfg;
    cfg.issuer_url = "https://idp.example.com";
    cfg.client_id  = "myapp";

    OIDCDiscoveryDocument doc;
    doc.issuer                         = "https://idp.example.com";
    doc.jwks_uri                       = "https://idp.example.com/jwks";
    doc.authorization_endpoint         = "https://idp.example.com/authorize";
    doc.token_endpoint                 = "https://idp.example.com/token";
    doc.device_authorization_endpoint  = "https://idp.example.com/device";

    OIDCProvider provider(cfg);
    provider.setDiscoveryDocumentForTesting(doc);

    EXPECT_EQ(provider.discoveryDocument().issuer, "https://idp.example.com");
    EXPECT_EQ(provider.discoveryDocument().device_authorization_endpoint,
              "https://idp.example.com/device");
}

// ── Token validation ─────────────────────────────────────────────────────────

TEST(OIDCProviderTest, ValidateTokenSuccess) {
    RSAFixture fix;
    const std::string issuer = "https://idp.example.com";
    const auto now = std::chrono::system_clock::now();
    const int64_t exp =
        std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count() + 300;

    json payload = {{"sub","user1"},{"email","user1@example.com"},
                    {"iss", issuer},{"aud","myapp"},{"exp", exp}};
    std::string token = buildToken(fix.pkey, payload);

    // Build discovery doc with injected JWKS
    OIDCProviderConfig cfg;
    cfg.issuer_url        = issuer;
    cfg.client_id         = "myapp";
    cfg.expected_audience = "myapp";

    OIDCDiscoveryDocument doc;
    doc.issuer                 = issuer;
    doc.jwks_uri               = issuer + "/jwks";
    doc.authorization_endpoint = issuer + "/authorize";
    doc.token_endpoint         = issuer + "/token";

    OIDCProvider provider(cfg);
    provider.setDiscoveryDocumentForTesting(doc);
    // Inject the JWKS into the validator directly for testing
    provider.validator().setJWKSForTesting(makeJWKS(fix.rsa));

    JWTClaims claims;
    ASSERT_NO_THROW(claims = provider.validateToken(token));
    EXPECT_EQ(claims.sub, "user1");
    EXPECT_EQ(claims.email, "user1@example.com");
    EXPECT_EQ(claims.issuer, issuer);
}

TEST(OIDCProviderTest, ValidateTokenExpiredThrows) {
    RSAFixture fix;
    const std::string issuer = "https://idp.example.com";
    const int64_t exp = 1; // far in the past

    json payload = {{"sub","user1"},{"iss", issuer},{"aud","myapp"},{"exp", exp}};
    std::string token = buildToken(fix.pkey, payload);

    OIDCProviderConfig cfg;
    cfg.issuer_url        = issuer;
    cfg.client_id         = "myapp";
    cfg.expected_audience = "myapp";
    cfg.clock_skew        = std::chrono::seconds(0);

    OIDCDiscoveryDocument doc;
    doc.issuer                 = issuer;
    doc.jwks_uri               = issuer + "/jwks";
    doc.authorization_endpoint = issuer + "/authorize";
    doc.token_endpoint         = issuer + "/token";

    OIDCProvider provider(cfg);
    provider.setDiscoveryDocumentForTesting(doc);
    provider.validator().setJWKSForTesting(makeJWKS(fix.rsa));

    EXPECT_THROW(provider.validateToken(token), std::runtime_error);
}

// ── Device flow creation ─────────────────────────────────────────────────────

TEST(OIDCProviderTest, CreateDeviceFlowSuccess) {
    OIDCProviderConfig cfg;
    cfg.issuer_url = "https://idp.example.com";
    cfg.client_id  = "myapp";
    cfg.scopes     = {"openid", "email"};

    OIDCDiscoveryDocument doc;
    doc.issuer                        = "https://idp.example.com";
    doc.jwks_uri                      = "https://idp.example.com/jwks";
    doc.authorization_endpoint        = "https://idp.example.com/authorize";
    doc.token_endpoint                = "https://idp.example.com/token";
    doc.device_authorization_endpoint = "https://idp.example.com/device_authorization";

    OIDCProvider provider(cfg);
    provider.setDiscoveryDocumentForTesting(doc);

    EXPECT_NO_THROW(provider.createDeviceFlow());
}

TEST(OIDCProviderTest, CreateDeviceFlowNoEndpointThrows) {
    OIDCProviderConfig cfg;
    cfg.issuer_url = "https://idp.example.com";
    cfg.client_id  = "myapp";

    OIDCDiscoveryDocument doc;
    doc.issuer                 = "https://idp.example.com";
    doc.jwks_uri               = "https://idp.example.com/jwks";
    doc.authorization_endpoint = "https://idp.example.com/authorize";
    doc.token_endpoint         = "https://idp.example.com/token";
    // device_authorization_endpoint intentionally absent

    OIDCProvider provider(cfg);
    provider.setDiscoveryDocumentForTesting(doc);

    EXPECT_THROW(provider.createDeviceFlow(), AuthException);
}

TEST(OIDCProviderTest, CreateDeviceFlowAutoDiscoversAndThrowsOnHttpError) {
    OIDCProviderConfig cfg;
    cfg.issuer_url = "https://idp.example.com";
    cfg.client_id  = "myapp";

    OIDCProvider provider(cfg);
    // Inject a failing HTTP mock – createDeviceFlow() must trigger discover() and
    // surface the AuthException from the failed HTTP request.
    provider.setHttpGetForTesting([](const std::string&) -> std::string {
        throw std::runtime_error("connection refused");
    });

    EXPECT_THROW(provider.createDeviceFlow(), AuthException);
}

// ── Discovery is idempotent ──────────────────────────────────────────────────

TEST(OIDCProviderTest, DiscoverCalledTwiceIsIdempotent) {
    const std::string issuer = "https://idp.example.com";
    OIDCProviderConfig cfg;
    cfg.issuer_url = issuer;
    cfg.client_id  = "myapp";

    int call_count = 0;
    OIDCProvider provider(cfg);
    provider.setHttpGetForTesting([&](const std::string&) -> std::string {
        ++call_count;
        return makeDiscoveryJson(issuer, issuer + "/jwks").dump();
    });

    provider.discover();
    provider.discover();  // second call must be no-op
    EXPECT_EQ(call_count, 1);
}

// ── scopes default to openid ─────────────────────────────────────────────────

TEST(OIDCProviderTest, DefaultScopeIsOpenid) {
    OIDCProviderConfig cfg;
    cfg.issuer_url = "https://idp.example.com";
    cfg.client_id  = "myapp";
    // scopes left empty

    OIDCDiscoveryDocument doc;
    doc.issuer                        = "https://idp.example.com";
    doc.jwks_uri                      = "https://idp.example.com/jwks";
    doc.authorization_endpoint        = "https://idp.example.com/authorize";
    doc.token_endpoint                = "https://idp.example.com/token";
    doc.device_authorization_endpoint = "https://idp.example.com/device";

    OIDCProvider provider(cfg);
    provider.setDiscoveryDocumentForTesting(doc);

    // Verify createDeviceFlow succeeds (scope handling exercised internally)
    EXPECT_NO_THROW(provider.createDeviceFlow());
}

// ── createDeviceFlow() auto-discovers ────────────────────────────────────────

TEST(OIDCProviderTest, CreateDeviceFlowAutoDiscovery) {
    const std::string issuer = "https://idp.example.com";
    OIDCProviderConfig cfg;
    cfg.issuer_url = issuer;
    cfg.client_id  = "myapp";
    cfg.scopes     = {"openid", "email"};

    OIDCProvider provider(cfg);
    // Inject a mock so no real HTTP call is made; createDeviceFlow() must
    // trigger discover() automatically before constructing the flow.
    provider.setHttpGetForTesting([&](const std::string&) -> std::string {
        return makeDiscoveryJson(issuer, issuer + "/jwks").dump();
    });

    // No explicit discover() call – must succeed via lazy auto-discovery
    EXPECT_NO_THROW(provider.createDeviceFlow());
    // Discovery document should now be cached
    EXPECT_EQ(provider.discoveryDocument().issuer, issuer);
}
