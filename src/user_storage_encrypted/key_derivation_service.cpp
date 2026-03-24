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
}

} // namespace user_storage
} // namespace plugins
} // namespace themis
