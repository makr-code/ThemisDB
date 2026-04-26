/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            import_wizard_builder.h                            ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:46:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     43                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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
 * REST API (/api/v1/import/{name}) via vanilla JavaScript.  No external CDN
 * resources are referenced, making the wizard suitable for air-gapped
 * deployments.
 *
 * Served at: GET /import/wizard
 */
std::string buildImportWizardHtml();

} // namespace server
} // namespace themis
