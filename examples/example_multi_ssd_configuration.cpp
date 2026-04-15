/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_multi_ssd_configuration.cpp                ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:08:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     241                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Example: Multi-SSD Configuration for ThemisDB
 * 
 * This example demonstrates how to configure ThemisDB to use multiple SSDs
 * for maximum throughput. It shows:
 * 1. Separate WAL directory on dedicated SSD
 * 2. Multiple paths for SSTable distribution
 * 3. Performance tuning for multi-SSD setups
 */

#include "storage/rocksdb_wrapper.h"
#include <iostream>
#include <chrono>
#include <vector>

using namespace themis;
using namespace std::chrono;

void example_single_ssd() {
    std::cout << "\n=== Example 1: Single SSD (Default) ===\n";
    
    RocksDBWrapper::Config config;
    config.db_path = "/data/themisdb_single";
    config.enable_wal = true;
    
    // No wal_dir = WAL stored under db_path
    // No db_paths = all SSTables in db_path
    
    std::cout << "Configuration:\n";
    std::cout << "  DB Path: " << config.db_path << "\n";
    std::cout << "  WAL Dir: <default under db_path>\n";
    std::cout << "  DB Paths: <single path>\n";
    std::cout << "  Use Case: Development, small datasets\n";
}

void example_two_ssd_wal_separation() {
    std::cout << "\n=== Example 2: 2 SSDs - WAL Separation ===\n";
    
    RocksDBWrapper::Config config;
    config.db_path = "/mnt/nvme0/themisdb";
    config.wal_dir = "/mnt/nvme1/themisdb_wal";  // Separate WAL!
    config.enable_wal = true;
    
    // Performance tuning for separate WAL
    config.max_background_jobs = 8;
    config.memtable_size_mb = 256;
    config.block_cache_size_mb = 1024;
    
    std::cout << "Configuration:\n";
    std::cout << "  DB Path: " << config.db_path << "\n";
    std::cout << "  WAL Dir: " << config.wal_dir << " (SEPARATE!)\n";
    std::cout << "  DB Paths: <single path>\n";
    std::cout << "  Benefits:\n";
    std::cout << "    - I/O isolation (WAL vs compaction)\n";
    std::cout << "    - Better P99 latency consistency\n";
    std::cout << "    - Separate wear leveling\n";
    std::cout << "  Use Case: Write-intensive, latency-sensitive\n";
}

void example_multi_ssd_high_performance() {
    std::cout << "\n=== Example 3: 6 SSDs - Maximum Throughput ===\n";
    
    RocksDBWrapper::Config config;
    config.db_path = "/mnt/nvme0/themisdb";
    config.wal_dir = "/mnt/nvme_wal/themisdb_wal";  // Dedicated fastest SSD
    
    // Distribute SSTables across 5 SSDs
    config.db_paths = {
        {"/mnt/nvme0/themisdb", 1ULL * 1024 * 1024 * 1024 * 1024},  // 1 TB
        {"/mnt/nvme1/themisdb", 1ULL * 1024 * 1024 * 1024 * 1024},  // 1 TB
        {"/mnt/nvme2/themisdb", 1ULL * 1024 * 1024 * 1024 * 1024},  // 1 TB
        {"/mnt/nvme3/themisdb", 1ULL * 1024 * 1024 * 1024 * 1024},  // 1 TB
        {"/mnt/nvme4/themisdb", 1ULL * 1024 * 1024 * 1024 * 1024}   // 1 TB
    };
    
    config.enable_wal = true;
    
    // High parallelism for multi-SSD
    config.max_background_jobs = 20;
    config.max_background_compactions = 12;
    config.max_background_flushes = 4;
    config.max_subcompactions = 4;
    config.background_threads_low = 12;
    config.background_threads_high = 4;
    
    // Enable auto-tuning
    config.enable_high_parallel_tuning = true;
    
    // Larger memory for high throughput
    config.memtable_size_mb = 512;
    config.block_cache_size_mb = 4096;
    config.block_cache_shard_bits = 6;  // 64 shards
    config.db_write_buffer_size_mb = 2048;
    
    std::cout << "Configuration:\n";
    std::cout << "  DB Path: " << config.db_path << "\n";
    std::cout << "  WAL Dir: " << config.wal_dir << " (fastest SSD)\n";
    std::cout << "  DB Paths: " << config.db_paths.size() << " paths\n";
    for (size_t i = 0; i < config.db_paths.size(); ++i) {
        std::cout << "    [" << i << "] " << config.db_paths[i].path 
                  << " (" << (config.db_paths[i].target_size_bytes / 1024 / 1024 / 1024) << " GB)\n";
    }
    std::cout << "  Parallelism:\n";
    std::cout << "    - Background jobs: " << config.max_background_jobs << "\n";
    std::cout << "    - Subcompactions: " << config.max_subcompactions << "\n";
    std::cout << "    - Compaction threads: " << config.background_threads_low << "\n";
    std::cout << "  Expected Performance:\n";
    std::cout << "    - Write: ~400-500 MB/s (3-4x single SSD)\n";
    std::cout << "    - Read: ~2500-3000 MB/s (3-4x single SSD)\n";
    std::cout << "    - P99 Latency: <10ms\n";
    std::cout << "  Use Case: Large-scale production, hyperscaler\n";
}

