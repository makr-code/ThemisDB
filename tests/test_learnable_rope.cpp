#include <gtest/gtest.h>
#include "index/learnable_rope.h"
#include <numeric>
#include <cmath>
#include <filesystem>
#include <fstream>

using namespace themis;

// ============================================================================
// Test Fixture
// ============================================================================

class LearnableRopeTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.hidden_dim = 128;
        config_.num_rotation_pairs = 64;
        config_.base_theta = 10000.0;
        config_.computeThetaCache();
        
        learnable_rope_ = std::make_unique<LearnableRotaryEmbedding>(config_, true);
    }
    
    void TearDown() override {
        learnable_rope_.reset();
    }
    
    RotationConfig config_;
    std::unique_ptr<LearnableRotaryEmbedding> learnable_rope_;
    
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
    
    // Helper: create simple training samples
    std::vector<TrainingSample> createTrainingSamples(size_t count) {
        std::vector<TrainingSample> samples;
        samples.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            std::vector<float> embedding(128);
            std::iota(embedding.begin(), embedding.end(), static_cast<float>(i));
            
            samples.emplace_back(embedding, i, 0.9f);
        }
        
        return samples;
    }
};

// ============================================================================
// Basic Configuration Tests
// ============================================================================

TEST_F(LearnableRopeTest, Initialization) {
    EXPECT_TRUE(learnable_rope_->isTrainable());
    EXPECT_FALSE(learnable_rope_->isTraining());
    
    // Check that learnable theta is initialized from base config
    const auto& learnable_theta = learnable_rope_->getLearnableTheta();
    EXPECT_EQ(learnable_theta.size(), config_.theta_cache.size());
    
    // Should be equal to base theta initially
    for (size_t i = 0; i < learnable_theta.size(); ++i) {
        EXPECT_DOUBLE_EQ(learnable_theta[i], config_.theta_cache[i]);
    }
}

TEST_F(LearnableRopeTest, NonTrainableMode) {
    LearnableRotaryEmbedding non_trainable(config_, false);
    
    EXPECT_FALSE(non_trainable.isTrainable());
    EXPECT_FALSE(non_trainable.isTraining());
}

TEST_F(LearnableRopeTest, TrainingModeSwitch) {
    EXPECT_FALSE(learnable_rope_->isTraining());
    
    learnable_rope_->setTrainingMode(true);
    EXPECT_TRUE(learnable_rope_->isTraining());
    
    learnable_rope_->setTrainingMode(false);
    EXPECT_FALSE(learnable_rope_->isTraining());
}

// ============================================================================
// Parameter Manipulation Tests
// ============================================================================

TEST_F(LearnableRopeTest, SetLearnableTheta) {
    std::vector<double> new_theta(64);
    std::iota(new_theta.begin(), new_theta.end(), 1.0);
    
    learnable_rope_->setLearnableTheta(new_theta);
    
    const auto& theta = learnable_rope_->getLearnableTheta();
    EXPECT_EQ(theta.size(), new_theta.size());
    
    for (size_t i = 0; i < theta.size(); ++i) {
        EXPECT_DOUBLE_EQ(theta[i], new_theta[i]);
    }
}

TEST_F(LearnableRopeTest, SetLearnableThetaSizeMismatch) {
    std::vector<double> wrong_size(32);  // Wrong size
    
    EXPECT_THROW(learnable_rope_->setLearnableTheta(wrong_size), std::invalid_argument);
}

TEST_F(LearnableRopeTest, ResetToBase) {
    // Modify theta
    std::vector<double> modified_theta(64, 5.0);
    learnable_rope_->setLearnableTheta(modified_theta);
    
    // Reset to base
    learnable_rope_->resetToBase();
    
    // Check that it matches base config
    const auto& theta = learnable_rope_->getLearnableTheta();
    for (size_t i = 0; i < theta.size(); ++i) {
        EXPECT_DOUBLE_EQ(theta[i], config_.theta_cache[i]);
    }
}

// ============================================================================
// Gradient Computation Tests
// ============================================================================

