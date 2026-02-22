/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            reports_api_handler.h                              ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     53                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
