/**
 * @file bench_lora_framework.cpp
 * @brief Google Benchmark performance tests for LoRA framework components
 */

#ifndef THEMIS_BENCHMARK_BUILD
#define THEMIS_BENCHMARK_BUILD 1
#endif

#include <benchmark/benchmark.h>
#include "llm/lora_framework/lora_storage_service.h"
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/lora_orchestrator.h"

#include <chrono>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

using namespace themis::llm::lora;

namespace {

std::string makeBenchPath(const std::string& name) {
    return (std::filesystem::temp_directory_path() / ("themis_lora_bench_" + name)).string();
}

AdapterWeights generateTestWeights(int rank, size_t num_bytes) {
    AdapterWeights weights;
    weights.hyperparameters.rank = rank;
    weights.hyperparameters.alpha = 16.0f;
    weights.data.resize(num_bytes);
    weights.size_bytes = num_bytes;

    std::mt19937 generator(42);
    std::uniform_int_distribution<int> distribution(0, 255);
    for (auto& byte : weights.data) {
        byte = static_cast<uint8_t>(distribution(generator));
    }

    return weights;
}

AdapterMetadata generateTestMetadata(const std::string& adapter_id) {
    AdapterMetadata metadata;
    metadata.adapter_id = adapter_id;
    metadata.version = "v1";
    metadata.base_model = "llama-2-7b";
    metadata.description = "LoRA benchmark adapter";
    metadata.training_samples = 1000;
    metadata.validation_accuracy = 0.95f;
    metadata.created_at = std::chrono::system_clock::now();
    metadata.updated_at = metadata.created_at;
    return metadata;
}

TrainingData generateTrainingData(const std::string& dataset_name, int sample_count) {
    TrainingData data;
    data.dataset_name = dataset_name;
    data.samples.reserve(static_cast<size_t>(sample_count));

    for (int i = 0; i < sample_count; ++i) {
        TrainingDataSample sample;
        sample.input = "Question " + std::to_string(i);
        sample.output = "Answer " + std::to_string(i);
        data.samples.push_back(std::move(sample));
    }

    return data;
}

LoRAStorageService::Config createStorageConfig(const std::string& suffix) {
    LoRAStorageService::Config config;
    config.backend = LoRAStorageService::Backend::FileSystem;
    config.filesystem_path = makeBenchPath(suffix);
    config.enable_encryption = false;
    config.enable_signatures = false;
    return config;
}

}  // namespace

static void BM_Storage_SaveAdapter_64KB(benchmark::State& state) {
    LoRAStorageService storage(createStorageConfig("save_64kb"));
    size_t operation = 0;

    for (auto _ : state) {
        auto weights = generateTestWeights(8, 64 * 1024);
        auto adapter_id = "bench_save_" + std::to_string(operation++);
        auto metadata = generateTestMetadata(adapter_id);
        benchmark::DoNotOptimize(storage.saveAdapter(adapter_id, weights, metadata));
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * 64 * 1024);
}
BENCHMARK(BM_Storage_SaveAdapter_64KB);

static void BM_Storage_LoadMetadata(benchmark::State& state) {
    LoRAStorageService storage(createStorageConfig("load_meta"));

    const int precreated = static_cast<int>(state.range(0));
    for (int i = 0; i < precreated; ++i) {
        auto adapter_id = "bench_meta_" + std::to_string(i);
        auto weights = generateTestWeights(8, 8 * 1024);
        auto metadata = generateTestMetadata(adapter_id);
        storage.saveAdapter(adapter_id, weights, metadata);
    }

    int read_index = 0;
    for (auto _ : state) {
        auto adapter_id = "bench_meta_" + std::to_string(read_index % precreated);
        benchmark::DoNotOptimize(storage.loadMetadata(adapter_id));
        ++read_index;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Storage_LoadMetadata)->Range(1, 128);

static void BM_Storage_ListAdapters(benchmark::State& state) {
    LoRAStorageService storage(createStorageConfig("list_adapters"));

    const int precreated = static_cast<int>(state.range(0));
    for (int i = 0; i < precreated; ++i) {
        auto adapter_id = "bench_list_" + std::to_string(i);
        auto weights = generateTestWeights(4, 4 * 1024);
        auto metadata = generateTestMetadata(adapter_id);
        storage.saveAdapter(adapter_id, weights, metadata);
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(storage.listAdapters());
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Storage_ListAdapters)->Range(10, 1000);

static void BM_Training_OnTheFly_8Samples(benchmark::State& state) {
    LoRATrainingService::Config config;
    LoRATrainingService training(config);
    size_t operation = 0;

    for (auto _ : state) {
        auto adapter_id = "bench_train_otf_" + std::to_string(operation++);
        auto data = generateTrainingData("small_dataset", 8);
        auto result = training.trainOnTheFly(adapter_id, data);
        benchmark::DoNotOptimize(result.success);
    }

    state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_Training_OnTheFly_8Samples);

static void BM_Training_Batch_4x16(benchmark::State& state) {
    LoRATrainingService::Config config;
    LoRATrainingService training(config);
    size_t operation = 0;

    std::vector<TrainingData> batch;
    batch.reserve(4);
    for (int i = 0; i < 4; ++i) {
        batch.push_back(generateTrainingData("batch_" + std::to_string(i), 16));
    }

    for (auto _ : state) {
        auto adapter_id = "bench_train_batch_" + std::to_string(operation++);
        auto result = training.trainBatch(adapter_id, batch);
        benchmark::DoNotOptimize(result.success);
    }

    state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_Training_Batch_4x16);

static void BM_Orchestrator_HealthCheck(benchmark::State& state) {
    LoRAOrchestrator::Config config;
    config.storage_config = createStorageConfig("orch_health");
    config.enable_job_queue = false;
    config.enable_health_monitoring = true;

    LoRAOrchestrator orchestrator(config);

    for (auto _ : state) {
        benchmark::DoNotOptimize(orchestrator.healthCheck());
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Orchestrator_HealthCheck);

static void BM_Orchestrator_GetStats(benchmark::State& state) {
    LoRAOrchestrator::Config config;
    config.storage_config = createStorageConfig("orch_stats");
    config.enable_job_queue = false;

    LoRAOrchestrator orchestrator(config);

    for (auto _ : state) {
        auto stats = orchestrator.getStats();
        benchmark::DoNotOptimize(stats.dump().size());
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Orchestrator_GetStats);

BENCHMARK_MAIN();
