# ThemisDB Process Module

<!-- Status: Production Ready (Phase 1-6 Complete, Wave A Reliability) | validated: 2026-08-14 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · PERFORMANCE_EXPECTATIONS.md · PRODUCTION_REQUIREMENTS.md · PHASE_6_ACCEPTANCE_CHECKLIST.md · MODULE_GAPS.md -->
<!-- Wave Context: Wave A (Runtime Reliability) — Recovery Determinism + Fail-Closed Verification -->

## Module Purpose

Production-capable process modeling runtime with hardened edge-case behavior, unified diagnostics framework, and bounded resource constraints. Provides process-model import/export, lifecycle operations, process-linking, and process-oriented retrieval/RAG support surfaces for ThemisDB. **Batch 5 enhancement focus: Recovery determinism, phase isolation, crash consistency**.

**Status:** Phase 1-6 (High-Churn Hardening Initiative) ✓ COMPLETE (2026-08-06) → Batch 5 Wave A alignment complete (2026-08-14)

## Module Scope and Verified Behaviors

### In Scope ✓
- Process model lifecycle with explicit concurrency contracts (snapshot isolation)
- Multi-format serialization/import/export (BPMN, CMMN, EPK, DMN, OCEL, VCC-VPB, FIM)
- Deterministic conflict resolution (Last-Write-Wins with version clocks)
- Unified diagnostics framework with 8 incident classes
- Process-to-object and process-to-process linking with stale-link detection
- Process-oriented retrieval, descriptors, and RAG support surfaces
- Bounded resource constraints (parser depth, element count, timeout)
- High-churn scenario guarantees (5-15% conflict probability under >500 concurrent ops)

### Out of Scope
- Core workflow execution engine ownership outside module boundaries
- External mining/dashboard tool ownership (integrations only)
- Non-process business-domain operational logic

## Core Concurrency & Determinism Guarantees

### Thread-Safety Model
- **Serializers:** Stateless and fully thread-safe (no locks required)
- **Model Manager:** Snapshot isolation with version clocks (readers do not block writers)
- **Linker:** Fine-grained per-link locking (independent entities do not block each other)
- **Retriever:** Read-only snapshots with snapshot isolation guarantees

### Determinism Classification
- **Fully Deterministic:** BPMN parsing (RFC 4122 v5), serialization, model retrieve
- **Conflict-Resolved:** Concurrent updates resolve deterministically via LWW (no ties)
- **Non-Deterministic:** Subprocess execution order (documented as non-deterministic)

### High-Churn Guarantees (>100 updates/sec or >500 concurrent ops)
- Conflict probability: 5-15% LWW conflicts expected
- No silent failures; all conflicts explicitly reported via CONCURRENCY_INCIDENT
- Performance envelope maintained independent of churn (no starving operations)
- Throughput: 100+ links/sec minimum under high churn
- Deadlock prevention: Consistent lock ordering ensures no deadlocks

**Reference:** `include/process/process_concurrency_contract.h`, `include/process/process_determinism_spec.h`

## Unified Diagnostics Framework

8 incident classes with actionable operator messages:
- **IMPORT_INCIDENT** – Import or deserialization failed
- **VALIDATION_INCIDENT** – Validation or constraint check failed
- **RETRIEVAL_INCIDENT** – Retrieval, linking, or context lookup failed
- **LINKING_INCIDENT** – Linking state transition or consistency check failed
- **RESOURCE_INCIDENT** – Parser resource limit exceeded
- **CONCURRENCY_INCIDENT** – Concurrent modification conflict detected
- **CYCLE_INCIDENT** – Cyclic dependency detected
- **MALFORMED_INPUT_INCIDENT** – Invalid schema or syntax error

All error paths explicitly signal via incident; no silent failures.

**Reference:** `include/process/process_diagnostics.h`, `include/process/process_diagnostics_api.h`

## Performance Guarantees

### Release-Backed Performance Targets
- **Model serialization:** <50 ms (P95), independent of churn
- **Link creation:** <10 ms (P95), scales with contention
- **Retrieval query:** <100 ms (P95), snapshot-based consistency
- **42 benchmark gates** locked with regression budget ≤10% vs release baseline

### High-Churn Scenario Performance
- Model updates: 100+ updates/sec with 5-15% conflict probability
- Link creation: 100+ links/sec throughput maintained
- Conflict resolution: Deterministic LWW outcome (same timing → same winner)

**Reference:** `src/process/PERFORMANCE_EXPECTATIONS.md` (42 gates documented)

## Edge-Case Guarantees

- **Parser Resource Limits:** Max depth 100, max elements 10K, timeout 30s, model size 100MB
- **Stale Link Handling:** Lazy detection at read-time; manual cleanup required (no cascading deletes)
- **Malformed Input:** Detected and reported with explicit error classification
- **No Automatic Rollback:** Manual retry with latest version; explicit conflict detection
- **Deterministic Outcomes:** LWW winner deterministic; no ties in version ordering

**Reference:** `src/process/PRODUCTION_REQUIREMENTS.md` (resource limits, stress scenarios)

## API Documentation Coverage

All public APIs in `include/process/` have complete Doxygen documentation:

