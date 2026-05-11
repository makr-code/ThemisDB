[docs](../../README.md) > [de](../README.md) > [chaos](./index.md) > [README](./README.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/chaos/README.md`
- `src/chaos/ARCHITECTURE.md`
- `src/chaos/ROADMAP.md`
- `src/chaos/FUTURE_ENHANCEMENTS.md`
- `include/chaos/README.md`

**Bezug / Reference:**
- Issue: #4685
- Kontext: Secondary-Doku für den modulweisen Reality-Check und die Doku-Migration des Moduls `chaos`.

---

# Chaos-Modul — Überblick und Verifikationsstand

## TL;DR

Das Modul `chaos` ist als **in-process Chaos-/Fault-Injection-Komponente** implementiert.
Die Kernklassen `FaultInjector` und `ChaosScheduler` sind produktiv vorhanden und durch Unit-/Stress-Tests sowie Benchmarks abgedeckt.

## Scope (Reality-Check)

- **Implementierung:** `src/chaos/chaos_framework.cpp`
- **Public API:** `include/chaos/chaos_framework.h`
- **Tests:** `tests/test_chaos_framework.cpp`, `tests/test_chaos_scheduler.cpp`, `tests/test_chaos_stress.cpp`
- **Benchmark:** `benchmarks/bench_chaos_stress.cpp`

## Verifizierte Aussagen (Task 1 + 2)

1. **Primary-Doku ↔ Code ist für Kernfunktionalität konsistent**
   - Fault-Typen, Fault-Lifecycle und Scheduler-Verhalten sind in `README`/`ARCHITECTURE` mit dem Code konsistent.
2. **ROADMAP-Status wurde gegen Evidenz geprüft**
   - `src/chaos/ROADMAP.md` bildet den aktuellen Umsetzungsstand inkl. Test-/Benchmark-Evidenz ab.
3. **FUTURE_ENHANCEMENTS ist implementierbar formuliert**
   - Sektionen `Scope`, `Design Constraints`, `Required Interfaces`, `Test Strategy`, `Performance Targets`, `Security / Reliability` sind vorhanden und konkret.
4. **Public Include Entry-Points dokumentiert**
   - `include/chaos/README.md` beschreibt den öffentlichen Header und die exportierten Typen/Klassen.

## Offene Punkte / bekannte Lücken

- Cluster-weite, verteilte Chaos-Koordination ist weiterhin offen (`src/chaos/ROADMAP.md`, In Progress).

## Installation

Build über den regulären ThemisDB-Flow:

```bash
cmake --preset linux-ninja-release
cmake --build --preset linux-ninja-release
```

## Usage

Relevante Einstiege:

- Source-Moduldoku: `src/chaos/README.md`
- Public-Header-Doku: `include/chaos/README.md`
- Lückenbericht: `docs/de/chaos/MISSING_IMPLEMENTATIONS.md`

Details und Priorisierung sind im Report dokumentiert:
- [`MISSING_IMPLEMENTATIONS.md`](./MISSING_IMPLEMENTATIONS.md)
