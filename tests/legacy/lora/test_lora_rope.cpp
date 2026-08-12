#include <gtest/gtest.h>
#include "index/lora_rope.h"
#include <numeric>
#include <algorithm>
#include <cmath>
#include <thread>

using namespace themis;

// ============================================================================
// Test Fixture
// ============================================================================

class LoRARopeTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.hidden_dim = 128;
        config_.num_rotation_pairs = 64;
        config_.base_theta = 10000.0;
        config_.computeThetaCache();
        
        registry_ = std::make_shared<LoRARopeAdapterRegistry>();
        lora_rope_ = std::make_unique<LoRARotaryEmbedding>(config_, registry_);
    }
    
    void TearDown() override {
        lora_rope_.reset();
        registry_.reset();
    }
    
    RotationConfig config_;
    std::shared_ptr<LoRARopeAdapterRegistry> registry_;
    std::unique_ptr<LoRARotaryEmbedding> lora_rope_;
    
    // Helper: compute cosine similarity
    float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
        if (a.size() != b.size()) return 0.0f;
        
        float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            dot += a[i] * b[i];
            norm_a += a[i] * a[i];
            norm_b += b[i] * b[i];
        }
        
        if (norm_a == 0.0f || norm_b == 0.0f) return 0.0f;
        return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
    }
    
    // Helper: create test embedding
    std::vector<float> createTestEmbedding() {
        std::vector<float> embedding(128);
        std::iota(embedding.begin(), embedding.end(), 1.0f);
        
        // Normalize
        float norm = 0.0f;
        for (auto v : embedding) norm += v * v;
        norm = std::sqrt(norm);
        for (auto& v : embedding) v /= norm;
        
        return embedding;
    }
};

// ============================================================================
// LoRARopeAdapter Tests
// ============================================================================

TEST_F(LoRARopeTest, AdapterCreateRandom) {
    auto adapter = LoRARopeAdapter::createRandom("test", "general", 64, 8, 1.0f);
    
    EXPECT_EQ(adapter.name, "test");
    EXPECT_EQ(adapter.domain, "general");
    EXPECT_EQ(adapter.rank, 8);
    EXPECT_EQ(adapter.alpha, 1.0f);
    EXPECT_TRUE(adapter.enabled);
    EXPECT_EQ(adapter.scaling, 1.0f);
    
    // Check dimensions
    EXPECT_EQ(adapter.matrix_B.size(), 64);
    EXPECT_EQ(adapter.matrix_A.size(), 8);
    
    for (const auto& row : adapter.matrix_B) {
        EXPECT_EQ(row.size(), 8);
    }
    
    for (const auto& row : adapter.matrix_A) {
        EXPECT_EQ(row.size(), 64);
    }
    
    // Check validation
    EXPECT_TRUE(adapter.isValid(64));
}

TEST_F(LoRARopeTest, AdapterCreateZero) {
    auto adapter = LoRARopeAdapter::createZero("zero", "test", 64, 8, 2.0f);
    
    EXPECT_EQ(adapter.name, "zero");
    EXPECT_EQ(adapter.rank, 8);
    EXPECT_EQ(adapter.alpha, 2.0f);
    
    // Check that all values are zero
    for (const auto& row : adapter.matrix_B) {
        for (auto val : row) {
            EXPECT_EQ(val, 0.0);
        }
    }
    
    for (const auto& row : adapter.matrix_A) {
        for (auto val : row) {
            EXPECT_EQ(val, 0.0);
        }
    }
    
    EXPECT_TRUE(adapter.isValid(64));
}

TEST_F(LoRARopeTest, AdapterValidation) {
    auto adapter = LoRARopeAdapter::createRandom("test", "general", 64, 8);
    
    // Valid for correct size
    EXPECT_TRUE(adapter.isValid(64));
    
    // Invalid for wrong size
    EXPECT_FALSE(adapter.isValid(32));
    EXPECT_FALSE(adapter.isValid(128));
    
    // Invalid if B has wrong dimensions
    adapter.matrix_B.resize(32);
    EXPECT_FALSE(adapter.isValid(64));
}

// ============================================================================
// LoRARopeAdapterRegistry Tests
// ============================================================================

