/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            signature_verifier.cpp                             ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:49:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   84.0/100                                       ║
    • Total Lines:     762                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 13e4bb2974  2026-03-26  Enhance GraphQL Performance Tests and Saga Operation Comp... ║
    • afc6b2738b  2026-03-26  fix: Resolve BSI/RAG production blockers – JWT, mTLS, CRL... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/security/signature_verifier.h"
#include <spdlog/spdlog.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/x509_vfy.h>
#include <curl/curl.h>
#include <cstring>

namespace themis {
namespace llm {
namespace security {

// ===== ISignatureVerifier Base =====

SignatureVerificationResult ISignatureVerifier::passToNext(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& signature,
    const std::string& cert_pem) {
    
    if (next_) {
        return next_->verify(data, signature, cert_pem);
    }
    
    SignatureVerificationResult result;
    result.is_valid = true;
    return result;
}

// ===== RSA_SHA256_Verifier =====

SignatureVerificationResult RSA_SHA256_Verifier::verify(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& signature,
    const std::string& cert_pem) {
    
    SignatureVerificationResult result;
    result.algorithm = "RSA-SHA256";
    
    // Validate inputs
    if (data.empty()) {
        result.is_valid = false;
        result.error_message = "Data is empty";
        spdlog::error("RSA_SHA256_Verifier: Data is empty");
        return result;
    }
    
    if (signature.empty()) {
        result.is_valid = false;
        result.error_message = "Signature is empty";
        spdlog::error("RSA_SHA256_Verifier: Signature is empty");
        return result;
    }
    
    if (cert_pem.empty()) {
        result.is_valid = false;
        result.error_message = "Certificate is empty";
        spdlog::error("RSA_SHA256_Verifier: Certificate is empty");
        return result;
    }
    
    try {
        // 1. Load certificate
        auto cert = loadCertificate(cert_pem);
        if (!cert) {
            result.is_valid = false;
            result.error_message = "Failed to load certificate";
            return result;
        }
        
        // 2. Extract public key
        auto pkey = extractPublicKey(cert.get());
        if (!pkey) {
            result.is_valid = false;
            result.error_message = "Failed to extract public key from certificate";
            return result;
        }
        
        // 3. Check key size (minimum 2048 bits for RSA)
        int key_bits = EVP_PKEY_bits(pkey.get());
        if (EVP_PKEY_id(pkey.get()) == EVP_PKEY_RSA && key_bits < 2048) {
            result.is_valid = false;
            result.error_message = "RSA key size too small: " + std::to_string(key_bits) + " bits (minimum 2048)";
            spdlog::error("RSA_SHA256_Verifier: {}", result.error_message);
            return result;
        }
        
        // 4. Compute SHA-256 hash of data
        std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
        SHA256(data.data(), data.size(), hash.data());
        
        // 5. Verify signature using EVP API
        std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(
            EVP_PKEY_CTX_new(pkey.get(), nullptr),
            EVP_PKEY_CTX_free
        );
        
        if (!ctx) {
            result.is_valid = false;
            result.error_message = "Failed to create EVP_PKEY_CTX";
            spdlog::error("RSA_SHA256_Verifier: Failed to create verification context");
            return result;
        }
        
        if (EVP_PKEY_verify_init(ctx.get()) <= 0) {
            result.is_valid = false;
            result.error_message = "Failed to initialize verification";
            spdlog::error("RSA_SHA256_Verifier: EVP_PKEY_verify_init failed");
            return result;
        }
        
        // Set padding and hash algorithm
        if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) <= 0) {
            spdlog::warn("RSA_SHA256_Verifier: Failed to set RSA padding (may not be RSA key)");
        }
        
        if (EVP_PKEY_CTX_set_signature_md(ctx.get(), EVP_sha256()) <= 0) {
            result.is_valid = false;
            result.error_message = "Failed to set signature hash algorithm";
            spdlog::error("RSA_SHA256_Verifier: Failed to set SHA-256 digest");
            return result;
        }
        
        // Perform verification
        int verify_result = EVP_PKEY_verify(
            ctx.get(),
            signature.data(),
            signature.size(),
            hash.data(),
            hash.size()
        );
        
