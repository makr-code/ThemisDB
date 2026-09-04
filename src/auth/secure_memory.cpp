/**
 * @file secure_memory.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Stub Implementation**: Provides memory security utilities for sensitive
 *       credential storage (passwords, keys, tokens). Uses OpenSSL's OPENSSL_cleanse
 *       for secure zeroing and platform-specific mlock for preventing swaps.
 */

#include "auth/secure_memory.h"
#include <spdlog/spdlog.h>
#include <openssl/crypto.h>

namespace themis {
namespace auth {

// SecureString implementation
SecureString::SecureString(const std::string& data) : data_(data) {
    secure_mlock(data_.data(), data_.capacity());
}

SecureString::~SecureString() {
    securely_clear();
}

void SecureString::securely_clear() noexcept {
    if (!data_.empty()) {
        OPENSSL_cleanse(data_.data(),static_cast<int>(data_.size()));
        data_.clear();
    }
}

std::string SecureString::extract() noexcept {
    std::string result = data_;
    securely_clear();
    return result;
}

const std::string& SecureString::get() const noexcept {
    return data_;
}

// SecureVector implementation
template <typename T>
SecureVector<T>::SecureVector(const std::vector<T>& data) : data_(data) {
    if (!data_.empty()) {
        secure_mlock(data_.data(), data_.capacity() * sizeof(T));
    }
}

template <typename T>
SecureVector<T>::~SecureVector() {
    securely_clear();
}

template <typename T>
void SecureVector<T>::securely_clear() noexcept {
    if (!data_.empty()) {
        OPENSSL_cleanse(data_.data(),static_cast<int>(data_.size()) * sizeof(T));
        data_.clear();
    }
}

template <typename T>
std::vector<T> SecureVector<T>::extract() noexcept {
    std::vector<T> result = data_;
    securely_clear();
    return result;
}

template <typename T>
const std::vector<T>& SecureVector<T>::get() const noexcept {
    return data_;
}

// Explicit template instantiations
template class SecureVector<uint8_t>;
template class SecureVector<char>;

// Namespace detail: memory locking and zeroing
namespace detail {

void secure_mlock(void* ptr, std::size_t len) noexcept {
    if (!ptr || len == 0) {
      return;
    }
#if defined(THEMIS_HAS_MLOCK)
    if (::mlock(ptr, len) != 0) {
        spdlog::warn("mlock() failed for {} bytes", len);
    }
#elif defined(THEMIS_HAS_VIRTUALLOCK)
    if (!::VirtualLock(ptr, len)) {
        spdlog::warn("VirtualLock() failed for {} bytes", len);
    }
#else
    spdlog::debug("Memory locking not available on this platform");
#endif
}

void secure_munlock(void* ptr, std::size_t len) noexcept {
    if (!ptr || len == 0) {
      return;
    }
#if defined(THEMIS_HAS_MLOCK)
    ::munlock(ptr, len);
#elif defined(THEMIS_HAS_VIRTUALLOCK)
    ::VirtualUnlock(ptr, len);
#endif
}

void secure_zero(void* ptr, std::size_t len) noexcept {
    if (!ptr || len == 0) {
      return;
    }
    OPENSSL_cleanse(ptr, len);
}

}  // namespace detail

/**
 * @brief Constant-time memory comparison (timing-attack resistant).
 * @param a First buffer.
 * @param b Second buffer.
 * @param len Number of bytes to compare.
 * @return 0 if buffers are equal, non-zero otherwise.
 */
int secure_memcmp(const void* a, const void* b, size_t len) noexcept {
    return CRYPTO_memcmp(a, b, len);
}

}  // namespace auth
}  // namespace themis
