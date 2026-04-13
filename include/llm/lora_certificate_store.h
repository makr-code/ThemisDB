/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lora_certificate_store.h                           ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 04:16:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     142                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 59bb49e3e5  2026-03-15  feat: add Windows HCERTSTORE fallback and fix stale banne... ║
    • f34b955773  2026-03-15  feat: implement LoRACertificateStore and fail-closed cert... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <vector>

namespace themis {
namespace llm {

/**
 * @brief Certificate store for LoRA adapter signature verification.
 *
 * Provides X.509 certificate PEM lookup by SHA-256 fingerprint.
 * Certificates are sourced from:
 *   1. In-memory cache (populated via registerCertificate or loaded from disk).
 *   2. Filesystem store at the configured directory path
 *      (default: config/security/lora_certs/).
 *   3. System certificate store (/etc/ssl/certs on Linux) as a fallback.
 *
 * Thread-safe for concurrent lookups.
 */
class LoRACertificateStore {
public:
    /**
     * @brief Construct a certificate store backed by the given directory.
     *
     * @param store_path  Directory containing PEM files named by fingerprint
     *                    (e.g., "config/security/lora_certs/").
     *                    May be empty to disable filesystem lookup.
     * @param system_store_path  Path to the system CA bundle or certificate
     *                           directory (e.g., "/etc/ssl/certs").
     *                           May be empty to disable system-store fallback.
     */
    explicit LoRACertificateStore(
        const std::string& store_path = "config/security/lora_certs/",
        const std::string& system_store_path = "/etc/ssl/certs");

    ~LoRACertificateStore() = default;

    // Non-copyable, movable
    LoRACertificateStore(const LoRACertificateStore&) = delete;
    LoRACertificateStore& operator=(const LoRACertificateStore&) = delete;
    LoRACertificateStore(LoRACertificateStore&&) = default;
    LoRACertificateStore& operator=(LoRACertificateStore&&) = default;

    /**
     * @brief Look up a certificate PEM by its SHA-256 fingerprint.
     *
     * Lookup order:
     *   1. In-memory cache.
     *   2. Filesystem store (store_path/<fingerprint>.pem).
     *   3. System certificate store (system_store_path/).
     *
     * @param fingerprint  Hex-encoded SHA-256 fingerprint (64 chars).
     * @return PEM string if found, std::nullopt otherwise.
     */
    std::optional<std::string> lookupByFingerprint(
        const std::string& fingerprint) const;

    /**
     * @brief Register a certificate in the in-memory cache.
     *
     * Does not write to disk. Use this for transient trust anchors or
     * test fixtures.
     *
     * @param fingerprint  Hex-encoded SHA-256 fingerprint.
     * @param cert_pem     PEM-encoded X.509 certificate.
     */
    void registerCertificate(const std::string& fingerprint,
                             const std::string& cert_pem);

    /**
     * @brief Remove a certificate from the in-memory cache.
     *
     * @param fingerprint  Fingerprint to evict.
     */
    void evictCertificate(const std::string& fingerprint);

    /**
     * @brief Return the configured local store path.
     */
    const std::string& storePath() const { return store_path_; }

    /**
     * @brief Return the configured system store path.
     */
    const std::string& systemStorePath() const { return system_store_path_; }

private:
    std::string store_path_;
    std::string system_store_path_;

    mutable std::mutex cache_mutex_;
    mutable std::unordered_map<std::string, std::string> cert_cache_;

    // Load a PEM file from disk and return its contents.
    static std::optional<std::string> loadPemFile(const std::string& path);

    // Compute SHA-256 fingerprint of a PEM certificate and compare.
    static bool fingerprintMatches(const std::string& cert_pem,
                                   const std::string& fingerprint);

    // Search the system certificate directory for a cert matching fingerprint
    // (Linux/macOS: iterates PEM/CRT files under system_store_path_).
    std::optional<std::string> searchSystemStore(
        const std::string& fingerprint) const;

#if defined(_WIN32)
    // Search the Windows system certificate store (HCERTSTORE) for a cert
    // matching fingerprint.  Falls back gracefully if the store cannot be opened.
    static std::optional<std::string> searchWindowsCertStore(
        const std::string& fingerprint);
#endif
};

} // namespace llm
} // namespace themis
