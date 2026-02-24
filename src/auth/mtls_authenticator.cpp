/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mtls_authenticator.cpp                             ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-24                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     532                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "auth/mtls_authenticator.h"
#include "utils/logger.h"
#include "utils/audit_logger.h"

#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/bn.h>

#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>

namespace themis {
namespace auth {

// ============================================================================
// Internal PIMPL implementation
// ============================================================================

struct MTLSAuthenticator::Impl {
    X509_STORE* ca_store = nullptr;

    Impl() = default;

    ~Impl() {
        if (ca_store) {
            X509_STORE_free(ca_store);
            ca_store = nullptr;
        }
    }

    // Non-copyable
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
};

// ============================================================================
// Helpers
// ============================================================================

namespace {

/** Collect the OpenSSL error queue into a human-readable string. */
std::string opensslErrors() {
    std::ostringstream oss;
    unsigned long err = 0;
    bool first = true;
    while ((err = ERR_get_error()) != 0) {
        char buf[256];
        ERR_error_string_n(err, buf, sizeof(buf));
        if (!first) oss << "; ";
        oss << buf;
        first = false;
    }
    return oss.str();
}

/** RAII wrapper for BIO* */
struct BIOPtr {
    BIO* ptr;
    explicit BIOPtr(BIO* p) : ptr(p) {}
    ~BIOPtr() { if (ptr) BIO_free(ptr); }
    BIOPtr(const BIOPtr&) = delete;
    BIOPtr& operator=(const BIOPtr&) = delete;
    operator BIO*() const { return ptr; }
};

/** RAII wrapper for X509* */
struct X509Ptr {
    X509* ptr;
    explicit X509Ptr(X509* p) : ptr(p) {}
    ~X509Ptr() { if (ptr) X509_free(ptr); }
    X509Ptr(const X509Ptr&) = delete;
    X509Ptr& operator=(const X509Ptr&) = delete;
    operator X509*() const { return ptr; }
    bool ok() const { return ptr != nullptr; }
};

/** RAII wrapper for X509_STORE_CTX* */
struct X509StoreCtxPtr {
    X509_STORE_CTX* ptr;
    explicit X509StoreCtxPtr(X509_STORE_CTX* p) : ptr(p) {}
    ~X509StoreCtxPtr() { if (ptr) X509_STORE_CTX_free(ptr); }
    X509StoreCtxPtr(const X509StoreCtxPtr&) = delete;
    X509StoreCtxPtr& operator=(const X509StoreCtxPtr&) = delete;
    operator X509_STORE_CTX*() const { return ptr; }
};

/** Extract the value of a specific field abbreviation (e.g. "CN") from a subject DN. */
std::string extractDNField(X509_NAME* name, int nid) {
    if (!name) return "";
    int idx = X509_NAME_get_index_by_NID(name, nid, -1);
    if (idx < 0) return "";
    X509_NAME_ENTRY* entry = X509_NAME_get_entry(name, idx);
    if (!entry) return "";
    ASN1_STRING* data = X509_NAME_ENTRY_get_data(entry);
    if (!data) return "";
    unsigned char* utf8 = nullptr;
    int len = ASN1_STRING_to_UTF8(&utf8, data);
    if (len < 0 || !utf8) return "";
    std::string result(reinterpret_cast<char*>(utf8), static_cast<size_t>(len));
    OPENSSL_free(utf8);
    return result;
}

/** Convert an X509_NAME to its one-line string representation (RFC 2253-ish). */
std::string x509NameToString(X509_NAME* name) {
    if (!name) return "";
    BIOPtr bio(BIO_new(BIO_s_mem()));
    if (!bio.ptr) return "";
    X509_NAME_print_ex(bio.ptr, name, 0,
                       XN_FLAG_RFC2253 | ASN1_STRFLGS_UTF8_CONVERT);
    BUF_MEM* bptr = nullptr;
    BIO_get_mem_ptr(bio.ptr, &bptr);
    if (!bptr || !bptr->data) return "";
    return std::string(bptr->data, bptr->length);
}

/** Convert ASN1_INTEGER to a hex serial number string. */
std::string serialToHex(const ASN1_INTEGER* serial) {
    if (!serial) return "";
    BIGNUM* bn = ASN1_INTEGER_to_BN(serial, nullptr);
    if (!bn) return "";
    char* hex = BN_bn2hex(bn);
    std::string result;
    if (hex) {
        result = hex;
        OPENSSL_free(hex);
    }
    BN_free(bn);
    return result;
}

/** Convert ASN1_TIME to system_clock::time_point. */
std::chrono::system_clock::time_point asn1TimeToTimePoint(const ASN1_TIME* t) {
    if (!t) return {};
    struct tm tm_val = {};
    if (ASN1_TIME_to_tm(t, &tm_val) != 1) return {};
#ifdef _WIN32
    time_t tt = _mkgmtime(&tm_val);
#else
    time_t tt = timegm(&tm_val);
#endif
    return std::chrono::system_clock::from_time_t(tt);
}

} // anonymous namespace

// ============================================================================
// MTLSAuthenticator
// ============================================================================

MTLSAuthenticator::MTLSAuthenticator()
    : impl_(std::make_unique<Impl>())
{}

MTLSAuthenticator::~MTLSAuthenticator() = default;

bool MTLSAuthenticator::initialize(const MTLSConfig& config) {
    if (initialized_) {
        THEMIS_WARN("MTLSAuthenticator already initialized");
        return true;
    }

    if (config.ca_cert_path.empty() && config.ca_cert_pem.empty()) {
        THEMIS_ERROR("mTLS: CA certificate is required (set ca_cert_path or ca_cert_pem)");
        return false;
    }

    config_ = config;

    impl_->ca_store = X509_STORE_new();
    if (!impl_->ca_store) {
        THEMIS_ERROR("mTLS: failed to create X509_STORE: {}", opensslErrors());
        return false;
    }

    if (!loadCACertificate()) {
        X509_STORE_free(impl_->ca_store);
        impl_->ca_store = nullptr;
        return false;
    }

    if (!config_.crl_path.empty() && !loadCRL()) {
        X509_STORE_free(impl_->ca_store);
        impl_->ca_store = nullptr;
        return false;
    }

    initialized_ = true;
    THEMIS_INFO("MTLSAuthenticator initialized (principal_field='{}', {} subject mappings)",
                config_.principal_field, config_.subject_mappings.size());
    return true;
}

MTLSClaims MTLSAuthenticator::authenticate(const std::string& cert_pem) {
    if (!initialized_) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_CONFIG_INVALID,
                         "mTLS authentication not configured",
                         "MTLSAuthenticator::authenticate called before initialize()");
    }

