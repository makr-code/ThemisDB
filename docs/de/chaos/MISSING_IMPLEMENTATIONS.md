[docs](../../README.md) > [de](../README.md) > [chaos](./index.md) > [missing-implementations](./MISSING_IMPLEMENTATIONS.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/chaos/ROADMAP.md`
- `include/chaos/ROADMAP.md`
- `src/chaos/FUTURE_ENHANCEMENTS.md`
- `include/chaos/FUTURE_ENHANCEMENTS.md`
- `src/chaos/chaos_framework.cpp`
- `include/chaos/chaos_framework.h`

**Bezug / Reference:**
- Issue: #4685
- Kontext: Missing-Implementations-Report für den Reality-Check des Moduls `chaos`.

---

# Chaos-Modul — Missing Implementations Report

## Zusammenfassung

Die Kernimplementierung ist vorhanden und testbar.  
Offene Punkte betreffen vor allem **geplante Erweiterungen** (verteilt/ABI), nicht die bestehende in-process Kernfunktionalität.

## Befund 1 – Cluster-weite verteilte Chaos-Koordination fehlt noch

| Feld | Inhalt |
|---|---|
| **Priorität** | Hoch |
| **Claim-Quelle** | `src/chaos/ROADMAP.md` → In Progress |
| **Claim** | `[ ] Cluster-wide distributed chaos coordination (Target: Q3 2026)` |
| **Erwartet** | Verteilte Koordination/Broadcast/Replay für Multi-Node-Szenarien zusätzlich zur lokalen Injection |
| **Beobachtet** | In `src/chaos/` existiert nur `chaos_framework.cpp` mit in-process `FaultInjector` + `ChaosScheduler`; keine verteilte Koordinationskomponente |
| **Impact** | Multi-Node-Orchestrierung muss außerhalb des Moduls gebaut werden; eingeschränkte E2E-Chaos-Automation |
| **Evidence** | Geprüfte Pfade: `src/chaos/chaos_framework.cpp`, `include/chaos/chaos_framework.h`, `src/chaos/ROADMAP.md` |
| **Issue-Titelvorschlag** | `feat(chaos): add distributed chaos coordination layer for multi-node scenarios` |
| **Label-Vorschläge** | `enhancement`, `chaos`, `distributed-systems`, `priority:high` |

---

## Befund 2 – ABI-Kompatibilitätsmatrix für Include-API offen

| Feld | Inhalt |
|---|---|
| **Priorität** | Mittel |
| **Claim-Quelle** | `include/chaos/ROADMAP.md` → Production Readiness Checklist |
| **Claim** | `[ ] ABI compatibility test matrix complete` |
| **Erwartet** | Nachweis stabiler ABI-/Kompatibilitätsprüfung für externe Consumer |
| **Beobachtet** | Funktions- und Stresstests vorhanden (`tests/test_chaos_framework.cpp`, `tests/test_chaos_scheduler.cpp`, `tests/test_chaos_stress.cpp`), aber keine dedizierte ABI-Matrix-Dokumentation |
| **Impact** | Potenzielles Risiko bei Integrationen mit extern kompilierten Consumers/Toolchains |
| **Evidence** | Geprüfte Pfade: `include/chaos/ROADMAP.md`, `tests/test_chaos_framework.cpp`, `tests/test_chaos_scheduler.cpp`, `tests/test_chaos_stress.cpp` |
| **Issue-Titelvorschlag** | `chore(chaos): define and run ABI compatibility matrix for public chaos headers` |
| **Label-Vorschläge** | `documentation`, `testing`, `chaos`, `priority:medium` |

---

## Verifizierte bereits umgesetzte Punkte (kein Gap)

- Scheduler-Tick/Wake-Strategie ist implementiert (`WakeStrategy`, `ChaosSchedulerConfig` in `include/chaos/chaos_framework.h`; Logik in `src/chaos/chaos_framework.cpp`).
- Phase-4-Testevidenz ist vorhanden (`tests/test_chaos_framework.cpp`, `tests/test_chaos_scheduler.cpp`, `tests/test_chaos_stress.cpp`).
- Stress-Benchmark ist vorhanden (`benchmarks/bench_chaos_stress.cpp`).
