/**
 * @file transport_security_checker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/logger.h"
#include <string>
#include <cstdlib>
#include <algorithm>
#include <cctype>

namespace themis {
namespace security {

/**
 * Transport Security Checker - Production Safety Utilities
 * 
 * Provides utilities for enforcing transport security requirements in production.
 * Ensures TLS/mTLS is used for Wire Protocol in production deployments.
 */
class TransportSecurityChecker {
public:
    /**
     * Check if running in production mode
     * @return true if production mode is active
     */
    static bool isProductionMode() {
        // Check environment variable (case-insensitive)
        const char* env_mode = std::getenv("THEMIS_PRODUCTION_MODE");
        if (env_mode) {
            std::string mode(env_mode);
            // Convert to lowercase for case-insensitive comparison
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
            return mode == "true" || mode == "1" || mode == "production";
        }
        
        // Check another common env var (case-insensitive)
        const char* env = std::getenv("THEMIS_ENVIRONMENT");
        if (env) {
            std::string environment(env);
            // Convert to lowercase for case-insensitive comparison
            std::transform(environment.begin(), environment.end(), environment.begin(), ::tolower);
            return environment == "production" || environment == "prod";
        }
        
        // Default to false (development mode)
        return false;
    }
    
    /**
     * Check if insecure transport override flag is present
     * @param argc: Command-line argument count
     * @param argv: Command-line arguments
     * @return true if --allow-insecure-wire-protocol flag is present
     */
    static bool hasAllowInsecureFlag(int argc, const char* const argv[]) {
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--allow-insecure-wire-protocol") {
                return true;
            }
        }
        return false;
    }
    
    /**
     * Validate Wire Protocol transport security for production
     * 
     * Enforces that TLS is enabled for Wire Protocol in production mode
     * unless explicitly overridden with --allow-insecure-wire-protocol flag.
     * 
     * @param enable_tls: Whether TLS is enabled
     * @param protocol_name: Name of the protocol (for logging)
     * @param argc: Command-line argument count
     * @param argv: Command-line arguments
     * @return true if configuration is safe, false if should exit
     */
    static bool validateProductionSafety(bool enable_tls, const std::string& protocol_name, int argc, const char* const argv[]) {
        // Not production mode - allow insecure transport for development
        if (!isProductionMode()) {
            if (!enable_tls) {
                THEMIS_WARN("{}: TLS is disabled (development mode)", protocol_name);
            }
            return true;
        }
        
        // Production mode - TLS must be enabled
        if (enable_tls) {
            THEMIS_INFO("{}: TLS enabled (production safe)", protocol_name);
            return true;
        }
        
        // TLS disabled in production - check for override flag
        if (hasAllowInsecureFlag(argc, argv)) {
            // Override flag present - log critical warning but allow
            THEMIS_CRITICAL("=================================================================");
            THEMIS_CRITICAL("*** PRODUCTION SECURITY OVERRIDE ACTIVE ***");
            THEMIS_CRITICAL("=================================================================");
            THEMIS_CRITICAL("--allow-insecure-wire-protocol flag used in PRODUCTION!");
            THEMIS_CRITICAL("{} transport security is DISABLED.", protocol_name);
            THEMIS_CRITICAL("Network traffic is UNENCRYPTED and vulnerable to:");
            THEMIS_CRITICAL("- Man-in-the-middle attacks");
            THEMIS_CRITICAL("- Eavesdropping and data theft");
            THEMIS_CRITICAL("- Packet sniffing and traffic analysis");
            THEMIS_CRITICAL("");
            THEMIS_CRITICAL("COMPLIANCE VIOLATIONS:");
            THEMIS_CRITICAL("[VIOLATION] NIST SP 800-52 (TLS Guidelines)");
            THEMIS_CRITICAL("[VIOLATION] ISO 27001 A.13.1 (Network Security)");
            THEMIS_CRITICAL("[VIOLATION] PCI DSS Requirement 4 (Encrypt Transmission)");
            THEMIS_CRITICAL("[VIOLATION] GDPR Article 32 (Security of Processing)");
            THEMIS_CRITICAL("");
            THEMIS_CRITICAL("IMMEDIATE ACTION REQUIRED:");
            THEMIS_CRITICAL("- Enable TLS in configuration");
            THEMIS_CRITICAL("- Configure server certificates");
            THEMIS_CRITICAL("- Remove --allow-insecure-wire-protocol flag");
            THEMIS_CRITICAL("- Review docs/de/guides/guides_tls_setup.md");
            THEMIS_CRITICAL("=================================================================");
            return true;
        }
        
        // TLS disabled in production without override - FAIL
        THEMIS_CRITICAL("=================================================================");
        THEMIS_CRITICAL("*** CRITICAL SECURITY FAILURE ***");
        THEMIS_CRITICAL("=================================================================");
        THEMIS_CRITICAL("Insecure {} transport is NOT ALLOWED in production!", protocol_name);
        THEMIS_CRITICAL("");
        THEMIS_CRITICAL("SECURITY RISK:");
        THEMIS_CRITICAL("Network traffic is transmitted in PLAINTEXT without");
        THEMIS_CRITICAL("encryption. This exposes sensitive data to interception.");
        THEMIS_CRITICAL("");
        THEMIS_CRITICAL("COMPLIANCE VIOLATIONS:");
        THEMIS_CRITICAL("[X] NIST SP 800-52 (TLS Guidelines)");
        THEMIS_CRITICAL("[X] ISO 27001 A.13.1 (Network Security)");
        THEMIS_CRITICAL("[X] PCI DSS Requirement 4 (Encrypt Transmission)");
        THEMIS_CRITICAL("[X] GDPR Article 32 (Security of Processing)");
        THEMIS_CRITICAL("[X] HIPAA 164.312(e)(1) (Transmission Security)");
        THEMIS_CRITICAL("");
        THEMIS_CRITICAL("REQUIRED ACTIONS:");
        THEMIS_CRITICAL("1. Enable TLS in Wire Protocol configuration");
        THEMIS_CRITICAL("2. Set enable_tls=true in config file");
        THEMIS_CRITICAL("3. Configure tls_cert_path and tls_key_path");
        THEMIS_CRITICAL("4. Set THEMIS_PRODUCTION_MODE=false for development");
        THEMIS_CRITICAL("5. Use --allow-insecure-wire-protocol to override");
        THEMIS_CRITICAL("   (NOT RECOMMENDED - security compliance violation!)");
        THEMIS_CRITICAL("");
        THEMIS_CRITICAL("TLS Configuration Example:");
        THEMIS_CRITICAL("wire_protocol:");
        THEMIS_CRITICAL("  enable_tls: true");
        THEMIS_CRITICAL("  tls_cert_path: /path/to/server.crt");
        THEMIS_CRITICAL("  tls_key_path: /path/to/server.key");
        THEMIS_CRITICAL("  tls_ca_cert_path: /path/to/ca.crt  # For mTLS");
        THEMIS_CRITICAL("  tls_require_client_cert: true      # Enable mTLS");
        THEMIS_CRITICAL("");
        THEMIS_CRITICAL("Documentation: docs/de/guides/guides_tls_setup.md");
        THEMIS_CRITICAL("=================================================================");
        
        return false;  // Signal that server should exit
    }
    
    /**
     * Get warning message for periodic security checks
     * @param enable_tls: Whether TLS is enabled
     * @param protocol_name: Name of the protocol
     * @return Warning message or empty string if no warning needed
     */
    static std::string getPeriodicWarning(bool enable_tls, const std::string& protocol_name) {
        if (!isProductionMode()) {
            return "";  // No warning in development
        }
        
        if (enable_tls) {
            return "";  // No warning when TLS is active
        }
        
        return "PRODUCTION SECURITY ALERT: " + protocol_name + " TLS disabled! "
               "Network traffic is UNENCRYPTED. Security compliance violation. "
               "See docs/de/guides/guides_tls_setup.md";
    }
};

} // namespace security
} // namespace themis
