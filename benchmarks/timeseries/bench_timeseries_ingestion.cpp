// Benchmark: Timeseries Ingestion Performance
// Measures time-series write throughput and compression efficiency

#include "timeseries/timeseries.h"
#include "timeseries/gorilla.h"
#include "storage/rocksdb_wrapper.h"
#include <benchmark/benchmark.h>
#include <filesystem>
#include <random>
#include <cmath>
#include <chrono>

using namespace themis;

// ============================================================================
// Test Setup
// ============================================================================

class TimeseriesBenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Use an absolute, per-fixture unique path to avoid Windows path parsing
        // edge cases and cross-thread collisions in benchmark runs.
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        tls_test_db_path_ = std::string("C:\\tmp\\bench_ts_") +
            std::to_string(state.thread_index()) + "_" +
            std::to_string(now);
        std::error_code ec;
        std::filesystem::remove_all(tls_test_db_path_, ec);
        std::filesystem::create_directories(tls_test_db_path_);
        
        // Create RocksDB wrapper
        RocksDBWrapper::Config config;
        config.db_path = tls_test_db_path_;
        config.memtable_size_mb = 256;
        config.block_cache_size_mb = 512;
        
        tls_db_ = std::make_unique<RocksDBWrapper>(config);
        if (!tls_db_->open()) {
            throw std::runtime_error("Failed to open database");
        }
        
        // Create timeseries store
        tls_ts_store_ = std::make_unique<TimeSeriesStore>(tls_db_->getRawDB(), nullptr);
    }
    
    void TearDown(const ::benchmark::State& /*state*/) override {
        tls_ts_store_.reset();
        if (tls_db_) {
            tls_db_->close();
        }
        tls_db_.reset();
        
        // Clean up test database
        std::error_code ec;
        std::filesystem::remove_all(tls_test_db_path_, ec);
        tls_test_db_path_.clear();
    }

protected:
    static TimeSeriesStore* tsStore() {
        return tls_ts_store_.get();
    }

private:
    static thread_local std::string tls_test_db_path_;
    static thread_local std::unique_ptr<RocksDBWrapper> tls_db_;
    static thread_local std::unique_ptr<TimeSeriesStore> tls_ts_store_;
};

thread_local std::string TimeseriesBenchmarkFixture::tls_test_db_path_;
thread_local std::unique_ptr<RocksDBWrapper> TimeseriesBenchmarkFixture::tls_db_;
thread_local std::unique_ptr<TimeSeriesStore> TimeseriesBenchmarkFixture::tls_ts_store_;

// ============================================================================
// Benchmark: Raw Data Ingestion
// ============================================================================

BENCHMARK_DEFINE_F(TimeseriesBenchmarkFixture, RawDataIngestion)(benchmark::State& state) {
    const std::string metric = "cpu_usage";
    const std::string entity = "server_1";
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> value_dist(0.0, 100.0);
    
    int64_t timestamp = 1700000000000; // Start timestamp
    size_t points_written = 0;
    
    for (auto _ : state) {
        TimeSeriesStore::DataPoint point;
        point.timestamp_ms = timestamp++;
        point.value = value_dist(rng);
        
        bool success = tsStore()->put(metric, entity, point);
        benchmark::DoNotOptimize(success);
        
        if (!success) {
            state.SkipWithError("Failed to write data point");
        }
        
        points_written++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()), benchmark::Counter::kIsRate);
    state.counters["total_points"] = static_cast<double>(points_written);
}

BENCHMARK_REGISTER_F(TimeseriesBenchmarkFixture, RawDataIngestion)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Batch Ingestion
// ============================================================================

