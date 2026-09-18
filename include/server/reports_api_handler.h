/**
 * @file reports_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace themis { namespace server {

class ReportsApiHandler {
public:
    ReportsApiHandler() = default;

    /**
     * @brief Generate Compliance Report.
     * @param[in] report_type Input parameter.
     * @return Return value.
     */
    nlohmann::json generateComplianceReport(const std::string& report_type);
};

}} // namespace themis::server
