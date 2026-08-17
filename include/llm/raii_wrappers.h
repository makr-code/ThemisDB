/**
 * @file raii_wrappers.h
 * @brief RAII wrapper classes for resource management in LLM module
 * @version 0.0.1
 * @note Batch 2: RAII & Resource Management Fixes
 * 
 * Provides exception-safe RAII wrappers for:
 * - OpenSSL objects (BIO, EVP_PKEY, EVP_MD_CTX)
 * - File handles
 * - Database connections
 * - GPU memory allocations
 * 
 * All wrappers are move-enabled and exception-safe (noexcept destructors).
 * This replaces manual delete/cleanup patterns with automatic resource management.
 */

#pragma once

#include <memory>
#include <cstdio>
#include <stdexcept>
#include <spdlog/spdlog.h>

// Forward declarations for OpenSSL (only needed if OPENSSL is available)
#ifdef OPENSSL_CRYPTO_H
struct bio_st;  // BIO opaque type
struct evp_pkey_st;  // EVP_PKEY opaque type
struct evp_md_ctx_st;  // EVP_MD_CTX opaque type
#else
// Minimal declarations to avoid including OpenSSL headers here
typedef struct bio_st BIO;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct evp_md_ctx_st EVP_MD_CTX;
#endif

namespace themis {
namespace llm {

/**
 * @class ScopedFile
 * @brief RAII wrapper for FILE* handles
 * 
 * Automatically closes the file on destruction. Supports move semantics
 * to enable use in containers and return values.
 * 
 * Exception-safe: destructor is noexcept and safely handles null pointers.
 * 
 * Example:
 * @code
 * {
 *     ScopedFile f(fopen("data.bin", "rb"));
 *     if (!f) {
 *         throw std::runtime_error("Failed to open file");
 *     }
 *     // Use f.get() to access FILE*
 * }  // fclose() automatically called here
 * @endcode
 */
class ScopedFile {
public:
    /// Construct from FILE* (takes ownership)
    explicit ScopedFile(FILE* fp = nullptr) noexcept : fp_(fp) {}
    
    /// Destructor: closes file if open
    ~ScopedFile() noexcept {
        if (fp_ != nullptr) {
            if (std::fclose(fp_) != 0) {
                spdlog::warn("ScopedFile::~ScopedFile: fclose failed");
            }
            fp_ = nullptr;
        }
    }
    
    // Delete copy semantics - file handle can only be owned by one instance
    ScopedFile(const ScopedFile&) = delete;
    ScopedFile& operator=(const ScopedFile&) = delete;
    
    /// Move constructor: transfer ownership
    ScopedFile(ScopedFile&& other) noexcept : fp_(other.release()) {}
    
    /// Move assignment: transfer ownership with cleanup
    ScopedFile& operator=(ScopedFile&& other) noexcept {
        reset(other.release());
        return *this;
    }
    
    /// Get the underlying FILE* pointer
    FILE* get() const noexcept { return fp_; }
    
    /// Explicit conversion to bool (valid if file is open)
    explicit operator bool() const noexcept { return fp_ != nullptr; }
    
    /// Release ownership without closing
    FILE* release() noexcept {
        FILE* temp = fp_;
        fp_ = nullptr;
        return temp;
    }
    
    /// Close the file and reset to nullptr
    void reset(FILE* fp = nullptr) noexcept {
        if (fp_ != nullptr) {
            if (std::fclose(fp_) != 0) {
                spdlog::warn("ScopedFile::reset: fclose failed");
            }
        }
        fp_ = fp;
    }
    
private:
    FILE* fp_ = nullptr;
};

/**
 * @class ScopedBIO
 * @brief RAII wrapper for OpenSSL BIO (Bio Input/Output)
 * 
 * Automatically frees the BIO on destruction via BIO_free.
 * Supports move semantics for exception-safe code.
 * 
 * Exception-safe: destructor is noexcept.
 * 
 * Example:
 * @code
 * {
 *     ScopedBIO bio(BIO_new_mem_buf(data, size));
 *     if (!bio) {
 *         throw std::runtime_error("Failed to create BIO");
 *     }
 *     // Use bio.get() to access BIO*
 * }  // BIO_free() automatically called here
 * @endcode
 */
class ScopedBIO {
public:
    /// Construct from BIO* (takes ownership)
    explicit ScopedBIO(BIO* bio = nullptr) noexcept : bio_(bio) {}
    
    /// Destructor: frees BIO if not null
    ~ScopedBIO() noexcept {
        if (bio_ != nullptr) {
            BIO_free(bio_);
            bio_ = nullptr;
        }
    }
    
    // Delete copy semantics
    ScopedBIO(const ScopedBIO&) = delete;
    ScopedBIO& operator=(const ScopedBIO&) = delete;
    
    /// Move constructor
    ScopedBIO(ScopedBIO&& other) noexcept : bio_(other.release()) {}
    
    /// Move assignment
    ScopedBIO& operator=(ScopedBIO&& other) noexcept {
        reset(other.release());
        return *this;
    }
    
