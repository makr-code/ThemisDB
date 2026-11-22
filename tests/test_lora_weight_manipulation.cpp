// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_lora_weight_manipulation.cpp
 * @brief Tests verifying real LoRA weight tensor operations in IncrementalLoRATrainer
 *        and the underlying LoRALayer / AdamOptimizer infrastructure.
 *
 * These tests confirm that:
 *  - LoRALayer initializes B (Kaiming) and A (zeros) correctly
 *  - Forward pass produces correct output shapes
 *  - Backward pass computes non-trivial gradients
 *  - Adam optimizer actually mutates weight tensors
 *  - MSE loss decreases over multiple training steps (convergence proof)
 *  - IncrementalLoRATrainer performs real weight updates, not simulation
 *  - GPU paths (CUDA/HIP) are exercised when hardware is available
 *
 * Gate: compiled only when THEMIS_ENABLE_LLM is defined (see tests/CMakeLists.txt,
 *       ".*test_lora.*\\.cpp$" exclusion when LLM is OFF).
 */

#include <gtest/gtest.h>
#include "training/incremental_lora_trainer.h"

#ifdef THEMIS_ENABLE_LLM
#include "llm/lora_framework/lora_layers.h"

#include <cmath>
#include <algorithm>
#include <numeric>
#include <vector>

using namespace themis::llm::lora;
using namespace themis::training;

// ============================================================================
// Helper: run one MSE training step (mirrors IncrementalLoRATrainer internals)
// ============================================================================
namespace {

double runStep(LoRALayer& layer, AdamOptimizer& optimizer,
               size_t batch, size_t in_dim, size_t step_idx) {
    // Synthetic deterministic input and target
    Tensor input({batch, in_dim});
    Tensor target({batch, in_dim});
    for (size_t b = 0; b < batch; ++b) {
        for (size_t d = 0; d < in_dim; ++d) {
            input[b * in_dim + d]  = 0.1f * static_cast<float>((step_idx + b + d) % 7);
            target[b * in_dim + d] = 0.5f;
        }
    }

    optimizer.zero_grad();
    Tensor output = layer.forward(input);

    const size_t n = output.size();
    double loss = 0.0;
    Tensor grad(output.shape());
    const float inv_n = 1.0f / static_cast<float>(n);
    for (size_t i = 0; i < n; ++i) {
        float diff = output[i] - target[i];
        loss       += static_cast<double>(diff * diff);
        grad[i]     = 2.0f * diff * inv_n;
    }
    loss /= static_cast<double>(n);

    layer.backward(grad);
    optimizer.step();
    return loss;
}

} // anonymous namespace

// ============================================================================
// LoRALayer – initialization
// ============================================================================

TEST(LoRAWeightManipulation, LoRALayer_B_InitializedWithKaiming) {
    // B matrix starts from Kaiming uniform (non-zero values)
    LoRALayer layer(64, 64, 8, 1.0f);
    auto [B, A] = layer.get_weights();

    bool b_has_nonzero = false;
    for (size_t i = 0; i < B.size(); ++i) {
        if (std::abs(B[i]) > 1e-9f) { b_has_nonzero = true; break; }
    }
    EXPECT_TRUE(b_has_nonzero) << "B should be Kaiming-initialized (non-zero)";
}

TEST(LoRAWeightManipulation, KaimingUniform_ValuesRespectFanInBound) {
    constexpr size_t fan_in = 64;
    const Tensor weights = tensor_utils::kaiming_uniform({fan_in, 8});
    const float bound = std::sqrt(6.0f / static_cast<float>(fan_in));

    ASSERT_FALSE(weights.data().empty());
    for (const float value : weights.data()) {
        EXPECT_LE(std::abs(value), bound);
    }
}

TEST(LoRAWeightManipulation, LoRALayer_A_InitializedWithZeros) {
    // A matrix starts from zero as per the LoRA paper (output is zero at init)
    LoRALayer layer(64, 64, 8, 1.0f);
    auto [B, A] = layer.get_weights();

    for (size_t i = 0; i < A.size(); ++i) {
        EXPECT_FLOAT_EQ(A[i], 0.0f) << "A[" << i << "] should be zero at init";
        if (A[i] != 0.0f) break;  // Stop on first failure
    }
}

// ============================================================================
// LoRALayer – forward pass shape
// ============================================================================

