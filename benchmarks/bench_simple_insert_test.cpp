/*
 * ThemisDB | File: bench_simple_insert_test.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 99/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Simple test to debug insert issues
#include <benchmark/benchmark.h>
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include <filesystem>
#include <iostream>

using namespace themis;

static void BM_SimpleInsert(benchmark::State& state) {
    const std::string DB_PATH = "simple_test_db";
    
    // Clean up
    if (std::filesystem::exists(DB_PATH)) {
        std::filesystem::remove_all(DB_PATH);
    }
    
    // Open database
    RocksDBWrapper::Config config;
    config.db_path = DB_PATH;
    config.compression_default = "lz4";
    config.block_cache_size_mb = 256;
    config.disable_wal_for_benchmark = false;  // Enable WAL for safety
    config.memtable_size_mb = 256;
    config.max_write_buffer_number = 3;
    config.allow_concurrent_memtable_write = true;
    config.enable_statistics = false;
    config.enable_blobdb = false;
    
    RocksDBWrapper db(config);
    if (!db.open()) {
        std::cerr << "Failed to open database" << std::endl;
        state.SkipWithError("Failed to open database");
        return;
    }
    
    SecondaryIndexManager indexMgr(db);
    
    // Create indexes
    auto st = indexMgr.createIndex("users", "email", false);
    if (!st.ok) {
        std::cerr << "Failed to create email index: " << st.message << std::endl;
    }
    
    st = indexMgr.createIndex("users", "username", true);
    if (!st.ok) {
        std::cerr << "Failed to create username index: " << st.message << std::endl;
    }
    
    for (auto _ : state) {
        // Create entity
        BaseEntity entity("test_user_1");
        entity.setField("email", "test@example.com");
        entity.setField("username", "testuser");
        entity.setField("age", "25");
        
        // Try to insert
        auto status = indexMgr.put("users", entity);
        if (!status.ok) {
            std::cerr << "Insert failed: " << status.message << std::endl;
            state.SkipWithError(("Insert failed: " + status.message).c_str());
            return;
        }
        
        // Clean up for next iteration
        indexMgr.erase("users", "test_user_1");
    }
    
    // Cleanup
    db.close();
    if (std::filesystem::exists(DB_PATH)) {
        std::filesystem::remove_all(DB_PATH);
    }
}

BENCHMARK(BM_SimpleInsert);
BENCHMARK_MAIN();
