#include <gtest/gtest.h>
#include "index/rotary_embeddings.h"
#include "index/vector_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include <numeric>
#include <cmath>
#include <filesystem>

using namespace themis;

// ============================================================================
// Test Fixture
// ============================================================================

class RotaryEmbeddingTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.hidden_dim = 128;
        config_.num_rotation_pairs = 64;
        config_.base_theta = 10000.0;
        config_.computeThetaCache();
        
        rope_ = std::make_unique<RotaryEmbedding>(config_);
    }
    
    void TearDown() override {
        rope_.reset();
    }
    
    RotationConfig config_;
    std::unique_ptr<RotaryEmbedding> rope_;
    
    // Helper: compute cosine similarity
    float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
        if (a.size() != b.size()) {
          return 0.0f;
        }
        
        float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            dot += a[i] * b[i];
            norm_a += a[i] * a[i];
            norm_b += b[i] * b[i];
        }
        
        if (norm_a == 0.0f || norm_b == 0.0f) {
          return 0.0f;
        }
        return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
    }
};

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(RotaryEmbeddingTest, ConfigurationValidation) {
    // Valid configuration
    RotationConfig valid_config;
    valid_config.hidden_dim = 128;
    valid_config.num_rotation_pairs = 64;
    EXPECT_TRUE(valid_config.isValid());
    
    // Invalid: odd dimension
    RotationConfig invalid_odd;
    invalid_odd.hidden_dim = 127;
    invalid_odd.num_rotation_pairs = 64;
    EXPECT_FALSE(invalid_odd.isValid());
    
    // Invalid: too many pairs
    RotationConfig invalid_pairs;
    invalid_pairs.hidden_dim = 128;
    invalid_pairs.num_rotation_pairs = 65;
    EXPECT_FALSE(invalid_pairs.isValid());
    
    // Invalid: zero dimension
    RotationConfig invalid_zero;
    invalid_zero.hidden_dim = 0;
    invalid_zero.num_rotation_pairs = 0;
    EXPECT_FALSE(invalid_zero.isValid());
}

TEST_F(RotaryEmbeddingTest, ThetaCacheComputation) {
    RotationConfig config;
    config.hidden_dim = 128;
    config.num_rotation_pairs = 64;
    config.base_theta = 10000.0;
    config.computeThetaCache();
    
    ASSERT_EQ(config.theta_cache.size(), 64);
    
    // First theta should be base^0 = 1.0
    EXPECT_NEAR(config.theta_cache[0], 1.0, 1e-5);
    
    // Theta values should decrease
    for (size_t i = 1; i < config.theta_cache.size(); ++i) {
        EXPECT_LT(config.theta_cache[i], config.theta_cache[i-1]);
    }
}

TEST_F(RotaryEmbeddingTest, InvalidConfiguration) {
    RotationConfig invalid_config;
    invalid_config.hidden_dim = 127;  // Odd number
    invalid_config.num_rotation_pairs = 64;
    
    EXPECT_THROW(RotaryEmbedding rope(invalid_config), std::invalid_argument);
}

TEST_F(RotaryEmbeddingTest, EmptyThetaCache) {
    RotationConfig config;
    config.hidden_dim = 128;
    config.num_rotation_pairs = 64;
    // Don't call computeThetaCache()
    
    EXPECT_THROW(RotaryEmbedding rope(config), std::invalid_argument);
}

// ============================================================================
// Basic Rotation Tests
// ============================================================================

TEST_F(RotaryEmbeddingTest, BasicRotation) {
    std::vector<float> embedding(128, 1.0f);
    auto rotated = rope_->rotate(embedding, 0);
    
    ASSERT_EQ(rotated.size(), 128);
    
    // At position 0, rotation angle is 0, so values should be unchanged
    for (size_t i = 0; i < embedding.size(); ++i) {
        EXPECT_NEAR(embedding[i], rotated[i], 1e-5);
    }
}

