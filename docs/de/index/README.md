# Index-Modul

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/index/README.md · ../../../include/index/README.md -->

**Stand:** 13. Mai 2026
**Version:** aktuell
**Kategorie:** Indexierung / Vektorsuche
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Index-Modul bündelt die Indexierungsinfrastruktur von ThemisDB für Vektor-, Sekundär-, Graph- und Spatial-Workloads. Es deckt sowohl Public-Header-APIs (`include/index`) als auch die produktive Kernimplementierung (`src/index`) ab.

**Primäre Quellen:** [`src/index/`](../../../src/index/) · [`include/index/`](../../../include/index/)

---

## Wichtige Entry-Points

| Bereich | Einstiegspunkt | Zweck |
|--------|----------------|------|
| Public API | [`include/index/index_manager.h`](../../../include/index/index_manager.h) | Zentrale Manager- und Factory-Schnittstelle |
| Public API | [`include/index/vector_index.h`](../../../include/index/vector_index.h) | Vektorindex-API (HNSW, Metriken, Search) |
| Public API | [`include/index/secondary_index.h`](../../../include/index/secondary_index.h) | Sekundärindizes (B-tree, Range, Composite, TTL, Geo, Full-Text) |
| Public API | [`include/index/spatial_index.h`](../../../include/index/spatial_index.h) | Räumliche Indexierung (R-tree, Radius-/BBox-Suche) |
| Public API | [`include/index/gpu_vector_index.h`](../../../include/index/gpu_vector_index.h) | GPU-Beschleunigung (Vulkan/CUDA/HIP, Fallback) |
| Implementierung | [`src/index/vector_index.cpp`](../../../src/index/vector_index.cpp) | Vektorindex-Verwaltung und ANN-Search |
| Implementierung | [`src/index/secondary_index.cpp`](../../../src/index/secondary_index.cpp) | Sekundärindex-Lifecycle und Lookup-Pfade |
| Implementierung | [`src/index/index_manager.cpp`](../../../src/index/index_manager.cpp) | Modul-Orchestrierung und Integrationspunkte |

---

## Konfiguration, Laufzeitverhalten und Grenzen

- Build-Optionen und optionale Backends sind in den Modul-READMEs dokumentiert (GPU/FAISS/CUDA/Vulkan/HIP).
- Schreibpfade für Sekundärindizes nutzen RocksDB-`WriteBatch` für atomare Daten+Index-Updates.
- GPU-Pfade sind backend- und treiberabhängig; bei nicht verfügbarem Backend ist CPU-Fallback vorgesehen.
- Bekannte Grenzen (z. B. HNSW-Delete/Rebuild, MBR-Approximation bei Polygonen, VRAM-Limits) sind in den Modul-READMEs inkl. Workarounds beschrieben.

---

## Usage & Troubleshooting

- **Usage-Snippets:** Siehe `API/Usage Examples` in [`src/index/README.md`](../../../src/index/README.md) und [`include/index/README.md`](../../../include/index/README.md).
- **Troubleshooting:** Siehe `Troubleshooting`-Abschnitte in beiden Modul-READMEs (ANN-Recall, GPU-Backend, Speicherverbrauch, Index-Konsistenz).

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/index/README.md`](../../../src/index/README.md) | Implementierungsübersicht, Kernkomponenten, Integrationspunkte, Usage |
| [`include/index/README.md`](../../../include/index/README.md) | Public Header/API-Übersicht und Header-Entry-Points |
| [`src/index/ROADMAP.md`](../../../src/index/ROADMAP.md) | Modul-Roadmap mit Phasen, Status und Readiness-Checklist |
| [`src/index/FUTURE_ENHANCEMENTS.md`](../../../src/index/FUTURE_ENHANCEMENTS.md) | Geplante Erweiterungen inkl. Constraints, Interfaces und Targets |
| [`PRIMARY_SOURCES.md`](./PRIMARY_SOURCES.md) | Vollständiger Primärquellen-Index für das Modul |

## Verwandte modulübergreifende Quellen

- [`src/ROADMAP.md`](../../../src/ROADMAP.md)
- [`src/FUTURE_ENHANCEMENTS.md`](../../../src/FUTURE_ENHANCEMENTS.md)
- [`docs/de/roadmap/index_roadmap.md`](../roadmap/index_roadmap.md)

## Installation

Das Index-Modul wird mit ThemisDB gebaut. Build-/Feature-Flags und optional aktivierbare Backends (FAISS, GPU, CUDA, Vulkan, HIP) sind in folgenden Modulquellen beschrieben:

- [`src/index/README.md`](../../../src/index/README.md)
- [`include/index/README.md`](../../../include/index/README.md)
