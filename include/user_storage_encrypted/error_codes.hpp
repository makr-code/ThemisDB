/**
 * @file error_codes.hpp
 * @brief Error codes and diagnostic types for user_storage_encrypted module
 * 
 * Provides explicit error classification for all failure modes across
 * the module's components. These are used in Result<T> error messages
 * and diagnostic events.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstdint>

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @brief Error codes for user_storage_encrypted module
 * 
 * Organized by category for easier debugging and metrics:
 * - 1xxx: Backend errors (gocryptfs, FUSE)
 * - 2xxx: Key rotation errors
 * - 3xxx: Multi-level storage errors
 * - 4xxx: KDF errors
 * - 5xxx: Path validation errors
 */
enum class ErrorCode : uint16_t {
    // Generic
    SUCCESS                          = 0,
    
    // Backend errors (1xxx)
    BACKEND_NOT_AVAILABLE            = 1001,  ///< gocryptfs binary not in PATH
    FUSE_NOT_AVAILABLE               = 1002,  ///< FUSE kernel module not loaded
    MOUNT_TIMEOUT                    = 1003,  ///< Mount operation timed out
    UNMOUNT_TIMEOUT                  = 1004,  ///< Unmount operation timed out
    MOUNT_FAILED                     = 1005,  ///< Mount operation failed
    UNMOUNT_FAILED                   = 1006,  ///< Unmount operation failed
    STALE_MOUNT_DETECTED             = 1007,  ///< Orphaned mount found and cleaned up
    FUSE_PERMISSION_DENIED           = 1008,  ///< Permission denied for FUSE operation
    MOUNT_POINT_INVALID              = 1009,  ///< Mount point path invalid
    ENCRYPTED_DIR_INVALID            = 1010,  ///< Encrypted directory path invalid
    STDIN_DELIVERY_TIMEOUT           = 1011,  ///< Key delivery via stdin timed out
    STDIN_DELIVERY_FAILED            = 1012,  ///< Failed to deliver key via stdin
    COMMAND_EXECUTION_TIMEOUT        = 1013,  ///< Command execution exceeded timeout
    
    // Key rotation errors (2xxx)
    ROTATION_CALLBACK_FAILED         = 2001,  ///< User callback returned error
    ROTATION_CALLBACK_TIMEOUT        = 2002,  ///< User callback timed out
    ROTATION_CALLBACK_EXCEPTION      = 2003,  ///< User callback threw exception
    ROTATION_STORE_FAILURE           = 2004,  ///< Failed to persist rotation state
    ROTATION_INTERVAL_INVALID        = 2005,  ///< Invalid rotation interval
    ROTATION_LEVEL_NOT_SCHEDULED     = 2006,  ///< Level not scheduled for rotation
    
    // Multi-level storage errors (3xxx)
    LEVEL_INITIALIZATION_FAILED      = 3001,  ///< Failed to initialize level
    LEVEL_MOUNT_FAILED               = 3002,  ///< Failed to mount level
    LEVEL_UNMOUNT_FAILED             = 3003,  ///< Failed to unmount level
    LEVEL_CONFIG_INVALID             = 3004,  ///< Invalid level configuration
    KEY_PROVIDER_NOT_FOUND           = 3005,  ///< Key provider not available
    KEY_PROVIDER_ERROR               = 3006,  ///< Key provider returned error
    
    // KDF errors (4xxx)
    KDF_INVALID_MASTER_KEY           = 4001,  ///< Master key is empty or too long
    KDF_SALT_GENERATION_FAILED       = 4002,  ///< Failed to generate random salt
    KDF_SALT_PERSISTENCE_FAILED      = 4003,  ///< Failed to persist salt file
    KDF_DERIVATION_FAILED            = 4004,  ///< KDF computation failed
    KDF_INVALID_PARAMETERS           = 4005,  ///< Invalid KDF parameters
    
    // Path validation errors (5xxx)
    PATH_EMPTY                       = 5001,  ///< Path cannot be empty
    PATH_NOT_ABSOLUTE_OR_RELATIVE    = 5002,  ///< Path not absolute or relative with ./
    PATH_TRAVERSAL_DETECTED          = 5003,  ///< Path contains .. sequences
    PATH_INVALID_CHARACTERS          = 5004,  ///< Path contains disallowed characters
    PATH_SYMLINK_TO_PARENT           = 5005,  ///< Path resolves to parent directory symlink
    PERMISSION_DENIED                = 5006,  ///< Insufficient permissions
};

/**
 * @brief Convert error code to human-readable string
 * @param code The error code
 * @return Human-readable error description
 */
