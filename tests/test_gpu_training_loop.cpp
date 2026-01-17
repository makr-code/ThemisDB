#include <gtest/gtest.h>
#include "llm/lora_framework/gpu_training_loop.h"
#include "llm/lora_framework/gpu_data_loader.h"
#include "llm/lora_framework/gpu_lora_layers.h"

using namespace themis::llm::lora;

/**
 * @brief Test GPU training loop integration
 * 
 * This test validates that the GPU training infrastructure works correctly:
 * - GPU data loader loads batches to GPU
 * - GPU LoRA layer executes on GPU
 * - Training loop orchestrates GPU training
 * - VRAM management stays within limits
 */
class GPUTrainingLoopTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Detect available GPU backends
        auto backends = GPUMemoryManager::detect_backends();
        
        has_gpu_ = false;
        for (const auto& backend : backends) {
            if (backend.available && 
                (backend.type == acceleration::BackendType::CUDA ||
                 backend.type == acceleration::BackendType::HIP ||
                 backend.type == acceleration::BackendType::VULKAN)) {
                has_gpu_ = true;
                break;
            }
        }
    }
    
    bool has_gpu_ = false;
};

TEST_F(GPUTrainingLoopTest, CreateGPUDataLoader) {
    if (!has_gpu_) {
        GTEST_SKIP() << "No GPU available";
    }
    
    // Create simple tokenizer
    auto tokenizer = std::make_shared<SimpleTokenizer>();
    
    // Create GPU data loader configuration
    GPUDataLoaderConfig config;
    config.batch_size = 2;
    config.max_sequence_length = 128;
    config.target_device = Device::cuda();
    config.async_loading = false;  // Disable async for testing
    
    // Create data loader
    GPUDataLoader loader(tokenizer, config);
    
    // Create sample data
    std::vector<InstructionDataSample> samples;
    for (int i = 0; i < 10; ++i) {
        InstructionDataSample sample;
        sample.instruction = "Test instruction " + std::to_string(i);
        sample.output = "Test output " + std::to_string(i);
        samples.push_back(sample);
    }
    
    // Load samples
    ASSERT_TRUE(loader.loadFromSamples(samples));
    EXPECT_EQ(loader.size(), 10);
    EXPECT_GT(loader.num_batches(), 0);
}

TEST_F(GPUTrainingLoopTest, CreateGPULoRALayer) {
    Device device = has_gpu_ ? Device::cuda() : Device::cpu();
    
    size_t in_dim = 64;
    size_t out_dim = 64;
    size_t rank = 8;
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, device, true);
    
    EXPECT_EQ(layer.in_dim(), in_dim);
    EXPECT_EQ(layer.out_dim(), out_dim);
    EXPECT_EQ(layer.rank(), rank);
    EXPECT_EQ(layer.parameter_count(), in_dim * rank + rank * out_dim);
    
    auto params = layer.parameters();
    EXPECT_EQ(params.size(), 2);  // B and A matrices
}

