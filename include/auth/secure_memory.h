/**
 * @file secure_memory.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <openssl/crypto.h>  // OPENSSL_cleanse, CRYPTO_memcmp

// Platform-specific memory locking
#if defined(__linux__) || defined(__APPLE__)
#  include <sys/mman.h>  // mlock / munlock
#  define THEMIS_HAS_MLOCK 1
#elif defined(_WIN32)
#  include <windows.h>   // VirtualLock / VirtualUnlock
#  define THEMIS_HAS_VIRTUALLOCK 1
#endif

namespace themis {
namespace auth {

namespace detail {

/**
 * @brief Lock a memory region against swapping to disk (best-effort; never throws).
 *
 * Linux/macOS: mlock(2)
 * Windows:     VirtualLock() – may require the process to hold the
 *              "Lock pages in memory" user right (SeLockMemoryPrivilege),
 *              configurable via Local Security Policy:
 *              Computer Configuration → Windows Settings → Security Settings →
 *              Local Policies → User Rights Assignment → Lock pages in memory.
 */
inline void secure_mlock(void* ptr, std::size_t len) noexcept {
    if (!ptr || len == 0) return;
#if defined(THEMIS_HAS_MLOCK)
    ::mlock(ptr, len);          // failure is non-fatal; data is still allocated
#elif defined(THEMIS_HAS_VIRTUALLOCK)
    ::VirtualLock(ptr, len);    // failure is non-fatal
#endif
}

/// Unlock a previously locked memory region.
inline void secure_munlock(void* ptr, std::size_t len) noexcept {
    if (!ptr || len == 0) return;
#if defined(THEMIS_HAS_MLOCK)
    ::munlock(ptr, len);
#elif defined(THEMIS_HAS_VIRTUALLOCK)
    ::VirtualUnlock(ptr, len);
#endif
}

/**
 * @brief Zero, unlock, and free a typed array.
 *
 * Uses OPENSSL_cleanse() which is guaranteed not to be optimised away
 * by the compiler (unlike memset which can be elided on dead stores).
 */
template<typename T>
void secure_release(T* ptr, std::size_t count) noexcept {
    if (!ptr || count == 0) return;
    const std::size_t byte_len = count * sizeof(T);
    OPENSSL_cleanse(ptr, byte_len);
    secure_munlock(ptr, byte_len);
    delete[] ptr;
}

} // namespace detail

// ============================================================================
// SecureString
// ============================================================================

/**
 * @brief Secure string for sensitive data (passwords, passphrases, key material).
 *
 * Security guarantees:
 *   - Pages are locked with mlock(2) / VirtualLock to prevent OS paging to disk.
 *   - Memory is zeroed with OPENSSL_cleanse() before deallocation, preventing
 *     sensitive data from appearing in core dumps or being re-read from freed
 *     heap pages by a subsequent allocation.
 *   - No implicit conversion to std::string prevents accidental serialisation
 *     or logging of secrets.
 *   - No stream operator (operator<<) prevents secrets appearing in log output.
 *
 * Copyable: each copy receives its own independently locked allocation so that
 * every copy is individually zeroed on destruction (defence-in-depth).
 */
class SecureString {
public:
    SecureString() noexcept = default;

    explicit SecureString(const char* s) {
        if (s) assign(s, std::strlen(s));
    }

    explicit SecureString(const std::string& s) {
        assign(s.data(), s.size());
    }

    // Copy – each copy receives its own locked allocation.
    SecureString(const SecureString& o) {
        assign(o.data_, o.size_);
    }

    SecureString& operator=(const SecureString& o) {
        if (this != &o) {
            release();
            assign(o.data_, o.size_);
        }
        return *this;
    }

    SecureString& operator=(const char* s) {
        release();
        if (s) assign(s, std::strlen(s));
        return *this;
    }

    SecureString& operator=(const std::string& s) {
        release();
        assign(s.data(), s.size());
        return *this;
    }

    // Move – transfer ownership; source becomes empty.
    SecureString(SecureString&& o) noexcept
        : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr;
        o.size_ = 0;
    }

    SecureString& operator=(SecureString&& o) noexcept {
        if (this != &o) {
            release();
            data_ = o.data_;
            size_ = o.size_;
            o.data_ = nullptr;
            o.size_ = 0;
        }
        return *this;
    }

    ~SecureString() { release(); }

    const char* c_str() const noexcept { return data_ ? data_ : ""; }
    char*       data()  noexcept       { return data_; }  ///< Writable pointer for in-place cleansing
    std::size_t size()  const noexcept { return size_; }
    bool        empty() const noexcept { return size_ == 0; }

    bool operator==(const char*        rhs) const noexcept {
        if (!rhs)       return empty();
        const std::size_t rlen = std::strlen(rhs);
        if (size_ != rlen) return false;
        return size_ == 0 || CRYPTO_memcmp(data_, rhs, size_) == 0;
    }
    bool operator==(const std::string& rhs) const noexcept {
        if (size_ != rhs.size()) return false;
        return size_ == 0 || CRYPTO_memcmp(data_, rhs.data(), size_) == 0;
    }
    bool operator==(const SecureString& rhs) const noexcept {
        if (size_ != rhs.size_) return false;
        return size_ == 0 || CRYPTO_memcmp(data_, rhs.data_, size_) == 0;
    }

    bool operator!=(const char*        rhs) const noexcept { return !(*this == rhs); }
    bool operator!=(const std::string& rhs) const noexcept { return !(*this == rhs); }
    bool operator!=(const SecureString& rhs) const noexcept { return !(*this == rhs); }

