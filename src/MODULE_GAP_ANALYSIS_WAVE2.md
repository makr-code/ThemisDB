# ThemisDB — Core Module Gap Analysis & Wave 2 / Wave 3 / Wave 4 Implementation Plan

> **Generated:** 2026-08-25  
> **Branch:** copilot/select-core-modules-gaps  
> **Method:** Automated gap scanner (Phase 5 verified) + subagent semantic analysis  
> **Scope:** Core modules in src/ — prioritized by CRITICAL/HIGH count and real source-code gaps

---

## Wave 4 Module Ranking — Real Source Gaps (2026-08-25)

> Full module scan: 2026-08-25 · Subagent triage (server / auth / transaction) in progress

### Gap Scan Summary — All Core Modules

| Module | Raw CRITICAL (Scanner) | Real TODO/Stub hits | Inflation Factor | Wave 4 Priority |
|--------|------------------------|---------------------|-----------------|-----------------|
| **server** | ~158 | **127** | ~3× | 🔴 **P1** — data_race in LLM handlers, missing_audit_log, iterator_invalidation in query_api |
| **auth** | 5 | **35** | 1.4× | 🔴 **P2** — missing audit events (7 CRITICAL), OAuth retry logic, crypto weakness |
| **transaction** | 3 | **17** | 1.2× | 🟡 **P3** — saga HIGH gaps, global_txn 22 HIGH, stub #279 RPC transport |
| **sharding** | 22 | **87** | 0.25× | 🟡 **P4** — already Wave 2-D patched; consensus/version-tracking open |
| **storage** | 64 | **69** | 0.9× | ✅ Wave 3-A closed real gaps; remaining 64 are scanner FPs (confirmed) |
| **query** | 29 | **60** | 0.5× | ✅ Wave 3-B closed real gaps; remaining mostly scope_mismatch FPs |
| **index** | 24 | **43** | 0.5× | ✅ Wave 3-C closed real gaps; remaining GPU FPs |
| **network** | 23 | **25** | 0.9× | ✅ Wave 3-D closed real gaps; remaining FPs |
| **core** | 9 | **12** | 0.75× | 🟢 Low priority — AdapterRegistry complete, minor edge cases |
| **cache** | 0 | **14** | — | 🟢 Low priority — hardening complete, expansion Q4 2026 |
| **replication** | 4 | **10** | 0.4× | 🟢 Low priority — verified clean per Wave 2-D |

**Inflation factor** = Raw CRITICAL ÷ Real TODO/Stub hits. Values <1 indicate the scanner found fewer CRITICAL than there are real TODO/stub markers — these modules have real implementation work to do.

---

### Wave 4 Confirmed Real Gaps — Server (direct source inspection 2026-08-25)

> scanner CRITICAL: ~158. After FP elimination: **~12 real CRITICAL + ~18 real HIGH**

| # | Gap | File | Line(s) | Severity | Evidence |
|---|-----|------|---------|----------|---------|
| S1 | `data_race` — shared LLM handler state accessed from request threads without lock | `llm_api_handler.cpp` | multiple | CRITICAL | `[&]` captures in concurrent request handlers; `plugin_mgr` shared ref without snapshot |
| S2 | `iterator_invalidation` — `query_results_` mutated while being iterated in pagination path | `query_api_handler.cpp` | ~1424, ~2161 | CRITICAL | Cycle guards added in Wave 2-A but container mutation under iteration not fixed |
| S3 | `missing_audit_log` — 12 remaining handler paths that call `authorize()` without subsequent audit event | various handlers | — | HIGH | Per ROADMAP §Wave B remaining ~12 |
| S4 | MCP stdio transport not implemented on non-Linux platforms | `mcp_server.cpp` | 2814 | HIGH | Source comment: "stdin reading not implemented" on unsupported platform |
| S5 | `model_integrity_gap` | `llm_api_handler.cpp` | 975-994 | ✅ **FP** | `ModelIntegrityVerifier::verifyModel` already called; ROADMAP item outdated |

False-Positives confirmed: `smart_ptr_misuse` on JS-string literals (`new Date()` / `new Error()`), `new_without_raii` same cause, `missing_audit_log` in `requireScope`/`requireAccess` paths (audit at line 10073-10081), `data_race` on function-local `[&]` captures (stack-local, not shared).

---

