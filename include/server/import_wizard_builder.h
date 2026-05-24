/*
 * ThemisDB | File: import_wizard_builder.h | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