| Concurrency & Thread-Safety | Determinism | Diagnostics | Error Handling |
|---|---|---|---|
| `process_concurrency_contract.h` | `process_determinism_spec.h` | `process_diagnostics.h` | `process_api_contract.h` |
| 4 concurrency patterns documented | Determinism classification | 8 incident classes | Error taxonomy with codes |
| Thread-safety invariants | Conflict resolution semantics | Factory methods | Exception safety guarantees |
| Usage examples | Rollback semantics | Trace context | Per-operation error paths |

**Format:** @brief, @param, @return, @throws, @note, @warning, @code/@endcode, @invariant

## Relevant Implementation Files

| Component | File | Role |
|---|---|---|
| Model Lifecycle | process_model_manager.cpp | Central process model CRUD orchestration |
| BPMN | bpmn_serializer.cpp | BPMN serialization and parsing |
| CMMN | cmmn_serializer.cpp | CMMN serialization support |
| EPK | epk_serializer.cpp, epk_aris_xml_importer.cpp | EPK serialization and ARIS XML import |
| DMN | dmn_evaluator.cpp | DMN rule evaluation |
| OCEL | ocel_exporter.cpp | OCEL export behavior |
| Import Formats | vcc_vpb_importer.cpp, fim_importer.cpp | VCC-VPB and FIM import |
| LLM Integration | llm_process_descriptor.cpp, llm_process_adapter.h | Process descriptors and prompt generation |
| Retrieval | process_graph_rag.cpp, process_agentic_rag.cpp, process_light_retriever.cpp | RAG context assembly |
| Linking | process_linker.cpp | Process-to-object and process-to-process linking |
| Analysis | process_community_detector.cpp, object_centric_tracer.cpp | Community detection and object-centric tracing |
| Generation | process_model_generator.cpp | Model generation support |

## Runtime Behavior and Limits

- **Behavior Quality:** Depends on model quality, input format correctness, and configured retrieval settings
- **Degraded Paths:** Malformed models degrade deterministically with explicit error classification
- **Bounded Behavior:** All resource-bound operations enforced (parser depth, element count, timeout, model size)
- **Retrieval Limits:** Prompt assembly and context generation bounded by module-local constraints
- **High-Churn:** Explicitly documented conflict probability and performance targets

## Production Readiness

**Acceptance Criteria (All ✓ Verified):**
- [x] All new/modified public APIs have complete Doxygen comments
- [x] Concurrency contracts documented with thread-safety guarantees
- [x] Determinism behavior documented with edge-case guarantees
- [x] Diagnostics API documented with incident classification and context
- [x] Production requirements finalized with resource limits and stress scenarios
- [x] Performance expectations finalized with 42 benchmark gates
- [x] Module scope verified and behaviors documented
- [x] Phase 1-6 deliverables all marked COMPLETE
- [x] 72 test cases (Phase 4) passing across C/D/G/P/L/R/S scenarios
- [x] 42 benchmark gates (Phase 5) locked with regression budget enforcement

**Status:** ✓ PRODUCTION READY (2026-08-06)

## Verification & References

**Sourcecode Verification (17 implementation files verified):**
- src/process/process_model_manager.cpp
- src/process/bpmn_serializer.cpp, cmmn_serializer.cpp, epk_serializer.cpp
- src/process/epk_aris_xml_importer.cpp, vcc_vpb_importer.cpp, fim_importer.cpp
- src/process/llm_process_descriptor.cpp, llm_process_adapter.cpp
- src/process/process_graph_rag.cpp, process_agentic_rag.cpp, process_light_retriever.cpp
- src/process/process_linker.cpp
- src/process/process_model_generator.cpp, process_community_detector.cpp, object_centric_tracer.cpp
- src/process/ocel_exporter.cpp, dmn_evaluator.cpp

**Key Documentation:**
- `ROADMAP.md` – Phase 1-6 delivery status and next-cycle planning (Federated Process Evolution Q1 2027)
- `FUTURE_ENHANCEMENTS.md` – Completed features and remaining backlog
- `PRODUCTION_REQUIREMENTS.md` – Edge-case guarantees, resource limits, security requirements
- `PERFORMANCE_EXPECTATIONS.md` – 42 benchmark gates, p95/p99 envelopes, regression budgets
- `PHASE_6_ACCEPTANCE_CHECKLIST.md` – Acceptance criteria and verification status
- `ARCHITECTURE.md` – Module architecture and component relationships
- Historical entries remain in `CHANGELOG.md`

## Known Limitations & Mitigations

| Limitation | Impact | Mitigation |
|---|---|---|
| LWW conflicts under high churn | 5-15% conflict probability | Implement exponential backoff retry logic |
| No cascading deletes | Manual cleanup required for stale links | Monitor MISSING_TARGET_INCIDENT, implement link GC |
| No nested transactions | Conflicts not automatically recovered | Manual merge or retry with conflict detection |
| Subprocess execution order non-deterministic | Different final states possible | Test only final state validity, not execution order |
| Parser resource limits | Large models may be rejected | Pre-validate model size, consider model splitting |

See `src/process/PRODUCTION_REQUIREMENTS.md` for complete operational requirements and stress scenario guarantees.