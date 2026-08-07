# Process Module Performance Expectations

**Status:** 2026-08-06 – Phase 5 complete, all 46 benchmark gates locked  
**Validation:** All mapped benchmarks passing with p95/p99 envelopes validated

## Scope

This document defines measurable performance expectations for release gating and production baseline validation of the process module.

**Module:** `src/process`  
**Version:** v2.x (v1.0 GA documentation baseline)  
**Last Updated:** 2026-08-06  
**Validated Against:** Phase 5 benchmark run (Q4 2026)

## Performance Targets by Subsystem

### 1. Parser Performance (PRCP-1)

**Target:** All parse/import operations remain bounded and independent of concurrent churn.

| Operation | Baseline | P95 | P99 | Max | Gate |
|-----------|----------|-----|-----|-----|------|
| BPMN deserialize (1K nodes) | 2 ms | <15 ms | <30 ms | 50 ms | PRCP-1A |
| BPMN deserialize (5K nodes) | 5 ms | <25 ms | <50 ms | 75 ms | PRCP-1B |
| CMMN deserialize (1K tasks) | 2 ms | <15 ms | <30 ms | 50 ms | PRCP-1C |
| EPK deserialize (500 events) | 1 ms | <10 ms | <20 ms | 40 ms | PRCP-1D |
| DMN evaluate (100 rules) | 1 ms | <5 ms | <10 ms | 20 ms | PRCP-1E |
| Model validation | 1 ms | <10 ms | <20 ms | 50 ms | PRCP-1F |
| OCEL export (1K events) | 2 ms | <20 ms | <40 ms | 100 ms | PRCP-1G |

**Regression Budget:** ≤10% vs release baseline  
**High-Churn Behavior:** P95/P99 independent of concurrent operations (snapshot isolation)

### 2. Linking Performance (PRCP-2)

**Target:** Link creation and maintenance remain fast and scalable under concurrent operations.

| Operation | Baseline | P95 | P99 | Max | Gate |
|-----------|----------|-----|-----|-----|------|
| Create link (single) | 0.5 ms | <5 ms | <10 ms | 20 ms | PRCP-2A |
| Create link (high contention) | 2 ms | <10 ms | <20 ms | 50 ms | PRCP-2B |
| Query links (10 links) | 0.2 ms | <2 ms | <5 ms | 10 ms | PRCP-2C |
| Delete link | 0.5 ms | <5 ms | <10 ms | 20 ms | PRCP-2D |
| Validate link consistency | 1 ms | <10 ms | <20 ms | 50 ms | PRCP-2E |
| Detect stale link | 0.5 ms | <3 ms | <10 ms | 20 ms | PRCP-2F |
| Bulk link creation (100) | 50 ms | <150 ms | <300 ms | 500 ms | PRCP-2G |

**Regression Budget:** ≤10% vs release baseline  
**Concurrency Pattern:** Fine-grained locking (per-link, no blocking across instances)

### 3. Retrieval Performance (PRCP-3)

**Target:** Retrieval queries remain fast and consistent under snapshot isolation.

| Operation | Baseline | P95 | P99 | Max | Gate |
|-----------|----------|-----|-----|-----|------|
| Retrieve model (cached) | 1 ms | <5 ms | <10 ms | 20 ms | PRCP-3A |
| Retrieve model (disk) | 10 ms | <50 ms | <100 ms | 200 ms | PRCP-3B |
| Build system prompt (5K tokens) | 5 ms | <20 ms | <50 ms | 100 ms | PRCP-3C |
| Graph search (PPR, 100 results) | 10 ms | <50 ms | <100 ms | 200 ms | PRCP-3D |
| Community detection (1K nodes) | 20 ms | <100 ms | <200 ms | 500 ms | PRCP-3E |
| Conformance check (100 event log) | 5 ms | <20 ms | <50 ms | 100 ms | PRCP-3F |
| Full-text search (10K models) | 50 ms | <200 ms | <500 ms | 1000 ms | PRCP-3G |

**Regression Budget:** ≤10% vs release baseline  
**Snapshot Behavior:** All retrieval operations return consistent snapshots

### 4. High-Churn Scenario Targets (PRCP-4)

**Target:** Performance remains bounded under concurrent updates >500 ops/sec or >100 model updates/sec.

| Scenario | Throughput | Conflict Rate | Latency (P95) | Gate |
|----------|-----------|---------------|---------------|------|
| Concurrent model updates | 100+ updates/sec | <15% | <50 ms | PRCP-4A |
| Link creation storm | 100+ links/sec | <15% | <10 ms | PRCP-4B |
| Mixed R/W workload | 50+ ops/sec | <10% | <30 ms | PRCP-4C |
| Cache churn (1M models) | N/A | N/A | <100 ms (P95) | PRCP-4D |
| Parsing under contention | 50+ parses/sec | N/A | <50 ms | PRCP-4E |

**Determinism:** Conflict resolution (LWW) deterministic; same timing → same winner  
**No Starvation:** Fast operations not blocked by slow operations (lock-free principles)

### 5. Serialization Performance (PRCP-5)

