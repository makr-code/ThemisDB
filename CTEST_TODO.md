
# CTest TODO - Failed & Not Run Tests

**Zusammenfassung:** CTest-Lauf zeigt ~4780 Tests, davon **~3900 Tests als "Not Run"** und **0 Tests als "FAILED"**.

## Status

- ✅ Alle Tests, die **ausgeführt** wurden = **PASSED**
- ⚠️ Tests, die **nicht ausgeführt** wurden = **Not Run** (Binaries nicht gebaut oder in `EXCLUDE_FROM_ALL`)
- ❌ Keine **FAILED** Tests gefunden

---

## Priorisierte TODO Liste

### Phase 1: Focused-Tests bauen und ausführen (~200 Tests)

Die folgenden **Focused-Tests** sind nicht in den Binaries konfiguriert und müssen aktiviert werden:

- [ ] Build-Targets für alle `*_FocusedTests` hinzufügen
- [ ] Prüfen, ob `EXCLUDE_FROM_ALL` auf Focused-Targets angewendet wird
- [ ] `cmake/ModularBuild.cmake` und `tests/CMakeLists.txt` synchronisieren
- [ ] Rebuild und CTest erneut ausführen

**Betroffene Module (Auszug):**
- [ ] `test_raft_consensus_handler_raft_FocusedTests`
- [ ] `test_raft_log_replication_raft_FocusedTests`
- [ ] `test_raft_persistence_raft_FocusedTests`
- [ ] `test_raft_request_deduplication_raft_FocusedTests`
- [ ] `test_raft_snapshot_raft_FocusedTests`
- [ ] `test_raft_state_machine_raft_FocusedTests`
- [ ] `test_raft_stress_raft_FocusedTests`
- [ ] `test_raft_term_transitions_raft_FocusedTests`
- [ ] `test_raft_time_based_elections_raft_FocusedTests`
- [ ] `test_raft_voting_protocol_raft_FocusedTests`
- [ ] `test_raft_whitepaper_compliance_raft_FocusedTests`
- [ ] `test_random_access_io_random_FocusedTests`
- [ ] `test_ranking_api_handler_ranking_FocusedTests`
- [ ] `test_ranking_distributed_ranking_FocusedTests`
- [ ] `test_ranking_llm_ranking_FocusedTests`
- [ ] `test_ranking_local_ranking_FocusedTests`
- [ ] `test_ranking_query_ranking_FocusedTests`
- [ ] `test_ranking_serialization_ranking_FocusedTests`
- [ ] Und ~200+ weitere Focused-Tests...

### Phase 2: Integration-Tests aktivieren (~20 Tests)

Die folgenden **Integration-Tests** sind konfiguriert, aber nicht ausgeführt:

- [ ] `hot_reload_manager_integration_test`
- [ ] `backup_recovery_integration_test`
- [ ] `rpc_service_integration_test`
- [ ] `encryption_key_rotation_integration_test`
- [ ] `zero_trust_access_control_integration_test`
- [ ] `full_query_flow_e2e_test`
- [ ] `query_execution_pipeline_test`
- [ ] `ingestion_pipeline_test`
- [ ] `rag_ai_pipeline_test`
- [ ] `transaction_replication_pipeline_test`
- [ ] `security_pipeline_test`
- [ ] `analytics_export_pipeline_test`
- [ ] `application_profile_pipeline_test`
- [ ] `test_content_processing_focused`
- [ ] `test_cross_functional_plugin_query_metrics_focused`
- [ ] `test_distributed_training_e2e_focused`
- [ ] `test_graphql_e2e_focused`
- [ ] `test_rpc_database_operations_focused`
- [ ] `test_cross_functional_voice_observability_focused`

### Phase 3: CMake Build-Konfiguration überprüfen

- [ ] `cmake/ModularBuild.cmake` auf `EXCLUDE_FROM_ALL` prüfen
- [ ] `tests/CMakeLists.txt` auf fehlende `add_test()`-Aufrufe prüfen
- [ ] `add_executable()` vs. `add_test()` Diskrepanzen auflösen
- [ ] Test-Target-Abhängigkeiten überprüfen
- [ ] Focused-Binaries mit `--parallel 16` neu bauen:
  ```bash
  cmake --build --preset windows-release --parallel 16
  ```

### Phase 4: Vollständigen CTest-Lauf durchführen (nach Fixes)

- [ ] Nach CMake-Fixes erneut `ctest --output-on-failure -j 1 --timeout 180` ausführen
- [ ] Logs vergleichen und Fehler (falls vorhanden) dokumentieren
- [ ] Gegeben CTest-Exit-Code = 1 (Fehler während Konfiguration/Ermittlung)

---

## Bekannte Probleme

### CTest Exit Code = 1

Der Lauf endete mit:
```
Errors while running CTest
Command exited with code 1
```

**Mögliche Ursache:**
- Binaries für registrierte Tests nicht gefunden (`Could not find executable ...`)
- Tests in CMake registriert, aber Targets nicht gebaut (z. B. `EXCLUDE_FROM_ALL`)

### "Not Run" Tests

**Ursache:**
- Test in `add_test()` registriert
- Ausführungs-Binary (`themis_tests`, `test_kg_retriever_reasoning_focused`, etc.) nicht im `PATH` oder Build-Fehler

