---
Author: "ThemisDB Team"
Created: "2026-11-21"
Last Updated: "2026-11-21"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "makr-code"
---

# Tensor Index & Distributed Tensor — Integrationsplan

Exact model backend not verifiable from final document text alone.

Target branch: `develop`.

## Phase A — TensorIndex Hardening (kein API-Bruch)

- [x] A1 `FlatTensorIndex::load()`: Obergrenzen für `n`, `nm`, `nc`, `nf`; `nf == rl*nn*rr` prüfen; `id`-Read prüfen; `bad_alloc` abfangen.
- [x] A2 `TensorIndexManager::createIndex()`: `max_rank`/`epsilon` an den Index durchreichen (Konstruktorparameter `FlatTensorIndex`), API-Defaultwerte erhalten.
- [x] A3 Concurrency-Gate: Check+Increment atomar via `compare_exchange_weak` statt load/fetch_add.
- [~] A4 Tests in `tests/tensor/test_tensor_index_manager.cpp`: korrupte/übergroße `.ttidx`, Parameterweitergabe, Gate-Limit.
  - Umgesetzt: TFI07–TFI10, TIM27–TIM28. Gate-Limit-Test entfallen (nicht deterministisch testbar).
  - Offen: Build + `ctest -R tensor_index_manager` (auf Wunsch nicht ausgeführt).

## Phase B — Server-Integration Ingestion

- [x] B1 `main_server.cpp`: Bei `tensor_index`-Config eine `IngestionToolbox` mit beiden Live-Bridges erzeugen und in `ToolboxRegistry` initialisieren.
- [x] B2 Irreführenden Kommentar zum Null-Backend in `main_server.cpp` korrigieren.
- [~] B3 Test `TIB21_ToolboxFactoryUsesLiveTensorBridges`: live decomposition erzeugt TT-Cores und der konfigurierte Sink schreibt sie; Build/CTest ausstehend.

## Phase C — Distributed Tensor Konsolidierung (ODR)

- [x] C1 Kanonische API festgelegt: `include/distributed_tensor/*` ist öffentlich; die interne Variante wird in `detail::` konsolidiert (Tests in `tests/distributed_tensor`, `tests/epic3_distributed_tensor`).
- [x] C2 Kollidierende Typen (`ArtifactManifest`, `ArtifactClass`, `DistributedTensorPlanner`, `RoutingSummary`, `TensorRetrievalPlan`, `FragmentLoadRequest/Result`, `IFragmentFetcher`, `Config`) auflösen: interne Variante in `themis::distributed_tensor::detail` verschoben; Regressionstest prüft die Trennung von öffentlicher und interner API. Build/CTest auf Wunsch weiterhin nicht ausgeführt.
- [~] C3 `src/distributed_tensor/CMakeLists.txt`: Lifecycle-, Fallback-, Staleness-, Recovery- und Lock-Implementierungen dem Modulziel zugeordnet; aktive Integritätsengine auf OpenSSL EVP SHA-256 mit Fail-Closed-Verhalten und versionierten Receipts umgestellt. Die separate Integritätsimplementierung (`src/distributed_tensor/src/integrity_verification.cc`, disjunkte Typen `ReceiptChain`/`MerkleProof`/…) ist als interne, nicht-produktive Test-/Benchmark-Suite festgelegt und in `src/distributed_tensor/src/README.md` dokumentiert; Promotion nur mit API-Review und Build-/Testevidenz. `ArtifactManifest::compute_manifest_hash()` ist als nicht-kryptografische Prüfsumme dokumentiert. Build/CTest auf Wunsch weiterhin nicht ausgeführt.
- [x] C4 Interne Planner-Quelle aus der Hauptbibliothek entfernt und dem `themis_distributed_tensor`-Ziel zugeordnet; der EPIC-3-Plannertest linkt das Modulziel. Build/CTest auf Wunsch weiterhin nicht ausgeführt.

## Phase D — Build-Hygiene

- [~] D1 Doppelt kompilierte `src/tensor/*.cpp` teilweise aufgelöst: `src/tensor/CMakeLists.txt` entfernt Quellen, die bereits in statischen Besitzern oder exportfähigen Shared-Besitzern liegen. Shared-Erkennung: Windows `WINDOWS_EXPORT_ALL_SYMBOLS`, andere Plattformen nicht-versteckte C++-Sichtbarkeit. Die Besitzer werden privat gelinkt; core/storage verlinken nicht zurück auf `themis_tensor`. `tests/tensor/CMakeLists.txt` filtert test-spezifische Direktquellen gegen dieselben Besitzer und bindet `tensor_compaction_filter.cpp` gezielt in `test_tensor_recompress` ein. `themis_tensor` und `module_tensor_test_tensor_recompress_focused` wurden mit CMake Tools erfolgreich gebaut. Offene Punkte: nicht-exportierende Shared-Konfigurationen und erfolgreiche CTest-Ausführung; CTest-Toolaufrufe endeten weiterhin mit „Fehler beim Buildvorgang“.
- [~] D2 `themis_tensor` unabhängig von `THEMIS_BUILD_ENTERPRISE_PLUGINS` registriert und vor Tests/Benchmarks angelegt, damit deren `TARGET themis_tensor`-Prüfung das Modul verwendet. Konfiguration/Build/CTest auf Wunsch nicht ausgeführt.

## Phase E — API-Exposition (optional, Entscheidung erforderlich)

- [ ] E1 HTTP-Endpunkte `/api/v1/tensor/indexes` (create/list/drop/add/search) mit Tenant-Auth.
- [ ] E2 Distributed-Planer an Sharding-Router anbinden.

## Abnahme

- Focused CTest: `ctest -R "tensor" -E "^bench_"` grün in `build-msvc-windows-bench`.
- Keine doppelten Typdefinitionen im Namespace `themis::distributed_tensor`.
- Doku-Sync: `src/tensor/ROADMAP.md`, `src/distributed_tensor/ROADMAP.md`, `MODULE_GAPS.md`.
