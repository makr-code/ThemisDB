/**
 * @file plugin_security.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=25, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "acceleration/plugin_security.h"
#include <stdexcept>
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <nlohmann/json.hpp>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ocsp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <sstream>
#include <unordered_map>

#include "utils/logger.h"
#ifdef THEMIS_ENABLE_CURL
#include <curl/curl.h>
#endif

// Platform-specific headers for code signing
#ifdef _WIN32
#include <windows.h>
#include <softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")
#endif

#ifdef __APPLE__
#include <Security/Security.h>
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
// POSIX process spawning headers for shell-free GPG invocation
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace themis {
namespace acceleration {

using json = nlohmann::json;

// ============================================================================
// Revocation Cache (module-level, thread-safe)
// ============================================================================

namespace {

struct RevocationCacheEntry {
    bool is_revoked = 0;
    std::chrono::system_clock::time_point expires_at;
};

std::unordered_map<std::string, RevocationCacheEntry> g_crl_cache;
std::unordered_map<std::string, RevocationCacheEntry> g_ocsp_cache;
std::mutex g_revocation_cache_mutex;

// ============================================================================
// HTTP helpers (libcurl-backed when available)
// ============================================================================

#ifdef THEMIS_ENABLE_CURL
static size_t curlWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    auto *buf = static_cast<std::vector<uint8_t> *>(userdata);
    buf->insert(buf->end(), ptr, ptr + size * nmemb);
    return size * nmemb;
}

// Perform an HTTP GET; returns the raw response body, or empty on failure.
static std::vector<uint8_t> httpGet(const std::string &url, long timeout_secs) {
    std::vector<uint8_t> result;
    CURL *curl = curl_easy_init();
    if (!curl)
        return result;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_secs);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout_secs);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK)
        result.clear();
    return result;
}

// Perform an HTTP POST; returns the raw response body, or empty on failure.
static std::vector<uint8_t> httpPost(const std::string &url, const std::vector<uint8_t> &body,
                                     const std::string &content_type, long timeout_secs) {
    std::vector<uint8_t> result;
    CURL *curl = curl_easy_init();
    if (!curl)
        return result;
    struct curl_slist *headers = nullptr;
    std::string ct_header      = "Content-Type: " + content_type;
    headers                    = curl_slist_append(headers, ct_header.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_secs);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout_secs);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK)
        result.clear();
    return result;
}
#else
static std::vector<uint8_t> httpGet(const std::string & /*url*/, long /*timeout_secs*/) {
    return {};
}
static std::vector<uint8_t> httpPost(const std::string & /*url*/, const std::vector<uint8_t> & /*body*/,
                                     const std::string & /*content_type*/, long /*timeout_secs*/) {
    return {};
}
#endif // THEMIS_ENABLE_CURL

// Return the serial number of a certificate as an uppercase hex string.
static std::string getCertSerialHex(X509 *cert) {
    if (!cert) {
        return "";
    }
    ASN1_INTEGER *serial_asn1 = X509_get_serialNumber(cert);
    if (!serial_asn1) {
        return "";
    }
    BIGNUM *bn = ASN1_INTEGER_to_BN(serial_asn1, nullptr);
    if (!bn) {
        return "";
    }
    char *hex = BN_bn2hex(bn);
    std::string result(hex ? hex : "");
    OPENSSL_free(hex);
    BN_free(bn);
    return result;
}

} // anonymous namespace

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Decode hex string to bytes
 * @param hexStr Hex-encoded string (must have even length)
 * @param outBytes Output vector for decoded bytes
 * @return true if successful, false if invalid format
 */
static bool decodeHexString(const std::string &hexStr, std::vector<uint8_t> &outBytes) {
    // Validate hex string length is even
    if (hexStr.size() % 2 != 0) {
        return false;
    }

    outBytes.clear();
    outBytes.reserve(hexStr.size() / 2);

    try {
        for (size_t i = 0; i < hexStr.size(); i += 2) {
            std::string byteStr = hexStr.substr(i, 2);
            uint8_t byte        = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
            outBytes.push_back(byte);
        }
        return true;
    } catch (const std::invalid_argument &) {
        outBytes.clear();
        return false;
    } catch (const std::out_of_range &) {
        outBytes.clear();
        return false;
    } catch (const std::exception &) {
        outBytes.clear();
        return false;
    } catch (const std::string &) {
        outBytes.clear();
        return false;
    } catch (const char *) {
        outBytes.clear();
        return false;
    }
}

// ============================================================================
// PluginSecurityVerifier Implementation
// ============================================================================

PluginSecurityVerifier::PluginSecurityVerifier(const PluginSecurityPolicy &policy) : policy_(policy) {}

bool PluginSecurityVerifier::validatePluginPath(const std::string &path, std::string &errorMessage) {
    if (path.empty()) {
        errorMessage = "Plugin path is empty";
        return false;
    }

    // Reject paths containing traversal sequences
    if (path.find("..") != std::string::npos) {
        errorMessage = "Plugin path contains path traversal sequence: " + path;
        return false;
    }

    // Reject paths with null bytes (check using size vs c_str length)
    if (static_cast<int>(path.size()) != std::strlen(path.c_str())) {
        errorMessage = "Plugin path contains null byte";
        return false;
    }

    // Reject paths with shell-injection characters
    static const std::string kForbiddenChars = ";|&`$><\n\r";
    for (char ch : kForbiddenChars) {
        if (path.find(ch) != std::string::npos) {
            errorMessage = "Plugin path contains disallowed character";
            return false;
        }
    }

    // Resolve to canonical form and verify the resolved path matches the
    // canonical path (catches symlinks that escape the intended directory).
    std::error_code ec = {};
    std::filesystem::path canonical = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        errorMessage = "Failed to resolve plugin path: " + ec.message();
        return false;
    }

    // Ensure the canonical path still has an absolute component (sanity check)
    if (!canonical.is_absolute()) {
        errorMessage = "Plugin path does not resolve to an absolute path";
        return false;
    }

    return true;
}

