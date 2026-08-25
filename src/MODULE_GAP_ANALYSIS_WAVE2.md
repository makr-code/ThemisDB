# ThemisDB — Core Module Gap Analysis & Wave 2 Implementation Plan

> **Generated:** 2026-08-25  
> **Branch:** copilot/select-core-modules-gaps  
> **Method:** Automated gap scanner (Phase 5 verified) + subagent semantic analysis  
> **Scope:** Core modules in src/ — prioritized by CRITICAL/HIGH count and real source-code gaps

## Implementation Status (2026-08-25)

| Track | Status | Commit |
|-------|--------|--------|
| **Wave 2-A** — Security & Data Integrity | ✅ COMPLETE | `896bc4b2` |
| **Wave 2-B** — RAII & Resource Safety | [~] In Progress | — |
| **Wave 2-C** — LLM Stub Replacement | [ ] Planned | — |
| **Wave 2-D** — Sharding/Replication | [ ] Partial (Replication ✅ already closed 2026-08-16) | — |

### Wave 2-A Closure (2026-08-25):
- [x] A1: Model Integrity Gate — `include/server/model_integrity_verifier.h` + `src/server/model_integrity_verifier.cpp` + `tests/server/test_model_integrity_wave2.cpp` (6 tests)
- [x] A2: Auth Sensitive Logging Redaction — `include/auth/auth_redaction.h` + jwt_key_rotation_manager edits + `tests/auth/test_auth_sensitive_data_redaction.cpp` (5 tests)
- [x] A3: Iterator Invalidation Fix — `src/server/query_api_handler.cpp` (cycle guards at ~1424 and ~2161) + `tests/server/test_query_iterator_safety.cpp` (3 tests)

---

## 1. Module Gap Priority Table

| Module | CRITICAL | HIGH | Real TODOs | Key Gap Classes |
|--------|----------|------|------------|-----------------|
| **llm** | 155 | 1,095 | 168 | db_connection_leak (192), pointer_arithmetic_unbounded (118), circular_lock_ordering (108), resource_leaked_in_exception (108) |
| **server** | ~158 | 468 | ~5 real | data_race (53), missing_audit_log (12), model_integrity_gap (10), iterator_invalidation (3), no_timeout (6) |
| **index** | 29 | 3,057 | 41 | unchecked_cuda_call (26), gpu_memory_leak (5), iterator_invalidation (12), o_n_squared (27), todo_as_productionlogic (79) |
| **auth** | 36 | 211 | 35 | sensitive_data_logging (155), no_retry_logic (22), uncaught_exception (54), crypto_weakness (9), missing_audit_log (7) |
| **storage** | 69 | 479 | 64 | null_dereference (44), circular_lock_ordering (39), db_connection_leak (23), no_transit_encryption (37), unchecked_cuda_call (36) |
| **sharding** | 0 ✅ | 795 | 86 | circular_lock_ordering (172), db_connection_leak (36), deadlock_risk (12), manual_cleanup (29), lock_contention (46) |
| **transaction** | 0 ✅ | 181 | 17 | scope_mismatch (1413 mostly FP), uninitialized_access (41), todo_as_productionlogic (34), o_n_squared (14) |
| **query** | ~? | 430 | 55 | scope_mismatch (bulk), iterator_invalidation, data_race in query handlers |
| **core** | 7 | 18 | 12 | braces_imbalance (FP), missing_dtor redis_cache (3), blocking_no_timeout (1), circular_lock_ordering (4) |
| **replication** | 0 ✅ | 194 | 10 | circular_lock_ordering (96), todo_as_productionlogic (20), no_timeout (10), pointer_arithmetic_unbounded (8) |
| **network** | 29 | 491 | 25 | (see network/MODULE_GAPS.md) |

**Legende:** ✅ = alle CRITICAL geschlossen in Wave 1 (2026-08-25)

---

## 2. Echte Source-Code-Gaps vs. Scanner-Artefakte

