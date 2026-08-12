#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include <thread>
#include <atomic>
#include "auth/jwt_validator.h"
#include "auth/token_blacklist.h"

using namespace themis::auth;

// Helper: base64url encode
static std::string b64url(const std::vector<uint8_t>& in) {
    static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64; b64.reserve(((in.size()+2)/3)*4);
    size_t i=0; while(i+3<=in.size()) { uint32_t n=(in[i]<<16)|(in[i+1]<<8)|in[i+2]; b64.push_back(tbl[(n>>18)&63]); b64.push_back(tbl[(n>>12)&63]); b64.push_back(tbl[(n>>6)&63]); b64.push_back(tbl[n&63]); i+=3; }
    if(i+1==in.size()){ uint32_t n=(in[i]<<16); b64.push_back(tbl[(n>>18)&63]); b64.push_back(tbl[(n>>12)&63]); b64.push_back('='); b64.push_back('='); }
    else if(i+2==in.size()){ uint32_t n=(in[i]<<16)|(in[i+1]<<8); b64.push_back(tbl[(n>>18)&63]); b64.push_back(tbl[(n>>12)&63]); b64.push_back(tbl[(n>>6)&63]); b64.push_back('='); }
    // convert to url form
    for(char& c: b64){ if(c=='+') c='-'; else if(c=='/') c='_'; }
    while(!b64.empty() && b64.back()=='=') b64.pop_back();
    return b64;
}

struct RSAFixture {
    RSA* rsa=nullptr; EVP_PKEY* pkey=nullptr; BIGNUM* bn=nullptr; 
    RSAFixture(){ 
        bn=BN_new(); 
        if (!bn) throw std::runtime_error("BN_new failed");
        if (BN_set_word(bn, RSA_F4) != 1) throw std::runtime_error("BN_set_word failed");
        rsa=RSA_new(); 
        if (!rsa) throw std::runtime_error("RSA_new failed");
        if (RSA_generate_key_ex(rsa, 2048, bn, nullptr) != 1) {
            throw std::runtime_error("RSA_generate_key_ex failed");
        }
        pkey=EVP_PKEY_new(); 
        if (!pkey) throw std::runtime_error("EVP_PKEY_new failed");
        if (EVP_PKEY_assign_RSA(pkey, rsa) != 1) {
            throw std::runtime_error("EVP_PKEY_assign_RSA failed");
        }
    }
    ~RSAFixture(){ if(pkey) EVP_PKEY_free(pkey); if(bn) BN_free(bn); /* rsa freed by pkey */ }
};

// Helper: sign using EVP_DigestSign
static std::string sign_RS256(EVP_PKEY* pkey, const std::string& header_payload){ 
    EVP_MD_CTX* mctx = EVP_MD_CTX_new(); 
    if(!mctx) throw std::runtime_error("EVP_MD_CTX_new failed"); 
    size_t siglen=0; 
    if(EVP_DigestSignInit(mctx,nullptr,EVP_sha256(),nullptr,pkey)<=0) throw std::runtime_error("EVP_DigestSignInit failed"); 
    if(EVP_DigestSignUpdate(mctx, header_payload.data(), header_payload.size())<=0) throw std::runtime_error("EVP_DigestSignUpdate failed"); 
    if(EVP_DigestSignFinal(mctx,nullptr,&siglen)<=0) throw std::runtime_error("EVP_DigestSignFinal (query) failed"); 
    std::vector<uint8_t> sig(siglen); 
    size_t siglen2 = siglen;
    if(EVP_DigestSignFinal(mctx,sig.data(),&siglen2)<=0) throw std::runtime_error("EVP_DigestSignFinal failed"); 
    sig.resize(siglen2); 
    EVP_MD_CTX_free(mctx); 
    return b64url(sig); 
}

