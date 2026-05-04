# Acceleration Module — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-04-06 -->
<!-- Primärdokumentation: ../../../src/acceleration/ -->

Dieser Report dokumentiert Funktionen, die in `src/acceleration/ROADMAP.md` oder anderen
Primary-Docs als implementiert oder geplant beschrieben werden, jedoch bei der
Reality-Check-Prüfung als **nicht vollständig umgesetzt** befunden wurden.

Prüfstand: 2026-03-09 | Branch: `develop`

---

## 1. CUDA ANN — End-to-End-Suche nicht vollständig verdrahtet

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/acceleration/ROADMAP.md` §"Short-term (Next 3-6 months)" (Issue #1369) |
| **Erwartet** | CUDA-beschleunigte Approximate-Nearest-Neighbor-Suche (HNSW-basiert) als primärer Suchpfad |
| **Beobachtet** | CUDA-Kernels vorhanden (`cuda/ann_kernels.cu`, `cuda/vector_kernels.cu`, 0 Stubs im Kernel-Code); HNSW-Graph-Traversal jedoch **nicht verdrahtet** — ANN-Anfragen fallen aktuell auf die CPU zurück |
| **Evidence (geprüfte Pfade)** | `src/acceleration/cuda/ann_kernels.cu` ✅, `src/acceleration/cuda/vector_kernels.cu` ✅, `src/acceleration/ROADMAP.md` §"Known Issues" ("CUDA ANN backends are still in progress; ANN vector operations fall through to CPU pending full HNSW integration") |
| **ROADMAP-Status** | `[~]` in progress (Issue: #1369, geschlossen 2026-02-23 ohne Merge-PR-Evidence) |
| **Issue-Titelvorschlag** | `[acceleration] Wire HNSW graph traversal into CUDAVectorBackend ANN search path` |
| **Label-Vorschläge** | `type:feature`, `priority:high`, `acceleration`, `status:open` |

---

## 2. CUDAGraphBackend — ✅ Implementiert (März 2026)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/acceleration/ROADMAP.md` §"Completed ✅" / Issue #1378 ("CUDA graph capture for recurring query workloads") |
| **Erwartet** | CUDA-beschleunigte Graphalgorithmen (BFS, Shortest Path) mit CUDA-Graph-Capture |
| **Status** | **Implementiert** — `CUDAGraphBackend` in `cuda_backend.cpp` ist vollständig implementiert: `isAvailable()` prüft CUDA-Gerät; `batchBFS()` und `batchShortestPath()` mit CUDA Graph Capture; CUDA-Kernels in `cuda/graph_kernels.cu` |
| **Implementierungsdetails** | `GraphBFSShape`/`CUDAGraphBFSCache` (Frontier-BFS) + `GraphSPShape`/`CUDAGraphSPCache` (Bellman-Ford) in `cuda_backend.h`/`cuda_backend.cpp`; Tests in `tests/test_acceleration.cpp` |

---

## 3. DirectXVectorBackend — Stub, keine Implementierung

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/acceleration/graphics_backends.cpp` (registriert als Backend-Option) |
| **Erwartet** | DirectX 12 Compute Shader Backend für Windows-Plattform |
| **Beobachtet** | `DirectXVectorBackend::isAvailable()` gibt immer `false` zurück mit Kommentar `// Stub: not fully implemented yet`; `batchKnnSearch()` und `computeDistances()` geben leere Vektoren zurück |
| **Evidence (geprüfte Pfade)** | `src/acceleration/graphics_backends.cpp` Zeilen 579–638 |
| **ROADMAP-Status** | Nicht als separater ROADMAP-Eintrag geführt — ist Teil des "DirectX backend support" |
| **Issue-Titelvorschlag** | `[acceleration] Implement DirectX 12 Compute Shader backend for Windows` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `acceleration`, `status:open` |

---

## 4. OpenGLVectorBackend — ✅ Implementiert (Issue #OPENGL-COMPUTE)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/acceleration/graphics_backends.cpp` |
| **Erwartet** | OpenGL 4.3+ Compute Shader Backend für breite Plattformkompatibilität |
| **Beobachtet** | Vollständig implementiert: `isAvailable()` prüft EGL/GL 4.3+-Verfügbarkeit; `initialize()` erstellt headless EGL-Kontext und kompiliert GLSL-Compute-Shader (L2/Cosine); CPU-Fallback wenn kein EGL-Treiber vorhanden; `batchKnnSearch()` und `computeDistances()` liefern korrekte Ergebnisse via GPU oder CPU-Fallback |
| **Evidence (geprüfte Pfade)** | `src/acceleration/graphics_backends.cpp` (OpenGLVectorBackendImpl PIMPL), `tests/test_opengl_backend.cpp` |
| **ROADMAP-Status** | Implementiert und getestet — Issue geschlossen |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `acceleration`, `status:closed` |

---

## Zusammenfassung

| # | Feature | Quelle | Kritikalität | Status |
|---|---|---|---|---|
| 1 | CUDA ANN HNSW-Integration | ROADMAP #1369 | **Hoch** | `[~]` in progress |
| 2 | CUDAGraphBackend (BFS/Shortest-Path) | cuda_backend.cpp | Mittel | ✅ Implementiert |
| 3 | DirectXVectorBackend | graphics_backends.cpp | Niedrig | Stub |
| 4 | OpenGLVectorBackend | graphics_backends.cpp | Niedrig | ✅ Implementiert |

*Alle anderen ROADMAP-Einträge (#1366–#1403, außer #1369) sind durch vorhandene
Implementierungsdateien auf `develop` belegt (kein HNSW-spezifischer Pfad ausgenommen).*
