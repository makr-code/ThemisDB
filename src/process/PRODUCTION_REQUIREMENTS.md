# ThemisDB Process Module - Production Requirements

**Status:** 2026-08-06 – Aligned with all Phase 1-6 deliverables  
**Validation:** Process module in production-ready state with explicit edge-case guarantees and bounded resource constraints

## Purpose and Scope

This document is the **canonical reference for production minimum requirements** of the Process Module. It defines binding operational and security requirements for:
- Agentic RAG process orchestration
- LLM process descriptors
- Process model lifecycle (CRUD, import/export)
- Process linking and retrieval
- High-churn and concurrent operation scenarios

## Document Governance

### Canonical Split

| Document | Ownership |
|----------|-----------|
| `src/process/PRODUCTION_REQUIREMENTS.md` (this file) | Mandatory production requirements (MUST/MUST NOT), security assumptions, operational limits |
| `src/process/README.md` | Feature overview, architecture context, API and usage examples |
| `src/process/ROADMAP.md` | Delivery phases, open/closed features, readiness planning |
| `src/process/FUTURE_ENHANCEMENTS.md` | Medium-term and long-term extensions and research areas |
| `src/process/PERFORMANCE_EXPECTATIONS.md` | Benchmark gates, p95/p99 envelopes, release validation |
| `include/process/` (header files) | API contracts, thread-safety guarantees, Doxygen documentation |

## Mandatory Production Requirements

### MUST: Bounded Concurrency Execution
- All process operations must complete within documented time envelopes
- Model serialization must complete in ≤50 ms (P95) regardless of concurrency level
- Link creation must complete in ≤10 ms (P95) per operation
- No unbounded loops in agentic RAG process; max iteration count enforced
- Timeout enforcement: Configurable per-operation timeout with graceful degradation

### MUST: Input Validation Before Execution
- All process model imports must be validated before activation
- Untrusted model descriptors must be rejected with explicit error
- Malformed BPMN/CMMN/DMN input detected and reported (no silent corruption)
- Parser resource limits enforced: max depth, max element count, max timeout
- Serialization payload size limits enforced

### MUST: Configuration Validation at Startup
- All security-relevant configuration values must be validated at startup
- Missing or invalid configuration values must cause fail-closed behavior
- No default values permitted for resource limits or security settings
- Explicit deployment-specific configuration required

### MUST NOT: Disable Security Checks in Production
- No code paths permitted to disable security or authorization checks
- Audit logging for security-relevant operations must be active
- Explicit error signaling for all authorization failures
- No silent permit fallback on security check failures

### MUST: Explicit Error Signaling
- All error paths must explicitly signal failures via incident classification
- No silent failures or implicit recovery paths in production
- Diagnostic records must include actionable operator guidance
- Stale link references must be detected and reported (not silently ignored)

## Edge-Case Guarantees

### High-Churn Scenario Guarantees (>100 updates/sec or >500 concurrent ops)

#### Concurrency Behavior
- **Pattern:** Snapshot isolation (Model Manager), fine-grained locking (Linker), stateless (Serializers)
- **Conflict Probability:** 5-15% LWW conflicts expected under >500 concurrent operations
- **Conflict Resolution:** Last-Write-Wins with monotonic version clocks (deterministic outcome)
- **No Silent Failures:** All conflicts explicitly reported via CONCURRENCY_INCIDENT
- **Deadlock Prevention:** Consistent lock ordering (instance_id → link_id) ensures no deadlocks

#### Performance Envelope Under High Churn
| Operation | Baseline | P95 | P99 | High-Churn |
|-----------|----------|-----|-----|-----------|
| Model serialization | <5 ms | <50 ms | <100 ms | <50 ms (independent) |
| Link creation | 1 ms | <10 ms | <20 ms | <10 ms (scales with contention) |
| Retrieval query | 10 ms | <100 ms | <200 ms | <100 ms (snapshot-based) |
| Model update | 2 ms | <20 ms | <50 ms | Version clock overhead |

#### Throughput Guarantees
- Minimum link creation throughput: **100+ links/sec** under high churn
- Model serialization independent of churn (no starving operations)
- Retrieval queries fully consistent (snapshot isolation prevents stale reads)

### Parser Resource Limits (Bounded Behavior)

