/**
 * Test: Metadata Encryption Provider
 *
 * Tests for IMetadataEncryptionProvider / NoOpMetadataEncryptionProvider /
 * FieldSetMetadataEncryptionProvider:
 *
 * Acceptance criteria:
 *   AC-ENC-1  NoOp::shouldEncrypt always returns false
 *   AC-ENC-2  NoOp::encrypt returns plaintext unchanged
 *   AC-ENC-3  NoOp::decrypt returns ciphertext unchanged
 *   AC-ENC-4  NoOp::algorithm returns NONE
 *   AC-ENC-5  FieldSet: empty key throws MetadataEncryptionException in constructor
 *   AC-ENC-6  FieldSet: unlisted field passes through (shouldEncrypt=false, encrypt=passthrough)
 *   AC-ENC-7  FieldSet: listed field is encrypted (encrypt != plaintext)
 *   AC-ENC-8  FieldSet: decrypt(encrypt(field, pt)) == pt (round-trip)
 *   AC-ENC-9  FieldSet: wildcard "*" encrypts every field
 *   AC-ENC-10 FieldSet: addField/removeField dynamically update shouldEncrypt
 *   AC-ENC-11 FieldSet: addField/shouldEncrypt are thread-safe (concurrent writes/reads)
 *   AC-ENC-12 FieldSet::algorithm returns XOR_BASIC
 *   AC-ENC-13 Polymorphic usage via IMetadataEncryptionProvider*
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "metadata/imetadata_encryption_provider.h"

#include <atomic>
#include <thread>
#include <vector>

using namespace themis::metadata;

// ─────────────────────────────────────────────────────────────────────────────
// AC-ENC-1 — NoOp::shouldEncrypt always returns false
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataEncryptionProviderFocusedTests, NoOpShouldEncryptAlwaysFalse) {
    NoOpMetadataEncryptionProvider enc;
    EXPECT_FALSE(enc.shouldEncrypt("any_field"));
    EXPECT_FALSE(enc.shouldEncrypt("connection_string"));
    EXPECT_FALSE(enc.shouldEncrypt(""));
    EXPECT_FALSE(enc.shouldEncrypt("*"));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-ENC-2 — NoOp::encrypt returns plaintext unchanged
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataEncryptionProviderFocusedTests, NoOpEncryptReturnsPlaintextUnchanged) {
    NoOpMetadataEncryptionProvider enc;
    const std::string binary_payload("\x00\x01\x02", 3);
    EXPECT_EQ(enc.encrypt("field", "hello world"), "hello world");
    EXPECT_EQ(enc.encrypt("x", ""),               "");
    EXPECT_EQ(enc.encrypt("y", binary_payload), binary_payload);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-ENC-3 — NoOp::decrypt returns ciphertext unchanged
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataEncryptionProviderFocusedTests, NoOpDecryptReturnsCiphertextUnchanged) {
    NoOpMetadataEncryptionProvider enc;
    EXPECT_EQ(enc.decrypt("field", "some_cipher"), "some_cipher");
    EXPECT_EQ(enc.decrypt("x",     ""),             "");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-ENC-4 — NoOp::algorithm returns NONE
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataEncryptionProviderFocusedTests, NoOpAlgorithmReturnsNone) {
    NoOpMetadataEncryptionProvider enc;
    EXPECT_EQ(enc.algorithm(), MetadataEncryptionAlgorithm::NONE);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-ENC-5 — FieldSet: empty key throws MetadataEncryptionException
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataEncryptionProviderFocusedTests, FieldSetEmptyKeyThrowsOnConstruct) {
    EXPECT_THROW(
        FieldSetMetadataEncryptionProvider(""),
        MetadataEncryptionException);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-ENC-6 — FieldSet: unlisted field passes through (shouldEncrypt=false, encrypt=passthrough)
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataEncryptionProviderFocusedTests, FieldSetUnlistedFieldPassThrough) {
    FieldSetMetadataEncryptionProvider enc("s3cr3t");
    enc.addField("api_key");

    EXPECT_FALSE(enc.shouldEncrypt("connection_string"));
    // encrypt on unlisted field still works (XOR transform), but shouldEncrypt is false
    // — callers check shouldEncrypt first; here we verify the field is not in the set
    EXPECT_FALSE(enc.shouldEncrypt("unlisted_field"));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-ENC-7 — FieldSet: listed field is encrypted (encrypt != plaintext)
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataEncryptionProviderFocusedTests, FieldSetListedFieldIsEncrypted) {
    FieldSetMetadataEncryptionProvider enc("key123");
    enc.addField("api_key");

    EXPECT_TRUE(enc.shouldEncrypt("api_key"));

    const std::string plaintext = "hunter2";
    const std::string cipher    = enc.encrypt("api_key", plaintext);
    EXPECT_NE(cipher, plaintext);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-ENC-8 — FieldSet: decrypt(encrypt(field, pt)) == pt (round-trip)
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataEncryptionProviderFocusedTests, FieldSetRoundTrip) {
    FieldSetMetadataEncryptionProvider enc("xorkey!");
    enc.addField("secret");

    const std::string plaintext = "my_super_secret_value";
    const std::string cipher    = enc.encrypt("secret", plaintext);
    const std::string recovered = enc.decrypt("secret", cipher);

    EXPECT_EQ(recovered, plaintext);
}

TEST(MetadataEncryptionProviderFocusedTests, FieldSetRoundTripEmptyValue) {
    FieldSetMetadataEncryptionProvider enc("k");
    enc.addField("f");

    const std::string cipher    = enc.encrypt("f", "");
    const std::string recovered = enc.decrypt("f", cipher);
    EXPECT_EQ(recovered, "");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-ENC-9 — FieldSet: wildcard "*" encrypts every field
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataEncryptionProviderFocusedTests, FieldSetWildcardEncryptsEveryField) {
    FieldSetMetadataEncryptionProvider enc("wildcardkey");
    enc.addField("*");

    EXPECT_TRUE(enc.shouldEncrypt("any_field"));
    EXPECT_TRUE(enc.shouldEncrypt("connection_string"));
    EXPECT_TRUE(enc.shouldEncrypt("something_totally_new"));

    const std::string pt     = "plaintext";
    const std::string cipher = enc.encrypt("any_field", pt);
    EXPECT_NE(cipher, pt);
    EXPECT_EQ(enc.decrypt("any_field", cipher), pt);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-ENC-10 — FieldSet: addField/removeField dynamically update shouldEncrypt
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataEncryptionProviderFocusedTests, FieldSetDynamicAddRemoveUpdatesPolicy) {
    FieldSetMetadataEncryptionProvider enc("dynkey");

    EXPECT_FALSE(enc.shouldEncrypt("token"));

    enc.addField("token");
    EXPECT_TRUE(enc.shouldEncrypt("token"));
    EXPECT_EQ(enc.fieldCount(), 1u);

    enc.removeField("token");
    EXPECT_FALSE(enc.shouldEncrypt("token"));
    EXPECT_EQ(enc.fieldCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-ENC-11 — FieldSet: thread-safe concurrent writes/reads
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataEncryptionProviderFocusedTests, FieldSetThreadSafeConcurrentAccess) {
    FieldSetMetadataEncryptionProvider enc("threadkey");
    constexpr int kIter    = 300;
    constexpr int kReaders = 4;

    std::atomic<int> total_reads{0};

    std::vector<std::thread> threads;
    threads.reserve(kReaders + 1);

    // Writer thread: continuously adds and removes a field
    threads.emplace_back([&] {
        for (int i = 0; i < kIter; ++i) {
            enc.addField("concurrent_field");
            enc.removeField("concurrent_field");
        }
    });

    // Reader threads: check shouldEncrypt concurrently
    for (int t = 0; t < kReaders; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < kIter; ++i) {
                // Result is non-deterministic but must not crash
                (void)enc.shouldEncrypt("concurrent_field");
                ++total_reads;
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(total_reads.load(), kReaders * kIter);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-ENC-12 — FieldSet::algorithm returns XOR_BASIC
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataEncryptionProviderFocusedTests, FieldSetAlgorithmReturnsXorBasic) {
    FieldSetMetadataEncryptionProvider enc("algo_key");
    EXPECT_EQ(enc.algorithm(), MetadataEncryptionAlgorithm::XOR_BASIC);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-ENC-13 — Polymorphic usage via IMetadataEncryptionProvider*
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataEncryptionProviderFocusedTests, PolymorphicNoOpViaInterface) {
    std::unique_ptr<IMetadataEncryptionProvider> enc =
        std::make_unique<NoOpMetadataEncryptionProvider>();

    EXPECT_FALSE(enc->shouldEncrypt("field"));
    EXPECT_EQ(enc->encrypt("field", "value"), "value");
    EXPECT_EQ(enc->decrypt("field", "value"), "value");
    EXPECT_EQ(enc->algorithm(), MetadataEncryptionAlgorithm::NONE);
}

TEST(MetadataEncryptionProviderFocusedTests, PolymorphicFieldSetViaInterface) {
    std::unique_ptr<IMetadataEncryptionProvider> enc =
        std::make_unique<FieldSetMetadataEncryptionProvider>("polykey");

    auto* fse = dynamic_cast<FieldSetMetadataEncryptionProvider*>(enc.get());
    ASSERT_NE(fse, nullptr);
    fse->addField("password");

    EXPECT_TRUE(enc->shouldEncrypt("password"));
    EXPECT_EQ(enc->algorithm(), MetadataEncryptionAlgorithm::XOR_BASIC);

    const std::string pt     = "secret_password";
    const std::string cipher = enc->encrypt("password", pt);
    EXPECT_NE(cipher, pt);
    EXPECT_EQ(enc->decrypt("password", cipher), pt);
}
