/**
 * ThemisDB RAID and LoRA Benchmarks
 * 
 * Performance benchmarks for:
 * - RAID modes (0, 1, 5, 10) with different data sizes
 * - LoRA loading, switching, fusion
 * - Multi-LoRA batch inference
 */

#include <benchmark/benchmark.h>
#include "sharding/redundancy_strategy.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "storage/blob_redundancy_manager.h"
#include "llm/multi_lora_manager.h"
#include <random>
#include <vector>
#include <map>

using namespace themis::sharding;
using themisdb::storage::BlobRedundancyManager;
using themisdb::storage::BlobType;
using themisdb::storage::BlobRedundancyConfig;
using namespace themis::llm;

// ═══════════════════════════════════════════════════════════
// Mock Storage for Benchmarks
// ═══════════════════════════════════════════════════════════

class BenchmarkStorage {
public:
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> storage;
    
    bool write(const std::string& shard, const std::string& key, 
               const std::vector<uint8_t>& data) {
        storage[shard][key] = data;
        return true;
    }
    
    std::optional<std::vector<uint8_t>> read(const std::string& shard, 
                                              const std::string& key) {
        if (storage.count(shard) && storage[shard].count(key)) {
            return storage[shard][key];
        }
        return std::nullopt;
    }
};

// ═══════════════════════════════════════════════════════════
// Helper Functions
// ═══════════════════════════════════════════════════════════

std::vector<uint8_t> generateRandomData(size_t size) {
    static std::mt19937 gen(42);
    static std::uniform_int_distribution<unsigned int> dist(0, 255);
    
    std::vector<uint8_t> data(size);
    for (auto& byte : data) {
        byte = static_cast<uint8_t>(dist(gen));
    }
    return data;
}

// ═══════════════════════════════════════════════════════════
// RAID 0 (STRIPE) Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_RAID0_Write_1KB(benchmark::State& state) {
    RedundancyConfig config;
    config.mode = RedundancyMode::STRIPE;
    config.stripe.stripe_size_kb = 64;
    
    RedundancyStrategy strategy(config);
    ConsistentHashRing ring(100);
    for (int i = 0; i < 6; ++i) {
        ring.addNode("shard-" + std::to_string(i));
    }
    ShardTopology topology;
    BenchmarkStorage storage;
    
    auto handler = [&storage](const std::string& shard, const std::string& key,
                              const std::vector<uint8_t>& data) {
        return storage.write(shard, key, data);
    };
    
    auto data = generateRandomData(1024);
    
    for (auto _ : state) {
        std::string doc_id = "doc-" + std::to_string(state.iterations());
        strategy.write(doc_id, data, "collection", ring, topology, handler);
    }
    
    state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_RAID0_Write_1KB);

static void BM_RAID0_Write_1MB(benchmark::State& state) {
    RedundancyConfig config;
    config.mode = RedundancyMode::STRIPE;
    config.stripe.stripe_size_kb = 64;
    
    RedundancyStrategy strategy(config);
    ConsistentHashRing ring(100);
    for (int i = 0; i < 6; ++i) {
        ring.addNode("shard-" + std::to_string(i));
    }
    ShardTopology topology;
    BenchmarkStorage storage;
    
    auto handler = [&storage](const std::string& shard, const std::string& key,
                              const std::vector<uint8_t>& data) {
        return storage.write(shard, key, data);
    };
    
    auto data = generateRandomData(1024 * 1024);
    
    for (auto _ : state) {
        std::string doc_id = "doc-" + std::to_string(state.iterations());
        strategy.write(doc_id, data, "collection", ring, topology, handler);
    }
    
    state.SetBytesProcessed(state.iterations() * 1024 * 1024);
}
BENCHMARK(BM_RAID0_Write_1MB);

// ═══════════════════════════════════════════════════════════
// RAID 1 (MIRROR) Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_RAID1_Write_RF3(benchmark::State& state) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    config.write_concern = WriteConcern::ALL;
    
    RedundancyStrategy strategy(config);
    ConsistentHashRing ring(100);
    for (int i = 0; i < 6; ++i) {
        ring.addNode("shard-" + std::to_string(i));
    }
    ShardTopology topology;
    BenchmarkStorage storage;
    
    auto handler = [&storage](const std::string& shard, const std::string& key,
                              const std::vector<uint8_t>& data) {
        return storage.write(shard, key, data);
    };
    
    auto data = generateRandomData(1024);
    
    for (auto _ : state) {
        std::string doc_id = "doc-" + std::to_string(state.iterations());
        strategy.write(doc_id, data, "collection", ring, topology, handler);
    }
    
    state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_RAID1_Write_RF3);

