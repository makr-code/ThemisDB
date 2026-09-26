# Embedding & Index Version Governance

**Status:** Design & Implementation  
**Created:** 2026-09-24  
**Version:** 1.0.0 (Specification)  
**Target Completion:** Q4 2026 – Q1 2027  
**Scope:** Versioning contract for embedding models, index schemas, and reindex decisions  

---

## Purpose

This document defines the binding versioning and lifecycle governance for embeddings and indexes in ThemisDB RAG systems. The contract ensures:

1. **Deterministic Reindex Decisions:** Embedding/chunking changes automatically trigger reindex decisions
2. **Version Tracking:** All index writes include metadata about the embedding model, dimensions, chunking profile, and schema version
3. **Migration Safety:** Canary deployments with dual-read comparison before full cutover
4. **Audit Trail:** Index manifests preserve version history for debugging and rollback

---

## Version Schema

### Embedding Model Versioning

Every embedding operation must track:

```json
{
  "embedding_metadata": {
    "embedding_model_id": "all-minilm-l6-v2-1.0",
    "embedding_model_name": "all-MiniLM-L6-v2",
    "embedding_model_version": "1.0",
    "embedding_dim": 384,
    "embedding_hash": "e5f3e3c9d7b4a1f6823c4d9e0a7b2f8c1d5e9a3b6c4f2e8d1a7b3c5e9f2a4d6",
    "embedding_normalized": true,
    "embedding_provider": "huggingface",
    "embedding_commit_hash": "abc123def456"
  }
}
```

**Uniqueness:** `embedding_model_id` format: `{model_name}-{version}` (e.g., `all-minilm-l6-v2-1.0`)

### Chunking Profile Versioning

```json
{
  "chunking_metadata": {
    "chunking_profile_id": "default_256_overlap_32",
    "chunk_size_bytes": 256,
    "chunk_overlap_bytes": 32,
    "chunking_strategy": "sliding_window",
    "preserve_sentence_boundaries": true,
    "min_chunk_size": 64,
    "max_chunk_size": 512,
    "version": "1.0"
  }
}
```

### Index Schema Versioning

```json
{
  "index_schema": {
    "schema_version": "2.0",
    "index_type": "hybrid_bm25_hnsw",
    "bm25_params": {
      "k1": 1.5,
      "b": 0.75,
      "delta": 0.5
    },
    "hnsw_params": {
      "m": 16,
      "ef_construction": 200,
      "ef_search": 50,
      "metric": "cosine"
    },
    "rrf_fusion_k": 60
  }
}
```

---

## Reindex Decision Logic

### Trigger Conditions

A reindex is **required** when:

1. **Embedding Model Change:**
   - `old.embedding_model_id ≠ new.embedding_model_id`
   - Example: Upgrade from `all-minilm-l6-v2-0.9` to `all-minilm-l6-v2-1.0`

2. **Embedding Dimensionality Change:**
   - `old.embedding_dim ≠ new.embedding_dim`
   - Example: Switch from 384-dim to 1024-dim model

3. **Chunking Profile Change:**
   - `old.chunking_profile_id ≠ new.chunking_profile_id`
   - Example: Change chunk size from 256 to 512 bytes

4. **Index Schema Major Version Change:**
   - `old.schema_version.major ≠ new.schema_version.major`
   - Example: Upgrade from schema 1.x to 2.0

### Decision Engine Implementation

```cpp
// pseudo-code in src/ingestion/reindex_decision_engine.cpp

struct ReindexDecision {
  bool reindex_required;
  std::string reason;  // "embedding_model_change", "chunking_change", etc.
  double estimated_duration_seconds;
  bool atomic_rollback_available;
};

ReindexDecision decideReindex(
    const EmbeddingMetadata& old_metadata,
    const EmbeddingMetadata& new_metadata,
    const ChunkingMetadata& new_chunking,
    const IndexSchema& new_schema) {
  
  if (old_metadata.embedding_model_id != new_metadata.embedding_model_id) {
    return {true, "embedding_model_change", 3600.0, true};
  }
  
  if (old_metadata.embedding_dim != new_metadata.embedding_dim) {
    return {true, "embedding_dim_change", 3600.0, true};
  }
  
  if (old_chunking.profile_id != new_chunking.profile_id) {
    return {true, "chunking_profile_change", 1800.0, true};
  }
  
  if (old_schema.major_version != new_schema.major_version) {
    return {true, "index_schema_major_change", 7200.0, true};
  }
  
  return {false, "no_reindex_needed", 0.0, true};
}
```