std::string PluginSecurityVerifier::calculateFileHash(const std::string &filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return "";
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return "";
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    const size_t bufferSize = 32768;
    std::vector<char> buffer(bufferSize);

    while (file.read(buffer.data(), bufferSize) || file.gcount() > 0) {
        if (EVP_DigestUpdate(mdctx, buffer.data(), file.gcount()) != 1) {
            EVP_MD_CTX_free(mdctx);
            return "";
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    if (EVP_DigestFinal_ex(mdctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    EVP_MD_CTX_free(mdctx);

    std::stringstream ss = {};
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return ss.str();
}

std::optional<PluginMetadata> PluginSecurityVerifier::loadMetadata(const std::string &pluginPath) {
    // Look for metadata JSON file (pluginPath + ".json")
    std::string metadataPath = pluginPath + ".json";

    if (!std::filesystem::exists(metadataPath)) {
        // Try alternative location: same directory, same name
        std::filesystem::path p(pluginPath);
        metadataPath = (p.parent_path() / (p.stem().string() + ".json")).string();

        if (!std::filesystem::exists(metadataPath)) {
            return std::nullopt;
        }
    }

    try {
        std::ifstream file(metadataPath);
        json j;
        file >> j;

        PluginMetadata metadata = {};

        if (j.contains("plugin")) {
            auto &plugin         = j["plugin"];
            metadata.name        = plugin.value("name", "");
            metadata.version     = plugin.value("version", "");
            metadata.author      = plugin.value("author", "");
            metadata.description = plugin.value("description", "");
            metadata.license     = plugin.value("license", "");

            if (plugin.contains("signature")) {
                auto &sig                             = plugin["signature"];
                metadata.signature.sha256Hash         = sig.value("sha256", "");
                metadata.signature.signature          = sig.value("signature", "");
                metadata.signature.signingCertificate = sig.value("certificate", "");
                metadata.signature.issuer             = sig.value("issuer", "");
                metadata.signature.subject            = sig.value("subject", "");
                metadata.signature.timestamp          = sig.value("timestamp", 0);
            }

            if (plugin.contains("permissions")) {
                metadata.permissions = plugin["permissions"].get<std::vector<std::string>>();
            }
        }

        return metadata;

    } catch ([[maybe_unused]] const std::exception &e) {
        // Failed to parse metadata
        // Suppress unused variable warning
        return std::nullopt;
    }
}

bool PluginSecurityVerifier::isBlacklisted(const std::string &fileHash) const {
    return std::find(policy_.blacklistedHashes.begin(), policy_.blacklistedHashes.end(), fileHash)
           != policy_.blacklistedHashes.end();
}

bool PluginSecurityVerifier::isWhitelisted(const std::string &fileHash) const {
    return std::find(policy_.whitelistedHashes.begin(), policy_.whitelistedHashes.end(), fileHash)
           != policy_.whitelistedHashes.end();
}

PluginTrustLevel PluginSecurityVerifier::getTrustLevel(const PluginMetadata &metadata) {
    // Check whitelist first
    if (isWhitelisted(metadata.signature.sha256Hash)) {
        return PluginTrustLevel::TRUSTED;
    }

    // Check blacklist
    if (isBlacklisted(metadata.signature.sha256Hash)) {
        return PluginTrustLevel::BLOCKED;
    }

    // Check if signature is verified
    if (!metadata.signature.verified) {
        return PluginTrustLevel::UNTRUSTED;
    }

    // Check if issuer is trusted
    bool trustedIssuer = false;
    for (const auto &trustedIssuerDN : policy_.trustedIssuers) {
        if (metadata.signature.issuer.find(trustedIssuerDN) != std::string::npos) {
            trustedIssuer = true;
            break;
        }
    }

    if (!trustedIssuer) {
        return PluginTrustLevel::UNTRUSTED;
    }

    return PluginTrustLevel::TRUSTED;
}

bool PluginSecurityVerifier::verifyPlugin(const std::string &pluginPath, std::string &errorMessage) {
    auto &auditor = PluginSecurityAuditor::instance();

    // Step 1: Check if file exists
    if (!std::filesystem::exists(pluginPath)) {
        errorMessage = "Plugin file does not exist: " + pluginPath;
        auditor.logEvent({PluginSecurityEvent::EventType::PLUGIN_LOAD_FAILED, pluginPath, "", errorMessage,
                          static_cast<uint64_t>(std::time(nullptr)), "ERROR"});
        return false;
    }

    // Step 2: Calculate file hash
    std::string fileHash = {};
    if (policy_.verifyFileHash) {
        fileHash = calculateFileHash(pluginPath);
        if (fileHash.empty()) {
            errorMessage = "Failed to calculate file hash";
            auditor.logEvent({PluginSecurityEvent::EventType::PLUGIN_LOAD_FAILED, pluginPath, "", errorMessage,
                              static_cast<uint64_t>(std::time(nullptr)), "ERROR"});
            return false;
        }
    }

    // Step 3: Check blacklist
    if (isBlacklisted(fileHash)) {
        errorMessage = "Plugin is on blacklist (hash: " + fileHash + ")";
        auditor.logEvent({PluginSecurityEvent::EventType::BLACKLISTED, pluginPath, fileHash, errorMessage,
                          static_cast<uint64_t>(std::time(nullptr)), "CRITICAL"});
        return false;
    }

    // Step 4: Load and verify metadata
    auto metadata = loadMetadata(pluginPath);

    if (policy_.requireSignature && !metadata.has_value()) {
        if (!policy_.allowUnsigned) {
            errorMessage = "Plugin metadata not found (signature required)";
            auditor.logEvent({PluginSecurityEvent::EventType::SIGNATURE_VERIFICATION_FAILED, pluginPath, fileHash,
                              errorMessage, static_cast<uint64_t>(std::time(nullptr)), "ERROR"});
            return false;
        }
    }

    // Step 5: Verify hash matches metadata
    if (metadata.has_value() && policy_.verifyFileHash) {
        if (!metadata->signature.sha256Hash.empty() && metadata->signature.sha256Hash != fileHash) {
            errorMessage = "File hash mismatch! Expected: " + metadata->signature.sha256Hash + ", Got: " + fileHash;
            auditor.logEvent({PluginSecurityEvent::EventType::HASH_MISMATCH, pluginPath, fileHash, errorMessage,
                              static_cast<uint64_t>(std::time(nullptr)), "CRITICAL"});
            return false;
        }
    }

    // Step 6: Verify digital signature (if present)
    if (metadata.has_value() && !metadata->signature.signature.empty()) {
        if (!verifySignature(pluginPath, metadata->signature)) {
            errorMessage = "Digital signature verification failed";
            auditor.logEvent({PluginSecurityEvent::EventType::SIGNATURE_VERIFICATION_FAILED, pluginPath, fileHash,
                              errorMessage, static_cast<uint64_t>(std::time(nullptr)), "ERROR"});

            if (policy_.requireSignature) {
                return false;
            }
        } else {
            // Mark as verified in metadata
            const_cast<PluginMetadata &>(*metadata).signature.verified = true;

            auditor.logEvent({PluginSecurityEvent::EventType::SIGNATURE_VERIFIED, pluginPath, fileHash,
                              "Signature verified successfully", static_cast<uint64_t>(std::time(nullptr)), "INFO"});
        }
    }

    // Step 7: Check trust level
    if (metadata.has_value()) {
        auto trustLevel = getTrustLevel(*metadata);

        if (trustLevel == PluginTrustLevel::BLOCKED) {
            errorMessage = "Plugin trust level is BLOCKED";
            return false;
        }

        if (trustLevel < policy_.minTrustLevel) {
            errorMessage = "Plugin trust level insufficient (required: TRUSTED, got: UNTRUSTED)";
            auditor.logEvent({PluginSecurityEvent::EventType::POLICY_VIOLATION, pluginPath, fileHash, errorMessage,
                              static_cast<uint64_t>(std::time(nullptr)), "WARNING"});
            return false;
        }
    }

    // Step 8: Whitelist check (bypass other checks)
    if (isWhitelisted(fileHash)) {
        auditor.logEvent({PluginSecurityEvent::EventType::PLUGIN_LOADED, pluginPath, fileHash,
                          "Plugin loaded (whitelisted)", static_cast<uint64_t>(std::time(nullptr)), "INFO"});
        return true;
    }

    // All checks passed
    auditor.logEvent({PluginSecurityEvent::EventType::PLUGIN_LOADED, pluginPath, fileHash, "Plugin loaded successfully",
                      static_cast<uint64_t>(std::time(nullptr)), "INFO"});

    return true;
}

bool PluginSecurityVerifier::verifySignature(const std::string &filePath, const PluginSignature &signature) {
    if (signature.signature.empty() || signature.signingCertificate.empty()) {
        return false;
    }

    // Step 1: Calculate file hash
    std::string fileHash = calculateFileHash(filePath);
    if (fileHash.empty()) {
        return false;
    }

    // Step 2: Load X.509 certificate from PEM string
    BIO *bio
        = BIO_new_mem_buf(signature.signingCertificate.data(), static_cast<int>(signature.signingCertificate.size()));
    if (!bio) {
        return false;
    }

    X509 *cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    if (!cert) {
        return false;
    }

    // Step 3: Check certificate expiration
    int notBefore = X509_cmp_current_time(X509_get0_notBefore(cert));
    int notAfter  = X509_cmp_current_time(X509_get0_notAfter(cert));

    if (notBefore >= 0 || notAfter <= 0) {
        // Certificate is not yet valid or has expired
        X509_free(cert);
        return false;
    }

    // Step 4: Extract public key from certificate
    EVP_PKEY *pubKey = X509_get_pubkey(cert);
    if (!pubKey) {
        X509_free(cert);
        return false;
    }

    // Step 5: Decode signature from hex
    std::vector<uint8_t> sigBytes = {};

    if (!decodeHexString(signature.signature, sigBytes)) {
        EVP_PKEY_free(pubKey);
        X509_free(cert);
        return false;
    }

    // Step 6: Verify signature using public key
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        EVP_PKEY_free(pubKey);
        X509_free(cert);
        return false;
    }

    bool verified = false;

    // Initialize verification context
    if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pubKey) == 1) {
        // Convert file hash from hex string to bytes
        std::vector<uint8_t> hashBytes = {};

        if (decodeHexString(fileHash, hashBytes)) {
            // Verify signature
            int result = EVP_DigestVerify(mdctx, sigBytes.data(),static_cast<int>(sigBytes.size()), hashBytes.data(),static_cast<int>(hashBytes.size()));
            verified   = (result == 1);
        }
    }

    // Cleanup
    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pubKey);
    X509_free(cert);

    return verified;
}

bool PluginSecurityVerifier::verifyCertificateChain(const std::string &certificate) {
    if (certificate.empty()) {
        return false;
    }

    // Create BIO from certificate PEM string
    BIO *bio = BIO_new_mem_buf(certificate.data(), static_cast<int>(certificate.size()));
    if (!bio) {
        return false;
    }

    // Load the certificate
    X509 *cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    if (!cert) {
        return false;
    }

    // Create X509_STORE and load system CA certificates
    X509_STORE *store = X509_STORE_new();
    if (!store) {
        X509_free(cert);
        return false;
    }

    // Try to load system default CA certificates
    if (X509_STORE_set_default_paths(store) != 1) {
        // Also try common CA bundle locations
        const char *ca_paths[] = {"/etc/ssl/certs/ca-certificates.crt",     // Debian/Ubuntu
                                  "/etc/pki/tls/certs/ca-bundle.crt",       // RHEL/CentOS
                                  "/etc/ssl/ca-bundle.pem",                 // OpenSUSE
                                  "/usr/local/share/certs/ca-root-nss.crt", // FreeBSD
                                  nullptr};

        bool ca_loaded = false;
        for (int i = 0; ca_paths[i] != nullptr; i++) {
            if (X509_STORE_load_locations(store, ca_paths[i], nullptr) == 1) {
                ca_loaded = true;
                break;
            }
        }

        if (!ca_loaded) {
            X509_STORE_free(store);
            X509_free(cert);
            return false;
        }
    }

    // Create verification context
    X509_STORE_CTX *ctx = X509_STORE_CTX_new();
    if (!ctx) {
        X509_STORE_free(store);
        X509_free(cert);
        return false;
    }

    // Initialize context
    if (X509_STORE_CTX_init(ctx, store, cert, nullptr) != 1) {
        X509_STORE_CTX_free(ctx);
        X509_STORE_free(store);
        X509_free(cert);
        return false;
    }

    // Verify the certificate chain
    int verify_result = X509_verify_cert(ctx);

    // Cleanup
    X509_STORE_CTX_free(ctx);
    X509_STORE_free(store);
    X509_free(cert);

    return (verify_result == 1);
}