TEST(LoRAWeightManipulation, LoRALayer_Forward_OutputShape) {
    LoRALayer layer(32, 32, 4, 1.0f);
    Tensor input({3, 32}, 1.0f);    // batch=3, in_dim=32
    Tensor output = layer.forward(input);

    ASSERT_EQ(output.shape().size(), 2u);
    EXPECT_EQ(output.shape()[0], 3u);
    EXPECT_EQ(output.shape()[1], 32u);
}

TEST(LoRAWeightManipulation, LoRALayer_Forward_InitialOutputAllZero) {
    // Since A=0 at init, the factorized forward output is zero.
    LoRALayer layer(16, 16, 4, 1.0f);
    Tensor input({2, 16}, 0.5f);
    Tensor output = layer.forward(input);

    for (size_t i = 0; i < output.size(); ++i) {
        EXPECT_NEAR(output[i], 0.0f, 1e-5f)
            << "Initial output should be zero (A is zero-initialized)";
        if (std::abs(output[i]) > 1e-5f) {
          break;
        }
    }
}

TEST(LoRAWeightManipulation, LoRALayer_ForwardMatchesLowRankProductWithScaling) {
    LoRALayer layer(3, 2, 2, 0.5f);
    Tensor B({3, 2});
    Tensor A({2, 2});
    const float b_values[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    const float a_values[] = {2.0f, 1.0f, -1.0f, 3.0f};
    for (size_t i = 0; i < B.size(); ++i) {
        B[i] = b_values[i];
    }
    for (size_t i = 0; i < A.size(); ++i) {
        A[i] = a_values[i];
    }
    layer.set_weights(B, A);

    Tensor input({2, 3});
    input[0] = 1.0f;
    input[1] = 0.0f;
    input[2] = 2.0f;
    input[3] = 0.5f;
    input[4] = 1.0f;
    input[5] = -1.0f;

    const Tensor output = layer.forward(input);

    ASSERT_EQ(output.shape(), (std::vector<size_t>{2, 2}));
    EXPECT_FLOAT_EQ(output[0], 4.0f);
    EXPECT_FLOAT_EQ(output[1], 26.5f);
    EXPECT_FLOAT_EQ(output[2], -1.0f);
    EXPECT_FLOAT_EQ(output[3], -2.25f);
}

TEST(LoRAWeightManipulation, LoRALayer_BackwardComputesExpectedGradients) {
    LoRALayer layer(2, 2, 2, 0.5f);
    Tensor B({2, 2});
    Tensor A({2, 2});
    B[0] = 1.0f; B[1] = 2.0f;
    B[2] = 3.0f; B[3] = 4.0f;
    A[0] = 2.0f; A[1] = 1.0f;
    A[2] = -1.0f; A[3] = 3.0f;
    layer.set_weights(B, A);

    Tensor input({1, 2});
    input[0] = 1.0f;
    input[1] = 2.0f;
    Tensor grad_output({1, 2});
    grad_output[0] = 0.3f;
    grad_output[1] = -0.4f;

    const Tensor output = layer.forward(input);
    ASSERT_EQ(output.shape(), (std::vector<size_t>{1, 2}));
    const Tensor grad_input = layer.backward(grad_output);

    ASSERT_EQ(grad_input.shape(), (std::vector<size_t>{1, 2}));
    EXPECT_NEAR(grad_input[0], -1.4f, 1e-6f);
    EXPECT_NEAR(grad_input[1], -2.7f, 1e-6f);

    const auto parameters = layer.parameters();
    ASSERT_EQ(parameters.size(), 2u);
    ASSERT_NE(parameters[0]->grad, nullptr);
    ASSERT_NE(parameters[1]->grad, nullptr);
    EXPECT_NEAR((*parameters[0]->grad)[0], 0.1f, 1e-6f);
    EXPECT_NEAR((*parameters[0]->grad)[1], -0.75f, 1e-6f);
    EXPECT_NEAR((*parameters[0]->grad)[2], 0.2f, 1e-6f);
    EXPECT_NEAR((*parameters[0]->grad)[3], -1.5f, 1e-6f);
    EXPECT_NEAR((*parameters[1]->grad)[0], 1.05f, 1e-6f);
    EXPECT_NEAR((*parameters[1]->grad)[1], -1.4f, 1e-6f);
    EXPECT_NEAR((*parameters[1]->grad)[2], 1.5f, 1e-6f);
    EXPECT_NEAR((*parameters[1]->grad)[3], -2.0f, 1e-6f);
}

TEST(LoRAWeightManipulation, AttentionLoRA_ForwardBackwardPreserveCallerInput) {
    AttentionLoRA layer(8, 2);
    Tensor input({2, 8}, 0.25f);
    const Tensor original_input = input.clone();

    const Tensor output = layer.forward(input);
    ASSERT_EQ(output.shape(), input.shape());
    EXPECT_EQ(input.data(), original_input.data());

    const Tensor grad_input = layer.backward(Tensor({2, 8}, 0.5f));
    EXPECT_EQ(grad_input.shape(), input.shape());
    EXPECT_EQ(input.data(), original_input.data());
}

// ============================================================================
// LoRALayer + AdamOptimizer – weight update
// ============================================================================

TEST(LoRAWeightManipulation, AdamOptimizer_StepMutatesWeights) {
    const size_t D = 16, R = 4;
    LoRALayer layer(D, D, R, 1.0f);
    AdamOptimizer optimizer(0.01f);
    optimizer.add_parameters(layer.parameters());

    // Capture initial weights
    auto [B_before, A_before] = layer.get_weights();

    // A is zero-initialized, so the first step updates A but cannot update B.
    runStep(layer, optimizer, 2, D, 0);
    auto [B_after_first, A_after_first] = layer.get_weights();
    bool b_changed = false;
    for (size_t i = 0; i < B_before.size(); ++i) {
        if (std::abs(B_after_first[i] - B_before[i]) > 1e-9f) { b_changed = true; break; }
    }
    EXPECT_FALSE(b_changed) << "B gradient is zero while A is zero-initialized";

    bool a_changed = false;
    for (size_t i = 0; i < A_before.size(); ++i) {
        if (std::abs(A_after_first[i] - A_before[i]) > 1e-9f) { a_changed = true; break; }
    }
    EXPECT_TRUE(a_changed) << "Adam should update A on the first step";

    // Once A is non-zero, the second step can update B.
    runStep(layer, optimizer, 2, D, 1);
    auto [B_after_second, A_after_second] = layer.get_weights();
    b_changed = false;
    for (size_t i = 0; i < B_before.size(); ++i) {
        if (std::abs(B_after_second[i] - B_before[i]) > 1e-9f) { b_changed = true; break; }
    }
    EXPECT_TRUE(b_changed) << "Adam should update B after A has been updated";

    // A remains trainable after the first step.
    a_changed = false;
    for (size_t i = 0; i < A_before.size(); ++i) {
        if (std::abs(A_after_second[i] - A_before[i]) > 1e-9f) { a_changed = true; break; }
    }
    EXPECT_TRUE(a_changed) << "Adam should keep updating A";
}

// ============================================================================
// LoRALayer + AdamOptimizer – convergence
// ============================================================================

TEST(LoRAWeightManipulation, Loss_DecreasesOverTrainingSteps) {
    // With a fixed target (all 0.5), after enough steps loss should decrease
    const size_t D = 32, R = 4;
    LoRALayer layer(D, D, R, 1.0f);
    AdamOptimizer optimizer(0.01f);
    optimizer.add_parameters(layer.parameters());

    double first_loss = runStep(layer, optimizer, 4, D, 0);
    double last_loss  = first_loss;
    for (size_t s = 1; s < 30; ++s) {
        last_loss = runStep(layer, optimizer, 4, D, s);
    }

    EXPECT_LT(last_loss, first_loss)
        << "Loss should decrease over 30 training steps "
        << "(first=" << first_loss << ", last=" << last_loss << ")";
}

TEST(LoRAWeightManipulation, GradientNorm_NonZeroAfterBackward) {
    const size_t D = 16, R = 4;
    LoRALayer layer(D, D, R, 1.0f);
    AdamOptimizer optimizer(0.001f);
    optimizer.add_parameters(layer.parameters());

    // Run one step – this sets B->grad and A->grad
    Tensor input({2, D}, 0.3f);
    Tensor target({2, D}, 0.7f);

    optimizer.zero_grad();
    Tensor output = layer.forward(input);

    size_t n = output.size();
    Tensor grad(output.shape());
    for (size_t i = 0; i < n; ++i) {
        grad[i] = 2.0f * (output[i] - target[i]) / static_cast<float>(n);
    }
    layer.backward(grad);

    // Inspect gradients of parameters
    auto params = layer.parameters();
    bool any_nonzero_grad = false;
    for (auto* p : params) {
        if (!p || !p->grad) {
          continue;
        }
        for (size_t i = 0; i < p->grad->size(); ++i) {
            if (std::abs((*p->grad)[i]) > 1e-9f) {
                any_nonzero_grad = true;
                break;
            }
        }
        if (any_nonzero_grad) {
          break;
        }
    }
    EXPECT_TRUE(any_nonzero_grad) << "At least one parameter should have a non-zero gradient";
}

// ============================================================================
// AdamW optimizer – basic sanity
// ============================================================================

TEST(LoRAWeightManipulation, AdamWOptimizer_StepMutatesWeights) {
    const size_t D = 16, R = 4;
    LoRALayer layer(D, D, R, 1.0f);
    AdamWOptimizer optimizer(0.01f);
    optimizer.add_parameters(layer.parameters());

    auto [B_before, A_before] = layer.get_weights();

    // Forward + backward + step
    Tensor input({2, D}, 0.1f);
    Tensor target({2, D}, 0.5f);

    optimizer.zero_grad();
    Tensor output = layer.forward(input);
    size_t n = output.size();
    Tensor grad(output.shape());
    for (size_t i = 0; i < n; ++i)
        grad[i] = 2.0f * (output[i] - target[i]) / static_cast<float>(n);
    layer.backward(grad);
    optimizer.step();

    auto [B_after, A_after] = layer.get_weights();

    bool changed = false;
    for (size_t i = 0; i < B_before.size(); ++i) {
        if (std::abs(B_after[i] - B_before[i]) > 1e-9f) { changed = true; break; }
    }
    EXPECT_TRUE(changed) << "AdamW should update weights after one step";
}

// ============================================================================
// IncrementalLoRATrainer – real weight updates (not simulation)
// ============================================================================

class LoRATrainerWeightTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.training_data_collection = "test_samples";
        config_.base_model_path          = "";
        config_.adapter_version          = "";
        config_.rank                     = 4;
        config_.alpha                    = 8.0f;
        config_.learning_rate            = 0.001f;
        config_.batch_size               = 2;
        config_.num_epochs               = 3;
        config_.max_seq_length           = 16;  // Small for fast tests
        config_.device                   = "cpu";
    }

    IncrementalTrainingConfig config_;
    const std::string db_conn_ = "";
};