    /// Get the underlying BIO*
    BIO* get() const noexcept { return bio_; }
    
    /// Explicit conversion to bool
    explicit operator bool() const noexcept { return bio_ != nullptr; }
    
    /// Release ownership without freeing
    BIO* release() noexcept {
        BIO* temp = bio_;
        bio_ = nullptr;
        return temp;
    }
    
    /// Free and reset to nullptr
    void reset(BIO* bio = nullptr) noexcept {
        if (bio_ != nullptr) {
            BIO_free(bio_);
        }
        bio_ = bio;
    }
    
private:
    BIO* bio_ = nullptr;
};

/**
 * @class ScopedEVPKey
 * @brief RAII wrapper for OpenSSL EVP_PKEY
 * 
 * Automatically frees the EVP_PKEY on destruction via EVP_PKEY_free.
 * Supports move semantics.
 * 
 * Exception-safe: destructor is noexcept.
 * 
 * Example:
 * @code
 * {
 *     ScopedEVPKey key(PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr));
 *     if (!key) {
 *         throw std::runtime_error("Failed to read key");
 *     }
 * }  // EVP_PKEY_free() automatically called here
 * @endcode
 */
class ScopedEVPKey {
public:
    /// Construct from EVP_PKEY* (takes ownership)
    explicit ScopedEVPKey(EVP_PKEY* pkey = nullptr) noexcept : pkey_(pkey) {}
    
    /// Destructor: frees EVP_PKEY if not null
    ~ScopedEVPKey() noexcept {
        if (pkey_ != nullptr) {
            EVP_PKEY_free(pkey_);
            pkey_ = nullptr;
        }
    }
    
    // Delete copy semantics
    ScopedEVPKey(const ScopedEVPKey&) = delete;
    ScopedEVPKey& operator=(const ScopedEVPKey&) = delete;
    
    /// Move constructor
    ScopedEVPKey(ScopedEVPKey&& other) noexcept : pkey_(other.release()) {}
    
    /// Move assignment
    ScopedEVPKey& operator=(ScopedEVPKey&& other) noexcept {
        reset(other.release());
        return *this;
    }
    
    /// Get the underlying EVP_PKEY*
    EVP_PKEY* get() const noexcept { return pkey_; }
    
    /// Explicit conversion to bool
    explicit operator bool() const noexcept { return pkey_ != nullptr; }
    
    /// Release ownership without freeing
    EVP_PKEY* release() noexcept {
        EVP_PKEY* temp = pkey_;
        pkey_ = nullptr;
        return temp;
    }
    
    /// Free and reset to nullptr
    void reset(EVP_PKEY* pkey = nullptr) noexcept {
        if (pkey_ != nullptr) {
            EVP_PKEY_free(pkey_);
        }
        pkey_ = pkey;
    }
    
private:
    EVP_PKEY* pkey_ = nullptr;
};

/**
 * @class ScopedEVPContext
 * @brief RAII wrapper for OpenSSL EVP_MD_CTX
 * 
 * Automatically frees the EVP_MD_CTX on destruction via EVP_MD_CTX_free.
 * Supports move semantics.
 * 
 * Exception-safe: destructor is noexcept.
 * 
 * Example:
 * @code
 * {
 *     ScopedEVPContext ctx(EVP_MD_CTX_new());
 *     if (!ctx) {
 *         throw std::runtime_error("Failed to create context");
 *     }
 *     EVP_DigestSignInit(ctx.get(), ...);
 * }  // EVP_MD_CTX_free() automatically called here
 * @endcode
 */
class ScopedEVPContext {
public:
    /// Construct from EVP_MD_CTX* (takes ownership)
    explicit ScopedEVPContext(EVP_MD_CTX* ctx = nullptr) noexcept : ctx_(ctx) {}
    
    /// Destructor: frees EVP_MD_CTX if not null
    ~ScopedEVPContext() noexcept {
        if (ctx_ != nullptr) {
            EVP_MD_CTX_free(ctx_);
            ctx_ = nullptr;
        }
    }
    
    // Delete copy semantics
    ScopedEVPContext(const ScopedEVPContext&) = delete;
    ScopedEVPContext& operator=(const ScopedEVPContext&) = delete;
    
    /// Move constructor
    ScopedEVPContext(ScopedEVPContext&& other) noexcept : ctx_(other.release()) {}
    
    /// Move assignment
    ScopedEVPContext& operator=(ScopedEVPContext&& other) noexcept {
        reset(other.release());
        return *this;
    }
    
    /// Get the underlying EVP_MD_CTX*
    EVP_MD_CTX* get() const noexcept { return ctx_; }
    
    /// Explicit conversion to bool
    explicit operator bool() const noexcept { return ctx_ != nullptr; }
    
    /// Release ownership without freeing
    EVP_MD_CTX* release() noexcept {
        EVP_MD_CTX* temp = ctx_;
        ctx_ = nullptr;
        return temp;
    }
    
