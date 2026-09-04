// ThemisDB Docker RAID Comprehensive Benchmark Suite
// 
// Purpose: Complete Docker RAID performance testing for multi-container
//          replication, failover, synchronization, and distributed operations
//
// Minimum Runtime: 1+ hour with configurable repetitions
// Test Scenarios: RAID0, RAID1, RAID5, RAID6, RAID10 across Docker containers
//
// Based on:
// - Google Benchmark best practices (github.com/google/benchmark)
// - RocksDB distributed benchmarks (github.com/facebook/rocksdb)
// - ThemisDB Python RAID suite (raid_sharding_test_suite.py)
// - ThemisDB C++ sharding benchmarks (bench_sharding_performance.cpp)
//
// Created: January 3, 2026
// Author: ThemisDB Benchmark Team

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <queue>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

// Prometheus Metrics Export
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>

// Docker SDK (assuming available or simulated)
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

// ============================================================================
// Configuration Constants
// ============================================================================

namespace themis {
namespace benchmarks {
namespace docker_raid {

// RAID Level Definitions
enum class RAIDLevel {
    RAID0 = 0,      // Pure striping, no redundancy, max performance
    RAID1 = 1,      // Mirroring, 2x capacity cost, high read performance
    RAID5 = 5,      // Striping with parity, (n-1)/n capacity
    RAID6 = 6,      // Dual parity, (n-2)/n capacity, higher fault tolerance
    RAID10 = 10     // Striped mirrors, balanced performance and redundancy
};

// Test Data Sizes (scalable for 1+ hour runtime)
constexpr size_t KB = 1024;
constexpr size_t MB = 1024 * KB;
constexpr size_t GB = 1024 * MB;

constexpr size_t SMALL_DOCUMENT_SIZE = 10 * KB;     // 10 KB documents
constexpr size_t MEDIUM_DOCUMENT_SIZE = 100 * KB;   // 100 KB documents
constexpr size_t LARGE_DOCUMENT_SIZE = 1 * MB;      // 1 MB documents
constexpr size_t BLOB_SIZE = 10 * MB;               // 10 MB blobs

// Container Configuration
constexpr int MIN_CONTAINERS = 3;
constexpr int MAX_CONTAINERS = 12;
constexpr int DEFAULT_REPLICAS = 3;

// Network Simulation (latencies in microseconds)
constexpr int LOCAL_NETWORK_LATENCY_US = 100;       // 0.1 ms
constexpr int CROSS_DC_LATENCY_US = 50000;          // 50 ms
constexpr int INTERNET_LATENCY_US = 200000;         // 200 ms

// Benchmark Duration Controls (for 1+ hour total runtime)
constexpr int QUICK_ITERATIONS = 100;               // Fast smoke test
constexpr int STANDARD_ITERATIONS = 1000;           // Standard test
constexpr int LONG_ITERATIONS = 10000;              // Stress test
constexpr int EXTREME_ITERATIONS = 100000;          // 1+ hour scenarios

// ============================================================================
// Docker Container Abstraction
// ============================================================================

struct DockerContainerConfig {
    std::string container_name;
    std::string image;
    int port;
    std::string hostname;
    size_t memory_limit_mb;
    int cpu_limit;
    std::string network_name;
    std::vector<std::string> volumes;
    std::map<std::string, std::string> env_vars;
};

class DockerContainer {
public:
    explicit DockerContainer(const DockerContainerConfig& config)
        : config_(config), running_(false), container_id_("") {}
    
    bool start() {
        // Simulate Docker container startup
        // In production: Use Docker C++ SDK or system() calls
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        running_ = true;
        container_id_ = "container_" + config_.container_name + "_" + 
                       std::to_string(reinterpret_cast<uintptr_t>(this));
        return true;
    }
    
    bool stop() {
        running_ = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return true;
    }
    
    bool restart() {
        return stop() && start();
    }
    
    bool isRunning() const { return running_; }
    
    std::string getId() const { return container_id_; }
    
    const DockerContainerConfig& getConfig() const { return config_; }
    
    // Simulate data write to container
    bool writeData(const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        data_store_.push_back(data);
        bytes_written_ += data.size();
        return true;
    }
    
    // Simulate data read from container
    std::vector<uint8_t> readData(size_t index) const {
        std::lock_guard<std::mutex> lock(data_mutex_);
        if (index < data_store_.size()) {
            bytes_read_ += data_store_[index].size();
            return data_store_[index];
        }
        return {};
    }
    
