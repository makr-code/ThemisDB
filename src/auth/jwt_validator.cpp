/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            jwt_validator.cpp                                  ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     558                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "auth/jwt_validator.h"
#include "auth/jwks_validator.h"
#include "utils/hkdf_helper.h"
#include "utils/openssl_deleter.h"
#include "utils/logger.h"

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <openssl/bn.h>

#include <curl/curl.h>

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <thread>

namespace themis {
namespace auth {

namespace {
size_t curlWriteToString(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto total = size * nmemb;
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, total);
    return total;
}
}

JWTValidator::JWTValidator(const std::string& jwks_url)
    : cfg_{JWTValidatorConfig{jwks_url, "", "", std::chrono::seconds(600), std::chrono::seconds(60)}}
    , jwks_url_(jwks_url)
    , jwks_cache_time_(std::chrono::system_clock::time_point::min()) {}

JWTValidator::JWTValidator(const JWTValidatorConfig& cfg)
    : cfg_(cfg)
    , jwks_url_(cfg.jwks_url)
    , jwks_cache_time_(std::chrono::system_clock::time_point::min()) {}

std::vector<uint8_t> JWTValidator::decodeBase64Url(const std::string& input) {
    std::string base64 = input;
    std::replace(base64.begin(), base64.end(), '-', '+');
    std::replace(base64.begin(), base64.end(), '_', '/');
    while (base64.size() % 4 != 0) base64 += '=';
    
    BIO* bmem = BIO_new_mem_buf(base64.data(), static_cast<int>(base64.size()));
    if (!bmem) return {};
    BIO* b64 = BIO_new(BIO_f_base64());
    if (!b64) { BIO_free(bmem); return {}; }
    auto bio = utils::BIOPtr(BIO_push(b64, bmem));  // BIO_push returns top of chain
    BIO_set_flags(bio.get(), BIO_FLAGS_BASE64_NO_NL);
    
    std::vector<uint8_t> decoded(base64.size());
    int len = BIO_read(bio.get(), decoded.data(), static_cast<int>(decoded.size()));
    if (len < 0) return {};
    decoded.resize(len);
    return decoded;
}

std::string JWTValidator::decodeBase64UrlToString(const std::string& input) {
    auto bytes = decodeBase64Url(input);
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

nlohmann::json JWTValidator::fetchJWKS() {
    auto now = std::chrono::system_clock::now();
    if (!jwks_cache_.empty() && now - jwks_cache_time_ < cfg_.cache_ttl) {
        return jwks_cache_;
    }
    
    std::string response;
    int attempt = 0;
    int retry_delay_ms = 100; // Start with 100ms delay
    CURLcode rc = CURLE_FAILED_INIT;
    long code = 0;
    
    // Retry with exponential backoff
    while (attempt < cfg_.jwks_max_retries) {
        attempt++;
        
        CURL* curl = curl_easy_init();
        if (!curl) {
            utils::Logger::error("Failed to init curl for JWKS fetch");
            if (attempt < cfg_.jwks_max_retries) {
                std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
                retry_delay_ms *= 2; // Exponential backoff
                continue;
            }
            throw std::runtime_error("Failed to init curl for JWKS fetch after " + std::to_string(attempt) + " attempts");
        }
        
        response.clear();
        curl_easy_setopt(curl, CURLOPT_URL, jwks_url_.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(cfg_.jwks_timeout_seconds));
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, static_cast<long>(cfg_.jwks_timeout_seconds));
        
        rc = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        curl_easy_cleanup(curl);
        
        if (rc == CURLE_OK && code == 200) {
            // Success
            break;
        }
        
        // Log error and retry if not the last attempt
        if (attempt < cfg_.jwks_max_retries) {
            utils::Logger::warn("JWKS fetch attempt " + std::to_string(attempt) + " failed (HTTP " + 
                              std::to_string(code) + ", curl error " + std::to_string(rc) + "), retrying...");
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
            retry_delay_ms *= 2; // Exponential backoff
        }
    }
    
    if (rc != CURLE_OK || code != 200) {
        utils::Logger::error("JWKS HTTP error after " + std::to_string(attempt) + 
                           " attempts: HTTP " + std::to_string(code) + ", curl error " + std::to_string(rc));
        throw std::runtime_error("JWKS HTTP error: " + std::to_string(code) + " (after " + 
                               std::to_string(attempt) + " attempts)");
    }
    
    auto json = nlohmann::json::parse(response);
    if (!json.is_object() || !json.contains("keys")) {
        utils::Logger::error("Invalid JWKS document (missing keys)");
        throw std::runtime_error("Invalid JWKS document (missing keys)");
    }
    
    // Validate JWKS schema (P1 security hardening)
    JWKSValidator jwks_validator;
    try {
        jwks_validator.validateOrThrow(json);
    } catch (const std::exception& e) {
        utils::Logger::error("JWKS schema validation failed: {}", e.what());
        throw std::runtime_error(std::string("JWKS schema validation failed: ") + e.what());
    }
    
    jwks_cache_ = json;
    jwks_cache_time_ = now;
    utils::Logger::info("JWKS fetched successfully on attempt " + std::to_string(attempt));
    return jwks_cache_;
}

const nlohmann::json* JWTValidator::findJwkForKid(const nlohmann::json& jwks, const std::string& kid) const {
    if (!jwks.contains("keys")) return nullptr;
    for (auto& k : jwks["keys"]) {
        if (k.is_object() && k.value("kid", std::string()) == kid) return &k;
    }
    return nullptr;
}

bool JWTValidator::verifySignatureRS256(const std::string& header_payload,
                                        const std::vector<uint8_t>& signature,
                                        const nlohmann::json& jwk) {
    if (jwk.value("kty", "") != "RSA") return false;
    auto n_b64 = jwk.value("n", "");
    auto e_b64 = jwk.value("e", "");
    if (n_b64.empty() || e_b64.empty()) return false;
    auto n_bytes = decodeBase64Url(n_b64);
    auto e_bytes = decodeBase64Url(e_b64);
    auto n = utils::BIGNUMPtr(BN_bin2bn(n_bytes.data(), (int)n_bytes.size(), nullptr));
    auto e = utils::BIGNUMPtr(BN_bin2bn(e_bytes.data(), (int)e_bytes.size(), nullptr));
    if (!n || !e) return false;
    
    // Use EVP_PKEY directly instead of deprecated RSA_new()
    auto pkey = utils::make_evp_key();
    if (!pkey) return false;
    
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)  // OpenSSL deprecated APIs
#endif
    auto rsa = utils::make_rsa();
    if (!rsa) return false;
    
