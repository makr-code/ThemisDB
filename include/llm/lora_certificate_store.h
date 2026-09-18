/**
 * @file lora_certificate_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <vector>

namespace themis {
namespace llm {

class LoRACertificateStore {
public:
    explicit LoRACertificateStore(
        const std::string& store_path = "config/security/lora_certs/",
        const std::string& system_store_path = "/etc/ssl/certs");

    ~LoRACertificateStore() = default;

    // Non-copyable, movable
    LoRACertificateStore(const LoRACertificateStore&) = delete;
    LoRACertificateStore& operator=(const LoRACertificateStore&) = delete;
    LoRACertificateStore(LoRACertificateStore&&) noexcept = default;
    LoRACertificateStore& operator=(LoRACertificateStore&&) noexcept = default;

    /**
     * @brief Lookup By Fingerprint.
     * @param[in] fingerprint Input parameter.
     * @return Return value.
     */
    std::optional<std::string> lookupByFingerprint(
        const std::string& fingerprint) const;

    /**
     * @brief Register Certificate.
     * @param[in] fingerprint Input parameter.
     * @param[in] cert_pem Input parameter.
     */
    void registerCertificate(const std::string& fingerprint,
                             const std::string& cert_pem);

    /**
     * @brief Evict Certificate.
     * @param[in] fingerprint Input parameter.
     */
    void evictCertificate(const std::string& fingerprint);

    const std::string& storePath() const { return store_path_; }

    const std::string& systemStorePath() const { return system_store_path_; }

private:
    std::string store_path_;
    std::string system_store_path_;

    mutable std::mutex cache_mutex_;
    mutable std::unordered_map<std::string, std::string> cert_cache_;

    /**
     * @brief Load a PEM file from disk and return its contents.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static std::optional<std::string> loadPemFile(const std::string& path);

    /**
     * @brief Compute SHA-256 fingerprint of a PEM certificate and compare.
     * @param[in] cert_pem Input parameter.
     * @param[in] fingerprint Input parameter.
     * @return True when the operation succeeds.
     */
    static bool fingerprintMatches(const std::string& cert_pem,
                                   const std::string& fingerprint);

    /**
     * @brief Search the system certificate directory for a cert matching fingerprint (Linux/macOS: iterates PEM/CRT files under system_store_path_).
     * @param[in] fingerprint Input parameter.
     * @return Return value.
     */
    std::optional<std::string> searchSystemStore(
        const std::string& fingerprint) const;

#if defined(_WIN32)
    /**
     * @brief Search the Windows system certificate store (HCERTSTORE) for a cert matching fingerprint.
     * @param[in] fingerprint Input parameter.
     * @return Return value.
     * @details Falls back gracefully if the store cannot be opened.
     */
    static std::optional<std::string> searchWindowsCertStore(
        const std::string& fingerprint);
#endif
};

} // namespace llm
} // namespace themis