### Wave 4 Confirmed Real Gaps — Auth (direct source inspection 2026-08-25)

> scanner CRITICAL: 5. After FP elimination: **~7 real CRITICAL + ~12 real HIGH**

| # | Gap | File | Line(s) | Severity | Evidence |
|---|-----|------|---------|----------|---------|
| A1 | `missing_audit_log` — no audit event on failed authentication attempts | `auth_audit_logger.cpp` | — | CRITICAL | ROADMAP §open item; `logFailedAttempt()` exists but not called from all auth paths |
| A2 | `missing_audit_log` — key rotation event not emitted to audit channel | `jwt_key_rotation_manager.cpp` | — | CRITICAL | ROADMAP lists as open; key rotation path confirmed lacks `auditKeyRotation()` call |
| A3 | `missing_audit_log` — role/permission change not audit-logged | `auth_audit_logger.cpp` | — | CRITICAL | ROADMAP §open; emitPermissionChange() path unimplemented |
| A4 | `no_retry_logic` — OAuth timeout in `federated_identity_manager.cpp` has no backoff | `federated_identity_manager.cpp` | — | HIGH | `ldap_connection_pool.cpp` has proper retry; federated manager does not |
| A5 | `crypto_weakness` — cipher/padding validation absent in mTLS path | `mtls_authenticator.cpp` | — | HIGH | ROADMAP §open item; OpenSSL cipher list not explicitly restricted |
| A6 | `sensitive_data_logging` | `auth_audit_logger.cpp`, `password_policy.cpp` | — | ✅ **FP** | grep found no plaintext password/token in log calls; redaction already in place |

---

### Wave 4 Confirmed Real Gaps — Transaction (direct source inspection 2026-08-25)

> scanner CRITICAL: 3. After FP elimination: **~2 real CRITICAL + ~8 real HIGH**

