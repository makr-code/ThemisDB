#include <gtest/gtest.h>
#include "llm/lora_framework/multi_gpu.h"
#include "llm/lora_framework/nccl_backend.h"
#include "llm/lora_framework/rccl_backend.h"
#include "llm/lora_framework/custom_allreduce.h"
#include "llm/lora_framework/multi_gpu_lora_layer.h"
#include "llm/lora_framework/multi_gpu_trainer.h"
#include "llm/lora_framework/distributed_dataloader.h"
#include <cmath>

using namespace themis::llm::lora;

namespace {
    constexpr float EPSILON = 1e-3f;
}

// ===== Multi-GPU Context Tests =====

TEST(MultiGPUTest, ContextCreation) {
    // Try to create context with available GPUs
    MultiGPUContext ctx(0);  // Use all available
    
    // Should work even with 0 GPUs (CPU fallback)
    EXPECT_GE(ctx.num_gpus(), 0);
    EXPECT_EQ(ctx.world_size(), ctx.num_gpus());
}

TEST(MultiGPUTest, ContextWithSpecificGPUs) {
    std::vector<int> gpu_ids = {0};
    MultiGPUContext ctx(0, gpu_ids);
    
    if (ctx.num_gpus() > 0) {
        EXPECT_EQ(ctx.get_device(0).id, 0);
    }
}

TEST(MultiGPUTest, GPUTopologyDetection) {
    MultiGPUContext ctx(0);
    
    if (ctx.num_gpus() > 0) {
        auto topology = GPUTopology::detect(ctx.devices());
        EXPECT_EQ(topology.num_gpus, ctx.num_gpus());
        EXPECT_EQ(topology.bandwidth_matrix.size(), static_cast<size_t>(ctx.num_gpus()));
    }
}

// ===== NCCL Backend Tests =====

TEST(NCCLBackendTest, Availability) {
    bool available = NCCLBackend::is_available();
    
    if (available) {
        std::string version = NCCLBackend::get_version();
        EXPECT_FALSE(version.empty());
        EXPECT_NE(version, "Not available");
    }
}

TEST(NCCLBackendTest, Initialization) {
    MultiGPUContext ctx(0);
    
    if (ctx.num_gpus() == 0 || !NCCLBackend::is_available()) {
        GTEST_SKIP() << "NCCL or GPUs not available";
    }
    
    NCCLBackend backend(ctx, 0, ctx.num_gpus());
    EXPECT_TRUE(backend.initialize());
    EXPECT_TRUE(backend.is_initialized());
}

// ===== RCCL Backend Tests =====

TEST(RCCLBackendTest, Availability) {
    bool available = RCCLBackend::is_available();
    
    if (available) {
        std::string version = RCCLBackend::get_version();
        EXPECT_FALSE(version.empty());
        EXPECT_NE(version, "Not available");
    }
}

// ===== Custom AllReduce Tests =====

TEST(CustomAllReduceTest, Initialization) {
    MultiGPUContext ctx(0);
    
    if (ctx.num_gpus() == 0) {
        GTEST_SKIP() << "No GPUs available";
    }
    
    CustomAllReduce backend(ctx, 0, ctx.num_gpus());
    EXPECT_TRUE(backend.initialize());
    EXPECT_TRUE(backend.is_initialized());
}

TEST(CustomAllReduceTest, AllReduceSingleGPU) {
    MultiGPUContext ctx(1);
    
    if (ctx.num_gpus() == 0) {
        GTEST_SKIP() << "No GPUs available";
    }
    
    CustomAllReduce backend(ctx, 0, 1);
    backend.initialize();
    
    // Single GPU - allreduce should be no-op
    GPUTensor tensor({4}, ctx.get_device(0));
    tensor.fill(5.0f);
    
    EXPECT_TRUE(backend.allreduce(tensor, false));
    
    auto data = tensor.cpu_data();
    for (float val : data) {
        EXPECT_NEAR(val, 5.0f, EPSILON);
    }
}