TEST_F(GPUTrainingLoopTest, GPUTrainingLoopBasic) {
    if (!has_gpu_) {
        GTEST_SKIP() << "No GPU available";
    }
    
    // Create tokenizer
    auto tokenizer = std::make_shared<SimpleTokenizer>();
    
    // Create small dataset
    std::vector<InstructionDataSample> samples;
    for (int i = 0; i < 5; ++i) {
        InstructionDataSample sample;
        sample.instruction = "Instruction " + std::to_string(i);
        sample.output = "Output " + std::to_string(i);
        samples.push_back(sample);
    }
    
    // Setup data loader
    GPUDataLoaderConfig loader_config;
    loader_config.batch_size = 2;
    loader_config.max_sequence_length = 64;
    loader_config.target_device = Device::cuda();
    loader_config.async_loading = false;
    
    auto data_loader = std::make_unique<GPUDataLoader>(tokenizer, loader_config);
    ASSERT_TRUE(data_loader->loadFromSamples(samples));
    
    // Create GPU LoRA layer
    size_t hidden_dim = 64;
    size_t rank = 4;
    auto lora_layer = std::make_unique<GPULoRALayer>(
        hidden_dim, hidden_dim, rank, 1.0f, Device::cuda(), true
    );
    
    // Setup training configuration
    GPUTrainingConfig config;
    config.num_epochs = 1;
    config.learning_rate = 0.001f;
    config.device = Device::cuda();
    config.use_mixed_precision = false;
    
    // Create training loop
    GPUTrainingLoop trainer(config);
    trainer.setDataLoader(std::move(data_loader));
    trainer.addLayer(lora_layer.get());
    
    // Track training progress
    int callback_count = 0;
    trainer.registerCallback([&callback_count](const GPUTrainingMetrics& metrics) {
        callback_count++;
        EXPECT_GE(metrics.current_step, 0);
        EXPECT_LE(metrics.progress, 1.0f);
    });
    
    // Run training
    bool success = trainer.train();
    EXPECT_TRUE(success);
    EXPECT_GT(callback_count, 0);
    
    // Check final loss is reasonable
    float final_loss = trainer.getFinalLoss();
    EXPECT_GT(final_loss, 0.0f);
    EXPECT_LT(final_loss, 1000.0f);  // Should not explode
}

TEST_F(GPUTrainingLoopTest, GPUDataLoaderBatchRetrieval) {
    Device device = has_gpu_ ? Device::cuda() : Device::cpu();
    
    auto tokenizer = std::make_shared<SimpleTokenizer>();
    
    GPUDataLoaderConfig config;
    config.batch_size = 3;
    config.max_sequence_length = 32;
    config.target_device = device;
    config.async_loading = false;
    
    GPUDataLoader loader(tokenizer, config);
    
    std::vector<InstructionDataSample> samples;
    for (int i = 0; i < 10; ++i) {
        InstructionDataSample sample;
        sample.instruction = "Test " + std::to_string(i);
        sample.output = "Response " + std::to_string(i);
        samples.push_back(sample);
    }
    
    ASSERT_TRUE(loader.loadFromSamples(samples));
    
    // Retrieve all batches
    int batch_count = 0;
    while (loader.hasNext()) {
        auto batch = loader.getNextBatch();
        EXPECT_TRUE(batch.is_valid());
        EXPECT_GT(batch.batch_size, 0);
        EXPECT_EQ(batch.seq_len, config.max_sequence_length);
        batch_count++;
    }
    
    EXPECT_EQ(batch_count, loader.num_batches());
}

TEST_F(GPUTrainingLoopTest, MemoryStatsTracking) {
    if (!has_gpu_) {
        GTEST_SKIP() << "No GPU available";
    }
    
    GPUTrainingConfig config;
    config.device = Device::cuda();
    config.num_epochs = 1;
    config.learning_rate = 0.001f;
    
    GPUTrainingLoop trainer(config);
    
    // Check that metrics can be retrieved
    auto metrics = trainer.getMetrics();
    EXPECT_EQ(metrics.current_epoch, 0);
    EXPECT_EQ(metrics.current_step, 0);
    EXPECT_EQ(metrics.status, "idle");
}

TEST_F(GPUTrainingLoopTest, CPUFallbackWorks) {
    // Test that training works on CPU when GPU not available
    auto tokenizer = std::make_shared<SimpleTokenizer>();
    
    std::vector<InstructionDataSample> samples;
    for (int i = 0; i < 3; ++i) {
        InstructionDataSample sample;
        sample.instruction = "CPU test " + std::to_string(i);
        sample.output = "CPU response " + std::to_string(i);
        samples.push_back(sample);
    }
    
    GPUDataLoaderConfig loader_config;
    loader_config.batch_size = 1;
    loader_config.max_sequence_length = 32;
    loader_config.target_device = Device::cpu();
    
    auto data_loader = std::make_unique<GPUDataLoader>(tokenizer, loader_config);
    ASSERT_TRUE(data_loader->loadFromSamples(samples));
    
    size_t hidden_dim = 32;
    auto lora_layer = std::make_unique<GPULoRALayer>(
        hidden_dim, hidden_dim, 4, 1.0f, Device::cpu(), false
    );
    
    GPUTrainingConfig config;
    config.num_epochs = 1;
    config.learning_rate = 0.001f;
    config.device = Device::cpu();
    
    GPUTrainingLoop trainer(config);
    trainer.setDataLoader(std::move(data_loader));
    trainer.addLayer(lora_layer.get());
    
    bool success = trainer.train();
    EXPECT_TRUE(success);
}