static void BM_RAID1_Read_Primary(benchmark::State& state) {
    RedundancyConfig config;
    config.mode = RedundancyMode::MIRROR;
    config.replication_factor = 3;
    config.read_preference = ReadPreference::PRIMARY;
    
    RedundancyStrategy strategy(config);
    ConsistentHashRing ring(100);
    for (int i = 0; i < 6; ++i) {
        ring.addNode("shard-" + std::to_string(i));
    }
    ShardTopology topology;
    BenchmarkStorage storage;
    
    auto write_handler = [&storage](const std::string& shard, const std::string& key,
                                     const std::vector<uint8_t>& data) {
        return storage.write(shard, key, data);
    };
    
    auto read_handler = [&storage](const std::string& shard, const std::string& key) {
        return storage.read(shard, key);
    };
    
    // Pre-populate data
    auto data = generateRandomData(1024);
    strategy.write("test-doc", data, "collection", ring, topology, write_handler);
    
    for (auto _ : state) {
        strategy.read("test-doc", "collection", ring, topology, read_handler);
    }
    
    state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_RAID1_Read_Primary);

// ═══════════════════════════════════════════════════════════
// RAID 5 (PARITY) Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_RAID5_Write_4Plus2(benchmark::State& state) {
    RedundancyConfig config;
    config.mode = RedundancyMode::PARITY;
    config.erasure_coding.data_shards = 4;
    config.erasure_coding.parity_shards = 2;
    
    RedundancyStrategy strategy(config);
    ConsistentHashRing ring(100);
    for (int i = 0; i < 8; ++i) {
        ring.addNode("shard-" + std::to_string(i));
    }
    ShardTopology topology;
    BenchmarkStorage storage;
    
    auto handler = [&storage](const std::string& shard, const std::string& key,
                              const std::vector<uint8_t>& data) {
        return storage.write(shard, key, data);
    };
    
    auto data = generateRandomData(4096);
    
    for (auto _ : state) {
        std::string doc_id = "doc-" + std::to_string(state.iterations());
        strategy.write(doc_id, data, "collection", ring, topology, handler);
    }
    
    state.SetBytesProcessed(state.iterations() * 4096);
}
BENCHMARK(BM_RAID5_Write_4Plus2);

// ═══════════════════════════════════════════════════════════
// RAID 10 (STRIPE_MIRROR) Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_RAID10_Write(benchmark::State& state) {
    RedundancyConfig config;
    config.mode = RedundancyMode::STRIPE_MIRROR;
    config.replication_factor = 2;
    config.stripe.stripe_size_kb = 64;
    
    RedundancyStrategy strategy(config);
    ConsistentHashRing ring(100);
    for (int i = 0; i < 8; ++i) {
        ring.addNode("shard-" + std::to_string(i));
    }
    ShardTopology topology;
    BenchmarkStorage storage;
    
    auto handler = [&storage](const std::string& shard, const std::string& key,
                              const std::vector<uint8_t>& data) {
        return storage.write(shard, key, data);
    };
    
    auto data = generateRandomData(1024 * 1024);
    
    for (auto _ : state) {
        std::string doc_id = "doc-" + std::to_string(state.iterations());
        strategy.write(doc_id, data, "collection", ring, topology, handler);
    }
    
    state.SetBytesProcessed(state.iterations() * 1024 * 1024);
}
BENCHMARK(BM_RAID10_Write);

// ═══════════════════════════════════════════════════════════
// Storage Efficiency Comparison
// ═══════════════════════════════════════════════════════════

static void BM_CompareRAIDModes_1MB(benchmark::State& state) {
    RedundancyMode modes[] = {
        RedundancyMode::STRIPE,
        RedundancyMode::MIRROR,
        RedundancyMode::PARITY,
        RedundancyMode::STRIPE_MIRROR
    };
    
    size_t mode_idx = state.range(0);
    
    RedundancyConfig config;
    config.mode = modes[mode_idx];
    config.replication_factor = 3;
    config.erasure_coding.data_shards = 4;
    config.erasure_coding.parity_shards = 2;
    
    RedundancyStrategy strategy(config);
    ConsistentHashRing ring(100);
    for (int i = 0; i < 8; ++i) {
        ring.addNode("shard-" + std::to_string(i));
    }
    ShardTopology topology;
    BenchmarkStorage storage;
    
    auto handler = [&storage](const std::string& shard, const std::string& key,
                              const std::vector<uint8_t>& data) {
        return storage.write(shard, key, data);
    };
    
    auto data = generateRandomData(1024 * 1024);
    
    for (auto _ : state) {
        std::string doc_id = "doc-" + std::to_string(state.iterations());
        strategy.write(doc_id, data, "collection", ring, topology, handler);
    }
    
    state.SetBytesProcessed(state.iterations() * 1024 * 1024);
    state.SetLabel(std::string("Mode ") + std::to_string(mode_idx));
}
BENCHMARK(BM_CompareRAIDModes_1MB)->DenseRange(0, 3);

// ═══════════════════════════════════════════════════════════
// LoRA Adapter Benchmarks
// ═══════════════════════════════════════════════════════════

#if defined(THEMIS_ENABLE_LLM) && THEMIS_ENABLE_LLM
static void BM_LoRA_LoadUnload(benchmark::State& state) {
    MultiLoRAManager::Config config;
    config.max_lora_vram_mb = 2048;
    config.max_lora_slots = 16;
    
    MultiLoRAManager manager(config);
    
    for (auto _ : state) {
        manager.loadLoRA("bench-lora", "/path/to/bench.bin", "base-model", 1.0f);
        benchmark::DoNotOptimize(manager.isLoRALoaded("bench-lora"));
        manager.unloadLoRA("bench-lora");
    }
}
BENCHMARK(BM_LoRA_LoadUnload);