TEST_F(LoRARopeTest, RegistryRegisterAdapter) {
    auto adapter1 = LoRARopeAdapter::createZero("medical", "medical", 64, 8);
    auto adapter2 = LoRARopeAdapter::createZero("legal", "legal", 64, 8);
    
    EXPECT_TRUE(registry_->registerAdapter(adapter1));
    EXPECT_TRUE(registry_->registerAdapter(adapter2));
    
    // Cannot register same name twice
    EXPECT_FALSE(registry_->registerAdapter(adapter1));
    
    EXPECT_EQ(registry_->size(), 2);
}

TEST_F(LoRARopeTest, RegistryGetAdapter) {
    auto adapter = LoRARopeAdapter::createZero("test", "test", 64, 8);
    registry_->registerAdapter(adapter);
    
    auto retrieved = registry_->getAdapter("test");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "test");
    
    auto not_found = registry_->getAdapter("nonexistent");
    EXPECT_FALSE(not_found.has_value());
}

TEST_F(LoRARopeTest, RegistryUnregisterAdapter) {
    auto adapter = LoRARopeAdapter::createZero("test", "test", 64, 8);
    registry_->registerAdapter(adapter);
    
    EXPECT_TRUE(registry_->hasAdapter("test"));
    EXPECT_TRUE(registry_->unregisterAdapter("test"));
    EXPECT_FALSE(registry_->hasAdapter("test"));
    
    // Cannot unregister twice
    EXPECT_FALSE(registry_->unregisterAdapter("test"));
}

TEST_F(LoRARopeTest, RegistryListAdapters) {
    registry_->registerAdapter(LoRARopeAdapter::createZero("adapter1", "test", 64, 8));
    registry_->registerAdapter(LoRARopeAdapter::createZero("adapter2", "test", 64, 8));
    registry_->registerAdapter(LoRARopeAdapter::createZero("adapter3", "test", 64, 8));
    
    auto names = registry_->listAdapters();
    EXPECT_EQ(names.size(), 3);
    
    // Check all names are present
    std::sort(names.begin(), names.end());
    EXPECT_EQ(names[0], "adapter1");
    EXPECT_EQ(names[1], "adapter2");
    EXPECT_EQ(names[2], "adapter3");
}

TEST_F(LoRARopeTest, RegistryEnableDisable) {
    auto adapter = LoRARopeAdapter::createZero("test", "test", 64, 8);
    registry_->registerAdapter(adapter);
    
    EXPECT_TRUE(registry_->setAdapterEnabled("test", false));
    auto retrieved = registry_->getAdapter("test");
    EXPECT_FALSE(retrieved->enabled);
    
    EXPECT_TRUE(registry_->setAdapterEnabled("test", true));
    retrieved = registry_->getAdapter("test");
    EXPECT_TRUE(retrieved->enabled);
}

TEST_F(LoRARopeTest, RegistryClear) {
    registry_->registerAdapter(LoRARopeAdapter::createZero("adapter1", "test", 64, 8));
    registry_->registerAdapter(LoRARopeAdapter::createZero("adapter2", "test", 64, 8));
    
    EXPECT_EQ(registry_->size(), 2);
    registry_->clear();
    EXPECT_EQ(registry_->size(), 0);
}

// ============================================================================
// LoRARotaryEmbedding Basic Tests
// ============================================================================

TEST_F(LoRARopeTest, Initialization) {
    EXPECT_NE(lora_rope_, nullptr);
    EXPECT_EQ(lora_rope_->getAdapterRegistry(), registry_);
    EXPECT_EQ(lora_rope_->listAdapters().size(), 0);
}

TEST_F(LoRARopeTest, InitializationWithoutRegistry) {
    LoRARotaryEmbedding rope_no_registry(config_);
    EXPECT_NE(rope_no_registry.getAdapterRegistry(), nullptr);
    EXPECT_EQ(rope_no_registry.listAdapters().size(), 0);
}

TEST_F(LoRARopeTest, RegisterAdapterThroughRope) {
    auto adapter = LoRARopeAdapter::createZero("medical", "medical", 64, 8);
    
    EXPECT_TRUE(lora_rope_->registerAdapter("medical", adapter));
    EXPECT_TRUE(lora_rope_->hasAdapter("medical"));
    EXPECT_EQ(lora_rope_->listAdapters().size(), 1);
}

TEST_F(LoRARopeTest, RegisterAdapterWithInvalidDimensions) {
    auto adapter = LoRARopeAdapter::createZero("invalid", "test", 32, 8);  // Wrong size
    
    EXPECT_FALSE(lora_rope_->registerAdapter("invalid", adapter));
    EXPECT_FALSE(lora_rope_->hasAdapter("invalid"));
}

