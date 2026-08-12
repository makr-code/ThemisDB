/**
 * @file signing.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>

#include "security/key_provider.h"

// Forward-declare HSMProvider so callers don't need to include the full header.
namespace themis { namespace security { class HSMProvider; } }

namespace themis {

struct SigningResult {
    std::vector<uint8_t> signature;
    std::string algorithm; // e.g., "CMS/DETACHED+SHA256"
    std::string error;     // Empty if successful, error message otherwise
};

/** @brief Signing service component. */
class SigningService {
public:
    virtual ~SigningService() = default;
    [[nodiscard]] virtual SigningResult sign(const std::vector<uint8_t>& data, const std::string& key_id) = 0;
    [[nodiscard]] virtual bool verify(const std::vector<uint8_t>& data,
                        const std::vector<uint8_t>& signature,
                        const std::string& key_id) = 0;
};

// Factories
std::shared_ptr<SigningService> createMockSigningService();

// KeyProvider-backed signing service: expects KeyProvider::getKey(key_id)
// to return private key bytes (PEM or DER). If a certificate is present,
// store it under key_id+":cert" and it will be used for CMS verification.
std::shared_ptr<SigningService> createKeyProviderSigningService(std::shared_ptr<KeyProvider> kp);

// HSM-backed signing service for update bundle signing with hardware-secured keys.
// @param hsm                Fully initialised HSMProvider (initialize() already called).
// @param default_key_label  Key label used when key_id passed to sign()/verify() is empty.
//                           Defaults to the HSMConfig::key_label value configured on the provider.
std::shared_ptr<SigningService> createHsmSigningService(
    std::shared_ptr<security::HSMProvider> hsm,
    const std::string& default_key_label = "themis-signing-key");

} // namespace themis
