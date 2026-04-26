> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Ethics AI Module

## Threat Model

### 1. Prompt Injection via Dilemma Text
- **Risk:** A caller supplies a crafted dilemma description containing instructions
  intended to manipulate the LLM backend or argument generation logic.
- **Mitigation:** In v0.0.1 argument content is generated from template strings, not an
  LLM; injection has no effect. When LLM integration is added (v0.1.0), all dilemma
  text and profile content must be treated as untrusted user input and passed in the
  `user` role, never in the `system` role.
- **Status:** ⚠️ Accepted for v0.0.1; must be addressed before LLM integration

### 2. Malicious YAML Philosophy Profiles
- **Risk:** An attacker supplies a crafted YAML file that exploits parser vulnerabilities
  or inserts oversized payloads to exhaust memory.
- **Mitigation:** `PhilosophyLoader::parseYAML` validates required fields and rejects
  profiles that fail validation. YAML files should be sourced from trusted, version-
  controlled directories only. Operators must restrict write access to the
  `philosophy_dir` path.
- **Status:** ✅ Field validation implemented; path restriction is operator responsibility

### 3. Unauthorised Access to Argument Store
- **Risk:** A caller reads or overwrites arguments and decisions belonging to another
  tenant or session.
- **Mitigation:** `ArgumentStore` key namespacing (`entity:ethics_*:{id}`) follows the
  ThemisDB BaseEntity convention; access control is enforced at the ThemisDB auth layer
  (`src/auth/`). The Ethics AI plugin does not bypass or duplicate auth checks.
- **Status:** ✅ Delegated to ThemisDB core auth

### 4. Denial of Service via Large Argument Store Scans
- **Risk:** A caller triggers `getArgumentsByPhilosophy()` or `buildContext()` without
  a `limit`, causing a full-collection scan that exhausts memory or starves other threads.
- **Mitigation:** All retrieval methods accept a `limit` parameter (type `size_t`) with
  a non-zero default enforced in `RAGContextEngine`. AQL queries include `LIMIT` clauses
  from constants in `ethics_aql_queries.h`.
- **Status:** ✅ Limit parameters present on all retrieval APIs

### 5. Sensitive Dilemma Data Leakage via Logs
- **Risk:** Dilemma descriptions may contain PII or confidential business information
  that could be inadvertently written to structured logs.
- **Mitigation:** The module does not log dilemma text or argument content at `INFO`
  level or above. Debug-level logging (disabled in production) should be reviewed before
  enabling.
- **Status:** ✅ No PII logged at production log levels

### 6. Replay / Duplicate Decision Injection
- **Risk:** An attacker replays a previously recorded `storeDecision` call to inject
  duplicate decisions with forged timestamps or scores.
- **Mitigation:** Decision IDs are generated from `std::chrono::system_clock::now()`
  with a monotonic component. Callers cannot supply arbitrary IDs. Duplicate protection
  is enforced by the RocksDB key uniqueness guarantee.
- **Status:** ✅ ID generation is server-side; clients cannot specify IDs

---

## Security Controls Summary

| Control | Implementation | Status |
|---------|---------------|--------|
| YAML input validation | `PhilosophyLoader::parseYAML` required-field checks | ✅ |
| Result size limits | `limit` parameter on all retrieval APIs | ✅ |
| Access control | Delegated to ThemisDB auth layer | ✅ |
| Log sanitisation | No dilemma/argument content in INFO+ logs | ✅ |
| Server-side ID generation | `decision_id` and `argument_id` computed server-side | ✅ |
| Prompt injection (LLM) | N/A in v0.0.1 (template generation only) | ⚠️ Pre-LLM |

---

## Known Limitations

| ID | Description | Severity | Status |
|----|-------------|----------|--------|
| ETHICS-SEC-01 | Prompt injection risk when LLM argument generation is added in v0.1.0 | High | Open (pre-feature) |
| ETHICS-SEC-02 | No rate limiting on `makeDecision()` calls; relies on caller/network layer | Medium | Open |
| ETHICS-SEC-03 | `philosophy_dir` path is trusted on init; no sandbox or path traversal prevention | Low | Open |
