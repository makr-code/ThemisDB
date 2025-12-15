#include <gtest/gtest.h>
#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include "utils/logger.h"
#include <vector>
#include <cmath>

using namespace themis;

/**
 * Phase 1: Vector Embedding Encryption Test Suite
 * 
 * Coverage:
 * 1. Basic encrypt/decrypt for std::vector<float>
 * 2. Empty vector handling
 * 3. Large vectors (768-dim, 1536-dim embeddings)
 * 4. Precision preservation (float accuracy)
 * 5. Serialization/deserialization roundtrip
 * 6. Base64 encoding/decoding
 * 7. Error handling (missing encryption, invalid data)
 * 8. Performance benchmarks
 */

class VectorEncryptionPhase1Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize MockKeyProvider
        key_provider_ = std::make_shared<MockKeyProvider>();
        key_provider_->createKey("vector_embeddings", 1);
        
        // Initialize FieldEncryption
        field_encryption_ = std::make_shared<FieldEncryption>(key_provider_);
        
        // Set global EncryptedField encryption instance
        EncryptedField<std::vector<float>>::setFieldEncryption(field_encryption_);
    }
    
    void TearDown() override {
        field_encryption_.reset();
        key_provider_.reset();
    }
    
    std::shared_ptr<MockKeyProvider> key_provider_;
    std::shared_ptr<FieldEncryption> field_encryption_;
};

// Test 1: Basic encryption and decryption
TEST_F(VectorEncryptionPhase1Test, BasicEncryptDecrypt) {
    std::vector<float> original = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
    
    // Encrypt
    EncryptedField<std::vector<float>> encrypted_field;
    encrypted_field.encrypt(original, "vector_embeddings");
    
    EXPECT_TRUE(encrypted_field.hasValue());
    EXPECT_TRUE(encrypted_field.isEncrypted());
    
    // Decrypt
    std::vector<float> decrypted = encrypted_field.decrypt();
    
    // Verify
    ASSERT_EQ(decrypted.size(), original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_FLOAT_EQ(decrypted[i], original[i]);
    }
}

// Test 2: Empty vector
TEST_F(VectorEncryptionPhase1Test, EmptyVector) {
    std::vector<float> empty;
    
    EncryptedField<std::vector<float>> encrypted_field;
    encrypted_field.encrypt(empty, "vector_embeddings");
    
    std::vector<float> decrypted = encrypted_field.decrypt();
    
    EXPECT_EQ(decrypted.size(), 0);
}

// Test 3: Large vectors (realistic embedding sizes)
TEST_F(VectorEncryptionPhase1Test, LargeVectors) {
    // Test 768-dimensional embedding (BERT, LLaMA)
    std::vector<float> embedding_768(768);
    for (size_t i = 0; i < 768; ++i) {
        embedding_768[i] = static_cast<float>(i) / 768.0f;
    }
    
    EncryptedField<std::vector<float>> enc1;
    enc1.encrypt(embedding_768, "vector_embeddings");
    std::vector<float> dec1 = enc1.decrypt();
    
    ASSERT_EQ(dec1.size(), 768);
    for (size_t i = 0; i < 768; ++i) {
        EXPECT_FLOAT_EQ(dec1[i], embedding_768[i]);
    }
    
    // Test 1536-dimensional embedding (OpenAI ada-002)
    std::vector<float> embedding_1536(1536);
    for (size_t i = 0; i < 1536; ++i) {
        embedding_1536[i] = std::sin(static_cast<float>(i) * 0.1f);
    }
    
    EncryptedField<std::vector<float>> enc2;
    enc2.encrypt(embedding_1536, "vector_embeddings");
    std::vector<float> dec2 = enc2.decrypt();
    
    ASSERT_EQ(dec2.size(), 1536);
    for (size_t i = 0; i < 1536; ++i) {
        EXPECT_FLOAT_EQ(dec2[i], embedding_1536[i]);
    }
}