// ============================================================================
// LoRA Rotation Tests
// ============================================================================

TEST_F(LoRARopeTest, RotateWithZeroAdapter) {
    // Register zero adapter (should not modify rotation)
    auto adapter = LoRARopeAdapter::createZero("zero", "test", 64, 8);
    lora_rope_->registerAdapter("zero", adapter);
    
    auto embedding = createTestEmbedding();
    
    // Base rotation
    auto base_rotated = lora_rope_->rotate(embedding, 10);
    
    // LoRA rotation with zero adapter
    auto lora_rotated = lora_rope_->rotateWithAdapter(embedding, 10, "zero");
    
    // Should be very similar (minor differences due to numerical computation)
    float similarity = cosineSimilarity(base_rotated, lora_rotated);
    EXPECT_GT(similarity, 0.99f);
}

TEST_F(LoRARopeTest, RotateWithRandomAdapter) {
    // Register random adapter (should modify rotation)
    auto adapter = LoRARopeAdapter::createRandom("random", "test", 64, 8, 0.1f);
    lora_rope_->registerAdapter("random", adapter);
    
    auto embedding = createTestEmbedding();
    
    // Base rotation
    auto base_rotated = lora_rope_->rotate(embedding, 10);
    
    // LoRA rotation with random adapter
    auto lora_rotated = lora_rope_->rotateWithAdapter(embedding, 10, "random");
    
    // Should be different (but still somewhat similar)
    float similarity = cosineSimilarity(base_rotated, lora_rotated);
    EXPECT_LT(similarity, 0.99f);
    EXPECT_GT(similarity, 0.5f);  // Not completely different
}

TEST_F(LoRARopeTest, RotateWithDisabledAdapter) {
    // Register adapter and disable it
    auto adapter = LoRARopeAdapter::createRandom("disabled", "test", 64, 8, 1.0f);
    lora_rope_->registerAdapter("disabled", adapter);
    lora_rope_->setAdapterEnabled("disabled", false);
    
    auto embedding = createTestEmbedding();
    
    // Base rotation
    auto base_rotated = lora_rope_->rotate(embedding, 10);
    
    // LoRA rotation with disabled adapter (should be same as base)
    auto lora_rotated = lora_rope_->rotateWithAdapter(embedding, 10, "disabled");
    
    // Should be very similar
    float similarity = cosineSimilarity(base_rotated, lora_rotated);
    EXPECT_GT(similarity, 0.99f);
}

TEST_F(LoRARopeTest, RotateWithNonexistentAdapter) {
    auto embedding = createTestEmbedding();
    
    // Should throw exception for nonexistent adapter
    EXPECT_THROW(
        lora_rope_->rotateWithAdapter(embedding, 10, "nonexistent"),
        std::runtime_error
    );
}

TEST_F(LoRARopeTest, RotateBatchWithAdapter) {
    auto adapter = LoRARopeAdapter::createZero("batch", "test", 64, 8);
    lora_rope_->registerAdapter("batch", adapter);
    
    // Create batch of embeddings
    std::vector<std::vector<float>> embeddings;
    std::vector<size_t> positions;
    
    for (size_t i = 0; i < 5; ++i) {
        embeddings.push_back(createTestEmbedding());
        positions.push_back(i * 10);
    }
    
    auto rotated_batch = lora_rope_->rotateBatchWithAdapter(embeddings, positions, "batch");
    
    EXPECT_EQ(rotated_batch.size(), 5);
    
    // Each result should match individual rotation
    for (size_t i = 0; i < 5; ++i) {
        auto individual = lora_rope_->rotateWithAdapter(embeddings[i], positions[i], "batch");
        float similarity = cosineSimilarity(rotated_batch[i], individual);
        EXPECT_GT(similarity, 0.999f);
    }
}

TEST_F(LoRARopeTest, RotateBatchSizeMismatch) {
    auto adapter = LoRARopeAdapter::createZero("batch", "test", 64, 8);
    lora_rope_->registerAdapter("batch", adapter);
    
    std::vector<std::vector<float>> embeddings(3, createTestEmbedding());
    std::vector<size_t> positions(2, 0);  // Different size
    
    EXPECT_THROW(
        lora_rope_->rotateBatchWithAdapter(embeddings, positions, "batch"),
        std::invalid_argument
    );
}

