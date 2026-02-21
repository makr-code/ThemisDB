/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cms_signing.h                                      ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:37:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     53                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • cd30d9ee9  2025-11-16  Stabilize WSL tests: Vault helper, policy override, index... ║
    • b6ac380c0  2025-11-12  feat(pki): add CMS-based CMSSigningService + tests ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "security/signing.h"
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <memory>

namespace themis {

class CMSSigningService : public SigningService {
public:
    // Construct from existing X509 cert and private key (shared ownership)
    CMSSigningService(std::shared_ptr<X509> cert, std::shared_ptr<EVP_PKEY> pkey);
    // Construct by taking ownership of raw OpenSSL pointers (cert/pkey may be nullptr)
    CMSSigningService(X509* cert, EVP_PKEY* pkey);
    ~CMSSigningService() override;

    SigningResult sign(const std::vector<uint8_t>& data, const std::string& key_id) override;
    bool verify(const std::vector<uint8_t>& data,
                const std::vector<uint8_t>& signature,
                const std::string& key_id) override;

private:
    std::shared_ptr<X509> cert_;
    std::shared_ptr<EVP_PKEY> pkey_;
};

} // namespace themis
