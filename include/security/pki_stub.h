#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace themis {
namespace security {

/// PKI (Public Key Infrastructure) stub interface
/// This is a placeholder for future PKI functionality
/// TODO GAP-004: Implement full PKI certificate management
class PKIManager {
public:
    PKIManager() = default;
    virtual ~PKIManager() = default;
    
    /// Generate a key pair (stub)
    /// @param key_size Key size in bits (e.g., 2048, 4096)
    /// @return Key pair ID (stub implementation returns placeholder)
    virtual std::string generateKeyPair(int key_size = 2048);
    
    /// Generate a certificate signing request (stub)
    /// @param key_id Key pair ID
    /// @param subject Subject distinguished name (e.g., "CN=example.com")
    /// @return CSR in PEM format (stub implementation returns placeholder)
    virtual std::string generateCSR(const std::string& key_id, const std::string& subject);
    
    /// Sign a certificate (stub)
    /// @param csr Certificate signing request
    /// @param ca_cert CA certificate
    /// @param ca_key CA private key
    /// @param validity_days Certificate validity in days
    /// @return Signed certificate in PEM format (stub implementation returns placeholder)
    virtual std::string signCertificate(
        const std::string& csr,
        const std::string& ca_cert,
        const std::string& ca_key,
        int validity_days = 365
    );
    
    /// Verify a certificate chain (stub)
    /// @param cert Certificate to verify
    /// @param ca_certs Chain of CA certificates
    /// @return true if valid (stub implementation returns true)
    virtual bool verifyCertificate(
        const std::string& cert,
        const std::vector<std::string>& ca_certs
    );
    
    /// Revoke a certificate (stub)
    /// @param cert_id Certificate ID or serial number
    /// @return true if revoked (stub implementation returns true)
    virtual bool revokeCertificate(const std::string& cert_id);
    
    /// Get certificate info (stub)
    struct CertificateInfo {
        std::string subject;
        std::string issuer;
        int64_t not_before = 0;
        int64_t not_after = 0;
        std::string serial_number;
        std::string fingerprint;
    };
    
    /// Parse certificate and extract information (stub)
    /// @param cert Certificate in PEM format
    /// @return Certificate information (stub implementation returns placeholder)
    virtual std::optional<CertificateInfo> parseCertificate(const std::string& cert);
    
    /// Export public key from certificate (stub)
    /// @param cert Certificate in PEM format
    /// @return Public key in PEM format (stub implementation returns placeholder)
    virtual std::string exportPublicKey(const std::string& cert);
    
    /// Check if implementation is a stub
    virtual bool isStub() const { return true; }
};

/// Signature operations stub interface
/// This is a placeholder for future signature functionality
/// TODO GAP-004: Implement full digital signature operations
class SignatureManager {
public:
    SignatureManager() = default;
    virtual ~SignatureManager() = default;
    
    /// Sign data with private key (stub)
    /// @param data Data to sign
    /// @param key_id Private key ID
    /// @param algorithm Signature algorithm (e.g., "RSA-SHA256", "ECDSA-SHA256")
    /// @return Signature bytes (stub implementation returns placeholder)
    virtual std::vector<uint8_t> sign(
        const std::vector<uint8_t>& data,
        const std::string& key_id,
        const std::string& algorithm = "RSA-SHA256"
    );
    
    /// Verify signature with public key (stub)
    /// @param data Original data
    /// @param signature Signature bytes
    /// @param public_key Public key in PEM format
    /// @param algorithm Signature algorithm
    /// @return true if valid (stub implementation returns true)
    virtual bool verify(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& public_key,
        const std::string& algorithm = "RSA-SHA256"
    );
    
    /// Sign and create detached signature (stub)
    /// @param data Data to sign
    /// @param key_id Private key ID
    /// @return Detached signature in Base64 (stub implementation returns placeholder)
    virtual std::string signDetached(
        const std::vector<uint8_t>& data,
        const std::string& key_id
    );
    
    /// Verify detached signature (stub)
    /// @param data Original data
    /// @param signature_b64 Detached signature in Base64
    /// @param public_key Public key in PEM format
    /// @return true if valid (stub implementation returns true)
    virtual bool verifyDetached(
        const std::vector<uint8_t>& data,
        const std::string& signature_b64,
        const std::string& public_key
    );
    
    /// Sign with timestamp (stub)
    /// @param data Data to sign
    /// @param key_id Private key ID
    /// @return Signed data with timestamp (stub implementation returns placeholder)
    struct TimestampedSignature {
        std::vector<uint8_t> signature;
        int64_t timestamp = 0;
        std::string tsa_certificate;
    };
    
    virtual TimestampedSignature signWithTimestamp(
        const std::vector<uint8_t>& data,
        const std::string& key_id
    );
    
    /// Get supported signature algorithms (stub)
    /// @return List of supported algorithms (stub implementation returns placeholder list)
    virtual std::vector<std::string> getSupportedAlgorithms();
    
    /// Check if implementation is a stub
    virtual bool isStub() const { return true; }
};

/// Factory for creating PKI and Signature managers
/// TODO GAP-004: Add real implementations when available
class SecurityManagerFactory {
public:
    /// Create PKI manager
    /// @param use_stub If true, returns stub implementation (default: true for now)
    /// @return PKI manager instance
    static std::unique_ptr<PKIManager> createPKIManager(bool use_stub = true);
    
    /// Create Signature manager
    /// @param use_stub If true, returns stub implementation (default: true for now)
    /// @return Signature manager instance
    static std::unique_ptr<SignatureManager> createSignatureManager(bool use_stub = true);
};

} // namespace security
} // namespace themis