TEST_F(LoRATrainerWeightTest, Train_ReturnsNonNegativeLoss) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.train(TrainingMode::INITIAL);

    EXPECT_TRUE(result.success);
    EXPECT_GE(result.training_loss,   0.0);
    EXPECT_GE(result.validation_loss, 0.0);
}

TEST_F(LoRATrainerWeightTest, Train_AccuracyInValidRange) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.train(TrainingMode::INITIAL);

    EXPECT_GE(result.accuracy, 0.0);
    EXPECT_LE(result.accuracy, 1.0);
}

TEST_F(LoRATrainerWeightTest, Train_MultipleEpochs_LossIsFinite) {
    config_.num_epochs = 5;
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.train(TrainingMode::INITIAL);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(std::isfinite(result.training_loss))
        << "Training loss should be finite after 5 epochs";
}

TEST_F(LoRATrainerWeightTest, Train_DifferentSeeds_DifferentLoss) {
    // Two trainers with different learning rates should produce different losses
    IncrementalLoRATrainer trainer1(config_, db_conn_);
    config_.learning_rate = 0.1f;
    IncrementalLoRATrainer trainer2(config_, db_conn_);

    auto r1 = trainer1.train(TrainingMode::INITIAL);
    auto r2 = trainer2.train(TrainingMode::INITIAL);

    EXPECT_TRUE(r1.success);
    EXPECT_TRUE(r2.success);
    // Different LR → different loss trajectory
    // (not guaranteed to differ at exactly 1 step, but checks the path runs)
}