static void BM_LoRA_CacheHit(benchmark::State& state) {
    MultiLoRAManager::Config config;
    config.max_lora_vram_mb = 2048;
    config.max_lora_slots = 16;
    
    MultiLoRAManager manager(config);
    manager.loadLoRA("cached-lora", "/path/to/cached.bin", "base-model", 1.0f);
    
    for (auto _ : state) {
        auto* lora = manager.getLoRA("cached-lora");
        benchmark::DoNotOptimize(lora);
    }
}
BENCHMARK(BM_LoRA_CacheHit);

static void BM_LoRA_BatchInference(benchmark::State& state) {
    MultiLoRAManager::Config config;
    config.enable_multi_lora_batch = true;
    config.max_lora_vram_mb = 2048;
    
    MultiLoRAManager manager(config);
    
    // Load LoRAs
    manager.loadLoRA("lora-a", "/path/to/a.bin", "base", 1.0f);
    manager.loadLoRA("lora-b", "/path/to/b.bin", "base", 1.0f);
    manager.loadLoRA("lora-c", "/path/to/c.bin", "base", 1.0f);
    
    size_t batch_size = state.range(0);
    
    for (auto _ : state) {
        std::vector<std::pair<InferenceRequest, std::string>> requests;
        
        for (size_t i = 0; i < batch_size; ++i) {
            InferenceRequest req;
            req.prompt = "Test prompt " + std::to_string(i);
            std::string lora = "lora-" + std::string(1, 'a' + (i % 3));
            requests.push_back({req, lora});
        }
        
        auto responses = manager.batchInferenceMultiLoRA(requests, nullptr);
        benchmark::DoNotOptimize(responses);
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_LoRA_BatchInference)->Range(1, 64);

static void BM_LoRA_Fusion(benchmark::State& state) {
    MultiLoRAManager::Config config;
    config.enable_adapter_fusion = true;
    config.max_lora_vram_mb = 4096;
    
    MultiLoRAManager manager(config);
    
    // Load source LoRAs
    manager.loadLoRA("lora-1", "/path/to/1.bin", "base", 1.0f);
    manager.loadLoRA("lora-2", "/path/to/2.bin", "base", 1.0f);
    manager.loadLoRA("lora-3", "/path/to/3.bin", "base", 1.0f);
    
    std::vector<std::string> sources = {"lora-1", "lora-2", "lora-3"};
    std::vector<float> weights = {0.5f, 0.3f, 0.2f};
    
    int fusion_count = 0;
    for (auto _ : state) {
        std::string fused_id = "fused-" + std::to_string(fusion_count++);
        bool result = manager.fuseLoRAs(sources, fused_id, weights);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_LoRA_Fusion);

static void BM_LoRA_SwitchingOverhead(benchmark::State& state) {
    MultiLoRAManager::Config config;
    config.max_lora_vram_mb = 2048;
    
    MultiLoRAManager manager(config);
    
    manager.loadLoRA("lora-a", "/path/to/a.bin", "base", 1.0f);
    manager.loadLoRA("lora-b", "/path/to/b.bin", "base", 1.0f);
    
    bool use_a = true;
    for (auto _ : state) {
        std::string lora_id = use_a ? "lora-a" : "lora-b";
        auto* lora = manager.getLoRA(lora_id);
        benchmark::DoNotOptimize(lora);
        use_a = !use_a;
    }
}
BENCHMARK(BM_LoRA_SwitchingOverhead);
#endif // THEMIS_ENABLE_LLM

// ═══════════════════════════════════════════════════════════
// Blob Redundancy Manager Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_BlobRegistration(benchmark::State& state) {
    BlobRedundancyManager::Config config;
    config.enable_blob_tracking = true;
    
    BlobRedundancyManager manager(config);
    
    int blob_count = 0;
    for (auto _ : state) {
        std::string path = "/path/to/blob" + std::to_string(blob_count++) + ".sst";
        manager.registerBlob(BlobType::SST_L0, path, 1024 * 1024, "test", "");
    }
}
BENCHMARK(BM_BlobRegistration);

static void BM_BlobHealthCheck(benchmark::State& state) {
    BlobRedundancyManager::Config config;
    BlobRedundancyManager manager(config);
    
    // Register some blobs
    std::vector<std::string> blob_ids = {};

    for (int i = 0; i < 100; ++i) {
        std::string path = "/path/to/blob" + std::to_string(i) + ".sst";
        auto id = manager.registerBlob(BlobType::SST_L1, path, 1024 * 1024, "", "");
        blob_ids.push_back(id);
    }
    
    size_t idx = 0;
    for (auto _ : state) {
        bool healthy = manager.verifyBlob(blob_ids[idx % blob_ids.size()]);
        benchmark::DoNotOptimize(healthy);
        idx++;
    }
}
BENCHMARK(BM_BlobHealthCheck);

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════

BENCHMARK_MAIN();
