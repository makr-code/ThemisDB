# Performance Troubleshooting Guide

The `performance` module provides low-level performance optimisation tooling for ThemisDB, including cycle-accurate metrics collection, NUMA topology awareness, async metrics export, feature flag management, and memory allocator integration (mimalloc, hugepages).

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `CycleMetrics: counter overflow` | Collection interval too long | Reduce `performance.cycle_metrics.interval_ms` |
| NUMA allocation always on node 0 | NUMA topology not detected | Enable `performance.numa.enabled: true` |
| `AsyncMetricsExporter: queue full` | Export rate too slow | Increase exporter threads |
| Feature flag changes not applied | Flag cache not invalidated | Call `themisdb-admin perf flags reload` |
| `Dostoevsky: bloom filter miss rate high` | Bloom filter too small | Increase `performance.bloom_filter.bits_per_key` |
| High memory fragmentation | Default allocator fragmentation | Switch to mimalloc |
| `Ligra: graph processing OOM` | Graph too large for available RAM | Use external memory mode |
| CPU cache miss rate high | NUMA cross-node access | Pin threads to NUMA node |
| `ChimeraExporter: connection refused` | Chimera adapter not running | Start chimera adapter service |
| Async metrics export lag | Batch size too small | Increase `performance.async_export.batch_size` |

## Common Issues

### Issue 1: NUMA Cross-Node Memory Access

**Description:** ThemisDB allocates memory on a remote NUMA node, causing high memory latency.

**Symptoms:**
- Metric `themisdb_numa_remote_access_pct > 30`
- Query latency is 2× higher than expected on multi-socket servers

**Cause:** NUMA topology awareness is disabled; memory is allocated from any NUMA node.

**Solution:**
```yaml
performance:
  numa:
    enabled: true
    pin_threads_to_node: true
    interleave_policy: local_first  # prefer local NUMA node
    huge_pages:
      enabled: true
      size: 2MB
```
```bash
# Check NUMA topology
numactl --hardware
numastat

# Run ThemisDB with NUMA binding
numactl --membind=0 --cpunodebind=0 /usr/bin/themisdb
```

---

### Issue 2: Memory Fragmentation from Default Allocator

**Description:** ThemisDB RSS grows significantly beyond actual data size due to allocator fragmentation.

**Symptoms:**
- RSS is 3× the data size in memory
- `jemalloc` or `malloc` stats show high fragmentation

**Cause:** Default system allocator fragments under ThemisDB's mixed-size allocation patterns.

**Solution:**
```yaml
performance:
  allocator: mimalloc             # "system" | "mimalloc" | "jemalloc"
  mimalloc:
    page_reset: true
    eager_commit: true
    arena_count: 8
```
```bash
# Set allocator via LD_PRELOAD (alternative)
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libmimalloc.so themisdb
```

---

### Issue 3: Feature Flag Changes Not Taking Effect

**Description:** Feature flags updated via admin API are not reflected in server behaviour.

**Symptoms:**
- Log: `Phase2FeatureFlags: cache hit for flag 'enable_gpu' (cached 3600s)`
- Flag change takes 1 hour to propagate

**Cause:** Feature flag cache TTL is too long.

**Solution:**
```yaml
performance:
  feature_flags:
    cache_ttl_ms: 10000           # 10 seconds instead of 1 hour
    reload_on_change: true
    hot_reload: true
```
```bash
# Force reload feature flags
themisdb-admin perf flags reload

# List current flags
themisdb-admin perf flags list
```

---

### Issue 4: Dostoevsky Bloom Filter High Miss Rate

**Description:** RocksDB read amplification is high because the bloom filter is too small.

**Symptoms:**
- Metric `themisdb_rocksdb_bloom_filter_miss_rate > 0.10`
- Read latency is high despite data being in cache

**Cause:** `bits_per_key` too low; bloom filter has too many false positives.

**Solution:**
```yaml
performance:
  bloom_filter:
    bits_per_key: 15              # increase from default 10; 15 gives ~1% FP rate
    whole_key_filtering: true
    cache_index_and_filter_blocks: true
```

