# Process Module — Phase 3 Production Requirements & Edge-Case Guarantees

**Status:** Phase 3 Hardening — Error Handling & Edge Cases  
**Date:** 2026-08-06  
**Version:** 1.0.0  
**Audience:** Operators, Integrators, Process Model Designers  

---

## Executive Summary

This document defines the operational guarantees for the ThemisDB Process Module after Phase 3 implementation. It covers:

1. **High-churn edge cases** — concurrent model updates, rapid create/delete cycles
2. **Parser robustness** — malformed input, truncated files, invalid references
3. **Linker determinism** — cyclic dependency detection, missing target validation
4. **Retriever graceful degradation** — resource exhaustion, timeouts, cache misses
5. **Unified error signaling** — no silent failures; all paths return explicit error codes

---

## I. Resource Limits & Bounds

All process operations are bounded by the following constraints to prevent denial-of-service and resource exhaustion:

### I.1 Model Input Size

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| **kMaxModelInputBytes** | 10 MiB | Security guard against large malformed inputs; typical process models < 1 MiB |
| **kMaxModelElements** | 10,000 | Max nodes + edges per model; typical administrative processes < 500 |
| **kMaxModelNestingDepth** | 100 | Max nesting depth for sub-processes; typical depth < 10 |

**Guarantee:** Input exceeding these limits is rejected at validation time with a `RESOURCE_INCIDENT` diagnostic.

### I.2 Retrieval Context

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| **kMaxRetrievalContextBytes** | 1 MiB | Max LLM context per query; prevents unbounded prompt growth |
| **kMaxRetrievalDepth** | 50 | Max graph traversal depth for linked processes; prevents cycles |
| **kMaxOperationTimeoutMs** | 30,000 ms | Hard timeout for serialization, validation, retrieval operations |

**Guarantee:** Operations that exceed these bounds are terminated with a timeout error and explicit diagnostic.

---

## II. High-Churn Edge Cases

### II.1 Concurrent Model Creation/Deletion

**Scenario:** Multiple threads rapidly create, update, and delete process models.

**Implementation:**
- ProcessModelManager uses RocksDB's atomic put/delete operations
- Each save() operation increments a revision counter
- Concurrent updates to the same model ID serialize via RocksDB's lock-free design
- No transactional guarantees across multiple models (by design for distributed systems)

**Guarantees:**
- ✅ No race conditions within a single model (atomic per-revision)
- ✅ Audit trail preserved: old revisions remain under versioned keys
- ✅ Soft-delete policy: delete() marks state as ARCHIVED rather than removing
- ❌ No atomicity across multiple models (use caller-level coordination if needed)

**Error handling:**
- Concurrent update conflicts are detected via revision mismatch
- Error code: `kInvalidTransition` (ProcError::kInvalidTransition)
- Diagnostic: `CONCURRENCY_INCIDENT` with message describing the conflicting revision

**Example:**
```cpp
auto result = manager.save(model);
if (!result.ok) {
    auto diag = ProcessDiagnostics::createConcurrencyIncident(
        result.error_code,
        model.id,
        result.message
    );
    log_error(diag.toFormattedMessage());
}
```

### II.2 Rapid Create/Delete Cleanup

**Scenario:** A large batch of models is created and immediately deleted.

**Implementation:**
- When delete() is called, the model is marked as ARCHIVED (not physically removed)
- The raw_payload field is zeroed to free storage
- Cleanup of old revisions is performed by a background garbage collector
- No guarantee of immediate physical deletion

**Guarantees:**
- ✅ Model is immediately invisible to list(), search(), findSimilar() queries
- ✅ Old revisions preserved for audit purposes
- ✅ No corruption or incomplete state
- ⚠️  Physical storage reclamation is asynchronous (may take seconds/minutes)

---

## III. Parser Edge Hardening

### III.1 Malformed Input Detection

**BPMN, EPK, CMMN, DMN Serializers**

All parsers now perform pre-validation before attempting to parse:

| Check | Coverage | Error Code |
|-------|----------|-----------|
| **Empty input** | All formats | MALFORMED_INPUT_INCIDENT |
| **Size check** | All formats | RESOURCE_INCIDENT (> 10 MiB) |
| **UTF-8 validation** | All text formats | MALFORMED_INPUT_INCIDENT |
| **XML truncation detection** | BPMN, CMMN, DMN | MALFORMED_INPUT_INCIDENT |
| **Element count limit** | All formats | RESOURCE_INCIDENT (> 10,000 elements) |
| **Nesting depth limit** | BPMN (sub-processes), CMMN (hierarchies) | RESOURCE_INCIDENT (> 100 depth) |

