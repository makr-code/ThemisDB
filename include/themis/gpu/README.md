> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: src/gpu/README.md · src/gpu/ROADMAP.md · src/gpu/ARCHITECTURE.md · src/gpu/FUTURE_ENHANCEMENTS.md · src/gpu/SECURITY.md -->

# ThemisDB GPU Module Headers

The GPU module headers define the public interfaces for ThemisDB's GPU subsystem. These headers expose the complete GPU resource lifecycle (device discovery, VRAM allocation, kernel validation, work dispatch, metrics, alerting, and safe-fail) together with higher-level facilities such as query acceleration, tensor buffers, training loop coordination, multi-GPU cluster management, and cross-vendor backends (CUDA, ROCm/HIP, Vulkan) — all without requiring consumers to depend on GPU hardware at compile time.

## Module Purpose

Provides production-safe GPU resource management for ThemisDB: it enforces per-tenant VRAM quotas, validates kernels before launch, implements circuit-breaker safe-fail with automatic GPU→CPU fallback, and exposes Prometheus-compatible metrics and a JSON admin API. Higher layers (LLM inference, vector index, analytics) interact with the GPU exclusively through these interfaces.

## Scope

**In Scope:**
- `GPUModule` — top-level integration façade: policy → circuit-breaker → VRAM alloc → launch → metrics + audit
- `GPUMemoryManager` — edition-aware VRAM allocator with per-tenant quotas
- `GPUMemoryPool` — slab-based pre-allocator with defragmentation
- `DeviceDiscovery` — CUDA/ROCm device enumeration; CPU-fallback sentinel
- `GPUSafeFail` — circuit-breaker safe-fail with GPU→CPU fallback
- `GPUPolicy` — default-deny capability gate for GPU usage
- `GPULauncher` — typed async work-item and batch launcher
- `GPUStreamManager` — named async GPU streams with CPU fallback budget
- `GPUKernelValidator` — FNV-1a checksum kernel whitelist; validate-before-launch
- `GPUMetrics` — Prometheus-compatible counter/gauge registry with thermal/power telemetry
- `GPUAlerts` — threshold-based alert manager with callbacks
- `GPUAuditLog` — ring-buffer structured audit event log
- `GPUAdminAPI` — JSON admin stats, tenant breakdown, dry-run simulation
- `GPUConfig` — startup validation and dry-run simulation
- `GPUFeatureFlags` — per-edition GPU feature gates with runtime overrides
- `GPUQueryAccelerator` — parallel scan/filter/sort/aggregate/join/ANN with CUDA graph capture
- `GPUGraphCache` — LRU CUDA graph capture cache keyed by `QueryShape`
- `GPUTensorBuffer` — typed multi-dimensional tensor with shape/dtype, views, checkpointing
- `GPUTrainingLoop` — batch training coordinator with loss tracking and early stopping
- `ROCmBackend` — ROCm/HIP stream lifecycle, device memory, launcher `BackendFn`
- `VulkanComputeBackend` — cross-vendor Vulkan compute (AMD/Intel/NVIDIA via SPIR-V)
- `GPUClusterTopology` — NVLink/InfiniBand topology detection and routing
- `GPUClusterCoordinator` — multi-node GPU cluster coordination with heartbeat/health
- `ClusterConfig` — multi-node cluster configuration
- `GPUProfiler` / `ScopedGPURange` — NVTX/rocTX profiling markers
- `GPUUnifiedMemoryAllocator` — CPU+GPU shared unified memory (CUDA/HIP managed memory)
- `GPUTimeSliceScheduler` — dynamic per-tenant GPU time-slicing
- `MIGManager` — MIG partition lifecycle for NVIDIA A/H series
- `GPUP2PTransferManager` — peer-to-peer GPU-to-GPU direct transfers (NVLink/PCIe)
- `WASMKernelSandbox` — WASM-based sandbox for untrusted third-party GPU kernels

**Out of Scope:**
- GPU kernel implementations for specific algorithms (in `src/acceleration/`)
- Model training orchestration (in `src/training/`)
- Host-side tensor operations not backed by device memory (in `src/llm/`)

## Key Components

| Header | Class / Type | Description |
|--------|-------------|-------------|
| **[Phase 1: Foundational Error Handling (NEW)]** | | |
| `gpu_error.h` | `GPUErrorHandler`, `GPUErrorClass`, `ErrorRecoveryPolicy` | Unified CUDA/HIP error classification, taxonomy, and recovery policies |
| `gpu_memory.h` | `unique_gpu_ptr<T>`, `shared_gpu_ptr<T>` | RAII GPU memory wrappers with automatic cleanup |
| `gpu_timeout.h` | `KernelSLAGuard` | Kernel SLA enforcement (5-second hard limit) |
| | | |
| `gpu_module.h` | `GPUModule` | Top-level integration façade |
| `memory_manager.h` | `GPUMemoryManager` | Edition-aware VRAM manager, tenant quotas |
| `memory_pool.h` | `GPUMemoryPool` | Slab pre-allocator with defragmentation |
| `device_discovery.h` | `DeviceDiscovery`, `DeviceInfo` | Device enumeration; CPU sentinel |
| `safe_fail.h` | `GPUSafeFail` | Circuit breaker, GPU→CPU fallback |
| `policy.h` | `GPUPolicy` | Default-deny capability gate |
| `launcher.h` | `GPULauncher` | Async work-item / batch launcher |
| `stream_manager.h` | `GPUStreamManager` | Named async streams, CPU budget |
| `kernel_validator.h` | `GPUKernelValidator` | FNV-1a checksum whitelist |
| `metrics.h` | `GPUMetrics` | Prometheus-compatible counters/gauges |
| `alerts.h` | `GPUAlerts` | Threshold alerts with callbacks |
| `audit_log.h` | `GPUAuditLog` | Ring-buffer audit event log |
| `admin_api.h` | `GPUAdminAPI` | JSON admin stats / dry-run simulation |
| `config.h` | `GPUConfig` | Startup validation, dry-run |
| `feature_flags.h` | `GPUFeatureFlags` | Per-edition feature gates |
| `query_accelerator.h` | `GPUQueryAccelerator` | Scan/sort/aggregate/join/ANN |
| `graph_cache.h` | `GPUGraphCache`, `QueryShape` | LRU CUDA graph capture cache |
| `tensor_buffer.h` | `GPUTensorBuffer`, `DType` | Typed tensors, views, checkpointing |
| `training_loop.h` | `GPUTrainingLoop` | Batch training coordinator |
| `rocm_backend.h` | `ROCmBackend` | HIP stream lifecycle + device memory |
| `vulkan_backend.h` | `VulkanComputeBackend` | SPIR-V Vulkan compute backend |
| `cluster_topology.h` | `GPUClusterTopology`, `TopologyLink`, `ClusterNode` | NVLink/IB topology |
| `cluster_coordinator.h` | `GPUClusterCoordinator` | Multi-node cluster coordination |
| `cluster_config.h` | `ClusterConfig` | Cluster node configuration |
| `profiler.h` | `GPUProfiler`, `ScopedGPURange` | NVTX/rocTX profiling markers |
| `unified_memory.h` | `GPUUnifiedMemoryAllocator` | CPU+GPU shared address space |
| `time_slice_scheduler.h` | `GPUTimeSliceScheduler` | Dynamic per-tenant time-slicing |
| `mig_manager.h` | `MIGManager` | MIG partition lifecycle |
| `p2p_transfer.h` | `GPUP2PTransferManager` | P2P GPU-to-GPU direct transfers |
| `wasm_kernel_sandbox.h` | `WASMKernelSandbox` | WASM kernel isolation sandbox |