// ===== Multi-GPU LoRA Layer Tests =====

TEST(MultiGPULoRALayerTest, Creation) {
    MultiGPUContext ctx(0);
    
    if (ctx.num_gpus() == 0) {
        GTEST_SKIP() << "No GPUs available";
    }
    
    MultiGPULoRALayer layer(64, 32, 8, 1.0f, ctx);
    
    EXPECT_EQ(layer.num_gpus(), ctx.num_gpus());
    EXPECT_EQ(layer.backend_type(), CommBackend::AUTO);
}

TEST(MultiGPULoRALayerTest, ForwardSingleGPU) {
    MultiGPUContext ctx(1);
    
    if (ctx.num_gpus() == 0) {
        GTEST_SKIP() << "No GPUs available";
    }
    
    MultiGPULoRALayer layer(4, 4, 2, 1.0f, ctx);
    
    // Create input tensor
    GPUTensor input({2, 4}, ctx.get_device(0));
    input.fill(1.0f);
    
    std::vector<GPUTensor> inputs = {std::move(input)};
    
    auto outputs = layer.forward(inputs);
    
    EXPECT_EQ(outputs.size(), 1);
    EXPECT_EQ(outputs[0].shape()[0], 2);
    EXPECT_EQ(outputs[0].shape()[1], 4);
}

TEST(MultiGPULoRALayerTest, BackwardAndGradientSync) {
    MultiGPUContext ctx(1);
    
    if (ctx.num_gpus() == 0) {
        GTEST_SKIP() << "No GPUs available";
    }
    
    MultiGPULoRALayer layer(4, 4, 2, 1.0f, ctx);
    
    GPUTensor input({2, 4}, ctx.get_device(0));
    input.fill(1.0f);
    
    std::vector<GPUTensor> inputs = {std::move(input)};
    auto outputs = layer.forward(inputs);
    
    // Backward pass
    GPUTensor grad_output({2, 4}, ctx.get_device(0));
    grad_output.fill(1.0f);
    
    std::vector<GPUTensor> grad_outputs = {std::move(grad_output)};
    auto grad_inputs = layer.backward(grad_outputs);
    
    EXPECT_EQ(grad_inputs.size(), 1);
    EXPECT_FALSE(layer.are_gradients_synced());
    
    // Synchronize gradients
    EXPECT_TRUE(layer.synchronize_gradients());
    EXPECT_TRUE(layer.are_gradients_synced());
}

TEST(MultiGPULoRALayerTest, ParameterBroadcast) {
    MultiGPUContext ctx(0);
    
    if (ctx.num_gpus() < 2) {
        GTEST_SKIP() << "Need at least 2 GPUs";
    }
    
    MultiGPULoRALayer layer(4, 4, 2, 1.0f, ctx);
    
    // Parameters should already be synced after creation
    auto& layer0 = layer.get_layer(0);
    auto& layer1 = layer.get_layer(1);
    
    auto params0 = layer0.parameters();
    auto params1 = layer1.parameters();
    
    EXPECT_EQ(params0.size(), params1.size());
}

// ===== Multi-GPU Trainer Tests =====

TEST(MultiGPUTrainerTest, Creation) {
    MultiGPUContext ctx(0);
    
    if (ctx.num_gpus() == 0) {
        GTEST_SKIP() << "No GPUs available";
    }
    
    MultiGPULoRATrainer::Config config;
    config.learning_rate = 0.01f;
    
    MultiGPULoRATrainer trainer(ctx, config);
    
    EXPECT_EQ(trainer.context().num_gpus(), ctx.num_gpus());
}

