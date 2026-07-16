> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Performance Optimization Benchmarks

This directory contains benchmarking and validation tools for research-based performance optimizations.

## Overview

Based on scientific research documented in:
- [`docs/de/research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md`](../../docs/de/research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md)
- [`docs/de/research/IMPLEMENTATION_VALIDATION_GUIDE.md`](../../docs/de/research/IMPLEMENTATION_VALIDATION_GUIDE.md)

## Quick Start

### Validate an Optimization

```bash
# Example: Validate mimalloc integration
python validate_optimization.py \
  --optimization mimalloc \
  --iterations 10 \
  --min-improvement 10.0 \
  --output mimalloc_validation.json
```

### Available Optimizations

#### Phase 1: Quick Wins (1-3 months)
- `mimalloc` - Fast allocator (+10-20% overall)
- `huge_pages` - 2MB/1GB pages (+15-30% memory-intensive)
- `rcu_index` - RCU for reads (+200-500% read-heavy)
- `lirs_cache` - LIRS cache policy (+30-40% hit rate)

#### Phase 2: Medium-Term (3-6 months)
- `wisckey` - Value separation (+40-60% writes)
- `cicada_cc` - Optimistic CC (+100-150% TX)

#### Phase 3: Long-Term (6-12 months)
- `diskann` - Billion-scale vector search (+300-400%)
- `bw_tree` - Lock-free index (+100-200% updates)

## Validation Workflow

### 1. Before Implementation
```bash
# Record baseline performance
cd benchmarks
python themis_complete_with_constraints.py --mode full --output baseline.json
```

### 2. After Implementation
```bash
# Build with feature flag
cmake -B build -S . -DTHEMIS_ENABLE_MIMALLOC=ON
cmake --build build --config Release

# Run validation
python performance_optimizations/validate_optimization.py \
  --optimization mimalloc \
  --iterations 10
```

### 3. Review Results
```bash
# Check validation report
cat validation_results.json

# Compare statistics
python compare_results.py baseline.json validation_results.json
```

## Validation Criteria

Each optimization must meet:
- ✅ **Minimum 10% improvement** in target metric
- ✅ **Statistical significance** (p < 0.05, t-test)
- ✅ **At least 10 repetitions** for reliable statistics
- ✅ **No regression** in other metrics (±5% tolerance)

## Rollback Strategy

### Tier 1: Runtime (< 1 minute)
```json
// config/performance_optimizations.json
{
  "performance": {
    "enable_mimalloc": false  // Toggle and restart
  }
}
```

### Tier 2: Build-time (< 10 minutes)
```bash
# Rebuild without feature flag
cmake -B build -S . -DTHEMIS_ENABLE_MIMALLOC=OFF
cmake --build build --config Release
```

### Tier 3: Git Revert (< 30 minutes)
```bash
# Revert the commits
git revert <commit-hash>
git push origin main
```

## Expected Performance Gains

### Phase 1 Total: +50-100% (Read-Heavy Workloads)

| Optimization | Effort | Gain | Priority |
|--------------|--------|------|----------|
| Mimalloc | 1 day | +10-20% | ⭐⭐⭐⭐⭐ |
| Huge Pages | 2 days | +15-30% | ⭐⭐⭐⭐ |
| RCU Index | 2 weeks | +200-500% reads | ⭐⭐⭐⭐ |
| LIRS Cache | 1 week | +30-40% hits | ⭐⭐⭐⭐ |

### Phase 2 Total: +100-200% (Overall)

| Optimization | Effort | Gain | Priority |
|--------------|--------|------|----------|
| WiscKey | 4 weeks | +40-60% writes | ⭐⭐⭐⭐ |
| Cicada CC | 6 weeks | +100-150% TX | ⭐⭐⭐⭐ |

### Phase 3 Total: +200-500% (Domain-Specific)

| Optimization | Effort | Gain | Priority |
|--------------|--------|------|----------|
| DiskANN | 8 weeks | +300-400% vector | ⭐⭐⭐⭐⭐ |
| Bw-Tree | 10 weeks | +100-200% index | ⭐⭐⭐⭐ |

## Hardware Requirements

Document your test environment in `baseline_hardware.json`:

```json
{
  "cpu": "Intel Xeon E5-2680 v4",
  "cores": 28,
  "ram_gb": 128,
  "storage": "NVMe SSD",
  "os": "Linux 5.15.0"
}
```

## Research Papers

All optimizations are based on peer-reviewed research:

- **Mimalloc**: "Mimalloc: Free List Sharding in Action" (ISMM'19)
- **Huge Pages**: "Optimizing Database Performance using Huge Pages" (FAST'14)
- **RCU**: "Scalable Read-Mostly Synchronization Using RCU" (ASPLOS'10)
- **LIRS**: "LIRS: An Efficient Low Inter-reference Recency Set" (SIGMETRICS'02)
- **WiscKey**: "WiscKey: Separating Keys from Values" (FAST'16)
- **Cicada**: "Cicada: Dependably Fast Multi-Core Transactions" (SIGMOD'17)
- **DiskANN**: "DiskANN: Fast Billion-point Nearest Neighbor Search" (NeurIPS'19)
- **Bw-Tree**: "The Bw-Tree: A Lock-Free B-Tree" (ICDE'18)

## Contributing

When adding a new optimization:

1. ✅ Add CMake option to `CMakeLists.txt`
2. ✅ Add runtime flag to `include/performance/feature_flags.h`
3. ✅ Add config entry to `config/performance_optimizations.json`
4. ✅ Create validation script
5. ✅ Document expected gains in research docs
6. ✅ Run validation with ≥10 iterations
7. ✅ Submit PR with benchmark results

## Support

For questions:
- 📖 Documentation: `docs/de/research/`
- 💬 Discussions: GitHub Discussions (tag: research)
- 📧 Email: research@themisdb.com

---

**Last Updated**: 2026-04-06  
**Version**: 1.0  
**Status**: ✅ Infrastructure Ready for Phase 1
