# Security - LLM Wiki Module

<!-- Status: current | validated: 2026-09-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Threat Model

| Threat | Impact | Mitigation Surface |
|---|---|---|
| Prompt injection attacks via malicious wiki content | Unauthorized LLM behavior, information disclosure | Guardrail pattern detection, content validation gates |
| Cross-workspace data leakage | Privilege escalation, data breach | Workspace isolation, access control enforcement |
| Corrupted workspace state causing incorrect responses | Reliability failure, incorrect information delivery | Checksum validation, atomic persistence, recovery paths |
| Unauthorized edition access (e.g., Military features in Community) | Feature/data access violation, compliance failure | Edition-gated API enforcement, capability matrix validation |
| Policy tampering via malformed YAML process policies | Denial of service, security gate bypass | Schema validation, fail-closed loading, hot-reload safety |
| Inadequate provenance validation allowing synthetic chain growth | Low-confidence answers, attribution failure | Chain-depth validation, re-anchor signaling, confidence thresholds |
| Unaudited evidence acceptance at synthesis time | Untrusted answer generation | Pre-synthesis allowlist gate, provenance confidence enforcement |

## Security Controls

### Input Validation Boundaries

#### Boundary 1: Prompt Injection Detection
**Location:** Pre-extraction gate; enforced before ingestion and synthesis  
**Protection:** Reject or sanitize inputs matching 60+ guardrail patterns  
**Enforced on:**
- All user-supplied queries
- All ingested wiki content
- All synthesized prompt instructions

