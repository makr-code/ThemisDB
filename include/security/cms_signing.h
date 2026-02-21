/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cms_signing.h                                      ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     54                                             ║
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
