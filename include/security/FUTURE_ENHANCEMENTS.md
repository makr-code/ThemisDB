# Security Module Headers - Future Enhancements

## Scope

- API-level enhancements to `include/security/` headers
- Post-quantum crypto interface (`KyberKEM`, `DilithiumSigner`, `HybridEncryption`)
- HSM/PKCS#11 API (`HSMKeyProvider`, async key operations)
- Zero-knowledge proof interface (`ZKProofSystem`, `RangeProof`, `ZKAuthentication`)
- Threshold signature API (`ThresholdSignature`, `DistributedKeyGenerator`)
- AES-256-GCM stream API (hardware-accelerated, AES-NI backed)

## Design Constraints

- [x] All crypto APIs use `Result<T>` for error handling — no exceptions thrown
- [x] PQC algorithms are compile-time selectable via feature flags (e.g., `THEMIS_HAS_POST_QUANTUM`)
- [~] HSM operations are async — blocking HSM calls must not stall the I/O thread
- [x] New headers are strictly additive — no modifications to existing stable API surfaces
- [x] All key material is represented as opaque byte vectors; no raw key types in header signatures

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `KyberKEM` / `DilithiumSigner` | Key exchange, digital signatures | NIST PQC standard; compile-time selectable level |
| `HybridEncryption` | Field-level encryption, data at rest | AES-256-GCM + Kyber-1024 defense-in-depth |
| `ZKProofSystem` / `RangeProof` | Privacy-preserving queries, age/range attestation | zkSNARK-based; proving key pre-computed |
| `ThresholdSignature` | Multi-party approval workflows | K-of-N combining; `DistributedKeyGenerator` dependency |
| `HSMKeyProvider` | Production key management | Async; PKCS#11 compatible |

Planned enhancements to the security module public API.

## Overview

This document describes planned additions and improvements to the security module's public interface, focusing on:
- New cryptographic capabilities
- Enhanced key management
- Advanced threat detection
- Compliance features
- Performance optimizations

## New Headers (Planned)

### Post-Quantum Cryptography

#### `post_quantum_encryption.h` (Q2 2025)
**Purpose**: NIST post-quantum cryptographic algorithms

```cpp
#pragma once

#include "security/encryption.h"
#include <vector>
#include <string>

namespace themis {
namespace security {

/**
 * @brief CRYSTALS-Kyber key encapsulation mechanism
 *
 * NIST-approved post-quantum algorithm for key exchange.
 * Provides 256-bit security level against quantum attacks.
 */
class KyberKEM {
public:
    enum class SecurityLevel {
        KYBER_512,   // 128-bit quantum security
        KYBER_768,   // 192-bit quantum security
        KYBER_1024   // 256-bit quantum security (recommended)
    };

    explicit KyberKEM(SecurityLevel level = SecurityLevel::KYBER_1024);

    // Generate key pair
    struct KeyPair {
        std::vector<uint8_t> public_key;
        std::vector<uint8_t> secret_key;
    };
    KeyPair generateKeyPair();

    // Encapsulate (generate shared secret + ciphertext)
    struct EncapsulationResult {
        std::vector<uint8_t> ciphertext;
        std::vector<uint8_t> shared_secret;
    };
    EncapsulationResult encapsulate(const std::vector<uint8_t>& public_key);

    // Decapsulate (recover shared secret)
    std::vector<uint8_t> decapsulate(const std::vector<uint8_t>& ciphertext,
                                     const std::vector<uint8_t>& secret_key);
};

/**
 * @brief CRYSTALS-Dilithium digital signatures
 *
 * NIST-approved post-quantum signature algorithm.
 */
class DilithiumSigner {
public:
    enum class SecurityLevel {
        DILITHIUM_2,  // 128-bit quantum security
        DILITHIUM_3,  // 192-bit quantum security
        DILITHIUM_5   // 256-bit quantum security (recommended)
    };

    explicit DilithiumSigner(SecurityLevel level = SecurityLevel::DILITHIUM_5);

    // Generate signing key pair
    struct KeyPair {
        std::vector<uint8_t> public_key;
        std::vector<uint8_t> secret_key;
    };
    KeyPair generateKeyPair();

    // Sign message
    std::vector<uint8_t> sign(const std::vector<uint8_t>& message,
                              const std::vector<uint8_t>& secret_key);

    // Verify signature
    bool verify(const std::vector<uint8_t>& message,
               const std::vector<uint8_t>& signature,
               const std::vector<uint8_t>& public_key);
};

/**
 * @brief Hybrid encryption (classical + post-quantum)
 *
 * Combines AES-256-GCM with Kyber-1024 for defense-in-depth.
 */
class HybridEncryption : public FieldEncryption {
public:
    explicit HybridEncryption(std::shared_ptr<KeyProvider> key_provider);

    // Encrypt with both classical and post-quantum keys
    EncryptedBlob encrypt(const std::string& key_id,
                         const std::string& plaintext) override;

    // Decrypt (works if either key is compromised)
    std::string decrypt(const EncryptedBlob& blob) override;

private:
    KyberKEM kyber_;
};

} // namespace security
} // namespace themis
```

