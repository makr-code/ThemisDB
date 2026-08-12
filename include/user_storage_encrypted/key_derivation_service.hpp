/**
 * @file key_derivation_service.hpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "encryption_backend_interface.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

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
 * @brief Abstract interface for per-container key derivation with domain separation.
 */
class KeyDerivationService {
public:
    virtual ~KeyDerivationService() = default;

    virtual std::vector<uint8_t> derive(
        const std::vector<uint8_t>& master_key,
        const std::string& user_id,
        const std::string& container_id,
        const std::vector<uint8_t>& salt
    ) = 0;

    virtual std::vector<uint8_t> generateSalt(size_t length) = 0;
};

/**
 * @brief Argon2id parameters.
 *
 * Defaults follow OWASP recommendations:
 *   - m = 65536 (64 MB), t = 3 iterations, p = 4 threads, output = 32 bytes
 */
struct Argon2idParams {
    uint32_t memory_kb   = 65536;
    uint32_t iterations  = 3;
    uint32_t parallelism = 4;
    uint32_t output_len  = 32;
};

/**
 * @brief Argon2id key derivation service — unified implementation.
 *
 * Inherits from both IKeyDerivationService (Result<>-based API) and
 * KeyDerivationService (plain-vector API) so a single instance can be used
 * with either interface.
 *
 * Parameters (NIST / OWASP recommended for interactive use):
 *   - Memory:      m = 65536 KiB (64 MiB)
 *   - Iterations:  t = 3
 *   - Parallelism: p = 4
 *   - Output:      32 bytes (256 bit)
 *
 * Requires libargon2 (apt: libargon2-dev).
 */
class Argon2idKeyDerivationService : public IKeyDerivationService, public KeyDerivationService {
public:
    // KDF parameters
    static constexpr uint32_t kMemoryCost  = 65536; ///< KiB (64 MiB)
    static constexpr uint32_t kTimeCost    = 3;
    static constexpr uint32_t kParallelism = 4;
    static constexpr uint32_t kKeyLength   = 32;    ///< bytes

    explicit Argon2idKeyDerivationService(const Argon2idParams& params = Argon2idParams{});
    ~Argon2idKeyDerivationService() override = default;

    // -----------------------------------------------------------------------
    // Injectable KDF bridge (STUB #31 — Argon2id SHA-256 fallback)
    // -----------------------------------------------------------------------
    using DeriveKeyFn = std::function<Result<std::vector<uint8_t>>(
        const std::vector<uint8_t>& master_key,
        const std::vector<uint8_t>& salt)>;

    /// Inject a custom KDF (e.g., real Argon2id). Pass empty fn to restore default.
    static void setDeriveKeyFn(DeriveKeyFn fn);

    // IKeyDerivationService interface
    Result<std::vector<uint8_t>> deriveKey(
        const std::vector<uint8_t>& master_key,
        const std::vector<uint8_t>& salt
    ) const override;

    Result<std::vector<uint8_t>> generateSalt() const override;

    Result<std::vector<uint8_t>> loadOrCreateSalt(
        const std::string& salt_file_path
    ) const override;

    // KeyDerivationService interface (domain-separated derive)
    std::vector<uint8_t> derive(
        const std::vector<uint8_t>& master_key,
        const std::string& user_id,
        const std::string& container_id,
        const std::vector<uint8_t>& salt
    ) override;

    std::vector<uint8_t> generateSalt(size_t length) override;

private:
    Argon2idParams params_;
};

/// @deprecated Use Argon2idKeyDerivationService which now implements both interfaces.
using Argon2idContainerKeyService = Argon2idKeyDerivationService;

} // namespace user_storage
} // namespace plugins
} // namespace themis