---

## Phase 1: Foundational Error Handling Infrastructure

**Status**: 🟢 PRODUCTION-READY (Implemented 2026-08-01)  
**Phase**: GPU Phase C Readiness - Foundational  
**Purpose**: Establish unified error handling, RAII memory management, and kernel SLA enforcement as prerequisites for Phase C (Hybrid Retrieval Rollout).

### Components

#### `gpu_error.h` — Error Taxonomy and Handler Interface

Defines unified error classification and recovery policies for CUDA/HIP calls.

**Error Classes** (GPUErrorClass enum):
- `kQuotaExceeded` → VRAM budget denial → CPU fallback
- `kKernelTimeout` → SLA violation (5s hard limit) → CPU fallback + diagnostic
- `kBackendUnavailable` → Device offline / driver error → CPU fallback + mark unavailable
- `kMemoryCommunication` → H2D/D2H transfer failure → Retry once; CPU fallback
- `kNumerical` → Precision loss, NaN detection → Emit warning; continue with caution
- `kUnsupportedOperation` → Kernel not available for config → CPU fallback

**Recovery Policies** (ErrorRecoveryPolicy enum):
- `kFallbackCPU` — Immediately degrade to CPU execution
- `kRetryOnce` — Single retry before CPU fallback
- `kMarkUnavailable` — Mark device unavailable for duration
- `kEmitWarning` — Log warning and continue (non-blocking)

**Interface** (GPUErrorHandler):
- `logError(cuda_err, context)` — Log error; no recovery action
- `handleError(cuda_err, context, policy)` — Classify, log, and apply recovery
- `classifyError(cuda_err)` → GPUErrorClass
- `defaultPolicy(error_class)` → ErrorRecoveryPolicy
- `errorClassName(error_class)`, `cudaErrorName(cuda_err)`, `hipErrorName(hip_err)` — Diagnostics

**Macros**:
- `CHECKED_CUDA(stmt)` — CUDA error checking; auto-logs and applies policy
- `CHECKED_HIP(stmt)` — HIP error checking; auto-logs and applies policy
- `TRY_CUDA(stmt, fallback_action)` — Custom fallback handling

**Usage Example**:
```cpp
// Error handling with CHECKED_CUDA
float* d_data = nullptr;
CHECKED_CUDA(cudaMalloc(&d_data, num_bytes));  // throws on OOM
CHECKED_CUDA(cudaMemcpy(d_data, h_data, num_bytes, cudaMemcpyHostToDevice));

// Custom fallback
TRY_CUDA(cudaMalloc(&ptr, size), {
  if (++retry_count < 3) {
    // retry logic
  } else {
    // fallback to CPU
  }
});
```

#### `gpu_memory.h` — RAII GPU Memory Wrapper

Provides C++ RAII idiom for GPU device memory with automatic cleanup and move semantics.

**Templates**:
- `unique_gpu_ptr<T>` — Unique ownership; move-only (like std::unique_ptr)
- `shared_gpu_ptr<T>` — Reference-counted shared ownership (like std::shared_ptr)

**Factory Functions**:
- `make_unique_gpu<T>(count)` — Allocate count*sizeof(T) bytes on GPU
- `make_shared_gpu<T>(count)` — Allocate with shared ownership

**Invariants**:
- RAII: automatic cleanup on scope exit via CHECKED_CUDA(cudaFree)
- Move semantics: efficient ownership transfer
- No copy: prevents accidental duplication
- Exception-safe: destructor noexcept; handles cleanup on throw

**Usage Example**:
```cpp
// Unique ownership (default)
{
  auto d_data = make_unique_gpu<float>(1000);  // allocates 4KB
  kernel<<<grid, block>>>(d_data.get(), ...);
}  // automatically frees

// Move ownership
auto allocate_buffer(size_t n) {
  return make_unique_gpu<float>(n);  // moves out
}
auto buffer = allocate_buffer(1000);
auto moved = std::move(buffer);  // ownership transfer

// Shared ownership (optional)
auto shared_buf = make_shared_gpu<float>(1000);
auto copy1 = shared_buf;  // +1 refcount
auto copy2 = shared_buf;  // +1 refcount
// All freed when last holder destroyed
```

#### `gpu_timeout.h` — Kernel SLA Enforcement

Enforces maximum kernel execution time to prevent GPU hangs and ensure deterministic latency.

**Class**: `KernelSLAGuard`
- Constructor takes timeout duration (default: 5 seconds)
- `checkTimeoutDeadline()` → true if SLA exceeded
- `getElapsedTime()` → elapsed duration
- `getRemainingTime()` → time until deadline
- Monotonic: once timeout detected, always returns true
- Uses steady_clock (monotonic; unaffected by system clock)

**Default SLA**: 5 seconds (production); tunable for testing.

**Usage Example**:
```cpp
// Production: 5-second SLA
KernelSLAGuard guard;

// Testing: 100ms SLA
KernelSLAGuard guard(std::chrono::milliseconds(100));

// Enforce SLA
myKernel<<<grid, block, 0, stream>>>(args);
if (guard.checkTimeoutDeadline()) {
  // Handle timeout: degrade to CPU
  HANDLE_GPU_TIMEOUT();
}
```

