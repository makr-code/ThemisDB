> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
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
- **Mitigation:** AQL injection detector in the security module; parameterized query API separates structure from values; SPARQL/SQL inputs are parsed and translated, never executed directly; all parser numeric conversions (`stoll`/`stod`) are wrapped in try-catch to prevent exception-based DoS from malformed numeric literals (REL-10..19, 2026-05-27)
- **Residual risk:** Low when parameterized API is used; direct string interpolation by callers bypasses protection

### T2 — Resource Exhaustion
- **Risk:** High — unbounded queries could exhaust memory, CPU, or I/O
- **Mitigation:** Per-query limits enforced: max-rows, max-memory, timeout; `QueryFederation` caps join inputs/results, scatter-gather merges, aggregation shard/output payloads, and federated RAG accumulation via `max_result_size_bytes`; query cancellation via request ID (`query_canceller.cpp`); limits are mandatory and not caller-optional; `CrossClusterFederator` HTTP response capped at 64 MiB (CCF-01, 2026-05-27); `ContinuousQueryEngineImpl` registry capped at 1 000 queries (CQE-03, 2026-05-27) and injection queue capped at 100 000 entries (CQE-02, 2026-05-27)
- **Residual risk:** Low — limits are enforced in the execution engine; extreme edge cases under benchmark review

### T3 — Cross-Tenant Data Access
- **Risk:** High — a query executing across tenant boundaries could leak data
- **Mitigation:** Tenant namespace isolation enforced via `KeySchema`; federation routing respects tenant boundaries
- **Residual risk:** Low — isolation is structural; misconfigured namespace mappings are an operational risk

### T4 — SPARQL / SQL Injection
- **Risk:** Medium — translated dialects could carry injection payloads
- **Mitigation:** SPARQL and SQL inputs are fully parsed into an intermediate AST and translated to AQL; raw dialect strings are never executed; injection payloads do not survive AST round-trip
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

## Reporting a Vulnerability

Report via the project's private security disclosure channel (see root `SECURITY.md`).
Do **not** open public issues for security vulnerabilities.
Response SLA: acknowledgement within 2 business days; severity assessment within 5 business days.