    /// Free and reset to nullptr
    void reset(EVP_MD_CTX* ctx = nullptr) noexcept {
        if (ctx_ != nullptr) {
            EVP_MD_CTX_free(ctx_);
        }
        ctx_ = ctx;
    }
    
private:
    EVP_MD_CTX* ctx_ = nullptr;
};

/**
 * @class ScopedConnection
 * @brief RAII wrapper for database connections
 * 
 * Manages the lifecycle of a database connection handle.
 * Automatically closes the connection on destruction.
 * 
 * Exception-safe: destructor is noexcept.
 * 
 * Generic implementation - can be specialized for specific database types.
 */
template<typename DBHandle>
class ScopedConnection {
public:
    using CloseFunction = std::function<void(DBHandle)>;
    
    /// Construct with connection handle and close function
    explicit ScopedConnection(DBHandle conn = nullptr, CloseFunction close_fn = nullptr) noexcept
        : conn_(conn), close_fn_(close_fn) {}
    
    /// Destructor: closes connection if not null
    ~ScopedConnection() noexcept {
        if (conn_ != nullptr && close_fn_) {
            try {
                close_fn_(conn_);
            } catch (...) {
                spdlog::warn("ScopedConnection::~ScopedConnection: close_fn threw exception");
            }
            conn_ = nullptr;
        }
    }
    
    // Delete copy semantics
    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;
    
    /// Move constructor
    ScopedConnection(ScopedConnection&& other) noexcept 
        : conn_(other.release()), close_fn_(other.close_fn_) {}
    
    /// Move assignment
    ScopedConnection& operator=(ScopedConnection&& other) noexcept {
        reset(other.release());
        close_fn_ = other.close_fn_;
        return *this;
    }
    
    /// Get the underlying connection handle
    DBHandle get() const noexcept { return conn_; }
    
    /// Explicit conversion to bool
    explicit operator bool() const noexcept { return conn_ != nullptr; }
    
    /// Release ownership without closing
    DBHandle release() noexcept {
        DBHandle temp = conn_;
        conn_ = nullptr;
        return temp;
    }
    
    /// Close and reset to nullptr
    void reset(DBHandle conn = nullptr) noexcept {
        if (conn_ != nullptr && close_fn_) {
            try {
                close_fn_(conn_);
            } catch (...) {
                spdlog::warn("ScopedConnection::reset: close_fn threw exception");
            }
        }
        conn_ = conn;
    }
    
private:
    DBHandle conn_ = nullptr;
    CloseFunction close_fn_;
};

/**
 * @class ScopedGPUBuffer
 * @brief RAII wrapper for GPU memory allocations
 * 
 * Wraps a pointer to GPU memory and ensures it's freed on destruction.
 * Integrates with the GPU memory manager for proper lifecycle management.
 * 
 * Exception-safe: destructor is noexcept.
 * 
 * Note: This is a simplified wrapper for manual GPU memory. For tracked
 * allocations, use GPUMemoryManager::allocateGPU() which returns pointers
 * managed by MemoryHolder.
 */
class ScopedGPUBuffer {
public:
    using FreeFn = std::function<void(void*)>;
    
    /// Construct with GPU pointer and free function
    explicit ScopedGPUBuffer(void* ptr = nullptr, FreeFn free_fn = nullptr) noexcept
        : ptr_(ptr), free_fn_(free_fn) {}
    
    /// Destructor: frees GPU memory
    ~ScopedGPUBuffer() noexcept {
        if (ptr_ != nullptr && free_fn_) {
            try {
                free_fn_(ptr_);
            } catch (...) {
                spdlog::warn("ScopedGPUBuffer::~ScopedGPUBuffer: free_fn threw exception");
            }
            ptr_ = nullptr;
        }
    }
    
    // Delete copy semantics
    ScopedGPUBuffer(const ScopedGPUBuffer&) = delete;
    ScopedGPUBuffer& operator=(const ScopedGPUBuffer&) = delete;
    
    /// Move constructor
    ScopedGPUBuffer(ScopedGPUBuffer&& other) noexcept 
        : ptr_(other.release()), free_fn_(other.free_fn_) {}
    
    /// Move assignment
    ScopedGPUBuffer& operator=(ScopedGPUBuffer&& other) noexcept {
        reset(other.release());
        free_fn_ = other.free_fn_;
        return *this;
    }
    
    /// Get the GPU buffer pointer
    void* get() const noexcept { return ptr_; }
    
    /// Explicit conversion to bool
    explicit operator bool() const noexcept { return ptr_ != nullptr; }
    
    /// Release ownership without freeing
    void* release() noexcept {
        void* temp = ptr_;
        ptr_ = nullptr;
        return temp;
    }
    
    /// Free and reset to nullptr
    void reset(void* ptr = nullptr) noexcept {
        if (ptr_ != nullptr && free_fn_) {
            try {
                free_fn_(ptr_);
            } catch (...) {
                spdlog::warn("ScopedGPUBuffer::reset: free_fn threw exception");
            }
        }
        ptr_ = ptr;
    }
    
private:
    void* ptr_ = nullptr;
    FreeFn free_fn_;
};

}  // namespace llm
}  // namespace themis