    // RSA_set0_key takes ownership only on success, so we need to release after success
    if (RSA_set0_key(rsa.get(), n.get(), e.get(), nullptr) != 1) {
        // Failed - n and e will be cleaned up by unique_ptr
        return false;
    }
    // Success - RSA now owns n and e, so release them from unique_ptr
    n.release();
    e.release();
    
    if (EVP_PKEY_assign_RSA(pkey.get(), rsa.get()) != 1) return false;
    // Success - pkey now owns rsa, so release it from unique_ptr
    rsa.release();
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // Verify using EVP_DigestVerify to compute SHA256 and PKCS#1 v1.5
    auto mctx = utils::make_evp_md_ctx();
    if (!mctx) return false;
    int ok = EVP_DigestVerifyInit(mctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get());
    if (ok != 1) return false;
    ok = EVP_DigestVerifyUpdate(mctx.get(), header_payload.data(), header_payload.size());
    if (ok != 1) return false;
    ok = EVP_DigestVerifyFinal(mctx.get(), signature.data(), signature.size());
    return ok == 1;
}

bool JWTValidator::verifySignatureES256(const std::string& header_payload,
                                        const std::vector<uint8_t>& signature,
                                        const nlohmann::json& jwk) {
    // Verify ECDSA P-256 / SHA-256 signature (ES256)
    // JWK format: {"kty":"EC","crv":"P-256","x":"<base64url>","y":"<base64url>"}
    if (jwk.value("kty", "") != "EC") return false;
    if (jwk.value("crv", "") != "P-256") return false;

    auto x_b64 = jwk.value("x", "");
    auto y_b64 = jwk.value("y", "");
    if (x_b64.empty() || y_b64.empty()) return false;

    auto x_bytes = decodeBase64Url(x_b64);
    auto y_bytes = decodeBase64Url(y_b64);
    if (x_bytes.size() != 32 || y_bytes.size() != 32) return false;

    // Build EC_KEY for P-256
    using ECKeyPtr = std::unique_ptr<EC_KEY, decltype(&EC_KEY_free)>;
    using ECGroupPtr = std::unique_ptr<EC_GROUP, decltype(&EC_GROUP_free)>;
    using ECPointPtr = std::unique_ptr<EC_POINT, decltype(&EC_POINT_free)>;

    ECGroupPtr group(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1), &EC_GROUP_free);
    if (!group) return false;

    ECKeyPtr ec_key(EC_KEY_new(), &EC_KEY_free);
    if (!ec_key) return false;
    if (EC_KEY_set_group(ec_key.get(), group.get()) != 1) return false;

    ECPointPtr pub_point(EC_POINT_new(group.get()), &EC_POINT_free);
    if (!pub_point) return false;

    auto x_bn = utils::BIGNUMPtr(BN_bin2bn(x_bytes.data(), (int)x_bytes.size(), nullptr));
    auto y_bn = utils::BIGNUMPtr(BN_bin2bn(y_bytes.data(), (int)y_bytes.size(), nullptr));
    if (!x_bn || !y_bn) return false;

    if (EC_POINT_set_affine_coordinates_GFp(group.get(), pub_point.get(),
                                            x_bn.get(), y_bn.get(), nullptr) != 1) return false;
    if (EC_KEY_set_public_key(ec_key.get(), pub_point.get()) != 1) return false;

    // Set EC_KEY into EVP_PKEY
    auto pkey = utils::make_evp_key();
    if (!pkey) return false;
    if (EVP_PKEY_set1_EC_KEY(pkey.get(), ec_key.get()) != 1) return false;

    // JWT ES256 signature is the raw (r || s) encoding (each 32 bytes = 64 bytes total).
    // OpenSSL ECDSA_verify expects DER-encoded ECDSA_SIG.  Convert r||s → DER.
    if (signature.size() != 64) return false;

    using ECDSASIGPtr = std::unique_ptr<ECDSA_SIG, decltype(&ECDSA_SIG_free)>;
    ECDSASIGPtr ecdsa_sig(ECDSA_SIG_new(), &ECDSA_SIG_free);
    if (!ecdsa_sig) return false;

    auto r_bn = utils::BIGNUMPtr(BN_bin2bn(signature.data(),      32, nullptr));
    auto s_bn = utils::BIGNUMPtr(BN_bin2bn(signature.data() + 32, 32, nullptr));
    if (!r_bn || !s_bn) return false;

    // ECDSA_SIG_set0 takes ownership on success
    if (ECDSA_SIG_set0(ecdsa_sig.get(), r_bn.get(), s_bn.get()) != 1) return false;
    r_bn.release();
    s_bn.release();

    // Encode to DER into managed memory.
    int der_len = i2d_ECDSA_SIG(ecdsa_sig.get(), nullptr);
    if (der_len <= 0) return false;
    std::vector<unsigned char> der_buf(static_cast<size_t>(der_len));
    unsigned char* der_ptr = der_buf.data();
    int encoded_len = i2d_ECDSA_SIG(ecdsa_sig.get(), &der_ptr);
    if (encoded_len != der_len) return false;

    // Verify using EVP_DigestVerify with SHA-256
    auto mctx = utils::make_evp_md_ctx();
    if (!mctx) return false;
    if (EVP_DigestVerifyInit(mctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) != 1) return false;
    if (EVP_DigestVerifyUpdate(mctx.get(), header_payload.data(), header_payload.size()) != 1) return false;
    return EVP_DigestVerifyFinal(mctx.get(), der_buf.data(), static_cast<size_t>(der_len)) == 1;
}