**Implementation:** `SerializerInputValidator` class in `serializer_hardening.h`

```cpp
auto validation = SerializerInputValidator::validateInput(bpmn_xml, "BPMN 2.0");
if (!validation.ok) {
    auto diag = ProcessDiagnostics::createMalformedInputIncident(
        kDeserialiserFailed,
        "model_v1.bpmn",
        validation.error_message
    );
    return failure_result;
}
```

### III.2 Truncated File Detection

**XML-based formats (BPMN, CMMN, DMN):**
- Count opening vs. closing XML tags
- If counts mismatch, input is truncated (report MALFORMED_INPUT_INCIDENT)
- Also check for missing root closing tag

**Example error message:**
```
Input appears truncated (unmatched XML tags)
Found 47 opening tags but only 45 closing tags.
Check that the file was not cut off during transfer.
```

### III.3 Invalid Reference Detection

**BPMN:**
- Sequence flow references non-existent source/target task ID
- Start/end events reference undefined process ID

**EPK:**
- Edge connects undefined source or target node
- Function/event ID not resolvable

**CMMN:**
- Case plan item entry/exit sentry references non-existent item
- Required input/output parameters reference missing case data

**DMN:**
- Input/output column references undefined external variable

**Error code:** `MISSING_TARGET_INCIDENT`  
**Guarantees:** Invalid references are detected before model is persisted; no partial state.

---

## IV. Linker Determinism & Cycle Detection

### IV.1 Cyclic Dependency Detection

**Scenario:** Create a link A → B, but B already links back to A (directly or indirectly).

**Implementation:**
- `ProcessLinker::wouldCreateCycle(source_id, target_id)` uses DFS to detect backward edges
- Bounded by `kMaxRetrievalDepth` (50) to prevent infinite traversal
- Returns `true` if cycle would be created, `false` if safe to link

**Guarantees:**
- ✅ Self-loops (A → A) are detected and rejected
- ✅ Direct cycles (A → B → A) are detected
- ✅ Transitive cycles (A → B → C → A) are detected
- ✅ Detection completes within timeout
- ⚠️  Very deep cycles (> 50 hops) may not be detected (conservative)

**Error code:** `kInvalidTransition`  
**Incident type:** `CYCLE_INCIDENT`

**Example:**
```cpp
auto [ok, link_id] = linker.linkProcesses(
    "instance_a",
    "instance_b",
    ProcessLinkType::SUB_PROCESS
);
if (!ok) {
    // Check if it's a cycle
    if (linker.wouldCreateCycle("instance_a", "instance_b")) {
        auto diag = ProcessDiagnostics::createCycleIncident(
            kInvalidTransition,
            link_id,
            "Creating this link would introduce a cycle: instance_a ← instance_b"
        );
    }
}
```

### IV.2 Link Target Validation

**Scenario:** Create a link pointing to a non-existent process instance.

**Implementation:**
- `ProcessLinker::isLinkTargetValid(target_id)` checks if target exists in DB
- Returns `false` for empty IDs or non-existent targets
- Prevents dangling references

**Guarantees:**
- ✅ All links point to resolvable entities
- ✅ Dangling references are rejected before persistence
- ✅ Error code: `MISSING_TARGET_INCIDENT`

### IV.3 Concurrent Link Updates

**Scenario:** Two threads simultaneously create links involving the same entities.

**Implementation:**
- ProcessLinker uses a shared mutex (`link_state_lock_`) for link consistency
- All link operations serialize through this lock
- Operation counter ensures deterministic ordering

**Guarantees:**
- ✅ No lost updates (link state is always consistent)
- ✅ No race conditions in cycle detection
- ⚠️  Threads may block waiting for the lock (bounded by `kMaxOperationTimeoutMs`)

---

## V. Retriever Graceful Degradation

### V.1 Cache Miss Handling

**ProcessLightRetriever:**

**Scenario:** Query the GLOBAL mode (community-based), but no community reports are cached.

**Implementation:**
- `ProcessLightRetriever::retrieve()` checks if communities are persisted
- If not, automatically falls back to LOCAL mode (entity-centric traversal)
- Logs a WARN diagnostic but does NOT fail silently

**Guarantees:**
- ✅ Always returns *some* context (either GLOBAL or LOCAL)
- ✅ No silent failures; all degradation is logged
- ✅ Caller can inspect `used_mode` in result to know which strategy was used

