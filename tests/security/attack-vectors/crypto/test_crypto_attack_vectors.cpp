/**
 * @file test_crypto_attack_vectors.cpp
 * @brief Cryptographic attack vector tests for ThemisDB security module.
 *
 * These tests verify that the cryptographic subsystem correctly resists
 * known attack vectors including IV reuse, tampered ciphertexts, key
 * confusion, and padding-related attacks.
 *
 * Attack vector categories covered:
 *   - IV/nonce reuse detection and rejection
 *   - Authentication tag tampering (GCM tag verification)
 *   - Ciphertext manipulation resistance (AES-256-GCM AEAD)
 *   - Key confusion: decryption under wrong key must fail
 *   - Key version confusion: wrong key version must fail
 *   - Hybrid classical/post-quantum ciphertext integrity
 *   - Zero-length and boundary-value plaintext handling
 *   - Post-quantum key confusion (Kyber wrong secret key)
 *   - Post-quantum signature forgery rejection (Dilithium)
 *
 * CWE mapping:
 *   CWE-329 – Not Using a Random IV with CBC Mode
 *   CWE-330 – Use of Insufficiently Random Values
 *   CWE-347 – Improper Verification of Cryptographic Signature
 *   CWE-916 – Use of Password Hash with Insufficient Computational Effort
 *
 * OWASP ASVS:
 *   V6.2  – Algorithms
 *   V6.3  – Random Values
 *   V6.4  – Secret Management
 *
 * Compliance:
 *   NIST SP 800-38D – Recommendation for Block Cipher Modes of Operation: GCM
 *   FIPS 140-3      – Security Requirements for Cryptographic Modules
 */

#include <gtest/gtest.h>
#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include "security/post_quantum_crypto.h"
#include "themis/runtime_license_gate.h"

#include <string>
#include <vector>
#include <memory>
#include <set>
#include <cstdint>

using namespace themis;
using namespace themis::security;

// ─── Test Fixture ─────────────────────────────────────────────────────────

class CryptoAttackVectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::string license_error = {};
        field_encryption_available_ =
            themis::license::RuntimeLicenseGate::instance().isFeatureAllowed("field_encryption", license_error);
        provider_ = std::make_shared<MockKeyProvider>();
        provider_->createKey("attack_test_key", 1);
        enc_ = std::make_shared<FieldEncryption>(provider_);
    }

    bool expectEncryptionUnavailable() {
        if (!field_encryption_available_) {
            EXPECT_THROW(enc_->encrypt("attack_test_key", "probe"), std::runtime_error);
            return true;
        }
        return false;
    }

    std::shared_ptr<MockKeyProvider> provider_;
    std::shared_ptr<FieldEncryption> enc_;
    bool field_encryption_available_{false};
};

// ============================================================================
// Attack Vector: IV / Nonce Reuse
// CWE-329: Not Using a Random IV
// ============================================================================

/**
 * @brief AES-256-GCM nonce reuse would allow an attacker to recover the
 *        XOR of two plaintexts and forge authentication tags.  Verify that
 *        independently encrypted blobs always produce distinct IVs.
 */
TEST_F(CryptoAttackVectorTest, IVNonceReuseAttack_DistinctIVsRequired) {
    if (expectEncryptionUnavailable()) {
      return;
    }
    constexpr int kSamples = 50;
    std::set<std::vector<uint8_t>> seen_ivs;

    for (int i = 0; i < kSamples; ++i) {
        auto blob = enc_->encrypt("attack_test_key", "constant plaintext");
        ASSERT_FALSE(blob.iv.empty())
            << "IV must not be empty (iteration " << i << ")";
        // Every IV must be unique across all encrypt calls.
        auto [it, inserted] = seen_ivs.insert(blob.iv);
        EXPECT_TRUE(inserted)
            << "IV reuse detected at iteration " << i
            << " (CWE-329: IV reuse breaks AES-GCM confidentiality and integrity)";
    }
}

/**
 * @brief Even for identical plaintexts the ciphertext must differ due to
 *        a fresh random IV — otherwise an attacker can detect that the same
 *        data is being stored (deterministic ciphertext leaks metadata).
 */
