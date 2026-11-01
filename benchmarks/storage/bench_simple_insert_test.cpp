// Simple test to debug insert issues
#include <benchmark/benchmark.h>
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "../benchmark_cleanup.h"
#include <filesystem>
#include <iostream>

using namespace themis;

static void BM_SimpleInsert(benchmark::State& state) {
    const std::string DB_PATH = "simple_test_db";
    
    std::error_code cleanup_error;
    if (!themis::bench::removeBenchmarkDirectory(DB_PATH, cleanup_error)) {
        state.SkipWithError(("Failed to clear SimpleInsert DB: " + cleanup_error.message()).c_str());
        return;
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
    {
        SecondaryIndexManager indexMgr(db);

        auto status = indexMgr.createIndex("users", "email", false);
        if (!status.ok) {
            std::cerr << "Failed to create email index: " << status.message << std::endl;
        }

        status = indexMgr.createIndex("users", "username", true);
        if (!status.ok) {
            std::cerr << "Failed to create username index: " << status.message << std::endl;
        }

        for (auto _ : state) {
            BaseEntity entity("test_user_1");
            entity.setField("email", "test@example.com");
            entity.setField("username", "testuser");
            entity.setField("age", "25");

            status = indexMgr.put("users", entity);
            if (!status.ok) {
                std::cerr << "Insert failed: " << status.message << std::endl;
                state.SkipWithError(("Insert failed: " + status.message).c_str());
                break;
            }

            indexMgr.erase("users", "test_user_1");
        }
    }
    
    // Cleanup
    db.close();
    if (!themis::bench::removeBenchmarkDirectory(DB_PATH, cleanup_error)) {
        std::fprintf(stderr, "SimpleInsert DB cleanup failed: %s\n", cleanup_error.message().c_str());
    }
}

BENCHMARK(BM_SimpleInsert);
BENCHMARK_MAIN();