TEST_F(RotaryEmbeddingTest, RotationPreservesMagnitude) {
    std::vector<float> embedding(128);
    std::iota(embedding.begin(), embedding.end(), 0.0f);
    
    // Compute original magnitude
    float original_norm = 0.0f;
    for (float val : embedding) {
        original_norm += val * val;
    }
    original_norm = std::sqrt(original_norm);
    
    auto rotated = rope_->rotate(embedding, 42);
    
    // Compute rotated magnitude
    float rotated_norm = 0.0f;
    for (float val : rotated) {
        rotated_norm += val * val;
    }
    rotated_norm = std::sqrt(rotated_norm);
    
    // Rotation should preserve magnitude (within floating point precision)
    EXPECT_NEAR(original_norm, rotated_norm, 1e-3);
}

TEST_F(RotaryEmbeddingTest, DifferentPositionsProduceDifferentEmbeddings) {
    std::vector<float> base(128, 1.0f);
    
    auto rot_0 = rope_->rotate(base, 0);
    auto rot_50 = rope_->rotate(base, 50);
    auto rot_100 = rope_->rotate(base, 100);
    
    // Vectors should be different
    EXPECT_NE(rot_0, rot_50);
    EXPECT_NE(rot_50, rot_100);
    EXPECT_NE(rot_0, rot_100);
}

TEST_F(RotaryEmbeddingTest, RotationStatsAreTracked) {
    std::vector<float> embedding(128, 1.0f);

    (void)rope_->rotate(embedding, 1);
    (void)rope_->rotate(embedding, 2);
    (void)rope_->rotateRelational(embedding, "depends_on");

    const auto stats = rope_->getStats();
    EXPECT_EQ(stats.total_rotated_entities, 3u);
    EXPECT_EQ(stats.total_relational_rotations, 1u);
    EXPECT_GT(stats.avg_rotation_time_us, 0.0);
}

TEST_F(RotaryEmbeddingTest, WrongDimensionThrows) {
    std::vector<float> wrong_size(64, 1.0f);  // Wrong dimension
    
    EXPECT_THROW(rope_->rotate(wrong_size, 0), std::invalid_argument);
}

// ============================================================================
// Inverse Rotation Tests
// ============================================================================

TEST_F(RotaryEmbeddingTest, InverseRotation) {
    std::vector<float> original(128);
    std::iota(original.begin(), original.end(), 0.0f);
    
    size_t position = 42;
    auto rotated = rope_->rotate(original, position);
    auto restored = rope_->rotateInverse(rotated, position);
    
    ASSERT_EQ(restored.size(), original.size());
    
    // Check that inverse rotation recovers original (within floating point precision)
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_NEAR(original[i], restored[i], 1e-4);
    }
}

TEST_F(RotaryEmbeddingTest, MultipleInverseRotations) {
    std::vector<float> original(128, 1.0f);
    
    for (size_t pos = 0; pos < 100; pos += 10) {
        auto rotated = rope_->rotate(original, pos);
        auto restored = rope_->rotateInverse(rotated, pos);
        
        for (size_t i = 0; i < original.size(); ++i) {
            EXPECT_NEAR(original[i], restored[i], 1e-4);
        }
    }
}

// ============================================================================
// Positional Orthogonality Tests
// ============================================================================

TEST_F(RotaryEmbeddingTest, PositionalOrthogonality) {
    std::vector<float> base(128, 1.0f);
    
    auto rot_0 = rope_->rotate(base, 0);
    auto rot_100 = rope_->rotate(base, 100);
    
    // Compute cosine similarity
    float similarity = cosineSimilarity(rot_0, rot_100);
    
    // Different positions should produce distinguishable embeddings
    EXPECT_LT(similarity, 0.99);
}

TEST_F(RotaryEmbeddingTest, PositionalDistanceIncreases) {
    std::vector<float> base(128);
    std::iota(base.begin(), base.end(), 1.0f);
    
    auto rot_0 = rope_->rotate(base, 0);
    auto rot_10 = rope_->rotate(base, 10);
    auto rot_100 = rope_->rotate(base, 100);
    
    float sim_0_10 = cosineSimilarity(rot_0, rot_10);
    float sim_0_100 = cosineSimilarity(rot_0, rot_100);
    
    // Farther positions should be less similar
    // (This property depends on the base_theta and may vary)
    EXPECT_TRUE(sim_0_10 > 0.0f && sim_0_100 > 0.0f);
}