void example_raid_comparison() {
    std::cout << "\n=== Example 4: Why NOT to use RAID ===\n";
    
    std::cout << "\n❌ RAID 0 for WAL:\n";
    std::cout << "  Problem: WAL is sequential, single writer\n";
    std::cout << "  Throughput gain: <10% (not worth complexity)\n";
    std::cout << "  Recommendation: Single fast NVMe is optimal\n";
    
    std::cout << "\n❌ RAID 5/6 for SSTables:\n";
    std::cout << "  Problem: Parity overhead + RocksDB compaction = very slow\n";
    std::cout << "  Throughput loss: -40% write performance\n";
    std::cout << "  Recommendation: Use RocksDB db_paths instead\n";
    
    std::cout << "\n✅ RocksDB db_paths (no RAID):\n";
    std::cout << "  Benefit: Native multi-path support\n";
    std::cout << "  Throughput gain: +300% (3-4x improvement)\n";
    std::cout << "  Complexity: Low (no RAID controller needed)\n";
    std::cout << "  Flexibility: SSDs can be replaced individually\n";
    std::cout << "  Recommendation: Always prefer over OS-level RAID\n";
}

void benchmark_write_throughput(RocksDBWrapper& db, const std::string& label) {
    const int num_writes = 10000;
    const int value_size = 1024;  // 1 KB values
    
    std::vector<uint8_t> value(value_size, 'x');
    
    auto start = high_resolution_clock::now();
    
    for (int i = 0; i < num_writes; ++i) {
        std::string key = "key_" + std::to_string(i);
        db.put(key, value);
    }
    
    auto end = high_resolution_clock::now();
    auto duration_ms = duration_cast<milliseconds>(end - start).count();
    
    double throughput_mbs = (num_writes * value_size) / 1024.0 / 1024.0 / (duration_ms / 1000.0);
    
    std::cout << "\n" << label << ":\n";
    std::cout << "  Writes: " << num_writes << "\n";
    std::cout << "  Duration: " << duration_ms << " ms\n";
    std::cout << "  Throughput: " << throughput_mbs << " MB/s\n";
}

int main() {
    std::cout << "==============================================\n";
    std::cout << "ThemisDB Multi-SSD Configuration Examples\n";
    std::cout << "==============================================\n";
    
    // Show configuration examples
    example_single_ssd();
    example_two_ssd_wal_separation();
    example_multi_ssd_high_performance();
    example_raid_comparison();
    
    std::cout << "\n==============================================\n";
    std::cout << "Key Takeaways\n";
    std::cout << "==============================================\n";
    std::cout << "1. WAL cannot be distributed (sequential writes)\n";
    std::cout << "2. Separate WAL SSD improves latency consistency\n";
    std::cout << "3. SSTables CAN be distributed (db_paths)\n";
    std::cout << "4. Multi-SSD gives 3-4x throughput improvement\n";
    std::cout << "5. RocksDB db_paths is better than RAID 0\n";
    std::cout << "6. Avoid RAID 5/6 (performance loss)\n";
    std::cout << "\n";
    
    std::cout << "\nNote: This example demonstrates configurations.\n";
    std::cout << "To run actual benchmarks, uncomment the benchmark code below\n";
    std::cout << "and update paths to match your system:\n";
    std::cout << "1. Create mount points: /mnt/nvme0, /mnt/nvme1, etc.\n";
    std::cout << "2. Update db_path values in the examples\n";
    std::cout << "3. Uncomment the benchmark calls below\n";
    std::cout << "4. Recompile and run\n";
    
    /*
    // Example: Uncomment to run actual benchmark
    std::cout << "\n=== Running Benchmarks ===\n";
    
    // Single SSD benchmark
    {
        RocksDBWrapper::Config cfg;
        cfg.db_path = "/tmp/bench_single";
        RocksDBWrapper db(cfg);
        if (db.open()) {
            benchmark_write_throughput(db, "Single SSD");
            db.close();
        }
    }
    
    // 2 SSD benchmark (separate WAL)
    {
        RocksDBWrapper::Config cfg;
        cfg.db_path = "/tmp/bench_2ssd";
        cfg.wal_dir = "/tmp/bench_2ssd_wal";
        RocksDBWrapper db(cfg);
        if (db.open()) {
            benchmark_write_throughput(db, "2 SSDs (WAL separate)");
            db.close();
        }
    }
    */
    
    return 0;
}