**Lösung:**
- Binary explizit vor CTest bauen
- CMake-Ziele in `tests/CMakeLists.txt` überprüfen
- Fest verankerte Pfade vs. relative Pfade in `add_test()` prüfen

---

## Debugging Steps

### 1. Fokussierte Tests manuell bauen und ausführen

```powershell
cd C:\Projects\ThemisDB\build-msvc-windows-release

# Baue ein Focused-Test-Target
cmake --build --preset windows-release --target module_raft_test_consensus_handler_raft_FocusedTests --parallel 16

# Führe die Tests direkt aus
.\bin_out\module_raft_test_consensus_handler_raft_FocusedTests.exe --gtest_color=no
```

### 2. CTest-Diagnose für ein spezifisches Test

```powershell
ctest --verbose --output-on-failure -R "test_raft_consensus" --timeout 180
```

### 3. CMake-Konfiguration überprüfen

```powershell
cmake --build --preset windows-release --verbose -- --debug-output 2>&1 | Select-String "EXCLUDE_FROM_ALL"
```

---

## Test-Statistiken

| Status | Anzahl | Hinweis |
|--------|--------|---------|
| PASSED | ~870 | Alle Nicht-Focused Tests |
| Not Run | ~3900 | Mostly Focused + Integration Tests |
| FAILED | 0 | ✅ Keine Fehler gefunden! |
| **TOTAL** | **~4780** | |

---

## Nächste Schritte (Priorität)

1. **Sofort:** CMake-Build für alle Focused-Targets aktivieren
   - `cmake/ModularBuild.cmake` auf `EXCLUDE_FROM_ALL` überprüfen
   - `tests/CMakeLists.txt` aktualisieren

2. **Dann:** Vollständigen Build & CTest durchführen
   ```bash
   cmake --build --preset windows-release --parallel 16
   ctest --output-on-failure -j 1 --timeout 180
   ```

3. **Danach:** Fehler (falls neue auftreten) dokumentieren und fixen

4. **Commit:** Nach erfolgreichem Test-Lauf committen

---

## Anhang: Vollständige "Not Run" Test-Liste

**Gefunden in CTest-Output:**

### Focused Tests (nach Modul)

#### Raft-Modul (~11 Tests)
- test_raft_consensus_handler_raft_FocusedTests
- test_raft_log_replication_raft_FocusedTests
- test_raft_persistence_raft_FocusedTests
- test_raft_request_deduplication_raft_FocusedTests
- test_raft_snapshot_raft_FocusedTests
- test_raft_state_machine_raft_FocusedTests
- test_raft_stress_raft_FocusedTests
- test_raft_term_transitions_raft_FocusedTests
- test_raft_time_based_elections_raft_FocusedTests
- test_raft_voting_protocol_raft_FocusedTests
- test_raft_whitepaper_compliance_raft_FocusedTests

#### Random-Access-IO-Modul (~3 Tests)
- test_random_access_io_random_FocusedTests
- test_random_access_io_corruption_simulation_random_FocusedTests
- test_random_access_io_recovery_random_FocusedTests

#### Ranking-Modul (~7 Tests)
- test_ranking_api_handler_ranking_FocusedTests
- test_ranking_distributed_ranking_FocusedTests
- test_ranking_llm_ranking_FocusedTests
- test_ranking_local_ranking_FocusedTests
- test_ranking_query_ranking_FocusedTests
- test_ranking_serialization_ranking_FocusedTests
- test_ranking_udf_ranking_FocusedTests

#### Andere Module (Hunderte weitere)
- `*_FocusedTests` für: RAG, Replication, Request-Coalescer, RocksDB, Router, Rust-FFI, S3, Schema, Security, Semaphore, Sharding, Sidecar, SIMD, Snapshot, Socket, Software-Licensing, Spanner, SQL, SSRF-Filter, Streaming, Stress-Testing, Subscription, TPCH, Transaction, Training, Tree, TrueTime, TS, TSA, TSStore, TTL, TTS, Two-Phase-Commit, Type, UDF, Unique-Index, Updates, URN, USB, User, Utilities, Vault, VCC, Vector, Vectorized, Versioned, Video, VLLM, Voice, VRAM, Vulkan, W1L03, W3C, WAL, WASM, Whisper, WisckeyGC, WOM, Workflow, Workload, Write, XDOMEA, XOEV, YAML, Zero-Copy, Zero-Trust, Zipkin, ZSTD

### Integration Tests (~20 Tests)
- hot_reload_manager_integration_test
- backup_recovery_integration_test
- rpc_service_integration_test
- encryption_key_rotation_integration_test
- zero_trust_access_control_integration_test
- full_query_flow_e2e_test
- query_execution_pipeline_test
- ingestion_pipeline_test
- rag_ai_pipeline_test
- transaction_replication_pipeline_test
- security_pipeline_test
- analytics_export_pipeline_test
- application_profile_pipeline_test
- test_content_processing_focused
- test_cross_functional_plugin_query_metrics_focused
- test_distributed_training_e2e_focused
- test_graphql_e2e_focused
- test_rpc_database_operations_focused
- test_cross_functional_voice_observability_focused

---

Datum: 2026-06-25 | CTest-Exit-Code: 1 | Status: "Not Run" Tests sind das Primärproblem