        if (verify_result == 1) {
            // Signature is valid
            result.is_valid = true;
            spdlog::info("RSA_SHA256_Verifier: Signature verification successful");
            
            // Extract signer identity from certificate using safer method
            X509_NAME* name = X509_get_subject_name(cert.get());
            if (name) {
                // Use BIO to safely convert name to string
                std::unique_ptr<BIO, decltype(&BIO_free)> bio(
                    BIO_new(BIO_s_mem()),
                    BIO_free
                );
                if (bio) {
                    X509_NAME_print_ex(bio.get(), name, 0, XN_FLAG_ONELINE);
                    BUF_MEM* mem = nullptr;
                    BIO_get_mem_ptr(bio.get(), &mem);
                    if (mem && mem->data && mem->length > 0) {
                        result.signer_identity = std::string(mem->data, mem->length);
                    }
                }
            }
            
            // Pass to next verifier in chain if exists
            if (next_) {
                return passToNext(data, signature, cert_pem);
            }
        } else if (verify_result == 0) {
            // Signature is invalid
            result.is_valid = false;
            result.error_message = "Signature verification failed: signature does not match";
            spdlog::warn("RSA_SHA256_Verifier: Signature verification failed");
        } else {
            // Error occurred
            result.is_valid = false;
            char err_buf[256];
            ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
            result.error_message = std::string("Verification error: ") + err_buf;
            spdlog::error("RSA_SHA256_Verifier: {}", result.error_message);
        }
        
    } catch (const std::exception& e) {
        result.is_valid = false;
        result.error_message = std::string("Exception during verification: ") + e.what();
        spdlog::error("RSA_SHA256_Verifier: {}", result.error_message);
    }
    
    return result;
}

std::unique_ptr<X509, decltype(&X509_free)> 
RSA_SHA256_Verifier::loadCertificate(const std::string& cert_pem) {
    
    spdlog::debug("Loading certificate from PEM");
    
    // Create BIO from PEM string
    std::unique_ptr<BIO, decltype(&BIO_free)> bio(
        BIO_new_mem_buf(cert_pem.data(), static_cast<int>(cert_pem.size())),
        BIO_free
    );
    
    if (!bio) {
        spdlog::error("Failed to create BIO from certificate PEM");
        return {nullptr, X509_free};
    }
    
    // Parse X.509 certificate from PEM
    X509* cert = PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr);
    
    if (!cert) {
        char err_buf[256];
        ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
        spdlog::error("Failed to parse X.509 certificate: {}", err_buf);
        return {nullptr, X509_free};
    }
    
    spdlog::debug("Certificate loaded successfully");
    return {cert, X509_free};
}

std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>
RSA_SHA256_Verifier::extractPublicKey(X509* cert) {
    
    if (!cert) {
        spdlog::error("Cannot extract public key from null certificate");
        return {nullptr, EVP_PKEY_free};
    }
    
    spdlog::debug("Extracting public key from certificate");
    
    // Extract public key from X.509 certificate
    EVP_PKEY* key = X509_get_pubkey(cert);
    
    if (!key) {
        char err_buf[256];
        ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
        spdlog::error("Failed to extract public key: {}", err_buf);
        return {nullptr, EVP_PKEY_free};
    }
    
    // Log key type and size
    int key_type = EVP_PKEY_id(key);
    int key_bits = EVP_PKEY_bits(key);
    const char* key_type_name = "UNKNOWN";
    
    switch (key_type) {
        case EVP_PKEY_RSA: key_type_name = "RSA"; break;
        case EVP_PKEY_EC: key_type_name = "EC"; break;
        case EVP_PKEY_ED25519: key_type_name = "Ed25519"; break;
        default: break;
    }
    
    spdlog::debug("Public key extracted: type={} ({}), size={} bits", 
                  key_type, key_type_name, key_bits);
    
    return {key, EVP_PKEY_free};
}

// ===== CertificateChainVerifier =====

CertificateChainVerifier::CertificateChainVerifier(
    const std::string& ca_bundle_path)
    : ca_bundle_path_(ca_bundle_path) {
    
    spdlog::info("CertificateChainVerifier created with CA bundle: {}", ca_bundle_path);
}

