/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            reports_api_handler.h                              ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     49                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
