# Acceleration Module — Fehlende / unvollständige Implementierungen

<!-- Status: current | validated: 2026-03-09 -->
<!-- Primärdokumentation: ../../../src/acceleration/ROADMAP.md -->
<!-- Erstellt aus Reality-Check gegen Sourcecode (Stand: 2026-03-09) -->

Dieser Bericht listet Implementierungslücken auf, die beim Reality-Check des Acceleration-Moduls (`src/acceleration/`) gefunden wurden. Jeder Eintrag enthält Claim-Quelle, erwartetes vs. beobachtetes Verhalten, Code-Evidence und einen Vorschlag für Issues/Labels.

---

## 1. CUDA ANN / HNSW-Integration nicht verdrahtet

| Feld | Inhalt |
|---|---|
| **Claim-Quelle** | `src/acceleration/ROADMAP.md` § "In Progress" / Phase 2; `src/acceleration/FUTURE_ENHANCEMENTS.md` § "CUDA Kernel Completion" |
| **Erwartet** | `CUDAVectorBackend::batchSimilaritySearch()` routet Anfragen an CUDA-Kernels in `cuda/ann_kernels.cu` + `cuda/vector_kernels.cu` |
| **Beobachtet** | ANN-Abfragen fallen durch zum CPU-Fallback (Dispatcher-Eintrag nicht mit CUDA-Kernel belegt); HNSW-Graph-Traversal nicht in `CUDAVectorBackend` verdrahtet |
| **Evidence** | `src/acceleration/cuda_backend.cpp` Header: `Stubs: 6`; `CUDAGraphBackend` (Graph-Analytics-Backend) hat `isAvailable()=false`, `initialize()=false`, `batchBFS()`/`batchShortestPath()` geben `{}` zurück (Zeilen 875–932); ROADMAP Known Issues: "ANN vector operations fall through to CPU" |
| **Betroffene Dateien** | `src/acceleration/cuda_backend.cpp` (Zeilen 875–932), `src/acceleration/cuda/ann_kernels.cu`, `include/acceleration/kernel_invocation.h` |
| **Issue-Titelvorschlag** | `feat(acceleration): wire HNSW graph traversal to CUDAVectorBackend ANN kernels` |
| **Label-Vorschläge** | `acceleration`, `cuda`, `ann`, `enhancement` |

---

## 2. `CUDAGraphBackend` — Graph-Analytics-Backend ist ein Stub

| Feld | Inhalt |
|---|---|
| **Claim-Quelle** | `include/acceleration/cuda_backend.h`; `src/acceleration/ARCHITECTURE.md` § "Component Architecture" (CUDA Backend) |
| **Erwartet** | GPU-beschleunigtes Graph-Analytics (BFS, Shortest Path) via CUDA |
| **Beobachtet** | `CUDAGraphBackend::isAvailable()` gibt immer `false` zurück; `initialize()` setzt `initialized_=false`; `batchBFS()` und `batchShortestPath()` geben `{}` zurück |
| **Evidence** | `src/acceleration/cuda_backend.cpp` Zeilen 875–932: "CUDAGraphBackend Stub Implementation"; Qualitäts-Header: `Stubs: 6` |
| **Betroffene Dateien** | `src/acceleration/cuda_backend.cpp` (Zeilen 875–932) |
| **Issue-Titelvorschlag** | `feat(acceleration): implement CUDAGraphBackend BFS and ShortestPath GPU kernels` |
| **Label-Vorschläge** | `acceleration`, `cuda`, `graph`, `enhancement` |

---

## 3. `DirectXVectorBackend` — Stub (nicht implementiert)

| Feld | Inhalt |
|---|---|
| **Claim-Quelle** | `include/acceleration/graphics_backends.h`; `docs/de/acceleration/backends.md` § "Backend-Übersicht" (DirectX: 🧪 Experimentell) |
| **Erwartet** | DirectX Compute Backend für Windows (`DirectXVectorBackend`) |
| **Beobachtet** | `initialize()` gibt `false` zurück (Stub); `batchSimilaritySearch()` gibt `{}` zurück (Stub); `deviceName = "DirectX 12 (Stub)"` |
| **Evidence** | `src/acceleration/graphics_backends.cpp` Zeilen 571–638: "DirectX Vector Backend Stub"; `Stubs: 12` im Qualitäts-Header |
| **Betroffene Dateien** | `src/acceleration/graphics_backends.cpp` (Zeilen 571–638), `src/acceleration/directx/` Shader-Dateien |
| **Issue-Titelvorschlag** | `feat(acceleration): implement DirectXVectorBackend (DirectX Compute)` |
| **Label-Vorschläge** | `acceleration`, `directx`, `windows`, `enhancement` |

