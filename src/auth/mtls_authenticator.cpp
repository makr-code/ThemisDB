/**
 * @file mtls_authenticator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=10, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/mtls_authenticator.h"

#include "auth/auth_audit_logger.h"

#include <iomanip>
#include <memory>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace auth {

namespace {

// RAII helpers for OpenSSL types
struct X509Deleter {
    void operator()(X509 *p) const {
        X509_free(p);
    }
};
struct X509StoreDeleter {
    void operator()(X509_STORE *p) const {
        X509_STORE_free(p);
    }
};
struct X509StoreCTXDeleter {
    void operator()(X509_STORE_CTX *p) const {
        X509_STORE_CTX_free(p);
    }
};
struct BIODeleter {
    void operator()(BIO *p) const {
        BIO_free_all(p);
    }
};
struct X509CRLDeleter {
    void operator()(X509_CRL *p) const {
        X509_CRL_free(p);
    }
};

using UniqueX509         = std::unique_ptr<X509, X509Deleter>;
using UniqueX509Store    = std::unique_ptr<X509_STORE, X509StoreDeleter>;
using UniqueX509StoreCtx = std::unique_ptr<X509_STORE_CTX, X509StoreCTXDeleter>;
using UniqueBIO          = std::unique_ptr<BIO, BIODeleter>;
using UniqueX509CRL      = std::unique_ptr<X509_CRL, X509CRLDeleter>;

// Collect the last OpenSSL error string
std::string opensslError() {
    char buf[256];
    ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
    return buf;
}

// Parse PEM certificate; returns nullptr on failure
UniqueX509 parsePEM(const std::string &pem) {
    UniqueBIO bio(BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size())));
    if (!bio) {
        return nullptr;
    }
    return UniqueX509(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
}

} // anonymous namespace

// ============================================================================
// PIMPL
// ============================================================================

struct MTLSAuthenticator::Impl {
    UniqueX509Store ca_store;
    UniqueX509CRL crl;
};

// ============================================================================
// Construction / destruction
// ============================================================================

MTLSAuthenticator::MTLSAuthenticator(const Config &config) : config_(config), impl_(std::make_unique<Impl>()) {
    if (config_.verify_chain && config_.ca_cert_pem.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "mTLS authenticator configuration error",
                                      "ca_cert_pem must be set when verify_chain is true"));
    }

    if (!config_.ca_cert_pem.empty() && !initCAStore()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "mTLS authenticator configuration error",
                                      "Failed to initialise CA certificate store"));
    }

    if (!config_.crl_pem.empty() && !initCRL()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "mTLS authenticator configuration error",
                                      "Failed to parse CRL"));
    }
}

MTLSAuthenticator::~MTLSAuthenticator() = default;

// ============================================================================
// Private initialisation helpers
// ============================================================================

bool MTLSAuthenticator::initCAStore() {
    impl_->ca_store.reset(X509_STORE_new());
    if (!impl_->ca_store) {
        spdlog::error("MTLSAuthenticator: X509_STORE_new failed");
        return false;
    }

    // Load one or more PEM-encoded CA certs from the concatenated PEM string
    UniqueBIO bio(BIO_new_mem_buf(config_.ca_cert_pem.data(), static_cast<int>(config_.ca_cert_pem.size())));
    if (!bio) {
        return false;
    }

    bool loaded_any = false;
    while (true) {
        UniqueX509 ca(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
        if (!ca) {
            break; // end of PEM stream
        }
        if (X509_STORE_add_cert(impl_->ca_store.get(), ca.get()) != 1) {
            spdlog::warn("MTLSAuthenticator: failed to add CA cert to store: {}", opensslError());
        } else {
            loaded_any = true;
        }
    }

    if (!loaded_any) {
        spdlog::error("MTLSAuthenticator: no CA certificates could be loaded");
        return false;
    }

    X509_STORE_set_flags(impl_->ca_store.get(), X509_V_FLAG_X509_STRICT);
    return true;
}

bool MTLSAuthenticator::initCRL() {
    UniqueBIO bio(BIO_new_mem_buf(config_.crl_pem.data(), static_cast<int>(config_.crl_pem.size())));
    if (!bio) {
        return false;
    }

    impl_->crl.reset(PEM_read_bio_X509_CRL(bio.get(), nullptr, nullptr, nullptr));
    if (!impl_->crl) {
        spdlog::error("MTLSAuthenticator: failed to parse CRL: {}", opensslError());
        return false;
    }
    return true;
}

// ============================================================================
// Core authentication
// ============================================================================

MTLSClaims MTLSAuthenticator::authenticate(const std::string &cert_pem) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Step 1: parse certificate
    UniqueX509 cert = parsePEM(cert_pem);
    if (!cert) {
        throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_INVALID, "Certificate authentication failed",
                                      "Failed to parse client certificate PEM: " + opensslError()));
    }

    // Step 2: chain verification
    if (config_.verify_chain && impl_->ca_store) {
        UniqueX509StoreCtx ctx(X509_STORE_CTX_new());
        if (!ctx) {
            throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Certificate authentication failed",
                                          "X509_STORE_CTX_new failed"));
        }

        if (X509_STORE_CTX_init(ctx.get(), impl_->ca_store.get(), cert.get(), nullptr) != 1) {
            throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Certificate authentication failed",
                                          "X509_STORE_CTX_init failed: " + opensslError()));
        }

        if (X509_verify_cert(ctx.get()) != 1) {
            int err = X509_STORE_CTX_get_error(ctx.get());
            throw AuthException(
                AuthError(AuthErrorCode::MTLS_CERT_INVALID, "Certificate authentication failed",
                          std::string("Certificate chain verification failed: ") + X509_verify_cert_error_string(err)));
        }
    }

    // Step 3: validity window
    const ASN1_TIME *not_before_asn1 = X509_get0_notBefore(cert.get());
    const ASN1_TIME *not_after_asn1  = X509_get0_notAfter(cert.get());

    int day = 0, sec = 0;
    // Check not-before: positive day/sec means "in the future" (cert not yet valid)
    if (!ASN1_TIME_diff(&day, &sec, nullptr, not_before_asn1) || day > 0 || sec > 0) {
        throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_EXPIRED, "Certificate not yet valid",
                                      "Certificate not-before is in the future"));
    }
    // Check not-after: negative day/sec means "in the past" (cert expired)
    if (!ASN1_TIME_diff(&day, &sec, nullptr, not_after_asn1) || day < 0 || sec < 0) {
        throw AuthException(
            AuthError(AuthErrorCode::MTLS_CERT_EXPIRED, "Certificate has expired", "Certificate not-after has passed"));
    }

    // Step 4: CRL check
    if (config_.check_revocation && impl_->crl) {
        const ASN1_INTEGER *serial_asn1 = X509_get0_serialNumber(cert.get());
        X509_REVOKED *revoked_entry     = nullptr;
        if (X509_CRL_get0_by_serial(impl_->crl.get(), &revoked_entry, const_cast<ASN1_INTEGER *>(serial_asn1)) == 1) {
            throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_REVOKED, "Certificate has been revoked",
                                          "Certificate serial found in CRL"));
        }
    }

    // Step 5: runtime revocation set
    std::string serial_hex = serialToHex(const_cast<ASN1_INTEGER *>(X509_get0_serialNumber(cert.get())));
    if (config_.check_revocation && revoked_serials_.count(serial_hex)) {
        if (audit_logger_) audit_logger_->logMTLSFailure("certificate_revoked:" + serial_hex);
        throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_REVOKED, "Certificate has been revoked",
                                      "Certificate serial " + serial_hex + " is in the runtime revocation list"));
    }

    // Step 5a: Extended Key Usage — require id-kp-clientAuth (C2)
    {
        auto* eku = static_cast<EXTENDED_KEY_USAGE*>(
            X509_get_ext_d2i(cert.get(), NID_ext_key_usage, nullptr, nullptr));
        if (eku) {
            bool found_client_auth = false;
            for (int i = 0; i < sk_ASN1_OBJECT_num(eku); ++i) {
                if (OBJ_obj2nid(sk_ASN1_OBJECT_value(eku, i)) == NID_client_auth) {
                    found_client_auth = true;
                    break;
                }
            }
            EXTENDED_KEY_USAGE_free(eku);
            if (!found_client_auth) {
                if (audit_logger_) audit_logger_->logMTLSFailure("missing_id-kp-clientAuth_EKU");
                throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_INVALID,
                                              "Certificate missing required Extended Key Usage",
                                              "Certificate does not have id-kp-clientAuth EKU"));
            }
        }
    }

    // Step 6: extract identity fields
    MTLSClaims claims;
    claims.serial_number       = serial_hex;
    claims.subject_dn          = x509NameToString(X509_get_subject_name(cert.get()));
    claims.issuer_dn           = x509NameToString(X509_get_issuer_name(cert.get()));
    claims.fingerprint_sha256  = computeFingerprint(cert.get());
    claims.san_dns_names       = extractSANs(cert.get(), GEN_DNS);
    claims.san_ip_addresses    = extractSANs(cert.get(), GEN_IPADD);
    claims.san_email_addresses = extractSANs(cert.get(), GEN_EMAIL);

    // Resolve not_before / not_after as time_point
    {
        auto asn1ToTimePoint = [](const ASN1_TIME *asn1_time) {
            struct tm tm_val = {};
            ASN1_TIME_to_tm(asn1_time, &tm_val);
#if defined(_WIN32)
            return std::chrono::system_clock::from_time_t(_mkgmtime(&tm_val));
#else
            return std::chrono::system_clock::from_time_t(timegm(&tm_val));
#endif
        };
        claims.not_before = asn1ToTimePoint(not_before_asn1);
        claims.not_after  = asn1ToTimePoint(not_after_asn1);
    }

    // Step 7: resolve principal (first email SAN, otherwise CN from subject)
    if (!claims.san_email_addresses.empty()) {
        claims.principal = claims.san_email_addresses.front();
    } else {
        // Extract CN from Subject DN
        X509_NAME *subj = X509_get_subject_name(cert.get());
        int idx         = X509_NAME_get_index_by_NID(subj, NID_commonName, -1);
        if (idx >= 0) {
            X509_NAME_ENTRY *entry = X509_NAME_get_entry(subj, idx);
            ASN1_STRING *data      = X509_NAME_ENTRY_get_data(entry);
            unsigned char *utf8    = nullptr;
            int len                = ASN1_STRING_to_UTF8(&utf8, data);
            if (len > 0 && utf8) {
                claims.principal = std::string(reinterpret_cast<char *>(utf8), static_cast<size_t>(len));
                OPENSSL_free(utf8);
            }
        }
    }

    spdlog::debug("MTLSAuthenticator: authenticated principal='{}' serial={}", claims.principal, claims.serial_number);
    if (audit_logger_) audit_logger_->logMTLSSuccess(claims.principal, claims.serial_number);
    return claims;
}

MTLSClaims MTLSAuthenticator::authenticateDER(const std::vector<uint8_t> &cert_der) {
    // Convert DER to PEM
    UniqueBIO der_bio(BIO_new_mem_buf(cert_der.data(), static_cast<int>(cert_der.size())));
    if (!der_bio) {
        throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_INVALID, "Certificate authentication failed",
                                      "Failed to create BIO for DER input"));
    }

    UniqueX509 cert(d2i_X509_bio(der_bio.get(), nullptr));
    if (!cert) {
        throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_INVALID, "Certificate authentication failed",
                                      "Failed to parse DER certificate: " + opensslError()));
    }

    // Convert to PEM
    UniqueBIO pem_bio(BIO_new(BIO_s_mem()));
    if (!pem_bio || PEM_write_bio_X509(pem_bio.get(), cert.get()) != 1) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Certificate authentication failed",
                                      "Failed to convert DER to PEM"));
    }

    BUF_MEM *bptr = nullptr;
    BIO_get_mem_ptr(pem_bio.get(), &bptr);
    if (!bptr || !bptr->data || bptr->length == 0) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_INTERNAL_ERROR, "Certificate authentication failed",
                                      "Failed to extract PEM buffer from OpenSSL BIO"));
    }
    std::string pem(bptr->data, bptr->length);
    return authenticate(pem);
}

// ============================================================================
// Runtime revocation management
// ============================================================================

void MTLSAuthenticator::revokeCertificate(const std::string &serial_hex) {
    if (serial_hex.empty()) {
        throw AuthException(
            AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "mTLS revocation error", "serial_hex must not be empty"));
    }
    std::lock_guard<std::mutex> lock(mutex_);
    revoked_serials_.insert(serial_hex);
}

void MTLSAuthenticator::unrevokeCertificate(const std::string &serial_hex) {
    std::lock_guard<std::mutex> lock(mutex_);
    revoked_serials_.erase(serial_hex);
}

bool MTLSAuthenticator::isRevoked(const std::string &serial_hex) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return revoked_serials_.count(serial_hex) > 0;
}

size_t MTLSAuthenticator::revokedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return revoked_serials_.size();
}

// ============================================================================
// Static utility helpers
// ============================================================================

std::string MTLSAuthenticator::certFingerprint(const std::string &cert_pem) {
    UniqueX509 cert = parsePEM(cert_pem);
    if (!cert) {
        throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_INVALID, "Certificate parse error",
                                      "certFingerprint: failed to parse PEM: " + opensslError()));
    }
    return computeFingerprint(cert.get());
}

std::string MTLSAuthenticator::extractSubjectCN(const std::string &cert_pem) {
    UniqueX509 cert = parsePEM(cert_pem);
    if (!cert) {
        throw AuthException(AuthError(AuthErrorCode::MTLS_CERT_INVALID, "Certificate parse error",
                                      "extractSubjectCN: failed to parse PEM: " + opensslError()));
    }

    X509_NAME *subj = X509_get_subject_name(cert.get());
    int idx         = X509_NAME_get_index_by_NID(subj, NID_commonName, -1);
    if (idx < 0) {
        return {};
    }

    X509_NAME_ENTRY *entry = X509_NAME_get_entry(subj, idx);
    ASN1_STRING *data      = X509_NAME_ENTRY_get_data(entry);
    unsigned char *utf8    = nullptr;
    int len                = ASN1_STRING_to_UTF8(&utf8, data);
    if (len <= 0 || !utf8) {
        return {};
    }

    std::string cn(reinterpret_cast<char *>(utf8), static_cast<size_t>(len));
    OPENSSL_free(utf8);
    return cn;
}

// ============================================================================
// Private static helpers
// ============================================================================

std::string MTLSAuthenticator::x509NameToString(void *name_ptr) {
    X509_NAME *name = static_cast<X509_NAME *>(name_ptr);
    if (!name) {
        return {};
    }

    UniqueBIO bio(BIO_new(BIO_s_mem()));
    if (!bio) {
        return {};
    }

    X509_NAME_print_ex(bio.get(), name, 0, XN_FLAG_RFC2253);
    BUF_MEM *bptr = nullptr;
    BIO_get_mem_ptr(bio.get(), &bptr);
    if (!bptr || !bptr->data || bptr->length == 0) {
        return {};
    }
    return std::string(bptr->data, bptr->length);
}

std::string MTLSAuthenticator::serialToHex(void *serial_ptr) {
    ASN1_INTEGER *serial = static_cast<ASN1_INTEGER *>(serial_ptr);
    if (!serial) {
        return {};
    }

    std::unique_ptr<BIGNUM, decltype(&BN_free)> bn(ASN1_INTEGER_to_BN(serial, nullptr), BN_free);
    if (!bn) {
        return {};
    }

    char *hex_raw = BN_bn2hex(bn.get());
    if (!hex_raw) {
        return {};
    }

    std::string result(hex_raw);
    OPENSSL_free(hex_raw);

    // Normalise to lowercase
    for (auto &c : result) {
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

std::string MTLSAuthenticator::computeFingerprint(void *x509_ptr) {
    X509 *cert = static_cast<X509 *>(x509_ptr);
    if (!cert) {
        return {};
    }

    unsigned char digest[SHA256_DIGEST_LENGTH];
    unsigned int digest_len = 0;
    if (X509_digest(cert, EVP_sha256(), digest, &digest_len) != 1) {
        return {};
    }

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i) {
        oss << std::setw(2) << static_cast<int>(digest[i]);
    }
    return oss.str();
}

std::vector<std::string> MTLSAuthenticator::extractSANs(void *x509_ptr, int san_type) {
    X509 *cert = static_cast<X509 *>(x509_ptr);
    std::vector<std::string> result;
    if (!cert) {
        return result;
    }

    auto *san_names = static_cast<GENERAL_NAMES *>(X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr));
    if (!san_names) {
        return result;
    }

    const int count = sk_GENERAL_NAME_num(san_names);
    for (int i = 0; i < count; ++i) {
        GENERAL_NAME *gn = sk_GENERAL_NAME_value(san_names, i);
        if (!gn || gn->type != san_type) {
            continue;
        }

        if (san_type == GEN_DNS || san_type == GEN_EMAIL) {
            const unsigned char *data = ASN1_STRING_get0_data(gn->d.ia5);
            const int len             = ASN1_STRING_length(gn->d.ia5);
            if (data && len > 0) {
                result.emplace_back(reinterpret_cast<const char *>(data), static_cast<size_t>(len));
            }
        } else if (san_type == GEN_IPADD) {
            // IPv4: 4 bytes, IPv6: 16 bytes
            const unsigned char *data = ASN1_STRING_get0_data(gn->d.iPAddress);
            const int len             = ASN1_STRING_length(gn->d.iPAddress);
            if (data && len == 4) {
                std::ostringstream oss;
                oss << static_cast<int>(data[0]) << '.' << static_cast<int>(data[1]) << '.' << static_cast<int>(data[2])
                    << '.' << static_cast<int>(data[3]);
                result.push_back(oss.str());
            } else if (data && len == 16) {
                std::ostringstream oss;
                oss << std::hex;
                for (int b = 0; b < 16; b += 2) {
                    if (b > 0) {
                        oss << ':';
                    }
                    oss << static_cast<int>(data[b]) * 256 + static_cast<int>(data[b + 1]);
                }
                result.push_back(oss.str());
            }
        }
    }

    GENERAL_NAMES_free(san_names);
    return result;
}

} // namespace auth
} // namespace themis
