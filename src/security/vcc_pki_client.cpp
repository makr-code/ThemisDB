/**
 * @file vcc_pki_client.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/vcc_pki_client.h"
#include <curl/curl.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <memory>

namespace themis {

namespace {

// ── RAII Wrappers for OpenSSL objects ─────────────────────────────────────────
struct BIO_Deleter {
    void operator()(BIO* p) const { if (p) BIO_free(p); }
};
struct X509_Deleter {
    void operator()(X509* p) const { if (p) X509_free(p); }
};
struct X509_STORE_Deleter {
    void operator()(X509_STORE* p) const { if (p) X509_STORE_free(p); }
};
struct X509_STORE_CTX_Deleter {
    void operator()(X509_STORE_CTX* p) const { if (p) X509_STORE_CTX_free(p); }
};
struct BIGNUM_Deleter {
    void operator()(BIGNUM* p) const { if (p) BN_free(p); }
};
struct OPENSSL_Free_Deleter {
    void operator()(char* p) const { if (p) OPENSSL_free(p); }
};

using BIO_ptr = std::unique_ptr<BIO, BIO_Deleter>;
using X509_ptr = std::unique_ptr<X509, X509_Deleter>;
using X509_STORE_ptr = std::unique_ptr<X509_STORE, X509_STORE_Deleter>;
using X509_STORE_CTX_ptr = std::unique_ptr<X509_STORE_CTX, X509_STORE_CTX_Deleter>;
using BIGNUM_ptr = std::unique_ptr<BIGNUM, BIGNUM_Deleter>;
using OPENSSL_string_ptr = std::unique_ptr<char, OPENSSL_Free_Deleter>;

bool starts_with(const std::string& value, const char* prefix) {
    return value.rfind(prefix, 0) == 0;
}

std::string extract_url_host(const std::string& url) {
    const size_t scheme_pos = url.find("://");
    const size_t host_start = (scheme_pos == std::string::npos) ? 0 : scheme_pos + 3;
    if (host_start >= static_cast<int>(url.size())) {
        return {};
    }

    if (url[host_start] == '[') {
        const size_t end_bracket = url.find(']', host_start + 1);
        if (end_bracket == std::string::npos) {
            return {};
        }
        return url.substr(host_start + 1, end_bracket - host_start - 1);
    }

    const size_t host_end = url.find_first_of(":/", host_start);
    return url.substr(host_start, host_end == std::string::npos ? std::string::npos
                                                                : host_end - host_start);
}

bool is_loopback_host(const std::string& host) {
    return host == "localhost" || host == "127.0.0.1" || host == "::1";
}

bool is_loopback_url(const std::string& url) {
    return is_loopback_host(extract_url_host(url));
}

void validate_transport_config(const std::string& base_url, const TLSConfig& tls_config, int timeout_ms) {
    if (base_url.empty()) {
        throw std::invalid_argument("PKI base URL must not be empty");
    }
    if (timeout_ms <= 0) {
        throw std::invalid_argument("PKI timeout must be greater than zero");
    }

    const bool uses_https = starts_with(base_url, "https://");
    const bool uses_http = starts_with(base_url, "http://");
    const bool loopback = is_loopback_url(base_url);

    if (!uses_https && !(uses_http && loopback)) {
        throw std::invalid_argument(
            "PKI endpoint must use HTTPS; plain HTTP is only allowed for loopback development endpoints");
    }
    if (!tls_config.verify_server && !loopback) {
        throw std::invalid_argument(
            "PKI TLS verification may only be disabled for loopback development endpoints");
    }
}

} // anonymous namespace

// Helper function to convert ASN1_TIME to milliseconds since epoch
static int64_t asn1_time_to_milliseconds(const ASN1_TIME* asn1_time) {
    if (!asn1_time) {
        return 0;
    }
    
    struct tm time_tm;
    std::memset(&time_tm, 0, sizeof(time_tm));
    
    // Convert ASN1_TIME to struct tm
    if (ASN1_TIME_to_tm(asn1_time, &time_tm) != 1) {
        return 0;
    }
    
    // Convert struct tm to time_t (seconds since epoch)
    // Platform-specific: timegm (POSIX) vs _mkgmtime (Windows)
#ifdef _WIN32
    time_t time_seconds = _mkgmtime(&time_tm);
#else
    time_t time_seconds = timegm(&time_tm);
#endif
    
    if (time_seconds == -1) {
        return 0;
    }
    
    // Convert seconds to milliseconds
    return static_cast<int64_t>(time_seconds) * 1000;
}

// ────────────────────────────────────────────────────────────────────────────
// X509Certificate Implementation
// ────────────────────────────────────────────────────────────────────────────

bool X509Certificate::isValid() const {
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    return now >= not_before_ms && now <= not_after_ms;
}

bool X509Certificate::isExpired(int64_t now_ms) const {
    return now_ms > not_after_ms;
}

nlohmann::json X509Certificate::toJson() const {
    return {
        {"id", id},
        {"pem", pem},
        {"subject", subject},
        {"issuer", issuer},
        {"not_before_ms", not_before_ms},
        {"not_after_ms", not_after_ms},
        {"key_usage", key_usage},
        {"san", san}
    };
}

X509Certificate X509Certificate::fromJson(const nlohmann::json& j) {
    X509Certificate cert;
    cert.id = j.value("id", "");
    cert.pem = j.value("pem", "");
    cert.subject = j.value("subject", "");
    cert.issuer = j.value("issuer", "");
    cert.not_before_ms = j.value("not_before_ms", static_cast<int64_t>(0));
    cert.not_after_ms = j.value("not_after_ms", static_cast<int64_t>(0));
    cert.key_usage = j.value("key_usage", "");
    cert.san = j.value("san", std::vector<std::string>{});
    return cert;
}

// ────────────────────────────────────────────────────────────────────────────
// CRLEntry Implementation
// ────────────────────────────────────────────────────────────────────────────

nlohmann::json CRLEntry::toJson() const {
    return {
        {"serial_number", serial_number},
        {"revocation_time_ms", revocation_time_ms},
        {"reason", reason}
    };
}

CRLEntry CRLEntry::fromJson(const nlohmann::json& j) {
    CRLEntry entry;
    entry.serial_number = j.value("serial_number", "");
    entry.revocation_time_ms = j.value("revocation_time_ms", static_cast<int64_t>(0));
    entry.reason = j.value("reason", "");
    return entry;
}

// ────────────────────────────────────────────────────────────────────────────
// CertificateRequest Implementation
// ────────────────────────────────────────────────────────────────────────────

nlohmann::json CertificateRequest::toJson() const {
    return {
        {"common_name", common_name},
        {"organization", organization},
        {"san", san},
        {"key_usage", key_usage},
        {"validity_days", validity_days}
    };
}

// ────────────────────────────────────────────────────────────────────────────
// VCCPKIClient::Impl - HTTP Client using libcurl
// ────────────────────────────────────────────────────────────────────────────

struct VCCPKIClient::Impl {
    CURL* curl;
    struct curl_slist* headers;
    std::string ca_cert_path = {};
    std::string client_cert_path = {};
    std::string client_key_path = {};
    bool verify_server = {};
    bool use_mtls = {};
    
    Impl() : curl(nullptr), headers(nullptr), verify_server(true), use_mtls(false) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("Failed to initialize CURL");
        }
        
        // Default headers
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");
    }
    
    ~Impl() {
        if (headers) {
            curl_slist_free_all(headers);
        }
        if (curl) {
            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
    }
    
    // Callback for reading response data
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        size_t total_size = size * nmemb;
        userp->append(static_cast<char*>(contents), total_size);
        return total_size;
    }
    
    std::string execute(const std::string& url, const std::string& method, const std::string& body, int timeout_ms) {
        std::string response;
        
        // Reset CURL for new request
        curl_easy_reset(curl);
        
        // Set URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        // Set timeout
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms / 2);
        
        // Set headers
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        // Set write callback
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        // TLS/SSL configuration
        if (verify_server) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            
            if (!ca_cert_path.empty()) {
                curl_easy_setopt(curl, CURLOPT_CAINFO, ca_cert_path.c_str());
            }
        } else {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        
        // mTLS (Mutual TLS)
        if (use_mtls) {
            if (client_cert_path.empty() || client_key_path.empty()) {
                throw std::runtime_error("mTLS enabled but client cert/key not provided");
            }
            curl_easy_setopt(curl, CURLOPT_SSLCERT, client_cert_path.c_str());
            curl_easy_setopt(curl, CURLOPT_SSLKEY, client_key_path.c_str());
        }
        
        // HTTP method
        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,static_cast<int>(body.size()));
        } else if (method == "GET") {
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        }
        
        // Execute request
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::string error = "CURL request failed: ";
            error += curl_easy_strerror(res);
            throw std::runtime_error(error);
        }
        
        // Check HTTP status code
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (http_code < 200 || http_code >= 300) {
            std::ostringstream oss = {};
            oss << "HTTP error " << http_code << ": " << response;
            throw std::runtime_error(oss.str());
        }
        
        return response;
    }
};

// ────────────────────────────────────────────────────────────────────────────
// VCCPKIClient Implementation
// ────────────────────────────────────────────────────────────────────────────

VCCPKIClient::VCCPKIClient(
    const std::string& base_url,
    const TLSConfig& tls_config,
    int timeout_ms
)
    : base_url_(base_url)
    , tls_config_(tls_config)
    , timeout_ms_(timeout_ms)
    , impl_(std::make_unique<Impl>())
{
    validate_transport_config(base_url_, tls_config_, timeout_ms_);

    // Configure TLS
    impl_->ca_cert_path = tls_config.ca_cert_path;
    impl_->client_cert_path = tls_config.client_cert_path;
    impl_->client_key_path = tls_config.client_key_path;
    impl_->verify_server = tls_config.verify_server;
    impl_->use_mtls = tls_config.use_mtls;
}

VCCPKIClient::~VCCPKIClient() = default;

VCCPKIClient::VCCPKIClient(VCCPKIClient&&) noexcept = default;
VCCPKIClient& VCCPKIClient::operator=(VCCPKIClient&&) noexcept = default;

std::string VCCPKIClient::httpGet(const std::string& path) {
    std::string url = base_url_ + path;
    return impl_->execute(url, "GET", "", timeout_ms_);
}

std::string VCCPKIClient::httpPost(const std::string& path, const nlohmann::json& body) {
    std::string url = base_url_ + path;
    std::string body_str = body.dump();
    return impl_->execute(url, "POST", body_str, timeout_ms_);
}

X509Certificate VCCPKIClient::requestCertificate(const CertificateRequest& request) {
    nlohmann::json body = request.toJson();
    
    std::string response = httpPost("/api/v1/certificates/request", body);
    
    nlohmann::json response_json = nlohmann::json::parse(response);
    
    return X509Certificate::fromJson(response_json);
}

X509Certificate VCCPKIClient::getCertificate(const std::string& cert_id) {
    std::string path = "/api/v1/certificates/" + cert_id;
    
    std::string response = httpGet(path);
    
    nlohmann::json response_json = nlohmann::json::parse(response);
    
    return X509Certificate::fromJson(response_json);
}

std::vector<CRLEntry> VCCPKIClient::getCRL() {
    std::string response = httpGet("/api/v1/crl");
    
    nlohmann::json response_json = nlohmann::json::parse(response);
    
    std::vector<CRLEntry> crl;
    
    if (response_json.contains("revoked_certificates")) {
        for (const auto& entry_json : response_json["revoked_certificates"]) {
            crl.push_back(CRLEntry::fromJson(entry_json));
        }
    }
    
    return crl;
}

bool VCCPKIClient::isRevoked(const std::string& cert_id, const std::vector<CRLEntry>& crl) const {
    return std::any_of(crl.begin(), crl.end(), [&cert_id](const CRLEntry& entry) {
        return entry.serial_number == cert_id;
    });
}

bool VCCPKIClient::healthCheck() {
    try {
        std::string response = httpGet("/api/v1/health");
        nlohmann::json response_json = nlohmann::json::parse(response);
        return response_json.value("status", "") == "ok";
    } catch (const std::exception&) {
        return false;
    }
}

X509Certificate VCCPKIClient::parseCertificate(const std::string& pem) {
    X509Certificate cert;
    
    // Parse PEM using OpenSSL
    BIO_ptr bio(BIO_new_mem_buf(pem.c_str(), static_cast<int>(pem.size())));
    if (!bio) {
        throw std::runtime_error("Failed to create BIO from PEM");
    }
    
    X509_ptr x509(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
    
    if (!x509) {
        throw std::runtime_error("Failed to parse X.509 certificate");
    }
    
    // Extract serial number (ID)
    ASN1_INTEGER* serial = X509_get_serialNumber(x509.get());
    BIGNUM_ptr bn(ASN1_INTEGER_to_BN(serial, nullptr));
    if (!bn) {
        throw std::runtime_error("Failed to convert certificate serial number");
    }

    OPENSSL_string_ptr hex(BN_bn2hex(bn.get()));
    if (!hex) {
        throw std::runtime_error("Failed to convert serial number to hex");
    }

    cert.id = hex.get();
    
    // Extract subject
    X509_NAME* subject = X509_get_subject_name(x509.get());
    char subject_buf[256];
    X509_NAME_oneline(subject, subject_buf, sizeof(subject_buf));
    cert.subject = subject_buf;
    
    // Extract issuer
    X509_NAME* issuer = X509_get_issuer_name(x509.get());
    char issuer_buf[256];
    X509_NAME_oneline(issuer, issuer_buf, sizeof(issuer_buf));
    cert.issuer = issuer_buf;
    
    // Extract validity period and convert to milliseconds
    const ASN1_TIME* not_before = X509_get0_notBefore(x509.get());
    const ASN1_TIME* not_after = X509_get0_notAfter(x509.get());
    
    cert.not_before_ms = asn1_time_to_milliseconds(not_before);
    cert.not_after_ms = asn1_time_to_milliseconds(not_after);
    
    // Plausibility checks for certificate validity
    if (cert.not_before_ms == 0 || cert.not_after_ms == 0) {
        throw std::runtime_error("Failed to parse certificate validity dates");
    }
    
    if (cert.not_before_ms >= cert.not_after_ms) {
        throw std::runtime_error("Invalid certificate validity period: not_before >= not_after");
    }
    
    cert.pem = pem;
    
    return cert;
}

bool VCCPKIClient::validateCertChain(const X509Certificate& cert) const {
    // Load the certificate from PEM
    BIO_ptr cert_bio(BIO_new_mem_buf(cert.pem.data(), static_cast<int>(cert.pem.size())));
    if (!cert_bio) {
        return false;
    }
    
    X509_ptr x509_cert(PEM_read_bio_X509(cert_bio.get(), nullptr, nullptr, nullptr));
    
    if (!x509_cert) {
        return false;
    }
    
    // Check basic expiry first (fast check)
    if (!cert.isValid()) {
        return false;
    }
    
    // Create X509 store and context for chain validation
    X509_STORE_ptr store(X509_STORE_new());
    if (!store) {
        return false;
    }
    
    // Load Root CA certificate if provided
    if (!tls_config_.ca_cert_path.empty()) {
        if (X509_STORE_load_locations(store.get(), tls_config_.ca_cert_path.c_str(), nullptr) != 1) {
            // If loading fails, try default system CA bundle
            X509_STORE_set_default_paths(store.get());
        }
    } else {
        // Use system default CA bundle
        X509_STORE_set_default_paths(store.get());
    }
    
    // Enable CRL checking if configured
    X509_STORE_set_flags(store.get(), X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
    
    // Create store context for verification
    X509_STORE_CTX_ptr ctx(X509_STORE_CTX_new());
    if (!ctx) {
        return false;
    }

    // Initialize context with certificate and store
    if (X509_STORE_CTX_init(ctx.get(), store.get(), x509_cert.get(), nullptr) != 1) {
        return false;
    }

    // Perform the actual verification
    int verify_result = X509_verify_cert(ctx.get());
    bool is_valid = (verify_result == 1);

    // Log verification errors if validation failed
    if (!is_valid) {
        int error = X509_STORE_CTX_get_error(ctx.get());
        (void)X509_verify_cert_error_string(error);
        // Note: In production, log this error for debugging
        // For now, we just fail silently to maintain minimal changes
    }

    return is_valid;
}

} // namespace themis