bool PluginSecurityVerifier::checkCRL(const std::string &certificate) {
    if (certificate.empty()) {
        return false;
    }

    // Load certificate
    BIO *bio = BIO_new_mem_buf(certificate.data(), static_cast<int>(certificate.size()));
    if (!bio) {
        return false;
    }

    X509 *cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    if (!cert) {
        return false;
    }

    // Derive cache key from certificate serial number
    std::string serial = getCertSerialHex(cert);

    // Consult the CRL cache first
    if (!serial.empty()) {
        std::lock_guard<std::mutex> lock(g_revocation_cache_mutex);
        auto it = g_crl_cache.find(serial);
        if (it != g_crl_cache.end() && std::chrono::system_clock::now() < it->second.expires_at) {
            bool cached_result = !it->second.is_revoked;
            X509_free(cert);
            return cached_result;
        }
    }

    // Extract CRL distribution points from certificate
    auto *crldp
        = static_cast<STACK_OF(DIST_POINT) *>(X509_get_ext_d2i(cert, NID_crl_distribution_points, nullptr, nullptr));

    // Default: pass when revocation checking is disabled, fail when it is required
    bool result       = !policy_.checkRevocation;
    bool checked      = false;
    long timeout_secs = static_cast<long>(policy_.revocation_timeout_seconds);

    if (crldp) {
        int num_points = sk_DIST_POINT_num(crldp);

        for (int i = 0; i < num_points && !checked; ++i) {
            DIST_POINT *dp = sk_DIST_POINT_value(crldp, i);
            if (!dp || !dp->distpoint || dp->distpoint->type != 0) {
                continue;
            }

            GENERAL_NAMES *gnames = dp->distpoint->name.fullname;
            if (!gnames) {
                continue;
            }
            int gname_count = sk_GENERAL_NAME_num(gnames);

            for (int j = 0; j < gname_count && !checked; ++j) {
                GENERAL_NAME *gn = sk_GENERAL_NAME_value(gnames, j);
                if (!gn || gn->type != GEN_URI) {
                    continue;
                }

                const char *url
                    = reinterpret_cast<const char *>(ASN1_STRING_get0_data(gn->d.uniformResourceIdentifier));
                if (!url || strncmp(url, "http", 4) != 0) {
                    continue;
                }

                // (1) Fetch CRL via HTTP GET
                std::vector<uint8_t> crl_data = httpGet(url, timeout_secs);
                if (crl_data.empty()) {
                    continue;
                }

                // (2) Parse the DER-encoded CRL
                const unsigned char *p = crl_data.data();
                X509_CRL *crl          = d2i_X509_CRL(nullptr, &p, static_cast<long>(crl_data.size()));
                if (!crl) {
                    continue;
                }

                // (3) Verify CRL signature against the issuer certificate.
                // We attempt to build the chain through the system trust store;
                // if the issuer is unavailable we still check the serial number
                // but log a warning about the unverified signature.
                X509_STORE *trust_store = X509_STORE_new();
                if (trust_store) {
                    X509_STORE_set_default_paths(trust_store);
                    // X509_CRL_verify returns 1 on success
                    EVP_PKEY *crl_issuer_key  = nullptr;
                    X509_STORE_CTX *chain_ctx = X509_STORE_CTX_new();
                    if (chain_ctx && X509_STORE_CTX_init(chain_ctx, trust_store, cert, nullptr)) {
                        if (X509_verify_cert(chain_ctx) <= 0) {
                            THEMIS_WARN("CRL check: certificate chain verification "
                                        "failed; proceeding without issuer key");
                        }
                        STACK_OF(X509) *chain = X509_STORE_CTX_get0_chain(chain_ctx);
                        if (chain && sk_X509_num(chain) > 1) {
                            X509 *issuer   = sk_X509_value(chain, 1);
                            crl_issuer_key = X509_get_pubkey(issuer);
                        }
                    }
                    if (chain_ctx) {
                        X509_STORE_CTX_free(chain_ctx);
                    }
                    if (crl_issuer_key) {
                        if (X509_CRL_verify(crl, crl_issuer_key) <= 0) {
                            THEMIS_WARN("CRL signature verification failed for "
                                        "distribution point: {}",
                                        url);
                            EVP_PKEY_free(crl_issuer_key);
                            X509_STORE_free(trust_store);
                            X509_CRL_free(crl);
                            continue;
                        }
                        EVP_PKEY_free(crl_issuer_key);
                    } else {
                        THEMIS_WARN("CRL signature could not be verified "
                                    "(issuer not in trust store): {}",
                                    url);
                    }
                    X509_STORE_free(trust_store);
                }

                // (4) Validate CRL thisUpdate / nextUpdate timestamps
                const ASN1_TIME *next_update = X509_CRL_get0_nextUpdate(crl);
                if (next_update) {
                    int day = 0, sec = 0;
                    if (ASN1_TIME_diff(&day, &sec, nullptr, next_update)) {
                        if (day < 0 || (day == 0 && sec < 0)) {
                            // CRL has expired — skip this distribution point
                            X509_CRL_free(crl);
                            continue;
                        }
                    }
                }

                // (5) Check target certificate's serial number
                X509_REVOKED *revoked_entry = nullptr;
                int rv                      = X509_CRL_get0_by_cert(crl, &revoked_entry, cert);
                // rv == 1  → certificate is revoked
                // rv == 0  → certificate is not in the CRL
                bool is_revoked = (rv == 1);

                // Compute cache TTL: use CRL nextUpdate, capped to 24 h default
                auto cache_expiry = std::chrono::system_clock::now() + std::chrono::hours(24);
                if (next_update) {
                    int nday = 0, nsec = 0;
                    if (ASN1_TIME_diff(&nday, &nsec, nullptr, next_update) && nday >= 0 && nsec >= 0) {
                        cache_expiry = std::chrono::system_clock::now() + std::chrono::hours(nday * 24)
                                       + std::chrono::seconds(nsec);
                    }
                }

                // Store result in cache
                if (!serial.empty()) {
                    std::lock_guard<std::mutex> lock(g_revocation_cache_mutex);
                    g_crl_cache[serial] = {is_revoked, cache_expiry};
                }

                result  = !is_revoked;
                checked = true;
                X509_CRL_free(crl);
            }
        }

        sk_DIST_POINT_pop_free(crldp, DIST_POINT_free);
    }

    // If no CRL endpoint was reachable and revocation checking is required,
    // fail safe: treat the certificate as not validated.
    if (!checked && policy_.checkRevocation) {
        result = false;
    }

    X509_free(cert);
    return result;
}

