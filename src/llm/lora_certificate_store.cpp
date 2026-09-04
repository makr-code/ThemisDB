/**
 * @file lora_certificate_store.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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

#if defined(_WIN32)
#  include <windows.h>
#  include <wincrypt.h>
#  include <openssl/x509v3.h>
#endif

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
#if defined(_WIN32)
    {
        auto pem = searchWindowsCertStore(fingerprint);
        if (pem.has_value()) {
            spdlog::debug("LoRACertificateStore: found cert in Windows system store for {}",
                          fingerprint);
            std::lock_guard<std::mutex> lock(cache_mutex_);
            cert_cache_[fingerprint] = *pem;
            return pem;
        }
    }
#else
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
#endif

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
    if (cert_pem.empty() || fingerprint.empty()) {
      return false;
    }

    BIO* bio = BIO_new_mem_buf(cert_pem.data(),
                               static_cast<int>(cert_pem.size()));
    if (!bio) {
      return false;
    }

    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    if (!cert) {
      return false;
    }

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

    // Validate fingerprint format before iterating the store.
    // A SHA-256 fingerprint must be exactly 64 lowercase hex chars.
    if (fingerprint.size() != 64) {
        spdlog::debug("LoRACertificateStore: skipping system store — fingerprint "
                      "size {} is not 64", fingerprint.size());
        return std::nullopt;
    }
    for (char c : fingerprint) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            spdlog::debug("LoRACertificateStore: skipping system store — "
                          "fingerprint contains non-hex character");
            return std::nullopt;
        }
    }

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

        if (ec) {
          break;
        }

        const auto& p = entry.path();
        if (!entry.is_regular_file(ec) || ec) {
          continue;
        }

        const std::string ext = p.extension().string();
        if (ext != ".pem" && ext != ".crt" && ext != ".cer") {
          continue;
        }

        auto pem = loadPemFile(p.string());
        if (!pem.has_value()) {
          continue;
        }

        if (fingerprintMatches(*pem, fingerprint)) {
            spdlog::debug(
                "LoRACertificateStore: matched system cert '{}' for fingerprint {}",
                p.string(), fingerprint);
            return pem;
        }
    }

    return std::nullopt;
}

#if defined(_WIN32)
// ---------------------------------------------------------------------------
// Windows HCERTSTORE integration
// ---------------------------------------------------------------------------
//
// Opens the "MY" and "ROOT" system certificate stores, iterates every
// certificate, converts each DER-encoded CERT_CONTEXT to an OpenSSL X509,
// computes its SHA-256 fingerprint, and returns the PEM string on a match.
//
// The caller is responsible for caching; this function has no side effects
// on the LoRACertificateStore state.
//
std::optional<std::string> LoRACertificateStore::searchWindowsCertStore(
    const std::string& fingerprint) {

    // Store names to search, in priority order
    static const LPCSTR kStoreNames[] = {"MY", "ROOT", "CA", nullptr};

    for (int si = 0; kStoreNames[si] != nullptr; ++si) {
        HCERTSTORE h_store = CertOpenSystemStoreA(0, kStoreNames[si]);
        if (!h_store) {
            spdlog::debug("LoRACertificateStore: cannot open Windows store '{}': error {}",
                          kStoreNames[si], GetLastError());
            continue;
        }

        PCCERT_CONTEXT ctx = nullptr;
        while ((ctx = CertEnumCertificatesInStore(h_store, ctx)) != nullptr) {
            // Convert DER → OpenSSL X509
            const unsigned char* der_ptr = ctx->pbCertEncoded;
            X509* x509 = d2i_X509(nullptr, &der_ptr,
                                   static_cast<long>(ctx->cbCertEncoded));
            if (!x509) {
              continue;
            }

            std::string computed = computeCertFingerprint(x509);
            X509_free(x509);

            if (computed.empty()) {
              continue;
            }

            // Case-insensitive comparison
            std::string lower_fp = fingerprint;
            std::transform(lower_fp.begin(), lower_fp.end(), lower_fp.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            std::string lower_computed = computed;
            std::transform(lower_computed.begin(), lower_computed.end(),
                           lower_computed.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            if (lower_fp == lower_computed) {
                // Convert DER → PEM via OpenSSL BIO
                const unsigned char* der_ptr2 = ctx->pbCertEncoded;
                X509* match = d2i_X509(nullptr, &der_ptr2,
                                       static_cast<long>(ctx->cbCertEncoded));

                std::string pem_str;
                if (match) {
                    BIO* bio = BIO_new(BIO_s_mem());
                    if (bio && PEM_write_bio_X509(bio, match) == 1) {
                        BUF_MEM* mem = nullptr;
                        BIO_get_mem_ptr(bio, &mem);
                        pem_str.assign(mem->data, mem->length);
                    }
                    if (bio) {
                      BIO_free(bio);
                    }
                    X509_free(match);
                }

                CertFreeCertificateContext(ctx);
                CertCloseStore(h_store, 0);

                if (!pem_str.empty()) {
                    spdlog::debug("LoRACertificateStore: matched cert in Windows store "
                                  "'{}' for fingerprint {}",
                                  kStoreNames[si], fingerprint);
                    return pem_str;
                }
            }
        }

        CertCloseStore(h_store, 0);
    }

    spdlog::debug("LoRACertificateStore: certificate not found in Windows system stores "
                  "for fingerprint {}", fingerprint);
    return std::nullopt;
}
#endif  // _WIN32

} // namespace llm
} // namespace themis

