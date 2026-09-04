/**
 * @file hsm_security_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=10; TODO=1, Stub=8, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "security/hsm_provider.h"
#include <string>
#include <sstream>

namespace themis {
namespace security {

/**
 * HSM Security Metrics Exporter
 * 
 * Exports HSM security status as Prometheus-compatible metrics.
 * Addresses FIND-002 monitoring requirements.
 */
class HSMSecurityMetrics {
public:
    /**
     * Export HSM security metrics in Prometheus text format
     * 
     * Metrics exported:
     * - hsm_security_stub_active: Gauge (0 = secure, 1 = stub active)
     * - hsm_security_warnings_total: Counter (cumulative warnings)
     * - hsm_provider_type: Info metric (provider type label)
     * - hsm_provider_ready: Gauge (0 = not ready, 1 = ready)
     * 
     * @param hsm: HSM provider instance
     * @param warnings_count: Total number of security warnings issued
     * @return Prometheus-formatted metrics string
     */
    static std::string exportMetrics(const HSMProvider& hsm, uint64_t warnings_count = 0) {
        std::ostringstream oss = {};
        
        // HSM security stub active gauge
        oss << "# HELP hsm_security_stub_active Indicates if HSM stub provider is active (0=secure, 1=stub)\n";
        oss << "# TYPE hsm_security_stub_active gauge\n";
        oss << "hsm_security_stub_active " << (hsm.isStubProvider() ? "1" : "0") << "\n";
        oss << "\n";
        
        // HSM insecure configuration metric (alias for compatibility)
        oss << "# HELP themis_hsm_insecure_config Indicates if HSM is in insecure configuration (0=secure, 1=insecure)\n";
        oss << "# TYPE themis_hsm_insecure_config gauge\n";
        oss << "themis_hsm_insecure_config " << (hsm.isStubProvider() ? "1" : "0") << "\n";
        oss << "\n";
        
        // HSM security warnings counter
        oss << "# HELP hsm_security_warnings_total Total number of HSM security warnings issued\n";
        oss << "# TYPE hsm_security_warnings_total counter\n";
        oss << "hsm_security_warnings_total " << warnings_count << "\n";
        oss << "\n";
        
        // HSM provider type info metric
        oss << "# HELP themis_hsm_provider_type HSM provider type information\n";
        oss << "# TYPE themis_hsm_provider_type gauge\n";
        std::string provider_type = hsm.isStubProvider() ? "stub" : "real";
        oss << "themis_hsm_provider_type{provider=\"" << provider_type << "\"} 1\n";
        oss << "\n";
        
        // Also export the original metric name for backward compatibility
        oss << "# HELP hsm_provider_type HSM provider type information (deprecated, use themis_hsm_provider_type)\n";
        oss << "# TYPE hsm_provider_type gauge\n";
        oss << "hsm_provider_type{provider=\"" << provider_type << "\"} 1\n";
        oss << "\n";
        
        // HSM provider ready status
        oss << "# HELP hsm_provider_ready HSM provider readiness status (0=not ready, 1=ready)\n";
        oss << "# TYPE hsm_provider_ready gauge\n";
        oss << "hsm_provider_ready " << (hsm.isReady() ? "1" : "0") << "\n";
        oss << "\n";
        
        // HSM performance stats (if available)
        auto stats = hsm.getStats();
        if (stats.sign_count > 0 || stats.verify_count > 0) {
            oss << "# HELP hsm_sign_operations_total Total HSM sign operations\n";
            oss << "# TYPE hsm_sign_operations_total counter\n";
            oss << "hsm_sign_operations_total " << stats.sign_count << "\n";
            oss << "\n";
            
            oss << "# HELP hsm_verify_operations_total Total HSM verify operations\n";
            oss << "# TYPE hsm_verify_operations_total counter\n";
            oss << "hsm_verify_operations_total " << stats.verify_count << "\n";
            oss << "\n";
            
            oss << "# HELP hsm_sign_errors_total Total HSM sign operation errors\n";
            oss << "# TYPE hsm_sign_errors_total counter\n";
            oss << "hsm_sign_errors_total " << stats.sign_errors << "\n";
            oss << "\n";
            
            oss << "# HELP hsm_verify_errors_total Total HSM verify operation errors\n";
            oss << "# TYPE hsm_verify_errors_total counter\n";
            oss << "hsm_verify_errors_total " << stats.verify_errors << "\n";
            oss << "\n";
            
            if (stats.sign_count > 0) {
                double avg_sign_time = static_cast<double>(stats.total_sign_time_us) / stats.sign_count;
                oss << "# HELP hsm_sign_duration_microseconds_avg Average HSM sign operation duration\n";
                oss << "# TYPE hsm_sign_duration_microseconds_avg gauge\n";
                oss << "hsm_sign_duration_microseconds_avg " << avg_sign_time << "\n";
                oss << "\n";
            }
            
            if (stats.verify_count > 0) {
                double avg_verify_time = static_cast<double>(stats.total_verify_time_us) / stats.verify_count;
                oss << "# HELP hsm_verify_duration_microseconds_avg Average HSM verify operation duration\n";
                oss << "# TYPE hsm_verify_duration_microseconds_avg gauge\n";
                oss << "hsm_verify_duration_microseconds_avg " << avg_verify_time << "\n";
                oss << "\n";
            }
        }
        
        // Compliance status metric (derived from stub status)
        oss << "# HELP hsm_compliance_status HSM compliance status (0=non-compliant, 1=compliant)\n";
        oss << "# TYPE hsm_compliance_status gauge\n";
        // Compliant if using real HSM
        oss << "hsm_compliance_status{standard=\"nist_sp_800_53_sc_12\"} " 
            << (hsm.isStubProvider() ? "0" : "1") << "\n";
        oss << "hsm_compliance_status{standard=\"iso_27001_a_8_24\"} " 
            << (hsm.isStubProvider() ? "0" : "1") << "\n";
        oss << "hsm_compliance_status{standard=\"pci_dss_3_6\"} " 
            << (hsm.isStubProvider() ? "0" : "1") << "\n";
        oss << "hsm_compliance_status{standard=\"gdpr_art_32\"} " 
            << (hsm.isStubProvider() ? "0" : "1") << "\n";
        oss << "\n";
        
        return oss.str();
    }
    
