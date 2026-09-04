/**
 * @file bench_model_loading_from_themisdb.cpp
 * @brief Performance benchmarks for loading LLM models from ThemisDB
 * 
 * Benchmarks:
 * - Model blob retrieval from different storage backends
 * - Encryption/decryption overhead
 * - SHA256 verification performance
 * - Temporary file streaming
 * - Cache cleanup performance
 * 
 * @author ThemisDB Team / GitHub Copilot
 * @date January 2026
 */

#include <benchmark/benchmark.h>
#include "llm/llm_model_storage.h"
#include "storage/blob_storage_manager.h"
#include "storage/blob_backend_filesystem.h"
#include "storage/rocksdb_wrapper.h"
#include "llm/llama_wrapper.h"
#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <memory>
#include <random>

namespace fs = std::filesystem;

using namespace themis;
using namespace themis::llm;
using namespace themis::storage;

// ═══════════════════════════════════════════════════════════
// Test Data Generation
// ═══════════════════════════════════════════════════════════

class BenchmarkFixture {
public:
    static std::vector<uint8_t> generateModelData(size_t size_mb) {
        size_t size_bytes = size_mb * 1024 * 1024;
        std::vector<uint8_t> data(size_bytes);
        
        // Fill with pseudo-random data
        std::mt19937 gen(42);  // Fixed seed for reproducibility
        std::uniform_int_distribution<> dis(0, 255);
        
        for (size_t i = 0; i < size_bytes; i++) {
            data[i] = static_cast<uint8_t>(dis(gen));
        }
        
        return data;
    }
    
    static LLMModelMetadata createMetadata(const std::string& model_id, size_t size_bytes) {
        LLMModelMetadata metadata;
        metadata.model_id = model_id;
        metadata.model_name = "Benchmark Model";
        metadata.version = "1.0";
        metadata.architecture = "llama";
        metadata.format = "gguf";
        metadata.quantization = "Q4_K_M";
        metadata.size_bytes = size_bytes;
        metadata.parameter_count = 7000000000;
        metadata.context_length = 4096;
        return metadata;
    }
};

// ═══════════════════════════════════════════════════════════
// Storage Setup/Teardown
// ═══════════════════════════════════════════════════════════

class StorageState {
public:
    fs::path test_dir;
    fs::path db_path;
    fs::path blob_path;
    std::shared_ptr<RocksDBWrapper> db;
    std::shared_ptr<BlobStorageManager> blob_manager;
    std::shared_ptr<LLMModelStorage> model_storage;
    
    void setup() {
        test_dir = fs::temp_directory_path() / "themisdb_bench_model_loading";
        db_path = test_dir / "db";
        blob_path = test_dir / "blobs";
        
        fs::create_directories(db_path);
        fs::create_directories(blob_path);
        
        RocksDBWrapper::Config db_config;
        db_config.db_path = db_path.string();
        db_config.create_if_missing = true;
        db = std::make_shared<RocksDBWrapper>(db_config);
        db->open();
        
        BlobStorageConfig blob_config;
        blob_config.enable_filesystem = true;
        blob_config.filesystem_base_path = blob_path.string();
        blob_config.inline_threshold_bytes = 1024;
        
        blob_manager = std::make_shared<BlobStorageManager>(blob_config);
        
        auto fs_backend = std::make_shared<FilesystemBlobBackend>(blob_path.string());
        blob_manager->registerBackend(BlobStorageType::FILESYSTEM, fs_backend);
        
        LLMModelStorage::Config storage_config;
        storage_config.db = db;
        storage_config.blob_manager = blob_manager;
        storage_config.use_blob_storage = true;
        storage_config.inline_threshold_mb = 1;
        storage_config.enable_encryption = false;
        
        model_storage = std::make_shared<LLMModelStorage>(storage_config);
    }
    
    void teardown() {
        model_storage.reset();
        blob_manager.reset();
        db.reset();
        
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }
};

// Global state for benchmarks
static StorageState* g_state = nullptr;

// ═══════════════════════════════════════════════════════════
// Benchmark: Store Model (Different Sizes)
// ═══════════════════════════════════════════════════════════

