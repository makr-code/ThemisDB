[docs](../INDEX.md) > [de](../INDEX.md) > [stable_diffusion](./index.md) > [index](./index.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/stable_diffusion/README.md`
- `src/stable_diffusion/ARCHITECTURE.md`
- `src/stable_diffusion/ROADMAP.md`
- `src/stable_diffusion/FUTURE_ENHANCEMENTS.md`
- `src/stable_diffusion/CHANGELOG.md`
- `src/stable_diffusion/sd_plugin.cpp`
- `include/stable_diffusion/sd_plugin.h`

**Bezug / Reference:**
- Issue: `[MODULE] stable_diffusion`
- Kontext: Modulweiser Reality-Check und Doku-Migration (Primary → Secondary)

---

# stable_diffusion — Reality-Check & Doku-Konsolidierung

## TL;DR

Der Abgleich gegen den Realstand ist durchgeführt. Die Secondary-Doku verankert jetzt nachvollziehbar:
- konkrete Abweichungen zwischen Primary-Doku und Implementierung,
- verifizierten ROADMAP/FUTURE-Status,
- Research-Hinweise inkl. Constraints,
- den Missing-Implementations-Report.

## Task 1 — Reality-Check gegen Sourcecode

| Thema | Primary-Claim | Beobachtung im Code | Evidence |
|---|---|---|---|
| Testumfang | README/ARCHITECTURE nannten ältere Teststände | `SDPluginFocusedTests` umfasst 51 Tests | `src/stable_diffusion/README.md`, `src/stable_diffusion/ARCHITECTURE.md`, `src/stable_diffusion/tests/test_sd_plugin.cpp`, `tests/CMakeLists.txt` |
| PNG-Encoder | Ältere Doku beschrieb Stub-PNG ohne IDAT | `encodeMinimalPng()` schreibt IHDR+IDAT+IEND mit CRC/Adler | `src/stable_diffusion/ARCHITECTURE.md`, `src/stable_diffusion/sd_plugin.cpp` |
| SDCppGenerator-Status | FUTURE/AUDIT enthielten alte "noch nicht implementiert"-Hinweise | `SDCppGenerator` ist im Header implementiert und per CMake-Flag integriert | `src/stable_diffusion/FUTURE_ENHANCEMENTS.md`, `src/stable_diffusion/AUDIT.md`, `include/stable_diffusion/sd_generator.h`, `src/stable_diffusion/CMakeLists.txt` |

## Task 2 — ROADMAP/FUTURE_ENHANCEMENTS-Verifikation

| Bereich | Ergebnis |
|---|---|
| ROADMAP-Phasen | Phase 1–4 und große Teile von Phase 5 sind implementiert und im Code nachweisbar |
| Offene ROADMAP-Punkte | Benchmark "time-to-PNG", SDCppGenerator-Parallel-Audit, E2E-Test mit realem Modell |
| FUTURE_ENHANCEMENTS-Qualität | Auf noch offene, umsetzbare Arbeitspakete fokussiert; bereits umgesetzte Punkte bereinigt |

## Task 3 — Research-Hinweise / Constraints

- `THEMIS_ENABLE_STABLE_DIFFUSION=ON` aktiviert den echten Backend-Pfad nur bei vorhandener Library.
- `SDPlugin` erzwingt Policy-Checks für `prompt` und `negative_prompt` vor jedem Generate-Pfad.
- `SDCppGenerator`-Parallelverhalten hängt teilweise vom Downstream (`stable-diffusion.cpp`) ab; daher eigener Audit-Task verbleibt.

## Verknüpfte Secondary-Dokumente

- [PRIMARY_SOURCES.md](./PRIMARY_SOURCES.md)
- [missing-implementations.md](./missing-implementations.md)
