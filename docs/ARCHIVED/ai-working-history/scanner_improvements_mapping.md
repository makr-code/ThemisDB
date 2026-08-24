# Scanner-Verbesserungen — Stage-Mapping

## 27 Verbesserungen → Scanner-Stages

| # | Verbesserung | Aktuelle Stage | Neue Filter | Priorität |
|---|---|---|---|---|
| 1 | `resource_leaked_in_exception` (1141) | gs3_step01_classic_memory.py | RAII-Pattern-Whitelist (unique_ptr, RAII-Wrapper) | HIGH |
| 2 | `thread_join_no_timeout` (678) | gs3_step01_thread_safety.py | join() von data_race ausschließen | HIGH |
| 3 | `legacy_or_compat_path` (473) | gs3_step03_legacy_duplication.py | Nur @deprecated-Tag-Funktionen auslösen | HIGH |
| 4 | `no_key_rotation` (464) | gs3_step03_key_failure.py | `std::string key` nur in Crypto-Kontexten | HIGH |
| 5 | `uninitialized_access` (337) | gs3_step02_uninitialized.py | Value-initialized-vor-use prüfen | HIGH |
| 6 | `explicit_delete` (288) | gs3_step01_classic_memory.py | RAII-Destruktoren-Pattern erkennen | HIGH |
| 7 | `data_race` (203) | gs3_step01_thread_safety.py | Local-lambda-ohne-captures ausschließen; lock_guard-scope prüfen | CRITICAL |
| 8 | `no_transit_encryption` (201) | gs3_step03_encryption_leak.py | TLS-verify-Opts whitelist | CRITICAL |
| 9 | `unspecified_consistency` (195) | gs3_step04_distributed_consistency.py | tests/** ausschließen; nur DB-APIs | HIGH |
| 10 | `range_temporary` (187) | gs3_step02_exception_safety.py | Moderne C++17 lifetime-semantik zulassen | MEDIUM |
| 11 | `db_connection_leak` (171) | gs3_step01_classic_memory.py | GPU-Memory-APIs ausschließen | HIGH |
| 12 | `o_n_squared` (159) | gs3_step04_performance_patterns.py | Nur Hot-Path-evidenz akzeptieren | MEDIUM |
| 13 | `missing_trace_point` (149) | gs3_step04_observability.py | benchmarks/**, tests/** ausschließen | HIGH |
| 14 | `no_rest_encryption` (144) | gs3_step03_encryption_leak.py | REST-Endpoints von Storage-Vars unterscheiden | MEDIUM |
| 15 | `delete_without_nullptr` (144) | gs3_step01_classic_memory.py | nur Raw-Pointer-Destruktoren | MEDIUM |
| 16 | `pointer_arithmetic_unbounded` (140) | gs3_step02_memory_safety.py | std::string, nlohmann::json ausschließen | HIGH |
| 17 | `nested_loop_find` (140) | gs3_step04_performance_patterns.py | Profiling-evidenz erforderlich | MEDIUM |
| 18 | `sensitive_data_logging` (130) | gs3_step03_data_leak.py | Test-Kontext und Krypto-Keyword-Präzisierung | MEDIUM |
| 19 | `sql_injection` (123) | gs3_step03_attack_vectors.py | RocksDB→SQL-Injection deaktivieren für non-SQL | HIGH |
| 20 | `no_timeout` (121) | gs3_step04_design_error_rules.py | Blocking-by-design (thread::join) ausschließen | MEDIUM |
| 21 | `test_missing_audit_log` (87) | gs3_step04_audit_logging.py | tests/** auf INFO/LOW herabstufen | HIGH |
| 22 | `test_missing_consensus` (71) | gs3_step04_distributed_consistency.py | tests/** ausschließen | HIGH |
| 23 | `test_missing_version_tracking` (63) | gs3_step04_determinism.py | tests/** ausschließen; CRDT-Unit-Tests OK | HIGH |
| 24 | `use_after_free_gpu` (?) | gs3_step03_key_failure.py | nur tatsächliche Zugriffe nach free | MEDIUM |
| 25 | `unchecked_cuda_call` (?) | gs3_step01_classic_memory.py | nur echte Aufrufe, nicht Kommentare | HIGH |
| 26 | `unapproved_algorithm` / `deprecated_cipher` (?) | gs3_step03_attack_vectors.py | nur Crypto-API-Symbole, nicht Text | MEDIUM |
| 27 | Miscellaneous patterns | — | Siehe einzelne Stages | LOW |

---

## Implementierungs-Reihenfolge (Abhängigkeiten)

### Phase 1: Memory & Concurrency (CRITICAL)
- [ ] Stage 01: `classic_memory.py` → Whitelist RAII-Pattern (1,6,11,15,25)
- [ ] Stage 01: `thread_safety.py` → lock_guard-scope + local-lambda filter (2,7)

### Phase 2: Encryption & Security (CRITICAL)
- [ ] Stage 03: `encryption_leak.py` → TLS-verify-opts whitelist + REST/Storage distinction (8,14)
- [ ] Stage 03: `attack_vectors.py` → SQL-Injection für RocksDB deaktivieren (19,26)

### Phase 3: Test-Code-Ausschlüsse (HIGH)
- [ ] Stage 04: `audit_logging.py` → tests/** auf INFO herabstufen (21)
- [ ] Stage 04: `distributed_consistency.py` → tests/** ausschließen (9,22)
- [ ] Stage 04: `determinism.py` → CRDT-Unit-Tests whitelist (23)
- [ ] Stage 04: `observability.py` → benchmarks/**, tests/** ausschließen (13)

### Phase 4: Type & Initialization (HIGH)
- [ ] Stage 02: `uninitialized.py` → Value-initialized-check (5)
- [ ] Stage 02: `memory_safety.py` → std::string/json safe-APIs ausschließen (16)

### Phase 5: Crypto & Keys (HIGH)
- [ ] Stage 03: `legacy_duplication.py` → @deprecated-Tag erforderlich (3)
- [ ] Stage 03: `key_failure.py` → std::string key nur Crypto-Kontext (4,24)

### Phase 6: Performance & Design (MEDIUM)
- [ ] Stage 04: `performance_patterns.py` → Hot-Path-Profiling erforderlich (12,17)
- [ ] Stage 04: `design_error_rules.py` → Blocking-by-design (thread::join) (20)
- [ ] Stage 02: `exception_safety.py` → C++17 lifetime-semantik (10)
- [ ] Stage 03: `data_leak.py` → Test-Kontext-Filter (18)

---

## Geschätzte Auswirkung

**Vor Verbesserungen:** ~8,334 Findings
**Nach Phase 1-2:** ~4,000 FPs eliminiert (50%)
**Nach Phase 1-3:** ~6,045 FPs eliminiert (72%)
**Nach Phase 1-6:** ~6,500+ FPs eliminiert (78%)

**Verbleibend:** ~1,500-2,000 echte + legitime Hinweise
