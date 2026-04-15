/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            key_derivation_service.cpp                         ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 04:15:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     122                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 256e7651d1  2026-03-24  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * key_derivation_service.cpp
 *
 * Argon2id-based key derivation for ThemisDB User Encrypted Storage.
 * Requires libargon2 (apt: libargon2-dev / libargon2-1).
 */

#include "../include/key_derivation_service.hpp"

#include <argon2.h>
#include <stdexcept>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>

namespace themis {
namespace plugins {
namespace user_storage {

Argon2idKeyDerivationService::Argon2idKeyDerivationService(const Argon2idParams& params)
    : params_(params) {}

std::vector<uint8_t> Argon2idKeyDerivationService::derive(
    const std::vector<uint8_t>& master_key,
    const std::string& user_id,
    const std::string& container_id,
    const std::vector<uint8_t>& salt
) {
    if (master_key.empty()) {
        throw std::invalid_argument("master_key must not be empty");
    }
    if (salt.size() < 8) {
        throw std::invalid_argument("salt must be at least 8 bytes");
    }

    // Build password: master_key || user_id bytes || container_id bytes.
    // This ensures domain separation between containers and users.
    std::vector<uint8_t> password;
    password.reserve(master_key.size() + user_id.size() + container_id.size());
    password.insert(password.end(), master_key.begin(), master_key.end());
    password.insert(password.end(),
                    reinterpret_cast<const uint8_t*>(user_id.data()),
                    reinterpret_cast<const uint8_t*>(user_id.data()) + user_id.size());
    password.insert(password.end(),
                    reinterpret_cast<const uint8_t*>(container_id.data()),
                    reinterpret_cast<const uint8_t*>(container_id.data()) + container_id.size());

    std::vector<uint8_t> derived_key(params_.output_len);

    int rc = argon2id_hash_raw(
        params_.iterations,
        params_.memory_kb,
        params_.parallelism,
        password.data(),  password.size(),
        salt.data(),      salt.size(),
        derived_key.data(), derived_key.size()
    );

    // Securely clear the combined password material.
    explicit_bzero(password.data(), password.size());

    if (rc != ARGON2_OK) {
        throw std::runtime_error(
            std::string("Argon2id derivation failed: ") + argon2_error_message(rc)
        );
    }

    return derived_key;
}

std::vector<uint8_t> Argon2idKeyDerivationService::generateSalt(size_t length) {
    if (length == 0) {
        throw std::invalid_argument("Salt length must be > 0");
    }

    std::vector<uint8_t> salt(length);

    int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        throw std::runtime_error("Failed to open /dev/urandom for salt generation");
    }

    ssize_t total = 0;
    while (static_cast<size_t>(total) < length) {
        ssize_t n = read(fd, salt.data() + total, length - static_cast<size_t>(total));
        if (n <= 0) {
            close(fd);
            throw std::runtime_error("Failed to read random bytes for salt");
        }
        total += n;
    }
    close(fd);

    return salt;
}

} // namespace user_storage
} // namespace plugins
} // namespace themis
