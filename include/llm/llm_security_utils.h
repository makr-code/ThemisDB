/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_security_utils.h                               ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 05:35:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     48                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • d0fa9e6093  2026-02-28  feat(llm): implement prompt injection mitigation and secu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>

namespace themis {
namespace llm {

/**
 * @brief Sanitize (mask) an LLM API key for safe logging and display.
 *
 * Replaces the middle portion of the key with asterisks so that the raw
 * credential never appears in log output or error messages.  At most 4
 * characters at the start and 4 at the end are shown; keys shorter than
 * 9 characters are fully masked.
 *
 * Example: "sk-abcdefghij1234567890xyz" → "sk-a***...***0xyz"
 *
 * @param api_key  The raw API key string (may be empty).
 * @return         A masked representation safe for logging.
 */
std::string sanitizeApiKey(const std::string& api_key);

} // namespace llm
} // namespace themis