// Test 4: Precision preservation
TEST_F(VectorEncryptionPhase1Test, PrecisionPreservation) {
    std::vector<float> precise = {
        1.23456789f,
        -0.00000123f,
        1000000.0f,
        std::numeric_limits<float>::epsilon(),
        std::numeric_limits<float>::min(),
        std::numeric_limits<float>::max()
    };
    
    EncryptedField<std::vector<float>> encrypted_field;
    encrypted_field.encrypt(precise, "vector_embeddings");
    
    std::vector<float> decrypted = encrypted_field.decrypt();
    
    ASSERT_EQ(decrypted.size(), precise.size());
    for (size_t i = 0; i < precise.size(); ++i) {
        EXPECT_FLOAT_EQ(decrypted[i], precise[i]) 
            << "Mismatch at index " << i;
    }
}

// Test 5: Base64 roundtrip
TEST_F(VectorEncryptionPhase1Test, Base64Roundtrip) {
    std::vector<float> original = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    
    EncryptedField<std::vector<float>> enc1;
    enc1.encrypt(original, "vector_embeddings");
    
    // Serialize to base64
    std::string b64 = enc1.toBase64();
    EXPECT_FALSE(b64.empty());
    
    // Deserialize from base64
    auto enc2 = EncryptedField<std::vector<float>>::fromBase64(b64);
    
    // Decrypt
    std::vector<float> decrypted = enc2.decrypt();
    
    // Verify
    ASSERT_EQ(decrypted.size(), original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_FLOAT_EQ(decrypted[i], original[i]);
    }
}

// Test 6: JSON roundtrip
TEST_F(VectorEncryptionPhase1Test, JsonRoundtrip) {
    std::vector<float> original = {0.5f, 1.5f, 2.5f};
    
    EncryptedField<std::vector<float>> enc1;
    enc1.encrypt(original, "vector_embeddings");
    
    // Serialize to JSON
    nlohmann::json j = enc1.toJson();
    EXPECT_TRUE(j.contains("key_id"));
    EXPECT_TRUE(j.contains("key_version"));
    EXPECT_TRUE(j.contains("iv"));
    EXPECT_TRUE(j.contains("ciphertext"));
    EXPECT_TRUE(j.contains("tag"));
    
    // Deserialize from JSON
    auto enc2 = EncryptedField<std::vector<float>>::fromJson(j);
    
    // Decrypt
    std::vector<float> decrypted = enc2.decrypt();
    
    // Verify
    ASSERT_EQ(decrypted.size(), original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_FLOAT_EQ(decrypted[i], original[i]);
    }
}

// Test 7: Error handling - missing encryption instance
TEST_F(VectorEncryptionPhase1Test, ErrorMissingEncryption) {
    // Clear global encryption instance
    EncryptedField<std::vector<float>>::setFieldEncryption(nullptr);
    
    std::vector<float> vec = {1.0f, 2.0f, 3.0f};
    EncryptedField<std::vector<float>> field;
    
    EXPECT_THROW({
        field.encrypt(vec, "vector_embeddings");
    }, std::runtime_error);
    
    // Restore for other tests
    EncryptedField<std::vector<float>>::setFieldEncryption(field_encryption_);
}

// Test 8: Error handling - decrypt without value
TEST_F(VectorEncryptionPhase1Test, ErrorDecryptEmpty) {
    EncryptedField<std::vector<float>> empty_field;
    
    EXPECT_FALSE(empty_field.hasValue());
    EXPECT_THROW({
        empty_field.decrypt();
    }, std::runtime_error);
}

// Test 9: Error handling - invalid serialization
TEST_F(VectorEncryptionPhase1Test, ErrorInvalidSerialization) {
    // Create a valid encrypted field first
    std::vector<float> original = {1.0f, 2.0f, 3.0f};
    EncryptedField<std::vector<float>> valid_field;
    valid_field.encrypt(original, "vector_embeddings");
    
    // Get the encrypted blob
    auto blob = valid_field.getBlob();
    
    // Corrupt the ciphertext (truncate it)
    blob.ciphertext.resize(2);  // Too short
    
    EncryptedField<std::vector<float>> corrupted_field(blob);
    
    // Decryption should fail
    EXPECT_THROW({
        corrupted_field.decrypt();
    }, DecryptionException);
}