    if (cert_pem.size() > MAX_MTLS_CERT_PEM_SIZE) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID,
                         "Certificate too large",
                         "PEM size exceeds MAX_MTLS_CERT_PEM_SIZE");
    }

    // Parse the PEM certificate
    BIOPtr bio(BIO_new_mem_buf(cert_pem.data(), static_cast<int>(cert_pem.size())));
    if (!bio.ptr) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INTERNAL_ERROR,
                         "Certificate parse error",
                         "BIO_new_mem_buf failed: " + opensslErrors());
    }

    X509Ptr cert(PEM_read_bio_X509(bio.ptr, nullptr, nullptr, nullptr));
    if (!cert.ok()) {
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED,
                                            "mTLS", "PEM parse failed", {});
        }
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID,
                         "Invalid certificate format",
                         "PEM_read_bio_X509 failed: " + opensslErrors());
    }

    // Verify certificate chain
    if (!verifyCertificateChain(cert.ptr)) {
        std::string err = opensslErrors();
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED,
                                            "mTLS", "Chain verification failed", {});
        }
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INVALID_CREDENTIALS,
                         "Certificate verification failed",
                         "Chain verify error: " + err);
    }

    // Check expiry
    if (config_.verify_expiry) {
        int day = 0, sec = 0;
        const ASN1_TIME* notAfter = X509_get0_notAfter(cert.ptr);
        if (notAfter && ASN1_TIME_diff(&day, &sec, nullptr, notAfter) == 1) {
            if (day < 0 || (day == 0 && sec < 0)) {
                if (audit_logger_) {
                    audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED,
                                                    "mTLS", "Certificate expired", {});
                }
                THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_EXPIRED,
                                 "Certificate has expired",
                                 "Certificate NotAfter is in the past");
            }
        }
    }

    // Extract Subject DN
    X509_NAME* subjectName = X509_get_subject_name(cert.ptr);
    std::string subject_dn = x509NameToString(subjectName);

    if (subject_dn.size() > MAX_MTLS_SUBJECT_LENGTH) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID,
                         "Certificate subject DN too long",
                         "Subject DN exceeds MAX_MTLS_SUBJECT_LENGTH");
    }

    // Extract Issuer DN
    X509_NAME* issuerName = X509_get_issuer_name(cert.ptr);
    std::string issuer_dn = x509NameToString(issuerName);

    // Extract CN (require if configured)
    std::string cn = extractDNField(subjectName, NID_commonName);
    if (config_.require_subject_cn && cn.empty()) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID,
                         "Certificate missing CN in Subject",
                         "Subject DN has no Common Name: " + subject_dn);
    }

    // Extract principal
    std::string principal = extractPrincipal(subject_dn);
    if (principal.empty()) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID,
                         "Cannot extract principal from certificate",
                         "principal_field='" + config_.principal_field +
                         "' yielded empty result for subject: " + subject_dn);
    }
    if (principal.size() > MAX_MTLS_PRINCIPAL_LENGTH) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID,
                         "Certificate principal too long",
                         "Extracted principal exceeds MAX_MTLS_PRINCIPAL_LENGTH");
    }

    // Map subject to roles
    auto [roles, tenant_id] = mapSubjectToRoles(subject_dn);

    // Build claims
    MTLSClaims claims;
    claims.subject_dn    = subject_dn;
    claims.principal     = principal;
    claims.issuer_dn     = issuer_dn;
    claims.serial_number = serialToHex(X509_get0_serialNumber(cert.ptr));
    claims.not_before    = asn1TimeToTimePoint(X509_get0_notBefore(cert.ptr));
    claims.not_after     = asn1TimeToTimePoint(X509_get0_notAfter(cert.ptr));
    claims.roles         = std::move(roles);
    claims.tenant_id     = std::move(tenant_id);

    THEMIS_INFO("mTLS: authenticated principal='{}' subject='{}' roles={}",
                principal, subject_dn, claims.roles.size());

    if (audit_logger_) {
        audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_SUCCESS,
                                        "mTLS", principal, {});
    }

    return claims;
}

