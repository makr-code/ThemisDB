/**
 * @file user_storage_encrypted_api_contract.h
 * @brief Frozen encrypted user-storage backend, key derivation, and scheduler contract for v1.x.
 * @version 1.0.0
 *
 * ## §Purpose
 *
 * Defines the normative contract for the user_storage_encrypted module covering
 * gocryptfs-backed backend lifecycle (mount/unmount), key derivation service,
 * key rotation scheduling, and multi-level encrypted storage orchestration.
 *
 * ## §API Contracts
 *
 * Key behavioural invariants:
 *   1. Mount operations are atomic: a partially-mounted container is automatically
 *      cleaned up on failure; callers receive kMountFailed with no residual state.
 *   2. Unmount is always attempted even on error paths; failure to unmount
 *      raises kUnmountFailed and triggers an operator alert.
 *   3. Key derivation inputs (passphrase + salt) are zeroed from memory
 *      immediately after the derived key material is handed off to the backend.
 *   4. Key rotation is transactional: the old key remains valid until the new
 *      key is confirmed written; rotation failure leaves old key active and
 *      returns kRotationFailed.
 *   5. Backend unavailability (gocryptfs or FUSE not present) produces
 *      kBackendUnavailable and prevents all storage operations fail-closed.
 *
 * ## §Error Taxonomy
 *
 * | Code  | Constant               | Meaning                                           |
 * |-------|------------------------|---------------------------------------------------|
 * | 0     | kSuccess               | Operation completed without error                 |
 * | 8600  | kMountFailed           | Encrypted container mount operation failed        |
 * | 8601  | kUnmountFailed         | Encrypted container unmount operation failed      |
 * | 8602  | kKeyDerivationFailed   | Key derivation (KDF) operation failed             |
 * | 8603  | kRotationFailed        | Key rotation could not complete atomically        |
 * | 8604  | kBackendUnavailable    | gocryptfs/FUSE backend not available              |
 * | 8605  | kInvalidPath           | Container path fails validation checks            |
 * | 8606  | kPermissionDenied      | Caller lacks required filesystem permissions      |
 * | 8607  | kInternalError         | Unclassified internal error; always deny          |
 *
 * ## §Threading Guarantees
 *
 * - Mount/unmount operations acquire a per-container mutex; concurrent
 *   operations on the same container are serialized.
 * - Key derivation is stateless and thread-safe; concurrent calls are allowed.
 * - The rotation scheduler runs on a single dedicated thread; rotation
 *   callbacks MUST NOT re-enter the backend or scheduler.
 *
 * ## §Contract Freeze
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/user_storage_encrypted/ROADMAP.md — Phase 1 item
 */

#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace themis {
namespace user_storage_encrypted {

// ============================================================================
// § 1  Error taxonomy
// ============================================================================

/**
 * @brief Canonical error codes for the user_storage_encrypted module.
 *
 * All encrypted storage operations return or throw with one of these codes.
 * Values are in the reserved range [8600, 8699].
 */
enum class UserStorageEncryptedError : int32_t {
    kSuccess             = 0,
    kMountFailed         = 8600, ///< Encrypted container mount failed.
    kUnmountFailed       = 8601, ///< Encrypted container unmount failed.
    kKeyDerivationFailed = 8602, ///< KDF operation failed.
    kRotationFailed      = 8603, ///< Key rotation did not complete atomically.
    kBackendUnavailable  = 8604, ///< gocryptfs/FUSE backend unavailable.
    kInvalidPath         = 8605, ///< Container path fails validation.
    kPermissionDenied    = 8606, ///< Insufficient filesystem permissions.
    kInternalError       = 8607, ///< Unclassified internal error.
};

// ============================================================================
// § 2  Storage descriptor constraints
// ============================================================================

/// Maximum encrypted container path length in bytes.
inline constexpr std::size_t kMaxContainerPathBytes = 4096;

/// Minimum allowed passphrase length in bytes for KDF.
inline constexpr std::size_t kMinPassphraseBytes = 16;

/// Maximum allowed passphrase length in bytes for KDF.
inline constexpr std::size_t kMaxPassphraseBytes = 1024;

/// KDF salt length in bytes (fixed).
inline constexpr std::size_t kKdfSaltBytes = 32;

/// Default key rotation interval.
inline constexpr std::chrono::hours kDefaultRotationInterval{24 * 90}; // 90 days

/// Maximum mount operation timeout.
inline constexpr std::chrono::seconds kMaxMountTimeout{60};

// ============================================================================
// § 3  Supporting structs
// ============================================================================

/**
 * @brief Descriptor for mounting an encrypted storage container.
 */
struct EncryptedMountDescriptor {
    std::string  container_path;    ///< Path to the encrypted container directory.
    std::string  mount_point;       ///< Target mount point path.
    bool         read_only{false};  ///< Mount as read-only.
    std::chrono::seconds timeout{kMaxMountTimeout}; ///< Mount operation timeout.
};

/**
 * @brief Inputs for the key derivation service.
 *
 * The passphrase field is zeroed by the KDF after use; callers MUST NOT
 * retain a reference to the passphrase buffer.
 */
struct KeyDerivationRequest {
    std::string  passphrase;   ///< Raw passphrase (min kMinPassphraseBytes).
    std::string  salt;         ///< KDF salt (must be exactly kKdfSaltBytes).
    uint32_t     iterations{100'000}; ///< KDF iteration count.
};

// ============================================================================
// § 4  Fail-closed contract
// ============================================================================

/**
 * @brief Returns true when the given error mandates fail-closed denial of storage access.
 */
[[nodiscard]] inline constexpr bool isUserStorageEncryptedFailClosed(
        UserStorageEncryptedError e) noexcept {
    return e == UserStorageEncryptedError::kBackendUnavailable
        || e == UserStorageEncryptedError::kKeyDerivationFailed
        || e == UserStorageEncryptedError::kInternalError;
}

} // namespace user_storage_encrypted
} // namespace themis
