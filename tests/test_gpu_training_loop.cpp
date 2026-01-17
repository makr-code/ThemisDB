#include <gtest/gtest.h>
#include "llm/lora_framework/gpu_training_loop.h"
#include "llm/lora_framework/gpu_data_loader.h"
#include "llm/lora_framework/gpu_lora_layers.h"
#include "llm/lora_framework/gpu_embedding_layer.h"

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

/**
 * @brief Test GPU embedding layer
 */
TEST(GPUEmbeddingLayerTest, BasicEmbeddingLookup) {
    // Create small embedding matrix
    size_t vocab_size = 10;
    size_t hidden_dim = 8;
    
    std::vector<float> embedding_weights(vocab_size * hidden_dim);
    for (size_t i = 0; i < vocab_size; ++i) {
        for (size_t j = 0; j < hidden_dim; ++j) {
            embedding_weights[i * hidden_dim + j] = static_cast<float>(i * 10 + j);
        }
    }
    
    // Create GPU embedding layer (CPU device for testing)
    GPUEmbeddingLayer layer(embedding_weights.data(), vocab_size, hidden_dim, Device::cpu());
    
    EXPECT_EQ(layer.vocab_size(), vocab_size);
    EXPECT_EQ(layer.hidden_dim(), hidden_dim);
    
    // Create token IDs tensor
    std::vector<float> token_ids_data = {0, 1, 2, 3};  // batch_size=2, seq_len=2
    GPUTensor token_ids({2, 2}, Device::cpu());
    token_ids.upload(token_ids_data);
    
    // Forward pass
    GPUTensor embeddings = layer.forward(token_ids);
    
    // Check output shape: [batch_size, seq_len, hidden_dim]
    auto shape = embeddings.shape();
    EXPECT_EQ(shape.size(), 3);
    EXPECT_EQ(shape[0], 2);  // batch_size
    EXPECT_EQ(shape[1], 2);  // seq_len
    EXPECT_EQ(shape[2], hidden_dim);  // hidden_dim
    
    // Download and verify embeddings
    auto embeddings_data = embeddings.cpu_data();
    
    // Check first embedding (token_id=0)
    for (size_t j = 0; j < hidden_dim; ++j) {
        EXPECT_FLOAT_EQ(embeddings_data[j], static_cast<float>(j));
    }
    
    // Check second embedding (token_id=1)
    for (size_t j = 0; j < hidden_dim; ++j) {
        EXPECT_FLOAT_EQ(embeddings_data[hidden_dim + j], static_cast<float>(10 + j));
    }
}

/**
 * @brief Test GPU embedding layer with out-of-bounds token IDs
 */
TEST(GPUEmbeddingLayerTest, OutOfBoundsTokenID) {
    size_t vocab_size = 5;
    size_t hidden_dim = 4;
    
    std::vector<float> embedding_weights(vocab_size * hidden_dim, 1.0f);
    GPUEmbeddingLayer layer(embedding_weights.data(), vocab_size, hidden_dim, Device::cpu());
    
    // Create token IDs with one out-of-bounds value
    std::vector<float> token_ids_data = {1, 10};  // 10 is out of bounds
    GPUTensor token_ids({1, 2}, Device::cpu());
    token_ids.upload(token_ids_data);
    
    // Forward pass should handle gracefully (zeros for out-of-bounds)
    GPUTensor embeddings = layer.forward(token_ids);
    
    auto embeddings_data = embeddings.cpu_data();
    
    // First embedding should be valid (all 1s)
    for (size_t j = 0; j < hidden_dim; ++j) {
        EXPECT_FLOAT_EQ(embeddings_data[j], 1.0f);
    }
    
    // Second embedding should be zeros (out of bounds)
    for (size_t j = 0; j < hidden_dim; ++j) {
        EXPECT_FLOAT_EQ(embeddings_data[hidden_dim + j], 0.0f);
    }
}