static nlohmann::json make_jwks(RSA* rsa){ const BIGNUM* n; const BIGNUM* e; RSA_get0_key(rsa,&n,&e,nullptr); std::vector<uint8_t> n_bytes(BN_num_bytes(n)); BN_bn2bin(n,n_bytes.data()); std::vector<uint8_t> e_bytes(BN_num_bytes(e)); BN_bn2bin(e,e_bytes.data()); nlohmann::json jwk={ {"kty","RSA"},{"kid","test-key-1"},{"alg","RS256"},{"use","sig"},{"n", b64url(n_bytes)},{"e", b64url(e_bytes)} }; return nlohmann::json{{"keys", nlohmann::json::array({jwk})}}; }

static std::string build_token(const std::string& kid, const nlohmann::json& payload){ 
    nlohmann::json header={{"alg","RS256"},{"typ","JWT"},{"kid",kid}}; 
    std::string header_str = header.dump();
    std::string payload_str = payload.dump();
    std::vector<uint8_t> h_vec(header_str.begin(), header_str.end());
    std::vector<uint8_t> p_vec(payload_str.begin(), payload_str.end());
    std::string h=b64url(h_vec); 
    std::string p=b64url(p_vec); 
    return h+"."+p; 
}

TEST(JWTValidatorTest, ValidToken) {
    RSAFixture fix; auto jwks = make_jwks(fix.rsa);
    JWTValidator validator(JWTValidatorConfig{"", "issuerX", "audX", std::chrono::seconds(600), std::chrono::seconds(60)});
    validator.setJWKSForTesting(jwks);
    auto now = std::chrono::system_clock::now(); auto exp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()+300;
    nlohmann::json payload={{"sub","u1"},{"email","u1@x"},{"iss","issuerX"},{"aud","audX"},{"exp",exp}};
    std::string unsigned_token = build_token("test-key-1", payload);
    std::string sig = sign_RS256(fix.pkey, unsigned_token);
    std::string token = unsigned_token+"."+sig;
    auto claims = validator.parseAndValidate(token);
    EXPECT_EQ(claims.sub, "u1");
    EXPECT_EQ(claims.issuer, "issuerX");
}

TEST(JWTValidatorTest, ExpiredToken) {
    RSAFixture fix; auto jwks = make_jwks(fix.rsa); JWTValidator validator(JWTValidatorConfig{"", "issuerX", "audX", std::chrono::seconds(600), std::chrono::seconds(0)}); validator.setJWKSForTesting(jwks); auto now = std::chrono::system_clock::now(); auto exp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()-10; nlohmann::json payload={{"sub","u1"},{"email","e"},{"iss","issuerX"},{"aud","audX"},{"exp",exp}}; std::string up=build_token("test-key-1",payload); std::string token=up+"."+sign_RS256(fix.pkey, up); EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error); }

TEST(JWTValidatorTest, IssuerMismatch) {
    RSAFixture fix; auto jwks=make_jwks(fix.rsa); JWTValidator validator(JWTValidatorConfig{"", "issuerX", "audX", std::chrono::seconds(600), std::chrono::seconds(60)}); validator.setJWKSForTesting(jwks); auto now=std::chrono::system_clock::now(); auto exp=std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()+100; nlohmann::json payload={{"sub","u1"},{"email","e"},{"iss","wrong"},{"aud","audX"},{"exp",exp}}; std::string up=build_token("test-key-1",payload); std::string token=up+"."+sign_RS256(fix.pkey, up); EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error); }

TEST(JWTValidatorTest, AudienceMismatch) {
    RSAFixture fix; auto jwks=make_jwks(fix.rsa); JWTValidator validator(JWTValidatorConfig{"", "issuerX", "audExpected", std::chrono::seconds(600), std::chrono::seconds(60)}); validator.setJWKSForTesting(jwks); auto now=std::chrono::system_clock::now(); auto exp=std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()+100; nlohmann::json payload={{"sub","u1"},{"email","e"},{"iss","issuerX"},{"aud","otherAud"},{"exp",exp}}; std::string up=build_token("test-key-1",payload); std::string token=up+"."+sign_RS256(fix.pkey, up); EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error); }