SignatureVerificationResult CertificateChainVerifier::verify(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& signature,
    const std::string& cert_pem) {
    
    SignatureVerificationResult result;
    result.algorithm = "Certificate Chain Validation";
    
    try {
        // 1. Load certificate from PEM
        std::unique_ptr<BIO, decltype(&BIO_free)> bio(
            BIO_new_mem_buf(cert_pem.data(), static_cast<int>(cert_pem.size())),
            BIO_free
        );
        
        if (!bio) {
            result.is_valid = false;
            result.error_message = "Failed to create BIO from certificate";
            spdlog::error("CertificateChainVerifier: {}", result.error_message);
            return result;
        }
        
        std::unique_ptr<X509, decltype(&X509_free)> cert(
            PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr),
            X509_free
        );
        
        if (!cert) {
            result.is_valid = false;
            result.error_message = "Failed to parse certificate";
            spdlog::error("CertificateChainVerifier: {}", result.error_message);
            return result;
        }
        
        // 2. Create X509_STORE and load CA bundle
        std::unique_ptr<X509_STORE, decltype(&X509_STORE_free)> store(
            X509_STORE_new(),
            X509_STORE_free
        );
        
        if (!store) {
            result.is_valid = false;
            result.error_message = "Failed to create X509_STORE";
            spdlog::error("CertificateChainVerifier: {}", result.error_message);
            return result;
        }
        
        // Load CA bundle - try multiple common locations
        bool ca_loaded = false;
        std::vector<std::string> ca_paths = {
            ca_bundle_path_,
            "/etc/ssl/certs/ca-certificates.crt",     // Debian/Ubuntu
            "/etc/pki/tls/certs/ca-bundle.crt",       // RHEL/CentOS
            "/etc/ssl/ca-bundle.pem",                  // OpenSUSE
            "/usr/local/share/certs/ca-root-nss.crt"  // FreeBSD
        };
        
        for (const auto& path : ca_paths) {
            if (X509_STORE_load_locations(store.get(), path.c_str(), nullptr) == 1) {
                ca_loaded = true;
                spdlog::debug("Loaded CA bundle from: {}", path);
                break;
            }
        }
        
        // Also try loading system default certificates
        if (!ca_loaded) {
            if (X509_STORE_set_default_paths(store.get()) == 1) {
                ca_loaded = true;
                spdlog::debug("Loaded system default CA certificates");
            }
        }
        
        if (!ca_loaded) {
            result.is_valid = false;
            result.error_message = "Failed to load CA bundle";
            spdlog::warn("CertificateChainVerifier: No CA bundle could be loaded - chain validation impossible");
            return result;
        }
        
        // 3. Verify certificate chain
        bool chain_valid = verifyCertificateChain(cert.get(), store.get());
        
        if (chain_valid) {
            result.is_valid = true;
            result.chain_valid = true;
            spdlog::info("CertificateChainVerifier: Certificate chain validation successful");
            
            // Extract chain information
            char subject_name[256] = {0};
            X509_NAME* name = X509_get_subject_name(cert.get());
            if (name) {
                X509_NAME_oneline(name, subject_name, sizeof(subject_name));
                result.signer_identity = subject_name;
            }
            
            // Pass to next verifier in chain
            if (next_) {
                return passToNext(data, signature, cert_pem);
            }
        } else {
            result.is_valid = false;
            result.chain_valid = false;
            result.error_message = "Certificate chain validation failed";
            spdlog::warn("CertificateChainVerifier: Chain validation failed");
        }
        
    } catch (const std::exception& e) {
        result.is_valid = false;
        result.error_message = std::string("Exception during chain validation: ") + e.what();
        spdlog::error("CertificateChainVerifier: {}", result.error_message);
    }
    
    return result;
}

bool CertificateChainVerifier::verifyCertificateChain(
    X509* cert,
    X509_STORE* store) {
    
    if (!cert || !store) {
        spdlog::error("verifyCertificateChain: Invalid certificate or store");
        return false;
    }
    
    spdlog::debug("Verifying certificate chain");
    
    // Create X509_STORE_CTX for verification
    std::unique_ptr<X509_STORE_CTX, decltype(&X509_STORE_CTX_free)> ctx(
        X509_STORE_CTX_new(),
        X509_STORE_CTX_free
    );
    
    if (!ctx) {
        spdlog::error("Failed to create X509_STORE_CTX");
        return false;
    }
    
    // Initialize context with certificate and store
    if (X509_STORE_CTX_init(ctx.get(), store, cert, nullptr) != 1) {
        char err_buf[256];
        ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
        spdlog::error("Failed to initialize X509_STORE_CTX: {}", err_buf);
        return false;
    }
    
    // Perform certificate chain verification
    int verify_result = X509_verify_cert(ctx.get());
    
    if (verify_result == 1) {
        spdlog::debug("Certificate chain verification successful");
        return true;
    } else {
        // Get detailed error
        int error_code = X509_STORE_CTX_get_error(ctx.get());
        const char* error_string = X509_verify_cert_error_string(error_code);
        int error_depth = X509_STORE_CTX_get_error_depth(ctx.get());
        
        spdlog::warn("Certificate chain verification failed: {} (depth: {})", 
                     error_string, error_depth);
        
        // Log specific error conditions
        switch (error_code) {
            case X509_V_ERR_CERT_HAS_EXPIRED:
                spdlog::warn("Certificate has expired");
                break;
            case X509_V_ERR_CERT_NOT_YET_VALID:
                spdlog::warn("Certificate is not yet valid");
                break;
            case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
                spdlog::warn("Self-signed certificate");
                break;
            case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
                spdlog::warn("Unable to get issuer certificate");
                break;
            case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
                spdlog::warn("Unable to verify leaf signature");
                break;
            default:
                break;
        }
        
        return false;
    }
}

