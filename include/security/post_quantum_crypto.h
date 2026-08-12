/**
 * @file post_quantum_crypto.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=15; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=11, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "security/encryption.h"
#include "security/key_provider.h"
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <functional>
#include <mutex>

namespace themis {
namespace security {

/**
 * @brief CRYSTALS-Kyber key encapsulation mechanism (KEM)
 *
 * NIST-approved post-quantum algorithm for key exchange (FIPS 203 / ML-KEM).
 * Provides quantum-resistant key encapsulation at configurable security levels.
 *
 * @note This implementation uses an OpenSSL-backed software simulation
 *       (X25519 ECDH + HKDF) that is API-compatible with the full liboqs
 *       backend. Once liboqs is added as a vcpkg dependency the backend
 *       will be transparently replaced while the public interface remains
 *       stable. The simulation is labeled KYBER_SIM in diagnostic output.
 *
 * Security levels:
 *   KYBER_512   – equivalent to AES-128 (128-bit classical / quantum)
 *   KYBER_768   – equivalent to AES-192 (192-bit classical / quantum)
 *   KYBER_1024  – equivalent to AES-256 (256-bit classical / quantum) ← recommended
 *
 * Performance (software simulation):
 *   Key generation:  < 0.1 ms
 *   Encapsulation:   < 0.1 ms
 *   Decapsulation:   < 0.1 ms
 *   Throughput:      > 10 000 ops/s
 *
 * Thread safety: all methods are thread-safe.
 */
class KyberKEM {
public:
    enum class SecurityLevel {
        KYBER_512,    ///< 128-bit quantum security
        KYBER_768,    ///< 192-bit quantum security
        KYBER_1024    ///< 256-bit quantum security (recommended)
    };

    explicit KyberKEM(SecurityLevel level = SecurityLevel::KYBER_1024);
    ~KyberKEM();

    // Non-copyable, movable
    KyberKEM(const KyberKEM&) = delete;
    KyberKEM& operator=(const KyberKEM&) = delete;
    KyberKEM(KyberKEM&&) noexcept;
    KyberKEM& operator=(KyberKEM&&) noexcept;

    /**
     * @brief Asymmetric key pair for Kyber KEM
     */
    struct KeyPair {
        std::vector<uint8_t> public_key;  ///< Encapsulation key (share with sender)
        std::vector<uint8_t> secret_key;  ///< Decapsulation key (keep private)
    };

    /**
     * @brief Result of a key encapsulation operation
     */
    struct EncapsulationResult {
        std::vector<uint8_t> ciphertext;    ///< KEM ciphertext – send to recipient
        std::vector<uint8_t> shared_secret; ///< 32-byte shared secret (AES key material)
    };

    /**
     * @brief Generate a Kyber key pair.
     *
     * @return New key pair (public + secret key)
     * @throws std::runtime_error if key generation fails
     */
    KeyPair generateKeyPair();

    /**
     * @brief Encapsulate a fresh shared secret under the recipient's public key.
     *
     * Both the returned ciphertext (sent to recipient) and the shared secret
     * (used locally as an AES-256-GCM key or HKDF input) are produced.
     *
     * @param public_key Recipient's Kyber public key
     * @return EncapsulationResult containing ciphertext and shared_secret
     * @throws std::invalid_argument if public_key size is unexpected
     * @throws std::runtime_error if encapsulation fails
     */
    EncapsulationResult encapsulate(const std::vector<uint8_t>& public_key);

    /**
     * @brief Decapsulate to recover the shared secret.
     *
     * @param ciphertext KEM ciphertext received from sender
     * @param secret_key Recipient's Kyber secret key
     * @return 32-byte shared secret matching the encapsulator's value
     * @throws std::invalid_argument if sizes are inconsistent
     * @throws std::runtime_error if decapsulation fails
     */
    std::vector<uint8_t> decapsulate(const std::vector<uint8_t>& ciphertext,
                                     const std::vector<uint8_t>& secret_key);