---

#### `zero_knowledge_proofs.h` (Q4 2025)
**Purpose**: Zero-knowledge proof generation and verification

```cpp
#pragma once

#include <vector>
#include <string>
#include <optional>

namespace themis {
namespace security {

/**
 * @brief zkSNARK proof system for verifiable computation
 *
 * Prove correctness of computation without revealing inputs.
 */
class ZKProofSystem {
public:
    // Generate proving/verification keys
    struct Keys {
        std::vector<uint8_t> proving_key;
        std::vector<uint8_t> verification_key;
    };
    Keys setup(const std::string& circuit);

    // Generate proof
    struct Proof {
        std::vector<uint8_t> proof_data;
        std::vector<uint8_t> public_inputs;
    };
    Proof prove(const std::vector<uint8_t>& proving_key,
               const std::vector<uint8_t>& witness);

    // Verify proof
    bool verify(const std::vector<uint8_t>& verification_key,
               const Proof& proof);
};

/**
 * @brief Range proofs (prove value in range without revealing value)
 */
class RangeProof {
public:
    // Prove value in [min, max]
    struct Proof {
        std::vector<uint8_t> commitment;
        std::vector<uint8_t> proof_data;
    };
    Proof proveInRange(int64_t value, int64_t min, int64_t max);

    // Verify range proof
    bool verify(const Proof& proof, int64_t min, int64_t max);

    // Example: Age verification (>= 18) without revealing exact age
    Proof proveAgeMinimum(int64_t age, int64_t minimum = 18);
};

/**
 * @brief Zero-knowledge authentication
 */
class ZKAuthentication {
public:
    // Generate challenge for user
    struct Challenge {
        std::vector<uint8_t> nonce;
        int64_t timestamp;
    };
    Challenge generateChallenge(const std::string& username);

    // User proves knowledge of password without sending it
    struct Proof {
        std::vector<uint8_t> response;
    };
    Proof provePassword(const std::string& password, const Challenge& challenge);

    // Verify proof
    bool verifyProof(const std::string& username,
                    const Challenge& challenge,
                    const Proof& proof);
};

} // namespace security
} // namespace themis
```

---

### Advanced Key Management

#### `distributed_key_generation.h` (Q2 2025)
**Purpose**: Threshold cryptography and distributed key generation

