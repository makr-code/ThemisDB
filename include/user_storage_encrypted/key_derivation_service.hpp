/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            key_derivation_service.hpp                         ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-13 04:22:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     187                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8e5567bf5e  2026-03-24  feat(user_storage_encrypted): v0.1.0 stdin key delivery, ... ║
    • 256e7651d1  2026-03-24  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "encryption_backend_interface.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <vector>
#include <cstdint>
#include <string>

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @brief Interface for key derivation services.
 *
 * Implementations derive a fixed-length cryptographic key from a master
 * key (password) and a salt using a password-based KDF.
 */
class IKeyDerivationService {
public:
    virtual ~IKeyDerivationService() = default;

    /**
     * @brief Derive a 256-bit (32-byte) key from master_key and salt.
     *
     * @param master_key  Raw key material (must not be empty).
     * @param salt        Per-container or per-user salt (must not be empty).
     * @return 32-byte derived key on success, or error.
     */
    virtual Result<std::vector<uint8_t>> deriveKey(
        const std::vector<uint8_t>& master_key,
        const std::vector<uint8_t>& salt
    ) const = 0;

    /**
     * @brief Generate a cryptographically random salt suitable for this KDF.
     *
     * @return 32-byte random salt.
     */
    virtual Result<std::vector<uint8_t>> generateSalt() const = 0;

    /**
     * @brief Load salt from a file, creating it with generateSalt() if absent.
     *
     * @param salt_file_path  Path to the salt file (e.g., ".themis_kdf_salt").
     * @return Salt bytes.
     */
    virtual Result<std::vector<uint8_t>> loadOrCreateSalt(
        const std::string& salt_file_path
    ) const = 0;
};

/**
 * @brief Argon2id key derivation service.
 *
 * Parameters (NIST / OWASP recommended for interactive use):
 *   - Memory:      m = 65536 KiB (64 MiB)
 *   - Iterations:  t = 3
 *   - Parallelism: p = 4
 *   - Output:      32 bytes (256 bit)
 *
 * Requires libargon2 (apt: libargon2-dev).
 */
class Argon2idKeyDerivationService : public IKeyDerivationService {
public:
    // KDF parameters
    static constexpr uint32_t kMemoryCost  = 65536; ///< KiB
    static constexpr uint32_t kTimeCost    = 3;
    static constexpr uint32_t kParallelism = 4;
    static constexpr uint32_t kKeyLength   = 32;    ///< bytes

    Result<std::vector<uint8_t>> deriveKey(
        const std::vector<uint8_t>& master_key,
        const std::vector<uint8_t>& salt
    ) const override;

    Result<std::vector<uint8_t>> generateSalt() const override;

    Result<std::vector<uint8_t>> loadOrCreateSalt(
        const std::string& salt_file_path
    ) const override;
 * @brief Abstract interface for per-container key derivation.
 *
 * Implementations derive a fixed-length encryption key from a master key,
 * an optional user identifier, a container identifier, and a per-container
 * random salt.  The derived key is used as the gocryptfs passphrase.
 */
class KeyDerivationService {
public:
    virtual ~KeyDerivationService() = default;

    /**
     * @brief Derive a container encryption key.
     *
     * @param master_key    Master secret (never stored; provided at startup)
     * @param user_id       Optional user identifier for domain separation
     * @param container_id  Unique container path / identifier
     * @param salt          16-byte (minimum) random salt; stored per-container
     * @return              Derived key bytes (32 bytes for AES-256-GCM)
     */
    virtual std::vector<uint8_t> derive(
        const std::vector<uint8_t>& master_key,
        const std::string& user_id,
        const std::string& container_id,
        const std::vector<uint8_t>& salt
    ) = 0;

    /**
     * @brief Generate a cryptographically random salt.
     *
     * @param length  Desired salt length in bytes (default: 16)
     * @return        Random salt bytes
     */
    virtual std::vector<uint8_t> generateSalt(size_t length = 16) = 0;
};

/**
 * @brief Argon2id parameters.
 *
 * Defaults follow OWASP recommendations:
 *   - m = 65536 (64 MB)
 *   - t = 3 iterations
 *   - p = 4 threads
 *   - output = 32 bytes (AES-256-GCM key size)
 */
struct Argon2idParams {
    uint32_t memory_kb   = 65536;
    uint32_t iterations  = 3;
    uint32_t parallelism = 4;
    uint32_t output_len  = 32;
};

/**
 * @brief Argon2id-based KeyDerivationService implementation.
 *
 * Uses libargon2 (argon2id_hash_raw) to derive a 256-bit container key from
 * the master key plus domain-separation fields.  The combined "password" fed
 * to Argon2id is: master_key || user_id || container_id.
 *
 * Latency budget: ≤ 200 ms on reference hardware (4-core / 4 GB RAM).
 */
class Argon2idKeyDerivationService : public KeyDerivationService {
public:
    explicit Argon2idKeyDerivationService(const Argon2idParams& params = Argon2idParams{});
    ~Argon2idKeyDerivationService() override = default;

    std::vector<uint8_t> derive(
        const std::vector<uint8_t>& master_key,
        const std::string& user_id,
        const std::string& container_id,
        const std::vector<uint8_t>& salt
    ) override;

    std::vector<uint8_t> generateSalt(size_t length = 16) override;

private:
    Argon2idParams params_;
};

} // namespace user_storage
} // namespace plugins
} // namespace themis