TEST_F(CryptoAttackVectorTest, IVReuseAttack_DifferentCiphertextsForSamePlaintext) {
    if (expectEncryptionUnavailable()) {
      return;
    }
    const std::string plaintext = "sensitive database record";
    auto b1 = enc_->encrypt("attack_test_key", plaintext);
    auto b2 = enc_->encrypt("attack_test_key", plaintext);

    EXPECT_NE(b1.iv, b2.iv)
        << "Two encryptions of identical plaintext must yield different IVs";
    EXPECT_NE(b1.ciphertext, b2.ciphertext)
        << "Two encryptions of identical plaintext must yield different ciphertexts";
    // Both must still decrypt correctly.
    EXPECT_EQ(enc_->decrypt(b1), plaintext);
    EXPECT_EQ(enc_->decrypt(b2), plaintext);
}

// ============================================================================
// Attack Vector: Authentication Tag Tampering
// CWE-347: Improper Verification of Cryptographic Signature
// ============================================================================

/**
 * @brief Flip a single bit in the GCM authentication tag.  AES-256-GCM must
 *        reject the modified ciphertext and throw DecryptionException rather
 *        than returning corrupted plaintext.
 */
TEST_F(CryptoAttackVectorTest, AuthTagTampering_SingleBitFlip) {
    if (expectEncryptionUnavailable()) {
      return;
    }
    auto blob = enc_->encrypt("attack_test_key", "authenticated message");
    ASSERT_FALSE(blob.tag.empty());

    // Flip the MSB of the first tag byte — a minimal single-bit change.
    auto tampered = blob;
    tampered.tag[0] ^= 0x80;

    EXPECT_THROW(enc_->decrypt(tampered), DecryptionException)
        << "AES-256-GCM must reject a blob with a tampered authentication tag";
}

/**
 * @brief Replace the entire GCM tag with zeroes.  The AEAD check must fail.
 */
TEST_F(CryptoAttackVectorTest, AuthTagTampering_ZeroedTag) {
    if (expectEncryptionUnavailable()) {
      return;
    }
    auto blob = enc_->encrypt("attack_test_key", "zeroed tag test");

    auto tampered = blob;
    std::fill(tampered.tag.begin(), tampered.tag.end(), 0x00);

    EXPECT_THROW(enc_->decrypt(tampered), DecryptionException)
        << "AES-256-GCM must reject a blob whose authentication tag is all zeros";
}

// ============================================================================
// Attack Vector: Ciphertext Manipulation (Bit-Flip)
// NIST SP 800-38D §5.2.1 — Integrity protection
// ============================================================================

/**
 * @brief Flip a byte in the middle of the ciphertext.  AES-256-GCM's
 *        authentication tag covers the ciphertext, so the tampered blob must
 *        be rejected rather than producing garbled plaintext.
 */
TEST_F(CryptoAttackVectorTest, CiphertextBitFlip_MiddleOfPayload) {
    if (expectEncryptionUnavailable()) {
      return;
    }
    const std::string plaintext(128, 'A');
    auto blob = enc_->encrypt("attack_test_key", plaintext);
    ASSERT_GE(blob.ciphertext.size(), 64u);

    auto tampered = blob;
    tampered.ciphertext[blob.ciphertext.size() / 2] ^= 0xFF;

    EXPECT_THROW(enc_->decrypt(tampered), DecryptionException)
        << "Bit-flip in ciphertext must be detected by AES-256-GCM authentication";
}

/**
 * @brief Truncate the ciphertext by one byte.  Decryption must fail, not
 *        return a truncated or garbage plaintext.
 */
TEST_F(CryptoAttackVectorTest, CiphertextTruncation_OneByteShort) {
    if (expectEncryptionUnavailable()) {
      return;
    }
    const std::string plaintext = "truncation test plaintext";
    auto blob = enc_->encrypt("attack_test_key", plaintext);
    ASSERT_FALSE(blob.ciphertext.empty());

    auto tampered = blob;
    tampered.ciphertext.pop_back();

    EXPECT_THROW(enc_->decrypt(tampered), DecryptionException)
        << "Truncated ciphertext must not produce partial plaintext";
}

// ============================================================================
// Attack Vector: Key Confusion
// CWE-321: Use of Hard-Coded Cryptographic Key
// ============================================================================

/**
 * @brief Attempt to decrypt a blob using a different key ID.  The decryption
 *        must fail; the blob is bound to a specific key_id / key_version pair.
 */
