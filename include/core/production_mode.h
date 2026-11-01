/**
 * @file production_mode.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace themis {
namespace core {

class ProductionMode {
public:
    /**
     * @brief Reports whether production safeguards must be enforced.
     * @return True if either canonical production variable enables production; false if both
     *         are unset or contain recognized non-production values.
     * @throws std::runtime_error if a configured value is not recognized.
     * @note THEMIS_PRODUCTION_MODE accepts 1, true, yes, on, production, 0, false, no, or off.
     *       THEMIS_ENVIRONMENT accepts production/prod or development/dev/test/testing/
     *       staging/stage/ci/local. Matching is case-insensitive.
     */
    [[nodiscard]] static bool isEnabled() {
        const char* prod_mode = std::getenv("THEMIS_PRODUCTION_MODE");
        const char* environment = std::getenv("THEMIS_ENVIRONMENT");

        const auto normalize = [](const char* value) {
            std::string normalized(value);
            std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                           [](unsigned char character) {
                               return static_cast<char>(std::tolower(character));
                           });
            return normalized;
        };

        bool enabled = false;
        if (prod_mode) {
            const std::string mode = normalize(prod_mode);
            if (mode == "1" || mode == "true" || mode == "yes" ||
                mode == "on" || mode == "production") {
                enabled = true;
            } else if (mode != "0" && mode != "false" &&
                       mode != "no" && mode != "off") {
                throw std::runtime_error(
                    "Unrecognized THEMIS_PRODUCTION_MODE value; use a documented true or false value");
            }
        }

        if (environment) {
            const std::string env = normalize(environment);
            if (env == "production" || env == "prod") {
                enabled = true;
            } else if (env != "development" && env != "dev" &&
                       env != "test" && env != "testing" &&
                       env != "staging" && env != "stage" &&
                       env != "ci" && env != "local") {
                throw std::runtime_error(
                    "Unrecognized THEMIS_ENVIRONMENT value; use a documented environment name");
            }
        }

        return enabled;
    }
    
    /**
     * @brief Enforces a safety condition when production mode is active.
     * @param condition Whether the requested operation satisfies its safety requirement.
     * @param error_message Diagnostic to include when the condition is false.
     * @throws std::runtime_error if production mode is invalid or the condition is false in production.
     */
    static void enforce(bool condition, const std::string& error_message) {
        if (isEnabled() && !condition) {
            throw std::runtime_error("Production mode violation: " + error_message);
        }
    }
    
    /**
     * @brief Returns the effective environment mode.
     * @return "production" when production safeguards are enabled, otherwise "development".
     * @throws std::runtime_error if a configured value is not recognized.
     */
    static std::string modeName() {
        return isEnabled() ? "production" : "development";
    }
};

} // namespace core
} // namespace themis