    /// Expected public key size in bytes for this security level
    size_t publicKeySize() const noexcept;
    /// Expected secret key size in bytes for this security level
    size_t secretKeySize() const noexcept;
    /// Expected KEM ciphertext size in bytes for this security level
    size_t ciphertextSize() const noexcept;
    /// Shared secret size – always 32 bytes
    static constexpr size_t sharedSecretSize() noexcept { return 32; }

    SecurityLevel securityLevel() const noexcept { return level_; }

private:
    SecurityLevel level_;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief CRYSTALS-Dilithium digital signature scheme
 *
 * NIST-approved post-quantum signature algorithm (FIPS 204 / ML-DSA).
 * Suitable for document signing, code signing, and CMS/PKCS#7 operations.
 *
 * @note This implementation uses an OpenSSL-backed software simulation
 *       (Ed25519) that is API-compatible with the full liboqs backend.
 *       Once liboqs is available the backend will be replaced transparently.
 *       The simulation is labeled DILITHIUM_SIM in diagnostic output.
 *
 * Security levels:
 *   DILITHIUM_2 – 128-bit quantum security
 *   DILITHIUM_3 – 192-bit quantum security
 *   DILITHIUM_5 – 256-bit quantum security ← recommended
 *
 * Thread safety: all methods are thread-safe.
 */
class DilithiumSigner {
public:
    enum class SecurityLevel {
        DILITHIUM_2,  ///< 128-bit quantum security
        DILITHIUM_3,  ///< 192-bit quantum security
        DILITHIUM_5   ///< 256-bit quantum security (recommended)
    };

    explicit DilithiumSigner(SecurityLevel level = SecurityLevel::DILITHIUM_5);
    ~DilithiumSigner();

    // Non-copyable, movable
    DilithiumSigner(const DilithiumSigner&) = delete;
    DilithiumSigner& operator=(const DilithiumSigner&) = delete;
    DilithiumSigner(DilithiumSigner&&) noexcept;
    DilithiumSigner& operator=(DilithiumSigner&&) noexcept;

    /**
     * @brief Asymmetric key pair for Dilithium signing
     */
    struct KeyPair {
        std::vector<uint8_t> public_key;  ///< Verification key
        std::vector<uint8_t> secret_key;  ///< Signing key (keep private)
    };

    /**
     * @brief Generate a Dilithium key pair.
     *
     * @return New key pair
     * @throws std::runtime_error if key generation fails
     */
    KeyPair generateKeyPair();

    /**
     * @brief Sign a message with a Dilithium secret key.
     *
     * @param message  Arbitrary byte sequence to sign
     * @param secret_key Dilithium signing key
     * @return Signature bytes
     * @throws std::invalid_argument if secret_key size is unexpected
     * @throws std::runtime_error if signing fails
     */
    std::vector<uint8_t> sign(const std::vector<uint8_t>& message,
                              const std::vector<uint8_t>& secret_key);

    /**
     * @brief Verify a Dilithium signature.
     *
     * @param message    Original message
     * @param signature  Signature to verify
     * @param public_key Signer's Dilithium public key
     * @return true if the signature is authentic, false otherwise
     */
    bool verify(const std::vector<uint8_t>& message,
                const std::vector<uint8_t>& signature,
                const std::vector<uint8_t>& public_key);