#### `gpu_error.cpp` — Implementation

Implements `GPUErrorHandler` interface:
- Thread-safe error classification and logging
- Error code taxonomy mapping (CUDA/HIP → GPUErrorClass)
- Singleton factory: `GPUErrorHandler::Create()`
- Logging via spdlog (configurable level)
- Exception safety: noexcept where possible

---



### `gpu_module.h`
**Location:** `include/themis/gpu/gpu_module.h`

Top-level entry point that ties together all GPU sub-systems. Callers never need to orchestrate policy, safe-fail, metrics, and audit-log themselves — `GPUModule` does it in a single call.

**Key Interface:**
```cpp
class GPUModule {
public:
    struct InitResult  { bool ok; std::string error; };
    struct SubmitResult { bool submitted; bool via_gpu; std::string error; };

    static GPUModule& GetInstance();

    InitResult   initialize(const GPUConfig& config,
                            GPULauncher::BackendFn backend = nullptr);
    bool         isInitialized() const noexcept;

    SubmitResult submitWork(const std::string& caller_id,
                            const std::string& tenant_id,
                            const GPULauncher::WorkItem& item);

    bool allocate(const std::string& caller_id, const std::string& tenant_id,
                  uint64_t bytes, const std::string& tag = "gpu_module");
    void deallocate(const std::string& tenant_id, uint64_t bytes);

    void grantCaller(const std::string& caller_id,
                     GPUPolicy::Capability cap = GPUPolicy::Capability::GPU_ANY);
    void revokeCaller(const std::string& caller_id);

    MIGManager& mig() noexcept;
};
```

**Typical Usage:**
```cpp
#include "themis/gpu/gpu_module.h"
using namespace themis::gpu;

GPUConfig cfg;
cfg.max_vram_bytes      = 8ULL << 30;   // 8 GB
cfg.enable_cpu_fallback = true;

auto& m = GPUModule::GetInstance();
auto init = m.initialize(cfg);
if (!init.ok) { /* handle */ }

m.grantCaller("analytics-engine");
auto res = m.submitWork("analytics-engine", "tenant-acme", {
    "sort_kernel", {}, 500 /* timeout_ms */
});
// res.via_gpu == false → CPU fallback was used (no GPU hardware present)
```

---

## VRAM Management

### `memory_manager.h`
**Location:** `include/themis/gpu/memory_manager.h`

Edition-aware VRAM allocator with per-tenant quotas and pre-allocation hints.

**Key Types:**
- `GPUMemoryManager::AllocationRecord` — per-allocation record (size, tag, tenant_id)
- `GPUMemoryManager::Stats` — global stats (allocated_bytes, peak_bytes, alloc/dealloc counts)
- `GPUMemoryManager::TenantStats` — per-tenant quota, current usage, peak

**Key Interface:**
```cpp
class GPUMemoryManager {
public:
    static GPUMemoryManager& GetInstance();

    static constexpr int      GetMaxGPUVRAMGB()    noexcept;
    static constexpr uint64_t GetMaxGPUVRAMBytes()  noexcept;

    bool     TryAllocateGPU(uint64_t bytes, const std::string& tag,
                             const std::string& tenant_id = "");
    bool     DeallocateGPU(uint64_t bytes, const std::string& tenant_id = "");

    bool     SetTenantQuota(const std::string& tenant_id, uint64_t quota_bytes);
    bool     RemoveTenant(const std::string& tenant_id);

    Stats        GetStats()        const;
    TenantStats  GetTenantStats(const std::string& tenant_id) const;
    std::vector<TenantStats> GetAllTenantStats() const;
    std::vector<AllocationRecord> GetAllocations() const;
    void         Reset();
};
```

**Usage:**
```cpp
#include "themis/gpu/memory_manager.h"
using namespace themis::gpu;

auto& mgr = GPUMemoryManager::GetInstance();
mgr.SetTenantQuota("tenant-a", 4ULL << 30); // 4 GB cap

if (mgr.TryAllocateGPU(1ULL << 30, "vector-index", "tenant-a")) {
    // use 1 GB VRAM
    mgr.DeallocateGPU(1ULL << 30, "tenant-a");
}

auto stats = mgr.GetStats();
// stats.allocated_bytes, peak_bytes, allocation_count, deallocation_count
```

---

### `memory_pool.h`
**Location:** `include/themis/gpu/memory_pool.h`

Slab-based VRAM pre-allocator with fragmentation tracking and defragmentation.

**Key Types:**
- `GPUMemoryPool::Slab` — one allocation unit (offset, size, in_use flag)
- `GPUMemoryPool::DefragResult` — bytes moved, slabs compacted, new fragmentation ratio
- `GPUMemoryPool::Stats` — total/used/free bytes, fragmentation ratio, slab count

**Key Interface:**
```cpp
class GPUMemoryPool {
public:
    explicit GPUMemoryPool(uint64_t pool_size_bytes,
                           uint64_t slab_size_bytes  = 64 * 1024,
                           bool     zero_on_release   = true);

    std::optional<uint64_t> allocate(uint64_t size_bytes);
    bool                    release(uint64_t offset, uint64_t size_bytes);

    float        fragmentationRatio() const;
    Stats        stats() const;
    DefragResult defragment();
    void         bindDeviceBase(void* device_ptr);
};
```

---

### `config.h`
**Location:** `include/themis/gpu/config.h`

GPU module startup configuration with dry-run simulation.

**Key Type:**
```cpp
struct GPUConfig {
    uint64_t    max_vram_bytes           = 0;         // 0 = use edition default
    int         max_tenants              = 16;
    bool        enable_cpu_fallback      = true;
    uint64_t    cpu_fallback_max_ms      = 5000;
    uint32_t    circuit_breaker_threshold = 3;
    uint32_t    circuit_breaker_reset_s  = 60;
};
```

---

## Device Management

### `device_discovery.h`
**Location:** `include/themis/gpu/device_discovery.h`

Enumerates physical GPU devices. Returns a CPU-fallback sentinel when no GPU runtime is present so callers never need to null-check.

