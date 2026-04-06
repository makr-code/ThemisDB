# Performance Optimizations Build Guide

This guide explains how to build ThemisDB with research-based performance optimizations.

## Overview

ThemisDB implements performance optimizations based on scientific research from leading conferences (SIGMOD, VLDB, OSDI, NeurIPS, etc.). Each optimization can be enabled independently via CMake flags.

## Quick Start

### 1. Standard Build (No Optimizations)

```bash
cmake -B build -S .
cmake --build build --config Release
```

### 2. Enable Specific Optimizations

```bash
# Enable mimalloc (+10-20% overall performance)
cmake -B build -S . -DTHEMIS_ENABLE_MIMALLOC=ON

# Enable multiple optimizations
cmake -B build -S . \
  -DTHEMIS_ENABLE_MIMALLOC=ON \
  -DTHEMIS_ENABLE_HUGE_PAGES=ON \
  -DTHEMIS_ENABLE_RCU_INDEX=ON

cmake --build build --config Release
```

### 3. Phase 1 Quick Wins (Recommended First Step)

```bash
cmake -B build -S . \
  -DTHEMIS_ENABLE_MIMALLOC=ON \
  -DTHEMIS_ENABLE_HUGE_PAGES=ON \
  -DTHEMIS_ENABLE_LIRS_CACHE=ON

cmake --build build --config Release
```

## Available Optimization Flags

### Phase 1: Quick Wins (1-3 Months)

**Low effort, high impact optimizations**

