#if 1
#include <benchmark/benchmark.h>
#include "bench_fixtures.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include <optional>
#include <random>
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>
#include <unordered_set>

/**
 * YCSB (Yahoo! Cloud Serving Benchmark) Lite for ThemisDB
 *
 * Based on "Benchmarking Cloud Serving Systems with YCSB"
 * Cooper et al., SoCC 2010
 * https://research.yahoo.com/files/ycsb.pdf
 *
 * Implements the 6 core YCSB workloads:
 * - Workload A: Update heavy (50% read, 50% update)
 * - Workload B: Read mostly (95% read, 5% update)
 * - Workload C: Read only (100% read)
 * - Workload D: Read latest (95% read, 5% insert)
 * - Workload E: Short ranges (95% scan, 5% insert)
 * - Workload F: Read-modify-write (50% read, 50% read-modify-write)
 *
 * Dataset / Warmup parameters:
 * - Arg(0): record_count (e.g. 10000, 100000, 1000000)
 * - Warmup: full dataset loaded in SetUp() before measurement starts.
 * - Each record: 10 fields × 100 bytes = ~1 KB.
 *
 * Performance targets (8-core, 32GB RAM, NVMe):
 * - Workload A: 100,000-150,000 ops/sec
 * - Workload B: 150,000-200,000 ops/sec
 * - Workload C: 200,000-300,000 ops/sec
 *
 * CI artifacts: exported via --benchmark_out=<file> --benchmark_out_format=json
 * Baseline reference: artifacts/perf_nv/targeted_validation/bench_ycsb_targeted_v2.json
 */

namespace {
    // YCSB Constants
    constexpr int DEFAULT_RECORD_COUNT = 10000;
    constexpr int FIELD_COUNT = 10;
    constexpr int FIELD_LENGTH = 100;
    
    // Deterministic, fixed seed for reproducible benchmark measurements.
    // Seeding from std::random_device is intentionally avoided here.
    std::mt19937 rng{themis::bench::kCanonicalRngSeed};
    
    std::string makeRandomString(size_t len) {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);
        std::string s;
        s.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            s += charset[dist(rng)];
        }
        return s;
    }
    
    void cleanupTestDB(const std::string& path) {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
    
    // Zipfian distribution for key access
    // Models realistic workload where some keys are accessed more frequently
    class ZipfianGenerator {
    public:
        explicit ZipfianGenerator(int64_t num_items, double zipfian_constant = 0.99)
            : num_items_(num_items), theta_(zipfian_constant) {
            zeta_n_ = zeta(num_items, theta_);
            zeta_2_ = zeta(2, theta_);
            alpha_ = 1.0 / (1.0 - theta_);
            eta_ = (1.0 - std::pow(2.0 / num_items, 1.0 - theta_)) / (1.0 - zeta_2_ / zeta_n_);
        }
        
        int64_t next() {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            double u = dist(rng);
            double uz = u * zeta_n_;
            
            if (uz < 1.0) {
              return 0;
            }
            if (uz < 1.0 + std::pow(0.5, theta_)) {
              return 1;
            }
            
            int64_t ret = static_cast<int64_t>(num_items_ * std::pow(eta_ * u - eta_ + 1.0, alpha_));
            return std::min(ret, num_items_ - 1);
        }
        
    private:
        double zeta(int64_t n, double theta) {
            double sum = 0.0;
            for (int64_t i = 1; i <= n; ++i) {
                sum += 1.0 / std::pow(i, theta);
            }
            return sum;
        }
        
        int64_t num_items_;
        double theta_;
        double zeta_n_;
        double zeta_2_;
        double alpha_;
        double eta_;
    };
}

/**
 * YCSB Lite Benchmark Fixture
 *
 * Loads initial dataset and provides workload implementations.
 *
 * Benchmark parameter:
 *   state.range(0) = record_count
 *     10,000   = quick CI validation
 *    100,000   = standard benchmark
 *  1,000,000   = full-scale (matches "YCSBLiteFixture/WorkloadC_ReadOnly/1000000"
 *                in PERFORMANCE_EXPECTATIONS.md §5.8)
 */
class YCSBLiteFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Use the OS temp directory so the path is valid regardless of the
        // working directory, and include a steady-clock timestamp for
        // uniqueness across concurrent or repeated runs.
        db_path_ = (std::filesystem::temp_directory_path() /
                    ("themis_bench_ycsb_" +
                     std::to_string(std::chrono::steady_clock::now()
                                        .time_since_epoch()
                                        .count())))
                       .string();
        cleanupTestDB(db_path_);
        
        // Configure database for YCSB workload
        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_;
        config.compression_default = "lz4";
        config.compression_bottommost = "zstd";
        config.block_cache_size_mb = 512;
        config.db_write_buffer_size_mb = 64;
        
        db_ = std::make_unique<themis::RocksDBWrapper>(config);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        secondary_ = std::make_unique<themis::SecondaryIndexManager>(*db_);
        
        // Record count from benchmark parameter (default: 10000)
        record_count_ = state.range(0);
        
        // Create index on primary key
        secondary_->createIndex("usertable", "user_id", true);
        
        // Load initial dataset
        loadData();
        
        // Initialize Zipfian generator for hot key distribution
        zipfian_ = std::make_unique<ZipfianGenerator>(record_count_);
    }
    
    void TearDown(const ::benchmark::State&) override {
        zipfian_.reset();
        secondary_.reset();
        db_.reset();
        cleanupTestDB(db_path_);
    }
    