**Key Types:**
```cpp
struct DeviceInfo {
    int         index;             // driver-assigned device index
    std::string name;
    std::string backend;           // "CUDA" | "ROCm" | "Vulkan" | "CPU_FALLBACK"
    uint64_t    total_vram_bytes;
    uint64_t    free_vram_bytes;
    int         compute_major;
    int         compute_minor;
    bool        is_healthy;
    std::string error_message;
    bool        mig_enabled;
    int         mig_max_instances;
};

class DeviceDiscovery {
public:
    static std::vector<DeviceInfo> Enumerate();
    static DeviceInfo              GetCPUFallbackDevice();
    static bool                    IsGPUAvailable();
};
```

---

## Safe-Fail & Policy

### `safe_fail.h`
**Location:** `include/themis/gpu/safe_fail.h`

Circuit-breaker implementing `CLOSED → OPEN → HALF_OPEN` transitions with automatic GPU→CPU fallback.

**Key Types:**
```cpp
class GPUSafeFail {
public:
    enum class State         { CLOSED, OPEN, HALF_OPEN };
    enum class FailureType   { EXECUTION_ERROR, TIMEOUT, OOM, DEVICE_LOST };
    struct Config {
        uint32_t failure_threshold    = 3;
        uint32_t reset_timeout_s      = 60;
        bool     enable_cpu_fallback  = true;
        uint64_t cpu_budget_ms        = 5000;
    };
    struct HealthStatus {
        State       state;
        uint32_t    failure_count;
        bool        can_use_gpu;
        std::string last_error;
    };

    bool executeWithFallback(
        const std::string& op_name,
        std::function<bool()> gpu_op,
        std::function<bool()> cpu_op = nullptr);

    void         recordFailure(FailureType type, const std::string& msg = "");
    void         recordSuccess();
    bool         canUseGPU()     const;
    HealthStatus getStatus()     const;
    void         forceFailed(const std::string& reason = "");
};
```

**Usage:**
```cpp
#include "themis/gpu/safe_fail.h"
using namespace themis::gpu;

GPUSafeFail sf;
bool ok = sf.executeWithFallback(
    "sort-kernel",
    []{ /* GPU sort */ return true; },
    []{ /* CPU sort */ std::sort(...); return true; }
);
// Automatically falls back to CPU when GPU circuit opens.
```

### `policy.h`
**Location:** `include/themis/gpu/policy.h`

Default-deny capability gate. All GPU work must be explicitly granted before dispatch.

```cpp
class GPUPolicy {
public:
    enum class Capability { GPU_ANY, GPU_ALLOCATE, GPU_LAUNCH, GPU_ADMIN };

    void grant(const std::string& caller_id, Capability cap = Capability::GPU_ANY);
    void revoke(const std::string& caller_id, Capability cap = Capability::GPU_ANY);
    void revokeAll(const std::string& caller_id);

    PolicyDecision check(const std::string& caller_id,
                         Capability cap = Capability::GPU_ALLOCATE) const;
    bool           isAllowed(const std::string& caller_id,
                              Capability cap = Capability::GPU_ALLOCATE) const;
};
```

---

## Kernel Validation

### `kernel_validator.h`
**Location:** `include/themis/gpu/kernel_validator.h`

FNV-1a 64-bit checksum whitelist. Every kernel must be registered and validated before launch; unknown or tampered kernels are rejected.

```cpp
class GPUKernelValidator {
public:
    enum class Status { OK, UNKNOWN_KERNEL, CHECKSUM_MISMATCH, EMPTY_BLOB };

    void   registerKernel(const std::string& kernel_id, uint64_t expected_checksum);
    void   registerKernelBlob(const std::string& kernel_id,
                              const std::vector<uint8_t>& blob);
    void   unregisterKernel(const std::string& kernel_id);
    bool   isRegistered(const std::string& kernel_id) const;

    ValidationResult validate(const std::string& kernel_id,
                               const std::vector<uint8_t>& blob) const;
    bool             isValid(const std::string& kernel_id,
                              const std::vector<uint8_t>& blob) const;

    static uint64_t computeChecksum(const std::vector<uint8_t>& data);
    static uint64_t computeChecksum(const uint8_t* data, size_t len);
};
```

---

## Async Launch

### `launcher.h`
**Location:** `include/themis/gpu/launcher.h`

Typed async work-item and batched launcher. Supports pluggable `BackendFn` for CUDA/HIP/CPU.

```cpp
class GPULauncher {
public:
    using BackendFn = std::function<bool(const WorkItem&)>;

    struct WorkItem {
        std::string kernel_id;
        std::vector<uint8_t> args;
        uint32_t    timeout_ms = 1000;
    };
    struct WorkResult {
        bool        success   = false;
        bool        timed_out = false;
        std::string error;
    };

    explicit GPULauncher(BackendFn backend = nullptr);
    WorkResult            submit(const WorkItem& item);
    std::vector<WorkResult> submitBatch(const std::vector<WorkItem>& batch);
    Stats stats() const;
};
```

### `stream_manager.h`
**Location:** `include/themis/gpu/stream_manager.h`

Named async GPU streams with per-stream CPU fallback budgets. Supports first-class CUDA stream creation via `createCudaStream()`.

```cpp
class GPUStreamManager {
public:
    struct StreamConfig {
        uint64_t cpu_budget_ms   = 5000;
        bool     auto_cpu_retry  = true;
    };

    static GPUStreamManager& GetInstance();

    bool createStream(const std::string& name,
                      int device_index = 0,
                      StreamConfig cfg  = {});
    // Creates a named CUDA stream on the given device (THEMIS_ENABLE_CUDA path).
    bool createCudaStream(const std::string& name, int device_index = 0);
    bool destroyStream(const std::string& name);

    bool submit(const std::string& stream_name,
                GPULauncher::WorkItem item);

    StreamStats                         getStreamStats(const std::string& name) const;
    std::vector<std::pair<std::string, StreamStats>> getAllStreamStats() const;
};
```

---

## Observability

### `metrics.h`
**Location:** `include/themis/gpu/metrics.h`

Prometheus-compatible counter/gauge registry with per-device thermal and power telemetry.

```cpp
class GPUMetrics {
public:
    static GPUMetrics& GetInstance();

    void incrementCounter(const std::string& name, double delta = 1.0);
    void setGauge(const std::string& name, double value);
    void updateVRAMGauge(uint64_t allocated_bytes);
    void updateTemperatureGauge(int device_index, double celsius);
    void updatePowerDrawGauge(int device_index, double watts);
    void updatePowerLimitGauge(int device_index, double watts);
    void recordKernelExecution(const KernelRecord& rec);

    std::vector<Sample>       getAllMetrics()  const;
    std::string               exportNsightJson()    const; // CUDA Nsight Compute format
    std::string               exportRocmTraceJson() const; // Chrome trace format
    void                      reset();
};
```