```cpp
#pragma once

#include "security/key_provider.h"
#include <vector>
#include <string>

namespace themis {
namespace security {

/**
 * @brief Distributed Key Generation (DKG)
 *
 * Generate keys across multiple parties without single point of trust.
 */
class DistributedKeyGenerator {
public:
    struct DKGConfig {
        int threshold;        // Minimum shares needed (e.g., 3 of 5)
        int total_shares;     // Total shares to generate
        std::vector<std::string> participant_ids;
    };

    // Key share for one participant
    struct KeyShare {
        std::string participant_id;
        uint32_t share_index;
        std::vector<uint8_t> share_data;
        std::vector<uint8_t> verification_data;
    };

    // Generate shares (coordinator runs this)
    std::vector<KeyShare> generateShares(const std::string& key_id,
                                         const DKGConfig& config);

    // Reconstruct key from threshold shares
    std::vector<uint8_t> reconstructKey(const std::vector<KeyShare>& shares);

    // Verify share is valid
    bool verifyShare(const KeyShare& share);

    // Refresh shares (change shares without changing key)
    std::vector<KeyShare> refreshShares(const std::vector<KeyShare>& old_shares);
};

/**
 * @brief Threshold signatures (require K of N signatures)
 */
class ThresholdSignature {
public:
    struct Config {
        int threshold;      // Minimum signatures required
        int total_parties;  // Total signing parties
    };

    // Partial signature from one party
    struct PartialSignature {
        std::string party_id;
        std::vector<uint8_t> partial_sig;
    };

    // Sign with one share
    PartialSignature signPartial(const std::vector<uint8_t>& message,
                                 const DistributedKeyGenerator::KeyShare& share);

    // Combine partial signatures
    std::vector<uint8_t> combineSignatures(
        const std::vector<PartialSignature>& partials);

    // Verify combined signature
    bool verify(const std::vector<uint8_t>& message,
               const std::vector<uint8_t>& signature,
               const std::vector<uint8_t>& public_key);
};

/**
 * @brief Secure Multi-Party Computation (MPC)
 */
class SecureMultiPartyComputation {
public:
    // Secret share input
    struct Share {
        uint32_t party_id;
        std::vector<uint8_t> share_data;
    };

    // Share value across parties
    std::vector<Share> share(const std::vector<uint8_t>& input,
                            int num_parties);

    // Compute function on shares (e.g., sum, average, max)
    enum class ComputeOp { SUM, AVERAGE, MAX, MIN };
    std::vector<Share> compute(const std::vector<std::vector<Share>>& inputs,
                               ComputeOp operation);

    // Reconstruct result
    std::vector<uint8_t> reconstruct(const std::vector<Share>& shares);
};

} // namespace security
} // namespace themis
```

---

### Hardware Security Enhancements

#### `trusted_execution_environment.h` (Q4 2025)
**Purpose**: Trusted Execution Environment (TEE) integration

```cpp
#pragma once

#include <vector>
#include <string>
#include <memory>

namespace themis {
namespace security {

/**
 * @brief Intel SGX enclave interface
 */
class SGXEnclave {
public:
    // Create enclave
    bool create(const std::string& enclave_path);

    // Destroy enclave
    void destroy();

    // Execute code in enclave
    struct EnclaveResult {
        std::vector<uint8_t> output;
        bool success;
    };
    EnclaveResult executeSecure(const std::string& function_name,
                                const std::vector<uint8_t>& input);

    // Seal data to enclave (encrypted with CPU key)
    std::vector<uint8_t> seal(const std::vector<uint8_t>& plaintext);

    // Unseal data
    std::vector<uint8_t> unseal(const std::vector<uint8_t>& sealed_data);

    // Generate attestation report
    struct AttestationReport {
        std::vector<uint8_t> report_data;
        std::vector<uint8_t> quote;
        std::string enclave_hash;
    };
    AttestationReport generateAttestation();

    // Verify attestation
    bool verifyAttestation(const AttestationReport& report);
};

/**
 * @brief AMD SEV (Secure Encrypted Virtualization)
 */
class SEVProtection {
public:
    // Enable SEV for VM
    bool enableSEV(const std::string& vm_id);

    // Get attestation token
    struct AttestationToken {
        std::vector<uint8_t> measurement;
        std::vector<uint8_t> signature;
    };
    AttestationToken getAttestation(const std::string& vm_id);

    // Verify VM integrity
    bool verifyVM(const AttestationToken& token);
};

/**
 * @brief ARM TrustZone secure world
 */
class TrustZoneSecureWorld {
public:
    // Execute operation in secure world
    struct SecureOperation {
        std::string operation_name;
        std::vector<uint8_t> input;
    };

    struct SecureResult {
        std::vector<uint8_t> output;
        bool success;
    };
    SecureResult executeSecure(const SecureOperation& op);

    // Store key in secure storage
    bool storeKeySecure(const std::string& key_id,
                       const std::vector<uint8_t>& key);

    // Retrieve key from secure storage
    std::optional<std::vector<uint8_t>> getKeySecure(const std::string& key_id);
};

/**
 * @brief TPM (Trusted Platform Module) integration
 */
class TPMProvider : public KeyProvider {
public:
    // Seal key to TPM (bound to PCR values)
    bool sealKeyToTPM(const std::string& key_id,
                     const std::vector<uint8_t>& key,
                     const std::vector<uint32_t>& pcr_indices);

    // Unseal key (only if PCRs match)
    std::optional<std::vector<uint8_t>> unsealKey(const std::string& key_id);

    // Generate attestation quote
    struct AttestationQuote {
        std::vector<uint8_t> quote;
        std::vector<uint8_t> signature;
        std::vector<uint32_t> pcr_values;
    };
    AttestationQuote quote(const std::vector<uint8_t>& nonce);

    // Verify quote
    bool verifyQuote(const AttestationQuote& quote,
                    const std::vector<uint8_t>& nonce);
};

} // namespace security
} // namespace themis
```