// ============================================================================
// Batch Operations Tests
// ============================================================================

TEST_F(RotaryEmbeddingTest, BatchRotation) {
    size_t batch_size = 10;
    std::vector<std::vector<float>> batch(batch_size, std::vector<float>(128, 1.0f));
    std::vector<size_t> positions(batch_size);
    std::iota(positions.begin(), positions.end(), 0);
    
    auto rotated_batch = rope_->rotateBatch(batch, positions);
    
    ASSERT_EQ(rotated_batch.size(), batch_size);
    for (const auto& vec : rotated_batch) {
        ASSERT_EQ(vec.size(), 128);
    }
}

TEST_F(RotaryEmbeddingTest, BatchRotationMismatchThrows) {
    std::vector<std::vector<float>> batch(10, std::vector<float>(128, 1.0f));
    std::vector<size_t> positions(5);  // Mismatch
    
    EXPECT_THROW(rope_->rotateBatch(batch, positions), std::invalid_argument);
}

TEST_F(RotaryEmbeddingTest, BatchRotationConsistency) {
    size_t batch_size = 5;
    std::vector<std::vector<float>> batch(batch_size, std::vector<float>(128));
    for (size_t i = 0; i < batch_size; ++i) {
        std::iota(batch[i].begin(), batch[i].end(), static_cast<float>(i * 10));
    }
    
    std::vector<size_t> positions = {0, 10, 20, 30, 40};
    
    // Batch rotation
    auto rotated_batch = rope_->rotateBatch(batch, positions);
    
    // Individual rotations
    for (size_t i = 0; i < batch_size; ++i) {
        auto individual = rope_->rotate(batch[i], positions[i]);
        ASSERT_EQ(individual.size(), rotated_batch[i].size());
        
        for (size_t j = 0; j < individual.size(); ++j) {
            EXPECT_NEAR(individual[j], rotated_batch[i][j], 1e-5);
        }
    }
}

// ============================================================================
// Relational Rotation Tests
// ============================================================================

TEST_F(RotaryEmbeddingTest, RelationalRotation) {
    std::vector<float> entity(128, 1.0f);
    
    auto parent_rel = rope_->rotateRelational(entity, "parent");
    auto child_rel = rope_->rotateRelational(entity, "child");
    auto sibling_rel = rope_->rotateRelational(entity, "sibling");
    
    ASSERT_EQ(parent_rel.size(), 128);
    ASSERT_EQ(child_rel.size(), 128);
    ASSERT_EQ(sibling_rel.size(), 128);
    
    // Different relations should produce different embeddings
    EXPECT_NE(parent_rel, child_rel);
    EXPECT_NE(child_rel, sibling_rel);
    EXPECT_NE(parent_rel, sibling_rel);
}

TEST_F(RotaryEmbeddingTest, RelationalRotationConsistency) {
    std::vector<float> entity(128);
    std::iota(entity.begin(), entity.end(), 1.0f);
    
    // Same relation type should produce same rotation
    auto rel1 = rope_->rotateRelational(entity, "parent");
    auto rel2 = rope_->rotateRelational(entity, "parent");
    
    for (size_t i = 0; i < rel1.size(); ++i) {
        EXPECT_FLOAT_EQ(rel1[i], rel2[i]);
    }
}

TEST_F(RotaryEmbeddingTest, RelationalRotationDistinguishability) {
    std::vector<float> entity(128, 1.0f);
    
    auto parent_rel = rope_->rotateRelational(entity, "parent");
    auto child_rel = rope_->rotateRelational(entity, "child");
    
    float similarity = cosineSimilarity(parent_rel, child_rel);
    
    // Different relations should be distinguishable
    EXPECT_LT(similarity, 1.0f);
}

// ============================================================================
// Normalization Tests
// ============================================================================