TEST(JWTValidatorTest, NotYetValidNbf) {
    RSAFixture fix; auto jwks=make_jwks(fix.rsa); JWTValidator validator(JWTValidatorConfig{"", "issuerX", "audX", std::chrono::seconds(600), std::chrono::seconds(0)}); validator.setJWKSForTesting(jwks); auto now=std::chrono::system_clock::now(); auto exp=std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()+300; auto nbf=exp; nlohmann::json payload={{"sub","u1"},{"email","e"},{"iss","issuerX"},{"aud","audX"},{"exp",exp},{"nbf",nbf}}; std::string up=build_token("test-key-1",payload); std::string token=up+"."+sign_RS256(fix.pkey, up); EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error); }

TEST(JWTValidatorTest, TamperedPayloadSignatureFails) {
    RSAFixture fix; auto jwks=make_jwks(fix.rsa); JWTValidator validator(JWTValidatorConfig{"", "issuerX", "audX", std::chrono::seconds(600), std::chrono::seconds(60)}); validator.setJWKSForTesting(jwks); auto now=std::chrono::system_clock::now(); auto exp=std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()+100; nlohmann::json payload={{"sub","u1"},{"email","e"},{"iss","issuerX"},{"aud","audX"},{"exp",exp}}; std::string up=build_token("test-key-1",payload); std::string sig=sign_RS256(fix.pkey, up); // tamper payload
    std::string tampered = up; tampered[up.find('.')+5] = (tampered[up.find('.')+5] == 'A' ? 'B' : 'A'); std::string token = tampered + "." + sig; 
    // Tampering should cause either parse error or signature validation failure
    EXPECT_THROW(validator.parseAndValidate(token), std::exception); 
}

TEST(JWTValidatorTest, MissingKidThrows) {
    RSAFixture fix; auto jwks = make_jwks(fix.rsa);
    JWTValidator validator(JWTValidatorConfig{"", "issuerX", "audX", std::chrono::seconds(600), std::chrono::seconds(60)});
    validator.setJWKSForTesting(jwks);
    // Build a token with header that lacks kid
    nlohmann::json header = {{"alg","RS256"},{"typ","JWT"}};
    nlohmann::json payload = {{"sub","uX"},{"iss","issuerX"},{"aud","audX"},{"exp", std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()+120}};
    std::string header_str = header.dump(); std::string payload_str = payload.dump();
    std::string unsigned_token = b64url(std::vector<uint8_t>(header_str.begin(), header_str.end())) + "." + b64url(std::vector<uint8_t>(payload_str.begin(), payload_str.end()));
    std::string token = unsigned_token + "." + sign_RS256(fix.pkey, unsigned_token);
    EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error);
}

TEST(JWTValidatorTest, HasAccess_SubjectMatch) {
    JWTClaims claims;
    claims.sub = "user123";
    claims.groups = {"group-a", "group-b"};
    EXPECT_TRUE(JWTValidator::hasAccess(claims, "user123"));
}

TEST(JWTValidatorTest, HasAccess_GroupMatch) {
    JWTClaims claims;
    claims.sub = "user123";
    claims.groups = {"group-a", "group-b"};
    EXPECT_TRUE(JWTValidator::hasAccess(claims, "group-a"));
    EXPECT_TRUE(JWTValidator::hasAccess(claims, "group-b"));
}

TEST(JWTValidatorTest, HasAccess_NoMatch) {
    JWTClaims claims;
    claims.sub = "user123";
    claims.groups = {"group-a"};
    EXPECT_FALSE(JWTValidator::hasAccess(claims, "other-user"));
    EXPECT_FALSE(JWTValidator::hasAccess(claims, "group-x"));
}

TEST(JWTValidatorTest, DeriveUserKey_DifferentFieldsProduceDifferentKeys) {
    JWTClaims claims;
    claims.sub = "user123";
    std::vector<uint8_t> dek(32, 0x42);
    auto key1 = JWTValidator::deriveUserKey(dek, claims, "field-email");
    auto key2 = JWTValidator::deriveUserKey(dek, claims, "field-phone");
    EXPECT_EQ(key1.size(), 32u);
    EXPECT_EQ(key2.size(), 32u);
    EXPECT_NE(key1, key2);
}

