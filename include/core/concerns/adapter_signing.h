/**
 * @file adapter_signing.h
 * @brief SHA-256 based adapter signing validator for AdapterRegistry.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Status: Production Ready
 *
 * Provides @c SignedAdapterValidator — a concrete @c AdapterValidator that
 * verifies an @c AdapterSignature against the SHA-256 digest of an adapter's
 * canonical string representation before registration is permitted.
 *
 * ## Integration with AdapterRegistry
 *
 * ```cpp
 * #include "core/concerns/adapter_registry.h"
 * #include "core/concerns/adapter_signing.h"
 *
 * AdapterSignature sig{"sha256", "<64-char hex digest>"};
 * SignedAdapterValidator validator{sig};
 *
 * registry.registerAdapter<ILogger>("my_logger", adapter, &validator);
 * // Throws std::invalid_argument if the digest does not match.
 * ```
 *
 * ## Canonical string format
 *
 * For programmatically-registered adapters the canonical input fed to SHA-256 is:
 *
 * ```
 * <id>:<apiVersion>:<description>
 * ```
 *
 * where @c description is the value in @c AdapterMetadata (may be empty).
 * To pre-compute the expected digest:
 *
 * ```bash
 * printf 'my_logger:1:' | openssl dgst -sha256 -hex
 * ```
 */

#pragma once

#include "core/concerns/adapter_metadata.h"

#include <string>
#include <string_view>

namespace themis {
namespace core {
namespace concerns {

// ---------------------------------------------------------------------------
// SignedAdapterValidator
// ---------------------------------------------------------------------------

/**
 * @brief Concrete @c AdapterValidator that enforces SHA-256 signature verification.
 *
 * Constructs the canonical string `id:apiVersion:description` from the
 * supplied @c AdapterMetadata, computes its SHA-256 hex digest, and compares
 * it against the expected digest in the @c AdapterSignature passed at
 * construction.
 *
 * Validation fails (returns @c false) if:
 *  - The signature object has an empty @c algorithm or @c digest.
 *  - The @c algorithm is not @c "sha256".
 *  - The computed SHA-256 hex digest does not match @c sig.digest (constant-time
 *    comparison is used to resist timing side-channels).
 *  - OpenSSL EVP digest computation fails internally.
 *
 * Thread-safety: @c validate() is safe to call concurrently from any thread.
 */
class SignedAdapterValidator final : public AdapterValidator {
public:
    /**
     * @brief Construct a validator for the given expected signature.
     *
     * @param expected_sig  The expected @c AdapterSignature.  Both
     *                      @c algorithm and @c digest must be non-empty for
     *                      any validation to succeed.
     */
    explicit SignedAdapterValidator(AdapterSignature expected_sig);

    /**
     * @brief Validate adapter metadata by verifying its SHA-256 digest.
     *
     * Computes @c sha256(canonicalString(m)) and compares against
     * @c expected_sig.digest using a constant-time comparison.
     *
     * @param m  Adapter metadata to validate.
     * @return   @c true if the computed digest matches the expected digest;
     *           @c false otherwise.
     */
    [[nodiscard]] bool validate(const AdapterMetadata& m) override;

    // -----------------------------------------------------------------------
    // Utilities
    // -----------------------------------------------------------------------

    /**
     * @brief Build the canonical string for SHA-256 input from metadata.
     *
     * Format: `<id>:<apiVersion>:<description>`
     *
     * @param m  Metadata to serialise.
     * @return   Canonical UTF-8 string suitable as hash input.
     */
    [[nodiscard]] static std::string canonicalString(const AdapterMetadata& m);

    /**
     * @brief Compute the lowercase hex SHA-256 digest of @p data.
     *
     * @param data  Arbitrary byte buffer.
     * @return      64-character lowercase hex string, or empty string on
     *              internal OpenSSL error.
     */
    [[nodiscard]] static std::string sha256Hex(std::string_view data);

private:
    AdapterSignature expected_sig_;
};

} // namespace concerns
} // namespace core
} // namespace themis
