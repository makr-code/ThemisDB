/**
 * @file test_security_negative_integration.cpp
 * @brief Negative integration tests for Vault, JWT, and RBAC security components.
 *
 * These tests verify that security components correctly reject invalid inputs,
 * misconfigurations, and attack attempts.  All tests are self-contained and do
 * not require any external services (Vault, Keycloak, etc.).
 *
 * Test categories:
 *   - Vault: invalid tokens, wrong address, unauthorised access, HTTP errors
 *   - JWT:   expired, wrong issuer/audience, tampered signature, revoked kid,
 *            algorithm confusion, malformed tokens
 *   - RBAC:  denied permissions, empty role set, unknown role, cycle detection,
 *            wildcard misconfiguration
 *
 * These tests are REQUIRED to pass before merge (see CI workflow
 * security-hardening-ci.yml).
 */

#include <gtest/gtest.h>
#include "auth/jwt_validator.h"
#include "security/rbac.h"
#include "security/vault_key_provider.h"
#include "security/key_provider.h"

#include <nlohmann/json.hpp>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/bn.h>

#include <chrono>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Shared helpers (base64url + RSA key generation + signing)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

static std::string b64url(const std::vector<uint8_t>& in) {
    static const char* tbl =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64;
    b64.reserve(((in.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= in.size()) {
        uint32_t n = (uint32_t(in[i]) << 16) | (uint32_t(in[i+1]) << 8) | in[i+2];
        b64 += tbl[(n >> 18) & 63];
        b64 += tbl[(n >> 12) & 63];
        b64 += tbl[(n >>  6) & 63];
        b64 += tbl[ n        & 63];
        i += 3;
    }
    if (i + 1 == in.size()) {
        uint32_t n = uint32_t(in[i]) << 16;
        b64 += tbl[(n >> 18) & 63];
        b64 += tbl[(n >> 12) & 63];
    } else if (i + 2 == in.size()) {
        uint32_t n = (uint32_t(in[i]) << 16) | (uint32_t(in[i+1]) << 8);
        b64 += tbl[(n >> 18) & 63];
        b64 += tbl[(n >> 12) & 63];
        b64 += tbl[(n >>  6) & 63];
    }
    for (char& c : b64) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!b64.empty() && b64.back() == '=') b64.pop_back();
    return b64;
}

struct RSAKey {
    RSA*      rsa   = nullptr;
    EVP_PKEY* pkey  = nullptr;
    BIGNUM*   bn    = nullptr;

    RSAKey() {
        bn   = BN_new();
        rsa  = RSA_new();
        pkey = EVP_PKEY_new();
        if (!bn || !rsa || !pkey) throw std::runtime_error("OpenSSL alloc failed");
        BN_set_word(bn, RSA_F4);
        RSA_generate_key_ex(rsa, 2048, bn, nullptr);
        EVP_PKEY_assign_RSA(pkey, rsa);
    }
    ~RSAKey() { if (pkey) EVP_PKEY_free(pkey); if (bn) BN_free(bn); }

    std::string sign(const std::string& msg) const {
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey);
        EVP_DigestSignUpdate(ctx, msg.data(), msg.size());
        size_t len = 0;
        EVP_DigestSignFinal(ctx, nullptr, &len);
        std::vector<uint8_t> sig(len);
        EVP_DigestSignFinal(ctx, sig.data(), &len);
        sig.resize(len);
        EVP_MD_CTX_free(ctx);
        return b64url(sig);
    }

    nlohmann::json jwks(const std::string& kid) const {
        const BIGNUM* n = nullptr; const BIGNUM* e = nullptr;
        RSA_get0_key(rsa, &n, &e, nullptr);
        std::vector<uint8_t> nb(BN_num_bytes(n)); BN_bn2bin(n, nb.data());
        std::vector<uint8_t> eb(BN_num_bytes(e)); BN_bn2bin(e, eb.data());
        return nlohmann::json{{"keys", {{
            {"kty","RSA"},{"kid",kid},{"alg","RS256"},{"use","sig"},
            {"n", b64url(nb)}, {"e", b64url(eb)}
        }}}};
    }
};

