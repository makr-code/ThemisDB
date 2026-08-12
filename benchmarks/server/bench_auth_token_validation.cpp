/**
 * @file bench_auth_token_validation.cpp
 * @brief Performance benchmarks for auth token validation latency (Issue #1551).
 *
 * Measures end-to-end latency for the following operations:
 *   1. JWT RS256 validation with cached JWKS (happy path)
 *   2. JWT validation with JTI blacklist lookup (revocation check)
 *   3. JWT validation failure – expired token
 *   4. JWT validation failure – wrong issuer
 *   5. TokenBlacklist::isRevoked – JTI present (revoked token)
 *   6. TokenBlacklist::isRevoked – JTI absent (clean token)
 *   7. TOTP validateTOTP computation
 *   8. AuthMiddleware static API token lookup (single token)
 *   9. AuthMiddleware static API token lookup (1 000-token map)
 *
 * Latency targets (from include/auth/README.md):
 *   - JWT validation (cached JWKS): < 5 ms
 *   - TOTP validation:              < 10 ms
 *
 * Build:
 *   cmake -DTHEMIS_BUILD_BENCHMARKS=ON ... && cmake --build . --target bench_auth_token_validation
 * Run:
 *   ./benchmarks/bench_auth_token_validation --benchmark_format=json
 */

#include <benchmark/benchmark.h>

#include "auth/jwt_validator.h"
#include "auth/token_blacklist.h"
#include "auth/mfa_authenticator.h"
#include "server/auth_middleware.h"

#include <nlohmann/json.hpp>

#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/bn.h>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::auth;

// ============================================================================
// Shared test-token helpers (same approach as test_jwt_validator.cpp)
// ============================================================================

namespace {

// Base64url encode without padding
static std::string b64url(const std::vector<uint8_t>& in) {
    static const char* tbl =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((in.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= in.size()) {
        uint32_t n = (in[i] << 16) | (in[i+1] << 8) | in[i+2];
        out += tbl[(n >> 18) & 63];
        out += tbl[(n >> 12) & 63];
        out += tbl[(n >> 6)  & 63];
        out += tbl[ n        & 63];
        i += 3;
    }
    if (i + 1 == in.size()) {
        uint32_t n = in[i] << 16;
        out += tbl[(n >> 18) & 63];
        out += tbl[(n >> 12) & 63];
        out += '='; out += '=';
    } else if (i + 2 == in.size()) {
        uint32_t n = (in[i] << 16) | (in[i+1] << 8);
        out += tbl[(n >> 18) & 63];
        out += tbl[(n >> 12) & 63];
        out += tbl[(n >> 6)  & 63];
        out += '=';
    }
    for (char& c : out) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!out.empty() && out.back() == '=') out.pop_back();
    return out;
}

// RAII RSA 2048-bit key pair
struct RSAKeyPair {
    EVP_PKEY* pkey = nullptr;
    RSA*      rsa  = nullptr;

    RSAKeyPair() {
        BIGNUM* bn = BN_new();
        if (!bn) throw std::runtime_error("BN_new failed");
        BN_set_word(bn, RSA_F4);
        rsa = RSA_new();
        if (!rsa) { BN_free(bn); throw std::runtime_error("RSA_new failed"); }
        RSA_generate_key_ex(rsa, 2048, bn, nullptr);
        BN_free(bn);
        pkey = EVP_PKEY_new();
        if (!pkey) { RSA_free(rsa); rsa = nullptr; throw std::runtime_error("EVP_PKEY_new failed"); }
        EVP_PKEY_assign_RSA(pkey, rsa); // pkey takes ownership of rsa
    }
    ~RSAKeyPair() {
        if (pkey) EVP_PKEY_free(pkey); // also frees rsa
    }
    RSAKeyPair(const RSAKeyPair&) = delete;
    RSAKeyPair& operator=(const RSAKeyPair&) = delete;
};

// Sign header.payload with RS256
static std::string signRS256(EVP_PKEY* pkey, const std::string& header_payload) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_MD_CTX_new failed");
    EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey);
    EVP_DigestSignUpdate(ctx, header_payload.data(), header_payload.size());
    size_t siglen = 0;
    EVP_DigestSignFinal(ctx, nullptr, &siglen);
    std::vector<uint8_t> sig(siglen);
    EVP_DigestSignFinal(ctx, sig.data(), &siglen);
    sig.resize(siglen);
    EVP_MD_CTX_free(ctx);
    return b64url(sig);
}

// Build JWKS from an RSA public key
static nlohmann::json makeJWKS(RSA* rsa) {
    const BIGNUM* n; const BIGNUM* e;
    RSA_get0_key(rsa, &n, &e, nullptr);
    std::vector<uint8_t> nb(BN_num_bytes(n)), eb(BN_num_bytes(e));
    BN_bn2bin(n, nb.data()); BN_bn2bin(e, eb.data());
    nlohmann::json jwk = {
        {"kty","RSA"}, {"kid","bench-key-1"}, {"alg","RS256"}, {"use","sig"},
        {"n", b64url(nb)}, {"e", b64url(eb)}
    };
    return nlohmann::json{{"keys", nlohmann::json::array({jwk})}};
}

