/**
 * @file signed_request.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=16, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/signed_request.h"
#include "sharding/pki_shard_certificate.h"
#include "utils/openssl_deleter.h"
#include "utils/logger.h"
#include <sstream>
#include <chrono>
#include <random>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <regex>
#include <cctype>
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
    constexpr const char* kAuditInvalidSignatureFormat = "SRV_INVALID_SIGNATURE_FORMAT";
    constexpr const char* kAuditInvalidKeyId = "SRV_INVALID_KEY_ID";
    constexpr const char* kAuditInvalidSignatureBase64 = "SRV_INVALID_SIGNATURE_BASE64";
    constexpr const char* kAuditUnknownKeyId = "SRV_UNKNOWN_KEY_ID";
    constexpr const char* kAuditTimestampExpired = "SRV_TIMESTAMP_EXPIRED";
    constexpr const char* kAuditNonceReplay = "SRV_NONCE_REPLAY";
    constexpr const char* kAuditNonceEviction = "SRV_NONCE_EVICTION";
    constexpr const char* kAuditInvalidCertSerial = "SRV_INVALID_CERT_SERIAL";
    constexpr const char* kAuditUnsupportedKeyType = "SRV_UNSUPPORTED_KEY_TYPE";

    [[nodiscard]] bool rejectWithAuditCode(const char* code, const std::string& details) {
        spdlog::warn("SignedRequestVerifier reject [{}]: {}", code, details);
        return false;
    }

    [[nodiscard]] bool isStrictBase64(const std::string& input) {
        if (input.empty() || (input.size() % 4) != 0) {
            return false;
        }

        size_t padding = 0;
        for (size_t i = input.size(); i > 0 && input[static_cast<int>(i - 1)] == '='; --i) {
            ++padding;
        }
        if (padding > 2) {
            return false;
        }

        for (size_t i = 0; i < input.size(); ++i) {
            const unsigned char ch = static_cast<unsigned char>(input[i]);
            const bool is_base64_char = std::isalnum(ch) || ch == '+' || ch == '/' || ch == '=';
            if (!is_base64_char) {
                return false;
            }
            if (ch == '='  && static_cast<size_t>(i) < static_cast<int>(input.size()) - padding) {
                return false;
            }
        }

        return true;
    }

    // Base64 encode helper
    [[nodiscard]] std::string base64Encode(const unsigned char* data, size_t len) {
        if (!data || len == 0) {
            spdlog::warn("base64Encode called with null or empty data");
            return "";
        }
        
        BIO* bmem = BIO_new(BIO_s_mem());
        if (!bmem) {
          return "";
        }
        BIO* b64 = BIO_new(BIO_f_base64());
        if (!b64) { BIO_free(bmem); return ""; }
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        auto bio = utils::BIOPtr(BIO_push(b64, bmem));  // BIO_push returns top of chain
        
        // W2-S07: Validate OpenSSL write result
        int written = BIO_write(bio.get(), data, static_cast<int>(len));
        if (written != static_cast<int>(len)) {
            spdlog::error("OpenSSL BIO_write incomplete: wrote {} of {} bytes", written, len);
            return "";
        }
        
        int flush_result = BIO_flush(bio.get());
        if (flush_result != 1) {
            spdlog::error("OpenSSL BIO_flush failed");
            return "";
        }
        
        BUF_MEM* buffer_ptr;
        BIO_get_mem_ptr(bio.get(), &buffer_ptr);
        
        if (!buffer_ptr || !buffer_ptr->data) {
            spdlog::error("OpenSSL buffer_ptr is null");
            return "";
        }
        
        std::string result(buffer_ptr->data, buffer_ptr->length);
        
        return result;
    }
    
    // Base64 decode helper
    std::optional<std::vector<unsigned char>> base64DecodeBytes(const std::string& encoded) {
        BIO* bmem = BIO_new_mem_buf(encoded.c_str(), static_cast<int>(encoded.size()));
        if (!bmem) {
          return std::nullopt;
        }
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

    // Certificate serial number pattern: up to 80 hex chars (RFC 5280 §4.1.2.2 caps
    // at 20 octets = 40 chars; 80 accommodates non-conformant enterprise CAs).
    // Static local — initialised exactly once (C++11 guarantee).
    const std::regex& certSerialPattern() {
        static const std::regex kPattern("^[0-9A-Fa-f]{1,80}$");
        return kPattern;
    }

    // Key IDs are used as trust-store file names (<key_id>.pem).
    // Restrict to a safe subset to prevent path traversal.
    const std::regex& keyIdPattern() {
        static const std::regex kPattern("^[A-Za-z0-9._-]{1,128}$");
        return kPattern;
    }
}

// ============================================================================
// SignedRequest
// ============================================================================

/** @brief Serialize signed request into JSON payload. */
nlohmann::json SignedRequest::toJSON() const {
    return nlohmann::json{
        {"shard_id", shard_id},
        {"operation", operation},
        {"path", path},
        {"body", body},
        {"timestamp_ms", timestamp_ms},
        {"nonce", nonce},
        {"signature_format", signature_format},
        {"key_id", key_id},
        {"signature_b64", signature_b64},
        {"cert_serial", cert_serial}
    };
}