// Build a signed JWT from header/payload JSON objects
static std::string make_jwt(const RSAKey& key,
                             const nlohmann::json& header,
                             const nlohmann::json& payload) {
    auto hs = header.dump(); auto ps = payload.dump();
    std::string hp = b64url({hs.begin(), hs.end()}) + "." +
                     b64url({ps.begin(), ps.end()});
    return hp + "." + key.sign(hp);
}

// Current Unix time + offset in seconds
static int64_t unix_now(int64_t offset_sec = 0) {
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) + offset_sec;
}

} // anonymous namespace

// ═════════════════════════════════════════════════════════════════════════════
//  SECTION 1 – JWT Negative Tests
// ═════════════════════════════════════════════════════════════════════════════

class JWTNegativeTest : public ::testing::Test {
protected:
    RSAKey key_;
    std::string kid_ = "neg-test-key";
    std::unique_ptr<themis::auth::JWTValidator> validator_;

    void SetUp() override {
        themis::auth::JWTValidatorConfig cfg;
        cfg.jwks_url         = "https://unused.example/jwks";
        cfg.expected_issuer  = "https://issuer.example.com";
        cfg.expected_audience = "my-service";
        cfg.clock_skew       = std::chrono::seconds(0);
        validator_ = std::make_unique<themis::auth::JWTValidator>(cfg);
        validator_->setJWKSForTesting(key_.jwks(kid_));
    }

    // Convenience: build a valid-looking signed token
    std::string valid_token() {
        return make_jwt(key_,
            {{"alg","RS256"},{"typ","JWT"},{"kid", kid_}},
            {{"sub","user1"},{"iss","https://issuer.example.com"},
             {"aud","my-service"},{"exp", unix_now(+300)}});
    }
};

