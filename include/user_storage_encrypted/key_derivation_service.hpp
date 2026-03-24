#pragma once

#include "encryption_backend_interface.hpp"
#include <string>
#include <vector>
#include <cstdint>

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
};

} // namespace user_storage
} // namespace plugins
} // namespace themis