std::pair<std::vector<std::string>, std::string>
MTLSAuthenticator::mapSubjectToRoles(const std::string& subject_dn) const {
    std::vector<std::string> roles;
    std::string tenant_id;

    for (const auto& mapping : config_.subject_mappings) {
        if (subjectMatchesPattern(subject_dn, mapping.subject_pattern)) {
            if (!mapping.role.empty()) {
                roles.push_back(mapping.role);
            }
            if (tenant_id.empty() && !mapping.tenant_id.empty()) {
                tenant_id = mapping.tenant_id;
            }
        }
    }

    return {roles, tenant_id};
}

std::string MTLSAuthenticator::extractPrincipal(const std::string& subject_dn) const {
    if (config_.principal_field == "DN") {
        return subject_dn;
    }

    // Parse CN from the Subject DN string "CN=alice,O=Corp,C=US"
    // We look for "CN=" prefix and take the value up to the next ","
    const std::string prefix = "CN=";
    std::string::size_type pos = subject_dn.find(prefix);
    if (pos == std::string::npos) {
        return "";
    }
    pos += prefix.size();
    auto end = subject_dn.find(',', pos);
    std::string cn = (end == std::string::npos)
                         ? subject_dn.substr(pos)
                         : subject_dn.substr(pos, end - pos);
    // Trim leading/trailing whitespace
    auto first = cn.find_first_not_of(" \t");
    auto last  = cn.find_last_not_of(" \t");
    if (first == std::string::npos) return "";
    return cn.substr(first, last - first + 1);
}

// ============================================================================
// Private helpers
// ============================================================================