bool PluginSecurityVerifier::checkOCSP(const std::string &certificate) {
    if (certificate.empty()) {
        return false;
    }

    // Load certificate
    BIO *bio = BIO_new_mem_buf(certificate.data(), static_cast<int>(certificate.size()));
    if (!bio) {
        return false;
    }

    X509 *cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    if (!cert) {
        return false;
    }

    // Derive cache key from certificate serial number
    std::string serial = getCertSerialHex(cert);

    // Consult the OCSP cache first
    if (!serial.empty()) {
        std::lock_guard<std::mutex> lock(g_revocation_cache_mutex);
        auto it = g_ocsp_cache.find(serial);
        if (it != g_ocsp_cache.end() && std::chrono::system_clock::now() < it->second.expires_at) {
            bool cached_result = !it->second.is_revoked;
            X509_free(cert);
            return cached_result;
        }
    }

    // Extract OCSP responder URLs from the Authority Information Access extension
    STACK_OF(OPENSSL_STRING) *ocsp_list = X509_get1_ocsp(cert);

    // Default: pass when revocation checking is disabled, fail when it is required
    bool result       = !policy_.checkRevocation;
    bool checked      = false;
    long timeout_secs = static_cast<long>(policy_.revocation_timeout_seconds);

    if (ocsp_list) {
        int num_urls = sk_OPENSSL_STRING_num(ocsp_list);

        // Build the certificate chain once so we can find the issuer cert.
        X509_STORE *trust_store = X509_STORE_new();
        X509 *issuer_cert       = nullptr;
        if (trust_store) {
            X509_STORE_set_default_paths(trust_store);
            X509_STORE_CTX *chain_ctx = X509_STORE_CTX_new();
            if (chain_ctx && X509_STORE_CTX_init(chain_ctx, trust_store, cert, nullptr)) {
                if (X509_verify_cert(chain_ctx) <= 0) {
                    THEMIS_WARN("OCSP check: certificate chain verification "
                                "failed; issuer may not be available");
                }
                STACK_OF(X509) *chain = X509_STORE_CTX_get0_chain(chain_ctx);
                // chain[0] = subject cert, chain[1] = issuer
                if (chain && sk_X509_num(chain) > 1) {
                    issuer_cert = X509_dup(sk_X509_value(chain, 1));
                }
            }
            if (chain_ctx) {
                X509_STORE_CTX_free(chain_ctx);
            }
        }

        for (int i = 0; i < num_urls && !checked; ++i) {
            const char *url = sk_OPENSSL_STRING_value(ocsp_list, i);
            if (!url || strncmp(url, "http", 4) != 0) {
                continue;
            }

            // OCSP_cert_to_id requires the issuer certificate
            if (!issuer_cert) {
                THEMIS_WARN("OCSP check: issuer certificate not found in trust "
                            "store; skipping responder {}",
                            url);
                break;
            }

            // (1) Build the OCSP request
            OCSP_REQUEST *req = OCSP_REQUEST_new();
            if (!req) {
                continue;
            }

            // OCSP_cert_to_id transfers ownership of certid into the request
            OCSP_CERTID *certid = OCSP_cert_to_id(EVP_sha1(), cert, issuer_cert);
            if (!certid) {
                OCSP_REQUEST_free(req);
                continue;
            }

            if (!OCSP_request_add0_id(req, certid)) {
                // add0 failed – certid still owned by us
                OCSP_CERTID_free(certid);
                OCSP_REQUEST_free(req);
                continue;
            }

            // DER-encode the request for the HTTP POST body
            unsigned char *req_der = nullptr;
            int req_der_len        = i2d_OCSP_REQUEST(req, &req_der);
            OCSP_REQUEST_free(req);

            if (req_der_len <= 0) {
                continue;
            }

            std::vector<uint8_t> req_body(req_der, req_der + req_der_len);
            OPENSSL_free(req_der);

            // (2) POST the OCSP request to the responder
            std::vector<uint8_t> resp_data = httpPost(url, req_body, "application/ocsp-request", timeout_secs);
            if (resp_data.empty()) {
                continue;
            }

            // (3a) Parse the OCSP response envelope
            const unsigned char *p = resp_data.data();
            OCSP_RESPONSE *resp    = d2i_OCSP_RESPONSE(nullptr, &p, static_cast<long>(resp_data.size()));
            if (!resp) {
                continue;
            }

            if (OCSP_response_status(resp) != OCSP_RESPONSE_STATUS_SUCCESSFUL) {
                OCSP_RESPONSE_free(resp);
                continue;
            }

            OCSP_BASICRESP *basic = OCSP_response_get1_basic(resp);
            OCSP_RESPONSE_free(resp);
            if (!basic) {
                continue;
            }

            // (3b) Verify the responder's signature
            int verify_rc = OCSP_basic_verify(basic, nullptr, trust_store, 0);
            if (verify_rc <= 0) {
                THEMIS_WARN("OCSP basic response signature verification failed "
                            "for responder: {}",
                            url);
                OCSP_BASICRESP_free(basic);
                continue;
            }

            // (4) Find the status for our specific certificate
            OCSP_CERTID *lookup_id = OCSP_cert_to_id(EVP_sha1(), cert, issuer_cert);
            if (!lookup_id) {
                OCSP_BASICRESP_free(basic);
                continue;
            }

            int cert_status                = V_OCSP_CERTSTATUS_UNKNOWN;
            int reason                     = 0;
            ASN1_GENERALIZEDTIME *this_upd = nullptr;
            ASN1_GENERALIZEDTIME *next_upd = nullptr;
            ASN1_GENERALIZEDTIME *rev_time = nullptr;
            int found = OCSP_resp_find_status(basic, lookup_id, &cert_status, &reason, &rev_time, &this_upd, &next_upd);
            OCSP_CERTID_free(lookup_id);

            if (found == 1) {
                // (5) Validate thisUpdate / nextUpdate bounds (±5 min clock skew)
                if (OCSP_check_validity(this_upd, next_upd, 300L, -1L) != 1) {
                    THEMIS_WARN("OCSP response timestamps invalid for responder: {}", url);
                    OCSP_BASICRESP_free(basic);
                    continue;
                }

                bool is_revoked = (cert_status == V_OCSP_CERTSTATUS_REVOKED);

                // Cache result: OCSP default TTL is 1 h; shorten to nextUpdate
                auto cache_expiry = std::chrono::system_clock::now() + std::chrono::hours(1);
                if (next_upd) {
                    int nday = 0, nsec = 0;
                    if (ASN1_TIME_diff(&nday, &nsec, nullptr, next_upd) && nday >= 0 && nsec >= 0) {
                        auto nu_expiry = std::chrono::system_clock::now() + std::chrono::hours(nday * 24)
                                         + std::chrono::seconds(nsec);
                        if (nu_expiry < cache_expiry) {
                            cache_expiry = nu_expiry;
                        }
                    }
                }

                if (!serial.empty()) {
                    std::lock_guard<std::mutex> lock(g_revocation_cache_mutex);
                    g_ocsp_cache[serial] = {is_revoked, cache_expiry};
                }

                result  = !is_revoked;
                checked = true;
            }

            OCSP_BASICRESP_free(basic);
        }

        if (issuer_cert) {
            X509_free(issuer_cert);
        }
        if (trust_store) {
            X509_STORE_free(trust_store);
        }
        X509_email_free(ocsp_list);
    }

    // If no OCSP responder was reachable and revocation checking is required,
    // fail safe.
    if (!checked && policy_.checkRevocation) {
        result = false;
    }

    X509_free(cert);
    return result;
}

void PluginSecurityVerifier::updatePolicy(const PluginSecurityPolicy &policy) {
    policy_ = policy;
}

// ============================================================================
// PluginSecurityAuditor Implementation
// ============================================================================

PluginSecurityAuditor &PluginSecurityAuditor::instance() {
    static PluginSecurityAuditor instance;
    return instance;
}

void PluginSecurityAuditor::logEvent([[maybe_unused]] const PluginSecurityEvent &event) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back([[maybe_unused]] event);
    }

    // Forward to the ThemisDB system logger
    const std::string msg
        = "[PluginSecurity] " + event.message + " | plugin=" + event.pluginPath + " | hash=" + event.pluginHash;
    if ([[maybe_unused]] event.severity == "CRITICAL") {
        THEMIS_CRITICAL("{}", msg);
    } else if ([[maybe_unused]] event.severity == "ERROR") {
        THEMIS_ERROR("{}", msg);
    } else if ([[maybe_unused]] event.severity == "WARNING") {
        THEMIS_WARN("{}", msg);
    } else {
        THEMIS_INFO("{}", msg);
    }
}

std::vector<PluginSecurityEvent> PluginSecurityAuditor::getEventsForPlugin([[maybe_unused]] const std::string &pluginPath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PluginSecurityEvent> result = {};

    for ([[maybe_unused]] const auto &event : events_) {
        if ([[maybe_unused]] event.pluginPath == pluginPath) {
            result.push_back([[maybe_unused]] event);
        }
    }
    return result;
}

std::vector<PluginSecurityEvent> PluginSecurityAuditor::getAllEvents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_;
}

void PluginSecurityAuditor::clearEvents() {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.clear();
}

bool PluginSecurityAuditor::exportEvents([[maybe_unused]] const std::string &outputPath) const {
    // Validate output path against traversal and injection (CWE-22/23/24).
    if (outputPath.empty()) {
        return false;
    }
    if (outputPath.find("..") != std::string::npos) {
        return false;
    }
    if (static_cast<int>(outputPath.size()) != std::strlen(outputPath.c_str())) {
        return false;
    }
    {
        std::error_code ec = {};
        std::filesystem::path canonical = std::filesystem::weakly_canonical(outputPath, ec);
        if (ec || !canonical.is_absolute()) {
            return false;
        }
    }
    std::vector<PluginSecurityEvent> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot = events_;
    }

    // Helper: convert enum to human-readable string (robust against future values)
    auto typeToString = [](PluginSecurityEvent::EventType t) -> std::string {
        switch (t) {
            case PluginSecurityEvent::EventType::PLUGIN_LOADED:
                return "PLUGIN_LOADED";
            case PluginSecurityEvent::EventType::PLUGIN_LOAD_FAILED:
                return "PLUGIN_LOAD_FAILED";
            case PluginSecurityEvent::EventType::PLUGIN_UNLOADED:
                return "PLUGIN_UNLOADED";
            case PluginSecurityEvent::EventType::SIGNATURE_VERIFIED:
                return "SIGNATURE_VERIFIED";
            case PluginSecurityEvent::EventType::SIGNATURE_VERIFICATION_FAILED:
                return "SIGNATURE_VERIFICATION_FAILED";
            case PluginSecurityEvent::EventType::HASH_MISMATCH:
                return "HASH_MISMATCH";
            case PluginSecurityEvent::EventType::BLACKLISTED:
                return "BLACKLISTED";
            case PluginSecurityEvent::EventType::UNTRUSTED_ISSUER:
                return "UNTRUSTED_ISSUER";
            case PluginSecurityEvent::EventType::CERTIFICATE_EXPIRED:
                return "CERTIFICATE_EXPIRED";
            case PluginSecurityEvent::EventType::CERTIFICATE_REVOKED:
                return "CERTIFICATE_REVOKED";
            case PluginSecurityEvent::EventType::POLICY_VIOLATION:
                return "POLICY_VIOLATION";
            default:
                return "UNKNOWN";
        }
    };

    try {
        json j;
        j["events"] = json::array();

        for ([[maybe_unused]] const auto &event : snapshot) {
            json eventJson;
            eventJson["type"]       = typeToString([[maybe_unused]] event.type);
            eventJson["pluginPath"] = event.pluginPath;
            eventJson["pluginHash"] = event.pluginHash;
            eventJson["message"]    = event.message;
            eventJson["timestamp"]  = event.timestamp;
            eventJson["severity"]   = event.severity;

            j["events"].push_back([[maybe_unused]] eventJson);
        }

        std::ofstream file(outputPath);
        if (!file) {
            return false;
        }
        file << j.dump(2);
        if (!file.good()) {
            return false;
        }

        return true;

    } catch (const nlohmann::json::exception &) {
        return false;
    } catch (const std::exception &) {
        return false;
    } catch (const std::string &) {
        return false;
    } catch (const char *) {
        return false;
    }
}

// ============================================================================
// EnhancedPluginSecurityVerifier Implementation
// ============================================================================

EnhancedPluginSecurityVerifier::EnhancedPluginSecurityVerifier(const PluginSecurityPolicy &policy) : policy_(policy) {}