### `alerts.h`
**Location:** `include/themis/gpu/alerts.h`

Threshold-based alert manager. Fires a callback when a metric crosses its threshold.

```cpp
class GPUAlerts {
public:
    using AlertCallback = std::function<void(const std::string& rule, double value)>;
    enum class AlertState { INACTIVE, FIRING };

    void addRule(const std::string& metric_name, double threshold,
                 AlertCallback callback = nullptr);
    void removeRule(const std::string& metric_name);
    void evaluate(const std::vector<GPUMetrics::Sample>& samples);
    std::vector<AlertStatus> getAlertStatuses() const;
    void reset();
};
```

### `audit_log.h`
**Location:** `include/themis/gpu/audit_log.h`

Ring-buffer structured audit log. Records every GPU allocation, deallocation, fallback, and admin event.

```cpp
class GPUAuditLog {
public:
    enum class EventType {
        ALLOCATION, DEALLOCATION, LAUNCH, FALLBACK_TO_CPU,
        CIRCUIT_OPEN, CIRCUIT_CLOSE, ADMIN, ERROR
    };
    struct Event {
        EventType   type;
        std::string tenant_id;
        std::string detail;
        uint64_t    timestamp_ms;
    };

    explicit GPUAuditLog(size_t capacity = 1024);

    void                 record(EventType type, const std::string& tenant_id,
                                const std::string& detail);
    std::vector<Event>   getEvents(size_t last_n = 0) const;
    size_t               size() const;
    void                 clear();
};
```

### `admin_api.h`
**Location:** `include/themis/gpu/admin_api.h`

JSON-serialising admin/ops API. Suitable for mounting on any HTTP endpoint or CLI tool.

```cpp
class GPUAdminAPI {
public:
    explicit GPUAdminAPI(
        GPUMemoryManager* mem_mgr   = nullptr,
        GPULoadBalancer*  balancer  = nullptr,
        GPUSafeFail*      safe_fail = nullptr);

    std::string statsJson()           const; // global GPU stats
    std::string tenantBreakdownJson() const; // per-tenant VRAM
    std::string deviceLoadJson()      const; // per-device load

    struct SimulationResult { bool would_succeed; std::string reason; };
    SimulationResult simulate(uint64_t bytes, const std::string& tenant_id) const;

    std::string geoBackendStatsJson() const; // GPU geo backend stats
    std::string migInstancesJson()    const; // active MIG partitions
};
```

---

## Edition Feature Gates

### `feature_flags.h`
**Location:** `include/themis/gpu/feature_flags.h`

Per-edition feature gate registry. Enables/disables GPU capabilities by deployment edition with per-feature runtime overrides.

```cpp
class GPUFeatureFlags {
public:
    enum class Feature {
        MULTI_GPU,          // ENTERPRISE+
        CLUSTER_COORD,      // ENTERPRISE+
        MIG_MANAGER,        // ENTERPRISE+
        VULKAN_BACKEND,     // all GPU editions
        P2P_TRANSFERS,      // ENTERPRISE+
        PEER_TO_PEER,       // ENTERPRISE+
        NVLINK_SCHEDULING,  // HYPERSCALER
        UNIFIED_MEMORY,     // ENTERPRISE+
        TIME_SLICING,       // ENTERPRISE+
        WASM_SANDBOX,       // ENTERPRISE+
        // ... additional features
    };

    static GPUFeatureFlags& GetInstance();

    bool         isEnabled(Feature feature)   const;
    std::string  featureName(Feature feature) const;
    void         enable(Feature feature);
    void         disable(Feature feature);
    void         resetToDefaults();
    std::vector<FeatureStatus> listAll() const;
    std::string  editionName()            const;
};
```

**Edition Defaults:**

| Feature | Community | Professional | Enterprise | Hyperscaler |
|---------|-----------|--------------|------------|-------------|
| GPU (any) | ❌ | ✅ | ✅ | ✅ |
| Multi-GPU | ❌ | ❌ | ✅ | ✅ |
| MIG Manager | ❌ | ❌ | ✅ | ✅ |
| P2P Transfers | ❌ | ❌ | ✅ | ✅ |
| Unified Memory | ❌ | ❌ | ✅ | ✅ |
| NVLink Scheduling | ❌ | ❌ | ❌ | ✅ |
| Vulkan Backend | ❌ | ✅ | ✅ | ✅ |
| WASM Sandbox | ❌ | ❌ | ✅ | ✅ |

---

## Query Acceleration

### `query_accelerator.h`
**Location:** `include/themis/gpu/query_accelerator.h`

Parallel scan, filter, sort, aggregate, hash join, and ANN search operations with GPU threshold dispatch and optional CUDA graph capture.

**Key Types:**
```cpp
class GPUQueryAccelerator {
public:
    struct Row { std::vector<double> values; };
    enum class PrecisionMode { FP32, FP16, BF16 };
    enum class AggFunc       { SUM, COUNT, MIN, MAX, AVG };
    enum class SortOrder     { ASC, DESC };

    struct ScanResult  { std::vector<Row> rows; bool used_gpu; };
    struct SortResult  { std::vector<Row> rows; bool used_gpu; };
    struct AggResult   { double value; bool used_gpu; };
    struct JoinResult  { std::vector<Row> rows; bool used_gpu; };
    struct AnnResult   { std::vector<AnnNeighbor> neighbors; bool used_gpu; };

    struct Config {
        size_t        gpu_row_threshold = 10000; // rows below → CPU path
        PrecisionMode precision         = PrecisionMode::FP32;
    };

    ScanResult scan(const std::vector<Row>& rows,
                    std::function<bool(const Row&)> predicate = nullptr);
    SortResult sort(const std::vector<Row>& rows,
                    std::function<double(const Row&)> key_fn,
                    SortOrder order = SortOrder::ASC);
    AggResult  aggregate(const std::vector<Row>& rows, AggFunc func,
                         std::function<double(const Row&)> value_fn);
    JoinResult hashJoin(const std::vector<Row>& left,
                        const std::vector<Row>& right,
                        std::function<std::string(const Row&)> key_fn);
    AnnResult  annSearch(const std::vector<float>& query,
                         const std::vector<std::vector<float>>& index,
                         size_t k, float ef_search = 200.0f);

    void  enableGraphCapture();
    void  disableGraphCapture();
    Stats getStats() const;
};
```

