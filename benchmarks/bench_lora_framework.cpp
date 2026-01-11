/**
 * @file bench_lora_framework.cpp
 * @brief Google Benchmark performance tests for LoRA Adapter Framework
 * 
 * Benchmarks:
 * - Adapter loading (cold start)
 * - Adapter hot-swapping
 * - Cache performance (hit rate)
 * - Training throughput
 * - Storage I/O performance
 * - Concurrent adapter operations
 * - Memory usage tracking
 * 
 * @note Requires Google Benchmark: vcpkg install benchmark OR apt-get install libbenchmark-dev
 * @build cmake -DTHEMIS_BUILD_BENCHMARKS=ON ..
 * @run ./benchmarks/bench_lora_framework
 * @run ./benchmarks/bench_lora_framework --benchmark_out=results.json
 */

#ifndef THEMIS_BENCHMARK_BUILD
#define THEMIS_BENCHMARK_BUILD 1
#endif

#include <benchmark/benchmark.h>
#include "llm/lora_framework/lora_adapter_manager.h"
#include "llm/lora_framework/lora_storage_service.h"
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/lora_orchestrator.h"
#include <memory>
#include <random>

using namespace themis::llm::lora;

// ============================================================================
// Test Data Generation
// ============================================================================

static AdapterWeights generateTestWeights(int rank, size_t num_weights) {
    AdapterWeights weights;
    weights.rank = rank;
    weights.alpha = 16.0;
    weights.weights_data.resize(num_weights);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    
    for (auto& w : weights.weights_data) {
        w = dis(gen);
    }
    
    return weights;
}

static AdapterMetadata generateTestMetadata(const std::string& adapter_id) {
    AdapterMetadata metadata;
    metadata.adapter_id = adapter_id;
    metadata.version = "v1.0";
    metadata.base_model = "llama-2-7b";
    metadata.description = "Benchmark test adapter";
    metadata.training_samples = 1000;
    metadata.validation_accuracy = 0.95;
    
    return metadata;
}

// ============================================================================
// Storage Benchmarks
// ============================================================================

