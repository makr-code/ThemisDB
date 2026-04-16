[docs](../../README.md) > [de](../README.md) > [chaos](./index.md) > [README](./README.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/chaos/README.md`
- `src/chaos/ARCHITECTURE.md`
- `src/chaos/ROADMAP.md`
- `src/chaos/FUTURE_ENHANCEMENTS.md`
- `include/chaos/README.md`
- `include/chaos/ROADMAP.md`

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
   - `include/chaos/ROADMAP.md` Phase 4 wurde auf `[x]` aktualisiert (konforme Test-Evidenz vorhanden).
3. **FUTURE_ENHANCEMENTS ist implementierbar formuliert**
   - Sektionen `Scope`, `Design Constraints`, `Required Interfaces`, `Test Strategy`, `Performance Targets`, `Security / Reliability` sind vorhanden und konkret.

## Offene Punkte / bekannte Lücken

- Cluster-weite, verteilte Chaos-Koordination ist weiterhin offen (`src/chaos/ROADMAP.md`, In Progress).
- ABI-Kompatibilitätsmatrix für externe Include-Consumer ist weiterhin offen (`include/chaos/ROADMAP.md`, Production Readiness Checklist).

Details und Priorisierung sind im Report dokumentiert:
- [`missing-implementations.md`](./missing-implementations.md)