// ===== CRLChecker =====

CRLChecker::CRLChecker(const std::string& crl_url)
    : crl_url_(crl_url) {
    
    spdlog::info("CRLChecker created with CRL URL: {}", crl_url);
}

SignatureVerificationResult CRLChecker::verify(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& signature,
    const std::string& cert_pem) {
    
    SignatureVerificationResult result;
    result.algorithm = "CRL Check";
    
    try {
        // 1. Load certificate from PEM
        std::unique_ptr<BIO, decltype(&BIO_free)> bio(
            BIO_new_mem_buf(cert_pem.data(), static_cast<int>(cert_pem.size())),
            BIO_free
        );
        
        if (!bio) {
            result.is_valid = false;
            result.error_message = "Failed to create BIO from certificate";
            spdlog::error("CRLChecker: {}", result.error_message);
            return result;
        }
        
        std::unique_ptr<X509, decltype(&X509_free)> cert(
            PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr),
            X509_free
        );
        
        if (!cert) {
            result.is_valid = false;
            result.error_message = "Failed to parse certificate";
            spdlog::error("CRLChecker: {}", result.error_message);
            return result;
        }
        
        // 2. Check if certificate is revoked
        bool is_revoked = isCertificateRevoked(cert.get());
        
        if (is_revoked) {
            result.is_valid = false;
            result.error_message = "Certificate has been revoked";
            spdlog::warn("CRLChecker: Certificate is revoked");
        } else {
            result.is_valid = true;
            spdlog::debug("CRLChecker: Certificate is not revoked");
            
            // Pass to next verifier in chain
            if (next_) {
                return passToNext(data, signature, cert_pem);
            }
        }
        
    } catch (const std::exception& e) {
        // Handle CRL check failures gracefully - log warning but don't fail
        // This is because CRL may be temporarily unavailable
        spdlog::warn("CRLChecker: Exception during CRL check: {}", e.what());
        result.is_valid = true;  // Optimistic - proceed if CRL check fails
        result.error_message = "CRL check skipped due to error: " + std::string(e.what());
        
        // Pass to next verifier in chain
        if (next_) {
            return passToNext(data, signature, cert_pem);
        }
    }
    
    return result;
}

// libcurl write callback – appends received bytes to a std::string.
static size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(ptr, size * nmemb);
    return size * nmemb;
}

X509_CRL* CRLChecker::downloadAndParseCRL() const {
    if (crl_url_.empty()) {
        return nullptr;
    }

    std::string raw;
    CURL* curl = curl_easy_init();
    if (!curl) {
        spdlog::error("CRLChecker: curl_easy_init() failed");
        return nullptr;
    }

    curl_easy_setopt(curl, CURLOPT_URL, crl_url_.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &raw);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);          // 10 s total
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode rc = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        spdlog::error("CRLChecker: CRL download failed ({}): {}", crl_url_,
                      curl_easy_strerror(rc));
        return nullptr;
    }

    if (raw.empty()) {
        spdlog::error("CRLChecker: Empty response when downloading CRL from {}", crl_url_);
        return nullptr;
    }

    // Try DER first, then PEM.
    const auto* der_data =
        reinterpret_cast<const unsigned char*>(raw.data());
    X509_CRL* crl = d2i_X509_CRL(nullptr, &der_data,
                                  static_cast<long>(raw.size()));
    if (!crl) {
        // Try PEM
        BIO* bio = BIO_new_mem_buf(raw.data(), static_cast<int>(raw.size()));
        if (bio) {
            crl = PEM_read_bio_X509_CRL(bio, nullptr, nullptr, nullptr);
            BIO_free(bio);
        }
    }

    if (!crl) {
        spdlog::error("CRLChecker: Failed to parse CRL from {}", crl_url_);
        return nullptr;
    }

    spdlog::info("CRLChecker: CRL downloaded and parsed from {}", crl_url_);
    return crl;
}