| Resource | Limit | Behavior |
|----------|-------|----------|
| Max nesting depth | 100 | Reject with RESOURCE_INCIDENT (kMaxDepthExceeded) |
| Max elements per model | 10,000 | Reject with RESOURCE_INCIDENT (kMaxElementsExceeded) |
| Max retrieval context size | 1 MB | Reject with RESOURCE_INCIDENT (kMaxContextSizeExceeded) |
| Parse timeout | 30 seconds | Abort with RESOURCE_INCIDENT (kExecutionTimeout) |
| Model size | 100 MB | Reject with MALFORMED_INPUT_INCIDENT (truncated or oversized) |

### Stale Link Handling (No Cascading Deletes)

**Guarantee:** When a process model is deleted, existing links become stale but are NOT corrupted or silently deleted.

- **Detection:** Stale links detected at read-time (lazy detection)
- **Signaling:** RETRIEVAL_INCIDENT reported when stale link is accessed
- **Recovery:** Manual operator intervention required (no automatic repair)
- **Consistency:** Link references persist even if target model deleted

**Operator Procedure for Stale Links:**
1. Monitor for RETRIEVAL_INCIDENT with MISSING_TARGET_INCIDENT classification
2. Query link status to identify stale references
3. Manually delete stale links or restore missing models
4. No automatic cleanup (prevents accidental data loss)

### Determinism Guarantees (Conflict Resolution)

**Guarantee:** Concurrent updates to the same model resolve deterministically via LWW.

| Scenario | Outcome | Invariant |
|----------|---------|-----------|
| Two concurrent updates to same model | Winner determined by version clock | No ties; winner is deterministic |
| Read during model update | Returns snapshot at consistent instant | Snapshot isolation enforced |
| Multiple serialization calls | Same model structure, same ID | RFC 4122 v5 namespace ensures stability |
| State transition under concurrent events | Valid final state per BPMN 2.0 | Subprocess ordering non-deterministic |

### No Automatic Rollback

**Guarantee:** Process module does not support nested transactions or automatic rollback.

- **Conflict Detection:** Conflicts detected via version clock mismatch
- **Operator Action:** Retry with latest model version or abort operation
- **Manual Remediation:** Inspect model state and apply corrective updates
- **No Cascade:** Partial failures not automatically undone

**Typical Conflict Recovery Sequence:**
1. Update fails with CONCURRENCY_INCIDENT
2. Re-fetch model at latest version
3. Merge changes manually or auto-retry operation
4. If merge fails, escalate to domain expert

## Mandatory Security Requirements

### Confidentiality & Integrity
- Security-relevant operations routed through dedicated control surfaces
- Errors in security-critical paths explicitly propagated (no silent permit)
- Audit logging active in production deployments for all security events
- No side-channel information leakage via timing or error messages

### Authorization & Access Control
- All process model access subject to authorization checks
- Link operations validated for both source and target authorization
- Retrieval queries subject to access control on model scope
- Failed authorization logged with operation details

### Configuration & Deployment
- Security configuration values validated at startup
- No default values for critical settings (explicit deployment config required)
- Production mode activated via `THEMIS_PRODUCTION_MODE` or `THEMIS_ENVIRONMENT`
- Configuration errors cause immediate failure (fail-closed)

## Operational Requirements

### Resource Limits (Deployment-Specific)

Must be explicitly configured per deployment; no unlimited defaults permitted:

| Resource | Requirement | Justification |
|----------|-------------|---|
| Parser max depth | Set per model complexity | Prevent stack exhaustion |
| Parser max elements | Set per performance envelope | Bound memory consumption |
| Model cache size | Set per available memory | Prevent OOM in retrieval layer |
| Link cache size | Set per high-churn expectation | Bound linker memory usage |
| Concurrent request limit | Set per CPU cores | Prevent thread exhaustion |
| Query timeout | Set per SLA requirements | Bounded operation latency |

### External Dependency Configuration

All external dependencies must have explicit timeouts and retry policies:

| Dependency | Requirement | Configuration |
|------------|-------------|---|
| Model storage | Connection timeout ≤5s | Explicit retry policy (3x exponential backoff) |
| Link storage | Read timeout ≤3s, write ≤10s | Circuit breaker after 10 failures |
| LLM service | Request timeout ≤60s | Max 3 retries with exponential backoff |
| Graph database | Query timeout ≤30s | Fallback to cached results |

### Production Environment Validation Checklist

- [ ] Module configuration complete and validated at startup
- [ ] Security and authorization checks active (no bypasses)
- [ ] Resource limits explicitly configured (no unlimited defaults)
- [ ] Audit logging active and forwarded to SIEM
- [ ] External dependencies configured with timeouts and retry policies
- [ ] Production mode activated (`THEMIS_PRODUCTION_MODE=true`)
- [ ] All database connections use TLS (no plaintext)
- [ ] API authentication required (no anonymous access)
- [ ] Rate limiting configured per endpoint
- [ ] Error messages do not leak sensitive information
- [ ] Monitoring and alerting configured for key metrics

