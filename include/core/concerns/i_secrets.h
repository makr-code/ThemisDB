/**
 * @file i_secrets.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class ISecrets {
public:
    /**
     * @brief ISecrets.
     * @return Return value.
     */
    virtual ~ISecrets() = default;

    // -----------------------------------------------------------------------
    // Core access methods
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual std::optional<std::string> getSecret(std::string_view name) const = 0;

    [[nodiscard]] virtual bool hasSecret(std::string_view name) const = 0;

    [[nodiscard]] virtual std::vector<std::string> listSecretNames() const = 0;

    // -----------------------------------------------------------------------
    // Lifecycle hooks
    // -----------------------------------------------------------------------

    virtual void flush() noexcept {}

    virtual void shutdown() noexcept {}

    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

} // namespace concerns
} // namespace core
} // namespace themis
