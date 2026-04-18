# Index Troubleshooting Guide

The `index` module provides advanced indexing for ThemisDB including HNSW vector indices, graph indices, temporal indices, GPU-accelerated approximate nearest neighbor (ANN) search, learnable quantization, and multi-GPU search.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| Low ANN recall (< 80%) | `ef_search` too low | Increase `index.hnsw.ef_search` |
| `GpuVectorIndex: CUDA OOM` | Index too large for VRAM | Reduce `index.gpu.max_vram_fraction` or use CPU fallback |
| Index build takes hours | `ef_construction` too high | Lower `ef_construction` for faster builds |
| `BinaryQuantizer: threshold not calibrated` | Quantizer trained on wrong distribution | Retrain quantizer on representative sample |
| Duplicate results in ANN search | Multi-GPU result deduplication disabled | Enable `index.multi_gpu.deduplicate_results` |
| `TemporalIndex: version not found` | Retention window too short | Increase `index.temporal.retention_days` |
| Graph index missing edges | Edge type not registered | Register edge type before inserting edges |
| `ApproximateRadiusSearch: FP rate too high` | Radius too large for index resolution | Tighten radius or use exact search for small radii |
| Index not used for range queries | Index type mismatch | Use range index; vector index is ANN-only |
| `AdaptiveIndex: strategy oscillation` | Workload classifier window too short | Increase `index.adaptive.stability_window_ms` |

## Common Issues

### Issue 1: Low ANN Recall

**Description:** Vector similarity searches return results with poor recall quality.

**Symptoms:**
- Recall@10 < 0.80 in evaluation
- Users report irrelevant semantic search results

**Cause:** `ef_search` (exploration factor at query time) is too low.

**Solution:**
```yaml
index:
  hnsw:
    ef_search: 200               # increase from default 50
    ef_construction: 200         # used at index build time
    m: 16                        # number of connections per node
    max_m: 32
```
```bash
# Evaluate recall for a collection
themisdb-admin index recall-test \
  --collection products \
  --field embedding \
  --k 10 \
  --ground-truth /tmp/gt.jsonl
```

---

### Issue 2: GPU Vector Index OOM

**Description:** Loading a large vector index into GPU memory fails.

**Symptoms:**
- Log: `GpuVectorIndex: CUDA OOM – requested 8192MB, available 6144MB`
- Index falls back to CPU, causing 10× slower search

**Cause:** Index is too large for available VRAM.

**Solution:**
```yaml
index:
  gpu:
    enabled: true
    device_ids: [0]
    max_vram_fraction: 0.70       # leave 30% VRAM for model
    fallback_to_cpu: true         # never crash; slow gracefully
    pq_compression:
      enabled: true
      subvectors: 64              # compress 1536d→64 subvectors
```
```bash
# Check available GPU memory
nvidia-smi --query-gpu=memory.free --format=csv,noheader

# Show index memory footprint
themisdb-admin index memory-usage --index products_embedding
```

---

### Issue 3: Slow Index Build for Large Collections

**Description:** Building an HNSW index on a large collection takes unacceptably long.

**Symptoms:**
- Log: `AdvancedVectorIndex: build progress 10% (eta: 8h)`
- Index build blocks other operations

**Cause:** `ef_construction` is too high; single-threaded build.

**Solution:**
```yaml
index:
  hnsw:
    ef_construction: 100          # reduce from 400 for faster builds
    build_threads: 8              # use parallel build
    build_batch_size: 10000
  build:
    background: true              # build in background
    max_cpu_fraction: 0.50        # don't starve other workloads
```

---

### Issue 4: Binary Quantizer Not Calibrated

**Description:** The binary quantizer threshold was not set for the data distribution, causing poor recall.

**Symptoms:**
- Log: `BinaryQuantizer: threshold=0.0 (not calibrated); using mean`
- ANN recall drops from 0.95 to 0.60 after enabling binary quantization

**Cause:** Quantizer must be calibrated on a representative sample of the vector data.

**Solution:**
```bash
# Calibrate quantizer on existing data
themisdb-admin index calibrate-quantizer \
  --collection products \
  --field embedding \
  --quantizer binary \
  --sample-size 50000
```
```yaml
index:
  binary_quantizer:
    auto_calibrate: true
    calibration_sample_size: 50000
    calibration_schedule: "0 1 * * 0"   # weekly recalibration
```

---

### Issue 5: Temporal Index Version Not Found

**Description:** Point-in-time queries fail because old index versions have been deleted.

**Symptoms:**
- Error: `TemporalIndex: version at timestamp=2025-01-01T00:00:00Z not found`
- Historical queries return empty results

**Cause:** Temporal index retention window is shorter than the requested history.