    size_t getDataCount() const {
        std::lock_guard<std::mutex> lock(data_mutex_);
        return data_store_.size();
    }
    
    size_t getBytesWritten() const { return bytes_written_; }
    size_t getBytesRead() const { return bytes_read_; }
    
private:
    DockerContainerConfig config_;
    bool running_;
    std::string container_id_;
    mutable std::mutex data_mutex_;
    std::vector<std::vector<uint8_t>> data_store_;
    std::atomic<size_t> bytes_written_{0};
    mutable std::atomic<size_t> bytes_read_{0};
};

// ============================================================================
// RAID Controller - Manages Docker Container Array
// ============================================================================

class DockerRAIDController {
public:
    explicit DockerRAIDController(RAIDLevel level, int num_containers)
        : raid_level_(level), num_containers_(num_containers) {
        initializeContainers();
    }
    
    ~DockerRAIDController() {
        shutdown();
    }
    
    bool start() {
        for (auto& container : containers_) {
            if (!container->start()) {
                return false;
            }
        }
        return true;
    }
    
    bool shutdown() {
        for (auto& container : containers_) {
            container->stop();
        }
        return true;
    }
    
    // Write data with RAID-specific logic
    bool writeData(const std::vector<uint8_t>& data, int stripe_id) {
        switch (raid_level_) {
            case RAIDLevel::RAID0:
                return writeRAID0(data, stripe_id);
            case RAIDLevel::RAID1:
                return writeRAID1(data);
            case RAIDLevel::RAID5:
                return writeRAID5(data, stripe_id);
            case RAIDLevel::RAID6:
                return writeRAID6(data, stripe_id);
            case RAIDLevel::RAID10:
                return writeRAID10(data, stripe_id);
            default:
                return false;
        }
    }
    
    // Read data with RAID-specific logic
    std::vector<uint8_t> readData(int stripe_id, size_t offset) {
        switch (raid_level_) {
            case RAIDLevel::RAID0:
                return readRAID0(stripe_id, offset);
            case RAIDLevel::RAID1:
                return readRAID1(offset);
            case RAIDLevel::RAID5:
                return readRAID5(stripe_id, offset);
            case RAIDLevel::RAID6:
                return readRAID6(stripe_id, offset);
            case RAIDLevel::RAID10:
                return readRAID10(stripe_id, offset);
            default:
                return {};
        }
    }
    
    // Simulate container failure
    bool failContainer(int index) {
        if (index >= 0 && index < num_containers_) {
            containers_[index]->stop();
            failed_containers_.insert(index);
            return true;
        }
        return false;
    }
    
    // Recover failed container
    bool recoverContainer(int index) {
        if (failed_containers_.count(index) > 0) {
            containers_[index]->start();
            failed_containers_.erase(index);
            // Simulate rebuild/resync
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return true;
        }
        return false;
    }
    
    // Get aggregate statistics
    size_t getTotalBytesWritten() const {
        size_t total = 0;
        for (const auto& c : containers_) {
            total += c->getBytesWritten();
        }
        return total;
    }
    
    size_t getTotalBytesRead() const {
        size_t total = 0;
        for (const auto& c : containers_) {
            total += c->getBytesRead();
        }
        return total;
    }
    
    int getNumContainers() const { return num_containers_; }
    
    int getNumFailedContainers() const { return failed_containers_.size(); }
    
private:
    void initializeContainers() {
        for (int i = 0; i < num_containers_; ++i) {
            DockerContainerConfig config;
            config.container_name = "themis_raid_node_" + std::to_string(i);
            config.image = "themisdb:latest";
            config.port = 8700 + i;
            config.hostname = "raid-node-" + std::to_string(i);
            config.memory_limit_mb = 2048;
            config.cpu_limit = 2;
            config.network_name = "themis_raid_network";
            config.env_vars["RAID_LEVEL"] = std::to_string(static_cast<int>(raid_level_));
            config.env_vars["NODE_ID"] = std::to_string(i);
            
            containers_.push_back(std::make_unique<DockerContainer>(config));
        }
    }
    
    // RAID0: Stripe data across all containers (no redundancy)
    bool writeRAID0(const std::vector<uint8_t>& data, int stripe_id) {
        int target_container = stripe_id % num_containers_;
        return containers_[target_container]->writeData(data);
    }
    