---

## Index Manifest Schema

All index writes must produce an `INDEX_MANIFEST.json` containing:

```json
{
  "manifest_version": "1.0",
  "index_id": "wiki-index-main",
  "created_at": "2026-09-24T05:43:22+00:00",
  "last_updated_at": "2026-09-24T06:30:15+00:00",
  
  "current_version": {
    "version_number": 42,
    "timestamp": "2026-09-24T06:30:15+00:00",
    "embedding_metadata": {
      "embedding_model_id": "all-minilm-l6-v2-1.0",
      "embedding_dim": 384
    },
    "chunking_metadata": {
      "chunking_profile_id": "default_256_overlap_32"
    },
    "index_schema": {
      "schema_version": "2.0"
    },
    "document_count": 6000000,
    "index_size_bytes": 4294967296,
    "last_indexed_at": "2026-09-24T06:30:15+00:00"
  },
  
  "version_history": [
    {
      "version_number": 41,
      "timestamp": "2026-09-22T03:15:10+00:00",
      "embedding_model_id": "all-minilm-l6-v2-1.0",
      "chunking_profile_id": "default_256_overlap_32",
      "schema_version": "2.0",
      "action": "incremental_update",
      "documents_added": 5000,
      "documents_removed": 0
    },
    {
      "version_number": 40,
      "timestamp": "2026-09-20T02:00:00+00:00",
      "embedding_model_id": "all-minilm-l6-v2-0.9",
      "chunking_profile_id": "default_256_overlap_32",
      "schema_version": "2.0",
      "action": "reindex_complete",
      "reason": "embedding_model_upgrade"
    }
  ],
  
  "reindex_tracking": {
    "last_reindex_reason": "embedding_model_upgrade",
    "last_reindex_start": "2026-09-19T18:00:00+00:00",
    "last_reindex_end": "2026-09-20T02:00:00+00:00",
    "last_reindex_status": "success",
    "atomic_rollback_available": true,
    "previous_version_accessible": true
  },
  
  "governance": {
    "contract_version": "EMBEDDING_VERSION_GOVERNANCE.md (2026-09-24)",
    "audit_trail_enabled": true,
    "canary_deployments_enabled": true
  }
}
```

---

## RocksDB Persistence

### Storage Schema

```cpp
// Column family: "default"
Key: "index_version"
Value: INDEX_MANIFEST.json (entire manifest as JSON blob)

// Column family: "embeddings"
Key: sha256(doc_id + content)  // 32-byte binary key
Value: {
  "model_id": "all-minilm-l6-v2-1.0",
  "dim": 384,
  "vector": [float32 array, 384 elements],
  "timestamp": "2026-09-24T06:30:15+00:00",
  "cached": true
}

// Column family: "version_history"
Key: version_number (32-bit int, big-endian)
Value: VersionHistoryEntry JSON
```

---

## Canary Deployment Pattern

### Phase 1: Setup (Day 0)

```
1. Configure new embedding model:
   - Set embedding_model_id = "all-minilm-l6-v2-1.0"
   - Set canary_percentage = 0.05 (5% of queries)
   
2. Enable dual-read mode:
   - Read from both old index and new index
   - Compute Recall@10 delta for each query
   - Log all comparisons
```

### Phase 2: Canary Validation (Day 1–3)

```
Metrics monitored:
- Recall@10 delta: target < 0.02 (2pp regression tolerance)
- nDCG@10 delta: target < 0.05
- p95 latency delta: target < 50ms
- Cost per query delta: target < $0.005

Decision logic:
- If all metrics pass canary criteria for 1 day → proceed to Phase 3
- If any metric fails → hold in canary; investigate root cause
- After 3 days in canary with <2pp degradation → automatic progression
```

### Phase 3: Gradual Rollout (Day 4–7)

```
Day 4: Increase canary_percentage to 10%
Day 5: Increase to 25%
Day 6: Increase to 50%
Day 7: Increase to 100% (full cutover)

At each stage, monitor same metrics; auto-halt if regression > 2pp detected
```