protected:
    void loadData() {
        for (int64_t i = 0; i < record_count_; ++i) {
            std::string key = "user" + std::to_string(i);
            themis::BaseEntity entity(key);
            entity.setField("user_id", static_cast<int64_t>(i));
            
            // YCSB has 10 fields per record, each 100 bytes
            for (int field = 0; field < FIELD_COUNT; ++field) {
                entity.setField("field" + std::to_string(field), makeRandomString(FIELD_LENGTH));
            }
            
            secondary_->put("usertable", entity);
        }
    }
    
    std::string getKeyZipfian() {
        int64_t key_num = zipfian_->next();
        return "user" + std::to_string(key_num);
    }
    
    std::string getKeyUniform() {
        std::uniform_int_distribution<int64_t> dist(0, record_count_ - 1);
        return "user" + std::to_string(dist(rng));
    }
    
    std::string getKeyLatest() {
        // Access most recently inserted keys with exponential distribution
        std::exponential_distribution<double> dist(1.0);
        double skew = dist(rng);
        int64_t key_num = record_count_ - 1 - static_cast<int64_t>(skew * record_count_ / 10.0);
        if (key_num < 0) {
            key_num = 0;
        } else if (key_num >= record_count_) {
            key_num = record_count_ - 1;
        }
        return "user" + std::to_string(key_num);
    }

    std::optional<themis::BaseEntity> loadEntity(const std::string& table, const std::string& pk) {
        const auto entity_key = themis::KeySchema::makeRelationalKey(table, pk);
        auto blob = db_->get(entity_key);
        if (!blob) {
            return std::nullopt;
        }
        return themis::BaseEntity::deserialize(pk, *blob);
    }
    
    void doRead(const std::string& key) {
        auto entity_opt = loadEntity("usertable", key);
        benchmark::DoNotOptimize(entity_opt);
    }
    
    void doUpdate(const std::string& key) {
        auto entity_opt = loadEntity("usertable", key);
        if (entity_opt) {
            // Update one random field
            std::uniform_int_distribution<int> field_dist(0, FIELD_COUNT - 1);
            int field_num = field_dist(rng);
            entity_opt->setField("field" + std::to_string(field_num), makeRandomString(FIELD_LENGTH));
            secondary_->put("usertable", *entity_opt);
        }
    }
    
    void doInsert(int64_t key_num) {
        std::string key = "user" + std::to_string(key_num);
        themis::BaseEntity entity(key);
        entity.setField("user_id", key_num);
        
        for (int field = 0; field < FIELD_COUNT; ++field) {
            entity.setField("field" + std::to_string(field), makeRandomString(FIELD_LENGTH));
        }
        
        secondary_->put("usertable", entity);
    }
    
    void doScan(const std::string& start_key, int scan_length) {
        // Simplified scan: read scan_length consecutive records
        for (int i = 0; i < scan_length; ++i) {
            auto entity_opt = loadEntity("usertable", start_key);
            benchmark::DoNotOptimize(entity_opt);
        }
    }
    
    void doReadModifyWrite(const std::string& key) {
        // Read-modify-write: read, modify, write back
        auto entity_opt = loadEntity("usertable", key);
        if (entity_opt) {
            // Modify one field
            entity_opt->setField("field0", makeRandomString(FIELD_LENGTH));
            secondary_->put("usertable", *entity_opt);
        }
    }
    
    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> secondary_;
    std::unique_ptr<ZipfianGenerator> zipfian_;
    int64_t record_count_;
    int64_t insert_counter_{0};
};

/**
 * YCSB Workload A: Update Heavy
 *
 * 50% reads, 50% updates
 * Application: Session store recording recent actions
 *
 * Expected: 100,000-150,000 ops/sec
 */
