# Acceleration Module — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-03-09 -->
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

## 2. CUDAGraphBackend — Vollständig als Stub implementiert

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/acceleration/ROADMAP.md` §"Completed ✅" / Issue #1378 ("CUDA graph capture for recurring query workloads") |
| **Erwartet** | CUDA-beschleunigte Graphalgorithmen (BFS, Shortest Path) mit CUDA-Graph-Capture |
| **Beobachtet** | `CUDAGraphBackend` in `cuda_backend.cpp` ist vollständig als Stub implementiert: `isAvailable()` gibt immer `false` zurück; `batchBFS()` und `batchShortestPath()` geben leere Vektoren zurück; kein CUDA-Code ausgeführt |
| **Evidence (geprüfte Pfade)** | `src/acceleration/cuda_backend.cpp` Zeilen 875–935: `// CUDAGraphBackend Stub Implementation`; Header-Metadaten: `Stubs: 6` |
| **ROADMAP-Status** | Bereits als `[x]` markiert (Claim: `CUDAGraphCache + batchKnnSearchWithGraph()`) — der Graph-Capture-Cache existiert für den **Vector**-Backend, aber der **Graph**-Backend ist ein Stub |
| **Issue-Titelvorschlag** | `[acceleration] Implement CUDAGraphBackend BFS and shortest-path kernels` |
| **Label-Vorschläge** | `type:feature`, `priority:medium`, `acceleration`, `status:open` |

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

## 4. OpenGLVectorBackend — Stub, keine Implementierung

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/acceleration/graphics_backends.cpp` (registriert als Backend-Option) |
| **Erwartet** | OpenGL 4.3+ Compute Shader Backend für breite Plattformkompatibilität |
| **Beobachtet** | `OpenGLVectorBackend::isAvailable()` gibt immer `false` zurück mit Kommentar `// Stub: not implemented yet`; `batchKnnSearch()` und `computeDistances()` geben leere Vektoren zurück |
| **Evidence (geprüfte Pfade)** | `src/acceleration/graphics_backends.cpp` Zeilen 1015–1085 |
| **ROADMAP-Status** | Nicht als separater ROADMAP-Eintrag geführt |
| **Issue-Titelvorschlag** | `[acceleration] Implement OpenGL Compute Shader backend` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `acceleration`, `status:open` |

---

## Zusammenfassung

| # | Feature | Quelle | Kritikalität | Status |
|---|---|---|---|---|
| 1 | CUDA ANN HNSW-Integration | ROADMAP #1369 | **Hoch** | `[~]` in progress |
| 2 | CUDAGraphBackend (BFS/Shortest-Path) | cuda_backend.cpp | Mittel | Stub |
| 3 | DirectXVectorBackend | graphics_backends.cpp | Niedrig | Stub |
| 4 | OpenGLVectorBackend | graphics_backends.cpp | Niedrig | Stub |

*Alle anderen ROADMAP-Einträge (#1366–#1403, außer #1369) sind durch vorhandene
Implementierungsdateien auf `develop` belegt (kein HNSW-spezifischer Pfad ausgenommen).*