---

### Advanced Threat Detection

#### `behavioral_analytics.h` (Q2 2026)
**Purpose**: Machine learning-based behavioral analysis

```cpp
#pragma once

#include <vector>
#include <string>
#include <chrono>

namespace themis {
namespace security {

/**
 * @brief Behavioral security analyzer
 */
class BehavioralAnalyzer {
public:
    // User behavior profile
    struct UserBehavior {
        std::string user_id;

        // Query patterns
        struct QueryPattern {
            std::string query_type;
            double frequency;
            std::vector<std::string> typical_collections;
        };
        std::vector<QueryPattern> query_patterns;

        // Access patterns
        struct AccessPattern {
            std::string resource;
            std::vector<int> typical_hours;  // Hours of day
            std::vector<std::string> typical_locations;
        };
        std::vector<AccessPattern> access_patterns;

        // Time profile
        struct TimeProfile {
            std::vector<int> active_hours;
            std::vector<int> active_days;
        };
        TimeProfile time_profile;
    };

    // Learn normal behavior from audit logs
    void train(const std::vector<AuditEvent>& events);

    // Anomaly score (0.0 = normal, 1.0 = highly anomalous)
    struct AnomalyScore {
        double score;
        std::string reason;
        std::vector<std::string> anomaly_factors;
    };
    AnomalyScore scoreEvent(const AuditEvent& event);

    // Detect anomalies in real-time
    struct SecurityAlert {
        std::string user_id;
        AnomalyScore score;
        AuditEvent event;
        std::chrono::system_clock::time_point timestamp;
    };
    std::vector<SecurityAlert> detectAnomalies(
        const std::vector<AuditEvent>& recent_events);
};

/**
 * @brief ML-based threat detection
 */
class MLThreatDetector {
public:
    // Threat classification
    enum class ThreatType {
        BENIGN,
        DATA_EXFILTRATION,
        PRIVILEGE_ESCALATION,
        ACCOUNT_TAKEOVER,
        SQL_INJECTION,
        DENIAL_OF_SERVICE,
        UNKNOWN
    };

    struct ThreatClassification {
        ThreatType type;
        double confidence;
        std::string explanation;
    };

    // Train model on labeled data
    void train(const std::vector<LabeledEvent>& training_data);

    // Classify new event
    ThreatClassification classify(const AuditEvent& event);

    // Adaptive learning (update model with new threats)
    void updateModel(const std::vector<ConfirmedThreat>& new_threats);

    // Model metrics
    struct ModelMetrics {
        double accuracy;
        double precision;
        double recall;
        double f1_score;
    };
    ModelMetrics getMetrics();
};

/**
 * @brief Real-time intrusion detection
 */
class IntrusionDetectionSystem {
public:
    // Detection rule
    struct Rule {
        std::string name;
        std::string pattern;
        enum class Severity { LOW, MEDIUM, HIGH, CRITICAL } severity;
        enum class Action { LOG, ALERT, BLOCK } action;
    };

    // Load detection rules (Snort/Suricata format)
    void loadRules(const std::vector<Rule>& rules);

    // Process event in real-time
    void processEvent(const AuditEvent& event);

    // Security incident
    struct SecurityIncident {
        Rule triggered_rule;
        AuditEvent event;
        std::chrono::system_clock::time_point timestamp;
        bool blocked;
    };

    // Get incidents
    std::vector<SecurityIncident> getIncidents(
        std::chrono::system_clock::time_point since);

    // Auto-response
    void triggerResponse(const SecurityIncident& incident);
};

} // namespace security
} // namespace themis
```

