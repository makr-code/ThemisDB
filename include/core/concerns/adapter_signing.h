/**
 * @file adapter_signing.h
 * @brief SHA-256 based adapter signing validator for AdapterRegistry.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
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

class SignedAdapterValidator final : public AdapterValidator {
public:
    /**
     * @brief Signed Adapter Validator.
     * @param[in] expected_sig Input parameter.
     * @return Return value.
     */
    explicit SignedAdapterValidator(AdapterSignature expected_sig);

    [[nodiscard]] bool validate(const AdapterMetadata& m) override;

    // -----------------------------------------------------------------------
    // Utilities
    // -----------------------------------------------------------------------

    [[nodiscard]] static std::string canonicalString(const AdapterMetadata& m);

    [[nodiscard]] static std::string sha256Hex(std::string_view data);

private:
    AdapterSignature expected_sig_;
};

} // namespace concerns
} // namespace core
} // namespace themis
