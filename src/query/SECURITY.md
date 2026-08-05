> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-05-27 -->
<!-- Links: README.md · ARCHITECTURE.md · SECURITY.md (root) -->

# Security Policy — Query Module

## Supported Versions

| Version | Security Fixes |
|---------|---------------|
| 1.5.x   | ✅ Active      |
| 1.4.x   | ✅ Active      |
| < 1.4   | ❌ EOL         |

## Threat Model

### T1 — AQL Injection
- **Risk:** High — unsanitized user input interpolated into AQL queries could allow data exfiltration or manipulation
- **Mitigation:** AQL injection detector in the security module; parameterized query API separates structure from values; SPARQL/SQL inputs are parsed and translated, never executed directly; all parser numeric conversions (`stoll`/`stod`) are wrapped in try-catch to prevent exception-based DoS from malformed numeric literals (REL-10..19, 2026-05-27); **Phase 1 Q3 2026**: enhanced parser validation for malformed input (empty queries, unclosed delimiters, invalid token sequences) tested in `test_query_parser_edge_cases.cpp` § 41 scenarios
- **Residual risk:** Low when parameterized API is used; direct string interpolation by callers bypasses protection

### T2 — Resource Exhaustion
- **Risk:** High — unbounded queries could exhaust memory, CPU, or I/O
- **Mitigation:** Per-query limits enforced: max-rows, max-memory, timeout; `QueryFederation` caps join inputs/results, scatter-gather merges, aggregation shard/output payloads, and federated RAG accumulation via `max_result_size_bytes`; query cancellation via request ID (`query_canceller.cpp`); limits are mandatory and not caller-optional; `CrossClusterFederator` HTTP response capped at 64 MiB (CCF-01, 2026-05-27); `ContinuousQueryEngineImpl` registry capped at 1 000 queries (CQE-03, 2026-05-27) and injection queue capped at 100 000 entries (CQE-02, 2026-05-27); **Phase 1 Q3 2026**: expression recursion depth `kMaxExprDepth=500` and graph traversal depth bounded; long-query and large-filter handling tested (Tests 40–41)
- **Residual risk:** Low — limits are enforced in the execution engine; extreme edge cases under benchmark review

### T3 — Cross-Tenant Data Access
- **Risk:** High — a query executing across tenant boundaries could leak data
- **Mitigation:** Query execution paths enforce caller-provided collection access checks via `collection_access_checker_` and return `ERR_QUERY_ACCESS_DENIED` on denial; tenant namespace isolation and federation routing boundaries remain additional controls; **Phase 1 Q3 2026**: documented in `ACCESS_VALIDATION_CHECKLIST.md` with entry-point matrix; tested via regression suite in `test_query_parser_edge_cases.cpp`
- **Residual risk:** Low — isolation is structural; misconfigured namespace mappings are an operational risk

### T4 — SPARQL / SQL Injection
- **Risk:** Medium — translated dialects could carry injection payloads
- **Mitigation:** SPARQL and SQL inputs are fully parsed into an intermediate AST and translated to AQL; raw dialect strings are never executed; injection payloads do not survive AST round-trip; **Phase 1 Q3 2026**: mutation safety validator enhanced to detect DML/DDL patterns in read-only contexts (Tests 24–31)
- **Residual risk:** Low — translation is parse-then-emit; not string-splice

### T5 — Cross-Cluster SSRF / Header Injection
- **Risk:** Medium — `CrossClusterFederator` dispatches HTTP requests to registered cluster endpoints; a compromised or misconfigured registration could target internal services
- **Mitigation:** `registerCluster()` rejects `base_url` values not starting with `http://` or `https://` (CCF-03, 2026-05-27) and rejects `auth_token` containing CR/LF to prevent header injection (CCF-04, 2026-05-27); redirect hops capped at 3 (CCF-02, 2026-05-27); libcurl request and redirect protocols restricted to HTTP/HTTPS only (`CURLOPT_PROTOCOLS`, `CURLOPT_REDIR_PROTOCOLS`, CCF-05, 2026-05-27); SSL peer verification enforced (`CURLOPT_SSL_VERIFYPEER`)
- **Residual risk:** Low — non-HTTP schemes and CR/LF header injection are blocked at registration/transport layer; residual risk limited to operationally allowed HTTP(S) endpoints (network-policy concern)