BENCHMARK_DEFINE_F(YCSBLiteFixture, WorkloadA)(benchmark::State& state) {
    std::uniform_int_distribution<int> op_dist(0, 99);
    
    for (auto _ : state) {
        std::string key = getKeyZipfian();
        
        if (op_dist(rng) < 50) {
            doRead(key);
        } else {
            doUpdate(key);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

/**
 * YCSB Workload B: Read Mostly
 *
 * 95% reads, 5% updates
 * Application: Photo tagging; add a tag is an update, but most operations are to read tags
 *
 * Expected: 150,000-200,000 ops/sec
 */
BENCHMARK_DEFINE_F(YCSBLiteFixture, WorkloadB)(benchmark::State& state) {
    std::uniform_int_distribution<int> op_dist(0, 99);
    
    for (auto _ : state) {
        std::string key = getKeyZipfian();
        
        if (op_dist(rng) < 95) {
            doRead(key);
        } else {
            doUpdate(key);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

/**
 * YCSB Workload C: Read Only
 *
 * 100% reads
 * Application: User profile cache, where profiles are constructed elsewhere
 *
 * Expected: 200,000-300,000 ops/sec (highest throughput)
 */
BENCHMARK_DEFINE_F(YCSBLiteFixture, WorkloadC)(benchmark::State& state) {
    for (auto _ : state) {
        std::string key = getKeyZipfian();
        doRead(key);
    }
    
    state.SetItemsProcessed(state.iterations());
}

/**
 * YCSB Workload C Read-Only (canonical CI configuration)
 *
 * Matches the reference entry "YCSBLiteFixture/WorkloadC_ReadOnly/1000000" in
 * PERFORMANCE_EXPECTATIONS.md §5.8.  Pure read path, Zipfian key distribution.
 * Arg(0): record_count (1,000,000 for the canonical CI reference).
 *
 * Expected: ~200,000-300,000 ops/sec
 */
BENCHMARK_DEFINE_F(YCSBLiteFixture, WorkloadC_ReadOnly)(benchmark::State& state) {
    for (auto _ : state) {
        std::string key = getKeyZipfian();
        doRead(key);
    }
    
    state.SetItemsProcessed(state.iterations());
}

/**
 * YCSB Workload D: Read Latest
 *
 * 95% reads, 5% inserts
 * New records are inserted, and the most recently inserted records are the most popular
 * Application: User status updates; people want to read the latest statuses
 *
 * Expected: Similar to Workload B
 */
BENCHMARK_DEFINE_F(YCSBLiteFixture, WorkloadD)(benchmark::State& state) {
    std::uniform_int_distribution<int> op_dist(0, 99);
    int64_t insert_key = record_count_;
    
    for (auto _ : state) {
        if (op_dist(rng) < 95) {
            std::string key = getKeyLatest();
            doRead(key);
        } else {
            doInsert(insert_key++);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

/**
 * YCSB Workload E: Short Ranges
 *
 * 95% scans, 5% inserts
 * Application: Threaded conversations, where each scan is for the posts in a given thread
 *
 * Expected: Lower throughput due to scan cost
 */
BENCHMARK_DEFINE_F(YCSBLiteFixture, WorkloadE)(benchmark::State& state) {
    std::uniform_int_distribution<int> op_dist(0, 99);
    std::uniform_int_distribution<int> scan_len_dist(1, 100);
    int64_t insert_key = record_count_;
    
    for (auto _ : state) {
        if (op_dist(rng) < 95) {
            std::string start_key = getKeyUniform();
            int scan_length = scan_len_dist(rng);
            doScan(start_key, scan_length);
        } else {
            doInsert(insert_key++);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

/**
 * YCSB Workload F: Read-Modify-Write
 *
 * 50% reads, 50% read-modify-write
 * Application: User database, where user records are read and modified by the user or to record user activity
 *
 * Expected: Similar to Workload A but slightly lower due to RMW overhead
 */
BENCHMARK_DEFINE_F(YCSBLiteFixture, WorkloadF)(benchmark::State& state) {
    std::uniform_int_distribution<int> op_dist(0, 99);
    
    for (auto _ : state) {
        std::string key = getKeyZipfian();
        
        if (op_dist(rng) < 50) {
            doRead(key);
        } else {
            doReadModifyWrite(key);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ─────────────────────────── Registration ────────────────────────────────────
// Arg(0): Number of records
//   10,000  = quick CI validation
//  100,000  = standard benchmark
// 1,000,000 = full-scale (canonical CI reference for WorkloadC_ReadOnly)

BENCHMARK_REGISTER_F(YCSBLiteFixture, WorkloadA)
    ->Arg(10000)
    ->Arg(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(YCSBLiteFixture, WorkloadB)
    ->Arg(10000)
    ->Arg(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(YCSBLiteFixture, WorkloadC)
    ->Arg(10000)
    ->Arg(100000)
    ->Unit(benchmark::kMicrosecond);

// Canonical CI reference point (matches PERFORMANCE_EXPECTATIONS.md §5.8):
BENCHMARK_REGISTER_F(YCSBLiteFixture, WorkloadC_ReadOnly)
    ->Arg(1000000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(YCSBLiteFixture, WorkloadD)
    ->Arg(10000)
    ->Arg(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(YCSBLiteFixture, WorkloadE)
    ->Arg(10000)
    ->Arg(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(YCSBLiteFixture, WorkloadF)
    ->Arg(10000)
    ->Arg(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
#endif
