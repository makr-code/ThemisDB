/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hsm_signing.cpp                                    ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-04-15 18:10:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     123                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file hsm_signing.cpp
 * @brief HSM-backed SigningService implementation for update bundle signing.
 *
 * Adapts HSMProvider (PKCS#11 / stub) to the SigningService interface so that
 * ManifestSigner and other callers can sign and verify update bundles using
 * hardware-backed keys without ever exposing raw private key material.
 *
 * The signature is transported as the raw bytes of the base64-encoded string
 * returned by HSMProvider, which keeps the wire format stable regardless of
 * whether a real HSM or the in-process stub is in use.
 */

#include "security/signing.h"
#include "security/hsm_provider.h"
#include <memory>
#include <string>
#include <stdexcept>

namespace themis {

namespace {

/**
 * @brief SigningService backed by an HSMProvider instance.
 *
 * sign()   – delegates to HSMProvider::sign(), stores the returned
 *            base64 signature string as raw UTF-8 bytes.
 * verify() – rehydrates those bytes as a string and delegates to
 *            HSMProvider::verify().
 */
class HsmSigningService : public SigningService {
public:
    /**
     * @param hsm   Fully initialised HSMProvider (initialize() already called).
     * @param default_key_label  Key label used when key_id is empty.
     */
    HsmSigningService(std::shared_ptr<security::HSMProvider> hsm,
                      std::string default_key_label)
        : hsm_(std::move(hsm))
        , default_key_label_(std::move(default_key_label))
    {
        if (!hsm_) {
            throw std::invalid_argument("HsmSigningService: HSMProvider cannot be null");
        }
    }

    SigningResult sign(const std::vector<uint8_t>& data,
                       const std::string& key_id) override
    {
        const std::string& label = key_id.empty() ? default_key_label_ : key_id;
        auto hsm_result = hsm_->sign(data, label);

        SigningResult result;
        if (!hsm_result.success) {
            result.error = hsm_result.error_message;
            return result;
        }

        result.algorithm = hsm_result.algorithm.empty()
                           ? "HSM/RSA-SHA256"
                           : hsm_result.algorithm;

        // Store the base64 signature string as raw bytes so callers can
        // round-trip it through verify() without caring about the encoding.
        const std::string& b64 = hsm_result.signature_b64;
        result.signature.assign(b64.begin(), b64.end());
        return result;
    }

    bool verify(const std::vector<uint8_t>& data,
                const std::vector<uint8_t>& signature,
                const std::string& key_id) override
    {
        if (signature.empty()) return false;

        // Reconstruct the base64 string from the stored bytes.
        std::string sig_b64(signature.begin(), signature.end());
        const std::string& label = key_id.empty() ? default_key_label_ : key_id;
        return hsm_->verify(data, sig_b64, label);
    }

private:
    std::shared_ptr<security::HSMProvider> hsm_;
    std::string default_key_label_;
};

} // anonymous namespace

std::shared_ptr<SigningService> createHsmSigningService(
    std::shared_ptr<security::HSMProvider> hsm,
    const std::string& default_key_label)
{
    return std::make_shared<HsmSigningService>(std::move(hsm),
                                              default_key_label);
}

} // namespace themis