bool JWTValidator::verifySignatureEdDSA(const std::string& header_payload,
                                        const std::vector<uint8_t>& signature,
                                        const nlohmann::json& jwk) {
    // JWK format: {"kty":"OKP","crv":"Ed25519","x":"<base64url-32-bytes>"}
    auto it_crv = jwk.find("crv");
    if (it_crv == jwk.end() || it_crv->get<std::string>() != "Ed25519") return false;

    auto it_x = jwk.find("x");
    if (it_x == jwk.end()) return false;
    auto pub_bytes = decodeBase64Url(it_x->get<std::string>());
    if (pub_bytes.size() != 32) return false;  // Ed25519 public key is exactly 32 bytes

    EVP_PKEY* raw_pkey = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_ED25519, nullptr, pub_bytes.data(), pub_bytes.size());
    if (!raw_pkey) return false;
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pkey(raw_pkey, EVP_PKEY_free);

    EVP_MD_CTX* raw_ctx = EVP_MD_CTX_new();
    if (!raw_ctx) return false;
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(raw_ctx, EVP_MD_CTX_free);

    // Ed25519 uses a single-pass DigestVerify with md=nullptr
    if (EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get()) != 1) return false;
    int result = EVP_DigestVerify(
        ctx.get(),
        signature.data(), signature.size(),
        reinterpret_cast<const unsigned char*>(header_payload.data()),
        header_payload.size());
    return result == 1;
}