TEST_F(LearnableRopeTest, ComputeGradients) {
    std::vector<float> embedding(128, 1.0f);
    float target_similarity = 0.9f;
    size_t position = 10;
    
    auto gradients = learnable_rope_->computeGradients(embedding, target_similarity, position);
    
    EXPECT_EQ(gradients.size(), learnable_rope_->getLearnableTheta().size());
    
    // Gradients should be computed (not all zero)
    bool has_nonzero = false;
    for (double grad : gradients) {
        if (std::abs(grad) > 1e-10) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
}

TEST_F(LearnableRopeTest, ComputeGradientsNonTrainable) {
    LearnableRotaryEmbedding non_trainable(config_, false);
    
    std::vector<float> embedding(128, 1.0f);
    
    EXPECT_THROW(
        non_trainable.computeGradients(embedding, 0.9f, 10),
        std::logic_error
    );
}

// ============================================================================
// Parameter Update Tests
// ============================================================================

TEST_F(LearnableRopeTest, UpdateParameters) {
    // Get initial theta
    auto initial_theta = learnable_rope_->getLearnableTheta();
    
    // Create some gradients
    std::vector<double> gradients(initial_theta.size(), 0.01);
    
    // Update parameters
    learnable_rope_->updateParameters(gradients, 0.1f);
    
    // Check that parameters changed
    const auto& updated_theta = learnable_rope_->getLearnableTheta();
    bool parameters_changed = false;
    for (size_t i = 0; i < initial_theta.size(); ++i) {
        if (std::abs(updated_theta[i] - initial_theta[i]) > 1e-10) {
            parameters_changed = true;
            break;
        }
    }
    EXPECT_TRUE(parameters_changed);
}

TEST_F(LearnableRopeTest, UpdateParametersNonTrainable) {
    LearnableRotaryEmbedding non_trainable(config_, false);
    std::vector<double> gradients(64, 0.01);
    
    EXPECT_THROW(
        non_trainable.updateParameters(gradients, 0.1f),
        std::logic_error
    );
}

TEST_F(LearnableRopeTest, UpdateParametersSizeMismatch) {
    std::vector<double> wrong_size(32, 0.01);  // Wrong size
    
    EXPECT_THROW(
        learnable_rope_->updateParameters(wrong_size, 0.1f),
        std::invalid_argument
    );
}

TEST_F(LearnableRopeTest, ParametersStayPositive) {
    // Large negative gradients should not make theta negative
    std::vector<double> large_gradients(64, 100.0);
    
    learnable_rope_->updateParameters(large_gradients, 1.0f);
    
    const auto& theta = learnable_rope_->getLearnableTheta();
    for (double val : theta) {
        EXPECT_GT(val, 0.0);
    }
}

// ============================================================================
// Training Tests
// ============================================================================

TEST_F(LearnableRopeTest, BasicTraining) {
    auto samples = createTrainingSamples(100);
    
    TrainingConfig train_config;
    train_config.learning_rate = 1e-3f;
    train_config.batch_size = 10;
    train_config.max_epochs = 5;
    train_config.validation_split = 0.0f;  // No validation for this test
    train_config.use_adam = false;
    
    auto loss_history = learnable_rope_->train(samples, train_config);
    
    EXPECT_EQ(loss_history.size(), 5);  // 5 epochs
    
    // Loss should be computed
    for (float loss : loss_history) {
        EXPECT_GE(loss, 0.0f);
    }
}

TEST_F(LearnableRopeTest, TrainingWithValidation) {
    auto samples = createTrainingSamples(100);
    
    TrainingConfig train_config;
    train_config.learning_rate = 1e-3f;
    train_config.batch_size = 10;
    train_config.max_epochs = 5;
    train_config.validation_split = 0.2f;  // 20% validation
    train_config.use_adam = false;
    
    auto loss_history = learnable_rope_->train(samples, train_config);
    
    EXPECT_LE(loss_history.size(), 5);  // May early stop
}

TEST_F(LearnableRopeTest, TrainingWithAdam) {
    auto samples = createTrainingSamples(50);
    
    TrainingConfig train_config;
    train_config.learning_rate = 1e-3f;
    train_config.batch_size = 10;
    train_config.max_epochs = 3;
    train_config.validation_split = 0.0f;
    train_config.use_adam = true;
    
    auto loss_history = learnable_rope_->train(samples, train_config);
    
    EXPECT_EQ(loss_history.size(), 3);
}

TEST_F(LearnableRopeTest, TrainingEmptyDataset) {
    std::vector<TrainingSample> empty_samples;
    TrainingConfig train_config;
    
    EXPECT_THROW(
        learnable_rope_->train(empty_samples, train_config),
        std::invalid_argument
    );
}

TEST_F(LearnableRopeTest, TrainingNonTrainable) {
    LearnableRotaryEmbedding non_trainable(config_, false);
    auto samples = createTrainingSamples(10);
    TrainingConfig train_config;
    
    EXPECT_THROW(
        non_trainable.train(samples, train_config),
        std::logic_error
    );
}

TEST_F(LearnableRopeTest, ParametersChangeAfterTraining) {
    auto initial_theta = learnable_rope_->getLearnableTheta();
    
    auto samples = createTrainingSamples(50);
    
    TrainingConfig train_config;
    train_config.learning_rate = 1e-2f;
    train_config.batch_size = 10;
    train_config.max_epochs = 3;
    train_config.validation_split = 0.0f;
    
    learnable_rope_->train(samples, train_config);
    
    const auto& trained_theta = learnable_rope_->getLearnableTheta();
    
    // Check that at least some parameters changed
    bool parameters_changed = false;
    for (size_t i = 0; i < initial_theta.size(); ++i) {
        if (std::abs(trained_theta[i] - initial_theta[i]) > 1e-6) {
            parameters_changed = true;
            break;
        }
    }
    EXPECT_TRUE(parameters_changed);
}

// ============================================================================
// Validation Loss Tests
// ============================================================================

TEST_F(LearnableRopeTest, ComputeValidationLoss) {
    auto samples = createTrainingSamples(20);
    
    float loss = learnable_rope_->computeValidationLoss(samples);
    
    EXPECT_GE(loss, 0.0f);
    EXPECT_LT(loss, 1000.0f);  // Reasonable range
}

TEST_F(LearnableRopeTest, ValidationLossEmptyDataset) {
    std::vector<TrainingSample> empty_samples;
    
    float loss = learnable_rope_->computeValidationLoss(empty_samples);
    
    EXPECT_FLOAT_EQ(loss, 0.0f);
}

// ============================================================================
// Serialization Tests
// ============================================================================

TEST_F(LearnableRopeTest, SaveAndLoadParameters) {
    // Create a temporary file path
    std::string temp_path = (std::filesystem::temp_directory_path() / "test_rope_params.json").string();
    
    // Modify theta values
    std::vector<double> custom_theta(64);
    std::iota(custom_theta.begin(), custom_theta.end(), 1.0);
    learnable_rope_->setLearnableTheta(custom_theta);
    
    // Save parameters
    bool save_success = learnable_rope_->saveParameters(temp_path);
    EXPECT_TRUE(save_success);
    
    // Check file exists
    EXPECT_TRUE(std::filesystem::exists(temp_path));
    
    // Create new instance and load
    LearnableRotaryEmbedding loaded_rope(config_, true);
    bool load_success = loaded_rope.loadParameters(temp_path);
    EXPECT_TRUE(load_success);
    
    // Check that loaded theta matches saved theta
    const auto& loaded_theta = loaded_rope.getLearnableTheta();
    for (size_t i = 0; i < custom_theta.size(); ++i) {
        EXPECT_NEAR(loaded_theta[i], custom_theta[i], 1e-6);
    }
    
    // Clean up
    std::filesystem::remove(temp_path);
}

TEST_F(LearnableRopeTest, SaveParametersInvalidPath) {
    std::string invalid_path = "/nonexistent/directory/params.json";
    
    bool success = learnable_rope_->saveParameters(invalid_path);
    EXPECT_FALSE(success);
}

TEST_F(LearnableRopeTest, LoadParametersNonexistentFile) {
    std::string nonexistent_path = "/tmp/nonexistent_params.json";
    
    bool success = learnable_rope_->loadParameters(nonexistent_path);
    EXPECT_FALSE(success);
}

TEST_F(LearnableRopeTest, SavedFileFormat) {
    std::string temp_path = (std::filesystem::temp_directory_path() / "test_format.json").string();
    
    learnable_rope_->saveParameters(temp_path);
    
    // Read the file and check format
    std::ifstream file(temp_path);
    ASSERT_TRUE(file.is_open());
    
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }
    file.close();
    
    // Check for key fields
    EXPECT_NE(content.find("\"version\""), std::string::npos);
    EXPECT_NE(content.find("\"hidden_dim\""), std::string::npos);
    EXPECT_NE(content.find("\"num_rotation_pairs\""), std::string::npos);
    EXPECT_NE(content.find("\"base_theta\""), std::string::npos);
    EXPECT_NE(content.find("\"learnable_theta\""), std::string::npos);
    
    std::filesystem::remove(temp_path);
}