**Usage:**
```cpp
#include "themis/gpu/query_accelerator.h"
using namespace themis::gpu;

GPUQueryAccelerator::Config cfg;
cfg.gpu_row_threshold = 50000;
GPUQueryAccelerator acc(cfg);

auto result = acc.scan(rows, [](const GPUQueryAccelerator::Row& r){
    return r.values[0] > 100.0;
});
// result.used_gpu indicates whether GPU dispatch was triggered
```

### `graph_cache.h`
**Location:** `include/themis/gpu/graph_cache.h`

LRU cache for CUDA graph capture. Recurring query patterns matching the same `QueryShape` (OpType × row_count × param_hash) reuse a captured graph, eliminating kernel launch overhead.

```cpp
struct QueryShape {
    enum class OpType : uint8_t { SCAN, FILTER, SORT, AGGREGATE, HASH_JOIN, ANN };
    OpType   op_type;
    uint32_t row_count;
    uint64_t param_hash;  // hash of predicate/key/aggregate configuration
};

class GPUGraphCache {
public:
    explicit GPUGraphCache(size_t max_entries = 32);

    std::optional<GraphEntry> lookup(const QueryShape& shape) const;
    void                      capture(const QueryShape& shape, GraphEntry entry);
    void                      evict(const QueryShape& shape);
    void                      clear();
    Stats                     stats() const;
};
```

---

## Tensor Containers

### `tensor_buffer.h`
**Location:** `include/themis/gpu/tensor_buffer.h`

Multi-dimensional typed tensor backed by host-side storage, with named views and binary checkpointing.

```cpp
enum class DType { FLOAT32, FLOAT16, BFLOAT16, INT32, INT64, UINT8 };

class GPUTensorBuffer {
public:
    struct Shape    { std::vector<int64_t> dims; };
    struct View     { std::string name; uint64_t offset_bytes; uint64_t size_bytes; Shape shape; };

    GPUTensorBuffer(DType dtype, Shape shape);

    void fill(double scalar);
    void copyFrom(const void* src, uint64_t bytes);
    void copyTo(void* dst, uint64_t bytes) const;

    View        createView(const std::string& name, uint64_t offset_bytes,
                           uint64_t size_bytes, Shape shape);
    std::optional<View>  getView(const std::string& name) const;

    std::vector<uint8_t> serialize() const;
    static GPUTensorBuffer deserialize(const std::vector<uint8_t>& bytes);

    DType    dtype()  const;
    Shape    shape()  const;
    uint64_t bytes()  const;
    Stats    stats()  const;
};
```

---

## Training

### `training_loop.h`
**Location:** `include/themis/gpu/training_loop.h`

Batch training loop coordinator: orchestrates epoch iteration, records per-step loss, and implements early stopping.

```cpp
class GPUTrainingLoop {
public:
    struct Config {
        int    max_epochs         = 100;
        float  early_stop_delta   = 1e-4f;
        int    early_stop_patience = 5;
    };
    struct StepRecord  { int epoch; int step; float loss; };
    struct EpochStats  { int epoch; float mean_loss; float min_loss; float max_loss; };

    using BatchFn   = std::function<float(int batch_idx)>;  // returns loss
    using EpochFn   = std::function<void(const EpochStats&)>;

    explicit GPUTrainingLoop(const Config& cfg = {});

    void run(int num_batches, BatchFn batch_fn, EpochFn on_epoch = nullptr);
    bool wasStopped() const;

    std::vector<StepRecord>  stepHistory()  const;
    std::vector<EpochStats>  epochHistory() const;
};
```

---

## Hardware Backends

### `rocm_backend.h`
**Location:** `include/themis/gpu/rocm_backend.h`

ROCm/HIP backend with full feature parity to CUDA. Provides HIP stream lifecycle, device memory management, and a launcher `BackendFn`.

```cpp
class ROCmBackend {
public:
    static int  deviceCount();
    static bool isAvailable();

    GPULauncher::BackendFn createBackendFn(int device_index = 0);

    bool createStream(const std::string& name, int device_index = 0);
    bool destroyStream(const std::string& name);
    bool synchronizeStream(const std::string& name);

    AllocationRecord allocate(size_t size_bytes);
    bool             free(const AllocationRecord& rec);
    bool             memset(const AllocationRecord& rec, int value, size_t bytes);

    Stats stats() const;
    void  resetStats();
};
```

### `vulkan_backend.h`
**Location:** `include/themis/gpu/vulkan_backend.h`

Cross-vendor Vulkan compute backend (AMD RDNA, Intel Arc, NVIDIA via SPIR-V). CPU simulation path always active; tests pass without Vulkan hardware.

```cpp
class VulkanComputeBackend {
public:
    static int  deviceCount();
    static bool isAvailable();
    static std::string vendorName(int device_index = 0);

    GPULauncher::BackendFn createBackendFn(int device_index = 0);

    bool createStream(const std::string& name, int device_index = 0);
    bool destroyStream(const std::string& name);
    bool synchronizeStream(const std::string& name);
    StreamHandle getStream(const std::string& name) const;
    bool         hasStream(const std::string& name) const;
    std::vector<std::string> streamNames() const;

    Stats getStats()  const;
    void  resetStats();
};
```

---

## Multi-GPU Load Balancing

### `load_balancer.h`
**Location:** `include/themis/gpu/load_balancer.h`

Distributes work across multiple GPUs using pluggable scheduling strategies.

```cpp
class GPULoadBalancer {
public:
    enum class Strategy { ROUND_ROBIN, LEAST_LOADED, FIRST_HEALTHY, TOPOLOGY_AWARE };

    explicit GPULoadBalancer(Strategy strategy = Strategy::LEAST_LOADED,
                             const std::vector<DeviceInfo>& devices = {});

    void setDevices(const std::vector<DeviceInfo>& devices);
    void markFailed(int device_index);
    void markHealthy(int device_index);
    void setTopology(const GPUClusterTopology& topology);

    std::optional<int> selectDevice() const;

    void notifyAllocated(int device_index, uint64_t bytes);
    void notifyFreed(int device_index, uint64_t bytes);

    std::vector<DeviceLoad> getDeviceLoads() const;
};
```

