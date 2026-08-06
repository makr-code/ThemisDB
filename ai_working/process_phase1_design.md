# Process Module Phase 1: Design & API Contract

**Date:** 2026-08-06  
**Status:** In Progress  
**Target:** Q4 2026

---

## Executive Summary

Phase 1 defines explicit API contracts for concurrent process model operations under high-churn scenarios. We formalize three core contracts:
1. **Concurrency Contract** – Thread-safety and atomicity guarantees for CRUD and linking operations
2. **Determinism Specification** – Guaranteed ordering and consistency under concurrent updates
3. **Extended Diagnostics Framework** – Operator-facing incident classification with trace context

---

## 1. Concurrency Contract

### Design Objective
Specify thread-safety for core process operations (Create, Retrieve, Update, Delete, Import, Link, Export) under concurrent access and high model churn.

### Key Guarantees

#### 1.1 Serializer Layer (BpmnSerializer, CmmnSerializer)
- **Status:** Stateless, fully thread-safe
- **Pattern:** Read-only snapshot; no shared mutable state
- **API:** Multiple concurrent threads may call serialize/deserialize concurrently without coordination
- **Exception:** None; serializers are inherently safe

#### 1.2 Model Manager Layer (ProcessModelManager)
- **Status:** Guarded mutable state
- **Concurrency Model:** Snapshot isolation via versioning
- **Thread-Safety Guarantees:**
  - `create()`, `update()`, `delete()` are atomic (single version bump)
  - `retrieve()` returns a consistent snapshot for a given version
  - Concurrent reads (snapshots) do not block writes
  - No write-write conflicts; first writer wins or conflict is returned
- **Conflict Resolution:** Last-write-wins (LWW) with version clock ordering
- **Atomicity Scope:** Single model operation only (not multi-model transactions)

#### 1.3 Linker Layer (ProcessLinker)
- **Status:** Guarded mutable state
- **Concurrency Model:** Fine-grained link-level locking
- **Thread-Safety Guarantees:**
  - Link creation/deletion is atomic
  - Concurrent link operations on disjoint instances do not block
  - Link-to-self updates use compare-and-swap (CAS) semantics
  - Broken links (stale references) are detected at read time
- **Atomicity Scope:** Single link operation only
- **Rollback Semantics:** Links have no transactional rollback; best-effort cleanup on error

#### 1.4 Retriever Layer (ProcessLightRetriever)
- **Status:** Read-only; thread-safe
- **Concurrency Model:** No locks required; snapshot-based reads
- **Thread-Safety Guarantees:**
  - `retrieve()` is fully thread-safe
  - Queries operate over a consistent graph snapshot
  - AUTO routing classifies queries without state mutation
- **Exception:** Underlying RocksDB snapshot must exist; retriever does not create snapshots

### High-Churn Scenario Behavior

**Definition:** High churn = >100 model updates/sec or concurrent linking of >1000 links/sec.

**Expected Behavior:**
- **Serialization latency:** 5-50 ms per model (independent of churn rate)
- **Link creation latency:** 1-10 ms per link (scales with contention)
- **Conflict probability:** 5-15% under >500 concurrent operations (LWW resolves)
- **No silent failures:** All conflicts/contentions reported via error codes

**Guaranteed Ordering:**
- Version clock monotonically increases (no time travel)
- Link creation order is preserved within a single instance
- Retrieval snapshots are isolated from concurrent updates

---

## 2. Determinism Specification

### Design Objective
Guarantee reproducible process model behavior under concurrent updates and model conflicts.

### Determinism Guarantees

#### 2.1 Model Import and Parsing
- **Input:** Identical BPMN/CMMN source files
- **Guarantee:** Output model is identical (same nodes, links, metadata)
- **Condition:** Parsing must be deterministic (no random ordering, UUID generation follows RFC 4122 v5 with stable namespace)

#### 2.2 State Transitions
- **Guarantee:** Process instance state transitions follow BPMN 2.0 token semantics
- **Ordering:** Deterministic within a single instance; no race conditions on state fields
- **Exception:** Subprocess invocation order may vary under concurrent execution (documented as non-deterministic)

#### 2.3 Conflict Resolution under Churn
- **Scenario:** Two threads update the same model concurrently
- **Resolution:** Last-write-wins (LWW) using version clock
- **Guarantee:** Final state is consistent with one of the write operations (no partial/merged state)
- **Ordering:** Version numbers form a total order; no tie-breaking required

#### 2.4 Link Consistency under Deletion
- **Scenario:** Model is deleted while links reference it
- **Guarantee:** Links become stale but are not corrupted
- **Detection:** Read-time validation reports broken links via RETRIEVAL_INCIDENT diagnostic
- **No cascading deletes:** Manual cleanup required (prevents accidental data loss)

#### 2.5 Retrieval Consistency
- **Guarantee:** Queries return consistent snapshots from a single instant in time
- **Ordering:** Graph traversal order is deterministic for a given snapshot
- **Community detection:** Deterministic algorithm; same input yields same communities
- **Exception:** LLM context generation may vary per model (non-deterministic)

### Rollback Semantics

**No automatic rollback:** Process module does not support nested transactions or automatic rollback.

**Manual remediation steps:**
1. Link creation fails → Manual deletion of orphaned links
2. Model update conflicts → Retry with latest version
3. Retrieval inconsistency → Reindex and re-run query

---

## 3. Extended Diagnostics Framework

### Design Objective
Provide operators with actionable incident classification and trace context for efficient triage.

### Incident Classification Extension

New incident types and context layers:

