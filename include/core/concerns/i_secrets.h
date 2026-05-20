/*
 * ThemisDB | File: i_secrets.h | Version: 0.0.15 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 107
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #2693 [core] Secrets interface for credential injection into components (2026-03-12T05:55:27Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "core/concerns/lifecycle.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Abstract interface for credential injection into components.
 *
 * ISecrets provides a unified, pluggable interface for retrieving named
 * secrets (API keys, database passwords, service tokens, etc.) so that
 * components receive credentials via dependency injection rather than
 * reading them directly from environment variables or configuration files.
 *
 * Implementations may be backed by:
 *   - An in-memory map (for testing)
 *   - Environment variables (EnvSecretsProvider)
 *   - A secrets manager such as HashiCorp Vault or AWS Secrets Manager
 *   - The security::SecretManager (versioned, rotation-aware store)
 *
 * Thread-safety: all public methods must be thread-safe.
 */
class ISecrets {
public:
    virtual ~ISecrets() = default;

    // -----------------------------------------------------------------------
    // Core access methods
    // -----------------------------------------------------------------------

    /**
     * @brief Retrieve the current value of a named secret.
     *
     * Returns the active secret value, or std::nullopt when the secret
     * is unknown, revoked, or the underlying store is unavailable.
     *
     * @param name  Secret name (e.g. "db.password", "api.key.stripe").
     * @return Active secret value, or std::nullopt if not found.
     */
    [[nodiscard]] virtual std::optional<std::string> getSecret(std::string_view name) const = 0;

    /**
     * @brief Check whether a named secret exists and is available.
     *
     * @param name  Secret name.
     * @return true when getSecret(name) would return a non-empty value.
     */
    [[nodiscard]] virtual bool hasSecret(std::string_view name) const = 0;

    /**
     * @brief Return the names of all secrets available through this provider.
     *
     * The returned list contains only names accessible via getSecret(); names
     * of revoked or otherwise inaccessible secrets are excluded.
     *
     * Implementations that cannot enumerate secrets (e.g. remote vaults with
     * no list permission) may return an empty vector.
     *
     * @return Sorted list of available secret names.
     */
    [[nodiscard]] virtual std::vector<std::string> listSecretNames() const = 0;

    // -----------------------------------------------------------------------
    // Lifecycle hooks
    // -----------------------------------------------------------------------

    /**
     * @brief Flush any pending writes or cache invalidations.
     *
     * For read-only providers this is a no-op.  For caching wrappers it
     * should ensure the cache is consistent with the backing store.
     */
    virtual void flush() noexcept {}

    /**
     * @brief Shut down the provider and release resources.
     *
     * After shutdown(), all getSecret() calls silently return std::nullopt.
     */
    virtual void shutdown() noexcept {}

    /**
     * @brief Probe whether the secrets backend is reachable and healthy.
     *
     * @return ProbeResult with ok=true when the backend is operational,
     *         ok=false with a descriptive message otherwise.
     */
    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

} // namespace concerns
} // namespace core
} // namespace themis