### Was echte Gaps sind:
- `db_connection_leak` in llm/: kein RAII für DB-Verbindungen — echte Production-Blocker
- `gpu_memory_leak` in index/: CUDA-Allokationen ohne Exception-safe Cleanup
- `unchecked_cuda_call` in index/: Kein `cudaGetLastError()` nach Kernel-Launches
- `sensitive_data_logging` in auth/: Passwörter/Keys in spdlog-Aufrufen
- `model_integrity_gap` in server/llm_api_handler: Kein SHA-256-Verifizierungs-Gate vor Model-Load
- `iterator_invalidation` in server/query_api_handler + index/: Container-Mutation während Iteration
- TODO-Stubs in llm/: `inline_training_engine.cpp` (5 Stubs), `inference_engine_enhanced.cpp` (8 Stubs)
- LDAP-Stubs in auth/: ~12 stubbed functions in `ldap_authenticator.cpp`

### Was Scanner-Artefakte sind (keine Code-Änderung nötig):
- `scope_mismatch` (Bulk in transaction, query, replication) — anonyme Namespaces in `namespace themis` — valides C++
- `braces_imbalance` at line:1 — Phantom-Findings des Scanners
- `blocking_no_timeout` bei `weak_ptr::lock()` — nicht-blockierend, Scanner-Fehlklassifikation

---

## 3. Wave 2 — Nächste Implementierungsschritte (Prioritized)

### 🔴 Wave 2-A: Security & Data Integrity (Woche 1–2)

#### A1. Model Integrity Gate — `server/llm_api_handler.cpp`
- **Gaps:** ~10 `model_integrity_gap` (CRITICAL)
- **Was implementieren:**
  - SHA-256/HMAC-Verifikation von Modelldateien vor `handleLoadModel()`
  - Allowlist-Manifest (`model_integrity_manifest.json`) laden und prüfen
  - Vergiftete/manipulierte Modelle ablehnen mit sicherem Fehler-Response
  - Audit-Log-Eintrag für jeden Model-Load-Versuch (success/failure)
- **Files:** `src/server/llm_api_handler.cpp:190,192,407+`, `include/server/model_integrity_verifier.h`
- **Tests:** `tests/server/test_model_integrity_wave2.cpp`
- **Target:** Q3 2026

#### A2. Auth Sensitive Data Logging Redaction — `auth/`
- **Gaps:** 155 `sensitive_data_logging` (HIGH), 7 `missing_audit_log` (CRITICAL)
- **Was implementieren:**
  - Redaction-Wrapper für alle spdlog-Aufrufe in `auth_audit_logger.cpp`, `password_policy.cpp`
  - Passwörter, Tokens, Keys durch `[REDACTED]` ersetzen
  - Audit-Events für: failed authentication, key rotation, token revocation
  - `jwt_key_rotation_manager.cpp`: Audit-Event bei Rotation
- **Files:** `src/auth/auth_audit_logger.cpp`, `src/auth/password_policy.cpp`, `src/auth/jwt_key_rotation_manager.cpp`
- **Tests:** `tests/auth/test_auth_sensitive_data_redaction.cpp`
- **Target:** Q3 2026

#### A3. Iterator Invalidation Fix — `server/query_api_handler.cpp`
- **Gaps:** ~3 `iterator_invalidation` (CRITICAL)
- **Was implementieren:**
  - Snapshot-Pattern: Keys in `std::vector` sammeln vor Container-Mutation
  - `parent.find()` + `parent.erase()` in `query_api_handler.cpp:1426,1959,2005`
  - Bounds-Checks für alle `.find()` Zugriffe
- **Files:** `src/server/query_api_handler.cpp`
- **Tests:** `tests/server/test_query_iterator_safety.cpp`
- **Target:** Q3 2026

---

### 🟠 Wave 2-B: RAII & Resource Safety (Woche 2–4)