// ============================================================================
// Integration Tests with Base Rotation
// ============================================================================

TEST_F(LearnableRopeTest, RotationPreservesMagnitude) {
    std::vector<float> embedding(128);
    std::iota(embedding.begin(), embedding.end(), 0.0f);
    
    // Compute original magnitude
    float original_norm = 0.0f;
    for (float val : embedding) {
        original_norm += val * val;
    }
    original_norm = std::sqrt(original_norm);
    
    // Rotate (use inherited rotate method)
    auto rotated = learnable_rope_->rotate(embedding, 42);
    
    // Compute rotated magnitude
    float rotated_norm = 0.0f;
    for (float val : rotated) {
        rotated_norm += val * val;
    }
    rotated_norm = std::sqrt(rotated_norm);
    
    // Rotation should preserve magnitude
    EXPECT_NEAR(original_norm, rotated_norm, 1e-3);
}

TEST_F(LearnableRopeTest, DifferentPositionsProduceDifferentEmbeddings) {
    std::vector<float> base(128, 1.0f);
    
    auto rot_0 = learnable_rope_->rotate(base, 0);
    auto rot_50 = learnable_rope_->rotate(base, 50);
    auto rot_100 = learnable_rope_->rotate(base, 100);
    
    // Vectors should be different
    EXPECT_NE(rot_0, rot_50);
    EXPECT_NE(rot_50, rot_100);
    EXPECT_NE(rot_0, rot_100);
}