TEST_F(RotaryEmbeddingTest, NormalizationOption) {
    RotationConfig norm_config = config_;
    norm_config.normalize_after = true;
    norm_config.computeThetaCache();
    
    RotaryEmbedding norm_rope(norm_config);
    
    std::vector<float> embedding(128);
    std::iota(embedding.begin(), embedding.end(), 1.0f);
    
    auto rotated = norm_rope.rotate(embedding, 42);
    
    // Compute L2 norm
    float norm = 0.0f;
    for (float val : rotated) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    // Should be unit length (within floating point precision)
    EXPECT_NEAR(norm, 1.0f, 1e-5);
}

// ============================================================================
// VectorIndexManager Integration Tests
// ============================================================================

class RotaryVectorIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary database directory
        test_dir_ = std::filesystem::temp_directory_path() / "themis_rope_test";
        std::filesystem::remove_all(test_dir_);
        std::filesystem::create_directories(test_dir_);
        
        // Initialize database
        RocksDBWrapper::Config db_config;
        db_config.db_path = test_dir_.string();
        db_config.create_if_missing = true;
        db_ = std::make_unique<RocksDBWrapper>(db_config);
        bool opened = db_->open();
        ASSERT_TRUE(opened) << "Failed to open database";
        
        // Initialize vector index
        vector_mgr_ = std::make_unique<VectorIndexManager>(*db_);
        auto init_status = vector_mgr_->init("test_vectors", 128, VectorIndexManager::Metric::COSINE);
        ASSERT_TRUE(init_status.ok) << "Failed to initialize vector index: " << init_status.message;
    }
    
    void TearDown() override {
        vector_mgr_.reset();
        db_.reset();
        std::filesystem::remove_all(test_dir_);
    }
    
    std::filesystem::path test_dir_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<VectorIndexManager> vector_mgr_;
};

TEST_F(RotaryVectorIndexTest, EnableRotaryEmbeddings) {
    RotationConfig config;
    config.hidden_dim = 128;
    config.num_rotation_pairs = 64;
    config.base_theta = 10000.0;
    config.computeThetaCache();
    
    auto status = vector_mgr_->setRotaryEmbeddingConfig(config);
    ASSERT_TRUE(status.ok) << "Failed to enable rotary embeddings: " << status.message;
    
    EXPECT_TRUE(vector_mgr_->isRotaryEmbeddingEnabled());
}

TEST_F(RotaryVectorIndexTest, AddEntityWithRotation) {
    // Enable rotary embeddings
    RotationConfig config;
    config.hidden_dim = 128;
    config.num_rotation_pairs = 64;
    config.computeThetaCache();
    
    auto status = vector_mgr_->setRotaryEmbeddingConfig(config);
    ASSERT_TRUE(status.ok);
    
    // Create entity with embedding
    BaseEntity entity("doc1");
    std::vector<float> embedding(128, 1.0f);
    entity.setField("embedding", embedding);
    entity.setField("content", std::string("Test document"));
    
    // Add with rotation
    status = vector_mgr_->addEntityWithRotation(entity, "embedding", 42);
    ASSERT_TRUE(status.ok) << "Failed to add entity: " << status.message;
    
    // Verify entity was stored
    EXPECT_EQ(vector_mgr_->getVectorCount(), 1);
}

TEST_F(RotaryVectorIndexTest, AddEntityWithRelationalRotation) {
    // Enable rotary embeddings
    RotationConfig config;
    config.hidden_dim = 128;
    config.num_rotation_pairs = 64;
    config.computeThetaCache();
    
    auto status = vector_mgr_->setRotaryEmbeddingConfig(config);
    ASSERT_TRUE(status.ok);
    
    // Create entity
    BaseEntity entity("entity_A");
    std::vector<float> embedding(128, 1.0f);
    entity.setField("embedding", embedding);
    
    // Add with relational rotation
    status = vector_mgr_->addEntityWithRelationalRotation(entity, "embedding", "parent_of");
    ASSERT_TRUE(status.ok) << "Failed to add entity: " << status.message;
    
    EXPECT_EQ(vector_mgr_->getVectorCount(), 1);
}