bool JWTValidator::checkAudience(const nlohmann::json& payload) const {
    if (cfg_.expected_audience.empty()) return true;
    if (!payload.contains("aud")) return false;
    if (payload["aud"].is_string()) {
        return payload["aud"].get<std::string>() == cfg_.expected_audience;
    }
    if (payload["aud"].is_array()) {
        for (auto& v : payload["aud"]) {
            if (v.is_string() && v.get<std::string>() == cfg_.expected_audience) return true;
        }
        return false;
    }
    return false;
}

void JWTValidator::setJWKSForTesting(const nlohmann::json& jwks,
                                     std::chrono::system_clock::time_point t) {
    jwks_cache_ = jwks;
    jwks_cache_time_ = t;
}

JWTClaims JWTValidator::parseAndValidate(const std::string& token) {
    std::string jwt = token;
    if (jwt.rfind("Bearer ", 0) == 0) {
        jwt = jwt.substr(7);
    }
    
    // Input validation: Check token size limit
    if (jwt.size() > MAX_JWT_TOKEN_SIZE) {
        utils::Logger::warn("JWT validation failed: Token exceeds maximum size");
        throw std::runtime_error("Token exceeds maximum size limit");
    }
    
    // Input validation: Check for empty token
    if (jwt.empty()) {
        utils::Logger::warn("JWT validation failed: Empty token");
        throw std::runtime_error("Empty token");
    }
    
    std::vector<std::string> parts;
    std::stringstream ss(jwt);
    std::string part;
    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }
    if (parts.size() != 3) {
        utils::Logger::warn("JWT validation failed: Invalid format (expected 3 parts)");
        throw std::runtime_error("Invalid JWT format (expected 3 parts)");
    }
    auto header_json = decodeBase64UrlToString(parts[0]);
    auto payload_json = decodeBase64UrlToString(parts[1]);
    auto header = nlohmann::json::parse(header_json);
    auto payload = nlohmann::json::parse(payload_json);
    std::string alg = header.value("alg", "");
    std::string kid = header.value("kid", "");
    
    // Check algorithm - support RS256, ES256 and EdDSA
    if (alg != "RS256" && alg != "ES256" && alg != "EdDSA") {
        utils::Logger::warn("JWT validation failed: Unsupported algorithm: " + alg);
        throw std::runtime_error("Unsupported alg: " + alg + " (supported: RS256, ES256, EdDSA)");
    }
    
    // Check kid revocation
    if (!kid.empty() && isKidRevoked(kid)) {
        utils::Logger::warn("JWT validation failed: Revoked kid: " + kid);
        throw std::runtime_error("Token signed with revoked key (kid: " + kid + ")");
    }
    
    JWTClaims claims;
    claims.sub = payload.value("sub", "");
    
    // Input validation: Check principal/subject length
    if (claims.sub.size() > MAX_PRINCIPAL_NAME_LENGTH) {
        utils::Logger::warn("JWT validation failed: Subject exceeds maximum length");
        throw std::runtime_error("Subject (principal) exceeds maximum length");
    }
    
    claims.email = payload.value("email", "");
    claims.jti = payload.value("jti", "");         // JWT ID – used for per-token revocation
    claims.tenant_id = payload.value("tenant_id", "");  // Extract tenant_id from JWT
    claims.issuer = payload.value("iss", "");
    if (payload.contains("groups")) {
        claims.groups = payload["groups"].get<std::vector<std::string>>();
    }
    if (payload.contains("roles")) {
        claims.roles = payload["roles"].get<std::vector<std::string>>();
    }
    auto now = std::chrono::system_clock::now();
    if (payload.contains("exp")) {
        int64_t exp = payload["exp"].get<int64_t>();
        claims.expiration = std::chrono::system_clock::time_point{std::chrono::seconds{exp}};
    } else {
        utils::Logger::warn("JWT validation failed: Missing exp claim");
        throw std::runtime_error("Missing exp claim");
    }
    if (payload.contains("nbf")) {
        int64_t nbf = payload["nbf"].get<int64_t>();
        claims.not_before = std::chrono::system_clock::time_point{std::chrono::seconds{nbf}};
        if (now + cfg_.clock_skew < *claims.not_before) {
            utils::Logger::warn("JWT validation failed: Token not yet valid (nbf)");
            throw std::runtime_error("Token not yet valid (nbf)");
        }
    }
    if (payload.contains("iat")) {
        int64_t iat = payload["iat"].get<int64_t>();
        claims.issued_at = std::chrono::system_clock::time_point{std::chrono::seconds{iat}};
        if (now + cfg_.clock_skew < *claims.issued_at) {
            utils::Logger::warn("JWT validation failed: iat in future");
            throw std::runtime_error("iat in future");
        }
    }
    if (payload.contains("aud")) {
        if (payload["aud"].is_string()) {
            claims.audience.push_back(payload["aud"].get<std::string>());
        } else if (payload["aud"].is_array()) {
            for (auto& v : payload["aud"]) if (v.is_string()) claims.audience.push_back(v.get<std::string>());
        }
    }
    if (claims.isExpired() && now > claims.expiration + cfg_.clock_skew) {
        utils::Logger::warn("JWT validation failed: Token expired");
        throw std::runtime_error("Token expired");
    }
    if (!cfg_.expected_issuer.empty() && claims.issuer != cfg_.expected_issuer) {
        utils::Logger::warn("JWT validation failed: Issuer mismatch (expected: " + cfg_.expected_issuer + ", got: " + claims.issuer + ")");
        throw std::runtime_error("Issuer mismatch");
    }
    if (!checkAudience(payload)) {
        utils::Logger::warn("JWT validation failed: Audience mismatch");
        throw std::runtime_error("Audience mismatch");
    }
    auto jwks = fetchJWKS();
    auto sig_bytes = decodeBase64Url(parts[2]);
    std::string header_payload = parts[0] + "." + parts[1];
    const nlohmann::json* jwk = nullptr;
    if (!kid.empty()) {
        jwk = findJwkForKid(jwks, kid);
        if (!jwk) {
            jwks_cache_time_ = std::chrono::system_clock::time_point::min();
            jwks = fetchJWKS();
            jwk = findJwkForKid(jwks, kid);
        }
    }
    if (!jwk) {
        utils::Logger::warn("JWT validation failed: JWK not found for kid: " + kid);
        throw std::runtime_error("JWK not found for kid");
    }
    bool sig_ok = false;
    if (alg == "RS256") {
        sig_ok = verifySignatureRS256(header_payload, sig_bytes, *jwk);
    } else if (alg == "ES256") {
        sig_ok = verifySignatureES256(header_payload, sig_bytes, *jwk);
    } else if (alg == "EdDSA") {
        sig_ok = verifySignatureEdDSA(header_payload, sig_bytes, *jwk);
    }
    if (!sig_ok) {
        utils::Logger::warn("JWT validation failed: Signature verification failed for kid: " + kid);
        throw std::runtime_error("Signature verification failed");
    }
    // Per-token revocation check: reject if the JTI is in the blacklist
    if (token_blacklist_ && !claims.jti.empty() && token_blacklist_->isRevoked(claims.jti)) {
        utils::Logger::warn("JWT validation failed: Token revoked (jti: " + claims.jti + ")");
        throw std::runtime_error("Token has been revoked");
    }
    return claims;
}