// ============================================================================
// Adapter Blending Tests
// ============================================================================

TEST_F(LoRARopeTest, RotateWithAdapterBlendSingle) {
    auto adapter = LoRARopeAdapter::createRandom("blend", "test", 64, 8, 0.1f);
    lora_rope_->registerAdapter("blend", adapter);
    
    auto embedding = createTestEmbedding();
    
    // Blend with single adapter (weight = 1.0)
    auto blended = lora_rope_->rotateWithAdapterBlend(
        embedding, 10, {"blend"}, {1.0f}
    );
    
    // Should be same as regular adapter rotation
    auto single = lora_rope_->rotateWithAdapter(embedding, 10, "blend");
    
    float similarity = cosineSimilarity(blended, single);
    EXPECT_GT(similarity, 0.99f);
}

TEST_F(LoRARopeTest, RotateWithAdapterBlendMultiple) {
    auto adapter1 = LoRARopeAdapter::createRandom("adapter1", "test", 64, 8, 0.1f);
    auto adapter2 = LoRARopeAdapter::createRandom("adapter2", "test", 64, 8, 0.1f);
    
    lora_rope_->registerAdapter("adapter1", adapter1);
    lora_rope_->registerAdapter("adapter2", adapter2);
    
    auto embedding = createTestEmbedding();
    
    // Blend with equal weights
    auto blended = lora_rope_->rotateWithAdapterBlend(
        embedding, 10, {"adapter1", "adapter2"}, {0.5f, 0.5f}
    );
    
    // Result should be different from both individual adapters
    auto result1 = lora_rope_->rotateWithAdapter(embedding, 10, "adapter1");
    auto result2 = lora_rope_->rotateWithAdapter(embedding, 10, "adapter2");
    
    float sim1 = cosineSimilarity(blended, result1);
    float sim2 = cosineSimilarity(blended, result2);
    
    EXPECT_LT(sim1, 0.99f);  // Different from adapter1
    EXPECT_LT(sim2, 0.99f);  // Different from adapter2
    EXPECT_GT(sim1, 0.5f);   // But still similar
    EXPECT_GT(sim2, 0.5f);
}

TEST_F(LoRARopeTest, RotateWithAdapterBlendEmpty) {
    auto embedding = createTestEmbedding();
    
    // Empty adapters list should return base rotation
    auto blended = lora_rope_->rotateWithAdapterBlend(
        embedding, 10, {}, {}
    );
    
    auto base = lora_rope_->rotate(embedding, 10);
    
    float similarity = cosineSimilarity(blended, base);
    EXPECT_GT(similarity, 0.999f);
}

TEST_F(LoRARopeTest, RotateWithAdapterBlendWeightNormalization) {
    auto adapter1 = LoRARopeAdapter::createRandom("adapter1", "test", 64, 8, 0.1f);
    auto adapter2 = LoRARopeAdapter::createRandom("adapter2", "test", 64, 8, 0.1f);
    
    lora_rope_->registerAdapter("adapter1", adapter1);
    lora_rope_->registerAdapter("adapter2", adapter2);
    
    auto embedding = createTestEmbedding();
    
    // Test with non-normalized weights
    auto blended1 = lora_rope_->rotateWithAdapterBlend(
        embedding, 10, {"adapter1", "adapter2"}, {1.0f, 1.0f}
    );
    
    // Should be same as normalized weights
    auto blended2 = lora_rope_->rotateWithAdapterBlend(
        embedding, 10, {"adapter1", "adapter2"}, {0.5f, 0.5f}
    );
    
    float similarity = cosineSimilarity(blended1, blended2);
    EXPECT_GT(similarity, 0.99f);
}

TEST_F(LoRARopeTest, RotateWithAdapterBlendMismatchSize) {
    auto embedding = createTestEmbedding();
    
    EXPECT_THROW(
        lora_rope_->rotateWithAdapterBlend(
            embedding, 10, {"adapter1", "adapter2"}, {1.0f}  // Size mismatch
        ),
        std::invalid_argument
    );
}

TEST_F(LoRARopeTest, RotateWithAdapterBlendInvalidWeights) {
    auto embedding = createTestEmbedding();
    
    EXPECT_THROW(
        lora_rope_->rotateWithAdapterBlend(
            embedding, 10, {"adapter1"}, {0.0f}  // Zero weight
        ),
        std::invalid_argument
    );
    
    EXPECT_THROW(
        lora_rope_->rotateWithAdapterBlend(
            embedding, 10, {"adapter1"}, {-1.0f}  // Negative weight
        ),
        std::invalid_argument
    );
}

