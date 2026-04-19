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
- **Mitigation:** AQL injection detector in the security module; parameterized query API separates structure from values; SPARQL/SQL inputs are parsed and translated, never executed directly
- **Residual risk:** Low when parameterized API is used; direct string interpolation by callers bypasses protection

### T2 — Resource Exhaustion
- **Risk:** High — unbounded queries could exhaust memory, CPU, or I/O
- **Mitigation:** Per-query limits enforced: max-rows, max-memory, timeout; query cancellation via request ID (`query_canceller.cpp`); limits are mandatory and not caller-optional
- **Residual risk:** Low — limits are enforced in the execution engine; extreme edge cases under benchmark review

### T3 — Cross-Tenant Data Access
- **Risk:** High — a query executing across tenant boundaries could leak data
- **Mitigation:** Tenant namespace isolation enforced via `KeySchema`; federation routing respects tenant boundaries
- **Residual risk:** Low — isolation is structural; misconfigured namespace mappings are an operational risk

### T4 — SPARQL / SQL Injection
- **Risk:** Medium — translated dialects could carry injection payloads
- **Mitigation:** SPARQL and SQL inputs are fully parsed into an intermediate AST and translated to AQL; raw dialect strings are never executed; injection payloads do not survive AST round-trip
- **Residual risk:** Low — translation is parse-then-emit; not string-splice

## Known Limitations

| ID    | Description                                                            | Target Fix   |
|-------|------------------------------------------------------------------------|--------------|
| KL-01 | `AQLParser` instances are not thread-safe; must be per-thread or mutex-protected | Planned refactor |
| KL-02 | Performance benchmarks for vectorized and federated paths not yet published | Q2 2026 |
| KL-03 | Full security audit (injection + resource exhaustion) in progress       | Q2 2026      |

## Reporting a Vulnerability

Report via the project's private security disclosure channel (see root `SECURITY.md`).
Do **not** open public issues for security vulnerabilities.
Response SLA: acknowledgement within 2 business days; severity assessment within 5 business days.
