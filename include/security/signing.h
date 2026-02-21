/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            signing.h                                          ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     60                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