---

### Compliance & Privacy

#### `differential_privacy.h` (Q4 2026)
**Purpose**: Privacy-preserving data analysis

```cpp
#pragma once

#include <vector>
#include <string>

namespace themis {
namespace security {

/**
 * @brief Differential privacy for query results
 */
class DifferentialPrivacy {
public:
    // Privacy parameters
    struct PrivacyParams {
        double epsilon;  // Privacy budget (smaller = more private)
        double delta;    // Probability of privacy breach
    };

    // Add calibrated noise to query result
    template<typename T>
    T addNoise(T value, const PrivacyParams& params);

    // Check privacy budget
    bool checkBudget(const std::string& dataset, double epsilon);

    // Consume privacy budget
    void consumeBudget(const std::string& dataset, double epsilon);

    // Reset budget (new analysis period)
    void resetBudget(const std::string& dataset);

    // Composition (calculate combined epsilon)
    double computeComposedEpsilon(const std::vector<double>& epsilons);
};

/**
 * @brief Synthetic data generation
 */
class SyntheticDataGenerator {
public:
    // Privacy parameters
    struct PrivacyParams {
        double epsilon;
        std::string method;  // "DP-GAN", "PATE", "PrivBayes"
    };

    // Generate synthetic dataset
    Dataset generateSynthetic(const Dataset& original,
                             const PrivacyParams& params);

    // Validate quality
    struct QualityMetrics {
        double statistical_similarity;
        double utility_score;
        double privacy_score;
    };
    QualityMetrics evaluateQuality(const Dataset& original,
                                   const Dataset& synthetic);
};

/**
 * @brief Homomorphic encryption (compute on encrypted data)
 */
class HomomorphicEncryption {
public:
    // Partially homomorphic (Paillier)
    class Paillier {
    public:
        struct Ciphertext {
            std::vector<uint8_t> data;
        };

        Ciphertext encrypt(int64_t plaintext);
        int64_t decrypt(const Ciphertext& ciphertext);

        // Homomorphic operations
        Ciphertext add(const Ciphertext& a, const Ciphertext& b);
        Ciphertext multiplyConstant(const Ciphertext& ct, int64_t constant);
    };

    // Fully homomorphic (Microsoft SEAL)
    class FullyHomomorphic {
    public:
        struct Ciphertext {
            std::vector<uint8_t> data;
        };

        Ciphertext encrypt(int64_t plaintext);
        int64_t decrypt(const Ciphertext& ciphertext);

        // Arbitrary arithmetic
        Ciphertext add(const Ciphertext& a, const Ciphertext& b);
        Ciphertext multiply(const Ciphertext& a, const Ciphertext& b);
        Ciphertext negate(const Ciphertext& a);
    };
};

} // namespace security
} // namespace themis
```

---

### Cloud Integration

#### `cloud_kms_provider.h` (Q2 2025)
**Purpose**: Cloud KMS integration (AWS, Azure, GCP)