// Encode string as base64url
static std::string encodeStr(const std::string& s) {
    return b64url(std::vector<uint8_t>(s.begin(), s.end()));
}

// Build a signed JWT
static std::string buildToken(EVP_PKEY* pkey,
                               const std::string& kid,
                               const nlohmann::json& payload) {
    std::string header_str = nlohmann::json{{"alg","RS256"},{"typ","JWT"},{"kid",kid}}.dump();
    std::string payload_str = payload.dump();
    std::string hp = encodeStr(header_str) + "." + encodeStr(payload_str);
    return hp + "." + signRS256(pkey, hp);
}

// Return seconds-since-epoch N seconds in the future
static int64_t futureEpoch(int seconds = 300) {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() + seconds;
}

// ============================================================================
// Shared fixture initialised once per process
// ============================================================================
struct BenchFixture {
    RSAKeyPair             keys;
    nlohmann::json         jwks;

    // Valid token (cached in JWTValidator)
    std::string            valid_token;

    // Token with JTI for revocation tests
    std::string            token_with_jti;
    std::string            jti_value = "bench-jti-001";

    // Expired token
    std::string            expired_token;

    // Wrong-issuer token
    std::string            wrong_issuer_token;

    BenchFixture()
        : jwks(makeJWKS(keys.rsa))
    {
        // Valid token
        nlohmann::json p = {
            {"sub","bench-user"}, {"email","bench@example.com"},
            {"iss","bench-issuer"}, {"aud","bench-audience"},
            {"exp", futureEpoch(3600)}
        };
        valid_token = buildToken(keys.pkey, "bench-key-1", p);

        // Token with JTI
        nlohmann::json p_jti = {
            {"sub","bench-user"}, {"email","bench@example.com"},
            {"iss","bench-issuer"}, {"aud","bench-audience"},
            {"exp", futureEpoch(3600)}, {"jti", jti_value}
        };
        token_with_jti = buildToken(keys.pkey, "bench-key-1", p_jti);

        // Expired token (exp in the past, zero clock_skew)
        nlohmann::json p_exp = {
            {"sub","bench-user"}, {"email","bench@example.com"},
            {"iss","bench-issuer"}, {"aud","bench-audience"},
            {"exp", futureEpoch(-60)}
        };
        expired_token = buildToken(keys.pkey, "bench-key-1", p_exp);

        // Wrong-issuer token
        nlohmann::json p_iss = {
            {"sub","bench-user"}, {"email","bench@example.com"},
            {"iss","wrong-issuer"}, {"aud","bench-audience"},
            {"exp", futureEpoch(3600)}
        };
        wrong_issuer_token = buildToken(keys.pkey, "bench-key-1", p_iss);
    }

    // Create a validator with JWKS pre-loaded (no network calls)
    std::unique_ptr<JWTValidator> makeValidator(
        std::chrono::seconds clock_skew = std::chrono::seconds(60)) const
    {
        JWTValidatorConfig cfg;
        cfg.jwks_url         = "";
        cfg.expected_issuer  = "bench-issuer";
        cfg.expected_audience = "bench-audience";
        cfg.cache_ttl        = std::chrono::seconds(3600);
        cfg.clock_skew       = clock_skew;
        auto v = std::make_unique<JWTValidator>(cfg);
        v->setJWKSForTesting(jwks);
        return v;
    }
};

// Single global fixture – initialised lazily on first use
static const BenchFixture& fixture() {
    static BenchFixture f;
    return f;
}

} // namespace

// ============================================================================
// 1. JWT RS256 validation – happy path (cached JWKS, no network)
// ============================================================================

static void BM_JWT_ValidToken_RS256(benchmark::State& state) {
    const auto& fx = fixture();
    auto validator = fx.makeValidator();

    for (auto _ : state) {
        auto claims = validator->parseAndValidate(fx.valid_token);
        benchmark::DoNotOptimize(claims);
    }
    state.SetLabel("cached JWKS, RS256, happy path");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_JWT_ValidToken_RS256);

// ============================================================================
// 2. JWT validation with JTI blacklist (revocation check on clean token)
// ============================================================================

