/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            key_derivation_service.cpp                         ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 18:53:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     256                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8e5567bf5e  2026-03-24  feat(user_storage_encrypted): v0.1.0 stdin key delivery, ... ║
    • 256e7651d1  2026-03-24  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "key_derivation_service.hpp"

#include <argon2.h>
#include <fstream>
#include <cstring>
#include <sys/stat.h>

#ifdef __linux__
#  include <sys/random.h>
#else
#  include <unistd.h>
#endif
/*
 * key_derivation_service.cpp
 *
 * Argon2id-based key derivation for ThemisDB User Encrypted Storage.
 * Requires libargon2 (apt: libargon2-dev / libargon2-1).
 */

#include "key_derivation_service.hpp"

#include <argon2.h>
#include <stdexcept>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>

namespace themis {
namespace plugins {
namespace user_storage {

// ---------------------------------------------------------------------------
// Argon2idKeyDerivationService
// ---------------------------------------------------------------------------

Result<std::vector<uint8_t>> Argon2idKeyDerivationService::deriveKey(
    const std::vector<uint8_t>& master_key,
    const std::vector<uint8_t>& salt
) const {
    if (master_key.empty()) {
        return Result<std::vector<uint8_t>>::error("master_key must not be empty");
    }
    if (salt.empty()) {
        return Result<std::vector<uint8_t>>::error("salt must not be empty");
    }

    std::vector<uint8_t> derived(kKeyLength, 0);

    int rc = argon2id_hash_raw(
        kTimeCost,
        kMemoryCost,
        kParallelism,
        master_key.data(), master_key.size(),
        salt.data(),       salt.size(),
        derived.data(),    kKeyLength
    );

    if (rc != ARGON2_OK) {
        return Result<std::vector<uint8_t>>::error(
            std::string("Argon2id failed: ") + argon2_error_message(rc)
        );
    }

    return Result<std::vector<uint8_t>>(derived);
}

Result<std::vector<uint8_t>> Argon2idKeyDerivationService::generateSalt() const {
    std::vector<uint8_t> salt(kKeyLength, 0);

#ifdef __linux__
    ssize_t got = getrandom(salt.data(), salt.size(), 0);
    if (got < 0 || static_cast<size_t>(got) != salt.size()) {
        return Result<std::vector<uint8_t>>::error("getrandom() failed");
    }
#else
    // POSIX fallback: /dev/urandom
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (!urandom) {
        return Result<std::vector<uint8_t>>::error("Failed to open /dev/urandom");
    }
    urandom.read(reinterpret_cast<char*>(salt.data()),
                 static_cast<std::streamsize>(salt.size()));
    if (!urandom) {
        return Result<std::vector<uint8_t>>::error("Failed to read from /dev/urandom");
    }
#endif

    return Result<std::vector<uint8_t>>(salt);
}

Result<std::vector<uint8_t>> Argon2idKeyDerivationService::loadOrCreateSalt(
    const std::string& salt_file_path
) const {
    // Try to load existing salt
    {
        std::ifstream f(salt_file_path, std::ios::binary);
        if (f) {
            f.seekg(0, std::ios::end);
            auto len = f.tellg();
            f.seekg(0);
            if (len > 0) {
                std::vector<uint8_t> salt(static_cast<size_t>(len));
                f.read(reinterpret_cast<char*>(salt.data()), len);
                if (f) {
                    return Result<std::vector<uint8_t>>(salt);
                }
            }
        }
    }

    // Generate and persist a new salt
    auto gen = generateSalt();
    if (gen.isError()) {
        return gen;
    }
    const auto& salt = gen.value();

    {
        // Create with restrictive permissions
        int fd = open(salt_file_path.c_str(),
                      O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (fd < 0) {
            // Another process may have created it concurrently — try to read
            std::ifstream f(salt_file_path, std::ios::binary);
            if (f) {
                f.seekg(0, std::ios::end);
                auto len = f.tellg();
                f.seekg(0);
                if (len > 0) {
                    std::vector<uint8_t> existing(static_cast<size_t>(len));
                    f.read(reinterpret_cast<char*>(existing.data()), len);
                    if (f) {
                        return Result<std::vector<uint8_t>>(existing);
                    }
                }
            }
            return Result<std::vector<uint8_t>>::error(
                "Failed to create salt file: " + salt_file_path
            );
        }

        ssize_t written = write(fd, salt.data(), salt.size());
        close(fd);

        if (written < 0 || static_cast<size_t>(written) != salt.size()) {
            return Result<std::vector<uint8_t>>::error(
                "Failed to write salt file: " + salt_file_path
            );
        }
    }

    return Result<std::vector<uint8_t>>(salt);
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