#### 3.1 Incident Context Enum
```cpp
enum class IncidentContext {
    CHURN_DETECTION,        // High model update rate detected
    RESOURCE_EXHAUSTION,    // Memory, file handles, or timeout limit hit
    CONFLICT_DETECTED,      // Write-write conflict or link staleness
    VALIDATION_FAILURE,     // Constraint or schema violation
    TIMEOUT_EXCEEDED,       // Operation deadline passed
};
```

#### 3.2 Trace Context
Each diagnostic record carries optional trace metadata:
- `trace_id`: Correlation ID for multi-step operations (import→validate→link)
- `span_id`: Local operation identifier within a trace
- `churn_metric`: Concurrent operation count at time of incident
- `conflict_count`: Number of detected conflicts
- `retry_count`: Retry attempts before failure

#### 3.3 Extended DiagnosticRecord
```cpp
struct DiagnosticRecord {
    // ... existing fields ...
    std::optional<std::string> trace_id;      // For distributed tracing
    std::optional<std::string> span_id;
    std::optional<int32_t> churn_metric;      // Concurrent ops at time of incident
    std::optional<int32_t> conflict_count;    // Conflicts detected
    std::optional<int32_t> retry_count;       // Retry attempts before failure
};
```

#### 3.4 Extended Factory Methods
```cpp
static DiagnosticRecord createChurnIncident(
    ProcError error,
    std::string_view input_id,
    std::string_view message,
    int32_t concurrent_ops,
    std::optional<std::string_view> trace_id = std::nullopt
);
```

---

## 4. Stress Scenario Definition

### Parser Edge Scenarios

1. **Deep Nesting Stress**
   - Nested sub-processes up to max depth (100 levels)
   - Expected: Success with deterministic output
   - Trigger: `PROC_MAX_DEPTH_EXCEEDED` if exceeded

2. **Large Element Count**
   - Process model with >10,000 nodes
   - Expected: Deserialize in <500 ms
   - Trigger: `PROC_MAX_ELEMENTS_EXCEEDED` if model too large

3. **Malformed XML Recovery**
   - BPMN with missing required attributes, broken nesting
   - Expected: Explicit error with actionable message
   - Trigger: `PROC_DESERIALISE_FAILED` with remediation hint

4. **Unsupported Gateway Types**
   - COMPLEX_AND, COMPLEX_OR not in BPMN 2.0
   - Expected: Explicit error before import
   - Trigger: `PROC_UNSUPPORTED_ELEMENT`

### Linker Edge Scenarios

1. **Orphaned Link Resolution**
   - Link references deleted model/instance
   - Expected: Link persists but marked as stale
   - Trigger: `RETRIEVAL_INCIDENT` on read

2. **Circular Reference Detection**
   - Process A triggers B, B triggers A
   - Expected: Allowed; documented as non-deterministic
   - Trigger: Warning log; no error

3. **Bulk Link Creation**
   - 10,000+ links added concurrently
   - Expected: 1-10 ms per link, no deadlocks
   - Trigger: `PROC_MAX_CONTEXT_EXCEEDED` if context size exceeds limit

4. **Link Attribute Mutation under Churn**
   - Concurrent updates to link metadata
   - Expected: Last-write-wins resolution
   - Trigger: LWW version clock governs order

### Retrieval Edge Scenarios

1. **Empty Graph Queries**
   - Retrieve from model with no instances
   - Expected: Return empty context gracefully
   - Trigger: No error; empty result set

2. **Large Context Size**
   - Graph traversal yields >1 MB context
   - Expected: Truncate context and log warning
   - Trigger: `PROC_MAX_CONTEXT_EXCEEDED` with truncation marker

3. **Community Detection Timeout**
   - Large graph (>100k edges) times out during community detection
   - Expected: Fallback to LOCAL retrieval
   - Trigger: `PROC_EXECUTION_TIMEOUT` with fallback logging

4. **Concurrent Query Churn**
   - 100+ concurrent retrieval queries
   - Expected: All queries complete within deadline
   - Trigger: Tail latency monitored; no silent failures

---

## 5. Acceptance Criteria

- [x] `process_concurrency_contract.h` created with explicit thread-safety documentation
- [x] `process_determinism_spec.h` created with conflict resolution and ordering guarantees
- [x] `process_diagnostics_api.h` created extending diagnostics with trace context
- [ ] `src/process/ROADMAP.md` updated with Phase 1 completion
- [ ] All headers include Doxygen documentation
- [ ] No behavioral changes to existing APIs (backward compatible)

---

## 6. Implementation Notes

### Threading Model Rationale
- **Serializers:** Stateless design enforces thread-safety without locks
- **Model Manager:** Snapshot isolation with version clocks avoids distributed consensus
- **Linker:** Fine-grained locking enables high throughput under link churn
- **Retriever:** Read-only abstraction layers eliminate concurrency issues

### Why No Global Transactions
- Process module operates on independent domain entities (models, links, instances)
- Cross-model transactions would introduce complexity without clear use case
- Current LWW approach is sufficient for administrative workflow scenarios

### Determinism Boundaries
- **Deterministic:** Model structure, state machines, link references
- **Non-deterministic:** Subprocess execution order, LLM context generation, community detection (though algorithm is deterministic per input)

---

## 7. Phase 2 Plan (Q4 2026)

1. **Stress Testing:** Implement hardened test suite exercising all edge scenarios
2. **Benchmark Gates:** Lock p95/p99 latency under high-churn conditions
3. **Incident Tracing:** Integrate OpenTelemetry support for production diagnostics
4. **Documentation:** Write operator playbooks for high-churn incident resolution

