/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            signing_provider.h                                 ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:09:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     46                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "security/key_provider.h"
#include "security/signing.h"

namespace themis {

/**
 * Optional interface for KeyProvider implementations that can perform
 * signing operations on behalf of the caller (e.g., HSM, KMS).
 *
 * Implementations should avoid exporting raw private key material and
 * instead return a `SigningResult` for the provided data.
 */
class SigningProvider : public virtual KeyProvider {
public:
    virtual ~SigningProvider() = default;

    // Sign data using the key identified by key_id. Returns a SigningResult
    // containing signature bytes and algorithm metadata.
    virtual SigningResult sign(const std::string& key_id, const std::vector<uint8_t>& data) = 0;
};

} // namespace themis