    /**
     * Get JSON-formatted metrics summary
     * Useful for REST API endpoints
     */
    static std::string exportJSON(const HSMProvider& hsm, uint64_t warnings_count = 0) {
        std::ostringstream oss = {};
        auto stats = hsm.getStats();
        
        oss << "{\n";
        oss << "  \"hsm_security\": {\n";
        oss << "    \"stub_active\": " << (hsm.isStubProvider() ? "true" : "false") << ",\n";
        oss << "    \"provider_type\": \"" << (hsm.isStubProvider() ? "stub" : "real") << "\",\n";
        oss << "    \"ready\": " << (hsm.isReady() ? "true" : "false") << ",\n";
        oss << "    \"warnings_total\": " << warnings_count << ",\n";
        oss << "    \"token_info\": \"" << hsm.getTokenInfo() << "\"\n";
        oss << "  },\n";
        oss << "  \"compliance\": {\n";
        oss << "    \"nist_sp_800_53_sc_12\": " << (hsm.isStubProvider() ? "false" : "true") << ",\n";
        oss << "    \"iso_27001_a_8_24\": " << (hsm.isStubProvider() ? "false" : "true") << ",\n";
        oss << "    \"pci_dss_3_6\": " << (hsm.isStubProvider() ? "false" : "true") << ",\n";
        oss << "    \"gdpr_art_32\": " << (hsm.isStubProvider() ? "false" : "true") << "\n";
        oss << "  },\n";
        oss << "  \"operations\": {\n";
        oss << "    \"sign_count\": " << stats.sign_count << ",\n";
        oss << "    \"verify_count\": " << stats.verify_count << ",\n";
        oss << "    \"sign_errors\": " << stats.sign_errors << ",\n";
        oss << "    \"verify_errors\": " << stats.verify_errors << "\n";
        oss << "  }\n";
        oss << "}\n";
        
        return oss.str();
    }
};

} // namespace security
} // namespace themis
