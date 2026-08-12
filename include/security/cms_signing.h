/**
 * @file cms_signing.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