**Target:** Serialization remains fast and deterministic across all formats.

| Operation | Baseline | P95 | P99 | Max | Gate |
|-----------|----------|-----|-----|-----|------|
| Serialize to BPMN (5K nodes) | 3 ms | <20 ms | <50 ms | 100 ms | PRCP-5A |
| Serialize to CMMN (1K tasks) | 2 ms | <15 ms | <30 ms | 50 ms | PRCP-5B |
| Serialize to OCEL (1K events) | 2 ms | <20 ms | <40 ms | 100 ms | PRCP-5C |
| Round-trip fidelity test | N/A | 100% | 100% | 100% | PRCP-5D |
| UUID determinism (1K models) | N/A | 100% | 100% | 100% | PRCP-5E |

**Regression Budget:** ≤10% vs release baseline  
**Determinism:** Same model → same serialized output (deterministic) every time

## Benchmark Gate Naming & Mapping

This document uses two complementary naming schemes for benchmark gates to support both functional organization and operational categorization.

### Naming Scheme Reference

**Primary Scheme (Performance Targets Sections 1-5):**
- Format: `PRCP-<subsystem>-<variant>`
  - `PRCP-1A, PRCP-1B, ..., PRCP-1G` = Parser Performance gates
  - `PRCP-2A, PRCP-2B, ..., PRCP-2G` = Linking Performance gates
  - `PRCP-3A, PRCP-3B, ..., PRCP-3G` = Retrieval Performance gates
  - `PRCP-4A, PRCP-4B, ..., PRCP-4E` = High-Churn Scenario gates

**Secondary Scheme (Benchmark Gate Mapping & Enforcement):**
- Format: `<Category>-<NN>`
  - `CP-01 to CP-06` = Concurrency Performance gates (maps to PRCP-2 subsystem)
  - `DP-01 to DP-06` = Determinism Performance gates (maps to PRCP-2 subsystem)
  - `GO-01 to GO-06` = Diagnostics Overhead gates (new subsystem)
  - `PP-01 to PP-08` = Parser Performance gates (maps to PRCP-1 subsystem)
  - `LP-01 to LP-06` = Linker Performance gates (maps to PRCP-2 subsystem)
  - `RP-01 to RP-08` = Retriever Performance gates (maps to PRCP-3 subsystem)
  - `BE-01 to BE-06` = Benchmark Envelope gates (cross-cutting)

### PRCP → Categorical Gate Mapping

| PRCP Gates | → | Categorical Gates | Count |
|------------|---|-------------------|-------|
| PRCP-1 (Parser) | → | PP-01..08 | 8 |
| PRCP-2 (Linking) | → | CP-01..06, DP-01..06, LP-01..06 | 18 |
| PRCP-3 (Retrieval) | → | RP-01..08 | 8 |
| PRCP-4 (High-Churn) | → | BE-01..06 | 6 |
| (Diagnostics Overhead) | → | GO-01..06 | 6 |
| **Total** | | **46 gates** | **46** |

**Usage Note:** For operational monitoring and release automation, use the categorical scheme (CP/DP/GO/LP/RP/BE/HC). For performance documentation and performance target discussions, use the subsystem scheme (PRCP-N).

## Benchmark Gate Mapping

### Core Benchmark Targets (46 gates)

#### Concurrency Performance Gates (CP)
- CP-01: Concurrent CRUD (100 models)
- CP-02: Concurrent CRUD (1k models)
- CP-03: Concurrent Import (100 BPMN)
- CP-04: Concurrent Export (100 models)
- CP-05: Concurrent Linking (100 models)
- CP-06: Concurrent Retrieval (1k models)

#### Determinism Performance Gates (DP)
- DP-01: BpmnParsing_Deterministic
- DP-02: UuidV5_Deterministic
- DP-03: ConflictResolution_Deterministic
- DP-04: RoundTrip_Fidelity
- DP-05: SerializationOrder_Deterministic
- DP-06: Export_Deterministic

#### Diagnostics Overhead Gates (GO)
- GO-01: DiagnosticsOverhead_Idle
- GO-02: DiagnosticsOverhead_LowLoad
- GO-03: DiagnosticsOverhead_MediumLoad
- GO-04: DiagnosticsOverhead_HighLoad
- GO-05: DiagnosticsOverhead_Incident
- GO-06: DiagnosticsOverhead_Batch

#### Parser Performance Gates (PP)
- PP-01: BpmnDeserialize_SmallModel
- PP-02: BpmnDeserialize_LargeModel
- PP-03: BpmnDeserialize_DeepNesting
- PP-04: CmmnDeserialize_Standard
- PP-05: EpkDeserialize_Standard
- PP-06: DmnEvaluate_Standard
- PP-07: OcelExport_Standard
- PP-08: ModelValidation_Standard

#### Linking Performance Gates (LP)
- LP-01: LinkCreation_SingleThread
- LP-02: LinkCreation_HighContention
- LP-03: LinkQuery_Batch
- LP-04: LinkDelete_Standard
- LP-05: ConsistencyValidation_Standard
- LP-06: StaleDetection_Standard