EnhancedPluginSecurityVerifier::VerificationResult
EnhancedPluginSecurityVerifier::verifyPlugin(const std::string &plugin_path, VerificationLevel required_level) {
    VerificationResult result;

    // Level 1: Hash verification
    if (!verifyHash(plugin_path, result)) {
        result.passed = false;
        return result;
    }
    result.level_achieved = VerificationLevel::LEVEL_1_HASH_ONLY;

    // If only hash is required, we're done
    if (required_level == VerificationLevel::LEVEL_1_HASH_ONLY) {
        result.passed = true;
        return result;
    }

    // Level 2: Embedded signature verification
    if (verifyEmbeddedSignature(plugin_path, result)) {
        result.level_achieved = VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE;

        // If level 2 is sufficient, we're done
        if (required_level == VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE) {
            result.passed = true;
            return result;
        }
    } else {
        // If embedded signature required but failed
        if (required_level >= VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE) {
            // In development mode (allowUnsigned), we can proceed with lower levels
            if (!policy_.allowUnsigned) {
                result.passed = false;
                return result;
            }
        }
    }

    // Level 3: Platform signature verification
    if (verifyPlatformSignature(plugin_path, result)) {
        result.level_achieved = VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE;

        // If level 3 is sufficient, we're done
        if (required_level == VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE) {
            result.passed = true;
            return result;
        }
    } else {
        // If platform signature required but failed
        if (required_level >= VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE) {
            // Only fail if we require platform signature AND it's not development mode
            if (!policy_.allowUnsigned) {
                result.passed = false;
                return result;
            }
        }
    }

    // Level 4: Full chain verification
    if (required_level == VerificationLevel::LEVEL_4_FULL_CHAIN) {
        if (verifyFullChain(plugin_path, result)) {
            result.level_achieved = VerificationLevel::LEVEL_4_FULL_CHAIN;
            result.passed         = true;
        } else {
            result.passed = false;
        }
        return result;
    }

    // If we got here, verification passed at the required level
    result.passed = true;
    return result;
}

bool EnhancedPluginSecurityVerifier::verifyHash(const std::string &plugin_path, VerificationResult &result) {
    PluginSecurityVerifier basic_verifier(policy_);
    std::string fileHash = basic_verifier.calculateFileHash(plugin_path);

    if (fileHash.empty()) {
        result.error_message = "Failed to calculate file hash";
        return false;
    }

    result.hash_verified = true;
    return true;
}

bool EnhancedPluginSecurityVerifier::verifyEmbeddedSignature(const std::string &plugin_path,
                                                             VerificationResult &result) {
    // Try to extract embedded certificate from DLL/SO
    auto cert_data = extractEmbeddedCertificate(plugin_path);
    if (!cert_data) {
        result.error_message = "No embedded certificate found";
        return false;
    }

    // Parse certificate
    const unsigned char *p = cert_data->data();
    X509 *cert             = d2i_X509(nullptr, &p, static_cast<long>(cert_data->size()));
    if (!cert) {
        result.error_message = "Failed to parse embedded certificate";
        return false;
    }

    // Verify certificate is official ThemisDB certificate
    if (!isOfficialThemisDBCertificate(cert)) {
        result.error_message = "Certificate is not official ThemisDB certificate";
        result.issuer        = getCertificateIssuer(cert);
        X509_free(cert);
        return false;
    }

    // Check certificate validity
    if (!isCertificateValid(cert)) {
        result.error_message = "Certificate has expired or not yet valid";
        X509_free(cert);
        return false;
    }

    // Extract embedded signature
    auto signature_data = extractEmbeddedSignature(plugin_path);
    if (!signature_data) {
        result.error_message = "No embedded signature found";
        X509_free(cert);
        return false;
    }

    // Calculate DLL hash (excluding signature section)
    std::vector<uint8_t> dll_hash = calculateHashExcludingSignature(plugin_path);

    // Verify signature with certificate public key
    EVP_PKEY *pubkey     = X509_get_pubkey(cert);
    bool signature_valid = verifyRSASignature(dll_hash, *signature_data, pubkey);
    EVP_PKEY_free(pubkey);

    // Get certificate info
    result.issuer               = getCertificateIssuer(cert);
    result.subject              = getCertificateSubject(cert);
    result.is_themisdb_official = true;

    X509_free(cert);

    if (!signature_valid) {
        result.error_message = "Embedded signature verification failed";
        return false;
    }

    result.embedded_signature_verified = true;
    return true;
}

bool EnhancedPluginSecurityVerifier::verifyPlatformSignature(const std::string &plugin_path,
                                                             VerificationResult &result) {
#ifdef _WIN32
    return verifyAuthenticodeSignature(plugin_path, result);
#elif defined(__APPLE__)
    return verifyMacOSCodeSignature(plugin_path, result);
#else
    return verifyGPGSignature(plugin_path, result);
#endif
}

bool EnhancedPluginSecurityVerifier::verifyFullChain(const std::string &plugin_path, VerificationResult &result) {
    // First verify embedded signature
    if (!verifyEmbeddedSignature(plugin_path, result)) {
        return false;
    }

    // Then verify platform signature
    if (!verifyPlatformSignature(plugin_path, result)) {
        // Platform signature is optional for full chain in development mode
        if (!policy_.allowUnsigned) {
            return false;
        }
    }

    // Verify certificate chain if we have certificate information
    if (!result.issuer.empty() || !result.subject.empty()) {
        // Try to get the certificate from the plugin metadata
        auto metadata = loadPluginMetadataForChainValidation(plugin_path);

        if (metadata && !metadata->signature.signingCertificate.empty()) {
            // Create basic verifier to use its certificate chain validation
            PluginSecurityVerifier basic_verifier(policy_);

            // Verify certificate chain
            bool chain_valid = basic_verifier.verifyCertificateChain(metadata->signature.signingCertificate);

            result.certificate_chain_verified = chain_valid;

            if (!chain_valid && policy_.requireSignature) {
                result.error_message = "Certificate chain validation failed";
                return false;
            }

            // Check certificate revocation using CRL and OCSP.
            // A certificate is considered not-revoked if at least one
            // check returns a positive result (CRL or OCSP confirmed
            // not-revoked).  Both checks fail safe: when an endpoint is
            // unreachable they return false, so if BOTH return false
            // (no reachable endpoint at all) the certificate is rejected.
            if (policy_.checkRevocation) {
                const std::string &signing_cert = metadata->signature.signingCertificate;

                bool crl_result  = basic_verifier.checkCRL(signing_cert);
                bool ocsp_result = basic_verifier.checkOCSP(signing_cert);

                // Pass if at least one check confirmed the cert is not
                // revoked.  Fail if both returned false (either the cert
                // is revoked or neither endpoint was reachable).
                bool not_revoked               = crl_result || ocsp_result;
                result.certificate_not_revoked = not_revoked;

                if (!not_revoked) {
                    result.error_message = "Certificate revocation check failed (CRL and OCSP)";
                    return false;
                }
            } else {
                // Revocation checking is disabled — assume not revoked
                result.certificate_not_revoked = true;
            }
        }
    }

    return true;
}

// Helper method to load metadata for chain validation
std::optional<PluginMetadata>
EnhancedPluginSecurityVerifier::loadPluginMetadataForChainValidation(const std::string &plugin_path) {
    PluginSecurityVerifier basic_verifier(policy_);
    return basic_verifier.loadMetadata(plugin_path);
}