---

## Multi-Node Cluster

### `cluster_config.h`
**Location:** `include/themis/gpu/cluster_config.h`

Configuration for the multi-node GPU cluster coordinator.

```cpp
struct ClusterConfig {
    bool        enabled            = false;
    enum class Mode { STANDALONE, COORDINATOR, WORKER } mode = Mode::STANDALONE;
    std::string node_id;
    int         rank               = 0;
    int         world_size         = 1;
    std::string coordinator_addr;
    uint32_t    heartbeat_interval_ms  = 5000;
    uint32_t    node_timeout_ms        = 15000;
    std::vector<NodeEntry> nodes;
    std::vector<std::string> peers;
    std::string ib_device;
    std::string ib_port;
    bool        nvlink_scheduling  = false;
    bool        ib_transfers       = false;
};
```

### `cluster_topology.h`
**Location:** `include/themis/gpu/cluster_topology.h`

NVLink/InfiniBand topology detection. Builds a bandwidth matrix across local GPUs and cluster nodes for topology-aware scheduling.

```cpp
enum class InterconnectType { NVLINK, PCIE, INFINIBAND, ETHERNET, UNKNOWN };

struct TopologyLink {
    int              src_device;
    int              dst_device;
    InterconnectType type;
    double           bandwidth_gb_s;
};

class GPUClusterTopology {
public:
    static GPUClusterTopology Detect(const std::vector<DeviceInfo>& devices);

    void         addNode(const ClusterNode& node);
    void         removeNode(const std::string& node_id);
    void         addIBLink(const std::string& src_node,
                           const std::string& dst_node,
                           double bandwidth_gb_s);

    bool   hasNVLink()       const;
    bool   hasPCIePeer()     const;
    bool   hasInfiniBand()   const;
    size_t deviceCount()     const;

    std::pair<int,int>         highestBandwidthNVLinkPair()  const;
    std::pair<std::string,std::string> highestBandwidthIBPair() const;

    double          estimateBandwidth(int src, int dst) const;
    InterconnectType bestInterconnect(int src, int dst) const;
};
```

### `cluster_coordinator.h`
**Location:** `include/themis/gpu/cluster_coordinator.h`

Multi-node cluster coordinator: heartbeat tracking, node health management, and least-loaded work placement.

```cpp
class GPUClusterCoordinator {
public:
    enum class NodeRole   { COORDINATOR, WORKER };
    enum class NodeStatus { ONLINE, OFFLINE, DEGRADED };

    struct NodeInfo {
        std::string  node_id;
        NodeRole     role;
        NodeStatus   status;
        uint64_t     free_vram_bytes;
        uint64_t     last_heartbeat_ms;
    };
    struct Placement    { std::string node_id; int device_index; };
    struct ClusterHealth { size_t online_count; size_t offline_count; bool coordinator_online; };

    explicit GPUClusterCoordinator(const ClusterConfig& cfg = {},
                                   const std::vector<DeviceInfo>& local = {});

    void registerPeer(const NodeInfo& node);
    void removePeer(const std::string& node_id);
    void rebuildTopology();

    void     recordHeartbeat(const std::string& node_id, uint64_t free_vram_bytes);
    void     markOffline(const std::string& node_id);
    void     evictStaleNodes(uint32_t timeout_ms = 15000);

    Placement      selectBestLocalDevice(uint64_t required_bytes = 0) const;
    Placement      selectBestTransferTarget(const std::string& src_node) const;
    Placement      scheduleWork(uint64_t required_bytes = 0) const;

    const GPUClusterTopology& topology() const;
    const ClusterConfig&      config()   const;
    ClusterHealth             health()   const;
};
```

---

## Profiling

### `profiler.h`
**Location:** `include/themis/gpu/profiler.h`

NVIDIA Nsight (NVTX) and AMD ROCm Profiler (rocTX) marker integration. Falls back to wall-clock timing when no profiling runtime is present.

```cpp
class GPUProfiler {
public:
    struct Range {
        std::string name;
        uint64_t    start_ns;
        uint64_t    end_ns;
        uint32_t    color;  // ARGB
    };

    static GPUProfiler& GetInstance();

    void push(const std::string& name, uint32_t color = 0xFF00FF00);
    void pop();
    void mark(const std::string& name, uint32_t color = 0xFFFFFF00);

    std::string            exportChromeTrace()  const;
    std::vector<Range>     getRanges()          const;
    void                   reset();
};

// RAII guard — push on enter, pop on exit
class ScopedGPURange {
public:
    explicit ScopedGPURange(const std::string& name, uint32_t color = 0xFF00FF00);
    ~ScopedGPURange();
};
```

**Usage:**
```cpp
#include "themis/gpu/profiler.h"
using namespace themis::gpu;

{
    ScopedGPURange r("vector-index-build");
    // ... index build work
}
// Range automatically closed on scope exit.
// Trace exportable as Chrome JSON for Nsight / ROCm Profiler.
```

---

## Advanced Facilities

### `unified_memory.h`
**Location:** `include/themis/gpu/unified_memory.h`

CPU+GPU shared address space (CUDA/HIP managed memory). Provides prefetching, memory advice, and optional hardware unified memory detection.

```cpp
class GPUUnifiedMemoryAllocator {
public:
    enum class MemAdvice { READ_MOSTLY, PREFERRED_GPU, PREFERRED_CPU, ACCESSED_BY_GPU };

    void*  allocate(size_t bytes, const std::string& tenant_id = "");
    bool   free(void* ptr);
    bool   prefetch(void* ptr, size_t bytes, int device_id);
    bool   advise(void* ptr, size_t bytes, MemAdvice advice, int device_id = 0);

    bool   supportsHardwareUnifiedMemory() const;
    Stats  stats() const;
    uint64_t tenantUsage(const std::string& tenant_id) const;
    void   reset();
};
```

### `time_slice_scheduler.h`
**Location:** `include/themis/gpu/time_slice_scheduler.h`

Dynamic per-tenant GPU time-slice dispatcher. Each tenant gets a configurable time window; the scheduler enforces fair access.

