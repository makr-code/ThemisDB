/**
 * @file hsm_security_checker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=16; TODO=1, Stub=14, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "security/hsm_provider.h"
#include "utils/logger.h"
#include <string>
#include <cstdlib>

namespace themis {
namespace security {

/**
 * HSM Security Checker - Production Safety Utilities
 * 
 * Provides utilities for enforcing HSM security requirements in production.
 * Addresses FIND-002 security audit findings.
 */
class HSMSecurityChecker {
public:
    /**
     * Check if running in production mode
     * @return true if production mode is active
     */
    static bool isProductionMode() {
        // Check environment variable
        const char* env_mode = std::getenv("THEMIS_PRODUCTION_MODE");
        if (env_mode) {
            std::string mode(env_mode);
            return mode == "true" || mode == "1" || mode == "production";
        }
        
        // Check another common env var
        const char* env = std::getenv("THEMIS_ENVIRONMENT");
        if (env) {
            std::string environment(env);
            return environment == "production" || environment == "prod";
        }
        
        // Default to false (development mode)
        return false;
    }
    
    /**
     * Check if stub HSM override flag is present
     * @param argc: Command-line argument count
     * @param argv: Command-line arguments
     * @return true if --allow-stub-hsm flag is present
     */
    static bool hasAllowStubFlag(int argc, char* argv[]) {
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--allow-stub-hsm") {
                return true;
            }
        }
        return false;
    }
    
    /**
     * Validate HSM configuration for production
     * 
     * Enforces that stub HSM provider is not used in production mode
     * unless explicitly overridden with --allow-stub-hsm flag.
     * 
     * @param hsm: HSM provider instance
     * @param argc: Command-line argument count
     * @param argv: Command-line arguments
     * @return true if configuration is safe, false if should exit
     */
    static bool validateProductionSafety(const HSMProvider& hsm, int argc, char* argv[]) {
        // Not production mode - allow everything
        if (!isProductionMode()) {
            return true;
        }
        
        // Check if stub provider is active
        if (!hsm.isStubProvider()) {
            // Real HSM active - safe for production
            return true;
        }
        
        // Stub provider in production - check for override flag
        if (hasAllowStubFlag(argc, argv)) {
            // Override flag present - log warning but allow
            THEMIS_WARN("╔═══════════════════════════════════════════════════════════════╗");
            THEMIS_WARN("║  ⚠️  PRODUCTION SAFETY OVERRIDE ACTIVE  ⚠️                    ║");
            THEMIS_WARN("╠═══════════════════════════════════════════════════════════════╣");
            THEMIS_WARN("║  --allow-stub-hsm flag used in PRODUCTION mode!              ║");
            THEMIS_WARN("║  HSM security is DISABLED.                                   ║");
            THEMIS_WARN("║  This violates security compliance requirements.             ║");
            THEMIS_WARN("║                                                               ║");
            THEMIS_WARN("║  IMMEDIATE ACTION REQUIRED:                                  ║");
            THEMIS_WARN("║  - Configure real HSM provider                               ║");
            THEMIS_WARN("║  - Remove --allow-stub-hsm flag                              ║");
            THEMIS_WARN("║  - Review docs/security/HSM_PRODUCTION_SETUP.md              ║");
            THEMIS_WARN("╚═══════════════════════════════════════════════════════════════╝");
            return true;
        }
        
        // Stub provider in production without override - FAIL
        THEMIS_CRITICAL("╔═══════════════════════════════════════════════════════════════╗");
        THEMIS_CRITICAL("║  🛑  CRITICAL SECURITY FAILURE  🛑                            ║");
        THEMIS_CRITICAL("╠═══════════════════════════════════════════════════════════════╣");
        THEMIS_CRITICAL("║  HSM stub provider is NOT ALLOWED in production mode!        ║");
        THEMIS_CRITICAL("║                                                               ║");
        THEMIS_CRITICAL("║  COMPLIANCE VIOLATIONS:                                      ║");
        THEMIS_CRITICAL("║  ❌ NIST SP 800-53 SC-12 (Key Management)                    ║");
        THEMIS_CRITICAL("║  ❌ ISO 27001 A.8.24 (Cryptography)                          ║");
        THEMIS_CRITICAL("║  ❌ PCI DSS Requirement 3.6 (Key Protection)                 ║");
        THEMIS_CRITICAL("║  ❌ GDPR Article 32 (Security of Processing)                 ║");
        THEMIS_CRITICAL("║                                                               ║");
        THEMIS_CRITICAL("║  REQUIRED ACTIONS:                                           ║");
        THEMIS_CRITICAL("║  1. Configure real HSM provider (see below)                  ║");
        THEMIS_CRITICAL("║  2. Set THEMIS_PRODUCTION_MODE=false for development         ║");
        THEMIS_CRITICAL("║  3. Use --allow-stub-hsm to override (NOT RECOMMENDED!)      ║");
        THEMIS_CRITICAL("║                                                               ║");
        THEMIS_CRITICAL("║  HSM Configuration Options:                                  ║");
        THEMIS_CRITICAL("║  - PKCS#11 HSM: Build with -DTHEMIS_ENABLE_HSM_REAL=ON      ║");
        THEMIS_CRITICAL("║  - AWS KMS: Configure in config/security.yaml                ║");
        THEMIS_CRITICAL("║  - Azure Key Vault: Configure in config/security.yaml        ║");
        THEMIS_CRITICAL("║  - GCP Cloud KMS: Configure in config/security.yaml          ║");
        THEMIS_CRITICAL("║  - HashiCorp Vault: Configure in config/security.yaml        ║");
        THEMIS_CRITICAL("║                                                               ║");
        THEMIS_CRITICAL("║  Documentation: docs/security/HSM_PRODUCTION_SETUP.md        ║");
        THEMIS_CRITICAL("╚═══════════════════════════════════════════════════════════════╝");
        
        return false;  // Signal that server should exit
    }
    
    /**
     * Get warning message for periodic security checks
     * @param hsm: HSM provider instance
     * @return Warning message or empty string if no warning needed
     */
    static std::string getPeriodicWarning(const HSMProvider& hsm) {
        if (!isProductionMode()) {
            return "";  // No warning in development
        }
        
        if (!hsm.isStubProvider()) {
            return "";  // No warning when real HSM is active
        }
        
        return "⚠️  PRODUCTION SECURITY ALERT: HSM stub provider active! "
               "Master keys are NOT hardware-protected. "
               "Compliance violation. See docs/security/HSM_PRODUCTION_SETUP.md";
    }
};

} // namespace security
} // namespace themis
