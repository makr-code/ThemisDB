/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pki_client.h                                       ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:06:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     103                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2680d3d042  2026-03-15  feat(pki): complete stub replacement — PKCS#10 CSR provis... ║
    • 0f0e5dc3bc  2026-03-15  feat(pki): replace fallback stub verification with real P... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
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

    // X.509 Chain Verification: path to PEM trust store (CA bundle or CA cert).
    // When set, verifyHash() validates the full certificate chain via X509_verify_cert()
    // before accepting any signature.  Must be configured for production traffic.
    std::string trust_store_path;

    // Internal CA / ACME URL for automatic PKCS#10 CSR-based certificate provisioning.
    // When set, signHash() generates a PKCS#10 CSR via X509_REQ_* API and submits it to
    // {ca_url}/sign-csr to obtain a real X.509 certificate; only falls back to the stub
    // when this is empty AND no key_path/cert_path is configured.
    std::string ca_url;
};

struct SignatureResult {
    bool ok = false;
    std::string signature_id;       // opaque id from PKI
    std::string algorithm;          // e.g. RSA-SHA256
    std::string signature_b64;      // signature over provided hash (base64)
    std::string cert_serial;        // certificate serial used
};

// PKI client: sign/verify data hashes using OpenSSL.
// Production paths: local RSA key/cert, REST PKI endpoint, or PKCS#10 CSR provisioning via ca_url.
// Stub fallback paths are only compiled when THEMIS_TEST_MODE is defined.
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

    // Generate a PKCS#10 CSR (PEM-encoded) using the configured key and service_id.
    // Returns the PEM string on success, or empty string on failure.
    // Requires key_path to be set in the configuration.
    std::string generateCSR() const;

private:
    PKIConfig cfg_;

    // In-memory certificate cache populated via CSR provisioning (ca_url path).
    // Protected by cert_cache_mutex_ for thread-safe lazy initialisation.
    mutable std::mutex    cert_cache_mutex_;
    mutable std::string   cached_cert_pem_;
    mutable std::string   cached_cert_serial_;
};

} // namespace utils
} // namespace themis