    std::vector<uint8_t> readRAID0(int stripe_id, size_t offset) {
        int target_container = stripe_id % num_containers_;
        if (failed_containers_.count(target_container) > 0) {
            return {}; // Data lost
        }
        return containers_[target_container]->readData(offset);
    }
    
    // RAID1: Mirror data to all containers (full redundancy)
    bool writeRAID1(const std::vector<uint8_t>& data) {
        bool success = true;
        for (auto& container : containers_) {
            if (failed_containers_.count(&container - &containers_[0]) == 0) {
                success &= container->writeData(data);
            }
        }
        return success;
    }
    
    std::vector<uint8_t> readRAID1(size_t offset) {
        // Read from first available container
        for (size_t i = 0; i < containers_.size(); ++i) {
            if (failed_containers_.count(i) == 0) {
                return containers_[i]->readData(offset);
            }
        }
        return {};
    }
    
    // RAID5: Stripe data with parity
    bool writeRAID5(const std::vector<uint8_t>& data, int stripe_id) {
        // Simplified: Write data to n-1 containers, parity to nth
        int data_containers = num_containers_ - 1;
        int target = stripe_id % data_containers;
        int parity_container = num_containers_ - 1;
        
        containers_[target]->writeData(data);
        containers_[parity_container]->writeData(computeParity(data));
        return true;
    }
    
    std::vector<uint8_t> readRAID5(int stripe_id, size_t offset) {
        int data_containers = num_containers_ - 1;
        int target = stripe_id % data_containers;
        
        if (failed_containers_.count(target) == 0) {
            return containers_[target]->readData(offset);
        }
        // Reconstruct from parity if needed
        return reconstructFromParity(target, offset);
    }
    
    // RAID6: Stripe data with dual parity
    bool writeRAID6(const std::vector<uint8_t>& data, int stripe_id) {
        int data_containers = num_containers_ - 2;
        int target = stripe_id % data_containers;
        int parity1 = num_containers_ - 2;
        int parity2 = num_containers_ - 1;
        
        containers_[target]->writeData(data);
        containers_[parity1]->writeData(computeParity(data));
        containers_[parity2]->writeData(computeDualParity(data));
        return true;
    }
    
    std::vector<uint8_t> readRAID6(int stripe_id, size_t offset) {
        int data_containers = num_containers_ - 2;
        int target = stripe_id % data_containers;
        
        if (failed_containers_.count(target) == 0) {
            return containers_[target]->readData(offset);
        }
        return reconstructFromDualParity(target, offset);
    }
    
    // RAID10: Striped mirrors
    bool writeRAID10(const std::vector<uint8_t>& data, int stripe_id) {
        // Write to pairs of containers (mirrored stripes)
        int mirror_pair = (stripe_id % (num_containers_ / 2)) * 2;
        containers_[mirror_pair]->writeData(data);
        containers_[mirror_pair + 1]->writeData(data);
        return true;
    }
    
    std::vector<uint8_t> readRAID10(int stripe_id, size_t offset) {
        int mirror_pair = (stripe_id % (num_containers_ / 2)) * 2;
        
        if (failed_containers_.count(mirror_pair) == 0) {
            return containers_[mirror_pair]->readData(offset);
        } else if (failed_containers_.count(mirror_pair + 1) == 0) {
            return containers_[mirror_pair + 1]->readData(offset);
        }
        return {};
    }
    
    // Simplified parity computation (XOR simulation)
    std::vector<uint8_t> computeParity(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> parity(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            parity[i] = data[i] ^ 0xFF; // Simplified XOR
        }
        return parity;
    }
    
    std::vector<uint8_t> computeDualParity(const std::vector<uint8_t>& data) {
        auto parity = computeParity(data);
        for (size_t i = 0; i < parity.size(); ++i) {
            parity[i] ^= static_cast<uint8_t>(i % 256); // Add position-based XOR
        }
        return parity;
    }
    
    std::vector<uint8_t> reconstructFromParity(int failed_idx, size_t offset) {
        // Simulate parity reconstruction
        int parity_container = num_containers_ - 1;
        return containers_[parity_container]->readData(offset);
    }
    
    std::vector<uint8_t> reconstructFromDualParity(int failed_idx, size_t offset) {
        int parity2 = num_containers_ - 1;
        return containers_[parity2]->readData(offset);
    }
    