static void BM_Storage_SaveAdapter_Rank8_1K(benchmark::State& state) {
    LoRAStorageService::Config config;
    config.storage_path = "/tmp/bench_lora_storage";
    config.enable_encryption = false;
    config.enable_signatures = false;
    
    LoRAStorageService storage(config);
    
    for (auto _ : state) {
        auto weights = generateTestWeights(8, 1024);
        auto metadata = generateTestMetadata("bench_adapter_" + std::to_string(state.iterations()));
        
        benchmark::DoNotOptimize(storage.saveAdapter(metadata.adapter_id, weights, metadata));
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Storage_SaveAdapter_Rank8_1K);

static void BM_Storage_SaveAdapter_Rank16_1M(benchmark::State& state) {
    LoRAStorageService::Config config;
    config.storage_path = "/tmp/bench_lora_storage";
    config.enable_encryption = false;
    config.enable_signatures = false;
    
    LoRAStorageService storage(config);
    
    for (auto _ : state) {
        auto weights = generateTestWeights(16, 1024 * 1024);
        auto metadata = generateTestMetadata("bench_adapter_" + std::to_string(state.iterations()));
        
        benchmark::DoNotOptimize(storage.saveAdapter(metadata.adapter_id, weights, metadata));
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * 1024 * 1024 * sizeof(float));
}
BENCHMARK(BM_Storage_SaveAdapter_Rank16_1M);

static void BM_Storage_LoadAdapter(benchmark::State& state) {
    LoRAStorageService::Config config;
    config.storage_path = "/tmp/bench_lora_storage";
    config.enable_encryption = false;
    config.enable_signatures = false;
    
    LoRAStorageService storage(config);
    
    // Pre-create adapters
    for (int i = 0; i < state.range(0); i++) {
        auto weights = generateTestWeights(8, 1024);
        auto metadata = generateTestMetadata("bench_adapter_" + std::to_string(i));
        storage.saveAdapter(metadata.adapter_id, weights, metadata);
    }
    
    int adapter_idx = 0;
    for (auto _ : state) {
        std::string adapter_id = "bench_adapter_" + std::to_string(adapter_idx % state.range(0));
        benchmark::DoNotOptimize(storage.getAdapterInfo(adapter_id));
        adapter_idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Storage_LoadAdapter)->Range(1, 100);

static void BM_Storage_WithEncryption(benchmark::State& state) {
    LoRAStorageService::Config config;
    config.storage_path = "/tmp/bench_lora_storage_encrypted";
    config.enable_encryption = true;  // Enable encryption
    config.enable_signatures = true;  // Enable signatures
    
    LoRAStorageService storage(config);
    
    for (auto _ : state) {
        auto weights = generateTestWeights(8, 1024);
        auto metadata = generateTestMetadata("bench_adapter_encrypted_" + std::to_string(state.iterations()));
        
        benchmark::DoNotOptimize(storage.saveAdapter(metadata.adapter_id, weights, metadata));
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Storage_WithEncryption);

// ============================================================================
// Adapter Manager Benchmarks
// ============================================================================

static void BM_Manager_LoadAdapter(benchmark::State& state) {
    LoRAStorageService::Config storage_config;
    storage_config.storage_path = "/tmp/bench_lora_storage";
    storage_config.enable_encryption = false;
    auto storage = std::make_shared<LoRAStorageService>(storage_config);
    
    // Pre-create adapters
    for (int i = 0; i < state.range(0); i++) {
        auto weights = generateTestWeights(8, 1024);
        auto metadata = generateTestMetadata("bench_adapter_" + std::to_string(i));
        storage->saveAdapter(metadata.adapter_id, weights, metadata);
    }
    
    LoRAAdapterManager::Config manager_config;
    manager_config.cache_size = 10;
    LoRAAdapterManager manager(manager_config, storage);
    
    int adapter_idx = 0;
    for (auto _ : state) {
        std::string adapter_id = "bench_adapter_" + std::to_string(adapter_idx % state.range(0));
        benchmark::DoNotOptimize(manager.loadAdapter(adapter_id));
        adapter_idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Manager_LoadAdapter)->Range(1, 50);

static void BM_Manager_HotSwap(benchmark::State& state) {
    LoRAStorageService::Config storage_config;
    storage_config.storage_path = "/tmp/bench_lora_storage";
    auto storage = std::make_shared<LoRAStorageService>(storage_config);
    
    // Create two adapters
    auto weights1 = generateTestWeights(8, 1024);
    auto metadata1 = generateTestMetadata("adapter1");
    storage->saveAdapter("adapter1", weights1, metadata1);
    
    auto weights2 = generateTestWeights(8, 1024);
    auto metadata2 = generateTestMetadata("adapter2");
    storage->saveAdapter("adapter2", weights2, metadata2);
    
    LoRAAdapterManager::Config manager_config;
    manager_config.cache_size = 10;
    LoRAAdapterManager manager(manager_config, storage);
    
    manager.loadAdapter("adapter1");
    
    for (auto _ : state) {
        if (state.iterations() % 2 == 0) {
            benchmark::DoNotOptimize(manager.switchAdapter("adapter1", "adapter2"));
        } else {
            benchmark::DoNotOptimize(manager.switchAdapter("adapter2", "adapter1"));
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Manager_HotSwap);

static void BM_Manager_CacheHitRate(benchmark::State& state) {
    LoRAStorageService::Config storage_config;
    storage_config.storage_path = "/tmp/bench_lora_storage";
    auto storage = std::make_shared<LoRAStorageService>(storage_config);
    
    // Create adapters
    const int num_adapters = 20;
    for (int i = 0; i < num_adapters; i++) {
        auto weights = generateTestWeights(8, 1024);
        auto metadata = generateTestMetadata("adapter_" + std::to_string(i));
        storage->saveAdapter(metadata.adapter_id, weights, metadata);
    }
    
    LoRAAdapterManager::Config manager_config;
    manager_config.cache_size = state.range(0);  // Variable cache size
    LoRAAdapterManager manager(manager_config, storage);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, num_adapters - 1);
    
    for (auto _ : state) {
        std::string adapter_id = "adapter_" + std::to_string(dis(gen));
        benchmark::DoNotOptimize(manager.loadAdapter(adapter_id));
    }
    
    auto stats = manager.getCacheStats();
    state.counters["CacheHitRate"] = static_cast<double>(stats.hits) / (stats.hits + stats.misses);
    state.counters["Evictions"] = stats.evictions;
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Manager_CacheHitRate)->Range(5, 20);

// ============================================================================
// Training Service Benchmarks
// ============================================================================

static void BM_Training_OnTheFly_SmallDataset(benchmark::State& state) {
    LoRATrainingService::Config config;
    config.default_rank = 8;
    config.default_alpha = 16.0;
    LoRATrainingService training(config);
    
    TrainingData data;
    data.adapter_id = "bench_adapter";
    data.base_model = "llama-2-7b";
    for (int i = 0; i < 10; i++) {
        data.training_samples.push_back({"Question " + std::to_string(i), "Answer " + std::to_string(i)});
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(training.trainOnTheFly(data));
    }
    
    state.SetItemsProcessed(state.iterations() * 10);
}
BENCHMARK(BM_Training_OnTheFly_SmallDataset);

static void BM_Training_Batch_LargeDataset(benchmark::State& state) {
    LoRATrainingService::Config config;
    config.default_rank = 8;
    LoRATrainingService training(config);
    
    std::vector<TrainingData> dataset;
    for (int i = 0; i < state.range(0); i++) {
        TrainingData data;
        data.adapter_id = "bench_adapter_" + std::to_string(i);
        data.base_model = "llama-2-7b";
        for (int j = 0; j < 100; j++) {
            data.training_samples.push_back({"Q" + std::to_string(j), "A" + std::to_string(j)});
        }
        dataset.push_back(data);
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(training.trainBatch(dataset));
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0) * 100);
}
BENCHMARK(BM_Training_Batch_LargeDataset)->Range(10, 100);

// ============================================================================
// Orchestrator Benchmarks
// ============================================================================

static void BM_Orchestrator_CreateAdapter(benchmark::State& state) {
    LoRAStorageService::Config storage_config;
    storage_config.storage_path = "/tmp/bench_lora_storage";
    auto storage = std::make_shared<LoRAStorageService>(storage_config);
    
    LoRAAdapterManager::Config manager_config;
    auto manager = std::make_shared<LoRAAdapterManager>(manager_config, storage);
    
    LoRATrainingService::Config training_config;
    auto training = std::make_shared<LoRATrainingService>(training_config);
    
    LoRAAuditLogger::Config audit_config;
    audit_config.log_path = "/tmp/bench_audit.jsonl";
    auto audit = std::make_shared<LoRAAuditLogger>(audit_config);
    
    LoRAOrchestrator orchestrator(storage, manager, training, audit);
    
    for (auto _ : state) {
        TrainingData data;
        data.adapter_id = "bench_adapter_" + std::to_string(state.iterations());
        data.base_model = "llama-2-7b";
        data.training_samples = {{"Q", "A"}};
        
        benchmark::DoNotOptimize(orchestrator.createAdapter(data.adapter_id, data));
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Orchestrator_CreateAdapter);

static void BM_Orchestrator_CRUD_Operations(benchmark::State& state) {
    LoRAStorageService::Config storage_config;
    storage_config.storage_path = "/tmp/bench_lora_storage";
    auto storage = std::make_shared<LoRAStorageService>(storage_config);
    
    LoRAAdapterManager::Config manager_config;
    auto manager = std::make_shared<LoRAAdapterManager>(manager_config, storage);
    
    LoRATrainingService::Config training_config;
    auto training = std::make_shared<LoRATrainingService>(training_config);
    
    LoRAAuditLogger::Config audit_config;
    audit_config.log_path = "/tmp/bench_audit.jsonl";
    auto audit = std::make_shared<LoRAAuditLogger>(audit_config);
    
    LoRAOrchestrator orchestrator(storage, manager, training, audit);
    
    // Pre-create adapters
    for (int i = 0; i < state.range(0); i++) {
        TrainingData data;
        data.adapter_id = "crud_adapter_" + std::to_string(i);
        data.base_model = "llama-2-7b";
        data.training_samples = {{"Q", "A"}};
        orchestrator.createAdapter(data.adapter_id, data);
    }
    
    int operation = 0;
    for (auto _ : state) {
        int adapter_idx = operation % state.range(0);
        std::string adapter_id = "crud_adapter_" + std::to_string(adapter_idx);
        
        // Cycle through CRUD operations
        switch (operation % 4) {
            case 0:  // Create (skip if exists)
                break;
            case 1:  // Read
                benchmark::DoNotOptimize(orchestrator.getAdapter(adapter_id));
                break;
            case 2:  // Update
                {
                    TrainingData update_data;
                    update_data.adapter_id = adapter_id;
                    update_data.base_model = "llama-2-7b";
                    update_data.training_samples = {{"Q2", "A2"}};
                    benchmark::DoNotOptimize(orchestrator.updateAdapter(adapter_id, update_data));
                }
                break;
            case 3:  // Delete (skip)
                break;
        }
        
        operation++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Orchestrator_CRUD_Operations)->Range(10, 100);

// ============================================================================
// Concurrent Operations Benchmarks
// ============================================================================

static void BM_Concurrent_AdapterLoading(benchmark::State& state) {
    LoRAStorageService::Config storage_config;
    storage_config.storage_path = "/tmp/bench_lora_storage";
    auto storage = std::make_shared<LoRAStorageService>(storage_config);
    
    // Pre-create adapters
    for (int i = 0; i < 100; i++) {
        auto weights = generateTestWeights(8, 1024);
        auto metadata = generateTestMetadata("concurrent_adapter_" + std::to_string(i));
        storage->saveAdapter(metadata.adapter_id, weights, metadata);
    }
    
    LoRAAdapterManager::Config manager_config;
    manager_config.cache_size = 50;
    auto manager = std::make_shared<LoRAAdapterManager>(manager_config, storage);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 99);
    
    for (auto _ : state) {
        std::string adapter_id = "concurrent_adapter_" + std::to_string(dis(gen));
        benchmark::DoNotOptimize(manager->loadAdapter(adapter_id));
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Concurrent_AdapterLoading)->ThreadRange(1, 8);

// ============================================================================
// Memory Usage Benchmarks
// ============================================================================

static void BM_Memory_AdapterFootprint(benchmark::State& state) {
    LoRAStorageService::Config storage_config;
    storage_config.storage_path = "/tmp/bench_lora_storage";
    auto storage = std::make_shared<LoRAStorageService>(storage_config);
    
    LoRAAdapterManager::Config manager_config;
    manager_config.cache_size = state.range(0);
    auto manager = std::make_shared<LoRAAdapterManager>(manager_config, storage);
    
    // Pre-create adapters
    for (int i = 0; i < state.range(0); i++) {
        auto weights = generateTestWeights(8, 1024 * 1024);  // 1M weights each
        auto metadata = generateTestMetadata("memory_adapter_" + std::to_string(i));
        storage->saveAdapter(metadata.adapter_id, weights, metadata);
    }
    
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); i++) {
            std::string adapter_id = "memory_adapter_" + std::to_string(i);
            manager->loadAdapter(adapter_id);
        }
    }
    
    // Estimate memory usage
    size_t estimated_memory = state.range(0) * 1024 * 1024 * sizeof(float);
    state.counters["MemoryMB"] = estimated_memory / (1024 * 1024);
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Memory_AdapterFootprint)->Range(1, 16);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