static void BM_JWT_ValidToken_WithBlacklist(benchmark::State& state) {
    const auto& fx = fixture();
    // Declare blacklist before validator so it outlives the validator (LIFO destruction).
    TokenBlacklist blacklist;
    auto validator = fx.makeValidator();
    validator->setTokenBlacklist(&blacklist);

    for (auto _ : state) {
        auto claims = validator->parseAndValidate(fx.token_with_jti);
        benchmark::DoNotOptimize(claims);
    }
    state.SetLabel("cached JWKS, RS256, with blacklist (not revoked)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_JWT_ValidToken_WithBlacklist);

// ============================================================================
// 3. JWT validation – expired token (early rejection)
// ============================================================================

static void BM_JWT_ExpiredToken(benchmark::State& state) {
    const auto& fx = fixture();
    auto validator = fx.makeValidator(std::chrono::seconds(0)); // no clock skew

    for (auto _ : state) {
        try {
            auto claims = validator->parseAndValidate(fx.expired_token);
            benchmark::DoNotOptimize(claims);
        } catch (const std::exception&) {
            // expected
        }
    }
    state.SetLabel("expired token – rejection path");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_JWT_ExpiredToken);

// ============================================================================
// 4. JWT validation – wrong issuer (rejection after signature verify)
// ============================================================================

static void BM_JWT_WrongIssuer(benchmark::State& state) {
    const auto& fx = fixture();
    auto validator = fx.makeValidator();

    for (auto _ : state) {
        try {
            auto claims = validator->parseAndValidate(fx.wrong_issuer_token);
            benchmark::DoNotOptimize(claims);
        } catch (const std::exception&) {
            // expected
        }
    }
    state.SetLabel("wrong issuer – rejection after sig verification");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_JWT_WrongIssuer);

// ============================================================================
// 5. TokenBlacklist::isRevoked – JTI present (revoked)
// ============================================================================

static void BM_TokenBlacklist_IsRevoked_Hit(benchmark::State& state) {
    TokenBlacklist bl;
    const std::string jti = "revoked-jti-bench";
    auto expiry = std::chrono::system_clock::now() + std::chrono::hours(1);
    bl.revoke(jti, expiry);

    for (auto _ : state) {
        bool revoked = bl.isRevoked(jti);
        benchmark::DoNotOptimize(revoked);
    }
    state.SetLabel("isRevoked – JTI in blacklist");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TokenBlacklist_IsRevoked_Hit);

// ============================================================================
// 6. TokenBlacklist::isRevoked – JTI absent (clean token)
// ============================================================================

static void BM_TokenBlacklist_IsRevoked_Miss(benchmark::State& state) {
    TokenBlacklist bl;
    // Pre-populate with unrelated entries so the map is non-trivial
    auto expiry = std::chrono::system_clock::now() + std::chrono::hours(1);
    for (int i = 0; i < 100; ++i) {
        bl.revoke("jti-" + std::to_string(i), expiry);
    }
    const std::string absent_jti = "clean-token-jti";

    for (auto _ : state) {
        bool revoked = bl.isRevoked(absent_jti);
        benchmark::DoNotOptimize(revoked);
    }
    state.SetLabel("isRevoked – JTI NOT in blacklist (100 entries)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TokenBlacklist_IsRevoked_Miss);

// ============================================================================
// 7. TOTP validateTOTP computation
// ============================================================================

static void BM_TOTP_Validate(benchmark::State& state) {
    MFAAuthenticator mfa;
    auto enrollment = mfa.generateEnrollment("bench-user");
    const auto& secret = enrollment.secret_base32;
    auto ts = std::chrono::system_clock::now();

    // Benchmark the rejection path: validateTOTP exercises the same HMAC-SHA1
    // computation regardless of whether the code matches – only the final
    // integer comparison differs.  The setup cost of generating the enrollment
    // is incurred once, outside the benchmark loop.
    const std::string code = "000000";

    for (auto _ : state) {
        bool ok = mfa.validateTOTP(secret, code, ts);
        benchmark::DoNotOptimize(ok);
    }
    state.SetLabel("TOTP HMAC-SHA1 compute + time-window check");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TOTP_Validate);

// ============================================================================
// 8. AuthMiddleware static API token – single token registered
// ============================================================================

static void BM_AuthMiddleware_StaticToken_Single(benchmark::State& state) {
    AuthMiddleware auth;
    auth.addToken(AuthMiddleware::TokenConfig{
        "bench-api-token-001", "bench-user", "tenant-1", {"data:read"}
    });
    const std::string token = "bench-api-token-001";

    for (auto _ : state) {
        auto result = auth.validateToken(token);
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("static API token lookup – 1 token in map");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AuthMiddleware_StaticToken_Single);

// ============================================================================
// 9. AuthMiddleware static API token – 1 000-token map
// ============================================================================

static void BM_AuthMiddleware_StaticToken_1000(benchmark::State& state) {
    AuthMiddleware auth;
    for (int i = 0; i < 1000; ++i) {
        auth.addToken(AuthMiddleware::TokenConfig{
            "bench-api-token-" + std::to_string(i),
            "user-" + std::to_string(i),
            "tenant-1",
            {"data:read"}
        });
    }
    const std::string token = "bench-api-token-500"; // middle of the map

    for (auto _ : state) {
        auto result = auth.validateToken(token);
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("static API token lookup – 1000 tokens in map");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AuthMiddleware_StaticToken_1000);