**Example:**
```cpp
auto result = light_retriever.retrieve(
    "describe the entire process flow",
    "instance_123",
    RetrievalMode::AUTO
);
if (result.used_mode != RetrievalMode::GLOBAL) {
    SPDLOG_WARN("Fell back from GLOBAL to {}", 
                result.used_mode == RetrievalMode::LOCAL ? "LOCAL" : "AUTO");
}
```

### V.2 Buffer Overflow Protection

**ProcessGraphRag, ProcessAgenticRag:**

**Scenario:** Traversing a highly connected graph could produce context > 1 MiB.

**Implementation:**
- Track accumulated context size during graph traversal
- Stop adding nodes when context size would exceed `kMaxRetrievalContextBytes`
- Return partial context with truncation indicator

**Guarantees:**
- ✅ Context size never exceeds 1 MiB
- ✅ No out-of-memory errors
- ✅ Caller receives what was available + truncation flag

### V.3 Timeout Enforcement

**All retrieval operations:**

**Scenario:** Graph traversal takes longer than `kMaxOperationTimeoutMs` (30 seconds).

**Implementation:**
- `ParserStateTracker::hasTimedOut()` checks elapsed time
- Long-running operations call `hasTimedOut()` in their main loops
- Return partial result + timeout error code on deadline

**Guarantees:**
- ✅ No indefinite hangs
- ✅ Timeout is enforced consistently across all retrieval modes
- ✅ Partial results are returned (not empty)

**Error code:** `kExecutionTimeout` (ProcError::kExecutionTimeout)

---

## VI. Unified Error Signaling

### VI.1 Error Codes

All process module errors map to one of the `ProcError` enum values in `process_api_contract.h`:

| Error Code | Used For | Incident Type |
|-----------|----------|---------------|
| `kDeserialiserFailed` | Parser errors, malformed input | MALFORMED_INPUT_INCIDENT |
| `kSerialiserFailed` | Export errors, encoding issues | VALIDATION_INCIDENT |
| `kInvalidTransition` | Concurrency conflicts, cycle detected | CONCURRENCY_INCIDENT, CYCLE_INCIDENT |
| `kExecutionTimeout` | Operation exceeded time limit | RESOURCE_INCIDENT |
| (others) | TBD by implementation | Varies |

### VI.2 Diagnostic Records

Every error path produces a `DiagnosticRecord` via the `ProcessDiagnostics` factory:

```cpp
enum class DiagnosticIncidentType {
    IMPORT_INCIDENT              = 3600,  // Model import failed
    VALIDATION_INCIDENT          = 3601,  // Constraint check failed
    RETRIEVAL_INCIDENT           = 3602,  // Lookup/traversal failed
    LINKING_INCIDENT             = 3603,  // Link operation failed
    RESOURCE_INCIDENT            = 3604,  // Resource limit exceeded
    CONCURRENCY_INCIDENT         = 3605,  // Update conflict
    CYCLE_INCIDENT               = 3606,  // Cyclic dependency detected
    MALFORMED_INPUT_INCIDENT     = 3607,  // Truncated/invalid input
    MISSING_TARGET_INCIDENT      = 3608,  // Dangling reference
};
```

**Factory pattern:**
```cpp
auto diag = ProcessDiagnostics::createMalformedInputIncident(
    kDeserialiserFailed,
    "model.bpmn",
    "UTF-8 validation failed at byte offset 512"
);
log_error(diag.toFormattedMessage());
```

### VI.3 Guarantees

- ✅ No operation returns without an error code (implicit or explicit)
- ✅ All error paths produce a DiagnosticRecord with actionable context
- ✅ All diagnostics include timestamp (UTC), operation name, and recovery hints
- ✅ Silent failures are eliminated (all failures are logged)

---

## VII. Operational Best Practices

### VII.1 Model Import Workflow

```cpp
// 1. Validate input
auto validation = SerializerInputValidator::validateInput(bpmn_xml);
if (!validation.ok) {
    log_error("Validation failed: " + validation.error_message);
    return;
}

// 2. Import with diagnostics
auto import_result = manager.importBpmn(bpmn_xml, metadata);
if (!import_result.ok) {
    auto diag = ProcessDiagnostics::createImportIncident(
        kDeserialiserFailed,
        "model.bpmn",
        import_result.message
    );
    log_error(diag.toFormattedMessage());
    return;
}

// 3. Persist
auto save_result = manager.save(import_result);
if (!save_result.ok) {
    log_error("Save failed: " + save_result.message);
    return;
}
```