    RAIDLevel raid_level_;
    int num_containers_;
    std::vector<std::unique_ptr<DockerContainer>> containers_;
    std::set<int> failed_containers_;
};

// ============================================================================
// Test Data Generation
// ============================================================================

class TestDataGenerator {
public:
    static std::vector<uint8_t> generateData(size_t size, uint64_t seed = 0) {
        std::mt19937_64 rng(seed);
        std::uniform_int_distribution<int> dist(0, 255);
        
        std::vector<uint8_t> data(size);
        for (auto& byte : data) {
            byte = static_cast<uint8_t>(dist(rng));
        }
        return data;
    }
    
    static std::string generateDocumentJSON(size_t target_size) {
        std::ostringstream oss = {};
        oss << R"({"id": ")" << generateUUID() << R"(", )";
        oss << R"("timestamp": "2026-01-03T10:00:00Z", )";
        oss << R"("data": ")";
        
        // Fill with random data to reach target size
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist('a', 'z');
        
        size_t current_size = oss.str().size();
        size_t remaining = (target_size > current_size) ? (target_size - current_size - 3) : 0;
        
        for (size_t i = 0; i < remaining; ++i) {
            oss << static_cast<char>(dist(rng));
        }
        oss << R"("})";
        
        return oss.str();
    }
    
    static std::string generateUUID() {
        std::mt19937_64 rng(std::random_device{}());
        std::uniform_int_distribution<uint64_t> dist;
        
        uint64_t a = dist(rng);
        uint64_t b = dist(rng);
        
        std::ostringstream oss = {};
        oss << std::hex << std::setfill('0');
        oss << std::setw(8) << (a >> 32);
        oss << "-" << std::setw(4) << ((a >> 16) & 0xFFFF);
        oss << "-4" << std::setw(3) << (a & 0xFFF);
        oss << "-" << std::setw(4) << ((b >> 48) & 0xFFFF);
        oss << "-" << std::setw(12) << (b & 0xFFFFFFFFFFFF);
        
        return oss.str();
    }
};

// ============================================================================
// Benchmark Fixture Base
// ============================================================================