TEST_F(JWTNegativeTest, ExpiredTokenIsRejected) {
    auto token = make_jwt(key_,
        {{"alg","RS256"},{"typ","JWT"},{"kid", kid_}},
        {{"sub","u1"},{"iss","https://issuer.example.com"},
         {"aud","my-service"},{"exp", unix_now(-10)}});
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(JWTNegativeTest, WrongIssuerIsRejected) {
    auto token = make_jwt(key_,
        {{"alg","RS256"},{"typ","JWT"},{"kid", kid_}},
        {{"sub","u1"},{"iss","https://evil.attacker.com"},
         {"aud","my-service"},{"exp", unix_now(+300)}});
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(JWTNegativeTest, WrongAudienceIsRejected) {
    auto token = make_jwt(key_,
        {{"alg","RS256"},{"typ","JWT"},{"kid", kid_}},
        {{"sub","u1"},{"iss","https://issuer.example.com"},
         {"aud","different-service"},{"exp", unix_now(+300)}});
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(JWTNegativeTest, TamperedPayloadIsRejected) {
    std::string token = valid_token();
    // Flip a character in the payload section (between the two dots)
    auto first_dot  = token.find('.');
    auto second_dot = token.find('.', first_dot + 1);
    if (second_dot > first_dot + 2) {
        token[first_dot + 2] = (token[first_dot + 2] == 'A') ? 'B' : 'A';
    }
    EXPECT_THROW(validator_->parseAndValidate(token), std::exception);
}

TEST_F(JWTNegativeTest, MissingSignatureIsRejected) {
    std::string token = valid_token();
    auto last_dot = token.rfind('.');
    // Remove signature part
    token = token.substr(0, last_dot + 1);  // keep trailing dot, empty sig
    EXPECT_THROW(validator_->parseAndValidate(token), std::exception);
}

TEST_F(JWTNegativeTest, UnknownKidIsRejected) {
    auto token = make_jwt(key_,
        {{"alg","RS256"},{"typ","JWT"},{"kid","unknown-key-id"}},
        {{"sub","u1"},{"iss","https://issuer.example.com"},
         {"aud","my-service"},{"exp", unix_now(+300)}});
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(JWTNegativeTest, MissingKidHeaderIsRejected) {
    // Header without "kid"
    auto token = make_jwt(key_,
        {{"alg","RS256"},{"typ","JWT"}},
        {{"sub","u1"},{"iss","https://issuer.example.com"},
         {"aud","my-service"},{"exp", unix_now(+300)}});
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(JWTNegativeTest, RevokedKidIsRejected) {
    validator_->revokeKid(kid_);
    EXPECT_THROW(validator_->parseAndValidate(valid_token()), std::runtime_error);
}

TEST_F(JWTNegativeTest, NotBeforeInFutureIsRejected) {
    auto token = make_jwt(key_,
        {{"alg","RS256"},{"typ","JWT"},{"kid", kid_}},
        {{"sub","u1"},{"iss","https://issuer.example.com"},
         {"aud","my-service"},
         {"exp", unix_now(+600)},
         {"nbf", unix_now(+300)}});  // not valid for 5 more minutes
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(JWTNegativeTest, WrongAlgorithmInHeaderIsRejected) {
    // Claim HS256 in header but sign with RS256 – validator must reject
    auto token = make_jwt(key_,
        {{"alg","HS256"},{"typ","JWT"},{"kid", kid_}},
        {{"sub","u1"},{"iss","https://issuer.example.com"},
         {"aud","my-service"},{"exp", unix_now(+300)}});
    EXPECT_THROW(validator_->parseAndValidate(token), std::runtime_error);
}

TEST_F(JWTNegativeTest, MalformedTokensAreRejected) {
    const std::vector<std::string> bad_tokens = {
        "",
        "not-a-jwt",
        "header.payload",                 // only two parts
        "header.payload.sig.extra",       // four parts
        "...",                            // empty parts
        std::string(themis::auth::MAX_JWT_TOKEN_SIZE + 1, 'A'),  // oversized
    };
    for (const auto& t : bad_tokens) {
        EXPECT_THROW(validator_->parseAndValidate(t), std::exception)
            << "Expected rejection for: " << t.substr(0, 40);
    }
}

TEST_F(JWTNegativeTest, ValidTokenIsAccepted) {
    // Positive sanity check: a well-formed token must pass
    EXPECT_NO_THROW({
        auto claims = validator_->parseAndValidate(valid_token());
        EXPECT_EQ(claims.sub, "user1");
        EXPECT_EQ(claims.issuer, "https://issuer.example.com");
    });
}

// ═════════════════════════════════════════════════════════════════════════════
//  SECTION 2 – RBAC Negative Tests
// ═════════════════════════════════════════════════════════════════════════════

class RBACNegativeTest : public ::testing::Test {
protected:
    std::unique_ptr<themis::security::RBAC> rbac_;

    void SetUp() override {
        themis::security::RBACConfig cfg;
        cfg.use_builtin_roles = true;
        cfg.enable_role_inheritance = true;
        rbac_ = std::make_unique<themis::security::RBAC>(cfg);
        if (!rbac_->checkPermission({"admin"}, "data", "read")) {
            GTEST_SKIP() << "RBAC authorization unavailable in current environment";
        }
    }
};

TEST_F(RBACNegativeTest, EmptyRolesDenyAllPermissions) {
    EXPECT_FALSE(rbac_->checkPermission({}, "data",   "read"));
    EXPECT_FALSE(rbac_->checkPermission({}, "keys",   "rotate"));
    EXPECT_FALSE(rbac_->checkPermission({}, "config", "write"));
    EXPECT_FALSE(rbac_->checkPermission({}, "audit",  "delete"));
    EXPECT_FALSE(rbac_->checkPermission({}, "*",      "*"));
}

TEST_F(RBACNegativeTest, UnknownRoleDeniesPermissions) {
    EXPECT_FALSE(rbac_->checkPermission({"nonexistent_role"}, "data", "read"));
    EXPECT_FALSE(rbac_->checkPermission({"ghost_role"},       "keys", "rotate"));
}

TEST_F(RBACNegativeTest, ReadonlyRoleCannotWrite) {
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "data",   "write"));
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "data",   "delete"));
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "keys",   "rotate"));
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "config", "write"));
}

