#include <benchmark/benchmark.h>
#include "llm/gpu_safe_fail.h"
#include "storage/database_connection_manager.h"
#include "storage/disk_space_monitor.h"
#include <memory>
#include <chrono>
#include <thread>

using namespace themis;

// ============================================================================
// GPU Safe-Fail Benchmarks
// ============================================================================

static void BM_GPUSafeFail_HealthCheck(benchmark::State& state) {
    llm::GPUSafeFailManager::Config config;
    llm::GPUSafeFailManager manager(config);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(manager.isHealthy());
    }
}
BENCHMARK(BM_GPUSafeFail_HealthCheck);

static void BM_GPUSafeFail_ShouldAttemptGPU(benchmark::State& state) {
    llm::GPUSafeFailManager::Config config;
    llm::GPUSafeFailManager manager(config);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(manager.shouldAttemptGPU());
    }
}
BENCHMARK(BM_GPUSafeFail_ShouldAttemptGPU);

static void BM_GPUSafeFail_RecordSuccess(benchmark::State& state) {
    llm::GPUSafeFailManager::Config config;
    llm::GPUSafeFailManager manager(config);
    
    for (auto _ : state) {
        manager.recordSuccess();
    }
}
BENCHMARK(BM_GPUSafeFail_RecordSuccess);

static void BM_GPUSafeFail_RecordFailure(benchmark::State& state) {
    llm::GPUSafeFailManager::Config config;
    config.failure_threshold = 1000000;  // High threshold to avoid circuit opening
    llm::GPUSafeFailManager manager(config);
    
    for (auto _ : state) {
        manager.recordFailure(llm::GPUSafeFailManager::FailureType::DEVICE_ERROR, "benchmark_op");
    }
}
BENCHMARK(BM_GPUSafeFail_RecordFailure);

static void BM_GPUSafeFail_ExecuteWithFallback_Success(benchmark::State& state) {
    llm::GPUSafeFailManager::Config config;
    llm::GPUSafeFailManager manager(config);
    
    int64_t operations = 0;
    
    for (auto _ : state) {
        auto result = manager.executeWithFallback(
            []() -> int { return 42; },  // GPU operation (always succeeds)
            []() -> int { return 0; },   // CPU fallback
            "benchmark_op"
        );
        benchmark::DoNotOptimize(result);
        operations++;
    }
    
    state.SetItemsProcessed(operations);
}
BENCHMARK(BM_GPUSafeFail_ExecuteWithFallback_Success);

static void BM_GPUSafeFail_ExecuteWithFallback_Failure(benchmark::State& state) {
    llm::GPUSafeFailManager::Config config;
    config.enable_cpu_fallback = true;
    config.failure_threshold = 1000000;  // High threshold to avoid circuit opening
    llm::GPUSafeFailManager manager(config);
    
    int64_t operations = 0;
    
    for (auto _ : state) {
        auto result = manager.executeWithFallback(
            []() -> int { throw std::runtime_error("GPU failed"); },
            []() -> int { return 42; },  // CPU fallback
            "benchmark_op"
        );
        benchmark::DoNotOptimize(result);
        operations++;
    }
    
    state.SetItemsProcessed(operations);
}
BENCHMARK(BM_GPUSafeFail_ExecuteWithFallback_Failure);

static void BM_GPUSafeFail_GetHealthMetrics(benchmark::State& state) {
    llm::GPUSafeFailManager::Config config;
    llm::GPUSafeFailManager manager(config);
    
    // Record some operations
    for (int i = 0; i < 100; ++i) {
        manager.recordSuccess();
    }
    
    for (auto _ : state) {
        auto status = manager.getHealthStatus();
        benchmark::DoNotOptimize(status.error_rate);
    }
}
BENCHMARK(BM_GPUSafeFail_GetHealthMetrics);

// ============================================================================
// Database Connection Manager Benchmarks
// ============================================================================

// Test connection implementation
class TestConnection : public storage::DatabaseConnectionManager::Connection {
public:
    bool isValid() const override { return true; }
    bool ping() override { return true; }
    std::string getError() const override { return ""; }
    void close() override {}
};

class TestConnectionManager : public storage::DatabaseConnectionManager {
public:
    explicit TestConnectionManager(const storage::DatabaseConnectionManager::ConnectionConfig& config)
        : DatabaseConnectionManager(config) {}
protected:
    std::shared_ptr<Connection> createConnection() override {
        return std::make_shared<TestConnection>();
    }
};

static void BM_ConnectionManager_AcquireRelease(benchmark::State& state) {
    storage::DatabaseConnectionManager::ConnectionConfig config;
    config.min_connections = 2;
    config.max_connections = 10;
    TestConnectionManager manager(config);
    
    int64_t operations = 0;
    
    for (auto _ : state) {
        auto conn = manager.acquireConnection();
        benchmark::DoNotOptimize(conn);
        manager.releaseConnection(conn, false);
        operations++;
    }
    
    state.SetItemsProcessed(operations);
}
BENCHMARK(BM_ConnectionManager_AcquireRelease);

static void BM_ConnectionManager_AcquireRelease_WithError(benchmark::State& state) {
    storage::DatabaseConnectionManager::ConnectionConfig config;
    config.min_connections = 2;
    config.max_connections = 10;
    config.max_retry_attempts = 1;  // Minimize retry overhead
    TestConnectionManager manager(config);
    
    int64_t operations = 0;
    
    for (auto _ : state) {
        auto conn = manager.acquireConnection();
        benchmark::DoNotOptimize(conn);
        manager.releaseConnection(conn, true);  // Report error
        operations++;
    }
    
    state.SetItemsProcessed(operations);
}
BENCHMARK(BM_ConnectionManager_AcquireRelease_WithError);