std::vector<uint8_t> JWTValidator::deriveUserKey(
    const std::vector<uint8_t>& dek,
    const JWTClaims& claims,
    const std::string& field_name) {
    std::vector<uint8_t> salt(claims.sub.begin(), claims.sub.end());
    std::string info = "user-field:" + field_name;
    return themis::utils::HKDFHelper::derive(dek, salt, info, 32);
}

bool JWTValidator::hasAccess(const JWTClaims& claims, const std::string& encryption_context) {
    if (claims.sub == encryption_context) {
        return true;
    }
    for (const auto& group : claims.groups) {
        if (group == encryption_context) {
            return true;
        }
    }
    return false;
}

void JWTValidator::setTokenBlacklist(TokenBlacklist* bl) {
    token_blacklist_ = bl;
}

void JWTValidator::revokeKid(const std::string& kid) {
    revoked_kids_runtime_.push_back(kid);
    utils::Logger::info("JWT kid revoked: " + kid);
}

bool JWTValidator::isKidRevoked(const std::string& kid) const {
    // Check config denylist
    for (const auto& revoked : cfg_.revoked_kids) {
        if (revoked == kid) {
            return true;
        }
    }
    // Check runtime denylist
    for (const auto& revoked : revoked_kids_runtime_) {
        if (revoked == kid) {
            return true;
        }
    }
    return false;
}

} // namespace auth
} // namespace themis