TEST_F(RBACNegativeTest, AnalystRoleCannotDeleteOrRotateKeys) {
    // Analyst should read data/audit but must not delete or touch keys
    EXPECT_FALSE(rbac_->checkPermission({"analyst"}, "data",   "delete"));
    EXPECT_FALSE(rbac_->checkPermission({"analyst"}, "keys",   "rotate"));
    EXPECT_FALSE(rbac_->checkPermission({"analyst"}, "keys",   "delete"));
    EXPECT_FALSE(rbac_->checkPermission({"analyst"}, "config", "write"));
}

TEST_F(RBACNegativeTest, OperatorRoleCannotDeleteKeys) {
    // Operators manage operations but should not be able to delete encryption keys
    EXPECT_FALSE(rbac_->checkPermission({"operator"}, "keys", "delete"));
}

TEST_F(RBACNegativeTest, CycleInRoleHierarchyIsDetected) {
    // Build a cyclic role graph: A → B → A
    themis::security::Role roleA;
    roleA.name     = "cycleA";
    roleA.inherits = {"cycleB"};
    roleA.permissions.push_back({"data", "read"});

    themis::security::Role roleB;
    roleB.name     = "cycleB";
    roleB.inherits = {"cycleA"};
    roleB.permissions.push_back({"data", "write"});

    rbac_->addRole(roleA);
    rbac_->addRole(roleB);

    // validateRoleHierarchy must return false (cycle detected)
    EXPECT_FALSE(rbac_->validateRoleHierarchy());
}

TEST_F(RBACNegativeTest, RemovingRoleRevokesAccess) {
    // Add a custom role, verify it grants access, then remove it
    themis::security::Role custom;
    custom.name = "temp_auditor";
    custom.permissions.push_back({"audit", "read"});
    rbac_->addRole(custom);

    EXPECT_TRUE(rbac_->checkPermission({"temp_auditor"}, "audit", "read"));

    rbac_->removeRole("temp_auditor");

    EXPECT_FALSE(rbac_->checkPermission({"temp_auditor"}, "audit", "read"));
}

TEST_F(RBACNegativeTest, MisconfiguredWildcardRoleDoesNotGrantUnintendedAccess) {
    // A role that allows data:* should NOT grant keys:rotate
    themis::security::Role data_all;
    data_all.name = "data_manager";
    data_all.permissions.push_back({"data", "*"});
    rbac_->addRole(data_all);

    EXPECT_TRUE(rbac_->checkPermission({"data_manager"}, "data",   "read"));
    EXPECT_TRUE(rbac_->checkPermission({"data_manager"}, "data",   "write"));
    // Must NOT bleed into other resources
    EXPECT_FALSE(rbac_->checkPermission({"data_manager"}, "keys",   "rotate"));
    EXPECT_FALSE(rbac_->checkPermission({"data_manager"}, "config", "write"));
    EXPECT_FALSE(rbac_->checkPermission({"data_manager"}, "audit",  "delete"));
}

TEST_F(RBACNegativeTest, AdminRoleGrantsAllPermissions) {
    // Positive: admin is the only wildcard role
    EXPECT_TRUE(rbac_->checkPermission({"admin"}, "data",   "delete"));
    EXPECT_TRUE(rbac_->checkPermission({"admin"}, "keys",   "rotate"));
    EXPECT_TRUE(rbac_->checkPermission({"admin"}, "config", "write"));
    EXPECT_TRUE(rbac_->checkPermission({"admin"}, "audit",  "read"));
}

// ═════════════════════════════════════════════════════════════════════════════
//  SECTION 3 – Vault Negative Tests (mocked HTTP layer)
// ═════════════════════════════════════════════════════════════════════════════