---

## 4. `OpenGLVectorBackend` — Stub (nicht implementiert)

| Feld | Inhalt |
|---|---|
| **Claim-Quelle** | `include/acceleration/graphics_backends.h`; öffentliche Header-Deklaration |
| **Erwartet** | OpenGL Compute Shader Backend (OpenGL 4.3+) für Systeme ohne CUDA/Vulkan |
| **Beobachtet** | `initialize()` gibt `false` zurück ("Stub: not implemented yet"); `batchSimilaritySearch()` gibt `{}` zurück; `deviceName = "OpenGL Compute (Stub)"` |
| **Evidence** | `src/acceleration/graphics_backends.cpp` Zeilen 1013–1067: "OpenGL Vector Backend Stub"; `Stubs: 12` im Qualitäts-Header |
| **Betroffene Dateien** | `src/acceleration/graphics_backends.cpp` (Zeilen 1013–1079) |
| **Issue-Titelvorschlag** | `feat(acceleration): implement OpenGLVectorBackend (OpenGL 4.3+ Compute)` |
| **Label-Vorschläge** | `acceleration`, `opengl`, `enhancement` |

---

## 5. NCCL/RCCL `ncclGroupStart`/`ncclGroupEnd` — Wiring ausstehend

| Feld | Inhalt |
|---|---|
| **Claim-Quelle** | `src/acceleration/FUTURE_ENHANCEMENTS.md` § "Multi-GPU Sharding"; `src/acceleration/ROADMAP.md` § Known Issues |
| **Erwartet** | Cross-GPU-Transfers via `ncclGroupStart` / `ncclGroupEnd` gebündelt |
| **Beobachtet** | NCCL/RCCL-Backends sind initialisiert; Group-Call-Wiring fehlt noch; aktuell werden keine echten NCCL-Collectives für Multi-GPU-Vektor-Search ausgeführt |
| **Evidence** | `src/acceleration/nccl_vector_backend.cpp` Header: `Stubs: 1`; ROADMAP Known Issues: "`ncclGroupStart`/`ncclGroupEnd` wiring deferred to v2.5+"; FUTURE_ENHANCEMENTS: `[~] ncclGroupStart/ncclGroupEnd` |
| **Betroffene Dateien** | `src/acceleration/nccl_vector_backend.cpp`, `src/acceleration/rccl_vector_backend.cpp`, `src/acceleration/multi_gpu_backend.cpp` |
| **Issue-Titelvorschlag** | `feat(acceleration): wire ncclGroupStart/ncclGroupEnd for multi-GPU fan-out KNN (v2.5+)` |
| **Label-Vorschläge** | `acceleration`, `multi-gpu`, `nccl`, `enhancement`, `deferred` |

---

## 6. HIP Top-K für k > 1024 — Heap-Selektion nicht implementiert

| Feld | Inhalt |
|---|---|
| **Claim-Quelle** | `src/acceleration/hip_backend.cpp` (TODO-Kommentar); Parität zu CUDA-Backend (`thrust::partial_sort` für k > 1024) |
| **Erwartet** | Heap-basierte oder Radix-Select-Top-K-Selektion für k > 1024 im HIP-Backend |
| **Beobachtet** | TODO-Kommentar bei Zeile 204: "For larger k, consider heap-based selection or radix select"; kein Fallback implementiert |
| **Evidence** | `src/acceleration/hip_backend.cpp` Zeilen ~204: TODO; Header: `TODOs: 1, Stubs: 1` |
| **Betroffene Dateien** | `src/acceleration/hip_backend.cpp` (Zeile 204) |
| **Issue-Titelvorschlag** | `fix(acceleration): add HIP heap-based top-K selection for k > 1024` |
| **Label-Vorschläge** | `acceleration`, `hip`, `amd`, `enhancement` |

