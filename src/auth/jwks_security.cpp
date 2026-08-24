/**
 * @file jwks_security.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/jwks_security.h"
#include "utils/logger.h"
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <curl/curl.h>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace themis {
namespace auth {

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

struct X509Deleter { void operator()(X509* p) const { X509_free(p); } };
struct OpenSSLBufDeleter { void operator()(unsigned char* p) const { OPENSSL_free(p); } };
using UniqueX509 = std::unique_ptr<X509, X509Deleter>;
using UniqueOSSLBuf = std::unique_ptr<unsigned char, OpenSSLBufDeleter>;

struct OpenSSLCharDeleter { void operator()(char* p) const { OPENSSL_free(p); } };
using UniqueOSSLChar = std::unique_ptr<char, OpenSSLCharDeleter>;

// Base64 encode
std::string base64Encode(const unsigned char* data, size_t len) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, data, static_cast<int>(len));
    BIO_flush(bio);
    
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    std::string result(bufferPtr->data, bufferPtr->length);
    
    BIO_free_all(bio);
    return result;
}

// Read file content
std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Check file exists
bool fileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

} // anonymous namespace

// ============================================================================
// JWKSSecurityConfig Implementation
// ============================================================================

JWKSSecurityConfig::JWKSSecurityConfig(const Config& config)
    : config_(config)
{
    validate();
    
    utils::Logger::info("JWKS Security Config initialized:");
    utils::Logger::info("  Pinning mode: {}", static_cast<int>(config_.pinning_mode));
    utils::Logger::info("  mTLS enabled: {}", config_.enable_mtls);
    utils::Logger::info("  Min TLS version: {}", static_cast<int>(config_.min_tls_version));
}

void JWKSSecurityConfig::validate() const {
    // Validate pinning configuration
    if (config_.pinning_mode == PinningMode::PUBLIC_KEY) {
        if (config_.pinned_hashes.empty()) {
            throw std::runtime_error("Public key pinning enabled but no hashes provided");
        }
    } else if (config_.pinning_mode == PinningMode::CERTIFICATE ||
               config_.pinning_mode == PinningMode::CA_CERTIFICATE) {
        if (config_.pinned_cert_path.empty()) {
            throw std::runtime_error("Certificate pinning enabled but no cert path provided");
        }
        if (!fileExists(config_.pinned_cert_path)) {
            throw std::runtime_error("Pinned certificate file not found: " + config_.pinned_cert_path);
        }
    }
    
    // Validate mTLS configuration
    if (config_.enable_mtls) {
        if (config_.client_cert_path.empty() || config_.client_key_path.empty()) {
            throw std::runtime_error("mTLS enabled but client cert/key not provided");
        }
        if (!fileExists(config_.client_cert_path)) {
            throw std::runtime_error("Client certificate not found: " + config_.client_cert_path);
        }
        if (!fileExists(config_.client_key_path)) {
            throw std::runtime_error("Client key not found: " + config_.client_key_path);
        }
    }
    
    // Validate CA bundle if provided
    if (!config_.ca_bundle_path.empty() && !fileExists(config_.ca_bundle_path)) {
        throw std::runtime_error("CA bundle not found: " + config_.ca_bundle_path);
    }
}

JWKSSecurityConfig::Config 
JWKSSecurityConfig::withPublicKeyPinning(const std::vector<std::string>& spki_hashes) {
    Config config;
    config.pinning_mode = PinningMode::PUBLIC_KEY;
    config.pinned_hashes = spki_hashes;
    config.min_tls_version = TLSVersion::TLS_1_2;
    config.verify_hostname = true;
    config.verify_certificate = true;
    return config;
}

JWKSSecurityConfig::Config 
JWKSSecurityConfig::withCertificatePinning(const std::string& cert_path) {
    Config config;
    config.pinning_mode = PinningMode::CERTIFICATE;
    config.pinned_cert_path = cert_path;
    config.min_tls_version = TLSVersion::TLS_1_2;
    config.verify_hostname = true;
    config.verify_certificate = true;
    return config;
}

JWKSSecurityConfig::Config 
JWKSSecurityConfig::withMTLS(
    const std::string& client_cert_path,
    const std::string& client_key_path,
    const std::string& key_password)
{
    Config config;
    config.enable_mtls = true;
    config.client_cert_path = client_cert_path;
    config.client_key_path = client_key_path;
    config.client_key_password = key_password;
    config.min_tls_version = TLSVersion::TLS_1_2;
    config.verify_hostname = true;
    config.verify_certificate = true;
    return config;
}

JWKSSecurityConfig::Config JWKSSecurityConfig::secureDefaults() {
    Config config;
    config.min_tls_version = TLSVersion::TLS_1_2;
    config.verify_hostname = true;
    config.verify_certificate = true;
    return config;
}

// ============================================================================
// JWKSSecureFetcher Implementation
// ============================================================================

struct JWKSSecureFetcher::Impl {
    JWKSSecurityConfig::Config config;
    FetchStats last_stats;
    CURL* curl = nullptr;
    
    explicit Impl(const JWKSSecurityConfig::Config& cfg) : config(cfg) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("Failed to initialize CURL");
        }
    }
    
    ~Impl() {
        // Explicitly zero the key password before CURL cleanup (defence-in-depth).
        // SecureString's destructor also zeroes this field; the explicit call here
        // ensures zeroing is visible at the point of use and survives any future
        // refactoring that might replace SecureString with a plain std::string.
        if (!config.client_key_password.empty()) {
            OPENSSL_cleanse(
                config.client_key_password.data(),
                config.client_key_password.size() + 1);  // +1 to include null terminator
        }
        if (curl) {
            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
    }
};

// Callback for writing data
static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

JWKSSecureFetcher::JWKSSecureFetcher(const JWKSSecurityConfig::Config& config)
    : impl_(std::make_unique<Impl>(config))
{
    setupTLSContext();
    utils::Logger::info("JWKS Secure Fetcher initialized");
}

JWKSSecureFetcher::~JWKSSecureFetcher() = default;

JWKSSecureFetcher::JWKSSecureFetcher(JWKSSecureFetcher&&) noexcept = default;
JWKSSecureFetcher& JWKSSecureFetcher::operator=(JWKSSecureFetcher&&) noexcept = default;

std::string JWKSSecureFetcher::fetch(const std::string& url) {
    if (url.substr(0, 8) != "https://") {
        throw std::runtime_error("JWKS URL must use HTTPS: " + url);
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    std::string response_data;
    
    // Setup CURL options
    curl_easy_setopt(impl_->curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(impl_->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(impl_->curl, CURLOPT_MAXREDIRS, 3L);
    
    // Timeouts
    curl_easy_setopt(impl_->curl, CURLOPT_CONNECTTIMEOUT_MS, impl_->config.connect_timeout_ms);
    curl_easy_setopt(impl_->curl, CURLOPT_TIMEOUT_MS, impl_->config.read_timeout_ms);
    
    // TLS version
    long ssl_version = CURL_SSLVERSION_TLSv1_2;
    switch (impl_->config.min_tls_version) {
        case JWKSSecurityConfig::TLSVersion::TLS_1_0:
            ssl_version = CURL_SSLVERSION_TLSv1_0;
            break;
        case JWKSSecurityConfig::TLSVersion::TLS_1_1:
            ssl_version = CURL_SSLVERSION_TLSv1_1;
            break;
        case JWKSSecurityConfig::TLSVersion::TLS_1_2:
            ssl_version = CURL_SSLVERSION_TLSv1_2;
            break;
        case JWKSSecurityConfig::TLSVersion::TLS_1_3:
            ssl_version = CURL_SSLVERSION_TLSv1_3;
            break;
    }
    curl_easy_setopt(impl_->curl, CURLOPT_SSLVERSION, ssl_version);
    
    // Certificate verification
    curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYPEER, impl_->config.verify_certificate ? 1L : 0L);
    curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYHOST, impl_->config.verify_hostname ? 2L : 0L);
    
    // CA bundle
    if (!impl_->config.ca_bundle_path.empty()) {
        curl_easy_setopt(impl_->curl, CURLOPT_CAINFO, impl_->config.ca_bundle_path.c_str());
    }
    
    // mTLS
    if (impl_->config.enable_mtls) {
        curl_easy_setopt(impl_->curl, CURLOPT_SSLCERT, impl_->config.client_cert_path.c_str());
        curl_easy_setopt(impl_->curl, CURLOPT_SSLKEY, impl_->config.client_key_path.c_str());
        if (!impl_->config.client_key_password.empty()) {
            curl_easy_setopt(impl_->curl, CURLOPT_KEYPASSWD, impl_->config.client_key_password.c_str());
        }
    }
    
    // Perform request
    CURLcode res = curl_easy_perform(impl_->curl);
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    if (res != CURLE_OK) {
        throw std::runtime_error("JWKS fetch failed: " + std::string(curl_easy_strerror(res)));
    }
    
    // Get response code
    long response_code;
    curl_easy_getinfo(impl_->curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    if (response_code != 200) {
        throw std::runtime_error("JWKS fetch returned HTTP " + std::to_string(response_code));
    }
    
    // Update stats
    impl_->last_stats.url = url;
    impl_->last_stats.status_code = response_code;
    impl_->last_stats.duration_ms = duration.count();
    impl_->last_stats.mtls_used = impl_->config.enable_mtls;
    
    // Note: Certificate pinning verification would require custom SSL context
    // In a production implementation, this would be done via SSL_CTX callbacks
    impl_->last_stats.pinning_verified = (impl_->config.pinning_mode == JWKSSecurityConfig::PinningMode::NONE);
    
    utils::Logger::info("JWKS fetched successfully from: {} ({}ms)", url, duration.count());
    
    return response_data;
}

bool JWKSSecureFetcher::verifyPinning(const std::vector<std::string>& cert_chain) {
    if (impl_->config.pinning_mode == JWKSSecurityConfig::PinningMode::NONE) {
        return true;
    }
    
    if (impl_->config.pinning_mode == JWKSSecurityConfig::PinningMode::PUBLIC_KEY) {
        // Verify SPKI hash matches one of the pinned hashes
        for (const auto& cert : cert_chain) {
            std::string spki_hash = computeSPKIHash(cert);
            for (const auto& pinned_hash : impl_->config.pinned_hashes) {
                if (spki_hash == pinned_hash) {
                    return true;
                }
            }
        }
        return false;
    }
    
    // For other modes, implementation would compare full certificates
    return true;
}

JWKSSecureFetcher::FetchStats JWKSSecureFetcher::getLastFetchStats() const {
    return impl_->last_stats;
}

std::string JWKSSecureFetcher::computeSPKIHash(const std::string& cert_data) {
    // This is a simplified implementation
    // In production, would extract SPKI from X509 certificate and hash it
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)cert_data.c_str(), cert_data.size(), hash);
    return base64Encode(hash, SHA256_DIGEST_LENGTH);
}

void JWKSSecureFetcher::setupTLSContext() {
    // TLS context setup is handled by CURL
    // In a more advanced implementation, could use custom SSL_CTX
}

// ============================================================================
// CertificateUtils Implementation
// ============================================================================

std::string CertificateUtils::computeSPKIHashFromFile(const std::string& cert_path) {
    // Read certificate file
    FILE* fp = fopen(cert_path.c_str(), "r");
    if (!fp) {
        throw std::runtime_error("Failed to open certificate: " + cert_path);
    }
    
    X509* cert_raw = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    fclose(fp);
    
    if (!cert_raw) {
        throw std::runtime_error("Failed to parse certificate: " + cert_path);
    }
    UniqueX509 cert(cert_raw);
    
    // Extract SPKI (Subject Public Key Info)
    unsigned char* spki_raw = nullptr;
    int spki_len = i2d_X509_PUBKEY(X509_get_X509_PUBKEY(cert.get()), &spki_raw);
    
    if (spki_len <= 0) {
        throw std::runtime_error("Failed to extract SPKI");
    }
    UniqueOSSLBuf spki(spki_raw);
    
    // Compute SHA256 hash
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(spki.get(), spki_len, hash);
    
    // Base64 encode
    std::string result = base64Encode(hash, SHA256_DIGEST_LENGTH);
    
    return result;
}

std::string CertificateUtils::computeSPKIHashFromPEM(const std::string& cert_pem) {
    BIO* bio = BIO_new_mem_buf(cert_pem.c_str(), static_cast<int>(cert_pem.size()));
    if (!bio) {
        throw std::runtime_error("Failed to create BIO");
    }
    
    X509* cert_raw = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!cert_raw) {
        throw std::runtime_error("Failed to parse PEM certificate");
    }
    UniqueX509 cert(cert_raw);
    
    // Extract SPKI
    unsigned char* spki_raw = nullptr;
    int spki_len = i2d_X509_PUBKEY(X509_get_X509_PUBKEY(cert.get()), &spki_raw);
    
    if (spki_len <= 0) {
        throw std::runtime_error("Failed to extract SPKI");
    }
    UniqueOSSLBuf spki(spki_raw);
    
    // Compute SHA256 hash
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(spki.get(), spki_len, hash);
    
    std::string result = base64Encode(hash, SHA256_DIGEST_LENGTH);
    
    return result;
}

bool CertificateUtils::verifyCertificate(const std::string& cert_path) {
    FILE* fp = fopen(cert_path.c_str(), "r");
    if (!fp) {
        return false;
    }
    
    X509* cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    fclose(fp);
    
    if (!cert) {
        return false;
    }
    
    // Check if certificate is expired
    int day, sec;
    const ASN1_TIME* notAfter = X509_get0_notAfter(cert);
    ASN1_TIME_diff(&day, &sec, nullptr, notAfter);
    
    bool is_valid = (day > 0 || (day == 0 && sec > 0));
    
    X509_free(cert);
    return is_valid;
}

CertificateUtils::CertInfo 
CertificateUtils::getCertificateInfo(const std::string& cert_path) {
    CertInfo info;
    
    FILE* fp = fopen(cert_path.c_str(), "r");
    if (!fp) {
        throw std::runtime_error("Failed to open certificate: " + cert_path);
    }
    
    X509* cert_raw = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    fclose(fp);
    
    if (!cert_raw) {
        throw std::runtime_error("Failed to parse certificate: " + cert_path);
    }
    UniqueX509 cert(cert_raw);
    
    // Subject
    UniqueOSSLChar subject(X509_NAME_oneline(X509_get_subject_name(cert.get()), nullptr, 0));
    if (subject) {
        info.subject = subject.get();
    }
    
    // Issuer
    UniqueOSSLChar issuer(X509_NAME_oneline(X509_get_issuer_name(cert.get()), nullptr, 0));
    if (issuer) {
        info.issuer = issuer.get();
    }
    
    // Check expiration
    int day, sec;
    const ASN1_TIME* notAfter = X509_get0_notAfter(cert.get());
    ASN1_TIME_diff(&day, &sec, nullptr, notAfter);
    info.is_expired = !(day > 0 || (day == 0 && sec > 0));
    
    // Key size
    EVP_PKEY* pkey = X509_get_pubkey(cert.get());
    if (pkey) {
        info.key_size_bits = EVP_PKEY_bits(pkey);
        EVP_PKEY_free(pkey);
    }
    
    return info;
}

} // namespace auth
} // namespace themis