BENCHMARK_DEFINE_F(TimeseriesBenchmarkFixture, BatchIngestion)(benchmark::State& state) {
    const int batch_size = state.range(0);
    const std::string metric = "memory_usage";
    const std::string entity = "server_2";
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> value_dist(0.0, 16384.0); // MB
    
    int64_t timestamp = 1700000000000;
    
    for (auto _ : state) {
        std::vector<TimeSeriesStore::DataPoint> batch;
        batch.reserve(batch_size);
        
        for (int i = 0; i < batch_size; i++) {
            TimeSeriesStore::DataPoint point;
            point.timestamp_ms = timestamp++;
            point.value = value_dist(rng);
            batch.push_back(point);
        }
        
        // Write batch
        for (const auto& point : batch) {
            tsStore()->put(metric, entity, point);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.counters["batch_size"] = static_cast<double>(batch_size);
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * batch_size), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(TimeseriesBenchmarkFixture, BatchIngestion)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Multiple Metrics/Entities
// ============================================================================

BENCHMARK_DEFINE_F(TimeseriesBenchmarkFixture, MultipleMetrics)(benchmark::State& state) {
    const int num_entities = state.range(0);
    
    std::vector<std::string> metrics = {"cpu", "memory", "disk_io", "network_in", "network_out"};
    std::vector<std::string> entities = {};

    for (int i = 0; i < num_entities; i++) {
        entities.push_back("server_" + std::to_string(i));
    }
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> value_dist(0.0, 100.0);
    std::uniform_int_distribution<int> metric_dist(0, metrics.size() - 1);
    std::uniform_int_distribution<int> entity_dist(0, entities.size() - 1);
    
    int64_t timestamp = 1700000000000;
    
    for (auto _ : state) {
        TimeSeriesStore::DataPoint point;
        point.timestamp_ms = timestamp++;
        point.value = value_dist(rng);
        
        const std::string& metric = metrics[metric_dist(rng)];
        const std::string& entity = entities[entity_dist(rng)];
        
        tsStore()->put(metric, entity, point);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["num_entities"] = static_cast<double>(num_entities);
    state.counters["num_metrics"] = static_cast<double>(metrics.size());
}

BENCHMARK_REGISTER_F(TimeseriesBenchmarkFixture, MultipleMetrics)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Gorilla Compression
// ============================================================================

static void BM_GorillaCompression(benchmark::State& state) {
    const int num_points = state.range(0);
    
    std::mt19937 rng(42);
    // Generate realistic sensor data with gradual changes
    std::normal_distribution<double> change_dist(0.0, 1.0);
    
    std::vector<double> values;
    std::vector<int64_t> timestamps;
    
    double current_value = 20.0; // Start at 20°C
    int64_t current_timestamp = 1700000000000;
    
    for (int i = 0; i < num_points; i++) {
        current_value += change_dist(rng); // Small changes
        current_timestamp += 1000; // 1 second intervals
        
        values.push_back(current_value);
        timestamps.push_back(current_timestamp);
    }
    
    for (auto _ : state) {
        GorillaEncoder encoder;
        
        // Compress data
        for (size_t i = 0; i < values.size(); i++) {
            encoder.add(timestamps[i], values[i]);
        }
        
        auto compressed = encoder.finish();
        benchmark::DoNotOptimize(compressed);
        
        // Calculate compression ratio
        size_t raw_size = num_points * (sizeof(int64_t) + sizeof(double));
        size_t compressed_size = compressed.size();
        double compression_ratio = static_cast<double>(raw_size) / compressed_size;
        
        state.counters["raw_bytes"] = static_cast<double>(raw_size);
        state.counters["compressed_bytes"] = static_cast<double>(compressed_size);
        state.counters["compression_ratio"] = compression_ratio;
    }
    
    state.SetItemsProcessed(state.iterations() * num_points);
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * num_points), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GorillaCompression)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Gorilla Decompression
// ============================================================================

static void BM_GorillaDecompression(benchmark::State& state) {
    const int num_points = state.range(0);
    
    // Generate and compress data first
    GorillaEncoder encoder;
    int64_t timestamp = 1700000000000;
    double value = 20.0;
    
    std::mt19937 rng(42);
    std::normal_distribution<double> change_dist(0.0, 1.0);
    
    for (int i = 0; i < num_points; i++) {
        value += change_dist(rng);
        timestamp += 1000;
        encoder.add(timestamp, value);
    }
    
    auto compressed = encoder.finish();
    
    for (auto _ : state) {
        GorillaDecoder decoder(compressed);
        
        std::vector<std::pair<int64_t, double>> decompressed;
        decompressed.reserve(num_points);
        
        while (auto point = decoder.next()) {
            decompressed.push_back(*point);
        }
        
        benchmark::DoNotOptimize(decompressed);
        
        if (decompressed.size() != static_cast<size_t>(num_points)) {
            state.SkipWithError("Decompression failed");
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_points);
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * num_points), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GorillaDecompression)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Time-Range Query Performance
// ============================================================================

BENCHMARK_DEFINE_F(TimeseriesBenchmarkFixture, TimeRangeQuery)(benchmark::State& state) {
    const std::string metric = "temperature";
    const std::string entity = "sensor_1";
    
    // Pre-populate with 10,000 data points
    const int total_points = 10000;
    int64_t base_timestamp = 1700000000000;
    
    std::mt19937 rng(42);
    std::normal_distribution<double> value_dist(20.0, 5.0);
    
    for (int i = 0; i < total_points; i++) {
        TimeSeriesStore::DataPoint point;
        point.timestamp_ms = base_timestamp + (i * 1000); // 1 second intervals
        point.value = value_dist(rng);
        tsStore()->put(metric, entity, point);
    }
    
    // Query different time ranges
    const int range_size = state.range(0); // in seconds
    
    for (auto _ : state) {
        int64_t start_time = base_timestamp;
        int64_t end_time = start_time + (range_size * 1000);
        
        TimeSeriesStore::RangeQuery rq;
        rq.from_ms = start_time;
        rq.to_ms = end_time;
        auto results = tsStore()->query(metric, entity, rq);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["range_seconds"] = static_cast<double>(range_size);
}

BENCHMARK_REGISTER_F(TimeseriesBenchmarkFixture, TimeRangeQuery)
    ->Arg(60)      // 1 minute
    ->Arg(300)     // 5 minutes
    ->Arg(3600)    // 1 hour
    ->Arg(86400)   // 1 day
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Downsampling Performance
// ============================================================================

BENCHMARK_DEFINE_F(TimeseriesBenchmarkFixture, Downsampling)(benchmark::State& state) {
    const std::string metric = "requests_per_sec";
    const std::string entity = "app_server";
    
    // Pre-populate with high-resolution data (1 point per second for 1 hour)
    const int total_points = 3600;
    int64_t base_timestamp = 1700000000000;
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> value_dist(100.0, 1000.0);
    
    for (int i = 0; i < total_points; i++) {
        TimeSeriesStore::DataPoint point;
        point.timestamp_ms = base_timestamp + (i * 1000);
        point.value = value_dist(rng);
        tsStore()->put(metric, entity, point);
    }
    
    const int downsample_interval = state.range(0); // in seconds
    
    for (auto _ : state) {
        int64_t start_time = base_timestamp;
        int64_t end_time = base_timestamp + (3600 * 1000);
        
        // Aggregate over the requested time range
        TimeSeriesStore::RangeQuery rq;
        rq.from_ms = start_time;
        rq.to_ms = end_time;
        auto aggregated = tsStore()->aggregate(metric, entity, rq);
        
        benchmark::DoNotOptimize(aggregated);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["downsample_interval_sec"] = static_cast<double>(downsample_interval);
}

BENCHMARK_REGISTER_F(TimeseriesBenchmarkFixture, Downsampling)
    ->Arg(60)      // 1 minute
    ->Arg(300)     // 5 minutes
    ->Arg(3600)    // 1 hour
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Out-of-Order Writes
// ============================================================================

BENCHMARK_DEFINE_F(TimeseriesBenchmarkFixture, OutOfOrderWrites)(benchmark::State& state) {
    const std::string metric = "latency";
    const std::string entity = "service_1";
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> value_dist(0.0, 100.0);
    std::uniform_int_distribution<int> time_offset_dist(-3600, 3600); // ±1 hour jitter
    
    int64_t base_timestamp = 1700000000000;
    size_t point_count = 0;
    
    for (auto _ : state) {
        TimeSeriesStore::DataPoint point;
        // Add random time offset to simulate out-of-order arrival
        point.timestamp_ms = base_timestamp + (point_count * 1000) + (time_offset_dist(rng) * 1000);
        point.value = value_dist(rng);
        
        tsStore()->put(metric, entity, point);
        point_count++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(TimeseriesBenchmarkFixture, OutOfOrderWrites)
    ->Threads(1)
    ->Threads(4)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Downsampling Throughput (TS-6, Run-Plan 17)
// Standalone (no fixture) — creates its own DB per run.
// Measures throughput of bucketed 1-minute window downsampling over 1 hour
// of high-resolution (1 point/second) data.
// Reports: input-points/s, buckets_per_iter, P99 surrogate via counters.
// ============================================================================

// implements bucketed 1-minute aggregate downsampling directly on RocksDB
// (stores points with key="ts:<metric>:<entity>:<ts_ms>", scans per bucket)
static void BM_DownsamplingThroughput(benchmark::State& state) {
    namespace fs = std::filesystem;
    constexpr int64_t kPointsPerHour = 3600;
    constexpr int64_t kBucketMs      = 60 * 1000LL;
    constexpr int64_t kNumBuckets    = 60;

    const std::string db_path =
        std::string("C:\\tmp\\bench_ts_ds_") +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    fs::remove_all(db_path);
    fs::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.memtable_size_mb = 256;
    cfg.block_cache_size_mb = 512;
    cfg.enable_wal = false;
    cfg.disable_wal_for_benchmark = true;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open RocksDB for DownsamplingThroughput");
        return;
    }

    // Store data points as: key="ts:req_rate:ds_server:<ts_ms_str>", value=double as string
    const int64_t base_ts = 1700000000000LL;
    std::mt19937 rng(77);
    std::uniform_real_distribution<double> val_dist(50.0, 500.0);

    for (int64_t i = 0; i < kPointsPerHour; ++i) {
        int64_t ts_ms = base_ts + i * 1000;
        std::string key = "ts:req_rate:ds_server:" + std::to_string(ts_ms);
        // Store value as 8-byte little-endian double
        double v = val_dist(rng);
        std::string val(sizeof(double), '\0');
        std::memcpy(val.data(), &v, sizeof(double));
        db.put(key, val);
    }

    int64_t total_buckets = 0;
    std::vector<int64_t> bucket_latencies_us;
    bucket_latencies_us.reserve(static_cast<size_t>(kNumBuckets * 50));

    for (auto _ : state) {
        double grand_sum = 0.0;
        for (int64_t b = 0; b < kNumBuckets; ++b) {
            int64_t from_ms = base_ts + b * kBucketMs;
            int64_t to_ms   = base_ts + (b + 1) * kBucketMs - 1;

            auto t0 = std::chrono::steady_clock::now();

            // Scan bucket: sequential get() for all 60 points in this minute
            double bucket_sum = 0.0;
            int64_t bucket_count = 0;
            for (int64_t ts_ms = from_ms; ts_ms <= to_ms; ts_ms += 1000) {
                std::string key = "ts:req_rate:ds_server:" + std::to_string(ts_ms);
                std::string val;
                if (db.get(key, val) && val.size() == sizeof(double)) {
                    double v = 0.0;
                    std::memcpy(&v, val.data(), sizeof(double));
                    bucket_sum += v;
                    ++bucket_count;
                }
            }
            double bucket_avg = (bucket_count > 0) ? (bucket_sum / bucket_count) : 0.0;
            benchmark::DoNotOptimize(bucket_avg);
            grand_sum += bucket_avg;

            auto t1 = std::chrono::steady_clock::now();
            bucket_latencies_us.push_back(
                std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
        }
        benchmark::DoNotOptimize(grand_sum);
        total_buckets += kNumBuckets;
    }

    state.SetItemsProcessed(state.iterations() * kPointsPerHour);
    state.counters["buckets_per_iter"] = static_cast<double>(kNumBuckets);
    state.counters["input_pts_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * kPointsPerHour),
        benchmark::Counter::kIsRate);
    state.counters["total_buckets"] = static_cast<double>(total_buckets);

    if (!bucket_latencies_us.empty()) {
        std::sort(bucket_latencies_us.begin(), bucket_latencies_us.end());
        const size_t p99_idx = bucket_latencies_us.size() * 99 / 100;
        state.counters["bucket_agg_P99_us"] =
            static_cast<double>(bucket_latencies_us[p99_idx]);
    }

    db.close();
    fs::remove_all(db_path);
}

BENCHMARK(BM_DownsamplingThroughput)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(50)
    ->UseRealTime();

BENCHMARK_MAIN();
