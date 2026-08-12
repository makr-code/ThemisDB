#include <benchmark/benchmark.h>
#include "index/rotary_embeddings.h"
#include "index/vector_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include <numeric>
#include <filesystem>

using namespace themis;

// ============================================================================
// Single Rotation Benchmarks
// ============================================================================

static void BM_RotaryEmbedding_SingleRotation(benchmark::State& state) {
    size_t hidden_dim = state.range(0);
    
    RotationConfig config;
    config.hidden_dim = hidden_dim;
    config.num_rotation_pairs = hidden_dim / 2;
    config.base_theta = 10000.0;
    config.computeThetaCache();
    
    RotaryEmbedding rope(config);
    std::vector<float> embedding(hidden_dim, 1.0f);
    
    for (auto _ : state) {
        auto rotated = rope.rotate(embedding, 100);
        benchmark::DoNotOptimize(rotated);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * hidden_dim * sizeof(float));
}
BENCHMARK(BM_RotaryEmbedding_SingleRotation)
    ->Arg(128)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024)
    ->Arg(2048)
    ->Arg(4096)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Inverse Rotation Benchmarks
// ============================================================================

static void BM_RotaryEmbedding_InverseRotation(benchmark::State& state) {
    size_t hidden_dim = state.range(0);
    
    RotationConfig config;
    config.hidden_dim = hidden_dim;
    config.num_rotation_pairs = hidden_dim / 2;
    config.base_theta = 10000.0;
    config.computeThetaCache();
    
    RotaryEmbedding rope(config);
    std::vector<float> embedding(hidden_dim, 1.0f);
    
    for (auto _ : state) {
        auto rotated = rope.rotateInverse(embedding, 100);
        benchmark::DoNotOptimize(rotated);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * hidden_dim * sizeof(float));
}
BENCHMARK(BM_RotaryEmbedding_InverseRotation)
    ->Arg(128)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024)
    ->Arg(2048)
    ->Arg(4096)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Batch Rotation Benchmarks
// ============================================================================

