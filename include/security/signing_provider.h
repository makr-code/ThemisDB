/*
 * ThemisDB | File: signing_provider.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    [[nodiscard]] virtual SigningResult sign(const std::string& key_id, const std::vector<uint8_t>& data) = 0;
};

} // namespace themis