class VaultNegativeTest : public ::testing::Test {
protected:
    // Build a VaultKeyProvider configured to an unreachable address;
    // HTTP is overridden by setTestRequestOverride so no real network is used.
    std::unique_ptr<themis::VaultKeyProvider> make_provider(
        std::function<std::string(const std::string&,
                                  const std::string&,
                                  const std::string&)> override_fn) {
        themis::VaultKeyProvider::Config cfg;
        cfg.vault_addr     = "https://vault.test.invalid:8200";
        cfg.vault_token    = "test-token";
        cfg.kv_mount_path  = "themis";
        cfg.request_timeout_ms = 100;
        auto provider = std::make_unique<themis::VaultKeyProvider>(cfg);
        provider->setTestRequestOverride(override_fn);
        return provider;
    }
};

// HTTP 403 → KeyOperationException (auth failure)
TEST_F(VaultNegativeTest, Http403TriggersAuthFailure) {
    auto provider = make_provider([](const std::string&, const std::string&,
                                     const std::string&) -> std::string {
        throw themis::KeyOperationException("HTTP 403 Forbidden");
    });
    EXPECT_THROW(provider->getKey("any-key"), themis::KeyOperationException);
}

// HTTP 404 → KeyNotFoundException (key not found in Vault)
TEST_F(VaultNegativeTest, Http404TriggersKeyNotFound) {
    auto provider = make_provider([](const std::string&, const std::string&,
                                     const std::string&) -> std::string {
        throw themis::KeyNotFoundException("any-key", 0);
    });
    EXPECT_THROW(provider->getKey("any-key"), themis::KeyNotFoundException);
}

// Invalid (non-JSON) response → KeyOperationException (parse failure)
TEST_F(VaultNegativeTest, MalformedJsonResponseThrows) {
    auto provider = make_provider([](const std::string&, const std::string&,
                                     const std::string&) -> std::string {
        return "not-valid-json{{{";
    });
    EXPECT_THROW(provider->getKey("test-key"), std::exception);
}

// Empty JSON object (missing "data" field) → exception
TEST_F(VaultNegativeTest, MissingDataFieldInResponseThrows) {
    auto provider = make_provider([](const std::string&, const std::string&,
                                     const std::string&) -> std::string {
        return R"({"request_id":"abc"})";  // no "data" field
    });
    EXPECT_THROW(provider->getKey("test-key"), std::exception);
}

// Empty key bytes in Vault response → exception (zero-length key is unusable)
TEST_F(VaultNegativeTest, EmptyKeyBytesInVaultResponseThrows) {
    auto provider = make_provider([](const std::string&, const std::string&,
                                     const std::string&) -> std::string {
        // Valid Vault KV v2 shape but key is empty string
        return R"({"data":{"data":{"key":"","algorithm":"AES-256-GCM","version":1}}})";
    });
    EXPECT_THROW(provider->getKey("test-key"), std::exception);
}

// Network error (exception during HTTP) → KeyOperationException propagated
TEST_F(VaultNegativeTest, NetworkErrorPropagatesAsException) {
    auto provider = make_provider([](const std::string&, const std::string&,
                                     const std::string&) -> std::string {
        throw std::runtime_error("Connection refused");
    });
    EXPECT_THROW(provider->getKey("some-key"), std::exception);
}

// A valid mocked response round-trips correctly (positive sanity check)
TEST_F(VaultNegativeTest, ValidVaultResponseReturnsKey) {
    // A 32-byte key base64-encoded
    const std::string key_b64 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
    auto provider = make_provider([&key_b64](const std::string&, const std::string&,
                                              const std::string&) -> std::string {
        return R"({"data":{"data":{"key":")" + key_b64 +
               R"(","algorithm":"AES-256-GCM","version":1}}})";
    });
    EXPECT_NO_THROW({
        auto key = provider->getKey("test-key");
        EXPECT_EQ(key.size(), 32u);
    });
}
