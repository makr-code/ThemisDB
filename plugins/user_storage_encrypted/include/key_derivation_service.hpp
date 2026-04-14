/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            key_derivation_service.hpp                         ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 11:31:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     114                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 256e7651d1  2026-03-24  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace themis {
namespace plugins {
namespace user_storage {

/**
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
