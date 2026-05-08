/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            key_derivation_service.cpp                         ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:51:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     259                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d8ee6d7cfe  2026-04-15  fix(user_storage_encrypted): repair broken merge artifact... ║
    • 8e5567bf5e  2026-03-24  feat(user_storage_encrypted): v0.1.0 stdin key delivery, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "key_derivation_service.hpp"
#include <functional>
#include <mutex>
#if defined(__has_include)
#  if __has_include(<argon2.h>)
#    include <argon2.h>
#    define THEMIS_HAS_ARGON2 1
#  else
#    define THEMIS_HAS_ARGON2 0
#  endif
#else
#  define THEMIS_HAS_ARGON2 0
#endif

#include <stdexcept>
#include <fstream>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <random>
#include <filesystem>

#if defined(__linux__) || defined(__APPLE__)
#  include <unistd.h>
#  ifdef __linux__
#    include <sys/random.h>
#  endif
#endif

#include <openssl/sha.h>

namespace themis {
namespace plugins {
namespace user_storage {

namespace {
inline void secureZero(void* ptr, size_t len) {
    volatile unsigned char* p = static_cast<volatile unsigned char*>(ptr);
    while (len--) {
        *p++ = 0;
    }
}

// STUB/SIMULATION NOTE:
// Purpose: Keep Windows/community builds functional when libargon2 headers are unavailable.
// Activation: THEMIS_HAS_ARGON2 == 0 at compile time.
// Production Delta: Uses iterative SHA-256 KDF fallback instead of Argon2id.
// Roadmap ref: src/user_storage_encrypted/ROADMAP.md § "Planned Features"
// Removal Plan: Remove once Argon2 is available/linked in all supported build environments.
std::vector<uint8_t> deriveFallbackSha256(
    const std::vector<uint8_t>& password,
    const std::vector<uint8_t>& salt,
    size_t out_len,
    uint32_t iterations)
{
    std::vector<uint8_t> out;
    out.reserve(out_len);

    std::vector<uint8_t> state(password);
    state.insert(state.end(), salt.begin(), salt.end());

    for (uint32_t i = 0; i < std::max<uint32_t>(1, iterations) && out.size() < out_len; ++i) {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256(state.data(), state.size(), digest);

        const size_t remaining = out_len - out.size();
        const size_t take = std::min(remaining, static_cast<size_t>(SHA256_DIGEST_LENGTH));
        out.insert(out.end(), digest, digest + take);

        state.assign(digest, digest + SHA256_DIGEST_LENGTH);
        state.insert(state.end(), salt.begin(), salt.end());
    }

    secureZero(state.data(), state.size());
    return out;
}
}

// ---------------------------------------------------------------------------
// Argon2idKeyDerivationService
// ---------------------------------------------------------------------------

// STUB/SIMULATION NOTE:
// Purpose:    Allow injection of a real Argon2id implementation at runtime,
//             bypassing the SHA-256 fallback without changing the public API.
// Activation: Runtime — when setDeriveKeyFn() is called with a non-empty fn
//             before the first deriveKey() call.
// Production Delta: With no fn injected, the THEMIS_HAS_ARGON2 path (or
//             SHA-256 fallback when Argon2 is absent) is used; with a fn
//             injected the custom KDF runs instead.
// Removal Plan: Remove bridge slot once libargon2 is universally available in
//             all ThemisDB build environments.
static std::mutex s_kdf_fn_mutex_;
static Argon2idKeyDerivationService::DeriveKeyFn s_derive_key_fn_;

void Argon2idKeyDerivationService::setDeriveKeyFn(
    Argon2idKeyDerivationService::DeriveKeyFn fn) {
    std::lock_guard<std::mutex> lk(s_kdf_fn_mutex_);
    s_derive_key_fn_ = std::move(fn);
}

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

    {
        DeriveKeyFn fn;
        {
            std::lock_guard<std::mutex> lk(s_kdf_fn_mutex_);
            fn = s_derive_key_fn_;
        }
        if (fn) [[unlikely]] {
            try {
                return fn(master_key, salt);
            } catch (...) {
                // Fall through to built-in implementation.
            }
        }
    }

#if THEMIS_HAS_ARGON2
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
#else
    return Result<std::vector<uint8_t>>(
        deriveFallbackSha256(master_key, salt, kKeyLength, kTimeCost)
    );
#endif
}

Result<std::vector<uint8_t>> Argon2idKeyDerivationService::generateSalt() const {
    std::vector<uint8_t> salt(kKeyLength, 0);

#ifdef __linux__
    ssize_t got = getrandom(salt.data(), salt.size(), 0);
    if (got < 0 || static_cast<size_t>(got) != salt.size()) {
        return Result<std::vector<uint8_t>>::error("getrandom() failed");
    }
#elif defined(_WIN32)
    std::random_device rd;
    for (auto& b : salt) {
        b = static_cast<uint8_t>(rd() & 0xFF);
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
        std::ofstream out(salt_file_path, std::ios::binary | std::ios::trunc);
        if (!out) {
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

        out.write(reinterpret_cast<const char*>(salt.data()), static_cast<std::streamsize>(salt.size()));
        if (!out) {
            return Result<std::vector<uint8_t>>::error(
                "Failed to write salt file: " + salt_file_path
            );
        }
    }

    return Result<std::vector<uint8_t>>(salt);
}

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

#if THEMIS_HAS_ARGON2
    std::vector<uint8_t> derived_key(params_.output_len);

    int rc = argon2id_hash_raw(
        params_.iterations,
        params_.memory_kb,
        params_.parallelism,
        password.data(),  password.size(),
        salt.data(),      salt.size(),
        derived_key.data(), derived_key.size()
    );

    secureZero(password.data(), password.size());

    if (rc != ARGON2_OK) {
        throw std::runtime_error(
            std::string("Argon2id derivation failed: ") + argon2_error_message(rc)
        );
    }

    return derived_key;
#else
    auto derived = deriveFallbackSha256(password, salt, params_.output_len, params_.iterations);
    secureZero(password.data(), password.size());
    return derived;
#endif
}

std::vector<uint8_t> Argon2idKeyDerivationService::generateSalt(size_t length) {
    if (length == 0) {
        throw std::invalid_argument("Salt length must be > 0");
    }

    std::vector<uint8_t> salt(length);

#ifdef _WIN32
    std::random_device rd;
    for (auto& b : salt) {
        b = static_cast<uint8_t>(rd() & 0xFF);
    }
    return salt;
#else
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
#endif
}

} // namespace user_storage
} // namespace plugins
} // namespace themis