// Test 10: Key versioning
TEST_F(VectorEncryptionPhase1Test, KeyVersioning) {
    std::vector<float> vec = {1.0f, 2.0f, 3.0f};
    
    // Encrypt with version 1
    EncryptedField<std::vector<float>> enc_v1;
    enc_v1.encrypt(vec, "vector_embeddings");
    
    auto blob_v1 = enc_v1.getBlob();
    EXPECT_EQ(blob_v1.key_id, "vector_embeddings");
    EXPECT_EQ(blob_v1.key_version, 1);
    
    // Create version 2 of the key
    key_provider_->createKey("vector_embeddings", 2);
    
    // Encrypt with version 2 (should use latest)
    EncryptedField<std::vector<float>> enc_v2;
    enc_v2.encrypt(vec, "vector_embeddings");
    
    auto blob_v2 = enc_v2.getBlob();
    EXPECT_EQ(blob_v2.key_version, 2);
    
    // Both should decrypt correctly
    auto dec_v1 = enc_v1.decrypt();
    auto dec_v2 = enc_v2.decrypt();
    
    EXPECT_EQ(dec_v1, vec);
    EXPECT_EQ(dec_v2, vec);
}

// Test 11: Multiple encryptions produce different ciphertexts (IV randomness)
TEST_F(VectorEncryptionPhase1Test, IVRandomness) {
    std::vector<float> vec = {1.0f, 2.0f, 3.0f};
    
    EncryptedField<std::vector<float>> enc1;
    enc1.encrypt(vec, "vector_embeddings");
    
    EncryptedField<std::vector<float>> enc2;
    enc2.encrypt(vec, "vector_embeddings");
    
    // IVs should be different
    EXPECT_NE(enc1.getBlob().iv, enc2.getBlob().iv);
    
    // Ciphertexts should be different (due to different IVs)
    EXPECT_NE(enc1.getBlob().ciphertext, enc2.getBlob().ciphertext);
    
    // But both should decrypt to the same value
    EXPECT_EQ(enc1.decrypt(), enc2.decrypt());
}

// Test 12: Performance benchmark
TEST_F(VectorEncryptionPhase1Test, PerformanceBenchmark) {
    const size_t num_vectors = 1000;
    const size_t dimension = 768;
    
    // Prepare test vectors
    std::vector<std::vector<float>> test_vectors(num_vectors);
    for (size_t i = 0; i < num_vectors; ++i) {
        test_vectors[i].resize(dimension);
        for (size_t j = 0; j < dimension; ++j) {
            test_vectors[i][j] = static_cast<float>(i + j) / 1000.0f;
        }
    }
    
    // Benchmark encryption
    auto start_enc = std::chrono::steady_clock::now();
    
    std::vector<EncryptedField<std::vector<float>>> encrypted_fields(num_vectors);
    for (size_t i = 0; i < num_vectors; ++i) {
        encrypted_fields[i].encrypt(test_vectors[i], "vector_embeddings");
    }
    
    auto end_enc = std::chrono::steady_clock::now();
    auto enc_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_enc - start_enc);
    
    // Benchmark decryption
    auto start_dec = std::chrono::steady_clock::now();
    
    std::vector<std::vector<float>> decrypted_vectors(num_vectors);
    for (size_t i = 0; i < num_vectors; ++i) {
        decrypted_vectors[i] = encrypted_fields[i].decrypt();
    }
    
    auto end_dec = std::chrono::steady_clock::now();
    auto dec_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_dec - start_dec);
    
    // Report results
    double enc_per_vector = static_cast<double>(enc_duration.count()) / num_vectors;
    double dec_per_vector = static_cast<double>(dec_duration.count()) / num_vectors;
    
    std::cout << "\n=== Vector Encryption Performance (768-dim) ===" << std::endl;
    std::cout << "Encryption: " << enc_duration.count() << " ms for " 
              << num_vectors << " vectors" << std::endl;
    std::cout << "Per-vector: " << enc_per_vector << " ms" << std::endl;
    std::cout << "Throughput: " << (num_vectors * 1000.0 / enc_duration.count()) 
              << " vectors/sec" << std::endl;
    std::cout << "\nDecryption: " << dec_duration.count() << " ms for " 
              << num_vectors << " vectors" << std::endl;
    std::cout << "Per-vector: " << dec_per_vector << " ms" << std::endl;
    std::cout << "Throughput: " << (num_vectors * 1000.0 / dec_duration.count()) 
              << " vectors/sec" << std::endl;
    
    // Acceptance criteria: < 1ms per vector
    EXPECT_LT(enc_per_vector, 1.0) << "Encryption too slow";
    EXPECT_LT(dec_per_vector, 1.0) << "Decryption too slow";
    
    // Verify correctness
    for (size_t i = 0; i < num_vectors; ++i) {
        ASSERT_EQ(decrypted_vectors[i].size(), dimension);
        for (size_t j = 0; j < dimension; ++j) {
            EXPECT_FLOAT_EQ(decrypted_vectors[i][j], test_vectors[i][j]);
        }
    }
}