#### B1. LLM DB Connection Leaks — `llm/`
- **Gaps:** 192 `db_connection_leak` (CRITICAL)
- **Was implementieren:**
  - RAII-Wrapper `class ScopedDbConnection` für alle DB-Zugriffe
  - Alle 192 Stellen auf RAII-Wrapper umstellen (Batch-Refactoring)
  - Connection Pool mit bounded size + timeout
  - Key Files: `ml_model_manager.cpp`, `lora_storage_service_themisdb.cpp`, `inference_engine_enhanced.cpp`
- **Tests:** `tests/llm/test_llm_raii_db_connections.cpp`
- **Target:** Q4 2026

#### B2. Index GPU Memory RAII — `index/`
- **Gaps:** 5 `gpu_memory_leak` (CRITICAL), 26 `unchecked_cuda_call`
- **Was implementieren:**
  - `CudaUniquePtr<T>` RAII-Wrapper mit `cudaFree()` im Destruktor
  - `THEMIS_CUDA_CHECK` Macro nach jedem Kernel-Launch (bereits in `include/storage/gpu_compression.h` — übernehmen)
  - Exception-safe Pfade in `cuda_hnsw_graph_traversal.cpp:362,370,381`
  - GPU memory leak in `gpu_memory_oversubscription.cpp:53`
- **Files:** `src/index/cuda_hnsw_graph_traversal.cpp`, `src/index/gpu_vector_index.cpp`, `src/index/gpu_memory_oversubscription.cpp`
- **Tests:** `tests/index/test_index_gpu_raii_wave2.cpp`
- **Target:** Q4 2026

#### B3. Auth LDAP Stubs → Echte Implementierung — `auth/ldap_authenticator.cpp`
- **Gaps:** ~12 Stub-Funktionen
- **Was implementieren:**
  - Connection Pool Management (bind context, size, timeout)
  - LDAP Search Pagination (kontrolled, bounded)
  - Retry-Logik mit Exponential Backoff für Bind-Fehler
  - `federated_identity_manager.cpp`: Cross-Provider State Sync
- **Files:** `src/auth/ldap_authenticator.cpp`, `src/auth/federated_identity_manager.cpp`
- **Tests:** `tests/auth/test_ldap_integration_wave2.cpp`
- **Target:** Q4 2026

---

### 🟡 Wave 2-C: LLM Stub Replacement (Woche 4–8)

#### C1. Inline Training Engine Stubs — `llm/inline_training_engine.cpp`
- **Gaps:** 5 Stubs, kein echter Training-Loop
- **Was implementieren:**
  - Echter Gradient-Update-Loop (SGD/Adam)
  - Model-Checkpoint-Persistenz in RocksDB
  - Training-Metrics (loss, perplexity, step/s)
  - Cancellation + Timeout-Support
- **Target:** Q4 2026

#### C2. Inference Engine Enhanced Stubs — `llm/inference_engine_enhanced.cpp`
- **Gaps:** 8 Stubs (speculative decode, kernel fusion)
- **Was implementieren:**
  - Speculative Decoding: Draft-Model + Verify-Step
  - CUDA Kernel Fusion für Attention-Berechnung
  - KV-Cache Eviction Policy (LRU)
- **Target:** Q4 2026

#### C3. Index GPU ANN Backend (CUDA Kernels) — `index/`
- **Gaps:** Wave B-Ziel (Q4 2026); ~800 IMPL-Gaps
- **Was implementieren:**
  - CUDA L2/Cosine/Dot-Product Kernels in `src/acceleration/cuda/cuda_hnsw_kernels.cu`
  - HIP Backend für AMD GPUs: `HIPVectorBackend::search()`
  - Buffer Lifecycle RAII für alle GPU-Allokationen im Index-Pfad
  - ThreadSanitizer-clean für Vec KNN Insert Pipeline
- **Target:** Q4 2026

---

### 🟡 Wave 2-D: Sharding & Replication Hardening (Woche 4–8)