```cpp
class GPUTimeSliceScheduler {
public:
    struct TenantConfig {
        uint32_t time_slice_ms  = 100;
        uint32_t priority       = 1;
    };
    using WorkFn = std::function<void()>;

    static GPUTimeSliceScheduler& GetInstance();

    void registerTenant(const std::string& tenant_id, TenantConfig cfg = {});
    void unregisterTenant(const std::string& tenant_id);
    bool isRegistered(const std::string& tenant_id) const;

    void enqueue(const std::string& tenant_id, WorkFn fn);
    int  queueDepth(const std::string& tenant_id) const;

    void dispatch();   // execute one round of scheduling
    void drainAll();
    bool allEmpty() const;

    TenantStats tenantStats(const std::string& tenant_id) const;
    Stats       aggregateStats() const;
    void        resetStats();
};
```

### `mig_manager.h`
**Location:** `include/themis/gpu/mig_manager.h`

MIG (Multi-Instance GPU) partition lifecycle for NVIDIA Ampere and Hopper (A/H series). CPU simulation path active when no NVML hardware is present.

```cpp
class MIGManager {
public:
    enum class Status {
        OK, MIG_NOT_SUPPORTED, MIG_FEATURE_DISABLED, INVALID_PROFILE,
        DEVICE_ERROR, INSTANCE_NOT_FOUND, TENANT_CONFLICT, ALREADY_ASSIGNED
    };
    struct MIGInstance {
        std::string instance_id;
        int         device_index;
        std::string profile;    // e.g. "1g.5gb", "2g.10gb", "4g.20gb"
        uint64_t    vram_bytes;
        std::string tenant_id;  // empty = unassigned
    };

    static MIGManager& GetInstance();

    Status createPartition(const std::string& profile, int device_index,
                           const std::string& instance_id = "");
    Status destroyPartition(const std::string& instance_id);
    Status assignToTenant(const std::string& instance_id, const std::string& tenant_id);
    Status unassignFromTenant(const std::string& instance_id);

    std::vector<MIGInstance> getInstances() const;
    std::vector<MIGInstance> getInstancesOnDevice(int device_index) const;
    std::vector<MIGInstance> getInstancesForTenant(const std::string& tenant_id) const;
    std::optional<MIGInstance> getInstance(const std::string& instance_id) const;

    static bool     supportsMIG(const DeviceInfo& device);
    static bool     isValidProfile(const std::string& profile);
    static uint64_t profileVRAMBytes(const std::string& profile);
};
```

### `p2p_transfer.h`
**Location:** `include/themis/gpu/p2p_transfer.h`

Peer-to-peer GPU-to-GPU direct transfers via NVLink or PCIe. CPU memcpy simulation path always active; tests pass without GPU hardware.

```cpp
class GPUP2PTransferManager {
public:
    enum class Status { OK, PEER_ACCESS_NOT_SUPPORTED, PEER_ACCESS_NOT_ENABLED,
                        TRANSFER_FAILED, INVALID_DEVICE, INVALID_POINTER };

    struct TransferRequest {
        int      src_device;
        int      dst_device;
        uintptr_t src_ptr;       // device pointer cast to uintptr_t
        uintptr_t dst_ptr;
        size_t   size_bytes;
    };
    struct TransferResult {
        Status   status;
        uint64_t bytes_transferred;
        double   bandwidth_gb_s;
    };

    bool   canAccessPeer(int src_device, int dst_device) const;
    Status enablePeerAccess(int src_device, int dst_device);
    Status disablePeerAccess(int src_device, int dst_device);
    bool   isPeerAccessEnabled(int src_device, int dst_device) const;

    TransferResult transfer(const TransferRequest& req);

    Stats stats() const;
    void  reset();
};
```

### `wasm_kernel_sandbox.h`
**Location:** `include/themis/gpu/wasm_kernel_sandbox.h`

WASM-based sandbox for untrusted third-party GPU kernel blobs. Isolates execution, enforces memory limits, and enforces wall-clock timeouts. Requires WasmRuntime injection for production WASM execution.

```cpp
class WASMKernelSandbox {
public:
    struct SandboxConfig {
        uint32_t max_memory_bytes = 64 * 1024 * 1024; // 64 MB
        uint32_t timeout_ms       = 1000;
        bool     allow_host_calls = false;
    };
    enum class Status {
        OK, WASM_NOT_AVAILABLE, EXECUTION_TIMEOUT,
        MEMORY_LIMIT_EXCEEDED, EXECUTION_FAILED, INVALID_BLOB
    };
    struct ExecutionResult {
        Status   status;
        uint64_t execution_time_ms;
        uint64_t peak_memory_bytes;
        std::string error_message;
    };

    explicit WASMKernelSandbox(SandboxConfig cfg = {});

    bool            isWasmAvailable() const;
    ExecutionResult execute(const std::vector<uint8_t>& wasm_blob,
                            const std::string& entry_point = "_start",
                            const std::vector<uint8_t>& input = {});
    Stats stats() const;
    void  resetStats();
};
```

---

## Namespace

All types are declared in `namespace themis::gpu`:

```cpp
namespace themis {
namespace gpu {
    class GPUModule;
    class GPUMemoryManager;
    // ... all types above
} // namespace gpu
} // namespace themis
```

## Thread Safety

All components use internal `std::mutex` or `std::shared_mutex` for thread safety. Concurrent allocations, metric writes, audit-log appends, and work submissions from different threads are safe.

## Edition Limits

| Edition | VRAM Limit | Multi-GPU | MIG | Cluster Coord |
|---------|-----------|-----------|-----|---------------|
| Community | 0 GB (CPU-only) | ❌ | ❌ | ❌ |
| Professional | 8 GB | ❌ | ❌ | ❌ |
| Enterprise | 24 GB | ✅ | ✅ | ✅ |
| Hyperscaler/Unlimited | No limit | ✅ | ✅ | ✅ |

## See Also

- [GPU Module Source Documentation](../../../src/gpu/README.md) — component architecture and quick-start
- [GPU Architecture Guide](../../../src/gpu/ARCHITECTURE.md) — design principles and data flows
- [GPU Roadmap](../../../src/gpu/ROADMAP.md) — delivery phases and open issues
- [GPU Security](../../../src/gpu/SECURITY.md) — threat model and security controls
- [GPU Future Enhancements](../../../src/gpu/FUTURE_ENHANCEMENTS.md) — planned features and integration notes
- [GPU Runbooks](../../../docs/gpu_runbooks.md) — on-call operational procedures
- [LLM Module](../../../src/llm/README.md) — GPU model inference
- [Vector Index](../../../src/index/README.md) — GPU-accelerated ANN indexing

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
