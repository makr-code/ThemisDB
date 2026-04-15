/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cms_signing.h                                      ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:13:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     50                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