TEST(JWTValidatorTest, DeriveUserKey_DifferentSubjectsProduceDifferentKeys) {
    JWTClaims claims1; claims1.sub = "user-alice";
    JWTClaims claims2; claims2.sub = "user-bob";
    std::vector<uint8_t> dek(32, 0x42);
    auto key1 = JWTValidator::deriveUserKey(dek, claims1, "field-ssn");
    auto key2 = JWTValidator::deriveUserKey(dek, claims2, "field-ssn");
    EXPECT_NE(key1, key2);
}

TEST(JWTValidatorTest, DeriveUserKey_Deterministic) {
    JWTClaims claims; claims.sub = "user123";
    std::vector<uint8_t> dek(32, 0x11);
    auto key1 = JWTValidator::deriveUserKey(dek, claims, "field-a");
    auto key2 = JWTValidator::deriveUserKey(dek, claims, "field-a");
    EXPECT_EQ(key1, key2);
}

TEST(JWTValidatorTest, TokenBlacklist_RevokedJtiRejected) {
    RSAFixture fix; auto jwks = make_jwks(fix.rsa);
    JWTValidator validator(JWTValidatorConfig{"", "issuerX", "audX", std::chrono::seconds(600), std::chrono::seconds(60)});
    validator.setJWKSForTesting(jwks);

    TokenBlacklist blacklist;
    validator.setTokenBlacklist(&blacklist);

    auto now = std::chrono::system_clock::now();
    auto exp_ts = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() + 300;
    nlohmann::json payload = {{"sub","u1"},{"email","u1@x"},{"iss","issuerX"},{"aud","audX"},
                               {"exp", exp_ts}, {"jti", "revoked-jti-001"}};
    std::string up = build_token("test-key-1", payload);
    std::string token = up + "." + sign_RS256(fix.pkey, up);

    // Revoke the JTI before validating
    auto exp_tp = std::chrono::system_clock::time_point(std::chrono::seconds(exp_ts));
    blacklist.revoke("revoked-jti-001", exp_tp);

    EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error);
}

TEST(JWTValidatorTest, TokenBlacklist_NonRevokedJtiAccepted) {
    RSAFixture fix; auto jwks = make_jwks(fix.rsa);
    JWTValidator validator(JWTValidatorConfig{"", "issuerX", "audX", std::chrono::seconds(600), std::chrono::seconds(60)});
    validator.setJWKSForTesting(jwks);

    TokenBlacklist blacklist;
    validator.setTokenBlacklist(&blacklist);

    auto now = std::chrono::system_clock::now();
    auto exp_ts = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() + 300;
    nlohmann::json payload = {{"sub","u2"},{"email","u2@x"},{"iss","issuerX"},{"aud","audX"},
                               {"exp", exp_ts}, {"jti", "valid-jti-002"}};
    std::string up = build_token("test-key-1", payload);
    std::string token = up + "." + sign_RS256(fix.pkey, up);

    // Revoke a *different* JTI
    blacklist.revoke("other-jti", std::chrono::system_clock::now() + std::chrono::hours(1));

    auto claims = validator.parseAndValidate(token);
    EXPECT_EQ(claims.sub, "u2");
    EXPECT_EQ(claims.jti, "valid-jti-002");
}

// --- Mandatory issuer/audience validation tests ---

TEST(JWTValidatorTest, ConstructorThrows_RequireIssuerValidation_NoIssuerSet) {
    JWTValidatorConfig cfg;
    cfg.jwks_url = "";
    cfg.expected_audience = "audX";
    cfg.require_issuer_validation = true;
    // expected_issuer is nullopt → must throw
    EXPECT_THROW(JWTValidator{cfg}, std::runtime_error);
}

TEST(JWTValidatorTest, ConstructorThrows_RequireAudienceValidation_NoAudienceSet) {
    JWTValidatorConfig cfg;
    cfg.jwks_url = "";
    cfg.expected_issuer = "issuerX";
    cfg.require_audience_validation = true;
    // expected_audience is nullopt → must throw
    EXPECT_THROW(JWTValidator{cfg}, std::runtime_error);
}