**Pattern Categories:**
- Shell command injection (>, |, ;, &, $, `, backtick escapes)
- Code/syntax injection (Python, SQL, regex, custom DSLs)
- Encoding attacks (Unicode, UTF-7, mixed-case, nested escapes)
- Privilege escalation (role-switching commands, permission-elevation payloads)
- Control-flow disruption (context switching, instruction override, ignore-previous-instruction variants)

**Rationale:** Prevents adversarial manipulation of LLM behavior and knowledge graph.

#### Boundary 2: Workspace Isolation
**Location:** Workspace creation and access paths  
**Protection:** Enforce strict workspace boundaries; no cross-workspace data leakage  
**Enforced on:**
- All workspace create/delete operations
- All query operations
- All ingest operations

**Rationale:** Multi-tenant safety; prevents privilege escalation across workspace boundaries.

#### Boundary 3: Edition-Gated Access Control
**Location:** Plugin instantiation and operation-level checks  
**Protection:** Enforce per-edition capability matrix  
**Enforced on:**
- Module instantiation (deny LLMWikiPlugin in Community edition)
- Query operations (allow only in Enterprise+ editions)
- Ingest operations (allow only in Enterprise+ editions)
- Guardrail patterns (enforce stricter patterns in Military edition)

**Rationale:** Ensures compliance with edition licensing and security posture requirements.

#### Boundary 4: Checksum & State Validation
**Location:** Workspace state persistence and load paths  
**Protection:** Validate workspace state integrity via checksums; reject corrupted state  
**Enforced on:**
- Workspace persistence (checksum computed and stored)
- Workspace load (checksum validated before use)
- Recovery paths (log-based replay with validation)

**Rationale:** Prevents silent state corruption leading to incorrect responses.

#### Boundary 5: YAML Process Policy Validation
**Location:** Policy loader initialization and hot-reload  
**Protection:** Validate policy against JSON schema; fail closed on schema violation  
**Enforced on:**
- Initial policy load (fail-closed on schema error)
- Hot-reload updates (restore last-known-good on validation failure)
- Runtime invariant checks (bounds validation on timing, threshold, gate settings)

**Rationale:** Prevents malformed policies from bypassing security gates or causing denial of service.

#### Boundary 6: Provenance Chain Validation
**Location:** Pre-synthesis gate  
**Protection:** Enforce maximum chain depth, minimum confidence, allowed evidence origins  
**Enforced on:**
- All evidence package synthesis
- All provenance metadata acceptance
- All transform-chain operations

**Rationale:** Prevents low-confidence, excessively-derived, or untrusted evidence from contaminating answer synthesis.

### Concurrency & Thread Safety

#### Shared Data Protection

All mutable shared state (workspace cache, policy, metrics) protected by:
- `std::shared_mutex` for read-write separation (concurrent reads, exclusive writes)
- `std::atomic<>` for flags and counters (lock-free operations)
- Scoped RAII lock guards (exception-safe locking)

#### Query Operations (Read-Safe)
- Multiple concurrent readers allowed
- No modification of workspace state
- Atomic cache hits/misses tracking

#### Modification Operations (Write-Exclusive)
- Ingest, delete, policy reload require exclusive write lock
- Blocks concurrent queries during modification
- Prevents race conditions on workspace metadata

#### Known Limitations

- No lock-free workspace reads (read-write mutex required for consistency)
- Concurrent modifications serialized (no parallel ingests/deletes)
- Policy hot-reload may cause temporary query blocking

### Memory Safety

#### Workspace Storage
- Fixed-size workspace state allocated once
- Overflow detection on ingest operations
- Memory freed via RAII destructors (no manual deletion)

#### Policy Objects
- Policy parameters validated at load time
- Out-of-range bounds checked before use
- Safe string handling (std::string, no buffer overflows)

#### Guard Pattern Matching
- Pattern strings pre-compiled and immutable
- Matching algorithm bounds-checked for input length
- No unbounded regex backtracking (patterns are literal strings)

### Error Reporting

All error conditions return explicit error codes or exceptions:
- Edition access denied: `E6500` (LWP-07 gate)
- Workspace not found: `E6501`
- Guardrail pattern matched: `E6502`
- Workspace state corrupted: `E6503` (checksum validation)
- Policy validation failed: `E6504`
- Chain depth exceeded: `E6505`
- Confidence below threshold: `E6506`
- Evidence origin not allowlisted: `E6507`

No silent failures; all error conditions logged.

## Defense-in-Depth Strategy

### Layer 1: Input Acceptance (Guardrails)
- Validate all user-supplied queries against guardrail patterns before processing
- Reject invalid dimensions, oversized batches, malformed inputs
- Log all rejected inputs for audit trail

### Layer 2: Workspace Isolation
- Enforce strict workspace boundaries at operation boundaries
- Validate workspace identifiers before any access
- Prevent cross-workspace data leakage via parameterized queries

### Layer 3: Edition Gating
- Verify edition entitlement at plugin instantiation
- Enforce capability matrix at operation-level checks
- Return explicit error on denied operations

### Layer 4: State Integrity
- Compute and persist checksums on workspace modifications
- Validate checksums before using workspace state
- Detect and reject silently corrupted state

### Layer 5: Provenance Validation
- Enforce allowlist rules on evidence origins
- Validate chain depth and confidence metrics
- Signal re-anchor requirement for degraded provenance

### Layer 6: Policy Enforcement
- Validate YAML policies against schema
- Enforce fail-closed semantics on policy load/reload
- Preserve last-known-good policy on rollback

### Layer 7: Error Isolation
- Distinguish between recoverable and fatal errors
- Graceful degradation for resource exhaustion
- Workspace preservation on corruption detection

## Integration Security

### LLM Module Integration
- Wiki queries receive synthesized prompts (untrusted inputs from LLM orchestration layer)
- Query results (evidence, confidence) returned to LLM for synthesis
- **Boundary:** Wiki does not access or cache LLM-internal state; LLM controls prompt composition.

### Retrieval Module Integration
- Wiki receives ranked evidence from retrieval module
- Evidence metadata (origin, confidence, chain depth) validated before use
- **Boundary:** Retrieval does not enforce wiki-specific security policies; wiki owns evidence allowlist.

### Server Integration
- HTTP endpoints receive query parameters from clients
- Edition verification performed at API layer
- **Boundary:** Server implements input sanitization; wiki enforces edition gates.

### Plugin Boundary
- Only Enterprise+ editions can instantiate LLMWikiPlugin
- Community edition fails at plugin load time
- **Boundary:** Plugin architecture enforces capability matrix; no runtime fallback to disabled features.

## Operational Security

### Audit Trail
- All workspace modifications logged with timestamp, operation, and editor identity
- All guardrail detections logged with matched pattern and query context
- All edition denials logged with edition requested vs. granted
- Audit log persisted alongside workspace state

### Access Control Audit
- Workspace access logs include operation type, edition, and actor identity
- Periodic audit scans for unauthorized access attempts
- Alerts on repeated denial patterns (potential attack probing)

### Policy Change Control
- Policy versions tracked alongside workspace state
- Policy changes validated and logged before activation
- Rollback procedure documented and tested
- Canary deployment process planned for Phase 6

### Maintenance Windows
- Workspace validation procedures documented
- Recovery procedures for checksum failures
- Health check commands for operational triage
- Policy update and validation procedures

## Known Vulnerabilities & Mitigations

| Vulnerability | Severity | Status | Mitigation |
|---|---|---|---|
| Concurrent modification race during workspace reload | High | Known, Mitigated | Write-exclusive lock during reload + version tracking |
| Guardrail pattern evasion via Unicode/mixed-case | High | Known, Mitigated | Normalization before pattern matching |
| Out-of-memory on very large evidence packages | High | Known, Mitigated | Evidence size limits + validation |
| Unsupervised schema extensions compromising validation | Medium | Known, Mitigated | Fail-closed validation; only allowlisted extensions accepted |
| Deserialization of untrusted policy files | Medium | Planned | Schema validation (implemented); checksums (Phase 6) |
| Provenance confidence manipulation | Medium | Known, Mitigated | Re-anchor signaling; confidence threshold enforcement |

## Security Testing

### Current Test Coverage
- Unit tests for guardrail pattern rejection (LWP-05)
- Edition access control enforcement tests (LWP-07)
- Workspace isolation and checksum validation tests (LWP-06)
- YAML policy schema validation tests
- Concurrent access stress tests (no data leakage)
- Error handling tests for malformed inputs

### Planned Test Coverage (Phase 5-6)
- Fuzzing: Malicious wiki content and query patterns
- Concurrency race detection via ThreadSanitizer
- Policy tampering resilience (corrupted YAML files)
- Cross-workspace boundary violation attempts
- Large-scale provenance chain attack scenarios
- Edition gate bypass attempts
- Guardrail pattern evasion (unicode, encoding)

## Compliance Notes

- No cryptographic operations in wiki module (scope exclusion; cryptography is caller's responsibility)
- No persistent authentication tokens or secrets in workspace state
- All access decisions are deterministic and auditable
- No background data collection or telemetry
- Privacy: Workspace data isolated per tenant; no cross-workspace aggregation

---

See [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md) for operational security boundaries and constraints.
