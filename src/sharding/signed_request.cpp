/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            signed_request.cpp                                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:50:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     349                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/signed_request.h"
#include "sharding/pki_shard_certificate.h"
#include "utils/openssl_deleter.h"
#include <sstream>
#include <chrono>
#include <random>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <regex>
#include <spdlog/spdlog.h>

// OpenSSL for signing / verification
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/x509.h>

namespace themis::sharding {

namespace {
    // Base64 encode helper
    std::string base64Encode(const unsigned char* data, size_t len) {
        BIO* bmem = BIO_new(BIO_s_mem());
        if (!bmem) return "";
        BIO* b64 = BIO_new(BIO_f_base64());
        if (!b64) { BIO_free(bmem); return ""; }
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        auto bio = utils::BIOPtr(BIO_push(b64, bmem));  // BIO_push returns top of chain
        
        BIO_write(bio.get(), data, static_cast<int>(len));
        BIO_flush(bio.get());
        
        BUF_MEM* buffer_ptr;
        BIO_get_mem_ptr(bio.get(), &buffer_ptr);
        
        std::string result(buffer_ptr->data, buffer_ptr->length);
        
        return result;
    }
    
    // Base64 decode helper
    std::optional<std::vector<unsigned char>> base64DecodeBytes(const std::string& encoded) {
        BIO* bmem = BIO_new_mem_buf(encoded.c_str(), static_cast<int>(encoded.size()));
        if (!bmem) return std::nullopt;
        BIO* b64 = BIO_new(BIO_f_base64());
        if (!b64) { BIO_free(bmem); return std::nullopt; }
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        auto bio = utils::BIOPtr(BIO_push(b64, bmem));  // BIO_push returns top of chain
        
        std::vector<unsigned char> decoded(encoded.size());
        int decoded_len = BIO_read(bio.get(), decoded.data(), static_cast<int>(decoded.size()));
        
        if (decoded_len < 0) {
            return std::nullopt;
        }
        
        decoded.resize(decoded_len);
        return decoded;
    }
}

// ============================================================================
// SignedRequest
// ============================================================================

nlohmann::json SignedRequest::toJSON() const {
    return nlohmann::json{
        {"shard_id", shard_id},
        {"operation", operation},
        {"path", path},
        {"body", body},
        {"timestamp_ms", timestamp_ms},
        {"nonce", nonce},
        {"signature_b64", signature_b64},
        {"cert_serial", cert_serial}
    };
}

std::optional<SignedRequest> SignedRequest::fromJSON(const nlohmann::json& j) {
    try {
        SignedRequest req;
        req.shard_id = j.at("shard_id").get<std::string>();
        req.operation = j.at("operation").get<std::string>();
        req.path = j.at("path").get<std::string>();
        req.body = j.value("body", nlohmann::json{});
        req.timestamp_ms = j.at("timestamp_ms").get<uint64_t>();
        req.nonce = j.at("nonce").get<uint64_t>();
        req.signature_b64 = j.at("signature_b64").get<std::string>();
        req.cert_serial = j.at("cert_serial").get<std::string>();
        
        return req;
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

std::string SignedRequest::getCanonicalString() const {
    std::ostringstream oss;
    oss << shard_id << "|"
        << operation << "|"
        << path << "|"
        << body.dump() << "|"
        << timestamp_ms << "|"
        << nonce;
    return oss.str();
}

// ============================================================================
// SignedRequestSigner
// ============================================================================

SignedRequestSigner::SignedRequestSigner(const Config& config)
    : config_(config) {
    // Extract certificate serial
    auto cert_info = PKIShardCertificate::parseCertificate(config.cert_path);
    if (cert_info) {
        cert_serial_ = cert_info->serial_number;
    }
}

bool SignedRequestSigner::sign(SignedRequest& request) {
    // Set shard ID
    request.shard_id = config_.shard_id;
    
    // Set timestamp
    request.timestamp_ms = getCurrentTimestampMs();
    
    // Generate nonce
    request.nonce = generateNonce();
    
    // Set certificate serial
    request.cert_serial = cert_serial_;
    
    // Create canonical string
    std::string canonical = request.getCanonicalString();
    
    // Sign
    auto signature = signData(canonical);
    if (!signature) {
        return false;
    }
    
    request.signature_b64 = *signature;
    return true;
}

SignedRequest SignedRequestSigner::createSignedRequest(const std::string& operation,
                                                       const std::string& path,
                                                       const nlohmann::json& body) {
    SignedRequest request;
    request.operation = operation;
    request.path = path;
    request.body = body;
    
    sign(request);
    return request;
}

uint64_t SignedRequestSigner::generateNonce() const {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
}

uint64_t SignedRequestSigner::getCurrentTimestampMs() const {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return ms.count();
}

std::optional<std::string> SignedRequestSigner::signData(const std::string& data) {
    // Read private key
    FILE* key_file = fopen(config_.key_path.c_str(), "r");
    if (!key_file) {
        return std::nullopt;
    }
    
    EVP_PKEY* pkey_raw = nullptr;
    if (!config_.key_passphrase.empty()) {
        pkey_raw = PEM_read_PrivateKey(key_file, nullptr, nullptr,
                                   const_cast<char*>(config_.key_passphrase.c_str()));
    } else {
        pkey_raw = PEM_read_PrivateKey(key_file, nullptr, nullptr, nullptr);
    }
    fclose(key_file);
    
    auto pkey = utils::EVPKeyPtr(pkey_raw);
    if (!pkey) {
        return std::nullopt;
    }
    
    // Create signature context
    auto md_ctx = utils::make_evp_md_ctx();
    if (!md_ctx) {
        return std::nullopt;
    }
    
    // Initialize signing
    if (EVP_DigestSignInit(md_ctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) != 1) {
        return std::nullopt;
    }
    
    // Update with data
    if (EVP_DigestSignUpdate(md_ctx.get(), data.c_str(), data.size()) != 1) {
        return std::nullopt;
    }
    
    // Get signature length
    size_t sig_len = 0;
    if (EVP_DigestSignFinal(md_ctx.get(), nullptr, &sig_len) != 1) {
        return std::nullopt;
    }
    
    // Get signature
    std::vector<unsigned char> signature(sig_len);
    if (EVP_DigestSignFinal(md_ctx.get(), signature.data(), &sig_len) != 1) {
        return std::nullopt;
    }
    
    // Base64 encode signature
    return base64Encode(signature.data(), sig_len);
}

// ============================================================================
// SignedRequestVerifier
// ============================================================================

SignedRequestVerifier::SignedRequestVerifier(const Config& config)
    : config_(config) {
}

bool SignedRequestVerifier::verify(const SignedRequest& request,
                                   const std::string& expected_shard_id) {
    // 1. Verify timestamp
    if (!verifyTimestamp(request.timestamp_ms)) {
        return false;
    }
    
    // 2. Verify nonce (replay protection)
    if (!verifyNonce(request.nonce, request.timestamp_ms)) {
        return false;
    }
    
    // 3. Verify expected shard ID if provided
    if (!expected_shard_id.empty() && request.shard_id != expected_shard_id) {
        return false;
    }
    
    // 4. Verify signature
    if (!verifySignature(request)) {
        return false;
    }
    
    return true;
}

void SignedRequestVerifier::cleanupExpiredNonces() {
    std::lock_guard<std::mutex> lock(nonce_mutex_);
    
    // In production, would track (nonce, timestamp) pairs and expire old ones
    // For Phase 2, we keep it simple
    if (seen_nonces_.size() > config_.max_nonce_cache) {
        seen_nonces_.clear();
    }
}

bool SignedRequestVerifier::verifyTimestamp(uint64_t timestamp_ms) const {
    uint64_t current_time = getCurrentTimestampMs();
    uint64_t time_diff = (current_time > timestamp_ms) ?
        (current_time - timestamp_ms) : (timestamp_ms - current_time);
    
    return time_diff <= config_.max_time_skew_ms;
}

bool SignedRequestVerifier::verifyNonce(uint64_t nonce, [[maybe_unused]] uint64_t timestamp_ms) {
    // Future: implement timestamp-based nonce expiry
    std::lock_guard<std::mutex> lock(nonce_mutex_);
    
    // Check if nonce was seen before
    if (seen_nonces_.find(nonce) != seen_nonces_.end()) {
        return false; // Replay attack detected
    }
    
    // Add nonce to seen set
    seen_nonces_.insert(nonce);
    
    // Cleanup if cache is too large
    if (seen_nonces_.size() > config_.max_nonce_cache) {
        // Remove oldest half (simplified cleanup)
        auto it = seen_nonces_.begin();
        std::advance(it, config_.max_nonce_cache / 2);
        seen_nonces_.erase(seen_nonces_.begin(), it);
    }
    
    return true;
}

bool SignedRequestVerifier::verifySignature(const SignedRequest& request) {
    // Decode base64 signature; reject if empty or malformed
    auto signature_bytes = base64DecodeBytes(request.signature_b64);
    if (!signature_bytes || signature_bytes->empty()) {
        return false;
    }

    // Step 1: Locate the peer certificate by serial number.
    // Certs are stored as <trusted_certs_dir>/<cert_serial>.pem.
    if (config_.trusted_certs_dir.empty()) {
        // Cannot verify without a cert directory; reject the request.
        return false;
    }

    // Sanitize cert_serial: only allow hexadecimal characters (certificate serial
    // numbers are hex strings).  Reject anything that could be used to escape the
    // trusted_certs_dir via path traversal (e.g. "../", absolute paths, null bytes).
    static const std::regex kSerialPattern("^[0-9A-Fa-f]{1,64}$");
    if (!std::regex_match(request.cert_serial, kSerialPattern)) {
        return false;
    }

    // Use std::filesystem::path for safe concatenation.
    namespace fs = std::filesystem;
    fs::path cert_path = fs::path(config_.trusted_certs_dir) / (request.cert_serial + ".pem");

    // Verify the resolved path is actually inside trusted_certs_dir (defence-in-depth).
    auto canonical_dir  = fs::weakly_canonical(fs::path(config_.trusted_certs_dir));
    auto canonical_cert = fs::weakly_canonical(cert_path);
    if (canonical_cert.string().find(canonical_dir.string()) != 0) {
        return false;
    }

    std::ifstream cert_file(cert_path);
    if (!cert_file.good()) {
        return false;
    }
    std::string cert_pem((std::istreambuf_iterator<char>(cert_file)),
                          std::istreambuf_iterator<char>());
    if (cert_pem.empty()) {
        return false;
    }

    // Step 2: Parse the certificate and extract the public key.
    auto bio = utils::make_bio_mem_buf(cert_pem.c_str(), static_cast<int>(cert_pem.size()));
    if (!bio) return false;
    auto cert = utils::read_x509_from_bio(bio.get());
    if (!cert) return false;

    // Step 3: Verify the certificate against the CA (if ca_cert_path is configured).
    // Note: if ca_cert_path is empty, chain validation is skipped.  Production
    // deployments MUST configure ca_cert_path; log a warning when it is absent.
    if (config_.ca_cert_path.empty()) {
        spdlog::warn("SignedRequestVerifier: ca_cert_path not configured — "
                     "certificate chain validation is SKIPPED.  Set Config::ca_cert_path "
                     "to the CA certificate to enable full chain validation.");
    } else if (!PKIShardCertificate::verifyCertificate(cert_path.string(), config_.ca_cert_path)) {
        return false;
    }

    // Step 4: Check Certificate Revocation List if configured.
    if (!config_.crl_path.empty() &&
        PKIShardCertificate::isRevoked(request.cert_serial, config_.crl_path)) {
        return false;
    }

    // Step 5: Extract public key from the parsed certificate.
    auto pubkey = utils::EVPKeyPtr(X509_get_pubkey(cert.get()));
    if (!pubkey) return false;

    // Step 6: Verify RSA/ECDSA-SHA-256 signature against the canonical request string.
    const std::string canonical = request.getCanonicalString();
    auto md_ctx = utils::make_evp_md_ctx();
    if (!md_ctx) return false;
    if (EVP_DigestVerifyInit(md_ctx.get(), nullptr, EVP_sha256(), nullptr, pubkey.get()) != 1) {
        return false;
    }
    if (EVP_DigestVerifyUpdate(md_ctx.get(), canonical.c_str(), canonical.size()) != 1) {
        return false;
    }
    return EVP_DigestVerifyFinal(md_ctx.get(), signature_bytes->data(), signature_bytes->size()) == 1;
}

uint64_t SignedRequestVerifier::getCurrentTimestampMs() const {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return ms.count();
}

} // namespace themis::sharding