TEST(JWTValidatorTest, ConstructorSucceeds_RequireFlags_False_NoValues) {
    JWTValidatorConfig cfg;
    cfg.jwks_url = "";
    cfg.require_issuer_validation = false;
    cfg.require_audience_validation = false;
    // No expected_issuer/audience set, but require flags are false → no throw
    EXPECT_NO_THROW(JWTValidator{cfg});
}

TEST(JWTValidatorTest, ConstructorSucceeds_BothSet) {
    JWTValidatorConfig cfg;
    cfg.jwks_url = "";
    cfg.expected_issuer = "issuerX";
    cfg.expected_audience = "audX";
    EXPECT_NO_THROW(JWTValidator{cfg});
}

TEST(JWTValidatorTest, MissingIssClaim_WhenIssuerValidationEnabled) {
    RSAFixture fix; auto jwks = make_jwks(fix.rsa);
    JWTValidatorConfig cfg;
    cfg.expected_issuer = "issuerX";
    cfg.expected_audience = "audX";
    JWTValidator validator(cfg);
    validator.setJWKSForTesting(jwks);
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() + 300;
    // Token without 'iss' claim
    nlohmann::json payload = {{"sub","u1"},{"email","u1@x"},{"aud","audX"},{"exp",exp}};
    std::string up = build_token("test-key-1", payload);
    std::string token = up + "." + sign_RS256(fix.pkey, up);
    // Empty issuer from token won't match expected "issuerX" → should throw
    EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error);
}

TEST(JWTValidatorTest, MissingAudClaim_WhenAudienceValidationEnabled) {
    RSAFixture fix; auto jwks = make_jwks(fix.rsa);
    JWTValidatorConfig cfg;
    cfg.expected_issuer = "issuerX";
    cfg.expected_audience = "audX";
    JWTValidator validator(cfg);
    validator.setJWKSForTesting(jwks);
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() + 300;
    // Token without 'aud' claim
    nlohmann::json payload = {{"sub","u1"},{"email","u1@x"},{"iss","issuerX"},{"exp",exp}};
    std::string up = build_token("test-key-1", payload);
    std::string token = up + "." + sign_RS256(fix.pkey, up);
    // No 'aud' in token but audience validation is configured → should throw
    EXPECT_THROW(validator.parseAndValidate(token), std::runtime_error);
}

TEST(JWTValidatorTest, ValidToken_IssuerValidationDisabled) {
    RSAFixture fix; auto jwks = make_jwks(fix.rsa);
    JWTValidatorConfig cfg;
    cfg.require_issuer_validation = false;   // no issuer check
    cfg.expected_audience = "audX";
    JWTValidator validator(cfg);
    validator.setJWKSForTesting(jwks);
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() + 300;
    // Token with arbitrary issuer – should be accepted
    nlohmann::json payload = {{"sub","u1"},{"email","u1@x"},{"iss","any-issuer"},{"aud","audX"},{"exp",exp}};
    std::string up = build_token("test-key-1", payload);
    std::string token = up + "." + sign_RS256(fix.pkey, up);
    auto claims = validator.parseAndValidate(token);
    EXPECT_EQ(claims.sub, "u1");
}

TEST(JWTValidatorTest, ValidToken_AudienceValidationDisabled) {
    RSAFixture fix; auto jwks = make_jwks(fix.rsa);
    JWTValidatorConfig cfg;
    cfg.expected_issuer = "issuerX";
    cfg.require_audience_validation = false;  // no audience check
    JWTValidator validator(cfg);
    validator.setJWKSForTesting(jwks);
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() + 300;
    // Token with arbitrary audience – should be accepted
    nlohmann::json payload = {{"sub","u1"},{"email","u1@x"},{"iss","issuerX"},{"aud","any-audience"},{"exp",exp}};
    std::string up = build_token("test-key-1", payload);
    std::string token = up + "." + sign_RS256(fix.pkey, up);
    auto claims = validator.parseAndValidate(token);
    EXPECT_EQ(claims.sub, "u1");
}

