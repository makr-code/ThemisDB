#include "llm/lora_certificate_store.h"

#include <spdlog/spdlog.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/sha.h>

#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>

namespace themis {
namespace llm {

namespace {

// Convert a raw byte digest to lowercase hex string.
std::string bytesToHex(const unsigned char* bytes, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(bytes[i]);
    }
    return oss.str();
}

// Compute the SHA-256 fingerprint of the DER-encoded subject public key
// (SPKI fingerprint) — the same format commonly used to identify certs.
// Returns empty string on failure.
std::string computeCertFingerprint(X509* cert) {
    if (!cert) return {};

    unsigned char digest[SHA256_DIGEST_LENGTH];
    unsigned int digest_len = SHA256_DIGEST_LENGTH;

    if (X509_digest(cert, EVP_sha256(), digest, &digest_len) != 1) {
        return {};
    }

    return bytesToHex(digest, digest_len);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

LoRACertificateStore::LoRACertificateStore(const std::string& store_path,
                                           const std::string& system_store_path)
    : store_path_(store_path), system_store_path_(system_store_path) {
    spdlog::debug("LoRACertificateStore: local_store='{}' system_store='{}'",
                  store_path_, system_store_path_);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::optional<std::string> LoRACertificateStore::lookupByFingerprint(
    const std::string& fingerprint) const {

    if (fingerprint.empty()) {
        spdlog::warn("LoRACertificateStore: empty fingerprint provided");
        return std::nullopt;
    }

    // 1. In-memory cache
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = cert_cache_.find(fingerprint);
        if (it != cert_cache_.end()) {
            spdlog::debug("LoRACertificateStore: cache hit for fingerprint {}",
                          fingerprint);
            return it->second;
        }
    }

    // 2. Filesystem store: <store_path_>/<fingerprint>.pem
    if (!store_path_.empty()) {
        std::string local_path = store_path_;
        if (local_path.back() != '/' && local_path.back() != '\\') {
            local_path += '/';
        }
        local_path += fingerprint + ".pem";

        auto pem = loadPemFile(local_path);
        if (pem.has_value()) {
            spdlog::debug("LoRACertificateStore: loaded cert from filesystem: {}",
                          local_path);
            // Populate cache for future lookups
            std::lock_guard<std::mutex> lock(cache_mutex_);
            cert_cache_[fingerprint] = *pem;
            return pem;
        }
    }

    // 3. System certificate store fallback
    if (!system_store_path_.empty()) {
        auto pem = searchSystemStore(fingerprint);
        if (pem.has_value()) {
            spdlog::debug("LoRACertificateStore: found cert in system store for {}",
                          fingerprint);
            std::lock_guard<std::mutex> lock(cache_mutex_);
            cert_cache_[fingerprint] = *pem;
            return pem;
        }
    }

    spdlog::warn("LoRACertificateStore: certificate not found for fingerprint {}",
                 fingerprint);
    return std::nullopt;
}

void LoRACertificateStore::registerCertificate(const std::string& fingerprint,
                                               const std::string& cert_pem) {
    if (fingerprint.empty() || cert_pem.empty()) {
        spdlog::warn("LoRACertificateStore: registerCertificate called with empty args");
        return;
    }
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cert_cache_[fingerprint] = cert_pem;
    spdlog::debug("LoRACertificateStore: registered certificate for fingerprint {}",
                  fingerprint);
}

void LoRACertificateStore::evictCertificate(const std::string& fingerprint) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cert_cache_.erase(fingerprint);
    spdlog::debug("LoRACertificateStore: evicted certificate for fingerprint {}",
                  fingerprint);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::optional<std::string> LoRACertificateStore::loadPemFile(
    const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::nullopt;
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    std::string pem = oss.str();
    if (pem.empty()) {
        return std::nullopt;
    }
    return pem;
}

bool LoRACertificateStore::fingerprintMatches(const std::string& cert_pem,
                                              const std::string& fingerprint) {
    if (cert_pem.empty() || fingerprint.empty()) return false;

    BIO* bio = BIO_new_mem_buf(cert_pem.data(),
                               static_cast<int>(cert_pem.size()));
    if (!bio) return false;

    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    if (!cert) return false;

    std::string computed = computeCertFingerprint(cert);
    X509_free(cert);

    // Case-insensitive comparison
    std::string lower_fp = fingerprint;
    std::transform(lower_fp.begin(), lower_fp.end(), lower_fp.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    std::string lower_computed = computed;
    std::transform(lower_computed.begin(), lower_computed.end(),
                   lower_computed.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return lower_fp == lower_computed;
}

std::optional<std::string> LoRACertificateStore::searchSystemStore(
    const std::string& fingerprint) const {

    std::error_code ec;
    if (!std::filesystem::exists(system_store_path_, ec) || ec) {
        spdlog::debug("LoRACertificateStore: system store path '{}' not accessible",
                      system_store_path_);
        return std::nullopt;
    }

    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(
             system_store_path_,
             std::filesystem::directory_options::skip_permission_denied, ec)) {

        if (ec) break;

        const auto& p = entry.path();
        if (!entry.is_regular_file(ec) || ec) continue;

        const std::string ext = p.extension().string();
        if (ext != ".pem" && ext != ".crt" && ext != ".cer") continue;

        auto pem = loadPemFile(p.string());
        if (!pem.has_value()) continue;

        if (fingerprintMatches(*pem, fingerprint)) {
            spdlog::debug(
                "LoRACertificateStore: matched system cert '{}' for fingerprint {}",
                p.string(), fingerprint);
            return pem;
        }
    }

    return std::nullopt;
}

} // namespace llm
} // namespace themis