bool MTLSAuthenticator::loadCACertificate() {
    if (!impl_->ca_store) return false;

    auto addCertFromBIO = [this](BIO* bio) -> bool {
        X509Ptr ca(PEM_read_bio_X509(bio, nullptr, nullptr, nullptr));
        if (!ca.ok()) {
            THEMIS_ERROR("mTLS: failed to parse CA certificate PEM: {}", opensslErrors());
            return false;
        }
        if (X509_STORE_add_cert(impl_->ca_store, ca.ptr) != 1) {
            // Ignore "already in hash table" errors (error code 11)
            unsigned long err = ERR_peek_last_error();
            if (ERR_GET_REASON(err) != X509_R_CERT_ALREADY_IN_HASH_TABLE) {
                THEMIS_ERROR("mTLS: X509_STORE_add_cert failed: {}", opensslErrors());
                return false;
            }
            ERR_clear_error();
        }
        return true;
    };

    if (!config_.ca_cert_path.empty()) {
        BIOPtr bio(BIO_new_file(config_.ca_cert_path.c_str(), "r"));
        if (!bio.ptr) {
            THEMIS_ERROR("mTLS: cannot open CA cert file '{}': {}",
                         config_.ca_cert_path, opensslErrors());
            return false;
        }
        // Load all certs in the file (PEM bundle support)
        bool loaded_any = false;
        while (true) {
            X509Ptr ca(PEM_read_bio_X509(bio.ptr, nullptr, nullptr, nullptr));
            if (!ca.ok()) {
                ERR_clear_error();
                break;
            }
            if (X509_STORE_add_cert(impl_->ca_store, ca.ptr) != 1) {
                unsigned long err = ERR_peek_last_error();
                if (ERR_GET_REASON(err) != X509_R_CERT_ALREADY_IN_HASH_TABLE) {
                    THEMIS_ERROR("mTLS: X509_STORE_add_cert failed: {}", opensslErrors());
                    return false;
                }
                ERR_clear_error();
            }
            loaded_any = true;
        }
        if (!loaded_any) {
            THEMIS_ERROR("mTLS: no CA certificates found in '{}'", config_.ca_cert_path);
            return false;
        }
        THEMIS_INFO("mTLS: loaded CA certificate(s) from '{}'", config_.ca_cert_path);
    } else {
        BIOPtr bio(BIO_new_mem_buf(config_.ca_cert_pem.data(),
                                   static_cast<int>(config_.ca_cert_pem.size())));
        if (!bio.ptr || !addCertFromBIO(bio.ptr)) {
            return false;
        }
        THEMIS_INFO("mTLS: loaded CA certificate from inline PEM");
    }

    return true;
}

bool MTLSAuthenticator::loadCRL() {
    if (!impl_->ca_store || config_.crl_path.empty()) return true;

    BIOPtr bio(BIO_new_file(config_.crl_path.c_str(), "r"));
    if (!bio.ptr) {
        THEMIS_ERROR("mTLS: cannot open CRL file '{}': {}", config_.crl_path, opensslErrors());
        return false;
    }

    X509_CRL* crl = PEM_read_bio_X509_CRL(bio.ptr, nullptr, nullptr, nullptr);
    if (!crl) {
        THEMIS_ERROR("mTLS: failed to parse CRL '{}': {}", config_.crl_path, opensslErrors());
        return false;
    }

    if (X509_STORE_add_crl(impl_->ca_store, crl) != 1) {
        THEMIS_ERROR("mTLS: X509_STORE_add_crl failed: {}", opensslErrors());
        X509_CRL_free(crl);
        return false;
    }
    X509_CRL_free(crl);

    // Enable CRL checking in the store
    X509_STORE_set_flags(impl_->ca_store,
                         X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
    THEMIS_INFO("mTLS: CRL loaded from '{}'", config_.crl_path);
    return true;
}

bool MTLSAuthenticator::verifyCertificateChain(void* x509) const {
    if (!impl_->ca_store || !x509) return false;

    X509StoreCtxPtr ctx(X509_STORE_CTX_new());
    if (!ctx.ptr) return false;

    X509* cert = static_cast<X509*>(x509);
    if (X509_STORE_CTX_init(ctx.ptr, impl_->ca_store, cert, nullptr) != 1) {
        THEMIS_DEBUG("mTLS: X509_STORE_CTX_init failed: {}", opensslErrors());
        return false;
    }

    int rc = X509_verify_cert(ctx.ptr);
    if (rc != 1) {
        int err = X509_STORE_CTX_get_error(ctx.ptr);
        THEMIS_WARN("mTLS: certificate verification failed: {}",
                    X509_verify_cert_error_string(err));
        return false;
    }
    return true;
}

// static
bool MTLSAuthenticator::subjectMatchesPattern(const std::string& subject_dn,
                                               const std::string& pattern) {
    // Simple wildcard matching: '*' matches any sequence of characters
    const char* s = subject_dn.c_str();
    const char* p = pattern.c_str();

    const char* star_pos   = nullptr;
    const char* match_pos  = s;

    while (*s) {
        if (*p == '*') {
            star_pos  = p++;
            match_pos = s;
        } else if (*p == *s) {
            ++p;
            ++s;
        } else if (star_pos) {
            p = star_pos + 1;
            s = ++match_pos;
        } else {
            return false;
        }
    }
    while (*p == '*') ++p;
    return *p == '\0';
}

} // namespace auth
} // namespace themis