class DockerRAIDBenchmarkBase : public ::benchmark::Fixture {
public:
    void SetUp(::benchmark::State& state) override {
        num_containers_ = state.range(0);
        raid_level_ = static_cast<RAIDLevel>(state.range(1));
        
        controller_ = std::make_unique<DockerRAIDController>(raid_level_, num_containers_);
        
        if (!controller_->start()) {
            state.SkipWithError("Failed to start RAID controller");
            return;
        }
        
        // Warmup
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void TearDown(::benchmark::State& state) override {
        if (controller_) {
            controller_->shutdown();
        }
        controller_.reset();
    }
    
protected:
    int num_containers_;
    RAIDLevel raid_level_;
    std::unique_ptr<DockerRAIDController> controller_;
};

// ============================================================================
// BENCHMARK SUITE 1: Basic RAID Write Performance
// Target: 5-10 minutes
// ============================================================================

BENCHMARK_DEFINE_F(DockerRAIDBenchmarkBase, SmallDocumentWrite)
(::benchmark::State& state) {
    int batch_size = state.range(2);
    
    for (auto _ : state) {
        for (int i = 0; i < batch_size; ++i) {
            auto data = TestDataGenerator::generateData(SMALL_DOCUMENT_SIZE, i);
            controller_->writeData(data, i);
        }
    }
    
    state.SetBytesProcessed(state.iterations() * batch_size * SMALL_DOCUMENT_SIZE);
    state.counters["containers"] = num_containers_;
    state.counters["raid_level"] = static_cast<double>(raid_level_);
    state.counters["total_bytes_written"] = controller_->getTotalBytesWritten();
}

BENCHMARK_REGISTER_F(DockerRAIDBenchmarkBase, SmallDocumentWrite)
    // RAID0: 3-6 containers
    ->Args({3, 0, 100})      // 3 containers, RAID0, 100 docs/batch
    ->Args({3, 0, 1000})     // 3 containers, RAID0, 1000 docs/batch
    ->Args({6, 0, 100})
    ->Args({6, 0, 1000})
    // RAID1: Mirror performance
    ->Args({3, 1, 100})
    ->Args({3, 1, 1000})
    ->Args({6, 1, 100})
    // RAID5: Parity overhead
    ->Args({4, 5, 100})
    ->Args({4, 5, 1000})
    ->Args({6, 5, 100})
    // RAID10: Balanced
    ->Args({4, 10, 100})
    ->Args({6, 10, 1000})
    ->MinTime(10.0)          // At least 10 seconds per configuration
    ->Unit(::benchmark::kMillisecond);

// ============================================================================
// BENCHMARK SUITE 2: Medium Document Write Performance
// Target: 10-15 minutes
// ============================================================================

BENCHMARK_DEFINE_F(DockerRAIDBenchmarkBase, MediumDocumentWrite)
(::benchmark::State& state) {
    int batch_size = state.range(2);
    
    for (auto _ : state) {
        for (int i = 0; i < batch_size; ++i) {
            auto data = TestDataGenerator::generateData(MEDIUM_DOCUMENT_SIZE, i);
            controller_->writeData(data, i);
        }
    }
    
    state.SetBytesProcessed(state.iterations() * batch_size * MEDIUM_DOCUMENT_SIZE);
    state.counters["throughput_mbps"] = 
        (state.iterations() * batch_size * MEDIUM_DOCUMENT_SIZE) / 
        (state.iterations() * state.counters["real_time"].value * 1e6);
}

BENCHMARK_REGISTER_F(DockerRAIDBenchmarkBase, MediumDocumentWrite)
    ->Args({3, 0, 50})
    ->Args({3, 0, 500})
    ->Args({4, 5, 50})
    ->Args({4, 5, 500})
    ->Args({6, 1, 50})
    ->Args({6, 10, 500})
    ->MinTime(15.0)
    ->UseRealTime()
    ->Unit(::benchmark::kMillisecond);

// ============================================================================
// BENCHMARK SUITE 3: Large Blob Write Performance
// Target: 15-20 minutes (heavy I/O)
// ============================================================================

BENCHMARK_DEFINE_F(DockerRAIDBenchmarkBase, LargeBlobWrite)
(::benchmark::State& state) {
    int num_blobs = state.range(2);
    
    for (auto _ : state) {
        for (int i = 0; i < num_blobs; ++i) {
            auto blob = TestDataGenerator::generateData(BLOB_SIZE, i);
            controller_->writeData(blob, i);
        }
    }
    
    state.SetBytesProcessed(state.iterations() * num_blobs * BLOB_SIZE);
    state.counters["blobs_per_sec"] = 
        (state.iterations() * num_blobs) / state.counters["real_time"].value;
}

BENCHMARK_REGISTER_F(DockerRAIDBenchmarkBase, LargeBlobWrite)
    ->Args({3, 0, 10})       // 10 blobs of 10MB each
    ->Args({4, 5, 10})
    ->Args({6, 1, 5})
    ->Args({6, 10, 20})
    ->MinTime(20.0)
    ->UseRealTime()
    ->Unit(::benchmark::kSecond);

// ============================================================================
// BENCHMARK SUITE 4: Read Performance
// Target: 10-15 minutes
// ============================================================================

BENCHMARK_DEFINE_F(DockerRAIDBenchmarkBase, RandomRead)
(::benchmark::State& state) {
    // Pre-populate with data
    int num_documents = 1000;
    for (int i = 0; i < num_documents; ++i) {
        auto data = TestDataGenerator::generateData(MEDIUM_DOCUMENT_SIZE, i);
        controller_->writeData(data, i);
    }
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, num_documents - 1);
    
    int reads_per_iteration = state.range(2);
    
    for (auto _ : state) {
        for (int i = 0; i < reads_per_iteration; ++i) {
            int doc_id = dist(rng);
            auto data = controller_->readData(doc_id, doc_id);
            ::benchmark::DoNotOptimize(data);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * reads_per_iteration);
    state.counters["total_bytes_read"] = controller_->getTotalBytesRead();
}

BENCHMARK_REGISTER_F(DockerRAIDBenchmarkBase, RandomRead)
    ->Args({3, 0, 100})
    ->Args({3, 1, 100})
    ->Args({4, 5, 100})
    ->Args({6, 10, 500})
    ->MinTime(12.0)
    ->Unit(::benchmark::kMillisecond);

// ============================================================================
// BENCHMARK SUITE 5: Failover & Recovery
// Target: 10-15 minutes (includes sleep for recovery)
// ============================================================================

BENCHMARK_DEFINE_F(DockerRAIDBenchmarkBase, ContainerFailover)
(::benchmark::State& state) {
    // Pre-populate
    int num_documents = 500;
    for (int i = 0; i < num_documents; ++i) {
        auto data = TestDataGenerator::generateData(SMALL_DOCUMENT_SIZE, i);
        controller_->writeData(data, i);
    }
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Simulate container failure
        int failed_container = 1; // Fail second container
        controller_->failContainer(failed_container);
        
        state.ResumeTiming();
        
        // Test read performance with failed container
        for (int i = 0; i < 100; ++i) {
            auto data = controller_->readData(i, i);
            ::benchmark::DoNotOptimize(data);
        }
        
        state.PauseTiming();
        
        // Recover container
        auto recovery_start = std::chrono::high_resolution_clock::now();
        controller_->recoverContainer(failed_container);
        auto recovery_end = std::chrono::high_resolution_clock::now();
        
        auto recovery_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            recovery_end - recovery_start).count();
        state.counters["recovery_time_ms"] = recovery_ms;
        
