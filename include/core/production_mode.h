/**
 * @file production_mode.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <cstdlib>
#include <stdexcept>

namespace themis {
namespace core {

/**
 * @brief Production mode detection and enforcement utilities.
 *
 * The helpers centralize the fail-closed policy used by security-sensitive
 * builders and validators. Callers should not duplicate environment parsing
 * rules outside this class.
 */
class ProductionMode {
public:
    /**
     * @brief Check if production mode is enabled
     * 
     * Production mode is enabled when:
     * - THEMIS_PRODUCTION_MODE=1 (or true, yes, on)
     * - OR THEMIS_ENVIRONMENT=production
     * 
    * Invalid or unset environment values are treated as development mode.
    *
    * @return true if production mode is enabled, false otherwise.
     */
    static bool isEnabled() {
        const char* prod_mode = std::getenv("THEMIS_PRODUCTION_MODE");
        const char* environment = std::getenv("THEMIS_ENVIRONMENT");
        
        // Check THEMIS_PRODUCTION_MODE
        if (prod_mode) {
            std::string mode_str(prod_mode);
            if (mode_str == "1" || mode_str == "true" || 
                mode_str == "True" || mode_str == "TRUE" ||
                mode_str == "yes" || mode_str == "Yes" ||
                mode_str == "on" || mode_str == "On") {
                return true;
            }
        }
        
        // Check THEMIS_ENVIRONMENT
        if (environment) {
            std::string env_str(environment);
            if (env_str == "production" || env_str == "prod") {
                return true;
            }
        }
        
        return false;
    }
    
    /**
     * @brief Enforce production mode requirement
     * 
     * When production mode is active and @p condition is false, this function
     * throws to prevent a permissive fallback path from reaching runtime.
     *
     * @param condition The security condition that must be met.
     * @param error_message Error message if condition fails in production.
     * @throws std::runtime_error if production mode is enabled and condition
     *         is false.
     */
    static void enforce(bool condition, const std::string& error_message) {
        if (isEnabled() && !condition) {
            throw std::runtime_error("Production mode violation: " + error_message);
        }
    }
    
    /**
     * @brief Get the current mode name for logging
     *
     * @return "production" or "development".
     */
    static std::string modeName() {
        return isEnabled() ? "production" : "development";
    }
};

} // namespace core
} // namespace themis