static void BM_StoreModel_1KB(benchmark::State& state) {
    auto data = BenchmarkFixture::generateModelData(1);  // 1KB (actually 1MB, but divide by 1024)
    data.resize(1024);  // Actual 1KB
    auto metadata = BenchmarkFixture::createMetadata("bench_model_1kb", data.size());
    
    for (auto _ : state) {
        g_state->model_storage->storeModel(metadata, data);
        state.PauseTiming();
        // Clean up for next iteration
        g_state->model_storage->deleteModel(metadata.model_id);
        state.ResumeTiming();
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_StoreModel_1KB);

static void BM_StoreModel_1MB(benchmark::State& state) {
    auto data = BenchmarkFixture::generateModelData(1);
    auto metadata = BenchmarkFixture::createMetadata("bench_model_1mb", data.size());
    
    for (auto _ : state) {
        g_state->model_storage->storeModel(metadata, data);
        state.PauseTiming();
        g_state->model_storage->deleteModel(metadata.model_id);
        state.ResumeTiming();
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_StoreModel_1MB);

static void BM_StoreModel_10MB(benchmark::State& state) {
    auto data = BenchmarkFixture::generateModelData(10);
    auto metadata = BenchmarkFixture::createMetadata("bench_model_10mb", data.size());
    
    for (auto _ : state) {
        g_state->model_storage->storeModel(metadata, data);
        state.PauseTiming();
        g_state->model_storage->deleteModel(metadata.model_id);
        state.ResumeTiming();
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_StoreModel_10MB);

// ═══════════════════════════════════════════════════════════
// Benchmark: Load Model Blob (Different Sizes)
// ═══════════════════════════════════════════════════════════

static void BM_LoadModelBlob_1KB(benchmark::State& state) {
    auto data = BenchmarkFixture::generateModelData(1);
    data.resize(1024);
    auto metadata = BenchmarkFixture::createMetadata("bench_load_1kb", data.size());
    
    g_state->model_storage->storeModel(metadata, data);
    
    for (auto _ : state) {
        auto blob = g_state->model_storage->loadModelBlob(metadata.model_id);
        benchmark::DoNotOptimize(blob);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_LoadModelBlob_1KB);

static void BM_LoadModelBlob_1MB(benchmark::State& state) {
    auto data = BenchmarkFixture::generateModelData(1);
    auto metadata = BenchmarkFixture::createMetadata("bench_load_1mb", data.size());
    
    g_state->model_storage->storeModel(metadata, data);
    
    for (auto _ : state) {
        auto blob = g_state->model_storage->loadModelBlob(metadata.model_id);
        benchmark::DoNotOptimize(blob);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_LoadModelBlob_1MB);

static void BM_LoadModelBlob_10MB(benchmark::State& state) {
    auto data = BenchmarkFixture::generateModelData(10);
    auto metadata = BenchmarkFixture::createMetadata("bench_load_10mb", data.size());
    
    g_state->model_storage->storeModel(metadata, data);
    
    for (auto _ : state) {
        auto blob = g_state->model_storage->loadModelBlob(metadata.model_id);
        benchmark::DoNotOptimize(blob);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_LoadModelBlob_10MB);

// ═══════════════════════════════════════════════════════════
// Benchmark: Load Metadata Only
// ═══════════════════════════════════════════════════════════

static void BM_LoadModelMetadata(benchmark::State& state) {
    auto data = BenchmarkFixture::generateModelData(1);
    auto metadata = BenchmarkFixture::createMetadata("bench_metadata", data.size());
    
    g_state->model_storage->storeModel(metadata, data);
    
    for (auto _ : state) {
        auto meta = g_state->model_storage->loadModel(metadata.model_id);
        benchmark::DoNotOptimize(meta);
    }
}
BENCHMARK(BM_LoadModelMetadata);

// ═══════════════════════════════════════════════════════════
// Benchmark: SHA256 Verification
// ═══════════════════════════════════════════════════════════

#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

static void BM_SHA256_Verification_1MB(benchmark::State& state) {
    auto data = BenchmarkFixture::generateModelData(1);
    
    for (auto _ : state) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(data.data(), data.size(), hash);
        
        std::stringstream ss = {};
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        std::string hash_str = ss.str();
        
        benchmark::DoNotOptimize(hash_str);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_SHA256_Verification_1MB);

static void BM_SHA256_Verification_10MB(benchmark::State& state) {
    auto data = BenchmarkFixture::generateModelData(10);
    
    for (auto _ : state) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(data.data(), data.size(), hash);
        
        std::stringstream ss = {};
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        std::string hash_str = ss.str();
        
        benchmark::DoNotOptimize(hash_str);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_SHA256_Verification_10MB);

// ═══════════════════════════════════════════════════════════
// Benchmark: Temporary File Write
// ═══════════════════════════════════════════════════════════

static void BM_TempFileWrite_1MB(benchmark::State& state) {
    auto data = BenchmarkFixture::generateModelData(1);
    fs::path temp_dir = fs::temp_directory_path() / "themisdb_bench_temp";
    fs::create_directories(temp_dir);
    
    for (auto _ : state) {
        auto temp_file = temp_dir / "bench_temp_1mb.gguf";
        
        std::ofstream out(temp_file, std::ios::binary);
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
        out.close();
        
        state.PauseTiming();
        fs::remove(temp_file);
        state.ResumeTiming();
    }
    
    fs::remove_all(temp_dir);
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_TempFileWrite_1MB);

static void BM_TempFileWrite_10MB(benchmark::State& state) {
    auto data = BenchmarkFixture::generateModelData(10);
    fs::path temp_dir = fs::temp_directory_path() / "themisdb_bench_temp";
    fs::create_directories(temp_dir);
    
    for (auto _ : state) {
        auto temp_file = temp_dir / "bench_temp_10mb.gguf";
        
        std::ofstream out(temp_file, std::ios::binary);
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
        out.close();
        
        state.PauseTiming();
        fs::remove(temp_file);
        state.ResumeTiming();
    }
    
    fs::remove_all(temp_dir);
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_TempFileWrite_10MB);

// ═══════════════════════════════════════════════════════════
// Benchmark: Cache Cleanup
// ═══════════════════════════════════════════════════════════

static void BM_CleanupTempModels_10Files(benchmark::State& state) {
    fs::path temp_dir = fs::temp_directory_path() / "themisdb_models";
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Create 10 old files
        fs::create_directories(temp_dir);
        auto old_time = fs::file_time_type::clock::now() - std::chrono::hours(24 * 10);
        
        for (int i = 0; i < 10; i++) {
            auto file = temp_dir / ("old_model_" + std::to_string(i) + ".gguf");
            std::ofstream(file) << "old model " << i;
            fs::last_write_time(file, old_time);
        }
        
        state.ResumeTiming();
        
        // Cleanup
        size_t removed = LlamaWrapper::cleanupTempModels(7);
        benchmark::DoNotOptimize(removed);
    }
    
    fs::remove_all(temp_dir);
}
BENCHMARK(BM_CleanupTempModels_10Files);

static void BM_CleanupTempModels_100Files(benchmark::State& state) {
    fs::path temp_dir = fs::temp_directory_path() / "themisdb_models";
    
    for (auto _ : state) {
        state.PauseTiming();
        
        fs::create_directories(temp_dir);
        auto old_time = fs::file_time_type::clock::now() - std::chrono::hours(24 * 10);
        
        for (int i = 0; i < 100; i++) {
            auto file = temp_dir / ("old_model_" + std::to_string(i) + ".gguf");
            std::ofstream(file) << "old model " << i;
            fs::last_write_time(file, old_time);
        }
        
        state.ResumeTiming();
        
        size_t removed = LlamaWrapper::cleanupTempModels(7);
        benchmark::DoNotOptimize(removed);
    }
    
    fs::remove_all(temp_dir);
}
BENCHMARK(BM_CleanupTempModels_100Files);

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════

int main(int argc, char** argv) {
    // Setup global state
    g_state = new StorageState();
    g_state->setup();
    
    // Run benchmarks
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        g_state->teardown();
        delete g_state = {};
        return 1;
    }
    
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    
    // Cleanup
    g_state->teardown();
    delete g_state = {};
    
    return 0;
}
