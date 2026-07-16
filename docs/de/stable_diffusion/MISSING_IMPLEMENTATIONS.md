[docs](../INDEX.md) > [de](../INDEX.md) > [stable_diffusion](./index.md) > [missing-implementations](./MISSING_IMPLEMENTATIONS.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/stable_diffusion/ROADMAP.md`
- `src/stable_diffusion/SECURITY.md`
- `src/stable_diffusion/sd_plugin.cpp`
- `include/stable_diffusion/sd_generator.h`
- `src/stable_diffusion/tests/test_sd_plugin.cpp`

**Bezug / Reference:**
- Issue: `[MODULE] stable_diffusion`
- Kontext: Fehlende Implementierungen aus Primary-Roadmap/Security gegen Realstand dokumentieren

---

# stable_diffusion — Missing Implementations Report

Prüfstand: 2026-04-16 (`/home/runner/work/ThemisDB/ThemisDB`)

## Offene Lücken mit Impact, Evidence, Priorisierung und Folge-Issues

| ID | Lücke | Impact | Evidence | Priorität | Folge-Issue-Vorschlag |
|---|---|---|---|---|---|
| SD-MISS-001 | Kein Benchmark "time-to-PNG" (512×512, Stub vs Realmodell) | Performance-Aussagen für Release/Regressionen nicht belastbar | `src/stable_diffusion/ROADMAP.md` (Phase 5 offen), kein passender Benchmark-Treffer in `benchmarks/` | Hoch | `[stable_diffusion] Add 512x512 time-to-PNG benchmark (stub vs SDCppGenerator)` |
| SD-MISS-002 | Kein dokumentierter Parallel-Audit für `SDCppGenerator` | Unklarheit zu Nebenläufigkeit unter Last bei echtem Backend | `src/stable_diffusion/ROADMAP.md` (Phase 5 + Production Checklist offen), `include/stable_diffusion/sd_generator.h` (`SDCppGenerator` ohne separaten Concurrency-Guard) | Hoch | `[stable_diffusion] Audit SDCppGenerator parallel-call safety and document guarantees` |
| SD-MISS-003 | Kein E2E-Test mit realem Modellfile | Integrationsrisiko zwischen CMake-Flag, Model-Load und Inferenzpfad bleibt bestehen | `src/stable_diffusion/ROADMAP.md` (Production Readiness offen), Tests nur in `src/stable_diffusion/tests/test_sd_plugin.cpp` (stub/focused) | Mittel | `[stable_diffusion] Add gated end-to-end test with real GGUF model fixture` |
| SD-MISS-004 | Modell-Integritätsprüfung (SHA-256) nicht umgesetzt | Security-Gap bei manipulierten Modellartefakten in sensitiven Deployments | `src/stable_diffusion/SECURITY.md` (Model integrity planned), kein Hash-Check in `src/stable_diffusion/sd_plugin.cpp`/`include/stable_diffusion/sd_generator.h` | Mittel | `[stable_diffusion] Implement model file integrity verification before backend init` |

## Hinweise

- Nicht als "Missing Implementation" gewertet: `generateBatch`, `generateImg2Img`, realer PNG-IDAT-Encoder und `SDCppGenerator` (im Code vorhanden).
- Dokumentationsdrift ist separat in `docs/de/stable_diffusion/index.md` festgehalten.