#### D1. Sharding Circular Lock Ordering — `sharding/`
- **Gaps:** 172 `circular_lock_ordering` (HIGH) — Deadlock-Risiko in Production
- **Was implementieren:**
  - Kanonische Lock-Reihenfolge dokumentieren und erzwingen
  - `std::lock()` für Multi-Mutex-Acquires
  - Lock-Hierarchy mit `hierarchical_mutex` oder Dokumentation
- **Target:** Q4 2026

#### D2. Replication TODO-Stubs — `replication/`
- **Gaps:** 20 `todo_as_productionlogic`
- **Was implementieren:**
  - Alle 20 TODO-Stellen in `replication_manager.cpp`, `logical_replication.cpp`
  - Hauptfokus: durability callbacks, slot persistence, WAL-slot-cleanup
- **Target:** Q4 2026

---

## 4. Phase-Abhängigkeiten

```
Wave 2-A (Security) ──→ Wave 2-B (RAII/Resource) ──→ Wave 2-C (LLM Stubs)
       ↓                                                        ↓
   GA-Gate                                              Feature-Complete
       ↓
Wave 2-D (Sharding/Replication) [parallel zu B+C]
```

**GA-Blocker:** A1 (Model Integrity), A2 (Auth Sensitive Logging), A3 (Iterator Safety)

---

## 5. Acceptance Criteria pro Wave

### Wave 2-A Done-Kriterien:
- [ ] Model-Load ohne SHA-256-Verifikation schlägt fehl (Test: `test_model_integrity_wave2.cpp`)
- [ ] `grep -rn "password\|secret\|token" src/auth/ --include="*.cpp"` zeigt keine unkomprimierte Plaintext-Ausgabe in spdlog
- [ ] `tests/server/test_query_iterator_safety.cpp` — alle 3 Iterator-Invalidation-Szenarien grün

### Wave 2-B Done-Kriterien:
- [ ] `valgrind --leak-check=full` auf LLM-Tests: 0 DB-Connection-Leaks
- [ ] CUDA-Builds: `cudaGetLastError()` nach jedem Kernel-Launch in Index-Pfad
- [ ] LDAP-Authenticator: Connection Pool mit Timeout löst `LDAP_CONNECT_TIMEOUT` korrekt aus

### Wave 2-C Done-Kriterien:
- [ ] `inline_training_engine`: echter Gradient-Update mit `test_inline_training_basic.cpp` (loss sinkt über Epochs)
- [ ] GPU ANN: Phase B Gate ThreadSanitizer-clean (`test_ann_cpu_parity`)

---

## 6. Bekannte Einschränkungen

- **LLM-Module Scanner-Findings:** Die 2146 Gesamt-Findings enthalten viele Header-Kommentar-Artefakte (jede `.cpp` hat ein Standard-Gap-Summary am Anfang). Echte Stubs: ~120 Dateien mit je 1 Stub-Annotation in Zeile 7.
- **`scope_mismatch` Bulk-Findings:** 1413 in transaction, 395 in core, 1262 in replication — alle bestätigte false positives (anonyme Namespaces in `namespace themis`). Kein Code-Change nötig.
- **GPU-Backend:** Index-Wave-B ist abhängig von GPU-Verfügbarkeit in CI. CPU-Fallback bleibt aktiv.
- **LDAP-Implementierung:** Benötigt Integration-Test-Umgebung (OpenLDAP Docker-Container).

---

## 7. Referenzen

- `src/llm/MODULE_GAPS.md` — LLM Gap Details
- `src/server/MODULE_GAPS.md` — Server Gap Details (inkl. Wave 1 Closure)
- `src/index/MODULE_GAPS.md` — Index Gap Details
- `src/auth/MODULE_GAPS.md` — Auth Gap Details (Wave C Status)
- `src/storage/MODULE_GAPS.md` — Storage Gap Details (Wave 1 Closure)
- `src/sharding/MODULE_GAPS.md` — Sharding Gap Details
- `src/TODO_ALL_CRITICAL_GAPS.md` — Cross-Module Critical Summary
- `ROADMAP.md` — Root Wave A→D Gate Model
