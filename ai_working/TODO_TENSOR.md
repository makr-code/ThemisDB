# TODO — Tensor-bezogene Implementierungen

Nur tensor-relevante offene Stubs (aus `src/STUB_INVENTORY.md`).  
Stand: 2026-05-06

---

## Governance-Verknüpfung (Root)

- Dokumenttyp: Enhancement-Backlog (tensor-spezifische offene Punkte), kein Release-Log.
- Planungsbezug: `ROADMAP.md` (Milestones/Features) und `FUTURE_ENHANCEMENTS.md` (offene Enhancements).
- Abschlussregel: Umgesetzte Punkte zusätzlich in `CHANGELOG.md` unter `[Unreleased]` mit Milestone-Bezug dokumentieren.

---

## #154 — `tensor/hnsw_tt_bridge.cpp::HnswLayer` linear-scan ersetzen
**Priorität:** Phase 2 (Q4 2026)  
**Stub-ID:** HTB-01

**Problem:**  
`HnswLayer::search()` iteriert linear über alle Sketche → O(n) Laufzeit.  
Recall ist korrekt, aber Latenz skaliert linear mit Indexgröße.

**Lösung:**  
- `HnswLayer` auf `hnswlib::HierarchicalNSW<float>` umstellen  
- CMake-Integration von hnswlib (Header-only, bereits als Sub-Modul vorhanden)  
- Sketch-Vektoren als `float*`-Punkt in HNSW einfügen  
- `search()` → `hnsw_->searchKnn(query_sketch, ef_search)` delegieren  
- Bestehende Tests HTB-01..HTB-07 müssen weiter grün sein  
- Performance-Test: ≥ 10× Speedup bei n ≥ 100 k Vektoren gegenüber Linear-Scan

**Dateien:**  
- `src/tensor/hnsw_tt_bridge.cpp`  
- `include/tensor/hnsw_tt_bridge.h`  
- `tests/test_hnsw_tt_bridge.cpp`

---

## #149 — `storage/tensor_router.cpp::runPilot()` κ-Schätzung verbessern
**Priorität:** Q3 2026 Benchmark  
**Stub-ID:** TRR-pilot

**Problem:**  
`runPilot()` schätzt den κ-Komprimierbarkeitswert über eine log-Ratio-Näherung des
Pilot-TT-Ranks — ohne echten TT-SVD-Lauf auf einem Korpus-Sample.  
Abweichung bis ±20 % bei Grenzfällen (z. B. LLM Embedding-Batches).

**Lösung:**  
- Pilot-Sample (mind. 512 Vektoren, konfigurierbar) per `TensorTrainDecomposer::truncatedSVD()` verarbeiten  
- κ = σ₁/σ_r (Singularwert-Verhältnis) als echtes Maß berechnen  
- Einheit-Tests: κ soll für synthetische rankarme Matrizen < 5 % Fehler aufweisen  
- `runPilot()` optional asynchron (Thread-Pool), um Block des Ingestion-Pfads zu vermeiden

**Dateien:**  
- `src/storage/tensor_router.cpp`  
- `include/storage/tensor_router.h`  
- `tests/test_tensor_router.cpp` (neue Tests TR-pilot-01..TR-pilot-04)

---

## #152 — `tensor/tensor_index_manager.cpp::ggmlCorePtrs()` mmap-Bridge
**Priorität:** Phase 3 (Q1 2027)  
**Stub-ID:** TIM-01

**Problem:**  
`ggmlCorePtrs()` gibt nackte `float*`-Zeiger zurück, ohne:
- mmap-Pin (Pointer kann nach Index-Reload ungültig werden)
- GGML-Typ-Registrierung (`GGML_TYPE_TT`)
- Lifetime-Fence (kein Destruktor-Callback für GGML-Tensor)

**Lösung:**  
- GGML `ggml_type` `GGML_TYPE_TT` registrieren (ggml.h Extension-API)  
- `ggml_tensor`-Wrapper mit `ggml_new_tensor_raw()` anlegen, der die TT-Core-Daten pinnt  
- Returned `ggml_tensor*` hält Referenz auf `FlatTensorIndex`-Entry → kein dangling pointer  
- Tests: TIM-mmap-01..TIM-mmap-03 (Lifetime, Daten-Integrität nach Index-Reload)

**Dateien:**  
- `src/tensor/tensor_index_manager.cpp`  
- `include/tensor/tensor_index_manager.h`  
- `tests/test_tensor_index_manager.cpp`

---

## #158 — `ingestion/inference_backend.h::NullTensorDecompositionBackend` verdrahten
**Priorität:** Server-Bootstrap (kein fixer Termin)  
**Stub-ID:** TIB-null  
**Hinweis:** Diese Klasse bleibt als Default-Fallback erhalten; sie wird **nicht** entfernt.

**Problem:**  
`ChunkTtDecomposeStep` erhält standardmäßig `NullTensorDecompositionBackend`  
→ `decompose()` gibt leeren `TensorCoreRecord` zurück  
→ TT-Core-Generierung übersprungen, keine Fehlermeldung

**Lösung:**  
- Im Server-Bootstrap (`main_server.cpp`) eine echte `TensorIngestionBridge`-Instanz erzeugen  
- Mit `ChunkTtDecomposeStep::setBackend(std::make_shared<TensorIngestionBridge>(...))` injizieren  
- Konfigurierbar per `themis.yaml` (`tensor.ingestion.enabled: true`)  
- Smoke-Test: nach Bootstrap `NullTensorDecompositionBackend::shouldDecompose()` darf nicht mehr `false` zurückgeben

**Dateien:**  
- `src/main_server.cpp`  
- `include/ingestion/inference_backend.h`  
- `src/tensor/tensor_ingestion_bridge.cpp`

---

## Abgeschlossene Tensor-Stubs (Referenz)

| Stub | Beschreibung | Abgeschlossen |
|------|-------------|---------------|
| #157 | `simpleSVD()` Golub-Reinsch SVD | 2026-05-05 |
| #161/162 | `TensorDeduplicationManager::retrieve()` | 2026-05-05 |
| #148/160 | `RocksDBTensorBackend` put/get/del/listKeys | 2026-05-06 |
| #150/151 | `FlatTensorIndex::save()/load()` Binär-Persistenz | 2026-05-06 |
| #153 | `TensorIndexManager::dropTenantIndexes()` RocksDB | 2026-05-06 |
| #155/156 | `HnswTTBridge::save()/load()` Binär-Persistenz | 2026-05-06 |
| #159 | `shouldDecompose()` Rademacher-Projektion | 2026-05-06 |
| #163 | `TensorRouter::Route/DataProfile/decide()` API | 2026-05-06 |
| #147 | `AdaLoraTTBridge::findSimilarAdapters()` | 2026-05-05 |