```cpp
#pragma once

#include "security/key_provider.h"
#include <string>
#include <vector>

namespace themis {
namespace security {

/**
 * @brief AWS KMS key provider
 */
class AWSKMSProvider : public KeyProvider {
public:
    struct Config {
        std::string region;
        std::string access_key_id;
        std::string secret_access_key;
        std::string role_arn;  // Optional: assume role
    };

    explicit AWSKMSProvider(const Config& config);

    // Envelope encryption
    struct EnvelopeKey {
        std::vector<uint8_t> plaintext_key;
        std::vector<uint8_t> encrypted_key;
        std::string key_arn;
    };
    EnvelopeKey generateDataKey(const std::string& key_arn);

    // Encrypt/decrypt with KMS
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext,
                                 const std::string& key_arn) override;
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext,
                                 const std::string& key_arn) override;
};

/**
 * @brief Azure Key Vault provider
 */
class AzureKeyVaultProvider : public KeyProvider {
public:
    struct Config {
        std::string vault_url;
        std::string tenant_id;
        std::string client_id;
        std::string client_secret;
    };

    explicit AzureKeyVaultProvider(const Config& config);

    // Wrap/unwrap keys
    std::vector<uint8_t> wrapKey(const std::vector<uint8_t>& key,
                                 const std::string& vault_key_name) override;
    std::vector<uint8_t> unwrapKey(const std::vector<uint8_t>& wrapped_key,
                                    const std::string& vault_key_name) override;
};

/**
 * @brief Google Cloud KMS provider
 */
class GCPKMSProvider : public KeyProvider {
public:
    struct Config {
        std::string project_id;
        std::string location;
        std::string key_ring;
        std::string credentials_path;
    };

    explicit GCPKMSProvider(const Config& config);

    // Asymmetric operations
    std::vector<uint8_t> asymmetricEncrypt(
        const std::vector<uint8_t>& plaintext,
        const std::string& key_path) override;
    std::vector<uint8_t> asymmetricDecrypt(
        const std::vector<uint8_t>& ciphertext,
        const std::string& key_path) override;
};

} // namespace security
} // namespace themis
```

---

## API Evolution Strategy

