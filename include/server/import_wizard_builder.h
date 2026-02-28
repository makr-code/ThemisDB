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