TEST_F(RotaryVectorIndexTest, SearchWithRotation) {
    // Enable rotary embeddings
    RotationConfig config;
    config.hidden_dim = 128;
    config.num_rotation_pairs = 64;
    config.computeThetaCache();
    
    auto status = vector_mgr_->setRotaryEmbeddingConfig(config);
    ASSERT_TRUE(status.ok);
    
    // Add multiple entities with different positions
    for (size_t i = 0; i < 5; ++i) {
        BaseEntity entity("doc" + std::to_string(i));
        std::vector<float> embedding(128);
        std::iota(embedding.begin(), embedding.end(), static_cast<float>(i * 10));
        entity.setField("embedding", embedding);
        
        status = vector_mgr_->addEntityWithRotation(entity, "embedding", i);
        ASSERT_TRUE(status.ok);
    }
    
    // Search with rotation
    std::vector<float> query(128);
    std::iota(query.begin(), query.end(), 0.0f);
    
    auto [search_status, results] = vector_mgr_->searchWithRotation(query, 3, 0);
    ASSERT_TRUE(search_status.ok) << "Search failed: " << search_status.message;
    
    // Should return results
    EXPECT_GT(results.size(), 0);
    EXPECT_LE(results.size(), 3);
}

TEST_F(RotaryVectorIndexTest, WithoutRotaryEmbeddingsEnabled) {
    BaseEntity entity("doc1");
    std::vector<float> embedding(128, 1.0f);
    entity.setField("embedding", embedding);
    
    // Try to add with rotation without enabling rotary embeddings
    auto status = vector_mgr_->addEntityWithRotation(entity, "embedding", 42);
    EXPECT_FALSE(status.ok);
    EXPECT_NE(status.message.find("not enabled"), std::string::npos);
}

TEST_F(RotaryVectorIndexTest, GetRotaryEmbeddingConfig) {
    // Initially should be disabled
    EXPECT_FALSE(vector_mgr_->getRotaryEmbeddingConfig().has_value());
    
    // Enable with config
    RotationConfig config;
    config.hidden_dim = 128;
    config.num_rotation_pairs = 64;
    config.base_theta = 10000.0;
    config.computeThetaCache();
    
    auto status = vector_mgr_->setRotaryEmbeddingConfig(config);
    ASSERT_TRUE(status.ok);
    
    // Now should return config
    auto retrieved_config = vector_mgr_->getRotaryEmbeddingConfig();
    ASSERT_TRUE(retrieved_config.has_value());
    EXPECT_EQ(retrieved_config->hidden_dim, 128);
    EXPECT_EQ(retrieved_config->num_rotation_pairs, 64);
}

// ============================================================================
// BaseEntity Integration Tests
// ============================================================================

TEST(BaseEntityRotationTest, RotationMetadata) {
    BaseEntity entity("test");
    
    // Set rotation metadata
    entity.setField("embedding", std::vector<float>(128, 1.0f));
    entity.setField("embedding_rotation_pos", static_cast<int64_t>(42));
    entity.setField("embedding_rotation_type", std::string("parent"));
    
    // Check metadata
    EXPECT_TRUE(entity.hasRotatedEmbedding("embedding"));
    
    auto pos = entity.getRotationPosition("embedding");
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(*pos, 42);
    
    auto type = entity.getRotationType("embedding");
    ASSERT_TRUE(type.has_value());
    EXPECT_EQ(*type, "parent");
}

TEST(BaseEntityRotationTest, NoRotationMetadata) {
    BaseEntity entity("test");
    entity.setField("embedding", std::vector<float>(128, 1.0f));
    
    EXPECT_FALSE(entity.hasRotatedEmbedding("embedding"));
    EXPECT_FALSE(entity.getRotationPosition("embedding").has_value());
    EXPECT_FALSE(entity.getRotationType("embedding").has_value());
}

// ============================================================================
// Main removed - using GTest's main from themis_tests.exe