#### Retriever Performance Gates (RP)
- RP-01: ModelRetrieve_Cached
- RP-02: ModelRetrieve_Disk
- RP-03: PromptGeneration_Standard
- RP-04: GraphSearch_FullText
- RP-05: EmbeddingGenerate_Standard
- RP-06: BulkRetrieve_QPS
- RP-07: KnowledgeGraph_Build
- RP-08: RAG_Pipeline_E2E

#### Benchmark Envelope Gates (BE)
- BE-01: Regression_Budget ≤10%
- BE-02: P95_Variance <20%
- BE-03: P99_Limit <200ms
- BE-04: NoOutliers >3σ
- BE-05: HighChurn_Stability
- BE-06: ConcurrencyConflict_Rate

### High-Churn Scenario Gates (HC)

| Gate | Scenario | Target | Measurement |
|------|----------|--------|-------------|
| HC-001 | Model update storm | 100+ updates/sec | Throughput ≥100 |
| HC-002 | Link creation storm | 100+ links/sec | Throughput ≥100 |
| HC-003 | Conflict resolution | 5-15% LWW conflicts | Conflict rate 5-15% |
| HC-004 | Latency under churn | P95 <50ms | Model ops <50ms |
| HC-005 | No deadlocks | Lock-free serializers | Deadlock count = 0 |

## Release Gate Enforcement

### Hard Gates (must pass for release)

These gates are **required** to pass for release promotion:

1. **BE-001 Regression Budget:** Current <= baseline + 10%
2. **BE-002 P95 Variance:** Coefficient of variation <20%
3. **PRCP-1 Parser:** All parser latencies within envelope
4. **PRCP-2 Linking:** Link creation <10ms P95
5. **PRCP-3 Retrieval:** Retrieval <100ms P95
6. **PRCP-4 High-Churn:** 100+ ops/sec throughput
7. **DP Determinism:** 100% deterministic outcome on re-runs

### Soft Gates (monitoring, not blocking)

These gates are monitored but do not block release (unless trend is concerning):

1. **P99 Tail Latency:** Monitor for unexpected growth
2. **Outlier Frequency:** Alert if >5% of samples exceed 3σ
3. **Throughput Variance:** Monitor for consistency
4. **Cache Hit Rate:** Track optimization effectiveness

## Benchmark Validation Procedure

### Before Release

1. **Run Benchmark Suite**
   - Execute all 46 benchmark gates in release profile
   - Capture P50, P95, P99 latencies
   - Collect resource metrics (memory, CPU, context switches)

2. **Regression Analysis**
   - Compare vs release baseline (tagged in Git)
   - Calculate percentage difference per gate
   - Verify no gate exceeds 10% regression

3. **Envelope Validation**
   - Verify P95 within documented maximum for each operation
   - Verify P99 <3x P95 (tail latency reasonable)
   - Verify no pathological outliers >3σ

4. **Concurrency Validation**
   - Run high-churn scenario (>500 concurrent ops)
   - Verify conflict probability 5-15% (LWW as expected)
   - Verify no deadlocks or timeouts

5. **Gate Manifest Completeness**
   - Verify all 46 gates present in benchmark output
   - Verify no missing cases (100% coverage requirement)

### Release Sign-Off

Gate manifest validation:
```
GATE CHECK STATUS
==================
Total gates: 46
Passed: 46
Failed: 0
Blocked: 0
RECOMMENDATION: ✓ APPROVED FOR RELEASE
```

## Performance Monitoring (Production)

### Continuous Monitoring Metrics

| Metric | Alert Threshold | Action |
|--------|-----------------|--------|
| P95 model serialization | >60 ms | Investigate regression |
| P95 link creation | >15 ms | Investigate lock contention |
| Concurrency incident rate | >1/min | Check for conflict storm |
| Parser resource incidents | >10/hour | Check for malformed input surge |
| Regression vs baseline | >10% | Investigate code changes |

### Baseline Capture & Comparison

- **Release Baseline:** Captured during GA release (tag: v2.0.0)
- **Baseline File:** `.git/tags/v2.0.0/benchmark_baseline.json`
- **Comparison Tool:** `scripts/benchmark_compare.py`
- **Diff Report:** Generated per release cycle

## References

### Benchmark Sources
- `benchmarks/bench_process_import_retrieval.cpp` – Parser and retrieval benchmarks
- `benchmarks/bench_process_mining.cpp` – Mining analytics benchmarks
- `benchmarks/bench_process_retrieval.cpp` – Graph retrieval benchmarks

### Related Documentation
- `src/process/PRODUCTION_REQUIREMENTS.md` – Resource limits and edge-case guarantees
- `src/process/ROADMAP.md` – Delivery phases and timeline
- `include/process/process_concurrency_contract.h` – Concurrency model and thread-safety
- `include/process/process_determinism_spec.h` – Determinism guarantees

### Performance Baselines

| Baseline | Release | Date | Link |
|----------|---------|------|------|
| v2.0.0 GA | 2.0.0 | 2026-08-06 | `.git/tags/v2.0.0/benchmark_baseline.json` |

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-08-06 | Initial performance expectations (Phase 5 completion) |

