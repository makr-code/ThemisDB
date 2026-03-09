# Observability-Modul — Missing Implementations

<!-- Status: current | validated: 2026-03-09 -->
<!-- Primärdokumentation: ../../../src/observability/ -->

Dieser Report dokumentiert Funktionen, die in `src/observability/ROADMAP.md` oder anderen
Primary-Docs als implementiert oder abgeschlossen beschrieben werden, jedoch beim
Reality-Check als **nicht vollständig umgesetzt** oder **fehlerhaft dokumentiert** befunden wurden.

Prüfstand: 2026-03-09 | Branch: `develop`

---

## 1. `tracer.cpp` und `log_aggregator.cpp` existieren nicht

| Feld | Wert |
|---|---|
| **ID** | OBS-MISSING-001 |
| **Claim-Quelle** | `src/observability/README.md` §"Relevant Interfaces" |
| **Erwartet** | `tracer.cpp` (OpenTelemetry Span-Management) und `log_aggregator.cpp` (Structured Log Collection) als eigenständige Implementierungsdateien |
| **Beobachtet** | Weder `src/observability/tracer.cpp` noch `src/observability/log_aggregator.cpp` existieren. Distributed Tracing wird in `continuous_profiler.cpp` umgesetzt; strukturiertes Logging läuft über das Core `ILogger`-Interface. |
| **Evidence** | `ls src/observability/` liefert: `alertmanager.cpp`, `continuous_profiler.cpp`, `distributed_flame_graph.cpp`, `ebpf_tracer.cpp`, `metrics_collector.cpp`, `performance_analyzer.cpp`, `query_profiler.cpp`, `storage_profiler.cpp` — kein `tracer.cpp`, kein `log_aggregator.cpp` |
| **Fix (Dokumentation)** | `README.md` korrigiert (2026-03-09): stale Einträge entfernt, korrekte Dateien ergänzt, Hinweis hinzugefügt |
| **Kritikalität** | Niedrig (nur Dokumentationsfehler; Funktionalität ist vorhanden) |
| **Issue-Titelvorschlag** | `[docs] observability/README.md: remove stale tracer.cpp / log_aggregator.cpp references` |
| **Label-Vorschläge** | `type:docs`, `priority:low`, `observability` |

---

## 2. `query_profiler.cpp`, `storage_profiler.cpp`, `performance_analyzer.cpp` fehlten in CMakeLists

| Feld | Wert |
|---|---|
| **ID** | OBS-MISSING-002 |
| **Claim-Quelle** | `src/observability/ROADMAP.md` §"Completed ✅" (QueryProfiler, StorageProfiler) |
| **Erwartet** | `query_profiler.cpp`, `storage_profiler.cpp`, `performance_analyzer.cpp` werden in `themis_core` kompiliert und sind für `test_observability_profilers.cpp` linkbar |
| **Beobachtet** | Alle drei Dateien existieren in `src/observability/` (je 0 Stubs), werden aber **nicht** in `cmake/CMakeLists.txt` unter der Observability-Sektion aufgelistet. `test_observability_profilers.cpp` (280 Zeilen) linkt gegen `themis_core` — ohne die drei `.cpp`-Dateien würde das Linken fehlschlagen. |
| **Evidence** | `cmake/CMakeLists.txt` Zeile 2009–2015 (Observability-Sektion vor Fix); `tests/test_observability_profilers.cpp` existiert und ist in `ALL_TEST_SOURCES` via GLOB_RECURSE |
| **Status** | ✅ **Behoben** am 2026-03-09: drei Einträge zu Observability-Sektion in `cmake/CMakeLists.txt` hinzugefügt |
| **Kritikalität** | Hoch — Linker-Fehler bei `test_observability_profilers.cpp` ohne Fix |
| **Fix-Branch** | `copilot/update-documentation-sync` (2026-03-09) |

---

## 3. eBPF-Tracer und Distributed Flame Graph als nicht implementiert markiert

| Feld | Wert |
|---|---|
| **ID** | OBS-MISSING-003 |
| **Claim-Quelle** | `src/observability/ROADMAP.md` §"Long-term (6-12 months)": eBPF `[P]` (PR in progress, #2055); Distributed Flame Graph `[I]` (#2108) |
| **Erwartet** | Beide Features laut ROADMAP noch nicht abgeschlossen |
| **Beobachtet** | `src/observability/ebpf_tracer.cpp` (526 Zeilen, 0 Stubs) und `src/observability/distributed_flame_graph.cpp` (0 Stubs) sind vollständig implementiert. Unit-Tests existieren in `tests/test_ebpf_tracer.cpp` und `tests/test_distributed_flame_graph.cpp`. File-Header beider Dateien meldet "Maturity: 🟢 PRODUCTION-READY". |
| **Evidence** | Sourcecode in `src/observability/`; File-Header-Metriken: TODOs: 0, Stubs: 0 |
| **ROADMAP-Status** | Fälschlicherweise `[P]`/`[I]` — korrigiert auf `[x]` im ROADMAP-Update 2026-03-09 |
| **Kritikalität** | Mittel (falsche ROADMAP-Darstellung, kein Build-Fehler) |
| **Issue-Titelvorschlag** | n/a (nur ROADMAP-Korrektur nötig) |

---

## 4. OTLP-Export nicht implementiert

| Feld | Wert |
|---|---|
| **ID** | OBS-MISSING-004 |
| **Claim-Quelle** | `src/observability/ROADMAP.md` §Phase 2: "OpenTelemetry SDK direct export via OTLP gRPC/HTTP (`observability/otlp_exporter.cpp`, Target: Q2 2026)" |
| **Erwartet** | `src/observability/otlp_exporter.cpp` (OTLP gRPC/HTTP Export an Jaeger/Tempo/Collector) |
| **Beobachtet** | Datei existiert **nicht**; ROADMAP markiert diese als `[?]` (blocked) — korrekt |
| **Evidence** | `ls src/observability/otlp_exporter.cpp` → "No such file" |
| **ROADMAP-Status** | `[?]` — korrekt (Open/Blocked) |
| **Kritikalität** | Mittel — Traces werden nur intern propagiert; kein direkter OTLP-Export möglich |
| **Issue-Titelvorschlag** | `[observability] Implement OTLP gRPC/HTTP exporter (otlp_exporter.cpp)` |
| **Label-Vorschläge** | `type:feature`, `priority:medium`, `observability`, `status:open` |

---

## Zusammenfassung

| ID | Titel | Kritikalität | Status |
|---|---|---|---|
| OBS-MISSING-001 | `tracer.cpp` / `log_aggregator.cpp` in README referenziert, existieren nicht | Niedrig | ✅ Doku korrigiert (2026-03-09) |
| OBS-MISSING-002 | 3 `.cpp`-Dateien fehlten in CMakeLists | Hoch | ✅ Behoben (2026-03-09) |
| OBS-MISSING-003 | eBPF-Tracer + Flame Graph als "planned" markiert, sind fertig | Mittel | ✅ ROADMAP korrigiert (2026-03-09) |
| OBS-MISSING-004 | OTLP-Export (`otlp_exporter.cpp`) fehlt | Mittel | 🔴 Offen |