TEST_F(CryptoAttackVectorTest, KeyConfusion_WrongKeyId) {
    if (expectEncryptionUnavailable()) {
      return;
    }
    provider_->createKey("other_key", 1);

    auto blob = enc_->encrypt("attack_test_key", "key confusion payload");
    // Spoof the key_id to fool the key lookup.
    auto confused = blob;
    confused.key_id = "other_key";

    EXPECT_THROW(enc_->decrypt(confused), DecryptionException)
        << "Decryption under a different key must fail (key confusion attack)";
}

/**
 * @brief Attempt to decrypt using a key that has been rotated.  The provider
 *        holds both versions; the blob must only decrypt correctly against its
 *        own version.
 */
TEST_F(CryptoAttackVectorTest, KeyConfusion_WrongKeyVersion) {
    if (expectEncryptionUnavailable()) {
      return;
    }
    // Encrypt with v1, then rotate to v2.
    auto blob = enc_->encrypt("attack_test_key", "version confusion payload");
    ASSERT_EQ(blob.key_version, 1u);

    provider_->rotateKey("attack_test_key");  // creates v2

    // Manually bump the version to force a wrong-version decryption attempt.
    auto confused = blob;
    confused.key_version = 2;

    EXPECT_THROW(enc_->decrypt(confused), DecryptionException)
        << "Decryption with a mismatched key version must fail";
}

/**
 * @brief After key rotation the old blob (v1) must still decrypt correctly
 *        (backward compatibility) while a new blob uses v2.
 */
TEST_F(CryptoAttackVectorTest, KeyRotation_OldBlobStillDecrypts) {
    if (expectEncryptionUnavailable()) {
      return;
    }
    const std::string plaintext = "data encrypted before rotation";
    auto old_blob = enc_->encrypt("attack_test_key", plaintext);
    ASSERT_EQ(old_blob.key_version, 1u);

    provider_->rotateKey("attack_test_key");  // v2 now active

    // Old blob must still decrypt with v1.
    EXPECT_EQ(enc_->decrypt(old_blob), plaintext);
}

// ============================================================================
// Attack Vector: Boundary Values
// ============================================================================

/**
 * @brief Zero-length plaintext is valid input.  Both encrypt and decrypt must
 *        round-trip without error.
 */
TEST_F(CryptoAttackVectorTest, BoundaryValue_EmptyPlaintext) {
    if (expectEncryptionUnavailable()) {
      return;
    }
    EXPECT_NO_THROW({
        auto blob = enc_->encrypt("attack_test_key", "");
        EXPECT_EQ(enc_->decrypt(blob), "");
    });
}

/**
 * @brief Very large plaintext (1 MiB) must encrypt and decrypt correctly.
 *        Verify no length-dependent overflow or partial-block error.
 */
TEST_F(CryptoAttackVectorTest, BoundaryValue_LargePlaintext_1MiB) {
    if (expectEncryptionUnavailable()) {
      return;
    }
    const std::string big(1024 * 1024, '\xAB');
    EXPECT_NO_THROW({
        auto blob = enc_->encrypt("attack_test_key", big);
        EXPECT_EQ(enc_->decrypt(blob), big);
    });
}

/**
 * @brief Plaintext consisting entirely of null bytes must not be confused with
 *        an empty string by any implicit null-termination logic.
 */
TEST_F(CryptoAttackVectorTest, BoundaryValue_AllNullBytes) {
    if (expectEncryptionUnavailable()) {
      return;
    }
    const std::string nulls(32, '\x00');
    auto blob = enc_->encrypt("attack_test_key", nulls);
    EXPECT_EQ(enc_->decrypt(blob), nulls);
}

// ============================================================================
// Attack Vector: Post-Quantum Key Confusion
// Kyber: wrong secret key must not recover the shared secret
// ============================================================================

TEST(PQCryptoAttackVector, Kyber_WrongSecretKeyDoesNotRecoverSharedSecret) {
    KyberKEM kem;
    auto kp_legit  = kem.generateKeyPair();
    auto kp_attacker = kem.generateKeyPair();

    auto enc = kem.encapsulate(kp_legit.public_key);

    // Attacker uses their own secret key — must not obtain the correct secret.
    auto wrong_ss = kem.decapsulate(enc.ciphertext, kp_attacker.secret_key);
    EXPECT_NE(enc.shared_secret, wrong_ss)
        << "Kyber decapsulation with a wrong secret key must not yield the correct shared secret";
}

/**
 * @brief Corrupted KEM ciphertext must cause decapsulation to throw rather than
 *        silently returning an incorrect shared secret (which would weaken
 *        the downstream AES key).
 */