TEST_F(GPUTrainingLoopTest, MSELossGPUKernel) {
    Device device = has_gpu_ ? Device::cuda() : Device::cpu();
    
    // Create test tensors with known values
    size_t n = 1000;
    GPUTensor predictions({n}, device);
    GPUTensor targets({n}, device);
    
    // Initialize with simple patterns
    std::vector<float> pred_data(n);
    std::vector<float> target_data(n);
    
    for (size_t i = 0; i < n; ++i) {
        pred_data[i] = static_cast<float>(i) / 100.0f;
        target_data[i] = static_cast<float>(i) / 100.0f + 0.1f;  // offset by 0.1
    }
    
    predictions.upload(pred_data);
    targets.upload(target_data);
    
    // Compute loss using GPU kernel
    float loss = computeMSELossGPU(predictions, targets);
    
    // Expected loss = mean((0.1)^2) = 0.01
    EXPECT_NEAR(loss, 0.01f, 1e-5f) << "MSE loss should be 0.01 for constant offset of 0.1";
    EXPECT_GT(loss, 0.0f) << "Loss should be positive";
}

TEST_F(GPUTrainingLoopTest, MSEGradientGPUKernel) {
    Device device = has_gpu_ ? Device::cuda() : Device::cpu();
    
    // Create test tensors
    size_t n = 500;
    GPUTensor predictions({n}, device);
    GPUTensor targets({n}, device);
    
    std::vector<float> pred_data(n);
    std::vector<float> target_data(n);
    
    for (size_t i = 0; i < n; ++i) {
        pred_data[i] = 2.0f;
        target_data[i] = 1.0f;
    }
    
    predictions.upload(pred_data);
    targets.upload(target_data);
    
    // Compute gradient
    GPUTensor grad = computeMSEGradientGPU(predictions, targets);
    
    // Check gradient shape
    EXPECT_EQ(grad.shape(), predictions.shape());
    
    // Check gradient values
    // MSE gradient: grad = (2/n) * (pred - target)
    // For our test case: (2/500) * (2 - 1) = 0.004
    auto grad_data = grad.cpu_data();
    EXPECT_EQ(grad_data.size(), n);
    
    float expected_grad = 2.0f / n * (2.0f - 1.0f);
    for (size_t i = 0; i < std::min(size_t(10), n); ++i) {
        EXPECT_NEAR(grad_data[i], expected_grad, 1e-6f) 
            << "Gradient at index " << i << " should be " << expected_grad;
    }
}

TEST_F(GPUTrainingLoopTest, MSEKernelsNumericalAccuracy) {
    Device device = has_gpu_ ? Device::cuda() : Device::cpu();
    
    // Test with various tensor sizes to ensure accuracy across different block configurations
    std::vector<size_t> test_sizes = {100, 256, 1000, 10000};
    
    for (size_t n : test_sizes) {
        GPUTensor predictions({n}, device);
        GPUTensor targets({n}, device);
        
        std::vector<float> pred_data(n);
        std::vector<float> target_data(n);
        
        // Create random-like pattern
        for (size_t i = 0; i < n; ++i) {
            pred_data[i] = static_cast<float>(i % 100) / 50.0f;
            target_data[i] = static_cast<float>((i + 1) % 100) / 50.0f;
        }
        
        predictions.upload(pred_data);
        targets.upload(target_data);
        
        // Compute loss
        float loss = computeMSELossGPU(predictions, targets);
        
        // Compute expected loss on CPU for verification
        float expected_sum = 0.0f;
        for (size_t i = 0; i < n; ++i) {
            float diff = pred_data[i] - target_data[i];
            expected_sum += diff * diff;
        }
        float expected_loss = expected_sum / n;
        
        // Check numerical accuracy (within floating point precision)
        EXPECT_NEAR(loss, expected_loss, 1e-4f) 
            << "Loss mismatch for size " << n 
            << ": GPU=" << loss << " vs expected=" << expected_loss;
        
        // Also test gradient accuracy
        GPUTensor grad = computeMSEGradientGPU(predictions, targets);
        auto grad_data = grad.cpu_data();
        
        float scale = 2.0f / n;
        for (size_t i = 0; i < std::min(size_t(5), n); ++i) {
            float expected_grad = scale * (pred_data[i] - target_data[i]);
            EXPECT_NEAR(grad_data[i], expected_grad, 1e-6f)
                << "Gradient mismatch at index " << i << " for size " << n;
        }
    }
}

