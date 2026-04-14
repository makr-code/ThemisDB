/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            import_wizard_builder.h                            ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-14 11:28:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     44                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • d88671344d  2026-02-28  feat(importers): implement web-based import wizard at GET... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>

namespace themis {
namespace server {

/**
 * @brief Build the self-contained HTML for the web-based import wizard.
 *
 * Returns a complete, dependency-free HTML page that drives the import
 * REST API (/api/v1/import/*) via vanilla JavaScript.  No external CDN
 * resources are referenced, making the wizard suitable for air-gapped
 * deployments.
 *
 * Served at: GET /import/wizard
 */
std::string buildImportWizardHtml();

} // namespace server
} // namespace themis
