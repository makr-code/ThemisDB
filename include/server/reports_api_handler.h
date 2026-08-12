/**
 * @file reports_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace themis { namespace server {

/**
 * @brief Reports Admin API Handler (Skeleton)
 * 
 * Provides REST endpoints for compliance reports:
 * - GET /api/reports/compliance - Generate compliance overview
 */
class ReportsApiHandler {
public:
    ReportsApiHandler() = default;

    /**
     * @brief Generate compliance report
     * @param report_type Type of report (e.g., "dsgvo", "sox", "overview")
     * @return JSON response: { "report_type": "...", "generated_at": "...", "metrics": {...} }
     */
    nlohmann::json generateComplianceReport(const std::string& report_type);
};

}} // namespace themis::server