        state.ResumeTiming();
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_REGISTER_F(DockerRAIDBenchmarkBase, ContainerFailover)
    ->Args({3, 1, 0})        // RAID1 - full redundancy
    ->Args({4, 5, 0})        // RAID5 - single parity
    ->Args({6, 6, 0})        // RAID6 - dual parity
    ->Args({6, 10, 0})       // RAID10 - striped mirrors
    ->Iterations(10)         // 10 failover cycles
    ->UseRealTime()
    ->Unit(::benchmark::kMillisecond);

// ============================================================================
// BENCHMARK SUITE 6: Concurrent Operations
// Target: 15-20 minutes (multi-threaded stress)
// ============================================================================

BENCHMARK_DEFINE_F(DockerRAIDBenchmarkBase, ConcurrentWrites)
(::benchmark::State& state) {
    int threads = state.range(2);
    std::atomic<int> stripe_counter{0};
    
    for (auto _ : state) {
        std::vector<std::thread> workers;
        
        for (int t = 0; t < threads; ++t) {
            workers.emplace_back([&, t]() {
                for (int i = 0; i < 50; ++i) {
                    int stripe_id = stripe_counter.fetch_add(1);
                    auto data = TestDataGenerator::generateData(SMALL_DOCUMENT_SIZE, stripe_id);
                    controller_->writeData(data, stripe_id);
                }
            });
        }
        
        for (auto& worker : workers) {
            worker.join();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * threads * 50);
    state.counters["concurrency"] = threads;
}

BENCHMARK_REGISTER_F(DockerRAIDBenchmarkBase, ConcurrentWrites)
    ->Args({3, 0, 4})        // 4 threads
    ->Args({3, 0, 8})
    ->Args({4, 5, 4})
    ->Args({6, 1, 8})
    ->Args({6, 10, 16})
    ->MinTime(15.0)
    ->UseRealTime()
    ->Unit(::benchmark::kMillisecond);

// ============================================================================
// BENCHMARK SUITE 7: Mixed Read/Write Workload
// Target: 15-20 minutes (realistic workload)
// ============================================================================

BENCHMARK_DEFINE_F(DockerRAIDBenchmarkBase, MixedReadWrite)
(::benchmark::State& state) {
    // Pre-populate
    int num_documents = 1000;
    for (int i = 0; i < num_documents; ++i) {
        auto data = TestDataGenerator::generateData(MEDIUM_DOCUMENT_SIZE, i);
        controller_->writeData(data, i);
    }
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> doc_dist(0, num_documents - 1);
    std::uniform_real_distribution<double> op_dist(0.0, 1.0);
    
    double read_ratio = state.range(2) / 100.0; // Percentage as decimal
    int operations_per_iteration = 100;
    
    for (auto _ : state) {
        for (int i = 0; i < operations_per_iteration; ++i) {
            if (op_dist(rng) < read_ratio) {
                // Read operation
                int doc_id = doc_dist(rng);
                auto data = controller_->readData(doc_id, doc_id);
                ::benchmark::DoNotOptimize(data);
            } else {
                // Write operation
                int doc_id = doc_dist(rng);
                auto data = TestDataGenerator::generateData(MEDIUM_DOCUMENT_SIZE, doc_id);
                controller_->writeData(data, doc_id);
            }
        }
    }
    
    state.SetItemsProcessed(state.iterations() * operations_per_iteration);
    state.counters["read_ratio"] = read_ratio;
}

BENCHMARK_REGISTER_F(DockerRAIDBenchmarkBase, MixedReadWrite)
    // Different read/write ratios
    ->Args({3, 0, 90})       // 90% reads, 10% writes
    ->Args({3, 0, 70})       // 70% reads
    ->Args({3, 0, 50})       // 50/50
    ->Args({4, 5, 90})
    ->Args({6, 1, 70})
    ->Args({6, 10, 50})
    ->MinTime(18.0)
    ->Unit(::benchmark::kMillisecond);

// ============================================================================
// BENCHMARK SUITE 8: Synchronization Performance
// Target: 10-15 minutes (measures consistency overhead)
// ============================================================================

BENCHMARK_DEFINE_F(DockerRAIDBenchmarkBase, SynchronizationLatency)
(::benchmark::State& state) {
    int num_sync_operations = state.range(2);
    
    for (auto _ : state) {
        for (int i = 0; i < num_sync_operations; ++i) {
            auto data = TestDataGenerator::generateData(SMALL_DOCUMENT_SIZE, i);
            
            auto sync_start = std::chrono::high_resolution_clock::now();
            
            // Write to primary
            controller_->writeData(data, i);
            
            // Simulate sync verification across replicas
            std::this_thread::sleep_for(std::chrono::microseconds(LOCAL_NETWORK_LATENCY_US));
            
            auto sync_end = std::chrono::high_resolution_clock::now();
            
            auto sync_us = std::chrono::duration_cast<std::chrono::microseconds>(
                sync_end - sync_start).count();
            
            state.counters["avg_sync_latency_us"] += sync_us;
        }
    }
    
    state.counters["avg_sync_latency_us"] /= (state.iterations() * num_sync_operations);
    state.SetItemsProcessed(state.iterations() * num_sync_operations);
}

BENCHMARK_REGISTER_F(DockerRAIDBenchmarkBase, SynchronizationLatency)
    ->Args({3, 1, 100})      // RAID1 - full sync
    ->Args({4, 5, 100})      // RAID5 - parity sync
    ->Args({6, 6, 100})      // RAID6 - dual parity
    ->MinTime(12.0)
    ->Unit(::benchmark::kMicrosecond);

// ============================================================================
// BENCHMARK SUITE 9: Cross-Container Query Performance
// Target: 10-15 minutes
// ============================================================================

BENCHMARK_DEFINE_F(DockerRAIDBenchmarkBase, CrossContainerQuery)
(::benchmark::State& state) {
    // Distribute data across containers
    int docs_per_container = 200;
    for (int container = 0; container < num_containers_; ++container) {
        for (int doc = 0; doc < docs_per_container; ++doc) {
            int global_id = container * docs_per_container + doc;
            auto data = TestDataGenerator::generateData(SMALL_DOCUMENT_SIZE, global_id);
            controller_->writeData(data, global_id);
        }
    }
    
    int queries_per_iteration = state.range(2);
    
    for (auto _ : state) {
        // Simulate queries that span multiple containers
        for (int q = 0; q < queries_per_iteration; ++q) {
            std::vector<std::vector<uint8_t>> results;
            
            // Query each container
            for (int c = 0; c < num_containers_; ++c) {
                int doc_id = c * docs_per_container + (q % docs_per_container);
                auto data = controller_->readData(doc_id, doc_id);
                results.push_back(data);
            }
            
            ::benchmark::DoNotOptimize(results);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * queries_per_iteration * num_containers_);
}

BENCHMARK_REGISTER_F(DockerRAIDBenchmarkBase, CrossContainerQuery)
    ->Args({3, 0, 50})
    ->Args({6, 0, 50})
    ->Args({4, 5, 100})
    ->Args({6, 10, 100})
    ->MinTime(12.0)
    ->Unit(::benchmark::kMillisecond);

// ============================================================================
// BENCHMARK SUITE 10: Rebalancing Performance
// Target: 15-20 minutes (simulates adding/removing containers)
// ============================================================================

BENCHMARK_DEFINE_F(DockerRAIDBenchmarkBase, DynamicRebalancing)
(::benchmark::State& state) {
    // Initial data load
    int initial_documents = 500;
    for (int i = 0; i < initial_documents; ++i) {
        auto data = TestDataGenerator::generateData(MEDIUM_DOCUMENT_SIZE, i);
        controller_->writeData(data, i);
    }
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Simulate adding a new container (in production: scale up)
        // Here we simulate the rebalancing cost
        auto rebalance_start = std::chrono::high_resolution_clock::now();
        
        state.ResumeTiming();
        
        // Redistribute data (simplified simulation)
        for (int i = 0; i < initial_documents / 10; ++i) {
            auto data = controller_->readData(i, i);
            controller_->writeData(data, i + initial_documents);
        }
        
        state.PauseTiming();
        
        auto rebalance_end = std::chrono::high_resolution_clock::now();
        auto rebalance_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            rebalance_end - rebalance_start).count();
        
        state.counters["rebalance_time_ms"] = rebalance_ms;
        
        state.ResumeTiming();
    }
    
    state.SetItemsProcessed(state.iterations() * (initial_documents / 10));
}

BENCHMARK_REGISTER_F(DockerRAIDBenchmarkBase, DynamicRebalancing)
    ->Args({3, 0, 0})
    ->Args({4, 5, 0})
    ->Args({6, 10, 0})
    ->Iterations(5)          // 5 rebalancing cycles
    ->UseRealTime()
    ->Unit(::benchmark::kMillisecond);

} // namespace docker_raid
} // namespace benchmarks
} // namespace themis

