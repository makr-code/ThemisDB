#include "auth/jwt_validator.h"
#include "utils/hkdf_helper.h"
#include "utils/openssl_deleter.h"
#include "utils/logger.h"

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/sha.h>

#include <curl/curl.h>

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <cstring>

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
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to init curl for JWKS fetch");
    curl_easy_setopt(curl, CURLOPT_URL, jwks_url_.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    CURLcode rc = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);
    if (rc != CURLE_OK || code != 200) {
        throw std::runtime_error("JWKS HTTP error: " + std::to_string(code));
    }
    auto json = nlohmann::json::parse(response);
    if (!json.is_object() || !json.contains("keys")) {
        throw std::runtime_error("Invalid JWKS document (missing keys)");
    }
    jwks_cache_ = json;
    jwks_cache_time_ = now;
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
    
    // Check algorithm
    if (alg != "RS256") {
        utils::Logger::warn("JWT validation failed: Unsupported algorithm: " + alg);
        throw std::runtime_error("Unsupported alg: " + alg + " (only RS256 is supported)");
    }
    
    // Check kid revocation
    if (!kid.empty() && isKidRevoked(kid)) {
        utils::Logger::warn("JWT validation failed: Revoked kid: " + kid);
        throw std::runtime_error("Token signed with revoked key (kid: " + kid + ")");
    }
    
    JWTClaims claims;
    claims.sub = payload.value("sub", "");
    claims.email = payload.value("email", "");
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
    if (!verifySignatureRS256(header_payload, sig_bytes, *jwk)) {
        utils::Logger::warn("JWT validation failed: Signature verification failed for kid: " + kid);
        throw std::runtime_error("Signature verification failed");
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
