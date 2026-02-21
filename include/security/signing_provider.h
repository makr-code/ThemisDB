/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            signing_provider.h                                 ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:34:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     43                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • cd30d9ee9  2025-11-16  Stabilize WSL tests: Vault helper, policy override, index... ║
    • 5c4fe4a0e  2025-11-12  feat(pki): add SigningProvider (HSM-friendly), use it in ... ║
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