private:
    char*       data_ = nullptr;
    std::size_t size_ = 0;

    void assign(const char* src, std::size_t len) {
        if (len == 0) return;
        data_ = new char[len + 1];
        std::memcpy(data_, src, len);
        data_[len] = '\0';
        size_ = len;
        detail::secure_mlock(data_, len + 1);
    }

    void release() noexcept {
        if (data_) {
            detail::secure_release(data_, size_ + 1);
            data_ = nullptr;
            size_ = 0;
        }
    }
};

// Symmetric comparison operators (allow e.g. "password" == some_secure_string)
inline bool operator==(const char*        lhs, const SecureString& rhs) noexcept { return rhs == lhs; }
inline bool operator==(const std::string& lhs, const SecureString& rhs) noexcept { return rhs == lhs; }
inline bool operator!=(const char*        lhs, const SecureString& rhs) noexcept { return !(rhs == lhs); }
inline bool operator!=(const std::string& lhs, const SecureString& rhs) noexcept { return !(rhs == lhs); }

// ============================================================================
// SecureBuffer<T>
// ============================================================================

/**
 * @brief Secure buffer for binary key material (e.g. AES-256 master keys).
 *
 * Provides the same security guarantees as SecureString but for typed
 * byte arrays.  Allows implicit construction from std::vector<T> so that
 * call-sites that currently pass vectors do not need to be changed.
 *
 * @tparam T Element type – typically uint8_t.
 */
template<typename T>
class SecureBuffer {
public:
    SecureBuffer() noexcept = default;

    /// Construct \p n elements initialised to \p val (mirrors std::vector(n, val)).
    SecureBuffer(std::size_t n, T val = T{}) {
        if (n == 0) return;
        data_ = new T[n];
        std::fill(data_, data_ + n, val);
        size_ = n;
        detail::secure_mlock(data_, n * sizeof(T));
    }

    /// Implicit construction from std::vector<T> – makes a locked copy.
    // NOLINTNEXTLINE(google-explicit-constructor)
    SecureBuffer(const std::vector<T>& v) {
        assign(v.data(), v.size());
    }

    // Copy – fresh locked allocation for each copy.
    SecureBuffer(const SecureBuffer& o) {
        assign(o.data_, o.size_);
    }

    SecureBuffer& operator=(const SecureBuffer& o) {
        if (this != &o) { release(); assign(o.data_, o.size_); }
        return *this;
    }

    SecureBuffer& operator=(const std::vector<T>& v) {
        release(); assign(v.data(), v.size()); return *this;
    }

    // Move – transfer ownership; source becomes empty.
    SecureBuffer(SecureBuffer&& o) noexcept
        : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
    }

    SecureBuffer& operator=(SecureBuffer&& o) noexcept {
        if (this != &o) {
            release();
            data_ = o.data_; size_ = o.size_;
            o.data_ = nullptr; o.size_ = 0;
        }
        return *this;
    }

    ~SecureBuffer() { release(); }

    T*          data()  noexcept       { return data_; }
    const T*    data()  const noexcept { return data_; }
    std::size_t size()  const noexcept { return size_; }
    bool        empty() const noexcept { return size_ == 0; }

    T&       operator[](std::size_t i)       noexcept { return data_[i]; }
    const T& operator[](std::size_t i) const noexcept { return data_[i]; }

    bool operator==(const SecureBuffer& o) const noexcept {
        if (size_ != o.size_) return false;
        return size_ == 0 ||
               CRYPTO_memcmp(static_cast<const void*>(data_),
                             static_cast<const void*>(o.data_),
                             size_ * sizeof(T)) == 0;
    }
    bool operator==(const std::vector<T>& v) const noexcept {
        if (size_ != v.size()) return false;
        return size_ == 0 ||
               CRYPTO_memcmp(static_cast<const void*>(data_),
                             static_cast<const void*>(v.data()),
                             size_ * sizeof(T)) == 0;
    }
    bool operator!=(const SecureBuffer& o) const noexcept { return !(*this == o); }
    bool operator!=(const std::vector<T>& v) const noexcept { return !(*this == v); }

private:
    T*          data_ = nullptr;
    std::size_t size_ = 0;

    void assign(const T* src, std::size_t n) {
        if (n == 0) return;
        data_ = new T[n];
        if (src) std::memcpy(data_, src, n * sizeof(T));
        size_ = n;
        detail::secure_mlock(data_, n * sizeof(T));
    }

    void release() noexcept {
        if (data_) {
            detail::secure_release(data_, size_);
            data_ = nullptr;
            size_ = 0;
        }
    }
};

// Symmetric comparison operators for SecureBuffer vs std::vector
template<typename T>
inline bool operator==(const std::vector<T>& lhs, const SecureBuffer<T>& rhs) noexcept { return rhs == lhs; }
template<typename T>
inline bool operator!=(const std::vector<T>& lhs, const SecureBuffer<T>& rhs) noexcept { return !(rhs == lhs); }

} // namespace auth
} // namespace themis
