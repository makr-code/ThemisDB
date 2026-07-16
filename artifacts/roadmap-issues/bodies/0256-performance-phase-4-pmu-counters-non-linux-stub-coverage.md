### Context

This issue implements the roadmap item 'Phase 4: PMU Counters — Non-Linux Stub Coverage' for the performance domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.9.0.

Primary detail section: Phase 4: PMU Counters — Non-Linux Stub Coverage

### Goal

Deliver the scoped changes for Phase 4: PMU Counters — Non-Linux Stub Coverage in src/performance/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### Phase 4: PMU Counters — Non-Linux Stub Coverage
**Priority:** Low
**Target Version:** v1.9.0

`phase4/pmu_counters.cpp` has explicit non-Linux stubs (lines 186, 218): all PMU counters report "unavailable" on non-Linux platforms and when disabled at compile time. macOS (`kperf`) and Windows (`QueryPerformanceCounter` + ETW hardware counters) support is not implemented.

**Implementation Notes:**
- `[ ]` Implement macOS PMU backend using `kperf` / `kpc` private API (available since macOS 10.12, public in macOS 14+) behind `#ifdef __APPLE__`.
- `[ ]` Implement Windows PMU backend using `QueryThreadCycleTime` + ETW hardware counter session behind `#ifdef _WIN32`.
- `[ ]` All non-Linux platforms should at minimum report `RDTSC`-based cycle counts as a fallback so the `CycleMetrics` class is not entirely useless on developer workstations.

---


**Priority:** High  
**Target Version:** v1.8.0  
**Research Basis:** Multiple papers on GPU database acceleration

Hardware acceleration for compute-intensive database operations using GPUs, FPGAs, and specialized accelerators.

**Features:**
- **GPU-Accelerated Joins**: Hash joins, sort-merge joins on CUDA/ROCm
- **FPGA Query Offload**: Pattern matching, compression/decompression
- **Vector Engine Integration**: ARM SVE, Intel AVX-512 for SIMD operations
- **Smart NIC Offload**: Filtering, aggregation at network card
- **Persistent Memory (PMem)**: Direct access to byte-addressable NVM

**Architecture:**
```cpp
class HardwareAccelerator {
public:
    enum class DeviceType {
        GPU_CUDA,
        GPU_ROCM,
        FPGA_INTEL,
        FPGA_XILINX,
        VECTOR_ENGINE,
        SMART_NIC,
        PMEM
    };
    
    struct AcceleratorConfig {
        DeviceType device;
        size_t device_memory_mb = 8192;
        bool enable_pipelining = true;
        bool enable_async_copy = true;
        size_t batch_size = 10000;
    };
    
    // Execute query operator on accelerator
    Result<ExecutionResult> execute(
        const QueryOperator& op,
        const AcceleratorConfig& config);
    
    // Check if operator can be accelerated
    bool can_accelerate(const QueryOperator& op) const;
    
    // Estimate speedup factor
    double estimate_speedup(const QueryOperator& op) const;
};

// Example usage
HardwareAccelerator accel;
if (accel.can_accelerate(join_operator)) {
    auto result = accel.execute(join_operator, {
        .device = DeviceType::GPU_CUDA,
        .device_memory_mb = 16384,
        .batch_size = 100000
    });
    // 5-20x speedup for large joins
}
```

**Performance Targets:**
- **Joins**: 5-20x speedup for >1M rows
- **Aggregations**: 10-50x speedup for complex aggregates
- **Pattern Matching**: 50-100x speedup with FPGA
- **Vector Operations**: 4-16x speedup with SIMD

**Implementation Phases:**
1. **Phase 1**: GPU join acceleration (v1.8.0)
2. **Phase 2**: FPGA pattern matching (v1.9.0)
3. **Phase 3**: Vector engine integration (v2.0.0)
4. **Phase 4**: Smart NIC and PMem (v2.1.0)

**Integration Points:**
- Query optimizer: Cost model for hardware selection
- Execution engine: Operator dispatch to accelerators
- Memory manager: Unified memory across devices

---

### Acceptance Criteria

- [ ] Implement macOS PMU backend using `kperf` / `kpc` private API (available since macOS 10.12, public in macOS 14+) behind `#ifdef __APPLE__`.
- [ ] Implement Windows PMU backend using `QueryThreadCycleTime` + ETW hardware counter session behind `#ifdef _WIN32`.
- [ ] All non-Linux platforms should at minimum report `RDTSC`-based cycle counts as a fallback so the `CycleMetrics` class is not entirely useless on developer workstations.
- [ ] **GPU-Accelerated Joins**: Hash joins, sort-merge joins on CUDA/ROCm
- [ ] **FPGA Query Offload**: Pattern matching, compression/decompression
- [ ] **Vector Engine Integration**: ARM SVE, Intel AVX-512 for SIMD operations
- [ ] **Smart NIC Offload**: Filtering, aggregation at network card
- [ ] **Persistent Memory (PMem)**: Direct access to byte-addressable NVM
- [ ] **Joins**: 5-20x speedup for >1M rows
- [ ] **Aggregations**: 10-50x speedup for complex aggregates
- [ ] **Pattern Matching**: 50-100x speedup with FPGA
- [ ] **Vector Operations**: 4-16x speedup with SIMD
- [ ] **Phase 1**: GPU join acceleration (v1.8.0)
- [ ] **Phase 2**: FPGA pattern matching (v1.9.0)
- [ ] **Phase 3**: Vector engine integration (v2.0.0)
- [ ] **Phase 4**: Smart NIC and PMem (v2.1.0)
- [ ] Query optimizer: Cost model for hardware selection
- [ ] Execution engine: Operator dispatch to accelerators
- [ ] Memory manager: Unified memory across devices

### Relationships

- Roadmap row: #256 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/performance/FUTURE_ENHANCEMENTS.md#phase-4-pmu-counters--non-linux-stub-coverage
- Source key: roadmap:256:performance:v1.9.0:phase-4-pmu-counters-non-linux-stub-coverage

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:256:performance:v1.9.0:phase-4-pmu-counters-non-linux-stub-coverage -->
<!-- roadmap-ref: row=256;module=performance;target=v1.9.0 -->
<!-- roadmap-detail: src/performance/FUTURE_ENHANCEMENTS.md#phase-4-pmu-counters--non-linux-stub-coverage -->