## Known Limitations

| ID    | Description                                                            | Target Fix   |
|-------|------------------------------------------------------------------------|--------------|
| KL-01 | ~~`AQLParser` instances are not thread-safe; must be per-thread or mutex-protected~~ — **Closed 2026-05-26**: `AQLParser` is stateless; every public method constructs local `Tokenizer`+`Parser` objects, so concurrent calls on a shared instance are safe. | ✅ Closed |
| KL-02 | Performance benchmarks for vectorized and federated paths not yet published | Q2 2026 |
| KL-03 | ~~Full security audit (injection + resource exhaustion) in progress~~ — **Closed 2026-05-27**: audit findings tracked in `AUDIT.md` are resolved (CCF-01..05, CQE-01..03, TC-01..15, REL-01..19, UNINIT-01..20, PERF-01..05, IV-01). | ✅ Closed |
| KL-04 | ~~Parser safety and access validation completeness~~ — **Closed 2026-08-05 Phase 1**: enhanced parser edge-case tests (41 scenarios), access validation consistency documented with entry-point matrix; see `ACCESS_VALIDATION_CHECKLIST.md`. | ✅ Closed |

## Phase 1 (Q3 2026): Safety and Access Hardening

### Deliverables

- **Parser Safety Test Suite** (`tests/query/test_query_parser_edge_cases.cpp`): 41 comprehensive edge-case tests covering:
  - Malformed queries (empty, whitespace-only, unclosed delimiters, invalid tokens)
  - Expression depth bounds (nested parentheses, function calls, arrays)
  - Invalid token sequences (missing clauses, duplicated bindings)
  - Mutation safety validation (INSERT, UPDATE, REMOVE, DELETE, REPLACE, UPSERT, DROP detection)
  - Access control scenarios (valid reads, shadowing, complex filters, graph traversal)
  - Resource exhaustion guards (long queries, many filters)

- **Access Validation Checklist** (`src/query/ACCESS_VALIDATION_CHECKLIST.md`): Verification matrix for:
  - Parser stage: collection name extraction; no SQL injection via AST round-trip
  - Execution stage: `collection_access_checker_` callback enforcement in all entry points
  - Federation stage: remote cluster checks + result merge bounds

- **Architecture Documentation** (`src/query/ARCHITECTURE.md` § 8.1–8.3): 
  - Parser safety hardening details with test cross-references
  - Three-stage access validation flow with entry-point matrix
  - General security properties

- **Security Policy Update** (`src/query/SECURITY.md` § Threat Model + Known Limitations):
  - Enhanced T1 (AQL Injection): reference to Phase 1 parser validation tests
  - Enhanced T2 (Resource Exhaustion): expression recursion depth, test coverage
  - Enhanced T3 (Cross-Tenant Data Access): reference to `ACCESS_VALIDATION_CHECKLIST.md`
  - Enhanced T4 (SPARQL/SQL Injection): mutation safety validator enhancements
  - New known limitation KL-04 closed 2026-08-05

## Reporting a Vulnerability

Report via the project's private security disclosure channel (see root `SECURITY.md`).
Do **not** open public issues for security vulnerabilities.
Response SLA: acknowledgement within 2 business days; severity assessment within 5 business days.

## Sourcecode Verification (Module: query/security)

- Verified files:
	- `src/query/query_engine.cpp`
	- `src/query/query_federation.cpp`
	- `src/query/cross_cluster_federation.cpp`
	- `src/query/continuous_query_engine.cpp`
	- `include/query/aql_parser.h`
- Verified controls:
	- Collection access denial path (`ERR_QUERY_ACCESS_DENIED`) in query execute entry points
	- Federation result-size guardrails (`max_result_size_bytes`)
	- Cross-cluster transport restrictions (`CURLOPT_PROTOCOLS`, `CURLOPT_REDIR_PROTOCOLS`, redirect cap)
	- Continuous-query registry/injection queue bounds
	- Parser statelessness and recursion-depth limits