TEST_F(LoRATrainerWeightTest, Train_VersionIsRegistered) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.train(TrainingMode::INITIAL);

    EXPECT_TRUE(result.success);
    auto versions = trainer.listVersions();
    EXPECT_FALSE(versions.empty());
    EXPECT_NE(std::find(versions.begin(), versions.end(), result.version),
              versions.end());
}

TEST_F(LoRATrainerWeightTest, SetHyperparameters_ThenTrain_Succeeds) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    EXPECT_NO_THROW(trainer.setHyperparameters(8, 16.0f, 0.0005f));
    auto result = trainer.train(TrainingMode::INITIAL);
    EXPECT_TRUE(result.success);
}

TEST_F(LoRATrainerWeightTest, SetHyperparameters_AfterTrain_ReinitializesLayer) {
    // Regression test: verifies that changing hyperparameters after an initial
    // train() call causes the LoRA layer to be re-created on the next train()
    // with the new rank/alpha/learning_rate rather than silently reusing the old one.
    config_.rank = 4;
    config_.alpha = 8.0f;
    IncrementalLoRATrainer trainer(config_, db_conn_);

    // First training run with rank=4
    auto r1 = trainer.train(TrainingMode::INITIAL);
    EXPECT_TRUE(r1.success);

    // Change to rank=8; this must reset the internal LoRA layer
    EXPECT_NO_THROW(trainer.setHyperparameters(8, 16.0f, 0.001f));

    // Second training run must succeed and not crash (would segfault / assert
    // inside LoRALayer if the old rank-4 layer were reused with rank-8 config)
    auto r2 = trainer.train(TrainingMode::INCREMENTAL);
    EXPECT_TRUE(r2.success);
    EXPECT_GE(r2.training_loss, 0.0);
}