---

### Issue 5: Cycle Metrics Counter Overflow

**Description:** Hardware performance counter overflows, producing incorrect metrics.

**Symptoms:**
- Log: `CycleMetrics: counter overflow detected on CPU 3`
- Cycle count drops to 0 unexpectedly

**Cause:** Collection interval too long; `perf` counter wraps on long intervals.

**Solution:**
```yaml
performance:
  cycle_metrics:
    enabled: true
    interval_ms: 100              # collect every 100ms to avoid overflow
    events:
      - instructions
      - cache_misses
      - branch_misses
    per_cpu: true
```

---

### Issue 6: Async Metrics Export Queue Overflow

**Description:** The asynchronous metrics export queue fills up and drops metrics.

**Symptoms:**
- Log: `AsyncMetricsExporter: queue full (1000); dropping metric`
- Prometheus shows gaps in metrics

**Cause:** Export rate is too slow; queue too small.

**Solution:**
```yaml
performance:
  async_export:
    queue_size: 10000             # increase from 1000
    batch_size: 500               # export in larger batches
    export_interval_ms: 1000
    exporter_threads: 4           # more threads for faster export
```

---

### Issue 7: Hugepages Not Allocated

**Description:** ThemisDB is configured to use hugepages but falls back to regular pages.

**Symptoms:**
- Log: `HugePages: mmap with MAP_HUGETLB failed; falling back to 4K pages`
- No performance improvement despite hugepages config

**Cause:** Hugepages not pre-allocated in the kernel.

**Solution:**
```bash
# Allocate 2GB hugepages (1024 × 2MB)
echo 1024 > /proc/sys/vm/nr_hugepages

# Persistent allocation
echo "vm.nr_hugepages = 1024" >> /etc/sysctl.conf
sysctl -p

# Verify allocation
cat /proc/meminfo | grep HugePages
```

## Diagnostic Commands

```bash
# Performance overview
themisdb-admin perf status

# NUMA stats
themisdb-admin perf numa-stats
numastat -p $(pidof themisdb)

# Feature flags
themisdb-admin perf flags list

# Cycle metrics
themisdb-admin perf cycle-stats

# Async export queue depth
curl -s http://localhost:9100/metrics | grep themisdb_perf_export_queue

# Tail performance logs
journalctl -u themisdb -f | grep -E "perf|numa|hugepage|bloom|cycle|feature.flag"
```

## Configuration Reference

```yaml
performance:
  allocator: mimalloc
  numa:
    enabled: true
    pin_threads_to_node: false
  huge_pages:
    enabled: false
  bloom_filter:
    bits_per_key: 15
  cycle_metrics:
    enabled: false
    interval_ms: 100
  async_export:
    queue_size: 5000
    exporter_threads: 2
  feature_flags:
    cache_ttl_ms: 30000
```

## Known Limitations

- Cycle metrics (`perf_event`) require `CAP_PERFMON` or `kernel.perf_event_paranoid <= 1`.
- Hugepages with `MAP_HUGETLB` require pre-allocation at boot; transparent hugepages are an alternative.
- Ligra graph processing algorithms are single-node only; use the graph module for distributed processing.
- mimalloc allocator may increase startup time slightly; the improvement is visible in steady-state workloads.

## Related Documentation

- [Performance Module ROADMAP](../../src/performance/ROADMAP.md)
- [Phase 1 Hugepages Implementation](../ARCHIVED/implementation-summaries/PHASE1_HUGE_PAGES_IMPLEMENTATION.md)
- [Phase 1 mimalloc Implementation](../ARCHIVED/implementation-summaries/PHASE1_MIMALLOC_IMPLEMENTATION.md)
- [Performance Roadmap](../PERFORMANCE_ROADMAP.md)
- [MMAP Performance Impact](../performance/MMAP_PERFORMANCE_IMPACT.md)
- [Build Performance Optimizations](../BUILD_PERFORMANCE_OPTIMIZATIONS.md)