inline std::string errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS: return "SUCCESS";
        case ErrorCode::BACKEND_NOT_AVAILABLE: return "BACKEND_NOT_AVAILABLE";
        case ErrorCode::FUSE_NOT_AVAILABLE: return "FUSE_NOT_AVAILABLE";
        case ErrorCode::MOUNT_TIMEOUT: return "MOUNT_TIMEOUT";
        case ErrorCode::UNMOUNT_TIMEOUT: return "UNMOUNT_TIMEOUT";
        case ErrorCode::MOUNT_FAILED: return "MOUNT_FAILED";
        case ErrorCode::UNMOUNT_FAILED: return "UNMOUNT_FAILED";
        case ErrorCode::STALE_MOUNT_DETECTED: return "STALE_MOUNT_DETECTED";
        case ErrorCode::FUSE_PERMISSION_DENIED: return "FUSE_PERMISSION_DENIED";
        case ErrorCode::MOUNT_POINT_INVALID: return "MOUNT_POINT_INVALID";
        case ErrorCode::ENCRYPTED_DIR_INVALID: return "ENCRYPTED_DIR_INVALID";
        case ErrorCode::STDIN_DELIVERY_TIMEOUT: return "STDIN_DELIVERY_TIMEOUT";
        case ErrorCode::STDIN_DELIVERY_FAILED: return "STDIN_DELIVERY_FAILED";
        case ErrorCode::COMMAND_EXECUTION_TIMEOUT: return "COMMAND_EXECUTION_TIMEOUT";
        case ErrorCode::ROTATION_CALLBACK_FAILED: return "ROTATION_CALLBACK_FAILED";
        case ErrorCode::ROTATION_CALLBACK_TIMEOUT: return "ROTATION_CALLBACK_TIMEOUT";
        case ErrorCode::ROTATION_CALLBACK_EXCEPTION: return "ROTATION_CALLBACK_EXCEPTION";
        case ErrorCode::ROTATION_STORE_FAILURE: return "ROTATION_STORE_FAILURE";
        case ErrorCode::ROTATION_INTERVAL_INVALID: return "ROTATION_INTERVAL_INVALID";
        case ErrorCode::ROTATION_LEVEL_NOT_SCHEDULED: return "ROTATION_LEVEL_NOT_SCHEDULED";
        case ErrorCode::LEVEL_INITIALIZATION_FAILED: return "LEVEL_INITIALIZATION_FAILED";
        case ErrorCode::LEVEL_MOUNT_FAILED: return "LEVEL_MOUNT_FAILED";
        case ErrorCode::LEVEL_UNMOUNT_FAILED: return "LEVEL_UNMOUNT_FAILED";
        case ErrorCode::LEVEL_CONFIG_INVALID: return "LEVEL_CONFIG_INVALID";
        case ErrorCode::KEY_PROVIDER_NOT_FOUND: return "KEY_PROVIDER_NOT_FOUND";
        case ErrorCode::KEY_PROVIDER_ERROR: return "KEY_PROVIDER_ERROR";
        case ErrorCode::KDF_INVALID_MASTER_KEY: return "KDF_INVALID_MASTER_KEY";
        case ErrorCode::KDF_SALT_GENERATION_FAILED: return "KDF_SALT_GENERATION_FAILED";
        case ErrorCode::KDF_SALT_PERSISTENCE_FAILED: return "KDF_SALT_PERSISTENCE_FAILED";
        case ErrorCode::KDF_DERIVATION_FAILED: return "KDF_DERIVATION_FAILED";
        case ErrorCode::KDF_INVALID_PARAMETERS: return "KDF_INVALID_PARAMETERS";
        case ErrorCode::PATH_EMPTY: return "PATH_EMPTY";
        case ErrorCode::PATH_NOT_ABSOLUTE_OR_RELATIVE: return "PATH_NOT_ABSOLUTE_OR_RELATIVE";
        case ErrorCode::PATH_TRAVERSAL_DETECTED: return "PATH_TRAVERSAL_DETECTED";
        case ErrorCode::PATH_INVALID_CHARACTERS: return "PATH_INVALID_CHARACTERS";
        case ErrorCode::PATH_SYMLINK_TO_PARENT: return "PATH_SYMLINK_TO_PARENT";
        case ErrorCode::PERMISSION_DENIED: return "PERMISSION_DENIED";
        default: return "UNKNOWN_ERROR_CODE";
    }
}

/**
 * @brief Diagnostic event for structured logging
 * 
 * All diagnostic events in the module follow this structure to enable
 * operator observability and metrics collection without external tools.
 */
struct DiagnosticEvent {
    enum class Type {
        MOUNT_STARTED,          ///< Mount operation started
        MOUNT_COMPLETED,        ///< Mount operation completed
        MOUNT_FAILED,           ///< Mount operation failed
        UNMOUNT_STARTED,        ///< Unmount operation started
        UNMOUNT_COMPLETED,      ///< Unmount operation completed
        UNMOUNT_FAILED,         ///< Unmount operation failed
        ROTATION_STARTED,       ///< Key rotation started
        ROTATION_COMPLETED,     ///< Key rotation completed
        ROTATION_FAILED,        ///< Key rotation failed
        STALE_MOUNT_RECONCILED, ///< Stale mount cleaned up
        ERROR_DETECTED,         ///< Error occurred
    };
    
    Type type;                              ///< Event type
    int64_t timestamp_ms;                   ///< Milliseconds since epoch
    ErrorCode error_code = ErrorCode::SUCCESS;  ///< Error code (if applicable)
    std::string component;                  ///< Component name (e.g., "gocryptfs_backend")
    std::string message;                    ///< Human-readable message
    std::string level;                      ///< Security level (if applicable)
    int system_errno_val = 0;               ///< System errno (if applicable)
    std::string remediation;                ///< Suggested remediation
    
    /**
     * @brief Convert event to JSON string (simple format)
     * @return JSON-formatted diagnostic event
     */
    std::string toJsonString() const;
};

/**
 * @brief Handler for diagnostic events
 * 
 * Diagnostic events are emitted by all components to enable operator visibility.
 * By default, events are logged via spdlog. Custom handlers can be registered
 * for metrics collection, external observability systems, etc.
 */
using DiagnosticEventHandler = std::function<void(const DiagnosticEvent&)>;

} // namespace user_storage
} // namespace plugins
} // namespace themis