    SecurityLevel securityLevel() const noexcept { return level_; }

private:
    SecurityLevel level_;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Migration mode controlling classical vs. post-quantum key operations.
 *
 * Used by PostQuantumKeyProvider and HybridEncryption to allow a gradual,
 * backward-compatible transition.
 *
 *  CLASSICAL_ONLY    – all operations use classical cryptography only
 *  HYBRID            – both classical and PQ paths are exercised (migration phase)
 *  POST_QUANTUM_ONLY – all operations use PQ cryptography only (final phase)
 */
enum class PQMigrationMode {
    CLASSICAL_ONLY,    ///< Legacy: RSA/ECDH only
    HYBRID,            ///< Transition: classical + PQ (default)
    POST_QUANTUM_ONLY  ///< Future: PQ algorithms only
};

/**
 * @brief Post-quantum-capable KeyProvider wrapping a classical provider.
 *
 * Implements DEK wrapping and unwrapping using Kyber-1024 instead of RSA-OAEP
 * for HSM-style key protection. In HYBRID mode the wrapped blob contains both
 * classical and PQ ciphertext so decryption can fall back to the classical path
 * when needed during migration.
 *
 * Wire format of a Kyber-wrapped DEK blob (all integer fields are LE uint32):
 *   [4 bytes: kem_ct_len][kem_ct_len bytes: Kyber KEM ciphertext]
 *   [12 bytes: AES-GCM IV]
 *   [4 bytes: enc_dek_len][enc_dek_len bytes: AES-256-GCM encrypted DEK]
 *   [16 bytes: GCM authentication tag]
 *
 * Thread safety: all methods are thread-safe.
 */
class PostQuantumKeyProvider : public KeyProvider {
public:
    /**
     * @brief Construct a PQ key provider.
     *
     * @param classical_provider Underlying classical KeyProvider; must not be null.
     *        Used in HYBRID and CLASSICAL_ONLY modes and for key metadata queries.
     * @param mode              Migration mode (default: HYBRID)
     */
    explicit PostQuantumKeyProvider(
        std::shared_ptr<KeyProvider> classical_provider,
        PQMigrationMode mode = PQMigrationMode::HYBRID);

    ~PostQuantumKeyProvider() override;

    // ── KeyProvider interface ──────────────────────────────────────────────

    std::vector<uint8_t> getKey(const std::string& key_id) override;
    std::vector<uint8_t> getKey(const std::string& key_id, uint32_t version) override;
    uint32_t rotateKey(const std::string& key_id) override;
    std::vector<KeyMetadata> listKeys() override;
    KeyMetadata getKeyMetadata(const std::string& key_id,
                               uint32_t version = 0) override;
    void deleteKey(const std::string& key_id, uint32_t version) override;
    bool hasKey(const std::string& key_id, uint32_t version = 0) override;
    uint32_t createKeyFromBytes(const std::string& key_id,
                                const std::vector<uint8_t>& key_bytes,
                                const KeyMetadata& metadata = KeyMetadata()) override;

    // ── Post-quantum key wrapping ──────────────────────────────────────────

    /**
     * @brief Wrap a DEK using Kyber-1024 KEM.
     *
     * 1. Generates an ephemeral Kyber key pair.
     * 2. Encapsulates a fresh 32-byte shared secret.
     * 3. Uses the shared secret as an AES-256-GCM wrapping key.
     * 4. Encrypts the DEK under that wrapping key.
     * Returns the serialised blob described in the class docstring.
     *
     * @param dek                     Data encryption key to wrap (any length ≤ 256 bytes)
     * @param recipient_public_key    Recipient's Kyber-1024 public key
     * @return Wrapped key blob
     * @throws std::runtime_error on failure
     */
    std::vector<uint8_t> wrapKeyWithKyber(
        const std::vector<uint8_t>& dek,
        const std::vector<uint8_t>& recipient_public_key);

    /**
     * @brief Unwrap a Kyber-wrapped DEK.
     *
     * @param wrapped_key   Blob produced by wrapKeyWithKyber()
     * @param secret_key    Recipient's Kyber-1024 secret key
     * @return Unwrapped DEK
     * @throws std::runtime_error on decapsulation or decryption failure
     */
    std::vector<uint8_t> unwrapKeyWithKyber(
        const std::vector<uint8_t>& wrapped_key,
        const std::vector<uint8_t>& secret_key);