TEST_F(GPUTrainingLoopTest, FusedMSELossGradientKernel) {
    Device device = has_gpu_ ? Device::cuda() : Device::cpu();
    
    // Create test tensors
    size_t n = 1000;
    GPUTensor predictions({n}, device);
    GPUTensor targets({n}, device);
    
    std::vector<float> pred_data(n);
    std::vector<float> target_data(n);
    
    for (size_t i = 0; i < n; ++i) {
        pred_data[i] = static_cast<float>(i) / 100.0f;
        target_data[i] = static_cast<float>(i) / 100.0f + 0.1f;  // offset by 0.1
    }
    
    predictions.upload(pred_data);
    targets.upload(target_data);
    
    // Compute using fused kernel
    GPUTensor grad_output;
    float loss = computeFusedMSELossGradientGPU(predictions, targets, grad_output);
    
    // Verify loss
    EXPECT_NEAR(loss, 0.01f, 1e-5f) << "Fused MSE loss should be 0.01";
    
    // Verify gradient
    EXPECT_EQ(grad_output.shape(), predictions.shape());
    auto grad_data = grad_output.cpu_data();
    
    float scale = 2.0f / n;
    for (size_t i = 0; i < std::min(size_t(10), n); ++i) {
        float expected_grad = scale * (pred_data[i] - target_data[i]);
        EXPECT_NEAR(grad_data[i], expected_grad, 1e-6f)
            << "Fused gradient at index " << i << " should match expected";
    }
}

TEST_F(GPUTrainingLoopTest, FusedVsSeparateMSEKernels) {
    Device device = has_gpu_ ? Device::cuda() : Device::cpu();
    
    // Create test tensors with various patterns
    std::vector<size_t> test_sizes = {256, 1000, 10000};
    
    for (size_t n : test_sizes) {
        GPUTensor predictions({n}, device);
        GPUTensor targets({n}, device);
        
        std::vector<float> pred_data(n);
        std::vector<float> target_data(n);
        
        for (size_t i = 0; i < n; ++i) {
            pred_data[i] = static_cast<float>(i % 100) / 50.0f;
            target_data[i] = static_cast<float>((i + 1) % 100) / 50.0f;
        }
        
        predictions.upload(pred_data);
        targets.upload(target_data);
        
        // Compute with separate kernels
        float loss_separate = computeMSELossGPU(predictions, targets);
        GPUTensor grad_separate = computeMSEGradientGPU(predictions, targets);
        auto grad_sep_data = grad_separate.cpu_data();
        
        // Compute with fused kernel
        GPUTensor grad_fused;
        float loss_fused = computeFusedMSELossGradientGPU(predictions, targets, grad_fused);
        auto grad_fused_data = grad_fused.cpu_data();
        
        // Verify results match
        EXPECT_NEAR(loss_fused, loss_separate, 1e-4f)
            << "Fused loss should match separate loss for size " << n;
        
        EXPECT_EQ(grad_fused_data.size(), grad_sep_data.size());
        
        for (size_t i = 0; i < std::min(size_t(10), n); ++i) {
            EXPECT_NEAR(grad_fused_data[i], grad_sep_data[i], 1e-5f)
                << "Fused gradient should match separate gradient at index " 
                << i << " for size " << n;
        }
    }
}