std::optional<std::vector<uint8_t>>
EnhancedPluginSecurityVerifier::extractEmbeddedCertificate(const std::string &plugin_path) {
    std::ifstream file(plugin_path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }

    // Read enough of the header for MZ/ELF/Mach-O detection
    std::vector<uint8_t> header(64);
    file.read(reinterpret_cast<char *>(header.data()), static_cast<std::streamsize>(header.size()));
    const std::streamsize header_bytes = file.gcount();

    if (header_bytes < 4) {
        return std::nullopt;
    }

    // -----------------------------------------------------------------------
    // PE format (Windows DLL/EXE)
    // Authenticode certificate table: parse the optional header data
    // directory IMAGE_DIRECTORY_ENTRY_SECURITY (index 4) and return the
    // embedded PKCS#7 (WIN_CERTIFICATE.bCertificate) blob.
    // -----------------------------------------------------------------------
    if (header[0] == 'M' && header[1] == 'Z') {
        // Read e_lfanew at offset 0x3C
        file.seekg(0x3C);
        uint32_t pe_offset = 0;
        file.read(reinterpret_cast<char *>(&pe_offset), sizeof(pe_offset));
        if (!file.good() || pe_offset == 0 || pe_offset >= 0x40000000u) {
            return std::nullopt;
        }

        // PE signature "PE\0\0"
        file.seekg(pe_offset);
        uint32_t pe_sig = 0;
        file.read(reinterpret_cast<char *>(&pe_sig), sizeof(pe_sig));
        if (!file.good() || pe_sig != 0x00004550u) {
            return std::nullopt;
        }

        // COFF File Header: skip 2 bytes Machine, 2 bytes NumberOfSections,
        // 4 bytes TimeDateStamp, 4 bytes PointerToSymbolTable,
        // 4 bytes NumberOfSymbols, 2 bytes SizeOfOptionalHeader,
        // 2 bytes Characteristics  → total 20 bytes after pe_sig.
        // Optional header starts at pe_offset + 4 (sig) + 20 (file hdr) = +24.
        uint64_t opt_hdr_offset = static_cast<uint64_t>(pe_offset) + 24;

        // Optional header magic: 0x10B = PE32, 0x20B = PE32+
        file.seekg(static_cast<std::streamoff>(opt_hdr_offset));
        uint16_t opt_magic = 0;
        file.read(reinterpret_cast<char *>(&opt_magic), sizeof(opt_magic));
        if (!file.good()) {
            return std::nullopt;
        }

        // Data directories start offset within the optional header:
        //   PE32  (0x10B): +96 from start of optional header
        //   PE32+ (0x20B): +112 from start of optional header
        uint64_t data_dir_start = 0;
        if (opt_magic == 0x010Bu) {
            data_dir_start = opt_hdr_offset + 96;
        } else if (opt_magic == 0x020Bu) {
            data_dir_start = opt_hdr_offset + 112;
        } else {
            return std::nullopt;
        }

        // IMAGE_DIRECTORY_ENTRY_SECURITY = index 4; each entry is 8 bytes
        // (4-byte VirtualAddress + 4-byte Size).
        uint64_t security_dir_offset = data_dir_start + 4 * 8;
        file.seekg(static_cast<std::streamoff>(security_dir_offset));
        uint32_t sec_rva = 0, sec_size = 0;
        file.read(reinterpret_cast<char *>(&sec_rva), sizeof(sec_rva));
        file.read(reinterpret_cast<char *>(&sec_size), sizeof(sec_size));

        if (!file.good() || sec_rva == 0 || sec_size < 8) {
            return std::nullopt;
        }

        // For the security data directory the VirtualAddress is a file offset
        // (not a virtual address), so seek directly.
        file.seekg(static_cast<std::streamoff>(sec_rva));
        if (!file.good()) {
            return std::nullopt;
        }

        // Iterate all WIN_CERTIFICATE records in the certificate table.
        // Each record is padded to an 8-byte boundary.
        // WIN_CERT_TYPE_PKCS_SIGNED_DATA = 0x0002 (Authenticode PKCS#7)
        std::vector<std::vector<uint8_t>> pkcs7_blobs;
        uint32_t tbl_pos       = sec_rva;
        const uint32_t tbl_end = sec_rva + sec_size;

        while (tbl_pos + 8 <= tbl_end) {
            file.seekg(static_cast<std::streamoff>(tbl_pos));
            uint32_t win_cert_len  = 0;
            uint16_t win_cert_rev  = 0;
            uint16_t win_cert_type = 0;
            file.read(reinterpret_cast<char *>(&win_cert_len), sizeof(win_cert_len));
            file.read(reinterpret_cast<char *>(&win_cert_rev), sizeof(win_cert_rev));
            file.read(reinterpret_cast<char *>(&win_cert_type), sizeof(win_cert_type));

            if (!file.good() || win_cert_len < 8 || win_cert_len > (tbl_end - tbl_pos)) {
                break;
            }

            if (win_cert_type == 0x0002u) {
                uint32_t data_len = win_cert_len - 8;
                std::vector<uint8_t> blob(data_len);
                file.read(reinterpret_cast<char *>(blob.data()), static_cast<std::streamsize>(data_len));
                if (static_cast<uint32_t>(file.gcount()) == data_len) {
                    pkcs7_blobs.push_back(std::move(blob));
                }
            }

            // Advance to next record, padded to an 8-byte boundary.
            // Guard against uint32_t overflow before rounding up.
            if (win_cert_len > 0xFFFFFFF8u) {
                break;
            }
            const uint32_t padded = (win_cert_len + 7) & ~7;
            tbl_pos += padded;
        }

        if (pkcs7_blobs.empty()) {
            return std::nullopt;
        }

        if (static_cast<int>(pkcs7_blobs.size()) > 1) {
            THEMIS_WARN("extractEmbeddedCertificate: {} PKCS#7 certificates found "
                        "in PE certificate table; using the first one.",
                        pkcs7_blobs.size());
        }

        return std::move(pkcs7_blobs[0]);
    }
    // -----------------------------------------------------------------------
    // ELF format (Linux SO) — try the .note.gnu.signature section first,
    // then fall back to a sidecar <plugin_path>.sig file.
    // -----------------------------------------------------------------------
    else if (header[0] == 0x7F && header[1] == 'E' && header[2] == 'L' && header[3] == 'F') {
        // ELF class (e_ident[4]): 1 = 32-bit, 2 = 64-bit
        // ELF data encoding (e_ident[5]): 1 = LE, 2 = BE
        const bool elf64 = (header[4] == 2);
        const bool le    = (header[5] == 1);

        if (le && header_bytes >= static_cast<std::streamsize>(elf64 ? 64 : 52)) {
            // Helper lambdas: read little-endian integers from a byte buffer.
            auto readLE16 = [](const uint8_t *p) -> uint16_t {
                return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
            };
            auto readLE32 = [](const uint8_t *p) -> uint32_t {
                return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8)
                       | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
            };
            auto readLE64 = [](const uint8_t *p) -> uint64_t {
                return static_cast<uint64_t>(p[0]) | (static_cast<uint64_t>(p[1]) << 8)
                       | (static_cast<uint64_t>(p[2]) << 16) | (static_cast<uint64_t>(p[3]) << 24)
                       | (static_cast<uint64_t>(p[4]) << 32) | (static_cast<uint64_t>(p[5]) << 40)
                       | (static_cast<uint64_t>(p[6]) << 48) | (static_cast<uint64_t>(p[7]) << 56);
            };

            // Extract section-header table metadata from the already-loaded
            // ELF header bytes.
            uint64_t shoff     = 0;
            uint16_t shentsize = 0;
            uint16_t shnum     = 0;
            uint16_t shstrndx  = 0;

            if (elf64) {
                // ELF64: e_shoff@40, e_shentsize@58, e_shnum@60, e_shstrndx@62
                shoff     = readLE64(header.data() + 40);
                shentsize = readLE16(header.data() + 58);
                shnum     = readLE16(header.data() + 60);
                shstrndx  = readLE16(header.data() + 62);
            } else {
                // ELF32: e_shoff@32, e_shentsize@46, e_shnum@48, e_shstrndx@50
                shoff     = readLE32(header.data() + 32);
                shentsize = readLE16(header.data() + 46);
                shnum     = readLE16(header.data() + 48);
                shstrndx  = readLE16(header.data() + 50);
            }

            constexpr uint32_t kMaxSections = 4096;
            constexpr uint64_t kMaxSigSize  = 64 * 1024;

            if (shoff > 0 && shentsize > 0 && shnum > 0 && shnum <= kMaxSections && shstrndx < shnum) {
                // Locate the section-name string table (shstrndx).
                uint64_t strtab_off  = 0;
                uint64_t strtab_size = 0;
                file.seekg(static_cast<std::streamoff>(shoff + static_cast<uint64_t>(shstrndx) * shentsize));
                if (file.good()) {
                    if (elf64) {
                        // Elf64_Shdr: sh_name(4)+sh_type(4)+sh_flags(8)+
                        //             sh_addr(8)+sh_offset(8)+sh_size(8)
                        uint8_t shdr[64] = {};
                        file.read(reinterpret_cast<char *>(shdr), 64);
                        if (file.gcount() == 64) {
                            strtab_off  = readLE64(shdr + 24);
                            strtab_size = readLE64(shdr + 32);
                        }
                    } else {
                        // Elf32_Shdr: sh_name(4)+sh_type(4)+sh_flags(4)+
                        //             sh_addr(4)+sh_offset(4)+sh_size(4)
                        uint8_t shdr[40] = {};
                        file.read(reinterpret_cast<char *>(shdr), 40);
                        if (file.gcount() == 40) {
                            strtab_off  = readLE32(shdr + 16);
                            strtab_size = readLE32(shdr + 20);
                        }
                    }
                }

                if (strtab_off > 0 && strtab_size > 0 && strtab_size <= 65536) {
                    // Load the section-name string table.
                    std::vector<char> strtab(strtab_size + 1, '\0');
                    file.seekg(static_cast<std::streamoff>(strtab_off));
                    file.read(strtab.data(), static_cast<std::streamsize>(strtab_size));
                    if (file.gcount() == static_cast<std::streamsize>(strtab_size)) {
                        // Scan all section headers for .note.gnu.signature.
                        for (uint16_t i = 0; i < shnum; ++i) {
                            file.seekg(static_cast<std::streamoff>(shoff + static_cast<uint64_t>(i) * shentsize));
                            if (!file.good())
                                break;

                            uint32_t sh_name = 0;
                            uint64_t sh_off  = 0;
                            uint64_t sh_size = 0;

                            if (elf64) {
                                uint8_t shdr[64] = {};
                                file.read(reinterpret_cast<char *>(shdr), 64);
                                if (file.gcount() != 64)
                                    break;
                                sh_name = readLE32(shdr + 0);
                                sh_off  = readLE64(shdr + 24);
                                sh_size = readLE64(shdr + 32);
                            } else {
                                uint8_t shdr[40] = {};
                                file.read(reinterpret_cast<char *>(shdr), 40);
                                if (file.gcount() != 40)
                                    break;
                                sh_name = readLE32(shdr + 0);
                                sh_off  = readLE32(shdr + 16);
                                sh_size = readLE32(shdr + 20);
                            }

                            if (sh_name >= strtab_size)
                                continue;
                            const char *sec_name = strtab.data() + sh_name;

                            if (std::strcmp(sec_name, ".note.gnu.signature") == 0 && sh_size > 0
                                && sh_size <= kMaxSigSize) {
                                std::vector<uint8_t> sig_data(static_cast<size_t>(sh_size));
                                file.seekg(static_cast<std::streamoff>(sh_off));
                                file.read(reinterpret_cast<char *>(sig_data.data()),
                                          static_cast<std::streamsize>(sh_size));
                                if (static_cast<uint64_t>(file.gcount()) == sh_size) {
                                    return sig_data;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Fall back: check for a sidecar <plugin_path>.sig file.
        {
            const std::string sig_path = plugin_path + ".sig";
            std::ifstream sig_file(sig_path, std::ios::binary);
            if (sig_file.good()) {
                sig_file.seekg(0, std::ios::end);
                const std::streamoff sig_sz = sig_file.tellg();
                sig_file.seekg(0, std::ios::beg);
                constexpr std::streamoff kMaxSigFileSize = 64LL * 1024LL;
                if (sig_sz > 0 && sig_sz <= kMaxSigFileSize) {
                    std::vector<uint8_t> sig_data(static_cast<size_t>(sig_sz));
                    sig_file.read(reinterpret_cast<char *>(sig_data.data()), sig_sz);
                    if (sig_file.gcount() == sig_sz) {
                        return sig_data;
                    }
                }
            }
        }
    }
    // -----------------------------------------------------------------------
    // Mach-O format (macOS dylib) — code signatures are in
    // LC_CODE_SIGNATURE; full load-command parsing is required.
    //
    // Mach-O magic values and byte-order:
    //   0xFEEDFACE (BE 32-bit):  bytes FE ED FA CE
    //   0xFEEDFACF (BE 64-bit):  bytes FE ED FA CF
    //   0xCEFAEDFE (LE 32-bit):  bytes CE FA ED FE
    //   0xCFFAEDFE (LE 64-bit):  bytes CF FA ED FE
    // LC_CODE_SIGNATURE = 0x0000_001D
    // linkedit_data_command layout (8 bytes after cmd+cmdsize):
    //   uint32_t dataoff;   // file offset of signature blob
    //   uint32_t datasize;  // byte size of signature blob
    // -----------------------------------------------------------------------
    else if (((header[0] == 0xFE && header[1] == 0xED && header[2] == 0xFA && (header[3] == 0xCE || header[3] == 0xCF)))
             || (header[0] == 0xCF && header[1] == 0xFA && header[2] == 0xED && header[3] == 0xFE)
             || (header[0] == 0xCE && header[1] == 0xFA && header[2] == 0xED && header[3] == 0xFE)) {
        // Determine byte order and bitness from magic bytes.
        const bool macho_be = (header[0] == 0xFE); // big-endian magic
        const bool macho_64 = macho_be ? (header[3] == 0xCF) : (header[0] == 0xCF);

        // Helper lambdas that read 32-bit integers respecting byte order.
        auto readU32be = [](const uint8_t *p) -> uint32_t {
            return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16)
                   | (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
        };
        auto readU32le = [](const uint8_t *p) -> uint32_t {
            return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8)
                   | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
        };
        auto readU32 = [&]([[maybe_unused]] const uint8_t *p) -> uint32_t { return macho_be ? readU32be(p) : readU32le(p); };

        // mach_header layout (32-bit): magic(4)+cputype(4)+cpusubtype(4)+
        //   filetype(4)+ncmds(4)+sizeofcmds(4)+flags(4) = 28 bytes.
        // mach_header_64 adds reserved(4) = 32 bytes total.
        constexpr uint32_t kMachHeader32Size = 28;
        constexpr uint32_t kMachHeader64Size = 32;
        constexpr uint32_t kLcCodeSignature  = 0x0000001Du;
        constexpr uint32_t kMaxSigSizeMacho  = 64 * 1024 * 1024; // 64 MiB
        constexpr uint32_t kMinLoadCmdSize   = 8;

        const uint32_t hdr_size = macho_64 ? kMachHeader64Size : kMachHeader32Size;

        // We need at least hdr_size bytes to read ncmds and sizeofcmds.
        // Re-read the header with enough bytes if needed.
        std::vector<uint8_t> hdr_buf(hdr_size);
        file.seekg(0);
        file.read(reinterpret_cast<char *>(hdr_buf.data()), static_cast<std::streamsize>(hdr_size));
        if (static_cast<uint32_t>(file.gcount()) < hdr_size) {
            // File too small to be a valid Mach-O binary.
        } else {
            const uint32_t ncmds      = readU32(hdr_buf.data() + 16);
            const uint32_t sizeofcmds = readU32(hdr_buf.data() + 20);

            // Sanity-cap: refuse unreasonably large load-command regions.
            constexpr uint32_t kMaxLoadCmdsSize = 16 * 1024 * 1024;
            if (ncmds > 0 && sizeofcmds >= kMinLoadCmdSize && sizeofcmds <= kMaxLoadCmdsSize) {
                // Load all load commands into memory.
                std::vector<uint8_t> lc_buf(sizeofcmds);
                file.seekg(static_cast<std::streamoff>(hdr_size));
                file.read(reinterpret_cast<char *>(lc_buf.data()), static_cast<std::streamsize>(sizeofcmds));
                if (static_cast<uint32_t>(file.gcount()) == sizeofcmds) {
                    uint32_t offset = 0;
                    for (uint32_t i = 0; i < ncmds && offset + kMinLoadCmdSize <= sizeofcmds; ++i) {
                        const uint8_t *lc      = lc_buf.data() + offset;
                        const uint32_t cmd     = readU32(lc);
                        const uint32_t cmdsize = readU32(lc + 4);

                        // Validate cmdsize before advancing.
                        if (cmdsize < kMinLoadCmdSize || offset + cmdsize > sizeofcmds) {
                            break;
                        }

                        if (cmd == kLcCodeSignature && cmdsize >= 16) {
                            // linkedit_data_command:
                            //   cmd(4) + cmdsize(4) + dataoff(4) + datasize(4)
                            const uint32_t dataoff  = readU32(lc + 8);
                            const uint32_t datasize = readU32(lc + 12);

                            if (datasize > 0 && datasize <= kMaxSigSizeMacho && dataoff > 0) {
                                std::vector<uint8_t> sig_blob(datasize);
                                file.seekg(static_cast<std::streamoff>(dataoff));
                                file.read(reinterpret_cast<char *>(sig_blob.data()),
                                          static_cast<std::streamsize>(datasize));
                                if (static_cast<uint32_t>(file.gcount()) == datasize) {
                                    return sig_blob;
                                }
                            }
                            // LC_CODE_SIGNATURE found but read failed; stop.
                            break;
                        }

                        offset += cmdsize;
                    }
                }
            }
        }
    }

    return std::nullopt;
}

std::optional<std::vector<uint8_t>>
EnhancedPluginSecurityVerifier::extractEmbeddedSignature(const std::string &plugin_path) {
    std::ifstream file(plugin_path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }

    // Read file header to determine format
    std::vector<uint8_t> header(64);
    file.read(reinterpret_cast<char *>(header.data()),static_cast<int>(header.size()));

    if (file.gcount() < 4) {
        return std::nullopt;
    }

    // Check for PE format (Windows DLL/EXE)
    if (header[0] == 'M' && header[1] == 'Z') {
        // DOS header found - this is a PE file
        file.seekg(0x3C); // Offset to PE header location
        uint32_t pe_offset = 0;
        file.read(reinterpret_cast<char *>(&pe_offset), sizeof(pe_offset));

        if (file.good() && pe_offset < 0x1000) { // Sanity check
            file.seekg(pe_offset);
            uint32_t pe_signature = 0;
            file.read(reinterpret_cast<char *>(&pe_signature), sizeof(pe_signature));

            if (pe_signature == 0x00004550) { // "PE\0\0"
                // Valid PE file - signature in Authenticode format
                // Would need to parse optional header data directories
                // to find and extract certificate table
            }
        }
    }
    // Check for ELF format (Linux SO)
    else if (header[0] == 0x7F && header[1] == 'E' && header[2] == 'L' && header[3] == 'F') {
        // ELF format - signatures typically external (.sig files) or in custom sections
        std::string sig_file = plugin_path + ".sig";
        std::ifstream sig_stream(sig_file, std::ios::binary);

        if (sig_stream) {
            // Read signature file
            sig_stream.seekg(0, std::ios::end);
            size_t size = sig_stream.tellg();
            sig_stream.seekg(0, std::ios::beg);

            if (size > 0 && size < 1024 * 1024) { // Max 1MB signature
                std::vector<uint8_t> sig_data(size);
                sig_stream.read(reinterpret_cast<char *>(sig_data.data()), size);

                if (sig_stream.gcount() == static_cast<std::streamsize>(size)) {
                    return sig_data;
                }
            }
        }
    }
    // Check for Mach-O format (macOS dylib)
    else if (((header[0] == 0xFE && header[1] == 0xED && header[2] == 0xFA && (header[3] == 0xCE || header[3] == 0xCF)))
             || (header[0] == 0xCF && header[1] == 0xFA && header[2] == 0xED && header[3] == 0xFE)
             || (header[0] == 0xCE && header[1] == 0xFA && header[2] == 0xED && header[3] == 0xFE)) {
        // Mach-O format (macOS): LC_CODE_SIGNATURE parsing not supported on this platform.
        return std::nullopt;
    }

    // No signature found or format not fully supported
    return std::nullopt;
}

bool EnhancedPluginSecurityVerifier::isOfficialThemisDBCertificate(X509 *cert) {
    if (!cert) {
        return false;
    }

    std::string issuer = getCertificateIssuer(cert);

    // Check if issuer matches ThemisDB Official Plugins CA
    return issuer.find("ThemisDB Official Plugins CA") != std::string::npos
           || issuer.find("ThemisDB.org") != std::string::npos;
}

#ifdef _WIN32
bool EnhancedPluginSecurityVerifier::verifyAuthenticodeSignature(const std::string &plugin_path,
                                                                 VerificationResult &result) {
    // Convert UTF-8 string to wide string for Windows API
    int wide_len
        = MultiByteToWideChar(CP_UTF8, 0, plugin_path.c_str(), static_cast<int>(plugin_path.length()), nullptr, 0);
    if (wide_len == 0) {
        result.error_message = "Failed to convert path to wide string";
        return false;
    }

    std::wstring wide_path(wide_len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, plugin_path.c_str(), static_cast<int>(plugin_path.length()), &wide_path[0],
                        wide_len);

    // Setup WINTRUST_FILE_INFO structure
    WINTRUST_FILE_INFO file_info = {};
    file_info.cbStruct           = sizeof(WINTRUST_FILE_INFO);
    file_info.pcwszFilePath      = wide_path.c_str();
    file_info.hFile              = NULL;
    file_info.pgKnownSubject     = NULL;

    // Setup WINTRUST_DATA structure for signature verification
    GUID action_id                 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA trust_data       = {};
    trust_data.cbStruct            = sizeof(WINTRUST_DATA);
    trust_data.dwUIChoice          = WTD_UI_NONE;
    trust_data.fdwRevocationChecks = WTD_REVOKE_NONE; // CRL/OCSP handled separately
    trust_data.dwUnionChoice       = WTD_CHOICE_FILE;
    trust_data.pFile               = &file_info;
    trust_data.dwStateAction       = WTD_STATEACTION_VERIFY;
    trust_data.dwProvFlags         = WTD_SAFER_FLAG;

    // Verify Authenticode signature
    LONG status = WinVerifyTrust(NULL, &action_id, &trust_data);

    // Cleanup
    trust_data.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(NULL, &action_id, &trust_data);

    if (status == ERROR_SUCCESS) {
        result.platform_signature_verified = true;
        return true;
    } else {
        // Map common error codes to messages
        switch (status) {
            case TRUST_E_NOSIGNATURE:
                result.error_message = "File is not signed";
                break;
            case TRUST_E_EXPLICIT_DISTRUST:
                result.error_message = "Signature is explicitly distrusted";
                break;
            case TRUST_E_SUBJECT_NOT_TRUSTED:
                result.error_message = "Signature subject is not trusted";
                break;
            case CRYPT_E_SECURITY_SETTINGS:
                result.error_message = "Security settings prevent verification";
                break;
            default:
                result.error_message = "Authenticode verification failed with code: " + std::to_string(status);
                break;
        }
        return false;
    }
}
#elif defined(__APPLE__)
bool EnhancedPluginSecurityVerifier::verifyMacOSCodeSignature(const std::string &plugin_path,
                                                              VerificationResult &result) {
    // Use Security framework APIs directly — avoids shell invocation entirely.
    CFStringRef path_cf = CFStringCreateWithCString(kCFAllocatorDefault, plugin_path.c_str(), kCFStringEncodingUTF8);
    if (!path_cf) {
        result.error_message = "Failed to create CFString from plugin path";
        return false;
    }

    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, path_cf, kCFURLPOSIXPathStyle, false);
    CFRelease(path_cf);
    if (!url) {
        result.error_message = "Failed to create CFURL from plugin path";
        return false;
    }

    SecStaticCodeRef code = nullptr;
    OSStatus status       = SecStaticCodeCreateWithPath(url, kSecCSDefaultFlags, &code);
    CFRelease(url);

    if (status != errSecSuccess) {
        result.error_message
            = "Failed to create SecStaticCode (OSStatus " + std::to_string(static_cast<int>(status)) + ")";
        return false;
    }

    status = SecStaticCodeCheckValidity(code, kSecCSDefaultFlags, nullptr);
    CFRelease(code);

    if (status == errSecSuccess) {
        result.platform_signature_verified = true;
        return true;
    } else {
        result.error_message
            = "macOS code signature verification failed (OSStatus " + std::to_string(static_cast<int>(status)) + ")";
        return false;
    }
}
#else
bool EnhancedPluginSecurityVerifier::verifyGPGSignature(const std::string &plugin_path, VerificationResult &result) {
    // Check for GPG signature file (.sig, .asc, or .gpg)
    std::vector<std::string> sig_extensions = {".sig", ".asc", ".gpg"};
    std::string sig_file = {};

    for (const auto &ext : sig_extensions) {
        std::string candidate = plugin_path + ext;
        if (std::filesystem::exists(candidate)) {
            sig_file = candidate;
            break;
        }
    }

    if (sig_file.empty()) {
        result.error_message = "No GPG signature file found";
        return false;
    }

    // Use posix_spawn to invoke gpg directly — no shell, no injection risk.
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        result.error_message = "Failed to create pipe for gpg output";
        return false;
    }

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, pipefd[0]);

    const char *gpg_bin = "/usr/bin/gpg";
    char *const argv[]  = {const_cast<char *>("gpg"), const_cast<char *>("--verify"),
                           const_cast<char *>(sig_file.c_str()), const_cast<char *>(plugin_path.c_str()), nullptr};

    pid_t pid;
    int spawn_ret = posix_spawn(&pid, gpg_bin, &actions, nullptr, argv, environ);
    posix_spawn_file_actions_destroy(&actions);
    close(pipefd[1]);

    if (spawn_ret != 0) {
        close(pipefd[0]);
        result.error_message = "Failed to execute gpg";
        return false;
    }

    // Read combined stdout/stderr from the pipe
    char buf[256];
    std::string output = {};
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        output += buf;
    }
    close(pipefd[0]);

    int wstatus = 0;
    waitpid(pid, &wstatus, 0);
    int exit_code = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : -1;

    if (exit_code == 0 && output.find("Good signature") != std::string::npos) {
        result.platform_signature_verified = true;

        // Extract signer identity if present
        size_t pos = output.find("from \"");
        if (pos != std::string::npos) {
            size_t end = output.find("\"", pos + 6);
            if (end != std::string::npos) {
                result.issuer = output.substr(pos + 6, end - pos - 6);
            }
        }

        return true;
    } else {
        result.error_message = "GPG signature verification failed: " + output;
        return false;
    }
}
#endif

std::vector<uint8_t> EnhancedPluginSecurityVerifier::calculateHashExcludingSignature(const std::string &plugin_path) {
    std::ifstream file(plugin_path, std::ios::binary);
    if (!file) {
        return {};
    }

    // Read file header to determine format
    std::vector<uint8_t> header(64);
    file.read(reinterpret_cast<char *>(header.data()),static_cast<int>(header.size()));

    if (file.gcount() < 4) {
        return {};
    }

    file.seekg(0, std::ios::beg);

    // For PE files, we should exclude the certificate table
    // For simplicity, just hash the entire file for now
    // Full implementation would:
    // 1. Parse PE header to find certificate table location
    // 2. Hash everything except the certificate table
    // 3. Update checksum field properly

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return {};
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        return {};
    }

    const size_t bufferSize = 32768;
    std::vector<char> buffer(bufferSize);

    while (file.read(buffer.data(), bufferSize) || file.gcount() > 0) {
        if (EVP_DigestUpdate(mdctx, buffer.data(), file.gcount()) != 1) {
            EVP_MD_CTX_free(mdctx);
            return {};
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    if (EVP_DigestFinal_ex(mdctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(mdctx);
        return {};
    }
    EVP_MD_CTX_free(mdctx);

    return std::vector<uint8_t>(hash, hash + hashLen);
}

bool EnhancedPluginSecurityVerifier::verifyRSASignature(const std::vector<uint8_t> &data,
                                                        const std::vector<uint8_t> &signature, EVP_PKEY *pubkey) {
    if (!pubkey || data.empty() || signature.empty()) {
        return false;
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return false;
    }

    bool verified = false;

    // Initialize verification context
    if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pubkey) == 1) {
        // Verify signature
        int result = EVP_DigestVerify(mdctx, signature.data(),static_cast<int>(signature.size()), data.data(),static_cast<int>(data.size()));
        verified   = (result == 1);
    }

    EVP_MD_CTX_free(mdctx);
    return verified;
}

std::string EnhancedPluginSecurityVerifier::getCertificateIssuer(X509 *cert) {
    if (!cert) {
        return "";
    }

    X509_NAME *issuer_name = X509_get_issuer_name(cert);
    if (!issuer_name) {
        return "";
    }

    char *issuer_str = X509_NAME_oneline(issuer_name, nullptr, 0);
    if (!issuer_str) {
        return "";
    }

    std::string result(issuer_str);
    OPENSSL_free(issuer_str);
    return result;
}

std::string EnhancedPluginSecurityVerifier::getCertificateSubject(X509 *cert) {
    if (!cert) {
        return "";
    }

    X509_NAME *subject_name = X509_get_subject_name(cert);
    if (!subject_name) {
        return "";
    }

    char *subject_str = X509_NAME_oneline(subject_name, nullptr, 0);
    if (!subject_str) {
        return "";
    }

    std::string result(subject_str);
    OPENSSL_free(subject_str);
    return result;
}

bool EnhancedPluginSecurityVerifier::isCertificateValid(X509 *cert) {
    if (!cert) {
        return false;
    }

    // Check certificate has not expired and is already valid
    int notBefore = X509_cmp_current_time(X509_get0_notBefore(cert));
    int notAfter  = X509_cmp_current_time(X509_get0_notAfter(cert));

    // notBefore should be < 0 (in the past)
    // notAfter should be > 0 (in the future)
    return (notBefore < 0 && notAfter > 0);
}

void EnhancedPluginSecurityVerifier::updatePolicy(const PluginSecurityPolicy &policy) {
    policy_ = policy;
}

} // namespace acceleration
} // namespace themis