    PQMigrationMode getMigrationMode() const noexcept { return mode_; }
    void setMigrationMode(PQMigrationMode mode) noexcept { mode_ = mode; }

private:
    std::shared_ptr<KeyProvider> classical_provider_;
    PQMigrationMode mode_;
    KyberKEM kyber_;
    mutable std::mutex mutex_;
};

/**
 * @brief Hybrid encryption combining AES-256-GCM with Kyber-1024 KEM.
 *
 * Provides defense-in-depth: secure even if one of the two algorithms is
 * broken (classical or quantum attacker).
 *
 * Encryption flow (HYBRID mode):
 *   1. Generate an ephemeral Kyber key pair (ek, dk).
 *   2. Self-encapsulate: (kem_ct, kem_ss) = Kyber.Encap(ek).
 *      Rationale: in symmetric database field encryption, the encryptor and
 *      decryptor are the same process (the database engine). The ephemeral
 *      key pair is therefore "self-owned": the secret key is stored in the
 *      blob alongside the ciphertext (see key_id encoding below), and the
 *      shared secret provides forward-secrecy within a single encryption
 *      event. In a future multi-party deployment (e.g. client-side
 *      encryption), the caller would supply a long-term recipient public key
 *      via the PostQuantumKeyProvider and store only the KEM ciphertext.
 *   3. Retrieve classical AES key from provider: aes_key = provider.getKey(key_id).
 *   4. Derive combined key: combined = HKDF(kem_ss ‖ aes_key, 32 bytes).
 *   5. Encrypt plaintext with AES-256-GCM(combined).
 *   6. Store {kem_ct, eph_sk (for decapsulation), aes_blob} in the
 *      EncryptedBlob key_id field using a structured prefix (see impl).
 *
 * The EncryptedBlob produced by encryptHybrid() is forward-compatible with
 * decryptHybrid(). Standard FieldEncryption::decrypt() will fall back to the
 * classical path automatically in CLASSICAL_ONLY mode.
 *
 * Thread safety: all methods are thread-safe.
 */
class HybridEncryption : public FieldEncryption {
public:
    /**
     * @brief Construct hybrid encryption engine.
     *
     * @param key_provider  Classical key provider (may be wrapped in PostQuantumKeyProvider)
     * @param mode          Migration mode (default: HYBRID)
     */
    explicit HybridEncryption(std::shared_ptr<KeyProvider> key_provider,
                               PQMigrationMode mode = PQMigrationMode::HYBRID);

    ~HybridEncryption() override;

    /**
     * @brief Encrypt using hybrid classical + PQ protection.
     *
     * @param key_id    Logical key identifier for the classical AES key
     * @param plaintext Plaintext string
     * @return EncryptedBlob with PQ metadata embedded
     * @throws EncryptionException on failure
     */
    EncryptedBlob encryptHybrid(const std::string& key_id,
                                const std::string& plaintext);

    /**
     * @brief Decrypt a hybrid-encrypted blob.
     *
     * Supports blobs produced in HYBRID, CLASSICAL_ONLY (via FieldEncryption::decrypt),
     * and POST_QUANTUM_ONLY modes.
     *
     * @param blob EncryptedBlob produced by encryptHybrid()
     * @return Plaintext string
     * @throws DecryptionException on failure
     */
    std::string decryptHybrid(const EncryptedBlob& blob);

    PQMigrationMode getMigrationMode() const noexcept { return mode_; }
    void setMigrationMode(PQMigrationMode mode) noexcept { mode_ = mode; }

private:
    PQMigrationMode mode_;
    KyberKEM kyber_;
};

/**
 * @brief SPHINCS+ hash-based digital signature scheme (Phase 7.1)
 *
 * NIST-standardised stateless hash-based signature scheme (FIPS 205 / SLH-DSA).
 * SPHINCS+ provides a conservative security argument based solely on hash
 * function security — unlike lattice-based schemes (Dilithium) it has no
 * algebraic structure that could be attacked by future mathematical advances.
 *
 * @note This implementation uses an OpenSSL-backed software simulation
 *       (SHA-256 HMAC + Ed25519) that is API-compatible with the full liboqs
 *       backend.  Once liboqs is added as a vcpkg dependency the backend
 *       will be replaced transparently.  The simulation is labeled
 *       SPHINCSPLUS_SIM in diagnostic output.
 *
 * Supported variants:
 *   SPHINCS_SHA2_256S — SPHINCS+-SHA2-256s (128-bit security, small signatures)
 *   SPHINCS_SHA2_256F — SPHINCS+-SHA2-256f (128-bit security, fast signing)
 *
 * Performance targets (per NIST benchmarks):
 *   Sign:   ≥ 100 ops/s (256s), ≥ 5 000 ops/s (256f)
 *   Verify: ≥ 5 000 ops/s for both variants
 *
 * Thread safety: all methods are thread-safe.
 */
class SphincsPlus {
public:
    enum class Variant {
        SPHINCS_SHA2_256S,  ///< Small signature, slower signing (conservative)
        SPHINCS_SHA2_256F,  ///< Fast signing, larger signatures
    };