| # | Gap | File | Line(s) | Severity | Evidence |
|---|-----|------|---------|----------|---------|
| T1 | RPC Phase-1/Phase-2 bridges (stub #279) — injectable callbacks, no real gRPC transport wired | `distributed_transaction_manager.cpp` | 67-120 | CRITICAL | `setRpcPhase1Fn`/`setRpcPhase2Fn` are valid injection points but no default gRPC impl exists; calls fail silently when not injected |
| T2 | `saga_orchestrator.cpp` — 10 HIGH scanner entries; source has 656 lines and no inline TODO markers | `saga_orchestrator.cpp` | — | HIGH | Scanner FP rate estimated high; subagent triage pending |
| T3 | `global_transaction_manager.cpp` — 22 HIGH from scanner, source review pending | `global_transaction_manager.cpp` | — | HIGH | Scanner FP rate estimated high; subagent triage pending |
| T4 | `lock_manager.cpp` — 2 CRITICAL, 2 HIGH | `lock_manager.cpp` | — | CRITICAL | Source review pending; deadlock detection paths suspect |

---

## Wave 4 Implementation Plan

> Target branch: `develop` · Target: Q3–Q4 2026

### Wave 4-A — Server: Integrity Gate Fix + Audit Completion (P1)

> **Subagent triage 2026-08-25 · Inflation factor: ~8–10× (~158 raw CRITICAL → 15–20 real)**

**Verified Real Gaps (subagent confirmed):**

| # | File | Line(s) | Type | Severity | Fix |
|---|------|---------|------|----------|-----|
| S1 | `llm_api_handler.cpp` | 978 | `integrity_gate_bypass` | HIGH | Replace `if (!path.empty())` silent skip with HTTP 400 when `path` absent |
| S2 | `llm_api_handler.cpp` | 967–969 | `path_traversal` | HIGH | Canonicalize `path` via `weakly_canonical()` + root-escape check before `verifyModel`/`loadModel` |
| S3 | `lora_api_handler.cpp` | post-authorize | `missing_audit_log` | HIGH | Add `THEMIS_INFO("[AUDIT] …")` on ALLOW+DENY branches |
| S4 | `import_api_handler.cpp` | post-authorize | `missing_audit_log` | HIGH | Same pattern as S3 |
| S5 | ~3 small handlers | post-authorize | `missing_audit_log` | HIGH | replication_topology, postgres_session, others per header C= |
| S6 | `mcp_server.cpp` | 2814 | `unimplemented_platform` | HIGH | Add `// STUB/SIMULATION NOTE` documenting non-Linux platform gap + removal plan |

**FPs Confirmed Closed (no code change):**

| Finding | Count | Root Cause |
|---|---|---|
| `model_integrity_gap` | 10 | SHA-256 gate already at `llm_api_handler.cpp:981`; scanner re-fires on dispatch + every post-gate call |
| `iterator_invalidation` query_api_handler | 3 | Container identity confusion (`parent` vs `visited`); read-only loops |
| `data_race` local `[&]` lambdas | ~15 | Function-local variables, single-threaded dispatch; confirmed Wave-1 |
| `new_without_raii`/`smart_ptr_misuse` | 5 | JS `new Date()`/`new Error()` inside C++ string literals |
| `missing_audit_log` http_server+session | 7 | Routes through `requireScope()`/`requireAccess()` with centralised audit at lines 10073-10081 |

**Note:** `prompt_injection` (docs_assistant.cpp:678) and `deadlock_risk` (ai_orchestrator.cpp:264-289) are real CRITICAL items in `src/llm/` module — tracked in LLM ROADMAP, not server scope.

**Acceptance Criteria:**
- [ ] Empty-path model-load request rejected with HTTP 400 (S1)
- [ ] User-supplied model path blocked from path traversal via canonicalization (S2)
- [ ] audit events present on ALLOW+DENY in lora, import, and ~3 small handlers (S3–S5)
- [ ] MCP stdio stub documented per governance rules (S6)
- Regression tests: `tests/server/test_wave4a_server_hardening.cpp` (8 tests)

**Files:** `src/server/llm_api_handler.cpp`, `src/server/lora_api_handler.cpp`, `src/server/import_api_handler.cpp`, `src/server/mcp_server.cpp`, ~3 small handlers, `src/server/MODULE_GAPS.md` (update 158→~146), `src/server/ROADMAP.md`

---

### Wave 4-B — Auth: Audit Events + OAuth Retry + Crypto Hardening (P2)

> **Subagent triage 2026-08-25 · 193 claimed gaps → 14 verified real**

**Verified Real Gaps (subagent confirmed):**

| # | File | Line(s) | Type | Severity | Fix |
|---|------|---------|------|----------|-----|
| A1 | `passkey_authenticator.cpp` | 880–892 | `missing_audit_log` | CRITICAL | Inject `AuthAuditLogger*`; call `logPasskeySuccess/Failure` — zero audit calls currently |
| A2 | `mtls_authenticator.cpp` | 281 | `missing_audit_log` | CRITICAL | Inject `AuthAuditLogger*`; add `logMTLSSuccess/Failure` — no `#include "auth/auth_audit_logger.h"` in file |
| A3 | `federated_identity_manager.cpp` | 202–578 | `missing_audit_log` | CRITICAL | Add `AuthAuditLogger*` injection; call `logJWTSuccess/Failure` in `validateToken()` + `exchangeToken()` |
| A4 | `auth_audit_logger.cpp` | (absent) | `missing_event_type` | CRITICAL | Add `SecurityEventType::ROLE_CHANGED`, `PERMISSION_CHANGED`; add `logRoleChange/logPermissionChange` |
| A5 | `jwt_key_rotation_manager.cpp` | 54 | `missing_audit_log` | HIGH | try/catch around `max_keys` throw → emit `KEY_ROTATION_FAILED` event before re-throw |
| A6 | `jwt_key_rotation_manager.cpp` | 99–100 | `missing_audit_log` | HIGH | Emit `KEY_REVOCATION_FAILED` before `return false` on unknown `kid` |
| A7 | `auth_audit_logger.cpp` | (absent) | `missing_audit_method` | HIGH | Add `logPasskeyRegistered(user_id, credential_id, rp_id)`; call from `registerCredential()` |
| B1 | `ldap_connection_pool.cpp` | 173–181 | `no_retry_logic` | HIGH | Add retry loop (max 3×, base 100ms, ×2, ±20ms jitter) around `createConnection()`; fall-through → PROVIDER_DEGRADED |
| B2 | `federated_identity_manager.cpp` | 390–393 | `no_retry_logic` | HIGH | Wrap `httpPost()` in retry loop; retry on `CURLE_COULDNT_CONNECT`, HTTP 429/503 |
| B3 | `oauth_pkce_flow.cpp` | 317–318 | `no_retry_logic` | HIGH | Same retry fix as B2; factor into shared retrying `httpPost()` helper |
| B4 | `oauth_device_flow.cpp` | 399–400 | `no_retry_logic` | MEDIUM | Retry HTTP transport errors within RFC poll loop (not the poll interval — RFC 8628 correct) |
| C1 | `passkey_authenticator.cpp` | 407–483 | `cose_alg_bypass` | HIGH | Add `alg` field allowlist in `coseKeyToEvpPkey()`; reject `kty=2` if `alg != -7`; reject `kty=3` if `alg != -257` |
| C2 | `mtls_authenticator.cpp` | 173–283 | `missing_eku_check` | HIGH | Add `X509_get_ext_d2i(NID_ext_key_usage)` check; reject certs lacking `id-kp-clientAuth` |
| C3 | `passkey_authenticator.cpp` | 447–482 | `rsa_keysize_floor` | MEDIUM | After EVP_PKEY build, call `EVP_PKEY_get_bits(pkey)` and reject if `< 2048` |

**FPs Confirmed Closed:** `sensitive_data_logging` (155) — scanner matched variable names near log calls, not values; `// NOPII` on ambiguous sites; no raw credential in any spdlog format arg. mTLS cipher claim is wrong file scope (no SSL_CTX in MTLSAuthenticator).

**Acceptance Criteria:**
- [ ] All 7 missing audit events implemented with regression tests (A1–A7)
- [ ] httpPost() retry helper covers federated, PKCE, device-flow (B2–B4); ldap createConnection retry (B1)
- [ ] COSE alg allowlist + EKU validation + RSA key-size floor in place (C1–C3)
- Regression tests: `tests/auth/test_wave4b_auth_hardening.cpp` (≥14 tests)

**Files:** `src/auth/passkey_authenticator.cpp`, `src/auth/mtls_authenticator.cpp`, `src/auth/federated_identity_manager.cpp`, `src/auth/auth_audit_logger.cpp`, `src/auth/jwt_key_rotation_manager.cpp`, `src/auth/ldap_connection_pool.cpp`, `src/auth/oauth_pkce_flow.cpp`, `src/auth/oauth_device_flow.cpp`, `tests/auth/test_wave4b_auth_hardening.cpp`, `src/auth/MODULE_GAPS.md`

---

### Wave 4-C — Transaction: Lock Upgrade Deadlock + GTM Phase-2 Under Lock (P3)

> **Subagent triage 2026-08-25 · 43 claimed gaps → 3 verified real HIGH + 2 MEDIUM**

**Verified Real Gaps (subagent confirmed):**

| # | File | Line(s) | Type | Severity | Fix |
|---|------|---------|------|----------|-----|
| T1 | `distributed_transaction_manager.cpp` | 67–91 | `guarded_stub` | HIGH | Add `// STUB/SIMULATION NOTE`; add PRODUCTION_REQUIREMENTS doc for mandatory transport injection |
| T2 | `lock_manager.cpp` | 258–265 | `upgrade_deadlock` | HIGH | Add mutual-upgrade cycle detection before enqueuing upgrade waiter, or wire `DeadlockPredictor` into wait path |
| T3 | `global_transaction_manager.cpp` | 248–252 | `phase2_under_global_lock` | HIGH | Apply snapshot-then-release pattern: snapshot participant list under lock → release → deliver Phase-2 → re-acquire to mark COMPLETED (mirrors DTM `runPhase1Unlocked`) |
| T4 | `lock_manager.cpp` | 530–538 | `silent_predicate_lock_drop` | MEDIUM | Add `THEMIS_WARN` + metric counter on `max_locks` capacity reject; false-positive SSI abort rate hidden |

**FPs Confirmed Closed:** LM C=2 stale metadata (iterator_invalidation FPs closed Wave-A), saga_orchestrator H=10 (Kahn's algorithm + circuit breaker FSM — correct patterns), GTM H=22 (`scope_mismatch` × 1413 + `circular_lock_ordering` FPs), DTM C=1 stale header.

**Acceptance Criteria:**
- [ ] stub #279 STUB NOTE present with transport injection requirement documented (T1)
- [ ] `upgradeLock` mutual-upgrade deadlock eliminated (T2)
- [ ] GTM `commit()`/`abort()`/`recoverInDoubt()` release global lock before Phase-2 delivery (T3)
- [ ] Predicate lock capacity-reject emits warn + metric (T4)
- Regression tests: `tests/transaction/test_wave4c_transaction_hardening.cpp`

**Files:** `src/transaction/distributed_transaction_manager.cpp`, `src/transaction/lock_manager.cpp`, `src/transaction/global_transaction_manager.cpp`, `tests/transaction/test_wave4c_transaction_hardening.cpp`, `src/transaction/MODULE_GAPS.md`, `src/transaction/ROADMAP.md`

---

## Implementation Status (2026-08-25)

### Wave 3 Status (2026-08-25 — COMPLETE)

| Track | Status | Commit-Inhalt |
|-------|--------|---------------|
| **Wave 3-A** — Storage Real Gaps | ✅ COMPLETE | columnar decode implementiert, encryptFile/compressPath fail-closed, diagnostics emit gewired, ggml nullptr-Guard; MODULE_GAPS.md 69→64 |
| **Wave 3-B** — Query Blocking + Timeout | ✅ COMPLETE | parallel_executor watchdog-wait, continuous_query timed-join, tbbWaitWithTimeout real cancellation, sequential null guard, JIT corruption sentinel; MODULE_GAPS.md 52→49 |
| **Wave 3-C** — Index GPU RAII + Iterator Safety | ✅ COMPLETE | VectorAutoBuffer `~VectorAutoBuffer() noexcept`; übrige 28 CRITICAL verifizierte FPs (pre-existing fixes bestätigt); MODULE_GAPS.md 29→28 |
| **Wave 3-D** — Network Command Injection + Deadlock | ✅ COMPLETE | command_injection RCE (`qos_manager.cpp`) → posix_spawn; health check stub (`raft_load_balancer.cpp`) → echter TCP-Probe; FD-Leak RAII; SO_SNDTIMEO POSIX-breit; Lock-Ordering doc; MODULE_GAPS.md 29→24 |

### Wave 3 Gesamtbilanz — Scanner-Inflation vs. echte Gaps

| Modul | Raw CRITICAL (Scanner) | Echte CRITICAL | Inflationsfaktor | Gefixt |
|-------|------------------------|----------------|-----------------|--------|
| storage | 69 | 2 | 34× | ✅ 2 CRITICAL + 3 HIGH |
| query | 52 | 3 | 17× | ✅ 3 CRITICAL + 2 HIGH |
| index | 29 | 1 | 29× | ✅ 1 CRITICAL (rest pre-existing fixed) |
| network | 29 | 4 | 7× | ✅ 4 CRITICAL + 4 HIGH |
| **Gesamt** | **179** | **10** | **18×** | ✅ **10 CRITICAL + 9 HIGH** |

**Hauptursachen Scanner-Inflation:**
- `scope_mismatch` auf anonyme Namespaces in `namespace themis` (valides C++) — 3.860+ Hits im query-Modul allein
- `braces_imbalance@line:1` — Scanner-Phantom vor jedem Parsing-Durchlauf
- `db_connection_leak` auf `shared_ptr`-verwaltete Verbindungen — Scanner sieht kein RAII
- `no_transit_encryption` bei SDK-verwalteter TLS (AWS SDK, Azure SDK, GCS Client)

---

### Wave 3-A Confirmed Real Gaps — Storage (2026-08-25 Subagent Triage):
Raw scanner CRITICAL count: 69 → **Verified real: 2 CRITICAL + 3 HIGH** (after false-positive triage)

| # | Gap | File | Severity | Status |
|---|-----|------|----------|--------|
| A1 | `ColumnSegment::decode()` stub — silent no-op | `columnar_format.cpp:1265` | CRITICAL | 🔄 Fixing |
| A2 | `encryptFile()` plaintext fallback returns `true` | `backup_manager.cpp:1726` | CRITICAL | 🔄 Fixing |
| A3 | `compressPath()` uncompressed fallback returns `true` | `backup_manager.cpp:1468` | HIGH | 🔄 Fixing |
| A4 | `emitDiagnosticEvent/RecoveryFault/Pressure` — TODO stubs | `storage_error_diagnostics.cpp:370,385,404` | HIGH | 🔄 Fixing |
| A5 | `asGgmlTensor()` returns fake ptr when allocfn unset | `ggml_tensor_bridge.cpp:190` | HIGH | 🔄 Fixing |

False-Positives confirmed by source inspection (no code change needed):
`db_connection_leak` (4 scanner entries — shared_ptr managed), `scope_mismatch` (anonymous ns),
`braces_imbalance@line:1` (6 phantom entries), `null_dereference` (guards in place),
`unchecked_cuda_call` (36 — THEMIS_CUDA_CHECK macro applied), `no_transit_encryption` (SDK-managed TLS),
`blocking_no_timeout` (acquire_timeout_ design), `iterator_invalidation` (2 — source-justified)

### Wave 3-C Confirmed Real Gaps — Index (Scanner-confirmed real):
| # | Gap | File | Line | Severity |
|---|-----|------|------|----------|
| C1 | exception_in_destructor | `graph_auto_buffer.cpp` | 52 | CRITICAL |
| C2 | exception_in_destructor | `vector_auto_buffer.cpp` | 66 | CRITICAL |
| C3 | gpu_memory_leak | `gpu_memory_oversubscription.cpp` | 53 | CRITICAL |
| C4 | gpu_memory_leak (×3) | `cuda_hnsw_graph_traversal.cpp` | 362,370,381 | CRITICAL |
| C5-C12 | iterator_invalidation (×8) | `vector_index.cpp:80`, `multi_vector_search.cpp:224,406`, `graph_index.cpp:244,247,248`, `edge_types.cpp:364`, `gpu_memory_oversubscription.cpp:230` | — | CRITICAL |

False-Positives: `braces_imbalance@line:1` (6 entries confirmed phantom)

### Wave 3-D Confirmed Real Gaps — Network (2026-08-25 Subagent Triage):
Raw scanner CRITICAL count: 29 → **Verified real: 4 CRITICAL + 5 HIGH** (7.25× inflation factor)

| # | Gap | File | Line | Severity | Status |
|---|-----|------|------|----------|--------|
| D1 | command_injection × 3 — `std::system()` with unsanitized `iface` | `qos_manager.cpp` | 663,672,685 | CRITICAL | 🔄 Fixing |
| D2 | `defaultHealthCheck()` always-true stub → 41 db_connection_leak downstream | `raft_load_balancer.cpp` | 425 | CRITICAL | 🔄 Fixing |
| D3 | deadlock — `connections_mutex_` ↔ `rate_limit_mutex_` ABBA | `wire_protocol_server.cpp` | 701,855 | HIGH | 🔄 Fixing |
| D4 | missing dtor + smart_ptr missing `closesocket()` deleter → FD leak | `socket_timeout_manager.cpp` | 71,202 | HIGH | 🔄 Fixing |
| D5 | `SO_SNDTIMEO` only on `__linux__` — no send timeout on macOS/FreeBSD | `service_mesh.cpp` | 175,194 | HIGH | 🔄 Fixing |

False-Positives confirmed: `braces_imbalance@line:1` (5 entries), `scope_mismatch` (1,404 stdlib/boost qualified-name hits — all FP), 5 of 7 `deadlock_risk` entries (sequential non-nested lock acquisitions)

### Wave 3-B Confirmed Real Gaps — Query (2026-08-25 Subagent Triage):
Raw scanner CRITICAL count: 52 → **Verified real: 3 CRITICAL + 6 HIGH** (84% FP — scope_mismatch dominates)

| # | Gap | File | Line | Severity | Status |
|---|-----|------|------|----------|--------|
| B1 | `(void)timeout_seconds; tg.wait()` — explicit void + infinite block | `parallel_executor.cpp` | 65 | CRITICAL | 🔄 Fixing |
| B2 | `loop_thread_.join()` in destructor — no deadline → streaming deadlock | `continuous_query_engine.cpp` | 143 | CRITICAL | 🔄 Fixing |
| B3 | `tg.wait()` inline, post-fact timeout comment — no real interrupt | `query_engine.cpp` | 4872 | CRITICAL | 🔄 Fixing |
| B4 | null_dereference — sequential fallback at :225 lacks null guard (TBB path at :238 has it) | `parallel_executor.cpp` | 225 | HIGH | 🔄 Fixing |
| B5 | `catch(...)` swallows all exceptions, masks JIT state corruption | `query_compiler.cpp` | 423 | HIGH | 🔄 Fixing |

False-Positives confirmed: `scope_mismatch` (3,860 hits — anonymous namespaces in `namespace themis`, valid C++),
`braces_imbalance@line:1` (2 phantom), `braces_imbalance_midfile` (121 — THEMIS_WARN `{}` format strings),
`db_connection_leak` in `cq_watermark.cpp` (lock-free atomics, confirmed FP), `aql_translator.cpp` 54×null (all guarded defensive returns)

---

| Track | Status | Commit |
|-------|--------|--------|
| **Wave 2-A** — Security & Data Integrity | ✅ COMPLETE | `896bc4b2` |
| **Wave 2-B** — RAII & Resource Safety | ✅ COMPLETE | `90f5b1e5` |
| **Wave 2-C** — LLM Stub Replacement / URL Security | ✅ CLOSED | `insecure_model_url` fixed; stubs #261/#262 documented (Removal: Q4 2026) |
| **Wave 2-D** — Sharding/Replication | ✅ COMPLETE | Canonical lock hierarchy block added; LKO-D1 documented |

### Wave 2-A Closure (2026-08-25):
- [x] A1: Model Integrity Gate — `include/server/model_integrity_verifier.h` + `src/server/model_integrity_verifier.cpp` + `tests/server/test_model_integrity_wave2.cpp` (6 tests)
- [x] A2: Auth Sensitive Logging Redaction — `include/auth/auth_redaction.h` + jwt_key_rotation_manager edits + `tests/auth/test_auth_sensitive_data_redaction.cpp` (5 tests)
- [x] A3: Iterator Invalidation Fix — `src/server/query_api_handler.cpp` (cycle guards at ~1424 and ~2161) + `tests/server/test_query_iterator_safety.cpp` (3 tests)

### Wave 2-B Closure (2026-08-25):
- [x] B1: GPU Memory Oversubscription RAII — `Impl::~Impl() noexcept` destructor frees all VRAM-partitions + `tests/index/test_index_gpu_oversubscription_raii.cpp` (3 tests)
- [x] B2: HNSW buildIndex cudaMalloc separation — combined `||` condition split into two explicit failure paths
- [x] B3: LLM gpu_memory_manager CUDA audit — confirmed all CUDA calls already checked (no change needed)
- [x] B4: LDAP Auth stub audit — confirmed permanent fallback block is intentional, not a stub (no change needed)

### Wave 2-C Closure (2026-08-25):
- [x] C1: `insecure_model_url` CRITICAL fixed — `validateOllamaUrl()` added to anonymous namespace in `src/llm/model_downloader.cpp`; called at entry of `pullFromOllama`, `exportOllamaModel`, `getOllamaManifest`, `listOllamaModels`
- [x] C2: Tests: `tests/llm/test_model_downloader_url_validation.cpp` — 9 test cases (URL_VAL_01..09) covering empty/file/ftp rejection, credential injection, plain-HTTP warning, localhost/HTTPS acceptance
- [x] C3: LLM stubs #261/#262 — deferred, already documented with removal target Q4 2026 (no production-code impact)

### Wave 2-D Closure (2026-08-25):
- [x] D1: Canonical lock hierarchy block added to `src/sharding/cross_shard_transaction.cpp` (before namespace body) — documents L1→L2→L3 ordering for `transactions_mutex_`, `callbacks_mutex_`, `deferred_mutex_` with constraint that L3 is never acquired under L1
- [x] D2: Replication verified clean: zero TODO/STUB/FIXME markers (`src/replication/ROADMAP.md §107`)
- [x] D3: Sharding circular_lock_ordering: LKO-01..06 tests pass; 172 remaining scanner findings are HIGH (not CRITICAL), all FP-class (single-mutex-per-function; real multi-mutex sites use documented ordering)

---

## 1. Core-First Priorisierung (Stand: 2026-08-25)

| Priority | Modul | Reale Lückenlage | Quelle |
|---|---|---|---|
| P1 | `server` | Wave 4-A offen: Integrity-Gate/Path-Validation + fehlende Audit-Events in Handlern + MCP non-Linux Stub-Dokumentation | `src/server/ROADMAP.md`, `src/server/MODULE_GAPS.md` |
| P2 | `auth` | Wave 4-B offen: Audit-Events, OAuth Retry-Backoff, mTLS/Passkey Crypto-Hardening | `src/auth/ROADMAP.md`, `src/auth/MODULE_GAPS.md` |
| P3 | `llm` | Groesstes reales Implementierungs-Backlog (~1.400 IMPL-Gaps): RAII, Stubs, Distributed/Telemetry-Wiring | `src/llm/MODULE_GAPS.md`, `src/llm/ROADMAP.md` |
| P4 | `transaction` | Wave 4-C offen: RPC transport stub #279, lock-upgrade deadlock risk, GTM phase-2 under lock | `src/transaction/MODULE_GAPS.md`, `src/transaction/ROADMAP.md` |
| P5 | `index` | Wave-B/Phase-B offen: GPU RAII, unchecked CUDA calls, ANN parity/perf gates | `src/index/MODULE_GAPS.md`, `src/index/ROADMAP.md` |

**Nicht priorisieren im naechsten Block (bereits weitgehend geschlossen/FP-dominant):**
- `storage` (Wave 3-A closed real gaps)
- `query` (Wave 3-B closure delivered; verbleibend primär benchmark/hardening track)
- `network` (Wave 3-D real CRITICAL/HIGH fixes abgeschlossen)
- `sharding` und `transaction` CRITICAL bereits geschlossen; verbleibende Arbeit = gezielte Hardening-Punkte

---

## 2. Naechste Implementierungsschritte (Core zuerst)

### Phase 1 — Security/Integrity Abschluss (P1 + P2)
- [ ] `server`: Wave 4-A Tasks S1–S6 abarbeiten und `test_wave4a_server_hardening.cpp` grün halten (Target: Q4 2026)
- [ ] `auth`: Wave 4-B Tasks A1–C3 umsetzen inkl. Auth-Audit-Erweiterungen und Retry-Helper (Target: Q4 2026)
- [ ] `auth`: AUTH-GRG-01..06 Gate-Evidence nach CI-Lauf finalisieren (Target: Q4 2026)

### Phase 2 — LLM/Index Produktionsluecken reduzieren (P3 + P5)
- [ ] `llm`: Wave 2-B/2-C Roadmap-Items fokussiert umsetzen (Scoped DB RAII, stub replacement inline_training/inference_enhanced, telemetry wiring) (Target: Q4 2026)
- [ ] `index`: Wave 2-B GPU-Hardening (CudaUniquePtr, THEMIS_CUDA_CHECK, iterator safety) + Phase-B parity/perf gates abschliessen (Target: Q4 2026)

### Phase 3 — Transaction Hardening Restpunkte (P4)
- [ ] `transaction`: Wave 4-C Tasks T1–T4 (transport injection requirements, deadlock-safe upgrades, phase-2 lock-release pattern, metrics on lock-drop) (Target: Q4 2026)

---

## 3. Akzeptanzkriterien fuer den naechsten Umsetzungsblock

- [ ] `server`/`auth` Wave-4 Regressionstests grün und offene CRITICAL-Items aus Wave 4-A/4-B auf `[x]`
- [ ] `llm` dokumentierte Stub- und RAII-Backlogpunkte reduziert (ROADMAP + MODULE_GAPS synchronisiert)
- [ ] `index` Phase-B-Gates (`test_ann_cpu_parity`, benchmark parity) mit belastbarer Evidenz aktualisiert
- [ ] `transaction` Wave 4-C Testnachweise in `tests/transaction/test_wave4c_transaction_hardening.cpp`
- [ ] Modul-ROADMAPs und `MODULE_GAPS.md` pro betroffenem Modul nach jedem Block synchron gehalten

---

## 4. Historischer Hinweis

Die bisherigen Abschnittsbloecke **"Wave 2 — Naechste Implementierungsschritte"** sind fachlich abgeschlossen (siehe Wave 2-A..2-D Closure oben) und werden nicht mehr als aktive Priorisierung genutzt.

---

## 5. Referenzen

- `src/server/ROADMAP.md`, `src/server/MODULE_GAPS.md`
- `src/auth/ROADMAP.md`, `src/auth/MODULE_GAPS.md`
- `src/llm/ROADMAP.md`, `src/llm/MODULE_GAPS.md`
- `src/transaction/ROADMAP.md`, `src/transaction/MODULE_GAPS.md`
- `src/index/ROADMAP.md`, `src/index/MODULE_GAPS.md`
- `src/TODO_ALL_CRITICAL_GAPS.md` (historischer Snapshot)
- `ROADMAP.md` (root, Wave A→D Gate Modell)