## Monitoring & Observability Requirements

### Mandatory Metrics

Monitor these metrics continuously in production:

- **Concurrency incidents per minute:** Alert if >1 per minute (indicates conflict storm)
- **Parser resource incidents per hour:** Alert if >10 per hour (indicates malformed input)
- **Link staleness rate:** Alert if >5% of queries return MISSING_TARGET_INCIDENT
- **P95 model serialization latency:** Alert if >60 ms (regression threshold)
- **P95 link creation latency:** Alert if >15 ms (regression threshold)
- **High-churn throughput (links/sec):** Alert if <80 links/sec (degradation)

### Diagnostic Context for Incidents

Each incident must capture:
- Timestamp (UTC, millisecond precision)
- Operation context (what was being attempted)
- Input identifier (model ID, instance ID, etc.)
- Resource metrics at incident time (depth, element count, etc.)
- Actionable remediation suggestion for operator
- Conflicting operation IDs (if concurrency incident)

## Documentation References

### Concurrency & Thread-Safety
- `include/process/process_concurrency_contract.h` – Thread-safety model with invariants
- **Guarantee:** Stateless serializers (no locks), snapshot isolation for model manager, fine-grained locking for linker
- **Usage:** Developers must follow documented patterns; no ad-hoc synchronization

### Determinism & Conflict Resolution
- `include/process/process_determinism_spec.h` – Determinism classifications and LWW semantics
- **Guarantee:** Parsing deterministic (RFC 4122 v5), conflicts deterministic (version clock), no ties
- **Usage:** Applications must implement retry logic with exponential backoff for conflicts

### Diagnostics & Error Handling
- `include/process/process_diagnostics.h` – 8 incident classes with factory methods
- **Guarantee:** All error paths produce explicit incidents; no silent failures
- **Usage:** Catch incidents by type and apply standard remediation procedures

### Production Readiness
- `src/process/PERFORMANCE_EXPECTATIONS.md` – Benchmark gates and release validation
- **Guarantee:** All hot paths have regression budgets; release gates enforced
- **Usage:** Monitor key metrics against release baseline

## Verification & Audit

### Self-Audit Checklist for Production Deployment

1. **Configuration Validation**
   - [ ] All resource limits configured explicitly
   - [ ] Security settings not using defaults
   - [ ] Timeouts configured for all external services
   - [ ] No feature flags disabling critical checks

2. **Operational Readiness**
   - [ ] Monitoring alerts configured for all mandatory metrics
   - [ ] Runbooks created for common incidents (CONCURRENCY, RESOURCE, VALIDATION)
   - [ ] Rollback procedures documented
   - [ ] Incident response team briefed on process module diagnostics

3. **Security Audit**
   - [ ] Authorization checks active (manual code review)
   - [ ] Audit logging forwarded to SIEM
   - [ ] API authentication enabled
   - [ ] TLS enforced for all connections

4. **Performance Baseline**
   - [ ] Release baseline benchmarks recorded
   - [ ] P95/P99 envelopes verified for hot paths
   - [ ] Regression budget enforcement enabled
   - [ ] High-churn scenario performance validated

### Audit Trail References

**Verified files:**
- `include/process/process_concurrency_contract.h` – Thread-safety patterns verified
- `include/process/process_determinism_spec.h` – Determinism guarantees verified
- `include/process/process_diagnostics.h` – Incident classification verified
- `include/process/process_api_contract.h` – Error taxonomy verified
- `src/process/process_model_manager.cpp` – Snapshot isolation implementation verified
- `src/process/process_linker.cpp` – Fine-grained locking implementation verified

## Known Limitations & Mitigations

| Limitation | Impact | Mitigation |
|-----------|--------|-----------|
| LWW conflicts under high churn | 5-15% conflict probability | Implement exponential backoff retry logic |
| No cascading deletes | Manual cleanup required for stale links | Monitor MISSING_TARGET_INCIDENT, implement link garbage collection |
| No nested transactions | Conflicts not automatically recovered | Manual merge or retry with conflict detection |
| Subprocess execution order non-deterministic | Different final states possible | Test only final state validity, not execution order |
| Parser resource limits | Large models may be rejected | Pre-validate model size, consider model splitting |

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-08-06 | Initial production requirements (Phase 6 completion) |