TEST(MultiGPUTrainerTest, BatchSharding) {
    MultiGPUContext ctx(2);
    
    if (ctx.num_gpus() < 2) {
        GTEST_SKIP() << "Need at least 2 GPUs";
    }
    
    // Create batch on CPU
    GPUTensor batch({8, 4}, Device::cpu());
    batch.fill(1.0f);
    
    auto shards = MultiGPULoRATrainer::shard_batch(batch, ctx);
    
    EXPECT_EQ(shards.size(), 2);
    EXPECT_EQ(shards[0].shape()[0], 4);  // 8 / 2 = 4
    EXPECT_EQ(shards[1].shape()[0], 4);
}

TEST(MultiGPUTrainerTest, TrainingStepSingleGPU) {
    MultiGPUContext ctx(1);
    
    if (ctx.num_gpus() == 0) {
        GTEST_SKIP() << "No GPUs available";
    }
    
    MultiGPULoRATrainer::Config config;
    config.learning_rate = 0.01f;
    
    MultiGPULoRATrainer trainer(ctx, config);
    
    auto layer = trainer.create_layer(4, 4, 2, 1.0f);
    
    // Create input and target
    GPUTensor input({2, 4}, ctx.get_device(0));
    input.fill(1.0f);
    
    GPUTensor target({2, 4}, ctx.get_device(0));
    target.fill(2.0f);
    
    std::vector<GPUTensor> inputs = {std::move(input)};
    std::vector<GPUTensor> targets = {std::move(target)};
    
    float loss = trainer.train_step(*layer, inputs, targets);
    
    EXPECT_GE(loss, 0.0f);
}

// ===== Distributed Data Loader Tests =====

class SimpleDataset : public DistributedDataLoader::Dataset {
public:
    SimpleDataset(size_t size, const std::vector<size_t>& shape)
        : size_(size), shape_(shape) {}
    
    GPUTensor get(size_t index) const override {
        GPUTensor tensor(shape_, Device::cpu());
        tensor.fill(static_cast<float>(index));
        return tensor;
    }
    
    size_t size() const override { return size_; }
    
private:
    size_t size_;
    std::vector<size_t> shape_;
};

TEST(DistributedDataLoaderTest, Creation) {
    MultiGPUContext ctx(1);
    
    if (ctx.num_gpus() == 0) {
        GTEST_SKIP() << "No GPUs available";
    }
    
    SimpleDataset dataset(100, {4});
    DistributedDataLoader loader(dataset, 10, ctx);
    
    EXPECT_EQ(loader.num_batches(), 10);
    EXPECT_EQ(loader.batch_size_per_gpu(), 10);
}

TEST(DistributedDataLoaderTest, Iteration) {
    MultiGPUContext ctx(1);
    
    if (ctx.num_gpus() == 0) {
        GTEST_SKIP() << "No GPUs available";
    }
    
    SimpleDataset dataset(20, {4});
    DistributedDataLoader loader(dataset, 5, ctx, false);  // No shuffle
    
    int batch_count = 0;
    for (auto& batch : loader) {
        EXPECT_EQ(batch.size(), 1);  // One shard per GPU
        batch_count++;
    }
    
    EXPECT_EQ(batch_count, 4);  // 20 / 5 = 4 batches
}

// ===== Integration Tests =====

TEST(MultiGPUIntegrationTest, EndToEndTrainingSingleGPU) {
    MultiGPUContext ctx(1);
    
    if (ctx.num_gpus() == 0) {
        GTEST_SKIP() << "No GPUs available";
    }
    
    // Create trainer
    MultiGPULoRATrainer::Config config;
    config.learning_rate = 0.01f;
    MultiGPULoRATrainer trainer(ctx, config);
    
    // Create layer
    auto layer = trainer.create_layer(8, 8, 4, 1.0f);
    
    // Training loop
    for (int step = 0; step < 10; ++step) {
        GPUTensor input({4, 8}, ctx.get_device(0));
        input.fill(1.0f);
        
        GPUTensor target({4, 8}, ctx.get_device(0));
        target.fill(0.5f);
        
        std::vector<GPUTensor> inputs = {std::move(input)};
        std::vector<GPUTensor> targets = {std::move(target)};
        
        float loss = trainer.train_step(*layer, inputs, targets);
        EXPECT_GE(loss, 0.0f);
    }
    
    // Check statistics
    auto stats = trainer.get_stats();
    EXPECT_EQ(stats.total_steps, 10);
}