---

## 7. Unit-Test-Abdeckung > 80 % — Noch nicht erreicht

| Feld | Inhalt |
|---|---|
| **Claim-Quelle** | `src/acceleration/ROADMAP.md` § "Production Readiness Checklist" (Issue: #1398) |
| **Erwartet** | Testabdeckung > 80 % für neuen Code |
| **Beobachtet** | Kein Coverage-Report vorhanden; Checklist-Eintrag `[I]` (offenes Issue) |
| **Evidence** | ROADMAP: `- [I] Unit tests coverage > 80% (Issue: #1398)` |
| **Betroffene Dateien** | `tests/` (Gesamtabdeckung) |
| **Issue-Titelvorschlag** | `chore(acceleration): measure and achieve >80% unit test coverage` |
| **Label-Vorschläge** | `acceleration`, `testing`, `coverage` |

---

## 8. Deterministische Tie-Breaking- und Partial-Failure-Behandlung

| Feld | Inhalt |
|---|---|
| **Claim-Quelle** | `src/acceleration/ROADMAP.md` Phase 3, Issue: #1388 |
| **Erwartet** | Konsistentes Verhalten bei identischen Distanzwerten (Tie-Breaking) und bei teilweisem Backend-Ausfall |
| **Beobachtet** | Kein deterministischer Tie-Breaking-Mechanismus implementiert; offenes Issue |
| **Evidence** | ROADMAP: `- [I] Add deterministic behavior constraints...` (Issue: #1388) |
| **Betroffene Dateien** | `include/acceleration/kernel_invocation.h`, `src/acceleration/cuda_backend.cpp` |
| **Issue-Titelvorschlag** | `feat(acceleration): add deterministic tie-breaking for top-K results` |
| **Label-Vorschläge** | `acceleration`, `determinism`, `enhancement` |

---

## 9. Abschluss-Review und API-Stabilitäts-Signoff

| Feld | Inhalt |
|---|---|
| **Claim-Quelle** | `src/acceleration/ROADMAP.md` Phase 6, Issue: #1397 |
| **Erwartet** | Formeller Production-Readiness-Review und API-Stabilitäts-Bestätigung für v1.0 |
| **Beobachtet** | Review steht noch aus; offenes Issue |
| **Evidence** | ROADMAP: `- [I] Final production-readiness review and API stability sign-off` (Issue: #1397); Breaking Changes: "GPU kernel APIs are not yet stable" |
| **Betroffene Dateien** | `src/acceleration/ROADMAP.md`, `include/acceleration/compute_backend.h` |
| **Issue-Titelvorschlag** | `chore(acceleration): final production-readiness review and v1.0 API stability sign-off` |
| **Label-Vorschläge** | `acceleration`, `api-stability`, `production-readiness` |

---

## Zusammenfassung

| # | Titel | Schwere | Status |
|---|---|---|---|
| 1 | CUDA ANN / HNSW-Integration | 🔴 Hoch | Kernels vorhanden, Wiring fehlt |
| 2 | CUDAGraphBackend (Graph-Analytics) | 🔴 Hoch | Vollständiger Stub |
| 3 | DirectXVectorBackend | 🟡 Mittel | Vollständiger Stub (Windows) |
| 4 | OpenGLVectorBackend | 🟡 Mittel | Vollständiger Stub |
| 5 | NCCL/RCCL Group-Wiring | 🟡 Mittel | Deferred v2.5+ |
| 6 | HIP Top-K k > 1024 | 🟢 Niedrig | TODO vorhanden |
| 7 | Unit-Test-Abdeckung > 80 % | 🟡 Mittel | Offenes Issue #1398 |
| 8 | Deterministische Tie-Breaking | 🟡 Mittel | Offenes Issue #1388 |
| 9 | Abschluss-Review & API-Signoff | 🟢 Niedrig | Offenes Issue #1397 |

---

## Verwandte Dokumentation

- [ROADMAP (src/acceleration)](../../../src/acceleration/ROADMAP.md)
- [FUTURE_ENHANCEMENTS (src/acceleration)](../../../src/acceleration/FUTURE_ENHANCEMENTS.md)
- [Modulübersicht (DE)](README.md)
- [Backend-Typen (DE)](backends.md)