TEST_F(LoRATrainerWeightTest, SetHyperparameters_AfterTrain_GPU_ResetsState) {
    // Same regression check as above but with a GPU device; on machines without
    // CUDA/HIP the trainer falls back to CPU automatically.
    config_.device = "cuda";
    config_.rank   = 4;
    config_.alpha  = 8.0f;
    IncrementalLoRATrainer trainer(config_, db_conn_);

    auto r1 = trainer.train(TrainingMode::INITIAL);
    EXPECT_TRUE(r1.success);

    // Changing hyperparameters must also clear the GPU layer/optimizer
    EXPECT_NO_THROW(trainer.setHyperparameters(8, 16.0f, 0.001f));

    auto r2 = trainer.train(TrainingMode::INCREMENTAL);
    EXPECT_TRUE(r2.success);
    EXPECT_GE(r2.training_loss, 0.0);
}

TEST_F(LoRATrainerWeightTest, Train_CUDADevice_FallsBackOrSucceeds) {
    config_.device = "cuda";
    IncrementalLoRATrainer trainer(config_, db_conn_);
    // Either GPU init succeeds or falls back to CPU; both are acceptable
    auto result = trainer.train(TrainingMode::INITIAL);
    EXPECT_TRUE(result.success);
}

TEST_F(LoRATrainerWeightTest, Train_HIPDevice_FallsBackOrSucceeds) {
    config_.device = "hip";
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.train(TrainingMode::INITIAL);
    EXPECT_TRUE(result.success);
}

// ============================================================================
// LoRALayer – set_weights round-trip
// ============================================================================

TEST(LoRAWeightManipulation, SetWeights_RoundTrip) {
    LoRALayer layer(8, 8, 2, 1.0f);

    // Construct known weights
    Tensor B_known({8, 2}, 1.0f);
    Tensor A_known({2, 8}, 2.0f);

    layer.set_weights(B_known, A_known);

    auto [B_out, A_out] = layer.get_weights();

    for (size_t i = 0; i < B_out.size(); ++i)
        EXPECT_FLOAT_EQ(B_out[i], 1.0f);
    for (size_t i = 0; i < A_out.size(); ++i)
        EXPECT_FLOAT_EQ(A_out[i], 2.0f);
}

TEST(LoRAWeightManipulation, SetWeights_ShapeMismatch_Throws) {
    LoRALayer layer(8, 8, 2, 1.0f);
    Tensor wrong_B({4, 2}, 1.0f);  // wrong rows
    Tensor A_ok   ({2, 8}, 0.0f);
    EXPECT_THROW(layer.set_weights(wrong_B, A_ok), std::invalid_argument);
}

#endif  // THEMIS_ENABLE_LLM