TEST(PQCryptoAttackVector, Kyber_CorruptedCiphertextThrows) {
    KyberKEM kem;
    auto kp = kem.generateKeyPair();
    auto enc = kem.encapsulate(kp.public_key);

    auto bad_ct = enc.ciphertext;
    // Corrupt the last byte of the KEM ciphertext.
    bad_ct.back() ^= 0xFF;

    // The implementation may either throw or return a random-looking secret;
    // both are acceptable by ML-KEM spec.  We verify it does NOT match the
    // legitimate shared secret to prevent key-recovery.
    try {
        auto recovered = kem.decapsulate(bad_ct, kp.secret_key);
        EXPECT_NE(recovered, enc.shared_secret)
            << "Corrupted KEM ciphertext must not yield the original shared secret";
    } catch (const std::exception&) {
        // Throwing on bad ciphertext is also correct.
        SUCCEED();
    }
}

// ============================================================================
// Attack Vector: Post-Quantum Signature Forgery
// Dilithium: forged/tampered signatures must not verify
// ============================================================================

TEST(PQCryptoAttackVector, Dilithium_TamperedSignatureRejected) {
    DilithiumSigner signer;
    auto kp = signer.generateKeyPair();
    const std::vector<uint8_t> msg = {0xDE, 0xAD, 0xBE, 0xEF};

    auto sig = signer.sign(msg, kp.secret_key);
    ASSERT_FALSE(sig.empty());

    // Flip every byte of the signature — should always fail verification.
    std::vector<uint8_t> forged = sig;
    for (auto& b : forged) {
      b ^= 0xFF;
    }

    EXPECT_FALSE(signer.verify(msg, forged, kp.public_key))
        << "Fully inverted Dilithium signature must not verify";
}

TEST(PQCryptoAttackVector, Dilithium_WrongPublicKeyRejected) {
    DilithiumSigner signer;
    auto kp_signer   = signer.generateKeyPair();
    auto kp_attacker = signer.generateKeyPair();

    const std::vector<uint8_t> msg = {0x01, 0x02, 0x03};
    auto sig = signer.sign(msg, kp_signer.secret_key);

    // Verify against a different public key — must return false.
    EXPECT_FALSE(signer.verify(msg, sig, kp_attacker.public_key))
        << "Dilithium signature must not verify under an unrelated public key";
}

TEST(PQCryptoAttackVector, Dilithium_TamperedMessageRejected) {
    DilithiumSigner signer;
    auto kp = signer.generateKeyPair();

    std::vector<uint8_t> msg = {0xCA, 0xFE, 0xBA, 0xBE};
    auto sig = signer.sign(msg, kp.secret_key);

    // Alter a single byte of the message.
    msg[0] ^= 0x01;
    EXPECT_FALSE(signer.verify(msg, sig, kp.public_key))
        << "Single-byte change to message must invalidate Dilithium signature";
}

// ============================================================================
// Attack Vector: Hybrid Encryption Blob Integrity
// ============================================================================

TEST(PQCryptoAttackVector, HybridEncryption_TamperedPQMetaRejected) {
    auto provider = std::make_shared<MockKeyProvider>();
    provider->createKey("hybrid_av_key", 1);
    HybridEncryption henc(provider, PQMigrationMode::HYBRID);

    const std::string plaintext = "hybrid attack vector test";
    auto blob = henc.encryptHybrid("hybrid_av_key", plaintext);

    // Tamper with the blob's key_id field (the PQ metadata prefix).
    if (blob.key_id.size() > 5) {
        blob.key_id[4] ^= 0xFF;
    }

    EXPECT_THROW(henc.decryptHybrid(blob), std::exception)
        << "Tampered PQ metadata in key_id must cause decryption to fail";
}

TEST(PQCryptoAttackVector, HybridEncryption_TamperedCiphertextRejected) {
    auto provider = std::make_shared<MockKeyProvider>();
    provider->createKey("hybrid_ct_key", 1);
    HybridEncryption henc(provider, PQMigrationMode::HYBRID);

    auto blob = henc.encryptHybrid("hybrid_ct_key", "plaintext to protect");

    if (!blob.ciphertext.empty()) {
        blob.ciphertext[0] ^= 0xFF;
    }

    EXPECT_THROW(henc.decryptHybrid(blob), std::exception)
        << "Tampered ciphertext in hybrid blob must be rejected by GCM authentication";
}