static void BM_RotaryEmbedding_BatchRotation(benchmark::State& state) {
    size_t batch_size = state.range(0);
    size_t hidden_dim = 128;
    
    RotationConfig config;
    config.hidden_dim = hidden_dim;
    config.num_rotation_pairs = hidden_dim / 2;
    config.base_theta = 10000.0;
    config.computeThetaCache();
    
    RotaryEmbedding rope(config);
    std::vector<std::vector<float>> batch(batch_size, std::vector<float>(hidden_dim, 1.0f));
    std::vector<size_t> positions(batch_size);
    std::iota(positions.begin(), positions.end(), 0);
    
    for (auto _ : state) {
        auto rotated = rope.rotateBatch(batch, positions);
        benchmark::DoNotOptimize(rotated);
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetBytesProcessed(state.iterations() * batch_size * hidden_dim * sizeof(float));
}
BENCHMARK(BM_RotaryEmbedding_BatchRotation)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Relational Rotation Benchmarks
// ============================================================================

static void BM_RotaryEmbedding_RelationalRotation(benchmark::State& state) {
    size_t hidden_dim = 128;
    
    RotationConfig config;
    config.hidden_dim = hidden_dim;
    config.num_rotation_pairs = hidden_dim / 2;
    config.base_theta = 10000.0;
    config.computeThetaCache();
    
    RotaryEmbedding rope(config);
    std::vector<float> embedding(hidden_dim, 1.0f);
    
    for (auto _ : state) {
        auto rotated = rope.rotateRelational(embedding, "parent");
        benchmark::DoNotOptimize(rotated);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RotaryEmbedding_RelationalRotation)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Comparison: With vs Without Rotation
// ============================================================================

static void BM_VectorIndex_AddWithoutRotation(benchmark::State& state) {
    // Setup
    auto test_dir = std::filesystem::temp_directory_path() / "themis_rope_bench_no_rot";
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directories(test_dir);
    
    RocksDBWrapper::Config config;
    config.db_path = test_dir.string();
    config.create_if_missing = true;
    
    RocksDBWrapper db(config);
    if (!db.open()) {
        state.SkipWithError("Failed to open database");
        return;
    }
    
    VectorIndexManager vector_mgr(db);
    auto init_status = vector_mgr.init("test_vectors", 128, VectorIndexManager::Metric::COSINE);
    if (!init_status.ok) {
        state.SkipWithError("Failed to initialize vector index");
        return;
    }
    
    size_t doc_id = 0;
    std::string entity_id_base = "doc";
    for (auto _ : state) {
        std::string entity_id = entity_id_base + std::to_string(doc_id++);
        BaseEntity entity(entity_id);
        std::vector<float> embedding(128, 1.0f);
        entity.setField("embedding", embedding);
        
        auto status = vector_mgr.addEntity(entity, "embedding");
        if (!status.ok) {
            state.SkipWithError("Failed to add entity");
            break;
        }
    }
    
    state.SetItemsProcessed(state.iterations());
    
    // Cleanup
    std::filesystem::remove_all(test_dir);
}
BENCHMARK(BM_VectorIndex_AddWithoutRotation)
    ->Unit(benchmark::kMicrosecond);

static void BM_VectorIndex_AddWithRotation(benchmark::State& state) {
    // Setup
    auto test_dir = std::filesystem::temp_directory_path() / "themis_rope_bench_with_rot";
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directories(test_dir);
    
    RocksDBWrapper::Config db_config;
    db_config.db_path = test_dir.string();
    db_config.create_if_missing = true;
    
    RocksDBWrapper db(db_config);
    if (!db.open()) {
        state.SkipWithError("Failed to open database");
        return;
    }
    
    VectorIndexManager vector_mgr(db);
    auto init_status = vector_mgr.init("test_vectors", 128, VectorIndexManager::Metric::COSINE);
    if (!init_status.ok) {
        state.SkipWithError("Failed to initialize vector index");
        return;
    }
    
    // Enable rotary embeddings
    RotationConfig rope_config;
    rope_config.hidden_dim = 128;
    rope_config.num_rotation_pairs = 64;
    rope_config.base_theta = 10000.0;
    rope_config.computeThetaCache();
    
    auto rope_status = vector_mgr.setRotaryEmbeddingConfig(rope_config);
    if (!rope_status.ok) {
        state.SkipWithError("Failed to enable rotary embeddings");
        return;
    }
    
    size_t doc_id = 0;
    std::string entity_id_base = "doc";
    for (auto _ : state) {
        std::string entity_id = entity_id_base + std::to_string(doc_id);
        BaseEntity entity(entity_id);
        std::vector<float> embedding(128, 1.0f);
        entity.setField("embedding", embedding);
        
        auto status = vector_mgr.addEntityWithRotation(entity, "embedding", doc_id);
        doc_id++;
        
        if (!status.ok) {
            state.SkipWithError("Failed to add entity");
            break;
        }
    }
    
    state.SetItemsProcessed(state.iterations());
    
    // Cleanup
    std::filesystem::remove_all(test_dir);
}
BENCHMARK(BM_VectorIndex_AddWithRotation)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Search Benchmarks
// ============================================================================

static void BM_VectorIndex_SearchWithRotation(benchmark::State& state) {
    // Setup
    auto test_dir = std::filesystem::temp_directory_path() / "themis_rope_bench_search";
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directories(test_dir);
    
    RocksDBWrapper::Config db_config;
    db_config.db_path = test_dir.string();
    db_config.create_if_missing = true;
    
    RocksDBWrapper db(db_config);
    if (!db.open()) {
        state.SkipWithError("Failed to open database");
        return;
    }
    
    VectorIndexManager vector_mgr(db);
    auto init_status = vector_mgr.init("test_vectors", 128, VectorIndexManager::Metric::COSINE);
    if (!init_status.ok) {
        state.SkipWithError("Failed to initialize vector index");
        return;
    }
    
    // Enable rotary embeddings
    RotationConfig rope_config;
    rope_config.hidden_dim = 128;
    rope_config.num_rotation_pairs = 64;
    rope_config.base_theta = 10000.0;
    rope_config.computeThetaCache();
    
    auto rope_status = vector_mgr.setRotaryEmbeddingConfig(rope_config);
    if (!rope_status.ok) {
        state.SkipWithError("Failed to enable rotary embeddings");
        return;
    }
    
    // Add documents
    size_t num_docs = state.range(0);
    std::string entity_id_base = "doc";
    for (size_t i = 0; i < num_docs; ++i) {
        std::string entity_id = entity_id_base + std::to_string(i);
        BaseEntity entity(entity_id);
        std::vector<float> embedding(128);
        std::iota(embedding.begin(), embedding.end(), static_cast<float>(i));
        entity.setField("embedding", embedding);
        
        auto status = vector_mgr.addEntityWithRotation(entity, "embedding", i);
        if (!status.ok) {
            state.SkipWithError("Failed to add entity");
            return;
        }
    }
    
    // Query
    std::vector<float> query(128);
    std::iota(query.begin(), query.end(), 0.0f);
    
    for (auto _ : state) {
        auto [status, results] = vector_mgr.searchWithRotation(query, 10, 0);
        benchmark::DoNotOptimize(results);
        
        if (!status.ok) {
            state.SkipWithError("Search failed");
            break;
        }
    }
    
    state.SetItemsProcessed(state.iterations());
    
    // Cleanup
    std::filesystem::remove_all(test_dir);
}
BENCHMARK(BM_VectorIndex_SearchWithRotation)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Theta Cache Computation Benchmark
// ============================================================================

static void BM_RotationConfig_ThetaCacheComputation(benchmark::State& state) {
    size_t hidden_dim = state.range(0);
    
    RotationConfig config;
    config.hidden_dim = hidden_dim;
    config.num_rotation_pairs = hidden_dim / 2;
    config.base_theta = 10000.0;
    
    for (auto _ : state) {
        config.computeThetaCache();
        benchmark::DoNotOptimize(config.theta_cache);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RotationConfig_ThetaCacheComputation)
    ->Arg(128)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024)
    ->Arg(2048)
    ->Arg(4096)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Normalized Rotation Benchmark
// ============================================================================

static void BM_RotaryEmbedding_RotationWithNormalization(benchmark::State& state) {
    size_t hidden_dim = state.range(0);
    
    RotationConfig config;
    config.hidden_dim = hidden_dim;
    config.num_rotation_pairs = hidden_dim / 2;
    config.base_theta = 10000.0;
    config.normalize_after = true;
    config.computeThetaCache();
    
    RotaryEmbedding rope(config);
    std::vector<float> embedding(hidden_dim, 1.0f);
    
    for (auto _ : state) {
        auto rotated = rope.rotate(embedding, 100);
        benchmark::DoNotOptimize(rotated);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * hidden_dim * sizeof(float));
}
BENCHMARK(BM_RotaryEmbedding_RotationWithNormalization)
    ->Arg(128)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024)
    ->Unit(benchmark::kMicrosecond);

// Run benchmarks
BENCHMARK_MAIN();