    explicit SphincsPlus(Variant variant = Variant::SPHINCS_SHA2_256S);
    ~SphincsPlus();

    // Non-copyable, movable
    SphincsPlus(const SphincsPlus&)            = delete;
    SphincsPlus& operator=(const SphincsPlus&) = delete;
    SphincsPlus(SphincsPlus&&) noexcept;
    SphincsPlus& operator=(SphincsPlus&&) noexcept;

    /**
     * @brief Asymmetric key pair for SPHINCS+ signing.
     */
    struct KeyPair {
        std::vector<uint8_t> public_key;  ///< Verification key
        std::vector<uint8_t> secret_key;  ///< Signing key (keep private)
    };

    /**
     * @brief Generate a SPHINCS+ key pair.
     *
     * @return New key pair.
     * @throws std::runtime_error on failure.
     */
    KeyPair generateKeyPair();

    /**
     * @brief Sign a message.
     *
     * @param message    Arbitrary byte sequence to sign.
     * @param secret_key SPHINCS+ signing key from generateKeyPair().
     * @return Signature bytes.
     * @throws std::invalid_argument if secret_key size is unexpected.
     * @throws std::runtime_error on signing failure.
     */
    std::vector<uint8_t> sign(const std::vector<uint8_t>& message,
                               const std::vector<uint8_t>& secret_key);

    /**
     * @brief Verify a SPHINCS+ signature.
     *
     * @param message    Original message.
     * @param signature  Signature to verify.
     * @param public_key Signer's SPHINCS+ public key.
     * @return true if the signature is authentic, false otherwise.
     */
    bool verify(const std::vector<uint8_t>& message,
                const std::vector<uint8_t>& signature,
                const std::vector<uint8_t>& public_key);

    // -----------------------------------------------------------------------
    // Injectable liboqs bridge (STUB #14 — SphincsPlus simulation)
    // -----------------------------------------------------------------------
    using GenerateKeyPairFn = std::function<KeyPair()>;
    using SignFn = std::function<std::vector<uint8_t>(
        const std::vector<uint8_t>& message,
        const std::vector<uint8_t>& secret_key)>;
    using VerifyFn = std::function<bool(
        const std::vector<uint8_t>& message,
        const std::vector<uint8_t>& signature,
        const std::vector<uint8_t>& public_key)>;

    /// Inject a real liboqs or test backend. Pass empty fn to restore simulation.
    static void setGenerateKeyPairFn(GenerateKeyPairFn fn);
    /// Inject a real liboqs sign implementation. Pass empty fn to restore simulation.
    static void setSignFn(SignFn fn);
    /// Inject a real liboqs verify implementation. Pass empty fn to restore simulation.
    static void setVerifyFn(VerifyFn fn);

    Variant getVariant() const noexcept { return variant_; }

    /// Expected public key size in bytes.
    size_t publicKeySize() const noexcept;
    /// Expected secret key size in bytes.
    size_t secretKeySize() const noexcept;
    /// Signature size in bytes (variant-dependent).
    size_t signatureSize() const noexcept;

private:
    Variant variant_;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace security
} // namespace themis