// ---------------------------------------------------------------------------
// Thread-safety: 32 threads is enough to reliably expose data races under
// TSAN and represents a realistic high-concurrency auth load.
// ---------------------------------------------------------------------------
static constexpr int JWKS_THREAD_SAFETY_TEST_THREADS = 32;

// Thread-safety: concurrent reads on warm JWKS cache (no data race)
// ---------------------------------------------------------------------------

TEST(JWTValidatorTest, ConcurrentValidate_WarmCache_NoDataRace) {
    RSAFixture fix;
    auto jwks = make_jwks(fix.rsa);

    // Build a valid token shared by all threads
    auto now = std::chrono::system_clock::now();
    auto exp_ts = std::chrono::duration_cast<std::chrono::seconds>(
                      now.time_since_epoch()).count() + 300;
    nlohmann::json payload = {{"sub","u-concurrent"},{"email","c@x"},
                               {"iss","issuerX"},{"aud","audX"},{"exp",exp_ts}};
    std::string up = build_token("test-key-1", payload);
    std::string token = up + "." + sign_RS256(fix.pkey, up);

    // Warm cache: TTL long enough that all threads hit the shared-lock path
    JWTValidatorConfig cfg{"", "issuerX", "audX",
                           std::chrono::seconds(600), std::chrono::seconds(60)};
    JWTValidator validator(cfg);
    validator.setJWKSForTesting(jwks);

    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};
    std::vector<std::thread> threads;
    threads.reserve(JWKS_THREAD_SAFETY_TEST_THREADS);

    for (int i = 0; i < JWKS_THREAD_SAFETY_TEST_THREADS; ++i) {
        threads.emplace_back([&]() {
            try {
                auto c = validator.parseAndValidate(token);
                if (c.sub == "u-concurrent") {
                    success_count.fetch_add(1, std::memory_order_relaxed);
                }
            } catch (...) {
                error_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads) t.join();

    EXPECT_EQ(success_count.load(), JWKS_THREAD_SAFETY_TEST_THREADS);
    EXPECT_EQ(error_count.load(), 0);
}

// ---------------------------------------------------------------------------
// Thread-safety: TTL=0 forces write-lock path — verifies no data race on
// concurrent cache-refresh attempts (thundering-herd protection).
// All threads will fail to fetch JWKS (no real server), but none must crash.
// ---------------------------------------------------------------------------

TEST(JWTValidatorTest, ConcurrentValidate_ExpiredCache_NoDataRace) {
    // jwks_url points to an unreachable host so curl fails fast; the important
    // thing is that no thread crashes or triggers a data race on jwks_cache_.
    JWTValidatorConfig cfg{"http://127.0.0.1:0/jwks", "issuerX", "audX",
                           std::chrono::seconds(0),   // TTL=0: cache always stale
                           std::chrono::seconds(60)};
    cfg.jwks_max_retries = 1;           // one attempt only to keep the test fast
    cfg.jwks_timeout_seconds = 1;       // 1-second curl timeout
    JWTValidator validator(cfg);

    std::atomic<int> throw_count{0};
    std::vector<std::thread> threads;
    threads.reserve(JWKS_THREAD_SAFETY_TEST_THREADS);

    // A dummy (unsigned) token — validation will never reach signature check
    // because fetchJWKS() will throw before that.
    const std::string dummy_token = "eyJhbGciOiJSUzI1NiIsImtpZCI6InRlc3QifQ.eyJzdWIiOiJ1In0.sig";

    for (int i = 0; i < JWKS_THREAD_SAFETY_TEST_THREADS; ++i) {
        threads.emplace_back([&]() {
            try {
                validator.parseAndValidate(dummy_token);
            } catch (const std::exception&) {
                throw_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads) t.join();

    // All threads must have thrown (no real JWKS endpoint); none must have crashed.
    EXPECT_EQ(throw_count.load(), JWKS_THREAD_SAFETY_TEST_THREADS);
}


// ===========================================================================
// validateAsync() — non-blocking future-based validation
// ===========================================================================

// A validator with a pre-loaded JWKS cache returns a ready future whose
// result is the same as the synchronous parseAndValidate() call.
TEST(JWTValidatorAsyncTest, ValidateAsyncMatchesSyncWithCachedJWKS)
{
    RSAFixture rsa;
    JWTValidatorConfig cfg;
    cfg.jwks_url = "https://idp.example.com/jwks";
    cfg.require_issuer_validation   = false;
    cfg.require_audience_validation = false;

    JWTValidator validator(cfg);

    // Reuse the shared helper used by stable sync tests.
    nlohmann::json jwks = make_jwks(rsa.rsa);
    validator.setJWKSForTesting(jwks);

    // Build a minimal valid JWT
    nlohmann::json hdr = {{"alg", "RS256"}, {"kid", "test-key-1"}};
    auto now_tp = std::chrono::system_clock::now();
    long now = static_cast<long>(std::chrono::duration_cast<std::chrono::seconds>(
        now_tp.time_since_epoch()).count());
    nlohmann::json payload = {
        {"sub", "testuser"},
        {"exp", now + 3600},
        {"iat", now}
    };

    const std::string hdr_str = hdr.dump();
    const std::string payload_str = payload.dump();
    auto b64hdr = b64url(std::vector<uint8_t>(hdr_str.begin(), hdr_str.end()));
    auto b64pay = b64url(std::vector<uint8_t>(payload_str.begin(), payload_str.end()));
    std::string header_payload = b64hdr + "." + b64pay;
    std::string sig = sign_RS256(rsa.pkey, header_payload);
    std::string token = header_payload + "." + sig;

    // Sync result
    JWTClaims sync_result;
    ASSERT_NO_THROW(sync_result = validator.parseAndValidate(token));

    // Async result must match
    auto fut = validator.validateAsync(token);
    ASSERT_TRUE(fut.valid());
    ASSERT_EQ(fut.wait_for(std::chrono::seconds(5)), std::future_status::ready);

    JWTClaims async_result;
    ASSERT_NO_THROW(async_result = fut.get());
    EXPECT_EQ(async_result.sub, sync_result.sub);
    EXPECT_EQ(async_result.sub, "testuser");
}

// validateAsync() must propagate exceptions through the future when the
// token fails validation.  Uses a pre-loaded JWKS cache so no network I/O
// is needed — the failure is caused by the malformed token structure, not
// by a missing/unreachable JWKS endpoint.
TEST(JWTValidatorAsyncTest, ValidateAsyncPropagatesExceptionForInvalidToken)
{
    RSAFixture rsa;
    JWTValidatorConfig cfg;
    cfg.jwks_url = "https://idp.example.com/jwks";
    cfg.require_issuer_validation   = false;
    cfg.require_audience_validation = false;

    JWTValidator validator(cfg);

    // Pre-load JWKS so no HTTP fetch is triggered.
    nlohmann::json jwks = make_jwks(rsa.rsa);
    validator.setJWKSForTesting(jwks);

    // Malformed token (not a valid JWT) must cause an exception that the
    // future propagates — entirely without network I/O.
    auto fut = validator.validateAsync("not.a.valid.jwt");
    ASSERT_TRUE(fut.valid());
    ASSERT_EQ(fut.wait_for(std::chrono::seconds(5)), std::future_status::ready);

    EXPECT_THROW(fut.get(), std::exception);
}

// Multiple concurrent validateAsync() calls must not deadlock or crash.
TEST(JWTValidatorAsyncTest, ConcurrentValidateAsyncNoCrash)
{
    RSAFixture rsa;
    JWTValidatorConfig cfg;
    cfg.jwks_url = "https://idp.example.com/jwks";
    cfg.require_issuer_validation   = false;
    cfg.require_audience_validation = false;

    JWTValidator validator(cfg);

    // Pre-load JWKS
    nlohmann::json jwks = make_jwks(rsa.rsa);
    validator.setJWKSForTesting(jwks);

    long now = static_cast<long>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    nlohmann::json hdr = {{"alg", "RS256"}, {"kid", "test-key-1"}};
    nlohmann::json payload = {{"sub", "u"}, {"exp", now + 3600}, {"iat", now}};
    const std::string hdr_str = hdr.dump();
    const std::string payload_str = payload.dump();
    auto b64hdr = b64url(std::vector<uint8_t>(hdr_str.begin(), hdr_str.end()));
    auto b64pay = b64url(std::vector<uint8_t>(payload_str.begin(), payload_str.end()));
    std::string hp = b64hdr + "." + b64pay;
    std::string token = hp + "." + sign_RS256(rsa.pkey, hp);

    constexpr int kTasks = 20;
    std::vector<std::future<JWTClaims>> futures;
    futures.reserve(kTasks);
    for (int i = 0; i < kTasks; ++i) {
        futures.push_back(validator.validateAsync(token));
    }

    int success_count = 0;
    for (auto& f : futures) {
        ASSERT_TRUE(f.valid());
        ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
        try {
            auto claims = f.get();
            EXPECT_EQ(claims.sub, "u");
            ++success_count;
        } catch (const std::exception& ex) {
            ADD_FAILURE() << "Unexpected exception: " << ex.what();
        }
    }
    EXPECT_EQ(success_count, kTasks);
}

// ---------------------------------------------------------------------------
// A-01 Thread-Safety Acceptance: 16-thread stress × high-iteration concurrent
// validate() with periodic cache-TTL expiry.
//
// Intent: Demonstrate that no data race occurs on jwks_cache_ / jwks_cache_time_
// under a sustained workload combining warm-cache reads (shared_lock path) and
// forced cache-refresh attempts (unique_lock path).  The validator is configured
// with a very short TTL so the cache expires mid-run, triggering write-lock paths
// concurrently with read-lock paths across all threads.
//
// We deliberately use a fast-failing JWKS URL (unreachable) so each refresh
// attempt returns an error quickly; the important invariant is zero data races
// (verifiable via ThreadSanitizer) and no crashes or deadlocks.
// ---------------------------------------------------------------------------

static constexpr int kA01StressThreads     = 16;
static constexpr int kA01StressIterations  = 500; // 16 × 500 = 8 000 total calls

TEST(JWTValidatorThreadStress, A01_16Thread_500Iter_NoDataRace) {
    RSAFixture fix;
    auto jwks = make_jwks(fix.rsa);

    // Build a valid signed token reused across all threads.
    auto now = std::chrono::system_clock::now();
    auto exp_ts = std::chrono::duration_cast<std::chrono::seconds>(
                      now.time_since_epoch()).count() + 600;
    nlohmann::json payload = {{"sub","u-stress"},{"email","s@x"},
                               {"iss","issuerX"},{"aud","audX"},{"exp",exp_ts}};
    std::string up    = build_token("test-key-1", payload);
    std::string token = up + "." + sign_RS256(fix.pkey, up);

    // Short TTL so the cache expires during the test run, exercising the
    // write-lock path concurrently with read-lock paths.
    JWTValidatorConfig cfg{"http://127.0.0.1:0/jwks", "issuerX", "audX",
                           std::chrono::milliseconds(20),  // 20 ms TTL — expires quickly
                           std::chrono::seconds(60)};
    cfg.jwks_max_retries     = 1;
    cfg.jwks_timeout_seconds = 1;
    JWTValidator validator(cfg);
    // Pre-warm with a real key so initial reads succeed.
    validator.setJWKSForTesting(jwks);

    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};

    std::vector<std::thread> threads;
    threads.reserve(kA01StressThreads);

    for (int t = 0; t < kA01StressThreads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < kA01StressIterations; ++i) {
                try {
                    auto claims = validator.parseAndValidate(token);
                    if (claims.sub == "u-stress") {
                        success_count.fetch_add(1, std::memory_order_relaxed);
                    }
                } catch (const std::exception&) {
                    // Refresh failures (unreachable JWKS URL) are expected once
                    // the in-memory cache expires.  Count them but don't fail.
                    error_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& th : threads) th.join();

    // At least some successes expected (while cache was warm) and total call
    // count must equal the configured load.
    const int total = success_count.load() + error_count.load();
    EXPECT_EQ(total, kA01StressThreads * kA01StressIterations);
    // No crash / no data race (verified externally via TSan).
}