/**
 * @brief Parse signed request from JSON.
 * @param j Input JSON value.
 * @return Parsed request object or std::nullopt when mandatory fields are missing/invalid.
 */
std::optional<SignedRequest> SignedRequest::fromJSON(const nlohmann::json& j) {
    try {
        SignedRequest req;
        req.shard_id = j.at("shard_id").get<std::string>();
        req.operation = j.at("operation").get<std::string>();
        req.path = j.at("path").get<std::string>();
        req.body = j.value("body", nlohmann::json{});
        req.timestamp_ms = j.at("timestamp_ms").get<uint64_t>();
        req.nonce = j.at("nonce").get<uint64_t>();
        req.signature_format = j.value("signature_format", std::string(SignedRequest::kSignatureFormatV1));
        req.key_id = j.value("key_id", std::string{});
        req.signature_b64 = j.at("signature_b64").get<std::string>();
        req.cert_serial = j.at("cert_serial").get<std::string>();
        if (req.key_id.empty()) {
            req.key_id = req.cert_serial;
        }
        
        return req;
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

/**
 * @brief Build canonical v1 signing string.
 * @return Deterministic line-based representation used for signing and verification.
 */
std::string SignedRequest::getCanonicalString() const {
    std::ostringstream oss = {};
    oss << "signature_format=" << signature_format << '\n'
        << "shard_id=" << shard_id << '\n'
        << "operation=" << operation << '\n'
        << "path=" << path << '\n'
        << "body=" << body.dump() << '\n'
        << "timestamp_ms=" << timestamp_ms << '\n'
        << "nonce=" << nonce << '\n'
        << "key_id=" << key_id << '\n'
        << "cert_serial=" << cert_serial << '\n';
    return oss.str();
}

// ============================================================================
// SignedRequestSigner
// ============================================================================

/**
 * @brief Construct signer and preload certificate serial when available.
 * @param config Signer configuration.
 */
SignedRequestSigner::SignedRequestSigner(const Config& config)
    : config_(config) {
    // Extract certificate serial
    auto cert_info = PKIShardCertificate::parseCertificate(config.cert_path);
    if (cert_info) {
        cert_serial_ = cert_info->serial_number;
    }
}

/**
 * @brief Populate metadata and signature fields.
 * @param request Request mutated in place.
 * @return true on successful signing.
 */
bool SignedRequestSigner::sign(SignedRequest& request) {
    // Set shard ID
    request.shard_id = config_.shard_id;
    
    // Set timestamp
    request.timestamp_ms = getCurrentTimestampMs();
    
    // Generate nonce
    request.nonce = generateNonce();
    
    // Set certificate serial
    request.cert_serial = cert_serial_;
    request.key_id = cert_serial_;
    request.signature_format = SignedRequest::kSignatureFormatV1;
    
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

/**
 * @brief Create and sign a request from primitive inputs.
 */
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

/** @brief Generate pseudo-random 64-bit nonce for replay protection. */
uint64_t SignedRequestSigner::generateNonce() const {
    std::random_device rd = {};
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
}

/** @brief Return current UNIX timestamp in milliseconds. */
uint64_t SignedRequestSigner::getCurrentTimestampMs() const {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return ms.count();
}

/**
 * @brief Sign canonical data with configured private key.
 * @param data Canonical request text.
 * @return Base64 signature, or std::nullopt when cryptographic setup/signing fails.
 */
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
    if (EVP_DigestSignUpdate(md_ctx.get(), data.c_str(),static_cast<int>(data.size())) != 1) {
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

/**
 * @brief Construct verifier with replay/certificate policy.
 * @param config Verification policy.
 */
SignedRequestVerifier::SignedRequestVerifier(const Config& config)
    : config_(config) {
}

/**
 * @brief Verify signed request metadata, freshness, replay and signature.
 * @param request Request to verify.
 * @param expected_shard_id Optional sender constraint.
 * @return true when all checks pass.
 */
bool SignedRequestVerifier::verify(const SignedRequest& request,
                                   const std::string& expected_shard_id) {
    // 0. Verify versioned request metadata (fail-closed)
    if (request.signature_format != SignedRequest::kSignatureFormatV1) {
        return rejectWithAuditCode(
            kAuditInvalidSignatureFormat,
            "unsupported signature_format='" + request.signature_format + "'");
    }
    if (request.key_id.empty() || !std::regex_match(request.key_id, keyIdPattern())) {
        return rejectWithAuditCode(
            kAuditInvalidKeyId,
            "invalid key_id='" + request.key_id + "'");
    }

    // 1. Verify timestamp
    if (!verifyTimestamp(request.timestamp_ms)) {
        return rejectWithAuditCode(
            kAuditTimestampExpired,
            "timestamp_ms=" + std::to_string(request.timestamp_ms));
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

/** @brief Remove expired replay-cache nonce entries. */
void SignedRequestVerifier::cleanupExpiredNonces() {
    std::lock_guard<std::mutex> lock(nonce_mutex_);
    purgeExpiredNoncesLocked(getCurrentTimestampMs());
}

/** @brief Verify timestamp skew is within configured bounds. */
bool SignedRequestVerifier::verifyTimestamp(uint64_t timestamp_ms) const {
    uint64_t current_time = getCurrentTimestampMs();
    uint64_t time_diff = (current_time > timestamp_ms) ?
        (current_time - timestamp_ms) : (timestamp_ms - current_time);
    
    return time_diff <= config_.max_time_skew_ms;
}

/**
 * @brief Verify nonce uniqueness and maintain replay cache.
 * @param nonce Nonce value to verify.
 * @param timestamp_ms Request timestamp used for expiry checks.
 * @return true when nonce is accepted.
 */
bool SignedRequestVerifier::verifyNonce(uint64_t nonce, uint64_t timestamp_ms) {
    std::lock_guard<std::mutex> lock(nonce_mutex_);

    if (config_.max_nonce_cache == 0) {
        return false;
    }

    const uint64_t now = getCurrentTimestampMs();

    // Fail-closed: reject stale requests outside replay window.
    if (now > timestamp_ms && (now - timestamp_ms) > config_.nonce_expiry_ms) {
        return false;
    }

    // Expire old nonces first.
    purgeExpiredNoncesLocked(now);

    // Check if nonce was seen before within replay window.
    if (seen_nonces_.find(nonce) != seen_nonces_.end()) {
        return rejectWithAuditCode(kAuditNonceReplay, "nonce=" + std::to_string(nonce));
    }

    while (static_cast<int>(seen_nonces_.size()) >= config_.max_nonce_cache && !nonce_fifo_.empty()) {
        const NonceEntry oldest = nonce_fifo_.front();
        nonce_fifo_.pop_front();
        const auto it = seen_nonces_.find(oldest.nonce);
        if (it != seen_nonces_.end() && it->second == oldest.timestamp_ms) {
            seen_nonces_.erase(it);
            spdlog::info("SignedRequestVerifier [{}]: evicted nonce due to cache pressure: {}",
                         kAuditNonceEviction,
                         oldest.nonce);
        }
    }

    // Add nonce to replay cache.
    seen_nonces_[nonce] = timestamp_ms;
    nonce_fifo_.push_back(NonceEntry{nonce, timestamp_ms});
    
    return true;
}

/**
 * @brief Verify signature using trust-store certificate selected by key_id.
 * @param request Signed request.
 * @return true when cryptographic verification succeeds.
 */
bool SignedRequestVerifier::verifySignature(const SignedRequest& request) {
    if (!isStrictBase64(request.signature_b64)) {
        return rejectWithAuditCode(
            kAuditInvalidSignatureBase64,
            "signature_b64 is not strict Base64");
    }

    // Decode base64 signature; reject if empty or malformed
    auto signature_bytes = base64DecodeBytes(request.signature_b64);
    if (!signature_bytes || signature_bytes->empty()) {
        return false;
    }

    // Step 1: Locate the peer certificate by key-id.
    // Certs are stored as <trusted_certs_dir>/<key_id>.pem.
    if (config_.trusted_certs_dir.empty()) {
        // Cannot verify without a cert directory; reject the request.
        // Warn once per process so operators know why verification is failing.
        static std::once_flag s_no_cert_dir_warn;
        std::call_once(s_no_cert_dir_warn, [] {
            spdlog::warn("SignedRequestVerifier: trusted_certs_dir is not configured — "
                         "all signature verifications will be REJECTED.  Set "
                         "Config::trusted_certs_dir to the directory containing "
                         "peer certificate PEM files.  "
                         "(This warning is printed once per process.)");
        });
        return false;
    }

    // Sanitize key_id: only allow safe trust-store file-name characters.
    if (!std::regex_match(request.key_id, keyIdPattern())) {
        THEMIS_WARN("verifySignature: rejected invalid key_id (path-traversal guard): '{}'",
                    request.key_id);
        return false;
    }

    // Use std::filesystem::path for safe concatenation.
    namespace fs = std::filesystem;
    fs::path cert_path = fs::path(config_.trusted_certs_dir) / (request.key_id + ".pem");

    // Verify the resolved certificate path is actually inside trusted_certs_dir
    // (defence-in-depth).  Compare parent_path() rather than doing string-prefix
    // matching, which can be fooled by directory names that share a prefix
    // (e.g. /trusted/certs vs /trusted/certs_evil).
    // Note on TOCTOU: there is an inherent window between weakly_canonical() and
    // the subsequent ifstream open below.  This is mitigated by (a) the regex
    // filter which rejects any non-safe key-id character before path construction, and
    // (b) running ThemisDB under a process user that has no write access to
    // trusted_certs_dir.
    // DEPLOYMENT REQUIREMENT: the process user MUST NOT have write access to
    // trusted_certs_dir (enforce via OS-level ACLs / container security context).
    // If this cannot be guaranteed, the administrator MUST use the fully atomic
    // openat(2)+O_NOFOLLOW approach (left to the OS-hardening layer) or mount the
    // directory read-only.  This requirement is documented in
    // docs/deployment/security_hardening.md §TrustedCertsDir.
    // A fully atomic solution would require openat(2) with O_NOFOLLOW or equivalent,
    // which is left to the OS-hardening layer.
    fs::path canonical_dir;
    fs::path canonical_cert;
    try {
        canonical_dir  = fs::weakly_canonical(fs::path(config_.trusted_certs_dir));
        canonical_cert = fs::weakly_canonical(cert_path);
    } catch (const fs::filesystem_error& e) {
        spdlog::warn("SignedRequestVerifier: path canonicalization failed for key_id='{}': {}",
                     request.key_id, e.what());
        return false;
    }
    if (canonical_cert.parent_path() != canonical_dir) {
        return false;
    }

    // Open the file using the canonicalized path to reduce the TOCTOU window
    // between weakly_canonical() and the read (the cert_path variable retains
    // the non-canonical form; using canonical_cert here means the fd refers to
    // the resolved inode, not a symlink that could be swapped after canonicalization).
    // A fully atomic solution would require openat(2)/O_NOFOLLOW; that is left
    // to the OS-hardening layer as noted above.
    std::ifstream cert_file(canonical_cert);
    if (!cert_file.good()) {
        // Log only the serial number to avoid leaking internal directory paths.
        return rejectWithAuditCode(
            kAuditUnknownKeyId,
            "certificate not found for key_id='" + request.key_id + "'");
    }
    std::string cert_pem((std::istreambuf_iterator<char>(cert_file)),
                          std::istreambuf_iterator<char>());
    if (cert_pem.empty()) {
        return false;
    }

    // Step 2: Parse the certificate and extract the public key.
    auto bio = utils::make_bio_mem_buf(cert_pem.c_str(), static_cast<int>(cert_pem.size()));
    if (!bio) {
      return false;
    }
    auto cert = utils::read_x509_from_bio(bio.get());
    if (!cert) {
      return false;
    }

    // Step 3: Verify the certificate against the CA (if ca_cert_path is configured).
    // Note: if ca_cert_path is empty, chain validation is skipped.  Production
    // deployments MUST configure ca_cert_path; warn once per process when absent.
    if (config_.ca_cert_path.empty()) {
        static std::once_flag s_ca_warn;
        std::call_once(s_ca_warn, [] {
            spdlog::warn("SignedRequestVerifier: ca_cert_path not configured — "
                         "certificate chain validation is SKIPPED.  Set Config::ca_cert_path "
                         "to the CA certificate to enable full chain validation. "
                         "(This warning is printed once per process.)");
        });
    } else if (!PKIShardCertificate::verifyCertificate(cert_path.string(), config_.ca_cert_path)) {
        return false;
    }

    // Step 4: Check Certificate Revocation List if configured.
    if (!config_.crl_path.empty()) {
        if (!std::regex_match(request.cert_serial, certSerialPattern())) {
            return rejectWithAuditCode(
                kAuditInvalidCertSerial,
                "CRL check requires valid cert_serial, got '" + request.cert_serial + "'");
        }
        if (PKIShardCertificate::isRevoked(request.cert_serial, config_.crl_path)) {
            return false;
        }
    }

    // Step 5: Extract public key from the parsed certificate.
    auto pubkey = utils::EVPKeyPtr(X509_get_pubkey(cert.get()));
    if (!pubkey) {
      return false;
    }

    // Step 6: Verify RSA/ECDSA-SHA-256 signature against the canonical request string.
    const std::string canonical = request.getCanonicalString();
    auto md_ctx = utils::make_evp_md_ctx();
    if (!md_ctx) {
      return false;
    }
    const int pubkey_type = EVP_PKEY_base_id(pubkey.get());
    if (pubkey_type == EVP_PKEY_ED25519) {
        if (EVP_DigestVerifyInit(md_ctx.get(), nullptr, nullptr, nullptr, pubkey.get()) != 1) {
            return false;
        }
        return EVP_DigestVerify(md_ctx.get(),
                                signature_bytes->data(),
                                signature_bytes->size(),
                                reinterpret_cast<const unsigned char*>(canonical.data()),
                                canonical.size()) == 1;
    } else if (pubkey_type == EVP_PKEY_RSA || pubkey_type == EVP_PKEY_EC) {
        if (EVP_DigestVerifyInit(md_ctx.get(), nullptr, EVP_sha256(), nullptr, pubkey.get()) != 1) {
            return false;
        }
        if (EVP_DigestVerifyUpdate(md_ctx.get(), canonical.c_str(),static_cast<int>(canonical.size())) != 1) {
            return false;
        }
        return EVP_DigestVerifyFinal(md_ctx.get(), signature_bytes->data(), signature_bytes->size()) == 1;
    }

    return rejectWithAuditCode(
        kAuditUnsupportedKeyType,
        "unsupported public key type id=" + std::to_string(pubkey_type));
}

/**
 * @brief Purge expired nonce entries from replay cache.
 * @param now_ms Current timestamp in milliseconds.
 */
void SignedRequestVerifier::purgeExpiredNoncesLocked(uint64_t now_ms) {
    while (!nonce_fifo_.empty()) {
        const NonceEntry oldest = nonce_fifo_.front();
        const bool is_expired =
            now_ms > oldest.timestamp_ms &&
            (now_ms - oldest.timestamp_ms) > config_.nonce_expiry_ms;
        if (!is_expired) {
            break;
        }

        nonce_fifo_.pop_front();
        const auto it = seen_nonces_.find(oldest.nonce);
        if (it != seen_nonces_.end() && it->second == oldest.timestamp_ms) {
            seen_nonces_.erase(it);
        }
    }
}

/** @brief Return current UNIX timestamp in milliseconds. */
uint64_t SignedRequestVerifier::getCurrentTimestampMs() const {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return ms.count();
}

} // namespace themis::sharding