### Phase 4: Rollback (Always Available)

```
If at any point regression > 2pp threshold:

1. Freeze new index writes
2. Keep old index serving 100% of traffic
3. Log rollback reason and timestamp
4. Alert RAG module owner
5. Old index remains accessible for 30 days (configurable)
6. Post-mortem analysis required before next attempt
```

---

## Dual-Read Implementation

### Query-Time Decision

```cpp
// In src/llm/wiki_index_store.cpp

struct DualReadResult {
  std::vector<RetrievedDoc> old_results;
  std::vector<RetrievedDoc> new_results;
  double recall_at_10_delta;
  double ndcg_at_10_delta;
  enum ComparisonStatus { PASS, REGRESSED, INCONCLUSIVE };
  ComparisonStatus status;
};

DualReadResult queryWithDualRead(const QueryRequest& query) {
  auto old_results = query_old_index(query);
  auto new_results = query_new_index(query);
  
  // Return new results to user (production serving)
  serve(new_results);
  
  // Asynchronously compute comparison metrics
  scheduleComparison({
    .old_results = old_results,
    .new_results = new_results,
    .query_id = query.id,
    .timestamp = now(),
    .user_id = query.user_id
  });
  
  return {old_results, new_results, recall_delta, ndcg_delta, status};
}
```

### Metrics Aggregation

```json
// Daily aggregation output: benchmarks/rag/canary_metrics_2026-09-25.json
{
  "date": "2026-09-25",
  "canary_percentage": 0.05,
  "queries_evaluated": 50000,
  "recall_at_10": {
    "old_mean": 0.75,
    "new_mean": 0.74,
    "delta": -0.01,
    "std_dev": 0.05,
    "p95": -0.03,
    "status": "PASS"
  },
  "ndcg_at_10": {
    "old_mean": 0.65,
    "new_mean": 0.66,
    "delta": 0.01,
    "std_dev": 0.08,
    "p95": 0.05,
    "status": "PASS"
  },
  "recommendation": "continue_canary"
}
```

---

## Index Version Rollback Utility

### Rollback Command

```bash
# Rollback to previous version
./scripts/rollback_index_version.sh \
  --index-id wiki-index-main \
  --target-version 40 \
  --dry-run false \
  --keep-new-version true

# Output:
# Rollback Plan:
# - Current version: 42 (all-minilm-l6-v2-1.0)
# - Target version: 40 (all-minilm-l6-v2-0.9)
# - New index will be archived as backup (accessible for 30 days)
# - Estimated rollback time: 120 seconds
#
# Proceeding with rollback...
# ✅ Index switched to version 40
# ✅ Version manifest updated
# ✅ Canary mode disabled
# ✅ Rollback complete at 2026-09-24T07:45:30Z
```

### Rollback Audit Log

```json
{
  "rollback_timestamp": "2026-09-24T07:45:30+00:00",
  "index_id": "wiki-index-main",
  "from_version": 42,
  "to_version": 40,
  "reason": "embedding_model_regression",
  "regression_details": {
    "recall_at_10_delta": -0.025,
    "threshold_exceeded": true,
    "threshold_value": -0.02
  },
  "rollback_duration_seconds": 125,
  "status": "success",
  "approver": "rag-module-owner-1",
  "archived_version_42_accessible_until": "2026-10-24T07:45:30+00:00"
}
```

---

## Configuration & Controls

### Environment Variables

```bash
# Enable/disable versioning tracking
THEMIS_EMBEDDING_VERSION_TRACKING=true

# Enable/disable canary deployments
THEMIS_CANARY_DEPLOYMENTS_ENABLED=true

# Canary percentage (0.0 to 1.0)
THEMIS_CANARY_PERCENTAGE=0.05

# Dual-read mode (true = always read both; false = only during canary)
THEMIS_DUAL_READ_ALWAYS_ON=false

# Recall regression tolerance (percentage points)
THEMIS_RECALL_REGRESSION_THRESHOLD=0.02

# Index manifest location
THEMIS_INDEX_MANIFEST_PATH=/var/lib/themisdb/index_manifests

# Rollback archival retention (days)
THEMIS_ROLLBACK_RETENTION_DAYS=30
```

### CMake Feature Gate

