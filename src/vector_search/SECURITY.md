# Security - Vector Search Module

<!-- Status: current | validated: 2026-09-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Threat Model

| Threat | Impact | Mitigation Surface |
|---|---|---|
| Malicious vectors corrupting index structure | Denial of service, incorrect results | Dimension validation, NaN/inf detection, integrity checks |
| Excessive memory consumption via large vectors | Memory exhaustion, system crash | Vector size limits, batch operation bounds |
| Invalid distance metric selection | Incorrect similarity rankings | Metric enum validation, error returns (E5401) |
| Concurrent modification races | Index corruption, undefined behavior | Read-write mutex protection, atomic flags |
| Serialized index tampering | Trust boundary violation | Index checksums (planned Phase 6), integrity validation |

## Security Controls

### Input Validation Boundaries

#### Boundary 1: Vector Dimension Validation
**Location:** Index construction and search paths  
**Protection:** Reject vectors with dimension mismatch  
**Enforced on:**
- All vectors added to index must match declared dimension
- Query vectors must match index dimension

**Rationale:** Prevents out-of-bounds memory access and algorithm corruption.

#### Boundary 2: Vector Value Validation
**Location:** Vector ingestion paths  
**Protection:** Reject vectors containing NaN, inf, or out-of-range values  
**Enforced on:**
- All floating-point vector components
- Distance score values

**Rationale:** Ensures deterministic distance computation and prevents floating-point anomalies.

#### Boundary 3: Batch Operation Bounds
**Location:** Index building and query execution  
**Protection:** Limit batch sizes and concurrent operations  
**Enforced on:**
- Maximum vectors per insert batch
- Maximum concurrent query threads
- Query result set sizes

**Rationale:** Prevents resource exhaustion and unbounded memory allocation.

#### Boundary 4: Distance Metric Validation
**Location:** Index creation and query setup  
**Protection:** Validate metric enum values; return error on unsupported metrics  
**Enforced on:**
- COSINE, L2, INNER_PRODUCT only
- Invalid metrics rejected with error code E5401

**Rationale:** Ensures consistent similarity semantics across search operations. Input validation is caller's responsibility; no silent fallback occurs.

### Concurrency & Thread Safety

#### Shared Data Protection

All mutable shared state (index structures, cache, metrics) protected by:
- `std::shared_mutex` for read-write separation (concurrent reads, exclusive writes)
- `std::atomic<>` for flags and counters (lock-free operations)
- Scoped RAII lock guards (exception-safe locking)

#### Search Operations (Read-Safe)
- Multiple readers allowed concurrently
- No modification of index structure
- Atomic cache hits/misses tracking

#### Modification Operations (Write-Exclusive)
- Add, delete, rebuild operations require exclusive write lock
- Blocks concurrent searches during modification
- Prevents race conditions on index metadata

#### Known Limitations

- No lock-free search (read-write mutex required for consistency)
- Concurrent modifications serialized (no parallel deletes/rebuilds)
- Large rebuilds may block search operations temporarily

### Memory Safety

#### Vector Storage
- Fixed-size vectors allocated once, never resized mid-operation
- Out-of-bounds access prevented by dimension validation
- Memory freed via RAII destructors (no manual deletion)

#### Graph Structures (HNSW)
- Node connectivity validated on access
- Dead pointer detection during graph traversal
- Index rebuilding clears stale references

#### Cluster Structures (IVF)
- Centroid stability checked after rebuild
- Empty cluster handling (no undefined behavior)
- Cluster index bounds validated on search

### Error Reporting

All error conditions return explicit error codes (E5400–E5499):
- E5400: Invalid vector dimension (security-relevant)
- E5401: Vector contains NaN or inf (security-relevant)
- E5402: Index is empty (operational)
- E5403: Search returned no results (operational)
- E5404: Index corruption detected (security-relevant)

No silent failures; all error conditions logged.

## Defense-in-Depth Strategy

### Layer 1: Input Acceptance
- Validate all user-supplied vectors before index modification
- Reject invalid dimensions, NaN/inf values, oversized batches

### Layer 2: Algorithm Execution
- Protect shared data with appropriate synchronization primitives
- Validate algorithm-specific preconditions (e.g., cluster validity in IVF)

### Layer 3: Result Delivery
- Validate result structures before returning to caller
- Ensure distance scores are finite and in expected range

### Layer 4: Error Isolation
- Distinguish between recoverable and fatal errors
- Graceful degradation for resource exhaustion
- Index preservation on corruption detection

## Integration Security

### RAG Module Integration
- Vector search receives query embeddings (untrusted inputs)
- Search results (document IDs, distances) returned to RAG
- No cross-contamination between search contexts

**Boundary:** Vector search does not access or cache RAG documents; RAG controls document retrieval.

### Server Integration
- HTTP endpoints receive distance metric selection from clients
- Metric enum validation before index access
- Query result sizes bounded to prevent response explosion

**Boundary:** Server implements input sanitization; vector search trusts pre-validated inputs.

## Operational Security

### Index Persistence (Planned)
- Checksums on serialized indices (Phase 6)
- Versioning for forward/backward compatibility
- Corruption detection on load

### Audit and Monitoring
- Query latency histograms (detect anomalies)
- Cache hit/miss ratios (detect workload changes)
- Error rate tracking (detect attacks or faults)

### Maintenance Windows
- Index validation procedures documented
- Rebuild procedures for corruption recovery
- Health check commands for operational triage

## Known Vulnerabilities & Mitigations

| Vulnerability | Severity | Status | Mitigation |
|---|---|---|---|
| Concurrent modification race during index rebuild | High | Known, Mitigated | Write-exclusive lock during rebuild |
| Out-of-memory on very large vector batches | High | Known, Mitigated | Batch size limits + validation |
| Floating-point anomalies in distance computation | Medium | Known, Mitigated | NaN/inf pre-validation + post-computation checks |
| Deserialization of untrusted index files | Medium | Planned | Index checksums (Phase 6) |

## Security Testing

### Current Test Coverage
- Unit tests for invalid vector rejection
- Dimension mismatch detection tests
- Concurrent access stress tests (no corruption)
- Out-of-memory handling tests

### Planned Test Coverage
- Malicious vector pattern injection (fuzz testing)
- Concurrency race condition detection (ThreadSanitizer)
- Index tampering resilience (checksum validation)
- Performance under adversarial query patterns

## Compliance Notes

- No cryptographic operations in vector search (scope exclusion)
- No network communication (local indices only)
- No persistent storage of sensitive metadata (current; Phase 6 may change)
- All index data is derived from user-supplied vectors (no secrets)

---

See [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md) for operational security boundaries and constraints.