### Backward Compatibility
- New headers are **additive** (don't break existing code)
- Use feature detection macros: `#ifdef THEMIS_HAS_POST_QUANTUM`
- Provide compatibility shims for deprecated APIs
- Maintain API stability guarantees (semantic versioning)

### Deprecation Process
1. **Announce**: Document deprecation in CHANGELOG
2. **Warn**: Add deprecation warnings (compiler attributes)
3. **Grace Period**: Minimum 2 major versions
4. **Remove**: Only in major version bumps

### Example Deprecation
```cpp
// Old API (deprecated in v2.0)
[[deprecated("Use FieldEncryption::encrypt() instead")]]
std::string encryptField(const std::string& data);

// New API (introduced in v2.0)
EncryptedBlob encrypt(const std::string& key_id, const std::string& data);
```

## Performance Goals

| Feature | Target Latency | Notes |
|---------|----------------|-------|
| Post-quantum encryption | <50μs | 2-5x slower than classical |
| Zero-knowledge proofs | <10ms | Proof generation |
| Threshold signatures | <100ms | K-of-N combining |
| Homomorphic addition | <1ms | Paillier scheme |
| Homomorphic multiply | <100ms | FHE scheme |
| Behavioral analysis | <500μs | Per-event scoring |
| ML threat detection | <1ms | Inference time |

## Migration Guide

### Adopting Post-Quantum Crypto
```cpp
// Phase 1: Hybrid mode (backwards compatible)
auto hybrid = std::make_shared<HybridEncryption>(key_provider);

// Phase 2: Full post-quantum
auto kyber = std::make_unique<KyberKEM>(SecurityLevel::KYBER_1024);

// Phase 3: Migrate existing data
// (background job re-encrypts with PQ keys)
```

### Enabling Behavioral Analytics
```cpp
// Step 1: Train on historical data
BehavioralAnalyzer analyzer;
analyzer.train(historical_audit_events);

// Step 2: Real-time scoring
auto score = analyzer.scoreEvent(current_event);
if (score.score > 0.8) {
    alertSecurityTeam(current_event, score);
}
```

## Testing Strategy

### Unit Tests
- Mock implementations for all new interfaces
- Property-based testing for crypto primitives
- Fuzzing for input validation

### Integration Tests
- Cloud KMS sandboxes (localstack, azurite)
- Hardware emulators (SoftHSM, SGX simulator)
- Performance benchmarks

### Security Audits
- External cryptographic review
- Penetration testing
- Formal verification (critical algorithms)

## Timeline

### 2025 Q2
- Post-quantum key encapsulation (Kyber)
- Distributed key generation
- AWS KMS integration
- Behavioral analytics

### 2025 Q4
- Post-quantum signatures (Dilithium)
- Zero-knowledge authentication
- Intel SGX support
- ML threat detection

### 2026 Q2
- Zero-knowledge proofs (zkSNARKs)
- Threshold signatures
- Azure/GCP KMS integration
- Advanced behavioral analytics

### 2026 Q4
- Homomorphic encryption (partial)
- Differential privacy
- TPM integration
- Synthetic data generation

### 2027+
- Fully homomorphic encryption
- Quantum key distribution (QKD)
- Complete post-quantum migration
- Advanced TEE support

## Contributing

Interested in implementing these features? See:
- [CONTRIBUTING.md](../../CONTRIBUTING.md)
- [Security development guide](../../docs/security-dev-guide.md)
- [API design guidelines](../../docs/api-design.md)

## References

### Standards
- [NIST Post-Quantum Cryptography](https://csrc.nist.gov/projects/post-quantum-cryptography)
- [FIPS 140-3](https://csrc.nist.gov/publications/detail/fips/140/3/final)
- [ISO/IEC 19790](https://www.iso.org/standard/52906.html) - Cryptographic Module Security

### Libraries
- [liboqs](https://github.com/open-quantum-safe/liboqs) - Open Quantum Safe
- [Microsoft SEAL](https://github.com/microsoft/SEAL) - Homomorphic Encryption
- [Intel SGX SDK](https://github.com/intel/linux-sgx)
- [libsodium](https://libsodium.gitbook.io/) - Modern crypto library

### Papers
- "Post-Quantum Cryptography" - Bernstein, Buchmann, Dahmen
- "Practical Homomorphic Encryption" - Gentry et al.
- "Zerocash: Decentralized Anonymous Payments" - Sasson et al.
- "The Algorithmic Foundations of Differential Privacy" - Dwork, Roth

## Test Strategy

- Unit tests for `KyberKEM` encapsulate/decapsulate roundtrip at all three security levels
- Property-based tests for `DilithiumSigner` sign/verify with random messages and key pairs
- Mock HSM tests for `HSMKeyProvider` async key operations under simulated latency
- ZK proof roundtrip tests for `RangeProof` with boundary values (min, max, out-of-range)
- Threshold signature recombination tests with K-1 (insufficient) and K (sufficient) partial signatures
- Compile-time flag tests confirming PQC headers absent without `THEMIS_HAS_POST_QUANTUM`

## Performance Targets

- AES-256-GCM stream throughput ≥ 1 GB/s with AES-NI hardware acceleration
- `KyberKEM::encapsulate()` ≥ 2,000 ops/s (Kyber-1024 security level)
- RSA-4096 signature verify ≤ 5 ms
- `DilithiumSigner::sign()` ≤ 2 ms per operation (Dilithium-5)
- ZK range proof generation ≤ 10 ms; verification ≤ 2 ms

---

## Paper 2 — Layer 7: IntentClassifier (IMPL-B7)

> Research paper: `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer 7
> Issue: `docs/issues/optimization_layers/IMPL-B7-intent-classifier.md`

### Scope
- `IntentClassifier` adds semantic query-intent analysis on top of existing `MLAnomalyDetector` pattern scoring
- `IntentType`: `NORMAL`, `SQL_INJECTION_ATTEMPT`, `MASS_EXPORT`, `PRIVILEGE_ESCALATION`, `RECONNAISSANCE`, `UNKNOWN`
- Confidence threshold 0.85 — below this, alert is logged but does not trigger ZeroTrust action

### Integration Notes
- `ZeroTrustPolicyEnforcer::session_risk_score` updated on high-confidence alerts
- `AIDecisionAuditor` receives a `DecisionRecord` for each alert (L9 audit trail)
- GDPR: `evidence_snippet` ≤ 128 chars; no PII in payload

### Performance Targets
- Classification latency ≤ 5 ms p99 per query
- False positive rate ≤ 2 % on benign SELECT-only workloads