### VII.2 Link Creation Workflow

```cpp
// 1. Validate target exists
if (!linker.isLinkTargetValid(target_id)) {
    auto diag = ProcessDiagnostics::createMissingTargetIncident(
        kInvalidTransition,
        target_id,
        "Target process instance not found"
    );
    log_error(diag.toFormattedMessage());
    return;
}

// 2. Check for cycles
if (linker.wouldCreateCycle(source_id, target_id)) {
    auto diag = ProcessDiagnostics::createCycleIncident(
        kInvalidTransition,
        "link:" + source_id + "→" + target_id,
        "Creating this link would introduce a cycle"
    );
    log_error(diag.toFormattedMessage());
    return;
}

// 3. Create link
auto [ok, link_id] = linker.linkProcesses(
    source_id, target_id, ProcessLinkType::SUB_PROCESS
);
if (!ok) {
    log_error("Link creation failed: " + link_id);
    return;
}
```

### VII.3 Retrieval Workflow

```cpp
// 1. Attempt retrieval with AUTO mode
auto result = light_retriever.retrieve(
    query,
    instance_id,
    RetrievalMode::AUTO
);

// 2. Check if degradation occurred
if (result.used_mode != RetrievalMode::GLOBAL) {
    log_info("Using " + std::string(result.used_mode == RetrievalMode::LOCAL ? "LOCAL" : "AUTO") + 
             " mode (communities unavailable)");
}

// 3. Check context size
if (result.llm_context.size() >= kMaxRetrievalContextBytes) {
    log_warn("Retrieved context truncated (at size limit)");
}

// 4. Use context in LLM prompt
llm_input.context = result.llm_context;
```

---

## VIII. Monitoring & Observability

### VIII.1 Recommended Metrics

- `process_import_total` (counter) — Total imports by format (BPMN/EPK/CMMN/DMN)
- `process_import_errors_total` (counter) — Total import failures by incident type
- `process_import_duration_ms` (histogram) — Import operation latency
- `process_link_creation_total` (counter) — Total link operations
- `process_cycles_detected_total` (counter) — Total cyclic dependencies detected
- `process_retrieval_duration_ms` (histogram) — Retrieval latency by mode
- `process_retrieval_mode_used` (gauge) — Current retrieval mode distribution

### VIII.2 Recommended Alerts

- Alert if `process_import_errors_total` rate > 10/min (malicious input attack)
- Alert if `process_import_duration_ms` > 10,000 ms (possible timeout)
- Alert if `process_cycles_detected_total` rate > 5/min (data model issues)
- Alert if `process_retrieval_duration_ms` > 5,000 ms (performance degradation)

---

## IX. Migration & Upgrade Path

### IX.1 From Phase 2 to Phase 3

Existing code continues to work unchanged. New features:

- Import validation is **automatic** (no code change needed)
- Cycle detection is **opt-in** — call `wouldCreateCycle()` before linking
- Graceful degradation is **automatic** for retrievers
- Diagnostics are **available** via `ProcessDiagnostics` factory

### IX.2 Recommended Upgrade Steps

1. Update dependency on `serializer_hardening.h` (new file)
2. Update error handlers to use new `CONCURRENCY_INCIDENT`, `CYCLE_INCIDENT`, etc.
3. Add cycle detection to link creation workflows
4. Ensure all error paths log diagnostics
5. Update monitoring dashboards with new metrics

---

## X. FAQ

**Q: What if I have a valid cycle in my process (loop condition)?**  
A: Use a different link type (e.g., `TRIGGERS` instead of `SUB_PROCESS`) to indicate the cycle is intentional. Cycle detection only flags certain link types.

**Q: Why no transactional guarantees across multiple models?**  
A: ThemisDB is designed for distributed systems where strong ACID guarantees are not feasible. Use caller-level coordination (e.g., Sagas) if you need cross-model atomicity.

**Q: What happens if a model exceeds 10 MiB?**  
A: Import is rejected with `RESOURCE_INCIDENT` diagnostic. Split the model into multiple smaller definitions or increase the limit if your use case justifies it (requires code change).

**Q: How long do deleted models stay in the audit trail?**  
A: This is determined by the backup/retention policy in your RocksDB configuration. Default is indefinite (until manual cleanup).

---

## XI. Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2026-08-06 | Initial Phase 3 release — error handling & edge-case hardening |

---

**Document Owner:** Process Module Team  
**Last Updated:** 2026-08-06  
**Status:** APPROVED FOR PRODUCTION