// ============================================================================
// Main Function with Extended Configuration
// ============================================================================

int main(int argc, char** argv) {
    // Initialize Prometheus metrics server
    prometheus::Exposer exposer{"0.0.0.0:9091"};
    auto registry = std::make_shared<prometheus::Registry>();
    exposer.RegisterCollectable(registry);
    
    // Build metrics
    auto& throughput_family = prometheus::BuildGauge()
        .Name("themis_raid_throughput_bytes_per_second")
        .Help("RAID write/read throughput in bytes per second")
        .Register(*registry);
    
    auto& latency_family = prometheus::BuildHistogram()
        .Name("themis_raid_operation_latency_seconds")
        .Help("RAID operation latency in seconds")
        .Register(*registry);
    
    auto& operations_family = prometheus::BuildCounter()
        .Name("themis_raid_operations_total")
        .Help("Total RAID operations completed")
        .Register(*registry);
    
    std::cout << "\n";
    std::cout << "Prometheus metrics server started on http://0.0.0.0:9091/metrics\n";
    std::cout << "\n";
    
    // Extended benchmark configuration for 1+ hour runtime
    ::benchmark::Initialize(&argc, argv);
    
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    
    std::cout << "\n";
    std::cout << "========================================================================\n";
    std::cout << "ThemisDB Docker RAID Comprehensive Benchmark Suite\n";
    std::cout << "========================================================================\n";
    std::cout << "Target Runtime: 1+ hour (configurable with --benchmark_min_time)\n";
    std::cout << "Test Scenarios: RAID0, RAID1, RAID5, RAID6, RAID10\n";
    std::cout << "Container Configurations: 3-12 containers\n";
    std::cout << "\n";
    std::cout << "Benchmark Suites:\n";
    std::cout << "  1. Small Document Write    (5-10 min)\n";
    std::cout << "  2. Medium Document Write   (10-15 min)\n";
    std::cout << "  3. Large Blob Write        (15-20 min)\n";
    std::cout << "  4. Random Read             (10-15 min)\n";
    std::cout << "  5. Failover & Recovery     (10-15 min)\n";
    std::cout << "  6. Concurrent Operations   (15-20 min)\n";
    std::cout << "  7. Mixed Read/Write        (15-20 min)\n";
    std::cout << "  8. Synchronization         (10-15 min)\n";
    std::cout << "  9. Cross-Container Query   (10-15 min)\n";
    std::cout << " 10. Dynamic Rebalancing    (15-20 min)\n";
    std::cout << "========================================================================\n";
    std::cout << "\n";
    std::cout << "Command line options:\n";
    std::cout << "  --benchmark_min_time=<seconds>    Min time per benchmark\n";
    std::cout << "  --benchmark_repetitions=<n>       Number of repetitions\n";
    std::cout << "  --benchmark_report_aggregates_only=true  Show only aggregates\n";
    std::cout << "  --benchmark_out=<filename>        Output to JSON/CSV file\n";
    std::cout << "  --benchmark_out_format=<json|csv> Output format\n";
    std::cout << "  --benchmark_filter=<regex>        Filter benchmarks to run\n";
    std::cout << "\n";
    std::cout << "Example for 2-hour run:\n";
    std::cout << "  ./bench_docker_raid_comprehensive --benchmark_min_time=120 \\\n";
    std::cout << "    --benchmark_repetitions=3 \\\n";
    std::cout << "    --benchmark_out=raid_results.json \\\n";
    std::cout << "    --benchmark_out_format=json\n";
    std::cout << "\n";
    
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    
    return 0;
}