// Test 13: Normalized vectors (common in ML)
TEST_F(VectorEncryptionPhase1Test, NormalizedVectors) {
    // Create L2-normalized vector
    std::vector<float> vec = {0.6f, 0.8f, 0.0f};  // L2 norm = 1.0
    
    float norm = std::sqrt(vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
    EXPECT_FLOAT_EQ(norm, 1.0f);
    
    EncryptedField<std::vector<float>> encrypted_field;
    encrypted_field.encrypt(vec, "vector_embeddings");
    
    std::vector<float> decrypted = encrypted_field.decrypt();
    
    // Verify normalization is preserved
    float dec_norm = std::sqrt(
        decrypted[0]*decrypted[0] + 
        decrypted[1]*decrypted[1] + 
        decrypted[2]*decrypted[2]
    );
    EXPECT_FLOAT_EQ(dec_norm, 1.0f);
}

// Test 14: Sparse vectors (many zeros)
TEST_F(VectorEncryptionPhase1Test, SparseVectors) {
    std::vector<float> sparse(1000, 0.0f);
    sparse[10] = 1.0f;
    sparse[100] = 2.0f;
    sparse[500] = 3.0f;
    
    EncryptedField<std::vector<float>> encrypted_field;
    encrypted_field.encrypt(sparse, "vector_embeddings");
    
    std::vector<float> decrypted = encrypted_field.decrypt();
    
    ASSERT_EQ(decrypted.size(), 1000);
    for (size_t i = 0; i < 1000; ++i) {
        EXPECT_FLOAT_EQ(decrypted[i], sparse[i]);
    }
}

// Test 15: Single-precision edge cases
TEST_F(VectorEncryptionPhase1Test, FloatEdgeCases) {
    std::vector<float> edge_cases = {
        0.0f,
        -0.0f,
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN()
    };
    
    EncryptedField<std::vector<float>> encrypted_field;
    encrypted_field.encrypt(edge_cases, "vector_embeddings");
    
    std::vector<float> decrypted = encrypted_field.decrypt();
    
    ASSERT_EQ(decrypted.size(), edge_cases.size());
    EXPECT_FLOAT_EQ(decrypted[0], 0.0f);
    EXPECT_FLOAT_EQ(decrypted[1], -0.0f);
    EXPECT_TRUE(std::isinf(decrypted[2]) && decrypted[2] > 0);
    EXPECT_TRUE(std::isinf(decrypted[3]) && decrypted[3] < 0);
    EXPECT_TRUE(std::isnan(decrypted[4]));
}
