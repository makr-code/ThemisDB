/**
 * @file adapter_metadata.h
 * @brief Adapter metadata, validation interface, and cryptographic signing for AdapterRegistry.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 95/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Status: Production Ready
 */

#pragma once

#include <string>
#include <cstdint>

namespace themis {
namespace core {
namespace concerns {

// ---------------------------------------------------------------------------
// API version constant
// ---------------------------------------------------------------------------

/**
 * @brief Current adapter API version enforced by AdapterRegistry.
 *
 * Every adapter registered via AdapterRegistry must carry an apiVersion >=
 * this constant.  Increment this value when a breaking change is made to
 * the adapter contract.
 */
static constexpr uint32_t kCurrentApiVersion = 1;

// ---------------------------------------------------------------------------
// AdapterMetadata
// ---------------------------------------------------------------------------

/**
 * @brief Metadata bundle attached to every registered adapter.
 *
 * Carries the adapter's unique identifier, API version, and an optional
 * human-readable description.  AdapterRegistry validates the @c id and
 * @c apiVersion fields before accepting a registration request.
 *
 * Constraints enforced at registration time:
 *  - @c id must be non-empty.
 *  - @c apiVersion must be >= 1 (0 is reserved and rejected).
 */
struct AdapterMetadata {
    /// Unique adapter identifier; must be non-empty at registration time.
    std::string  id;

    /// Semantic API version; must be >= 1.  Defaults to kCurrentApiVersion.
    uint32_t     apiVersion  = kCurrentApiVersion;

    /// Optional human-readable description for diagnostics and audit logs.
    std::string  description;
};

// ---------------------------------------------------------------------------
// AdapterSignature
// ---------------------------------------------------------------------------

/**
 * @brief Cryptographic signing digest for an adapter library or metadata bundle.
 *
 * Used by @c SignedAdapterValidator to verify that an adapter has not been
 * tampered with before it is registered in @c AdapterRegistry.
 *
 * ## Canonical data conventions
 *
 * When signing a **programmatically-registered** adapter, the canonical data
 * string is constructed by @c SignedAdapterValidator::canonicalString():
 *
 * ```
 * <id> ":" <apiVersion> ":" <description>
 * ```
 *
 * When signing a **file-based plugin** loaded via
 * @c AdapterRegistry::loadFromPlugin(), the canonical data is the raw bytes
 * of the plugin library file.  Compute the expected digest at build time with:
 *
 * ```bash
 * openssl dgst -sha256 -hex libmy_adapter.so
 * ```
 *
 * ## Supported algorithms
 *
 * Currently only @c "sha256" is accepted; other values cause
 * @c SignedAdapterValidator::validate() to return @c false.
 *
 * ## Empty-signature semantics
 *
 * An @c AdapterSignature with empty @c algorithm and @c digest is considered
 * "unsigned".  @c SignedAdapterValidator rejects unsigned adapters.
 */
struct AdapterSignature {
    /// Hash algorithm identifier.  Must be @c "sha256" to pass validation.
    std::string algorithm = {};

    /// Lowercase hex-encoded SHA-256 digest (64 characters for sha256).
    std::string digest = {};

    /**
     * @brief Return true when both fields are non-empty (i.e., signature present).
     * @return true if algorithm and digest are non-empty.
     */
    [[nodiscard]] bool present() const noexcept {
        return !algorithm.empty() && !digest.empty();
    }
};

// ---------------------------------------------------------------------------
// AdapterValidator
// ---------------------------------------------------------------------------

/**
 * @brief Synchronous adapter validation interface.
 *
 * Implementations perform arbitrary checks against an AdapterMetadata bundle —
 * for example, enforcing an API version range, verifying an id-naming policy,
 * or (in the future) verifying a cryptographic signature.
 *
 * A validator that returns @c false causes
 * AdapterRegistry::registerAdapter() to throw @c std::invalid_argument.
 *
 * Thread-safety: @c validate() must be safe to call concurrently from any
 * thread.
 */
class AdapterValidator {
public:
    virtual ~AdapterValidator() = default;

    /**
     * @brief Validate the given metadata before adapter registration.
     *
     * @param m  Metadata bundle to inspect.
     * @return   @c true if the metadata is acceptable; @c false to reject.
     */
    virtual bool validate(const AdapterMetadata& m) = 0;
};

} // namespace concerns
} // namespace core
} // namespace themis
