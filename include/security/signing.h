/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            signing.h                                          ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:34:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     53                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 18694b5f6  2026-01-12  Replace Security Stubs with Production Implementations (#... ║
    • a5d8e6795  2025-11-12  feat(pki): KeyProvider-backed signer + test; extend facto... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

#include "security/key_provider.h"

namespace themis {

struct SigningResult {
    std::vector<uint8_t> signature;
    std::string algorithm; // e.g., "CMS/DETACHED+SHA256"
    std::string error;     // Empty if successful, error message otherwise
};

class SigningService {
public:
    virtual ~SigningService() = default;
    virtual SigningResult sign(const std::vector<uint8_t>& data, const std::string& key_id) = 0;
    virtual bool verify(const std::vector<uint8_t>& data,
                        const std::vector<uint8_t>& signature,
                        const std::string& key_id) = 0;
};

// Factories
std::shared_ptr<SigningService> createMockSigningService();

// KeyProvider-backed signing service: expects KeyProvider::getKey(key_id)
// to return private key bytes (PEM or DER). If a certificate is present,
// store it under key_id+":cert" and it will be used for CMS verification.
std::shared_ptr<SigningService> createKeyProviderSigningService(std::shared_ptr<KeyProvider> kp);

} // namespace themis