TEST_F(LearnableRopeTest, LearnableThetaAffectsRotation) {
    std::vector<float> embedding(128, 1.0f);
    size_t position = 10;
    
    // Rotate with initial theta
    auto rotated_initial = learnable_rope_->rotate(embedding, position);
    
    // Modify theta
    std::vector<double> new_theta = learnable_rope_->getLearnableTheta();
    for (auto& val : new_theta) {
        val *= 1.5;  // Scale all theta values
    }
    learnable_rope_->setLearnableTheta(new_theta);
    
    // Rotate with modified theta
    auto rotated_modified = learnable_rope_->rotate(embedding, position);
    
    // Results should be different
    EXPECT_NE(rotated_initial, rotated_modified);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(LearnableRopeTest, ZeroLearningRate) {
    auto samples = createTrainingSamples(20);
    
    TrainingConfig train_config;
    train_config.learning_rate = 0.0f;
    train_config.batch_size = 10;
    train_config.max_epochs = 2;
    
    auto initial_theta = learnable_rope_->getLearnableTheta();
    
    learnable_rope_->train(samples, train_config);
    
    // Parameters should not change with zero learning rate
    const auto& final_theta = learnable_rope_->getLearnableTheta();
    for (size_t i = 0; i < initial_theta.size(); ++i) {
        EXPECT_DOUBLE_EQ(initial_theta[i], final_theta[i]);
    }
}

TEST_F(LearnableRopeTest, SingleSampleTraining) {
    std::vector<TrainingSample> single_sample;
    std::vector<float> embedding(128, 1.0f);
    single_sample.emplace_back(embedding, 0, 0.9f);
    
    TrainingConfig train_config;
    train_config.batch_size = 1;
    train_config.max_epochs = 1;
    train_config.validation_split = 0.0f;
    
    // Should not throw
    EXPECT_NO_THROW(learnable_rope_->train(single_sample, train_config));
}

// ============================================================================
// Main removed - using GTest's main from themis_tests.exe