TEST(MultiGPUIntegrationTest, GradientSynchronizationCorrectness) {
    MultiGPUContext ctx(0);
    
    if (ctx.num_gpus() < 2) {
        GTEST_SKIP() << "Need at least 2 GPUs for gradient sync test";
    }
    
    MultiGPULoRALayer layer(4, 4, 2, 1.0f, ctx, CommBackend::CUSTOM);
    
    // Create different inputs on each GPU
    std::vector<GPUTensor> inputs;
    for (int i = 0; i < ctx.num_gpus(); ++i) {
        GPUTensor input({2, 4}, ctx.get_device(i));
        input.fill(static_cast<float>(i + 1));
        inputs.push_back(std::move(input));
    }
    
    // Forward and backward
    auto outputs = layer.forward(inputs);
    
    std::vector<GPUTensor> grad_outputs;
    for (int i = 0; i < ctx.num_gpus(); ++i) {
        GPUTensor grad({2, 4}, ctx.get_device(i));
        grad.fill(1.0f);
        grad_outputs.push_back(std::move(grad));
    }
    
    layer.backward(grad_outputs);
    
    // Synchronize gradients
    EXPECT_TRUE(layer.synchronize_gradients());
    
    // After sync, all GPUs should have identical gradients
    auto& layer0 = layer.get_layer(0);
    auto grads0 = layer0.gradients();
    auto grads0_data = grads0[0]->cpu_data();
    
    for (int i = 1; i < ctx.num_gpus(); ++i) {
        auto& layeri = layer.get_layer(i);
        auto gradsi = layeri.gradients();
        auto gradsi_data = gradsi[0]->cpu_data();
        
        EXPECT_EQ(grads0_data.size(), gradsi_data.size());
        
        for (size_t j = 0; j < grads0_data.size(); ++j) {
            EXPECT_NEAR(grads0_data[j], gradsi_data[j], EPSILON);
        }
    }
}

// ===== Performance Tests =====

TEST(MultiGPUPerformanceTest, ScalingEfficiency) {
    MultiGPUContext ctx(0);
    
    if (ctx.num_gpus() < 2) {
        GTEST_SKIP() << "Need at least 2 GPUs for scaling test";
    }
    
    // Test with different GPU counts
    for (int num_gpus = 1; num_gpus <= std::min(4, ctx.num_gpus()); ++num_gpus) {
        MultiGPUContext test_ctx(num_gpus);
        
        MultiGPULoRATrainer::Config config;
        config.learning_rate = 0.01f;
        MultiGPULoRATrainer trainer(test_ctx, config);
        
        auto layer = trainer.create_layer(256, 256, 16, 1.0f);
        
        // Measure training time
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int step = 0; step < 5; ++step) {
            std::vector<GPUTensor> inputs;
            std::vector<GPUTensor> targets;
            
            for (int i = 0; i < num_gpus; ++i) {
                GPUTensor input({16, 256}, test_ctx.get_device(i));
                input.fill(1.0f);
                inputs.push_back(std::move(input));
                
                GPUTensor target({16, 256}, test_ctx.get_device(i));
                target.fill(0.5f);
                targets.push_back(std::move(target));
            }
            
            trainer.train_step(*layer, inputs, targets);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        float time_ms = std::chrono::duration<float, std::milli>(end - start).count();
        
        float avg_step_time = time_ms / 5.0f;
        
        std::cout << "GPUs: " << num_gpus 
                  << ", Avg step time: " << avg_step_time << " ms" << std::endl;
    }
}