**Solution:**
```yaml
index:
  temporal:
    retention_days: 365            # extend from default 30 days
    snapshot_interval_hours: 6     # more granular snapshots
    compact_old_versions: true
    keep_every_nth_version: 4      # thin out old versions to save space
```

---

### Issue 6: Graph Index Missing Edges After Bulk Import

**Description:** After a bulk insert of edges, some edges are missing from graph queries.

**Symptoms:**
- AQL traversal returns fewer edges than inserted
- Log: `GraphIndex: edge type 'FOLLOWS' not found; skipping insert`

**Cause:** Edge type was not registered in the index schema before bulk insert.

**Solution:**
```bash
# Register edge types before inserting
themisdb-admin index register-edge-type \
  --collection social_graph \
  --type FOLLOWS

# Rebuild index to pick up missed edges
themisdb-admin index rebuild \
  --collection social_graph \
  --type graph
```

---

### Issue 7: Multi-GPU Search Returns Duplicate Results

**Description:** ANN search returns the same document multiple times when using multi-GPU search.

**Symptoms:**
- Result set contains duplicate `_key` values
- Log: `GpuVectorIndex: multi-GPU merge: 15 duplicates found`

**Cause:** Each GPU searches a partition of the index; results are merged without deduplication.

**Solution:**
```yaml
index:
  multi_gpu:
    enabled: true
    device_ids: [0, 1, 2, 3]
    partition_strategy: sharded    # "sharded" | "replicated"
    deduplicate_results: true      # remove duplicates in merge step
    merge_topk: 200                # merge more candidates before final top-k
```

---

### Issue 8: Adaptive Index Strategy Oscillates

**Description:** The adaptive index switches strategies too frequently, causing repeated rebuilds.

**Symptoms:**
- Log: `AdaptiveIndex: switching strategy from hnsw to flat (workload change)`
- Index rebuild notifications every few minutes

**Cause:** Workload classifier window too short; noisy query patterns trigger frequent switches.

**Solution:**
```yaml
index:
  adaptive:
    enabled: true
    stability_window_ms: 300000    # require 5 min stable workload
    min_strategy_duration_ms: 3600000   # hold strategy for at least 1 hour
    rebuild_threshold_queries: 10000    # don't rebuild for small changes
```

## Diagnostic Commands

```bash
# List all indices
themisdb-admin index list --collection products

# Show index statistics
themisdb-admin index stats --index products_embedding

# Rebuild an index
themisdb-admin index rebuild --collection products --index products_embedding

# Evaluate ANN recall
themisdb-admin index recall-test \
  --collection products --field embedding --k 10

# Check GPU index memory usage
themisdb-admin index memory-usage --gpu

# Live index metrics
curl -s http://localhost:9100/metrics | grep themisdb_index

# Tail index logs
journalctl -u themisdb -f | grep -E "index|hnsw|quantiz|temporal|graph|gpu"
```

## Configuration Reference

```yaml
index:
  hnsw:
    ef_search: 100
    ef_construction: 200
    m: 16
    max_m: 32
    build_threads: 4
  gpu:
    enabled: false
    device_ids: [0]
    max_vram_fraction: 0.70
    fallback_to_cpu: true
  multi_gpu:
    enabled: false
    deduplicate_results: true
  temporal:
    retention_days: 90
    snapshot_interval_hours: 24
  binary_quantizer:
    auto_calibrate: true
    calibration_sample_size: 10000
  adaptive:
    enabled: true
    stability_window_ms: 300000
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `hnsw.ef_search` | `16` | `100–200` for high recall |
| `gpu.fallback_to_cpu` | `false` | `true` to avoid OOM crashes |
| `temporal.retention_days` | `7` | Match your query history depth |
| `multi_gpu.deduplicate_results` | `false` | `true` |

## Known Limitations

- HNSW index does not support online deletion; deleted vectors remain until index rebuild.
- GPU vector index requires CUDA 11.7+ or ROCm 5.4+.
- Binary quantization reduces memory by 32× but degrades recall by 5–15%.
- Graph index does not support weighted edges in the current implementation.
- Temporal index snapshots consume significant disk; monitor with `themisdb-admin index temporal-usage`.

## Related Documentation

- [Index Module ROADMAP](../../src/index/ROADMAP.md)
- [Index Roadmap](../index_roadmap.md)
- [Multi-Vector Search](../multi_vector_search.md)
- [Approximate Radius Search](../ApproximateRadiusSearch.md)
- [Vector Compression Implementation](../implementation-history/summaries/VECTOR_COMPRESSION_IMPLEMENTATION_SUMMARY.md)
- [GPU Roadmap](../gpu_roadmap.md)