X509_CRL* CRLChecker::getOrRefreshCRL() const {
    std::lock_guard<std::mutex> lk(cache_mutex_);

    auto now = std::chrono::steady_clock::now();
    if (crl_cache_.crl && now < crl_cache_.expires_at) {
        return crl_cache_.crl;   // still valid
    }

    // Free stale entry.
    if (crl_cache_.crl) {
        X509_CRL_free(crl_cache_.crl);
        crl_cache_.crl = nullptr;
    }

    X509_CRL* fresh = downloadAndParseCRL();
    if (!fresh) {
        return nullptr;
    }

    // Determine TTL from CRL's nextUpdate field; fall back to 1 hour.
    std::chrono::seconds ttl{3600};
    const ASN1_TIME* next_update = X509_CRL_get0_nextUpdate(fresh);
    if (next_update) {
        struct tm tm_next{};
        if (ASN1_TIME_to_tm(next_update, &tm_next) == 1) {
            time_t t_next;
#ifdef _WIN32
            t_next = _mkgmtime(&tm_next);
#else
            t_next = timegm(&tm_next);
#endif
            time_t t_now   = std::time(nullptr);
            long   secs    = static_cast<long>(t_next - t_now);
            if (secs > 60) {
                ttl = std::chrono::seconds{secs};
            }
        }
    }

    crl_cache_.crl        = fresh;
    crl_cache_.expires_at = now + ttl;
    spdlog::debug("CRLChecker: CRL cached for {} seconds", ttl.count());
    return fresh;
}

bool CRLChecker::isCertificateRevoked(X509* cert) {

    if (!cert) {
        spdlog::error("isCertificateRevoked: Invalid certificate");
        return false;
    }

    if (crl_url_.empty()) {
        spdlog::debug("No CRL URL configured – assuming not revoked");
        return false;
    }

    // Log serial for auditability.
    ASN1_INTEGER* serial_asn1 = X509_get_serialNumber(cert);
    if (serial_asn1) {
        BIGNUM* bn = ASN1_INTEGER_to_BN(serial_asn1, nullptr);
        if (bn) {
            char* hex = BN_bn2hex(bn);
            if (hex) {
                spdlog::debug("CRLChecker: Checking revocation for serial: {}", hex);
                OPENSSL_free(hex);
            }
            BN_free(bn);
        }
    }

    X509_CRL* crl = getOrRefreshCRL();
    if (!crl) {
        // Fail-open: CRL unavailable → assume not revoked (availability over strict validation).
        spdlog::warn("CRLChecker: CRL unavailable from {} – assuming not revoked (fail-open)",
                     crl_url_);
        return false;
    }

    // X509_CRL_get0_by_cert returns 1 when the certificate is found in the CRL.
    X509_REVOKED* revoked_entry = nullptr;
    int found = X509_CRL_get0_by_cert(crl, &revoked_entry, cert);
    if (found > 0) {
        spdlog::warn("CRLChecker: Certificate IS revoked (found in CRL at {})", crl_url_);
        return true;
    }

    spdlog::debug("CRLChecker: Certificate is NOT revoked");
    return false;
}

// ===== Builder =====

SignatureVerifierBuilder& SignatureVerifierBuilder::withRSA_SHA256() {
    auto verifier = std::make_shared<RSA_SHA256_Verifier>();
    
    if (!head_) {
        head_ = verifier;
        tail_ = verifier;
    } else {
        tail_->setNext(verifier);
        tail_ = verifier;
    }
    
    spdlog::debug("Added RSA-SHA256 verifier to chain");
    return *this;
}

SignatureVerifierBuilder& SignatureVerifierBuilder::withCertificateChainValidation(
    const std::string& ca_bundle_path) {
    
    auto verifier = std::make_shared<CertificateChainVerifier>(ca_bundle_path);
    
    if (!head_) {
        head_ = verifier;
        tail_ = verifier;
    } else {
        tail_->setNext(verifier);
        tail_ = verifier;
    }
    
    spdlog::debug("Added Certificate Chain verifier to chain");
    return *this;
}

SignatureVerifierBuilder& SignatureVerifierBuilder::withCRLCheck(
    const std::string& crl_url) {
    
    auto verifier = std::make_shared<CRLChecker>(crl_url);
    
    if (!head_) {
        head_ = verifier;
        tail_ = verifier;
    } else {
        tail_->setNext(verifier);
        tail_ = verifier;
    }
    
    spdlog::debug("Added CRL checker to chain");
    return *this;
}

std::shared_ptr<ISignatureVerifier> SignatureVerifierBuilder::build() {
    if (!head_) {
        spdlog::warn("SignatureVerifierBuilder: No verifiers added to chain");
    }
    return head_;
}

} // namespace security
} // namespace llm
} // namespace themis
