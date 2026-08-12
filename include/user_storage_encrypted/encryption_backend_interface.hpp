/**
 * @file encryption_backend_interface.hpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include "security_level.hpp"

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @brief Result type for operations that can fail
 * 
 * Simple Result<T> implementation for plugin API
 */
template<typename T>
class Result {
public:
    // Success constructor
    explicit Result(T value) 
        : value_(std::move(value))
        , success_(true) 
    {}
    
    // Error constructor
    static Result error(const std::string& error_msg) {
        Result r;
        r.success_ = false;
        r.error_ = error_msg;
        return r;
    }
    
    bool isSuccess() const { return success_; }
    bool isError() const { return !success_; }
    
    const T& value() const { 
        if (!success_) throw std::runtime_error("Accessing value of failed Result");
        return value_; 
    }
    
    T& value() { 
        if (!success_) throw std::runtime_error("Accessing value of failed Result");
        return value_; 
    }
    
    const std::string& error() const { return error_; }
    
private:
    Result() : success_(false) {}
    
    T value_;
    bool success_;
    std::string error_;
};

// Specialization for void
template<>
class Result<void> {
public:
    Result() : success_(true) {}
    
    static Result error(const std::string& error_msg) {
        Result r;
        r.success_ = false;
        r.error_ = error_msg;
        return r;
    }
    
    bool isSuccess() const { return success_; }
    bool isError() const { return !success_; }
    const std::string& error() const { return error_; }
    
private:
    bool success_;
    std::string error_;
};

/**
 * @brief Abstract interface for encryption backends
 * 
 * Enables pluggable encryption implementations:
 * - gocryptfs (filesystem-level encryption)
 * - fscrypt (Linux ext4/f2fs encryption)
 * - encfs (older FUSE-based encryption)
 * - Custom implementations
 */
class EncryptionBackendInterface {
public:
    virtual ~EncryptionBackendInterface() = default;
    
    /**
     * @brief Initialize encryption backend
     * 
     * @param config_json Configuration as JSON string
     * @return Result indicating success or error
     */
    virtual Result<void> initialize(const std::string& config_json) = 0;
    
    /**
     * @brief Create and initialize an encrypted container
     * 
     * @param encrypted_dir Path to encrypted data directory
     * @param mount_point Path where decrypted data will be accessible
     * @param key_material Encryption key bytes (32 bytes for AES-256)
     * @return Result indicating success or error
     */
    virtual Result<void> createContainer(
        const std::string& encrypted_dir,
        const std::string& mount_point,
        const std::vector<uint8_t>& key_material
    ) = 0;
    
    /**
     * @brief Mount an encrypted container
     * 
     * @param encrypted_dir Path to encrypted data directory
     * @param mount_point Path where decrypted data will be accessible
     * @param key_material Encryption key bytes
     * @return Result indicating success or error
     */
    virtual Result<void> mountContainer(
        const std::string& encrypted_dir,
        const std::string& mount_point,
        const std::vector<uint8_t>& key_material
    ) = 0;
    
    /**
     * @brief Unmount an encrypted container
     * 
     * @param mount_point Path to mounted container
     * @return Result indicating success or error
     */
    virtual Result<void> unmountContainer(
        const std::string& mount_point
    ) = 0;
    
    /**
     * @brief Check if a container is currently mounted
     * 
     * @param mount_point Path to check
     * @return true if mounted, false otherwise
     */
    virtual bool isMounted(const std::string& mount_point) = 0;
    
    /**
     * @brief Get backend name (e.g., "gocryptfs", "fscrypt")
     */
    virtual std::string getBackendName() const = 0;
    
    /**
     * @brief Get backend version
     */
    virtual std::string getBackendVersion() const = 0;
    
    /**
     * @brief Check if backend is available on the system
     * 
     * @return Result with error message if backend is not available
     */
    virtual Result<void> checkAvailability() = 0;
};

} // namespace user_storage
} // namespace plugins
} // namespace themis

