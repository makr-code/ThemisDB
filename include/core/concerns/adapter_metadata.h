/**
 * @file adapter_metadata.h
 * @brief Adapter metadata, validation interface, and signing stub for the AdapterRegistry.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 92/100
 * @note Gap Summary: total=1; TODO=0, Stub=1, Unimpl=0, Mock=0, Sim=0, Debt=0
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
// AdapterSignature — STUB/SIMULATION NOTE
//
// Purpose:    Placeholder for future adapter signing/trust validation
//             (Target: Q4 2026).
// Activation: NOT active — AdapterRegistry ignores the signature field
//             until the signing workflow ships.
// Production Delta: Signatures are not verified; any adapter passes the
//             trust check regardless of this field's contents.
// Removal Plan: Wire into AdapterValidator::validate() when the signing
//             pipeline is ready (Issue #1706 hardening block, Q4 2026).
// ---------------------------------------------------------------------------

/**
 * @brief Adapter cryptographic signing digest.
 *
 * @note STUB — not yet enforced.  See the STUB/SIMULATION NOTE above.
 *       Both fields are intentionally ignored by AdapterRegistry until
 *       the Q4 2026 signed-adapter hardening milestone is implemented.
 */
struct AdapterSignature {
    /// Hash algorithm used to produce @c digest (e.g. "sha256").
    std::string algorithm;

    /// Hex-encoded digest of the adapter's binary or metadata bundle.
    std::string digest;
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