static void BM_ConnectionManager_ParallelAcquire(benchmark::State& state) {
    storage::DatabaseConnectionManager::ConnectionConfig config;
    config.min_connections = 2;
    config.max_connections = state.range(0);
    TestConnectionManager manager(config);
    
    int64_t operations = 0;
    
    for (auto _ : state) {
        auto conn = manager.acquireConnection();
        benchmark::DoNotOptimize(conn);
        manager.releaseConnection(conn, false);
        operations++;
    }
    
    state.SetItemsProcessed(operations);
}
BENCHMARK(BM_ConnectionManager_ParallelAcquire)
    ->Arg(2)
    ->Arg(5)
    ->Arg(10)
    ->Arg(20)
    ->Arg(50);

static void BM_ConnectionManager_GetPoolStats(benchmark::State& state) {
    storage::DatabaseConnectionManager::ConnectionConfig config;
    TestConnectionManager manager(config);
    
    for (auto _ : state) {
        auto stats = manager.getStats();
        benchmark::DoNotOptimize(stats.active_connections);
    }
}
BENCHMARK(BM_ConnectionManager_GetPoolStats);

// ============================================================================
// Disk Space Monitor Benchmarks
// ============================================================================

static void BM_DiskSpaceMonitor_CanWrite_Small(benchmark::State& state) {
    storage::DiskSpaceMonitor::Config config;
    config.check_interval = std::chrono::hours(1);  // Disable background monitoring
    storage::DiskSpaceMonitor monitor("/tmp", config);
    
    size_t bytes_to_write = 1024;  // 1 KB
    int64_t checks = 0;
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(monitor.canWrite(bytes_to_write));
        checks++;
    }
    
    state.SetItemsProcessed(checks);
}
BENCHMARK(BM_DiskSpaceMonitor_CanWrite_Small);

static void BM_DiskSpaceMonitor_CanWrite_Large(benchmark::State& state) {
    storage::DiskSpaceMonitor::Config config;
    config.check_interval = std::chrono::hours(1);  // Disable background monitoring
    storage::DiskSpaceMonitor monitor("/tmp", config);
    
    size_t bytes_to_write = 1024 * 1024 * 1024;  // 1 GB
    int64_t checks = 0;
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(monitor.canWrite(bytes_to_write));
        checks++;
    }
    
    state.SetItemsProcessed(checks);
}
BENCHMARK(BM_DiskSpaceMonitor_CanWrite_Large);

static void BM_DiskSpaceMonitor_GetSpaceInfo(benchmark::State& state) {
    storage::DiskSpaceMonitor::Config config;
    config.check_interval = std::chrono::hours(1);  // Disable background monitoring
    storage::DiskSpaceMonitor monitor("/tmp", config);
    
    int64_t queries = 0;
    
    for (auto _ : state) {
        auto info = monitor.getSpaceInfo();
        benchmark::DoNotOptimize(info.available_bytes);
        queries++;
    }
    
    state.SetItemsProcessed(queries);
}
BENCHMARK(BM_DiskSpaceMonitor_GetSpaceInfo);

static void BM_DiskSpaceMonitor_GetDiskUsagePercent(benchmark::State& state) {
    storage::DiskSpaceMonitor::Config config;
    config.check_interval = std::chrono::hours(1);  // Disable background monitoring
    storage::DiskSpaceMonitor monitor("/tmp", config);
    
    for (auto _ : state) {
        auto info = monitor.getSpaceInfo();
        benchmark::DoNotOptimize(info.usage_percent);
    }
}
BENCHMARK(BM_DiskSpaceMonitor_GetDiskUsagePercent);

static void BM_DiskSpaceMonitor_RecordWrite(benchmark::State& state) {
    storage::DiskSpaceMonitor::Config config;
    config.check_interval = std::chrono::hours(1);  // Disable background monitoring
    storage::DiskSpaceMonitor monitor("/tmp", config);
    
    size_t bytes = 1024 * 1024;  // 1 MB
    int64_t writes = 0;
    
    for (auto _ : state) {
        // Simulate write by checking space level (recordWrite not available in API)
        benchmark::DoNotOptimize(monitor.getSpaceLevel());
        writes++;
    }
    
    state.SetItemsProcessed(writes);
    state.SetBytesProcessed(writes * bytes);
}
BENCHMARK(BM_DiskSpaceMonitor_RecordWrite);

// ============================================================================
// Combined Benchmarks (Realistic Scenarios)
// ============================================================================

static void BM_Combined_DatabaseOperation(benchmark::State& state) {
    // Setup all safe-fail mechanisms
    llm::GPUSafeFailManager::Config gpu_config;
    llm::GPUSafeFailManager gpu_manager(gpu_config);
    
    storage::DatabaseConnectionManager::ConnectionConfig conn_config;
    TestConnectionManager conn_manager(conn_config);
    
    storage::DiskSpaceMonitor::Config disk_config;
    disk_config.check_interval = std::chrono::hours(1);
    storage::DiskSpaceMonitor disk_monitor("/tmp", disk_config);
    
    size_t data_size = 1024;  // 1 KB
    int64_t operations = 0;
    
    for (auto _ : state) {
        // Check if we can write (using space level instead of canWrite)
        auto space_level = disk_monitor.getSpaceLevel();
        if (space_level == storage::DiskSpaceMonitor::SpaceLevel::EMERGENCY) {
            continue;
        }
        
        // Acquire connection
        auto conn = conn_manager.acquireConnection();
        if (!conn) {
            continue;
        }
        
        // Perform operation (simulated)
        benchmark::DoNotOptimize(conn);
        
        // Release connection
        conn_manager.releaseConnection(conn, false);
        
        operations++;
    }
    
    state.SetItemsProcessed(operations);
}
BENCHMARK(BM_Combined_DatabaseOperation);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
