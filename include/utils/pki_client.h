/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pki_client.h                                       ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     82                                             ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace themis {
namespace utils {

struct PKIConfig {
    std::string service_id;
    std::string endpoint;           // e.g. https://localhost:8443/api/v1
    std::string cert_path;          // optional: certificate path (PEM)
    std::string key_path;           // optional: private key path (PEM)
    std::string key_passphrase;     // optional: private key passphrase
    std::string signature_algorithm = "RSA-SHA256";
    
    // TLS Hardening: Certificate Pinning
    bool enable_cert_pinning = false;                   // Enable certificate pinning
    std::vector<std::string> pinned_cert_fingerprints;  // SHA256 fingerprints (hex) of allowed certificates
    bool pin_leaf_only = false;                         // If true, pin only leaf cert; if false, pin any cert in chain
};

struct SignatureResult {
    bool ok = false;
    std::string signature_id;       // opaque id from PKI
    std::string algorithm;          // e.g. RSA-SHA256
    std::string signature_b64;      // signature over provided hash (base64)
    std::string cert_serial;        // certificate serial used
};

// Minimal PKI client (stub) to sign/verify data hashes.
// Implementation is local-only for now; can later call a real REST API.
class VCCPKIClient {
public:
    explicit VCCPKIClient(PKIConfig cfg);

    // Sign a precomputed hash (e.g. SHA-256 over ciphertext batch)
    SignatureResult signHash(const std::vector<uint8_t>& hash_bytes) const;

    // Verify a signature against a precomputed hash
    bool verifyHash(const std::vector<uint8_t>& hash_bytes, const SignatureResult& sig) const;

    const PKIConfig& config() const { return cfg_; }

    // Return certificate serial (hex) if a certificate path is configured and readable.
    // Returns empty optional when no cert is available or parsing fails.
    std::optional<std::string> getCertSerial() const;

private:
    PKIConfig cfg_;
};

} // namespace utils
} // namespace themis