#### `THEMIS_ENABLE_MIMALLOC`
- **Description**: Use mimalloc allocator instead of system malloc
- **Expected Gain**: +10-20% overall performance
- **Paper**: "Mimalloc: Free List Sharding in Action" (ISMM'19)
- **Effort**: 1 day
- **Dependencies**: mimalloc library (vcpkg)
- **Status**: Infrastructure ready, implementation pending

```bash
cmake -B build -S . -DTHEMIS_ENABLE_MIMALLOC=ON
```

#### `THEMIS_ENABLE_HUGE_PAGES`
- **Description**: Enable 2MB/1GB huge pages for memory allocation
- **Expected Gain**: +15-30% for memory-intensive workloads
- **Paper**: "Optimizing Database Performance using Huge Pages" (FAST'14)
- **Effort**: 2 days
- **System Requirement**: `echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages`
- **Status**: Infrastructure ready, implementation pending

```bash
cmake -B build -S . -DTHEMIS_ENABLE_HUGE_PAGES=ON
```

#### `THEMIS_ENABLE_RCU_INDEX`
- **Description**: Use Read-Copy-Update for read-heavy index paths
- **Expected Gain**: +200-500% for read-heavy workloads (90%+ reads)
- **Paper**: "Scalable Read-Mostly Synchronization Using RCU" (ASPLOS'10)
- **Effort**: 2 weeks
- **Status**: Infrastructure ready, implementation pending

```bash
cmake -B build -S . -DTHEMIS_ENABLE_RCU_INDEX=ON
```

#### `THEMIS_ENABLE_LIRS_CACHE`
- **Description**: Use LIRS cache replacement policy instead of LRU
- **Expected Gain**: +30-40% cache hit rate
- **Paper**: "LIRS: Low Inter-reference Recency Set" (SIGMETRICS'02)
- **Effort**: 1 week
- **Status**: Infrastructure ready, implementation pending

```bash
cmake -B build -S . -DTHEMIS_ENABLE_LIRS_CACHE=ON
```

### Phase 2: Medium-Term (3-6 Months)

**Higher impact, medium effort optimizations**

#### `THEMIS_ENABLE_WISCKEY`
- **Description**: Separate large values (>1KB) from keys in LSM-tree
- **Expected Gain**: +40-60% write throughput for values >1KB
- **Paper**: "WiscKey: Separating Keys from Values" (FAST'16)
- **Effort**: 4 weeks
- **Status**: Infrastructure ready, implementation pending

```bash
cmake -B build -S . -DTHEMIS_ENABLE_WISCKEY=ON
```

#### `THEMIS_ENABLE_CICADA_CC`
- **Description**: Use Cicada optimistic concurrency control
- **Expected Gain**: +100-150% transaction throughput
- **Paper**: "Cicada: Dependably Fast Multi-Core Transactions" (SIGMOD'17)
- **Effort**: 6 weeks
- **Status**: Infrastructure ready, implementation pending

```bash
cmake -B build -S . -DTHEMIS_ENABLE_CICADA_CC=ON
```

### Phase 3: Long-Term (6-12 Months)

**Very high impact, high effort optimizations**

#### `THEMIS_ENABLE_DISKANN`
- **Description**: Use DiskANN for billion-scale vector search
- **Expected Gain**: +300-400% throughput for >100M vectors
- **Paper**: "DiskANN: Fast Billion-point Nearest Neighbor Search" (NeurIPS'19)
- **Effort**: 8 weeks
- **Status**: Infrastructure ready, implementation pending

```bash
cmake -B build -S . -DTHEMIS_ENABLE_DISKANN=ON
```

#### `THEMIS_ENABLE_BW_TREE`
- **Description**: Use lock-free Bw-Tree for indexes
- **Expected Gain**: +100-200% index update throughput
- **Paper**: "The Bw-Tree: A Lock-Free B-Tree" (ICDE'18)
- **Effort**: 10 weeks
- **Status**: Infrastructure ready, implementation pending

```bash
cmake -B build -S . -DTHEMIS_ENABLE_BW_TREE=ON
```

## Runtime Configuration

Even with compile-time flags enabled, optimizations can be toggled at runtime via configuration file:

### `config/performance_optimizations.json`

```json
{
  "performance": {
    "enable_mimalloc": true,
    "enable_huge_pages": true,
    "enable_rcu_index": false,
    "enable_lirs_cache": true
  }
}
```

### Load configuration:

```cpp
#include <performance/feature_flags.h>

// Load from JSON config
auto config = load_performance_config("config/performance_optimizations.json");
themis::performance::PerformanceFeatureFlags::instance().load_from_config(config);
```

## Validation Workflow

### Before Enabling in Production

1. **Baseline Benchmark**:
   ```bash
   cd benchmarks
   python themis_complete_with_constraints.py --mode full --output baseline.json
   ```

2. **Build with Optimization**:
   ```bash
   cmake -B build -S . -DTHEMIS_ENABLE_MIMALLOC=ON
   cmake --build build --config Release
   ```

3. **Run Validation**:
   ```bash
   python performance_optimizations/validate_optimization.py \
     --optimization mimalloc \
     --iterations 10 \
     --min-improvement 10
   ```

4. **Review Results**:
   - Check `validation_results.json`
   - Ensure ≥10% improvement
   - Verify no regressions in other metrics

## Rollback Strategy

### Tier 1: Runtime (< 1 minute)
Edit `config/performance_optimizations.json` and restart:
```json
{"performance": {"enable_mimalloc": false}}
```

### Tier 2: Build-time (< 10 minutes)
Rebuild without flag:
```bash
cmake -B build -S . -DTHEMIS_ENABLE_MIMALLOC=OFF
cmake --build build --config Release
```

### Tier 3: Git Revert (< 30 minutes)
```bash
git revert <commit-hash>
git push origin main
```

## Implementation Status

| Optimization | Flag | Status | ETA |
|--------------|------|--------|-----|
| Mimalloc | `THEMIS_ENABLE_MIMALLOC` | 🟡 Infrastructure ready | - |
| Huge Pages | `THEMIS_ENABLE_HUGE_PAGES` | 🟡 Infrastructure ready | - |
| RCU Index | `THEMIS_ENABLE_RCU_INDEX` | 🟡 Infrastructure ready | - |
| LIRS Cache | `THEMIS_ENABLE_LIRS_CACHE` | 🟡 Infrastructure ready | - |
| WiscKey | `THEMIS_ENABLE_WISCKEY` | 🟡 Infrastructure ready | - |
| Cicada CC | `THEMIS_ENABLE_CICADA_CC` | 🟡 Infrastructure ready | - |
| DiskANN | `THEMIS_ENABLE_DISKANN` | 🟡 Infrastructure ready | - |
| Bw-Tree | `THEMIS_ENABLE_BW_TREE` | 🟡 Infrastructure ready | - |

🟢 Implemented | 🟡 Infrastructure ready | 🔴 Not started

## Documentation

- **Research Papers**: [`docs/de/research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md`](../docs/de/research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md)
- **Implementation Guide**: [`docs/de/research/IMPLEMENTATION_VALIDATION_GUIDE.md`](../docs/de/research/IMPLEMENTATION_VALIDATION_GUIDE.md)
- **Additional Sources**: [`docs/de/research/ADDITIONAL_SOURCES.md`](../docs/de/research/ADDITIONAL_SOURCES.md)

## Contributing

When implementing an optimization:

1. ✅ Follow implementation guide in research docs
2. ✅ Add unit tests for new functionality
3. ✅ Run validation benchmark (≥10 iterations)
4. ✅ Document rollback procedure
5. ✅ Update this guide with implementation status
6. ✅ Submit PR with benchmark results

## Support

- 📖 Full documentation: `docs/de/research/`
- 💬 GitHub Discussions: Tag `performance`
- 📧 Email: research@themisdb.com

---

**Last Updated**: 2026-04-06  
**Version**: 1.0  
**Status**: ✅ Infrastructure complete, implementations pending