```cmake
# In cmake/features/RAGFeatures.cmake

option(THEMIS_EMBEDDING_VERSION_GOVERNANCE 
       "Enable embedding model versioning and reindex decision engine" ON)

if(THEMIS_EMBEDDING_VERSION_GOVERNANCE)
  target_compile_definitions(themis_rag_core 
    PRIVATE ENABLE_EMBEDDING_VERSION_GOVERNANCE)
endif()
```

---

## Testing & Validation

### Unit Tests

```cpp
// tests/ingestion/test_reindex_decision.cpp

TEST(ReindexDecisionEngine, EmbeddingModelChange_RequiresReindex) {
  EmbeddingMetadata old_meta = {"all-minilm-l6-v2-0.9", 384};
  EmbeddingMetadata new_meta = {"all-minilm-l6-v2-1.0", 384};
  
  ReindexDecision decision = decideReindex(old_meta, new_meta, ...);
  
  EXPECT_TRUE(decision.reindex_required);
  EXPECT_EQ(decision.reason, "embedding_model_change");
}

TEST(IndexManifest, PersistAndLoadVersion) {
  IndexManifest manifest;
  manifest.current_version.embedding_model_id = "all-minilm-l6-v2-1.0";
  manifest.current_version.document_count = 6000000;
  
  manifest.save("index_manifest.json");
  IndexManifest loaded = IndexManifest::load("index_manifest.json");
  
  EXPECT_EQ(loaded.current_version.embedding_model_id, manifest.current_version.embedding_model_id);
}

// tests/llm/test_embedding_canary_deployment.cpp

TEST(CanaryDeployment, DualReadComparison_WithinThreshold) {
  WikiIndexStore store;
  store.enableCanaryDeployment(0.05);  // 5% canary
  
  QueryRequest query = {"test query"};
  DualReadResult result = store.queryWithDualRead(query);
  
  EXPECT_LE(result.recall_at_10_delta, 0.02);  // < 2pp regression
  EXPECT_EQ(result.status, ComparisonStatus::PASS);
}

TEST(CanaryDeployment, RollbackOnRegression) {
  WikiIndexStore store;
  store.enableCanaryDeployment(0.05);
  
  // Simulate high regression
  simulateRecallRegression(0.05);  // 5pp regression
  
  EXPECT_TRUE(store.shouldRollback());
  store.rollbackToPreviousVersion();
  
  EXPECT_EQ(store.getCurrentEmbeddingModelId(), "all-minilm-l6-v2-0.9");
}
```

### Integration Tests

```cpp
// tests/llm/test_embedding_canary_e2e.cpp

TEST(CanaryDeploymentE2E, FullCycleWithGradualRollout) {
  WikiIndexStore store;
  store.enableCanaryDeployment(0.05);
  
  // Simulate 1 day in canary
  advanceTime(std::chrono::hours(24));
  store.evaluateCanaryMetrics();
  
  EXPECT_EQ(store.getCanaryPercentage(), 0.10);  // Auto-escalated
  
  // Simulate 7 days
  advanceTime(std::chrono::days(7));
  store.evaluateCanaryMetrics();
  
  EXPECT_EQ(store.getCanaryPercentage(), 1.0);  // Full cutover
  EXPECT_TRUE(store.isFullyRolledOut());
}
```

---

## Governance & Timeline

### Q4 2026 (Immediate)

- [ ] Implement reindex decision engine (`src/ingestion/reindex_decision_engine.cpp`)
- [ ] Implement index manifest persistence and loading
- [ ] Create INDEX_MANIFEST_V1_SCHEMA.json
- [ ] Add unit tests for decision logic
- [ ] Document in ROADMAP.md and FUTURE_ENHANCEMENTS.md

### Q1 2027 (Next Phase)

- [ ] Deploy canary mode with dual-read comparison
- [ ] Integrate with release_critical gates
- [ ] Create rollback automation and audit logging
- [ ] Validate against representative hardware
- [ ] Update operator runbooks with version management procedures

---

## References

- `audit/RAG_READINESS_AUDIT_2026-09-23.md` § 3 (Embedding/Index Version Governance)
- `src/rag/EVALUATION_CONTRACT_V1.md` (Baseline tracking and regression detection)
- `src/llm/wiki_index_store.cpp` (Embedding cache and index management)
- `benchmarks/rag/data/baselines_v1.json` (Embedding model specifications)