// ============================================================================
// Position Sensitivity Tests
// ============================================================================

TEST_F(LoRARopeTest, DifferentPositionsDifferentResults) {
    auto adapter = LoRARopeAdapter::createRandom("pos", "test", 64, 8, 0.2f);
    lora_rope_->registerAdapter("pos", adapter);
    
    auto embedding = createTestEmbedding();
    
    auto rotated_pos_0 = lora_rope_->rotateWithAdapter(embedding, 0, "pos");
    auto rotated_pos_10 = lora_rope_->rotateWithAdapter(embedding, 10, "pos");
    auto rotated_pos_100 = lora_rope_->rotateWithAdapter(embedding, 100, "pos");
    
    // Different positions should give different results
    float sim_0_10 = cosineSimilarity(rotated_pos_0, rotated_pos_10);
    float sim_0_100 = cosineSimilarity(rotated_pos_0, rotated_pos_100);
    float sim_10_100 = cosineSimilarity(rotated_pos_10, rotated_pos_100);
    
    EXPECT_LT(sim_0_10, 0.99f);
    EXPECT_LT(sim_0_100, 0.99f);
    EXPECT_LT(sim_10_100, 0.99f);
}

// ============================================================================
// Domain-Specific Adapter Tests
// ============================================================================

TEST_F(LoRARopeTest, DomainSpecificAdapters) {
    // Create domain-specific adapters
    auto medical = LoRARopeAdapter::createRandom("medical", "medical", 64, 8, 0.15f);
    auto legal = LoRARopeAdapter::createRandom("legal", "legal", 64, 8, 0.15f);
    auto technical = LoRARopeAdapter::createRandom("technical", "technical", 64, 8, 0.15f);
    
    lora_rope_->registerAdapter("medical", medical);
    lora_rope_->registerAdapter("legal", legal);
    lora_rope_->registerAdapter("technical", technical);
    
    auto embedding = createTestEmbedding();
    
    // Apply different domain adapters
    auto medical_rotated = lora_rope_->rotateWithAdapter(embedding, 10, "medical");
    auto legal_rotated = lora_rope_->rotateWithAdapter(embedding, 10, "legal");
    auto technical_rotated = lora_rope_->rotateWithAdapter(embedding, 10, "technical");
    
    // Different domains should produce different results
    float sim_medical_legal = cosineSimilarity(medical_rotated, legal_rotated);
    float sim_medical_technical = cosineSimilarity(medical_rotated, technical_rotated);
    float sim_legal_technical = cosineSimilarity(legal_rotated, technical_rotated);
    
    EXPECT_LT(sim_medical_legal, 0.99f);
    EXPECT_LT(sim_medical_technical, 0.99f);
    EXPECT_LT(sim_legal_technical, 0.99f);
}

// ============================================================================
// Adapter Scaling Tests
// ============================================================================

TEST_F(LoRARopeTest, AdapterScalingEffect) {
    auto adapter = LoRARopeAdapter::createRandom("scale", "test", 64, 8, 0.5f);
    adapter.scaling = 2.0f;  // Double the scaling
    
    lora_rope_->registerAdapter("scale", adapter);
    
    auto embedding = createTestEmbedding();
    
    auto base_rotated = lora_rope_->rotate(embedding, 10);
    auto lora_rotated = lora_rope_->rotateWithAdapter(embedding, 10, "scale");
    
    // Higher scaling should produce more difference from base
    float similarity = cosineSimilarity(base_rotated, lora_rotated);
    EXPECT_LT(similarity, 0.95f);  // Significant difference with scaling = 2.0
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(LoRARopeTest, ConcurrentAdapterRegistration) {
    const int num_threads = 4;
    const int adapters_per_thread = 10;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, adapters_per_thread]() {
            for (int i = 0; i < adapters_per_thread; ++i) {
                std::string name = "thread_" + std::to_string(t) + "_adapter_" + std::to_string(i);
                auto adapter = LoRARopeAdapter::createZero(name, "test", 64, 8);
                lora_rope_->registerAdapter(name, adapter);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(lora_rope_->listAdapters().size(), num_threads * adapters_per_thread);
}

// ============================================================================
// Main removed - using GTest's main from themis_tests.exe
